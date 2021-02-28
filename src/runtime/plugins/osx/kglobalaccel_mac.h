/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Marijn Kruisselbrink <m.kruisselbrink@student.tue.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KGLOBALACCEL_MAC_H
#define _KGLOBALACCEL_MAC_H

#include <QWidget>

/* including kglobalaccel.h here because Carbon.h includes AssertMacros.h which
   defines check(assertion) as nothing and breaks compilation */
#include "kglobalaccel.h"

#include <Carbon/Carbon.h>

template<class Key, class T>
class QMap;
template<class T>
class QList;

class GlobalShortcutsRegistry;
class KGlobalAccelImpl : public QWidget
{
    Q_OBJECT
public:
    KGlobalAccelImpl(GlobalShortcutsRegistry *owner);
    ~KGlobalAccelImpl();

public:
    /**
     * This function registers or unregisters a certain key for global capture,
     * depending on \b grab.
     *
     * Before destruction, every grabbed key will be released, so this
     * object does not need to do any tracking.
     *
     * \param key the Qt keycode to grab or release.
     * \param grab true to grab they key, false to release the key.
     *
     * \return true if successful, otherwise false.
     */
    bool grabKey(int key, bool grab);

    /// Enable/disable all shortcuts. There will not be any grabbed shortcuts at this point.
    void setEnabled(bool);

    void keyboardLayoutChanged();

private:
    friend OSStatus hotKeyEventHandler(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void *inUserData);
    /// Called by the carbon event handler when a key is pressed.
    bool keyPressed(int key);

    GlobalShortcutsRegistry *m_owner;
    EventTargetRef m_eventTarget;
    EventHandlerUPP m_eventHandler;
    EventTypeSpec m_eventType[2];
    EventHandlerRef m_curHandler;
    QMap<int, QList<EventHotKeyRef>> *refs;
};

#endif // _KGLOBALACCEL_MAC_H
