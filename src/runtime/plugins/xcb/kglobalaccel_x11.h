/* This file is part of the KDE libraries
    Copyright (C) 2001,2002 Ellis Whitehead <ellis@kde.org>

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

#ifndef _KGLOBALACCEL_X11_H
#define _KGLOBALACCEL_X11_H

#include "../../kglobalaccel_interface.h"

#include <QObject>
#include <QAbstractNativeEventFilter>

struct xcb_key_press_event_t;
typedef struct _XCBKeySymbols xcb_key_symbols_t;
/**
 * @internal
 *
 * The KGlobalAccel private class handles grabbing of global keys,
 * and notification of when these keys are pressed.
 */
class KGlobalAccelImpl : public KGlobalAccelInterface, public QAbstractNativeEventFilter
{
	Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kglobalaccel5.KGlobalAccelInterface" FILE "xcb.json")
    Q_INTERFACES(KGlobalAccelInterface)

public:
    KGlobalAccelImpl(QObject *parent = nullptr);
    virtual ~KGlobalAccelImpl();

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
	bool grabKey(int key, bool grab) override;
	
	/// Enable/disable all shortcuts. There will not be any grabbed shortcuts at this point.
	void setEnabled(bool) override;

        bool nativeEventFilter(const QByteArray &eventType, void *message, long *) override;

        static void syncX();

private:
	/**
	 * Filters X11 events ev for key bindings in the accelerator dictionary.
	 * If a match is found the activated activated is emitted and the function
	 * returns true. Return false if the event is not processed.
	 *
	 * This is public for compatibility only. You do not need to call it.
	 */
	void x11MappingNotify();
        bool x11KeyPress(xcb_key_press_event_t *event);
	
    xcb_key_symbols_t *m_keySymbols;
    uint8_t m_xkb_first_event;
};

#endif // _KGLOBALACCEL_X11_H
