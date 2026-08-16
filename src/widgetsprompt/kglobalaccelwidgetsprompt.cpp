/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalaccelpromptplugin_p.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QWidget>

/*
 * Asks with a message box. The strings keep the KGlobalAccel context they were written in, so the
 * translations of the library still apply to them.
 */
class KGlobalAccelWidgetsPrompt : public QObject, public KGlobalAccelPromptPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KGlobalAccelPromptPluginInterfaceId)
    Q_INTERFACES(KGlobalAccelPromptPlugin)

public:
    bool promptStealShortcut(QWidget *parent, const QList<KGlobalShortcutInfo> &shortcuts, const QKeySequence &seq) override
    {
        const QString component = shortcuts[0].componentFriendlyName();

        QString message;
        if (shortcuts.size() == 1) {
            message = QCoreApplication::translate("KGlobalAccel", "The '%1' key combination is registered by application %2 for action %3.")
                          .arg(seq.toString(), component, shortcuts[0].friendlyName());
        } else {
            QString actionList;
            for (const KGlobalShortcutInfo &info : shortcuts) {
                actionList +=
                    QCoreApplication::translate("KGlobalAccel", "In context '%1' for action '%2'\n").arg(info.contextFriendlyName(), info.friendlyName());
            }
            message = QCoreApplication::translate("KGlobalAccel", "The '%1' key combination is registered by application %2.\n%3")
                          .arg(seq.toString(), component, actionList);
        }

        QMessageBox box(parent);
        box.setWindowTitle(QCoreApplication::translate("KGlobalAccel", "Conflict With Registered Global Shortcut"));
        box.setText(message);
        box.addButton(QMessageBox::Ok)->setText(QCoreApplication::translate("KGlobalAccel", "Reassign"));
        box.addButton(QMessageBox::Cancel);

        return box.exec() == QMessageBox::Ok;
    }
};

#include "kglobalaccelwidgetsprompt.moc"
