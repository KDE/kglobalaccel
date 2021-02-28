/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalshortcutinfo.h"
#include "kglobalshortcutinfo_p.h"

QDBusArgument &operator<<(QDBusArgument &argument, const KGlobalShortcutInfo &shortcut)
{
    argument.beginStructure();
    argument << shortcut.uniqueName() << shortcut.friendlyName() << shortcut.componentUniqueName() << shortcut.componentFriendlyName()
             << shortcut.contextUniqueName() << shortcut.contextFriendlyName();
    argument.beginArray(qMetaTypeId<int>());
    for (const QKeySequence &key : shortcut.keys()) {
        argument << key[0];
    }
    argument.endArray();
    argument.beginArray(qMetaTypeId<int>());
    for (const QKeySequence &key : shortcut.defaultKeys()) {
        argument << key[0];
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutInfo &shortcut)
{
    argument.beginStructure();
    argument >> shortcut.d->uniqueName >> shortcut.d->friendlyName >> shortcut.d->componentUniqueName >> shortcut.d->componentFriendlyName
        >> shortcut.d->contextUniqueName >> shortcut.d->contextFriendlyName;
    argument.beginArray();
    while (!argument.atEnd()) {
        int key;
        argument >> key;
        shortcut.d->keys.append(QKeySequence(key));
    }
    argument.endArray();
    argument.beginArray();
    while (!argument.atEnd()) {
        int key;
        argument >> key;
        shortcut.d->defaultKeys.append(QKeySequence(key));
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}
