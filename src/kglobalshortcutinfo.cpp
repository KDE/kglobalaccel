/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalshortcutinfo.h"
#include "kglobalshortcutinfo_p.h"

KGlobalShortcutInfo::KGlobalShortcutInfo()
    : d(new KGlobalShortcutInfoPrivate)
{
}

KGlobalShortcutInfo::KGlobalShortcutInfo(const KGlobalShortcutInfo &rhs)
    : QObject()
    , d(new KGlobalShortcutInfoPrivate)
{
    d->contextUniqueName = rhs.d->contextUniqueName;
    d->contextFriendlyName = rhs.d->contextFriendlyName;
    d->componentFriendlyName = rhs.d->componentFriendlyName;
    d->componentUniqueName = rhs.d->componentUniqueName;
    d->friendlyName = rhs.d->friendlyName;
    d->uniqueName = rhs.d->uniqueName;
    d->keys = rhs.d->keys;
    d->defaultKeys = rhs.d->defaultKeys;
    d->triggers = rhs.d->triggers;
    d->defaultTriggers = rhs.d->defaultTriggers;
}

KGlobalShortcutInfo::KGlobalShortcutInfo(KGlobalShortcutInfo &&rhs)
    : QObject()
    , d(rhs.d)
{
    rhs.d = nullptr;
}

KGlobalShortcutInfo::~KGlobalShortcutInfo()
{
    delete d;
}

KGlobalShortcutInfo &KGlobalShortcutInfo::operator=(const KGlobalShortcutInfo &rhs)
{
    KGlobalShortcutInfo tmp(rhs);
    KGlobalShortcutInfoPrivate *swap;
    swap = d;
    d = tmp.d;
    tmp.d = swap;
    return *this;
}

KGlobalShortcutInfo &KGlobalShortcutInfo::operator=(KGlobalShortcutInfo &&rhs)
{
    delete d;
    d = rhs.d;
    rhs.d = nullptr;
    return *this;
}

QString KGlobalShortcutInfo::contextFriendlyName() const
{
    return d->contextFriendlyName.isEmpty() ? d->contextUniqueName : d->contextFriendlyName;
}

QString KGlobalShortcutInfo::contextUniqueName() const
{
    return d->contextUniqueName;
}

QString KGlobalShortcutInfo::componentFriendlyName() const
{
    return d->componentFriendlyName.isEmpty() ? d->componentUniqueName : d->componentFriendlyName;
}

QString KGlobalShortcutInfo::componentUniqueName() const
{
    return d->componentUniqueName;
}

static QList<QKeySequence> withKeysFromTriggers(QList<QKeySequence> keys, const QList<KGlobalShortcutTrigger> &triggers)
{
    for (const KGlobalShortcutTrigger &trigger : triggers) {
        if (const KeyboardShortcut *kbsc = trigger.asKeyboardShortcut(); kbsc != nullptr) {
            keys.append(kbsc->keySequence);
        }
    }
    return keys;
}

QList<QKeySequence> KGlobalShortcutInfo::defaultKeys() const
{
    // For forward-compatibility with a plasma/kglobalacceld that will set triggers instead of keys,
    // return the union of old d->defaultKeys and new d->defaultTriggers that are keyboard shortcuts.
    return withKeysFromTriggers(d->defaultKeys, d->defaultTriggers);
}

QString KGlobalShortcutInfo::friendlyName() const
{
    return d->friendlyName;
}

QList<QKeySequence> KGlobalShortcutInfo::keys() const
{
    // For forward-compatibility with a plasma/kglobalacceld that will set triggers instead of keys,
    // return the union of old d->keys and new d->triggers that are keyboard shortcuts.
    return withKeysFromTriggers(d->keys, d->triggers);
}

QList<KGlobalShortcutTrigger> KGlobalShortcutInfo::triggers() const
{
    return d->triggers;
}

QList<KGlobalShortcutTrigger> KGlobalShortcutInfo::defaultTriggers() const
{
    return d->defaultTriggers;
}

QString KGlobalShortcutInfo::uniqueName() const
{
    return d->uniqueName;
}

KGlobalShortcutInfoWrapperV3::KGlobalShortcutInfoWrapperV3()
{
}

KGlobalShortcutInfoWrapperV3::KGlobalShortcutInfoWrapperV3(KGlobalShortcutInfo wrapped)
    : m_wrapped(std::move(wrapped))
{
}

const KGlobalShortcutInfo &KGlobalShortcutInfoWrapperV3::value() const
{
    return m_wrapped;
}

KGlobalShortcutInfo &KGlobalShortcutInfoWrapperV3::value()
{
    return m_wrapped;
}

// static
QList<KGlobalShortcutInfo> KGlobalShortcutInfoWrapperV3::unwrap(QList<KGlobalShortcutInfoWrapperV3> &&listOfWrappers)
{
    QList<KGlobalShortcutInfo> result;
    result.reserve(listOfWrappers.size());

    for (KGlobalShortcutInfoWrapperV3 &wrapper : listOfWrappers) {
        result.emplaceBack(std::move(wrapper.m_wrapped));
    }
    listOfWrappers.clear();
    return result;
}

#include "moc_kglobalshortcutinfo.cpp"
