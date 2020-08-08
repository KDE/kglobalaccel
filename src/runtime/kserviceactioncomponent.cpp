/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kserviceactioncomponent.h"
#include "globalshortcutcontext.h"
#include "logging_p.h"

#include <QProcess>
#include <QDBusConnectionInterface>


namespace KdeDGlobalAccel {

KServiceActionComponent::KServiceActionComponent(
            const QString &serviceStorageId,
            const QString &friendlyName,
            GlobalShortcutsRegistry *registry)
    :   Component(serviceStorageId, friendlyName, registry),
        m_serviceStorageId(serviceStorageId),
        m_desktopFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kglobalaccel/") + serviceStorageId))
    {
    }


KServiceActionComponent::~KServiceActionComponent()
    {
    }

void KServiceActionComponent::emitGlobalShortcutPressed( const GlobalShortcut &shortcut )
{
    QDBusConnectionInterface *dbusDaemon = QDBusConnection::sessionBus().interface();
    const bool klauncherAvailable = dbusDaemon->isServiceRegistered(QStringLiteral("org.kde.klauncher5"));

    //we can't use KRun there as it depends from KIO and would create a circular dep
    if (shortcut.uniqueName() == QLatin1String("_launch")) {
        QStringList parts = m_desktopFile.desktopGroup().readEntry(QStringLiteral("Exec"), QString()).split(QChar(' '));
        if (parts.isEmpty()) {
            return;
        }
        const QString command = parts.first();
        //sometimes entries have an %u for command line parameters
        if (parts.last().contains(QChar('%'))) {
            parts.pop_back();
        }
        parts.pop_front();

        if (klauncherAvailable) {
            QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.klauncher5"),
                                                      QStringLiteral("/KLauncher"),
                                                      QStringLiteral("org.kde.KLauncher"),
                                                      QStringLiteral("exec_blind"));
            msg << command << parts;

            QDBusConnection::sessionBus().asyncCall(msg);
        } else {
            QProcess::startDetached(command, parts);
        }
        return;
    }
    const auto lstActions = m_desktopFile.readActions();
    for (const QString &action : lstActions) {
        if (action == shortcut.uniqueName()) {
            QStringList parts = m_desktopFile.actionGroup(action).readEntry(QStringLiteral("Exec"), QString()).split(QChar(' '));

            if (parts.isEmpty()) {
                return;
            }
            const QString command = parts.first();
            //sometimes entries have an %u for command line parameters
            if (parts.last().contains(QChar('%'))) {
                parts.pop_back();
            }
            parts.pop_front();

            if (klauncherAvailable) {
                QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.klauncher5"),
                                                        QStringLiteral("/KLauncher"),
                                                        QStringLiteral("org.kde.KLauncher"),
                                                        QStringLiteral("exec_blind"));
                msg << command << parts;

                QDBusConnection::sessionBus().asyncCall(msg);
            } else {
                QProcess::startDetached(command, parts);
            }
            return;
        }
    }
}



void KServiceActionComponent::loadFromService()
    {

    QString shortcutString;

    QStringList shortcuts = m_desktopFile.desktopGroup().readEntry(QStringLiteral("X-KDE-Shortcuts"), QString()).split(QChar(','));
    if (!shortcuts.isEmpty()) {
        shortcutString = shortcuts.join(QChar('\t'));
    }

    GlobalShortcut *shortcut = registerShortcut(QStringLiteral("_launch"), m_desktopFile.readName(), shortcutString, shortcutString);
    shortcut->setIsPresent(true);
    const auto lstActions = m_desktopFile.readActions();
    for (const QString &action : lstActions)
        {
        shortcuts = m_desktopFile.actionGroup(action).readEntry(QStringLiteral("X-KDE-Shortcuts"), QString()).split(QChar(','));
        if (!shortcuts.isEmpty())
            {
            shortcutString = shortcuts.join(QChar('\t'));
            }

        GlobalShortcut *shortcut = registerShortcut(action, m_desktopFile.actionGroup(action).readEntry(QStringLiteral("Name")), shortcutString, shortcutString);
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

    m_desktopFile.desktopGroup().writeEntry("NoDisplay", true);
    m_desktopFile.desktopGroup().sync();

    return Component::cleanUp();
}

} // namespace KdeDGlobalAccel

#include "moc_kserviceactioncomponent.cpp"
