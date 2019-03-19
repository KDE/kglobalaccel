/* This file is part of the KDE libraries
    Copyright (C) 2001,2002 Ellis Whitehead <ellis@kde.org>
    Copyright (C) 2006 Marijn Kruisselbrink <m.kruisselbrink@student.tue.nl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kglobalaccel_mac.h"

#include <QDebug>

#ifdef Q_OS_OSX

#include <QMultiMap>
#include <QList>

#include "globalshortcutsregistry.h"
#include "kkeyserver.h"
#include "logging_p.h"

OSStatus hotKeyEventHandler(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void * inUserData)
{
    UInt32 eventKind = GetEventKind(inEvent);
    if (eventKind == kEventRawKeyDown) {
        UInt32 keycode;
        if (GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(keycode), NULL, &keycode) != noErr) {
            qCWarning(KGLOBALACCELD) << "Error retrieving keycode parameter from event";
        }
        qCDebug(KGLOBALACCELD) << " key down, keycode = " << keycode;
    } else if (eventKind == kEventHotKeyPressed) {
        KGlobalAccelImpl* impl = static_cast<KGlobalAccelImpl *>(inUserData);
        EventHotKeyID hotkey;
        if (GetEventParameter(inEvent, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(hotkey), NULL, &hotkey) != noErr) {
            qCWarning(KGLOBALACCELD) << "Error retrieving hotkey parameter from event";
            return eventNotHandledErr;
        }
        // Typecasts necesary to prevent a warning from gcc
        return (impl->keyPressed(hotkey.id) ? (OSStatus) noErr : (OSStatus) eventNotHandledErr);
    }
    return eventNotHandledErr;
}
 
void layoutChanged(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
    static_cast<KGlobalAccelImpl *>(observer)->keyboardLayoutChanged();
}

KGlobalAccelImpl::KGlobalAccelImpl(GlobalShortcutsRegistry* owner)
	: m_owner(owner)
    , m_eventTarget(GetApplicationEventTarget())
    , m_eventHandler(NewEventHandlerUPP(hotKeyEventHandler))
{
    m_eventType[0].eventClass = kEventClassKeyboard;
    m_eventType[0].eventKind = kEventHotKeyPressed; 
    m_eventType[1].eventClass = kEventClassKeyboard; // only useful for testing, is not used because count passed in call to InstallEventHandler is 1
    m_eventType[1].eventKind = kEventRawKeyDown;
    refs = new QMap<int, QList<EventHotKeyRef> >();
    
    CFStringRef str = CFStringCreateWithCString(NULL, "AppleKeyboardPreferencesChangedNotification", kCFStringEncodingASCII);
    if (str) {
        CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), this, layoutChanged, str, NULL, CFNotificationSuspensionBehaviorHold);
        CFRelease(str);
    } else {
        qCWarning(KGLOBALACCELD) << "Couldn't create CFString to register for keyboard notifications";
    }
}

KGlobalAccelImpl::~KGlobalAccelImpl()
{
    DisposeEventHandlerUPP(hotKeyEventHandler);
    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), this, NULL, NULL);
    delete refs;
}

bool KGlobalAccelImpl::grabKey( int keyQt, bool grab )
{
    if (grab) {
        qCDebug(KGLOBALACCELD) << "Grabbing key " << keyQt;
        QList<uint> keyCodes;
        uint mod;
        KKeyServer::keyQtToCodeMac( keyQt, keyCodes );
        KKeyServer::keyQtToModMac( keyQt, mod );
        
        qCDebug(KGLOBALACCELD) << "keyQt: " << keyQt << " mod: " << mod;
        for (uint keyCode : qAsConst(keyCodes)) {
            qCDebug(KGLOBALACCELD) << "  keyCode: " << keyCode;
        }
        
        EventHotKeyID ehkid;
        ehkid.signature = 'Kgai';
        ehkid.id = keyQt;
        QList<EventHotKeyRef> hotkeys;
        for (uint keyCode : qAsConst(keyCodes)) {
            EventHotKeyRef ref;
            if (RegisterEventHotKey(keyCode, mod, ehkid, m_eventTarget, 0, &ref) != noErr) {
                qCWarning(KGLOBALACCELD) << "RegisterEventHotKey failed!";
            }
            hotkeys.append(ref);
        }
        refs->insert(keyQt, hotkeys);
    } else {
        qCDebug(KGLOBALACCELD) << "Ungrabbing key " << keyQt;
        if (refs->count(keyQt) == 0) qCWarning(KGLOBALACCELD) << "Trying to ungrab a key thas is not grabbed";
        const auto lstRef = refs->value(keyQt);
        for (const EventHotKeyRef &ref : lstRef) {
            if (UnregisterEventHotKey(ref) != noErr) {
                qCWarning(KGLOBALACCELD) << "UnregisterEventHotKey should not fail!";
            }
        }
        refs->remove(keyQt);
    }
    return true;
}

void KGlobalAccelImpl::setEnabled(bool enable)
{
    if (enable) {
        if (InstallEventHandler(m_eventTarget, m_eventHandler, 1, m_eventType, this, &m_curHandler) != noErr)
            qCWarning(KGLOBALACCELD) << "InstallEventHandler failed!";
    } else {
        if (RemoveEventHandler(m_curHandler) != noErr)
            qCWarning(KGLOBALACCELD) << "RemoveEventHandler failed!";
    }
}

bool KGlobalAccelImpl::keyPressed( int key )
{
    return m_owner->keyPressed(key);
}

void KGlobalAccelImpl::keyboardLayoutChanged()
{
    // Keyboard layout might have changed, first ungrab all keys
    QList<int> keys; // Array to store all the keys that were grabbed
    while (!refs->empty()) {
        int key = refs->begin().key();
        keys.append(key);
        grabKey(key, false);
    }
    // Now re-grab all the keys
    for (int key : qAsConst(keys)) {
        grabKey(key, true);
    }
}


#endif // !Q_OS_OSX
