/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kserviceactioncomponent.h"
#include "globalshortcutcontext.h"
#include "logging_p.h"

#include <QDBusConnectionInterface>
#include <QProcess>

#include <KShell>

namespace KdeDGlobalAccel
{
KServiceActionComponent::KServiceActionComponent(const QString &serviceStorageId, const QString &friendlyName, GlobalShortcutsRegistry *registry)
    : Component(serviceStorageId, friendlyName, registry)
    , m_serviceStorageId(serviceStorageId)
    , m_desktopFile(nullptr)
{
    auto fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kglobalaccel/") + serviceStorageId);
    if (fileName.isEmpty()) {
        // Fallback to applications data dir
        // for custom shortcut for instance
        fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("applications/") + serviceStorageId);
    }
    if (fileName.isEmpty()) {
        qCWarning(KGLOBALACCELD) << "No desktop file found for service " << serviceStorageId;
    }
    m_desktopFile.reset(new KDesktopFile(fileName));
}

KServiceActionComponent::~KServiceActionComponent()
{
}

void runProcess(const KConfigGroup &group, bool klauncherAvailable)
{
    QStringList parts = KShell::splitArgs(group.readEntry(QStringLiteral("Exec"), QString()));
    if (parts.isEmpty()) {
        return;
    }
    // sometimes entries have an %u for command line parameters
    if (parts.last().contains(QChar('%'))) {
        parts.pop_back();
    }

    const QString command = parts.takeFirst();

    const auto kstart = QStandardPaths::findExecutable(QStringLiteral("kstart5"));
    if (!kstart.isEmpty()) {
        parts.prepend(command);
        parts.prepend(QStringLiteral("--"));
        QProcess::startDetached(kstart, parts);
    } else if (klauncherAvailable) {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.klauncher5"),
                                                          QStringLiteral("/KLauncher"),
                                                          QStringLiteral("org.kde.KLauncher"),
                                                          QStringLiteral("exec_blind"));
        msg << command << parts;

        QDBusConnection::sessionBus().asyncCall(msg);
    } else {
        QProcess::startDetached(command, parts);
    }
}

void KServiceActionComponent::emitGlobalShortcutPressed(const GlobalShortcut &shortcut)
{
    // TODO KF6 use ApplicationLauncherJob to start processes when it's available in a framework that we depend on

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
        message << QVariantMap();
        QDBusConnection::sessionBus().asyncCall(message);
        return;
    }

    QDBusConnectionInterface *dbusDaemon = QDBusConnection::sessionBus().interface();
    const bool klauncherAvailable = dbusDaemon->isServiceRegistered(QStringLiteral("org.kde.klauncher5"));

    // we can't use KRun there as it depends from KIO and would create a circular dep
    if (shortcut.uniqueName() == QLatin1String("_launch")) {
        runProcess(m_desktopFile->desktopGroup(), klauncherAvailable);
        return;
    }
    const auto lstActions = m_desktopFile->readActions();
    for (const QString &action : lstActions) {
        if (action == shortcut.uniqueName()) {
            runProcess(m_desktopFile->actionGroup(action), klauncherAvailable);
            return;
        }
    }
}

void KServiceActionComponent::loadFromService()
{
    QString shortcutString;

    QStringList shortcuts = m_desktopFile->desktopGroup().readEntry(QStringLiteral("X-KDE-Shortcuts"), QString()).split(QChar(','));
    if (!shortcuts.isEmpty()) {
        shortcutString = shortcuts.join(QChar('\t'));
    }

    GlobalShortcut *shortcut = registerShortcut(QStringLiteral("_launch"), m_desktopFile->readName(), shortcutString, shortcutString);
    shortcut->setIsPresent(true);
    const auto lstActions = m_desktopFile->readActions();
    for (const QString &action : lstActions) {
        shortcuts = m_desktopFile->actionGroup(action).readEntry(QStringLiteral("X-KDE-Shortcuts"), QString()).split(QChar(','));
        if (!shortcuts.isEmpty()) {
            shortcutString = shortcuts.join(QChar('\t'));
        }

        GlobalShortcut *shortcut =
            registerShortcut(action, m_desktopFile->actionGroup(action).readEntry(QStringLiteral("Name")), shortcutString, shortcutString);
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

    m_desktopFile->desktopGroup().writeEntry("NoDisplay", true);
    m_desktopFile->desktopGroup().sync();

    return Component::cleanUp();
}

} // namespace KdeDGlobalAccel

#include "moc_kserviceactioncomponent.cpp"
