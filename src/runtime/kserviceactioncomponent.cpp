/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kserviceactioncomponent.h"
#include "logging_p.h"

#include <QDBusConnectionInterface>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include <KShell>
#include <KWindowSystem>

QString makeUniqueName(const KService::Ptr &service)
{
    if (service->storageId().startsWith(QLatin1Char('/'))) {
        return QFileInfo(service->storageId()).fileName();
    }

    return service->storageId();
}

KServiceActionComponent::KServiceActionComponent(KService::Ptr service)
    : Component(makeUniqueName(service), service->name())
    , m_service(service)
{
    m_isInApplicationsDir = !service->storageId().startsWith(QLatin1Char('/'));
}

KServiceActionComponent::~KServiceActionComponent() = default;

void KServiceActionComponent::startDetachedWithToken(const QString &program, const QStringList &args, const QString &token)
{
    QProcess p;
    p.setProgram(program);
    p.setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!token.isEmpty()) {
        env.insert(QStringLiteral("XDG_ACTIVATION_TOKEN"), token);
    }
    p.setProcessEnvironment(env);
    if (!p.startDetached()) {
        qCWarning(KGLOBALACCELD) << "Failed to start" << program;
    }
}

void KServiceActionComponent::runService(const QString &token)
{
    QStringList args = KShell::splitArgs(m_service->exec());
    if (args.isEmpty()) {
        return;
    }
    // sometimes entries have an %u for command line parameters
    if (args.last().contains(QLatin1Char('%'))) {
        args.pop_back();
    }

    const QString command = args.takeFirst();

    const auto kstart = QStandardPaths::findExecutable(QStringLiteral("kstart5"));
    if (!kstart.isEmpty()) {
        if (m_isInApplicationsDir) {
            startDetachedWithToken(kstart, {QStringLiteral("--application"), m_service->desktopEntryName()}, token);
        } else {
            args.prepend(command);
            args.prepend(QStringLiteral("--"));
            startDetachedWithToken(kstart, args, token);
        }
        return;
    }

    if (runWithKLauncher(command, args)) {
        return;
    }

    const QString cmdExec = QStandardPaths::findExecutable(command);
    if (cmdExec.isEmpty()) {
        qCWarning(KGLOBALACCELD) << "Could not find executable in PATH" << command;
        return;
    }
    startDetachedWithToken(cmdExec, args, token);
}

void KServiceActionComponent::runServiceAction(const KServiceAction &action, const QString &token)
{
    QStringList args = KShell::splitArgs(action.exec());
    if (args.isEmpty()) {
        return;
    }
    // sometimes entries have an %u for command line parameters
    if (args.last().contains(QLatin1Char('%'))) {
        args.pop_back();
    }

    const QString command = args.takeFirst();

    const auto kstart = QStandardPaths::findExecutable(QStringLiteral("kstart5"));
    if (!kstart.isEmpty()) {
        args.prepend(command);
        args.prepend(QStringLiteral("--"));
        startDetachedWithToken(kstart, args, token);
        return;
    }

    if (runWithKLauncher(command, args)) {
        return;
    }

    const QString cmdExec = QStandardPaths::findExecutable(command);
    if (cmdExec.isEmpty()) {
        qCWarning(KGLOBALACCELD) << "Could not find executable in PATH" << command;
        return;
    }
    startDetachedWithToken(cmdExec, args, token);
}

bool KServiceActionComponent::runWithKLauncher(const QString &command, QStringList &args)
{
    QDBusConnectionInterface *dbusDaemon = QDBusConnection::sessionBus().interface();
    const bool klauncherAvailable = dbusDaemon->isServiceRegistered(QStringLiteral("org.kde.klauncher5"));
    if (klauncherAvailable) {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.klauncher5"),
                                                          QStringLiteral("/KLauncher"),
                                                          QStringLiteral("org.kde.KLauncher"),
                                                          QStringLiteral("exec_blind"));
        msg << command << args;

        QDBusConnection::sessionBus().asyncCall(msg);
        return true;
    }
    return false;
}

void KServiceActionComponent::emitGlobalShortcutPressed(const GlobalShortcut &shortcut)
{
    // TODO KF6 use ApplicationLauncherJob to start processes when it's available in a framework that we depend on
    const QString desktopEntryName = m_service->desktopEntryName();

    auto launchWithToken = [this, shortcut, &desktopEntryName](const QString &token) {
        // DBusActivatatable spec as per https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#dbus
        if (m_service->property(QStringLiteral("DBusActivatable"), QMetaType::Bool).toBool()) {
            QString method;
            const QString objectPath = QStringLiteral("/%1").arg(desktopEntryName).replace(QLatin1Char('.'), QLatin1Char('/'));
            const QString interface = QStringLiteral("org.freedesktop.Application");
            QDBusMessage message;
            if (shortcut.uniqueName() == QLatin1String("_launch")) {
                message = QDBusMessage::createMethodCall(desktopEntryName, objectPath, interface, QStringLiteral("Activate"));
            } else {
                message = QDBusMessage::createMethodCall(desktopEntryName, objectPath, interface, QStringLiteral("ActivateAction"));
                message << shortcut.uniqueName() << QVariantList();
            }
            if (!token.isEmpty()) {
                message << QVariantMap{{QStringLiteral("activation-token"), token}};
            } else {
                message << QVariantMap();
            }

            QDBusConnection::sessionBus().asyncCall(message);
            return;
        }

        // we can't use KRun there as it depends from KIO and would create a circular dep
        if (shortcut.uniqueName() == QLatin1String("_launch")) {
            runService(token);
            return;
        }
        const auto lstActions = m_service->actions();
        for (const KServiceAction &action : lstActions) {
            if (action.name() == shortcut.uniqueName()) {
                runServiceAction(action, token);
                return;
            }
        }
    };
    if (KWindowSystem::isPlatformWayland()) {
        KWindowSystem::requestXdgActivationToken(nullptr, 0, desktopEntryName);
        connect(KWindowSystem::self(), &KWindowSystem::xdgActivationTokenArrived, this, [this, launchWithToken](int tokenSerial, const QString &token) {
            if (tokenSerial == 0) {
                launchWithToken(token);
                bool b = disconnect(KWindowSystem::self(), &KWindowSystem::xdgActivationTokenArrived, this, nullptr);
                Q_ASSERT(b);
            }
        });
    } else {
        launchWithToken({});
    }
}

void KServiceActionComponent::loadFromService()
{
    const QString shortcutString = m_service->property(QStringLiteral("X-KDE-Shortcuts")).toStringList().join(QLatin1Char('\t'));
    GlobalShortcut *shortcut = registerShortcut(QStringLiteral("_launch"), m_service->name(), shortcutString, shortcutString);
    shortcut->setIsPresent(true);

    const auto lstActions = m_service->actions();
    for (const KServiceAction &action : lstActions) {
        const QString shortcutString = action.property(QStringLiteral("X-KDE-Shortcuts"), QMetaType::QStringList).toStringList().join(QLatin1Char('\t'));
        GlobalShortcut *shortcut = registerShortcut(action.name(), action.text(), shortcutString, shortcutString);
        shortcut->setIsPresent(true);
    }
}

bool KServiceActionComponent::cleanUp()
{
    qCDebug(KGLOBALACCELD) << "Disabling desktop file";

    const auto shortcuts = allShortcuts();
    for (GlobalShortcut *shortcut : shortcuts) {
        shortcut->setIsPresent(false);
    }

    return Component::cleanUp();
}

#include "moc_kserviceactioncomponent.cpp"
