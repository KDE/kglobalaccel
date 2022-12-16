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

#include <KShell>
#include <KWindowSystem>

#include "config-kglobalaccel.h"
#if HAVE_X11
#include <KStartupInfo>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif
#endif

KServiceActionComponent::KServiceActionComponent(const QString &serviceStorageId, const QString &friendlyName)
    : Component(serviceStorageId, friendlyName)
    , m_serviceStorageId(serviceStorageId)
{
    QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kglobalaccel/") + serviceStorageId);
    if (filePath.isEmpty()) {
        // Fallback to applications data dir for custom shortcut for instance
        filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("applications/") + serviceStorageId);
        m_isInApplicationsDir = true;
    } else {
        QFileInfo info(filePath);
        if (info.isSymLink()) {
            const QString filePath2 = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("applications/") + serviceStorageId);
            if (info.symLinkTarget() == filePath2) {
                filePath = filePath2;
                m_isInApplicationsDir = true;
            }
        }
    }

    if (filePath.isEmpty()) {
        qCWarning(KGLOBALACCELD) << "No desktop file found for service " << serviceStorageId;
    }
    m_desktopFile.reset(new KDesktopFile(filePath));
}

KServiceActionComponent::~KServiceActionComponent() = default;

void KServiceActionComponent::runProcess(const KConfigGroup &group, const QString &token)
{
    QStringList args = KShell::splitArgs(group.readEntry(QStringLiteral("Exec"), QString()));
    if (args.isEmpty()) {
        return;
    }
    // sometimes entries have an %u for command line parameters
    if (args.last().contains(QLatin1Char('%'))) {
        args.pop_back();
    }

    const QString command = args.takeFirst();

    auto startDetachedWithToken = [token](const QString &program, const QStringList &args) {
        QProcess p;
        p.setProgram(program);
        p.setArguments(args);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        if (!token.isEmpty()) {
            if (KWindowSystem::isPlatformWayland()) {
                env.insert(QStringLiteral("XDG_ACTIVATION_TOKEN"), token);
            } else {
                env.insert(QStringLiteral("DESKTOP_STARTUP_ID"), token);
            }
        }
        p.setProcessEnvironment(env);
        if (!p.startDetached()) {
            qCWarning(KGLOBALACCELD) << "Failed to start" << program;
        }
    };

    const auto kstart = QStandardPaths::findExecutable(QStringLiteral("kstart5"));
    if (!kstart.isEmpty()) {
        if (group.name() == QLatin1String("Desktop Entry") && m_isInApplicationsDir) {
            startDetachedWithToken(kstart, {QStringLiteral("--application"), QFileInfo(m_desktopFile->fileName()).completeBaseName()});
        } else {
            args.prepend(command);
            args.prepend(QStringLiteral("--"));
            startDetachedWithToken(kstart, args);
        }
        return;
    }

    QDBusConnectionInterface *dbusDaemon = QDBusConnection::sessionBus().interface();
    const bool klauncherAvailable = dbusDaemon->isServiceRegistered(QStringLiteral("org.kde.klauncher5"));
    if (klauncherAvailable) {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.klauncher5"),
                                                          QStringLiteral("/KLauncher"),
                                                          QStringLiteral("org.kde.KLauncher"),
                                                          QStringLiteral("exec_blind"));
        msg << command << args;

        QDBusConnection::sessionBus().asyncCall(msg);
        return;
    }

    const QString cmdExec = QStandardPaths::findExecutable(command);
    if (cmdExec.isEmpty()) {
        qCWarning(KGLOBALACCELD) << "Could not find executable in PATH" << command;
        return;
    }
    startDetachedWithToken(cmdExec, args);
}

void KServiceActionComponent::emitGlobalShortcutPressed(const GlobalShortcut &shortcut)
{
    // TODO KF6 use ApplicationLauncherJob to start processes when it's available in a framework that we depend on

    auto launchWithToken = [this, shortcut](const QString &token) {
        // DBusActivatatable spec as per https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#dbus
        if (m_desktopFile->desktopGroup().readEntry("DBusActivatable", false)) {
            QString method;
            const QString serviceName = m_serviceStorageId.chopped(strlen(".desktop"));
            const QString objectPath = QStringLiteral("/%1").arg(serviceName).replace(QLatin1Char('.'), QLatin1Char('/'));
            const QString interface = QStringLiteral("org.freedesktop.Application");
            QDBusMessage message;
            if (shortcut.uniqueName() == QLatin1String("_launch")) {
                message = QDBusMessage::createMethodCall(serviceName, objectPath, interface, QStringLiteral("Activate"));
            } else {
                message = QDBusMessage::createMethodCall(serviceName, objectPath, interface, QStringLiteral("ActivateAction"));
                message << shortcut.uniqueName() << QVariantList();
            }
            if (!token.isEmpty()) {
                if (KWindowSystem::isPlatformWayland()) {
                    message << QVariantMap{{QStringLiteral("activation-token"), token}};
                } else {
                    message << QVariantMap{{QStringLiteral("desktop-startup-id"), token}};
                }
            } else {
                message << QVariantMap();
            }

            QDBusConnection::sessionBus().asyncCall(message);
            return;
        }

        // we can't use KRun there as it depends from KIO and would create a circular dep
        if (shortcut.uniqueName() == QLatin1String("_launch")) {
            runProcess(m_desktopFile->desktopGroup(), token);
            return;
        }
        const auto lstActions = m_desktopFile->readActions();
        for (const QString &action : lstActions) {
            if (action == shortcut.uniqueName()) {
                runProcess(m_desktopFile->actionGroup(action), token);
                return;
            }
        }
    };
    if (KWindowSystem::isPlatformWayland()) {
        const QString serviceName = m_serviceStorageId.chopped(strlen(".desktop"));
        KWindowSystem::requestXdgActivationToken(nullptr, 0, serviceName);
        connect(KWindowSystem::self(), &KWindowSystem::xdgActivationTokenArrived, this, [this, launchWithToken](int tokenSerial, const QString &token) {
            if (tokenSerial == 0) {
                launchWithToken(token);
                bool b = disconnect(KWindowSystem::self(), &KWindowSystem::xdgActivationTokenArrived, this, nullptr);
                Q_ASSERT(b);
            }
        });
    } else {
#if HAVE_X11
        launchWithToken(QString::fromUtf8(KStartupInfo::createNewStartupIdForTimestamp(QX11Info::appTime())));
#endif
    }
}

void KServiceActionComponent::loadFromService()
{
    auto registerGroupShortcut = [this](const QString &name, const KConfigGroup &group) {
        const QString shortcutString = group.readEntry(QStringLiteral("X-KDE-Shortcuts"), QString()).replace(QLatin1Char(','), QLatin1Char('\t'));
        GlobalShortcut *shortcut = registerShortcut(name, group.readEntry(QStringLiteral("Name"), QString()), shortcutString, shortcutString);
        shortcut->setIsPresent(true);
    };

    registerGroupShortcut(QStringLiteral("_launch"), m_desktopFile->desktopGroup());
    const auto lstActions = m_desktopFile->readActions();
    for (const QString &action : lstActions) {
        registerGroupShortcut(action, m_desktopFile->actionGroup(action));
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
