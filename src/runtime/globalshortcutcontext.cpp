/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "globalshortcutcontext.h"

#include "globalshortcut.h"


GlobalShortcutContext::GlobalShortcutContext(
        const QString &uniqueName,
        const QString &friendlyName,
        KdeDGlobalAccel::Component *component)

        :   _uniqueName(uniqueName),
            _friendlyName(friendlyName),
            _component(component),
            _actions()
    {}


GlobalShortcutContext::~GlobalShortcutContext()
    {
    qDeleteAll(_actions); _actions.clear();
    }


void GlobalShortcutContext::addShortcut(GlobalShortcut *shortcut)
    {
    _actions.insert(shortcut->uniqueName(), shortcut);
    }


QList<KGlobalShortcutInfo> GlobalShortcutContext::allShortcutInfos() const
    {
    QList<KGlobalShortcutInfo> rc;
    for (GlobalShortcut *shortcut : qAsConst(_actions))
        {
        rc.append(static_cast<KGlobalShortcutInfo>(*shortcut));
        }
    return rc;
    }


KdeDGlobalAccel::Component const *GlobalShortcutContext::component() const
    {
    return _component;
    }


KdeDGlobalAccel::Component *GlobalShortcutContext::component()
    {
    return _component;
    }


QString GlobalShortcutContext::friendlyName() const
    {
    return _friendlyName;
    }


GlobalShortcut *GlobalShortcutContext::getShortcutByKey(int key) const
    {
    // Qt triggers both shortcuts that include Shift+Backtab and Shift+Tab
    // when user presses Shift+Tab. Do the same here.
    int keySym = key & ~Qt::KeyboardModifierMask;
    int keyMod = key & Qt::KeyboardModifierMask;
    if ((keyMod & Qt::SHIFT) && (keySym == Qt::Key_Backtab ||
        keySym == Qt::Key_Tab))
        {
        for (GlobalShortcut *sc : qAsConst(_actions))
            {
            if (sc->keys().contains(keyMod | Qt::Key_Tab) ||
                sc->keys().contains(keyMod | Qt::Key_Backtab))
                return sc;
            }
        }
    else
        {
        for (GlobalShortcut *sc : qAsConst(_actions))
            {
            if (sc->keys().contains(key)) return sc;
            }
        }
    return nullptr;
    }


GlobalShortcut *GlobalShortcutContext::takeShortcut(GlobalShortcut *shortcut)
    {
    // Try to take the shortcut. Result could be nullptr if the shortcut doesn't
    // belong to this component.
    return _actions.take(shortcut->uniqueName());
    }


QString GlobalShortcutContext::uniqueName() const
    {
    return _uniqueName;
    }
