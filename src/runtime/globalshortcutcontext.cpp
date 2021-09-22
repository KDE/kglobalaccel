/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "globalshortcutcontext.h"

#include "globalshortcut.h"

#include "kglobalaccel.h"
#include "sequencehelpers_p.h"

GlobalShortcutContext::GlobalShortcutContext(const QString &uniqueName, const QString &friendlyName, KdeDGlobalAccel::Component *component)

    : _uniqueName(uniqueName)
    , _friendlyName(friendlyName)
    , _component(component)
    , _actions()
{
}

GlobalShortcutContext::~GlobalShortcutContext()
{
    qDeleteAll(_actions);
    _actions.clear();
}

void GlobalShortcutContext::addShortcut(GlobalShortcut *shortcut)
{
    _actions.insert(shortcut->uniqueName(), shortcut);
}

QList<KGlobalShortcutInfo> GlobalShortcutContext::allShortcutInfos() const
{
    QList<KGlobalShortcutInfo> rc;
    for (GlobalShortcut *shortcut : std::as_const(_actions)) {
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

GlobalShortcut *GlobalShortcutContext::getShortcutByKey(const QKeySequence &key, KGlobalAccel::MatchType type) const
{
    if (key.isEmpty()) {
        return nullptr;
    }
    QKeySequence keyMangled = mangleKey(key);
    for (GlobalShortcut *sc : std::as_const(_actions)) {
        const auto keys = sc->keys();
        for (const QKeySequence &other : keys) {
            QKeySequence otherMangled = mangleKey(other);
            switch (type) {
            case KGlobalAccel::MatchType::Equal:
                if (otherMangled == keyMangled) {
                    return sc;
                }
                break;
            case KGlobalAccel::MatchType::Shadows:
                if (!other.isEmpty() && contains(keyMangled, otherMangled)) {
                    return sc;
                }
                break;
            case KGlobalAccel::MatchType::Shadowed:
                if (!other.isEmpty() && contains(otherMangled, keyMangled)) {
                    return sc;
                }
                break;
            }
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
