/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KGLOBALACCELPROMPTPLUGIN_P_H
#define KGLOBALACCELPROMPTPLUGIN_P_H

#include "kglobalshortcutinfo.h"

#include <QKeySequence>
#include <QList>
#include <QtPlugin>

class QWidget;

/*
 * Asks the user whether a global shortcut may be taken away from what holds it.
 *
 * The dialog this takes is the one thing in KGlobalAccel that needs a widget toolkit, so it lives
 * in a plugin which is loaded the first time a question is asked. A service that only registers
 * shortcuts never asks one and never loads it.
 */
class KGlobalAccelPromptPlugin
{
public:
    virtual ~KGlobalAccelPromptPlugin() = default;

    /*
     * Returns true when the user agrees to reassign \a seq, which the actions in \a shortcuts hold.
     * \a parent is the widget the question belongs to and may be a nullptr.
     */
    virtual bool promptStealShortcut(QWidget *parent, const QList<KGlobalShortcutInfo> &shortcuts, const QKeySequence &seq) = 0;
};

#define KGlobalAccelPromptPluginInterfaceId "org.kde.KGlobalAccelPromptPlugin/6.0"
Q_DECLARE_INTERFACE(KGlobalAccelPromptPlugin, KGlobalAccelPromptPluginInterfaceId)

#endif // KGLOBALACCELPROMPTPLUGIN_P_H
