/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalshortcutinfo.h"
#include "kglobalshortcutinfo_p.h"

QDBusArgument &operator<<(QDBusArgument &argument, const QKeySequence &sequence)
{
    argument.beginStructure();
    argument.beginArray(qMetaTypeId<int>());
    for (int i = 0; i < maxSequenceLength; i++) {
        argument << (i < sequence.count() ? sequence[i].toCombined() : 0);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QKeySequence &sequence)
{
    int s1;
    int s2;
    int s3;
    int s4;
    argument.beginStructure();
    argument.beginArray();
    argument >> s1 >> s2 >> s3 >> s4;
    sequence = QKeySequence(s1, s2, s3, s4);
    argument.endArray();
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const KGlobalShortcutInfo &shortcut)
{
    qDebug() << "New marshalling";
    argument.beginStructure();
    /* clang-format off */
    argument << shortcut.uniqueName()
             << shortcut.friendlyName()
             << shortcut.componentUniqueName()
             << shortcut.componentFriendlyName()
             << shortcut.contextUniqueName()
             << shortcut.contextFriendlyName();
    /* clang-format on */
    argument << shortcut.keys();
    argument << shortcut.defaultKeys();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutInfo &shortcut)
{
    argument.beginStructure();
    /* clang-format off */
    argument >> shortcut.d->uniqueName
             >> shortcut.d->friendlyName
             >> shortcut.d->componentUniqueName
             >> shortcut.d->componentFriendlyName
             >> shortcut.d->contextUniqueName
             >> shortcut.d->contextFriendlyName;
    /* clang-format on */

    argument.beginArray();
    qDebug() << argument.currentType();
    // In the past KGlobalShortcutInfo QKeySequences were only marshalled as a single int, for backwards
    // compat and ease of code we make the demarshalling dynamic because the deprecated methods also need
    // to be kept around
    const bool usesOldMarshalling = argument.currentType() == QDBusArgument::BasicType;
    qDebug() << "using old marshalling" << usesOldMarshalling;
    while (!argument.atEnd()) {
        if (usesOldMarshalling) {
            int key;
            argument >> key;
            shortcut.d->keys.append(QKeySequence(key));
        } else {
            QKeySequence key;
            argument >> key;
            shortcut.d->keys.append(key);
        }
    }
    argument.endArray();
    argument.beginArray();
    while (!argument.atEnd()) {
        if (usesOldMarshalling) {
            int key;
            argument >> key;
            shortcut.d->defaultKeys.append(QKeySequence(key));
        } else {
            QKeySequence key;
            argument >> key;
            shortcut.d->defaultKeys.append(key);
        }
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}
