/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>
    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalaccel_x11.h"

#include "logging_p.h"
#include <KKeyServer>
#include <netwm.h>

#include <QDebug>

#include <QApplication>
#include <QWidget>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

#include <X11/keysym.h>

// xcb

// It uses "explicit" as a variable name, which is not allowed in C++
#define explicit xcb_explicit
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xkb.h>
#undef explicit

// g_keyModMaskXAccel
//	mask of modifiers which can be used in shortcuts
//	(meta, alt, ctrl, shift)
// g_keyModMaskXOnOrOff
//	mask of modifiers where we don't care whether they are on or off
//	(caps lock, num lock, scroll lock)
static uint g_keyModMaskXAccel = 0;
static uint g_keyModMaskXOnOrOff = 0;

static void calculateGrabMasks()
{
    g_keyModMaskXAccel = KKeyServer::accelModMaskX();
    g_keyModMaskXOnOrOff = KKeyServer::modXLock() | KKeyServer::modXNumLock() | KKeyServer::modXScrollLock() | KKeyServer::modXModeSwitch();
    // qCDebug(KGLOBALACCELD) << "g_keyModMaskXAccel = " << g_keyModMaskXAccel
    //	<< "g_keyModMaskXOnOrOff = " << g_keyModMaskXOnOrOff << endl;
}

//----------------------------------------------------

KGlobalAccelImpl::KGlobalAccelImpl(QObject *parent)
    : KGlobalAccelInterface(parent)
    , m_keySymbols(nullptr)
    , m_xkb_first_event(0)
{
    Q_ASSERT(QX11Info::connection());

    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(QX11Info::connection(), &xcb_xkb_id);
    if (reply && reply->present) {
        m_xkb_first_event = reply->first_event;
    }

    calculateGrabMasks();
}

KGlobalAccelImpl::~KGlobalAccelImpl()
{
    if (m_keySymbols) {
        xcb_key_symbols_free(m_keySymbols);
    }
}

bool KGlobalAccelImpl::grabKey(int keyQt, bool grab)
{
    // grabKey is called during shutdown
    // shutdown might be due to the X server being killed
    // if so, fail immediately before trying to make other xcb calls
    if (!QX11Info::connection() || xcb_connection_has_error(QX11Info::connection())) {
        return false;
    }

    if (!m_keySymbols) {
        m_keySymbols = xcb_key_symbols_alloc(QX11Info::connection());
        if (!m_keySymbols) {
            return false;
        }
    }

    if (!keyQt) {
        qCDebug(KGLOBALACCELD) << "Tried to grab key with null code.";
        return false;
    }

    uint keyModX;
    xcb_keysym_t keySymX;

    // Resolve the modifier
    if (!KKeyServer::keyQtToModX(keyQt, &keyModX)) {
        qCDebug(KGLOBALACCELD) << "keyQt (0x" << Qt::hex << keyQt << ") failed to resolve to x11 modifier";
        return false;
    }

    // Resolve the X symbol
    if (!KKeyServer::keyQtToSymX(keyQt, (int *)&keySymX)) {
        qCDebug(KGLOBALACCELD) << "keyQt (0x" << Qt::hex << keyQt << ") failed to resolve to x11 keycode";
        return false;
    }

    xcb_keycode_t *keyCodes = xcb_key_symbols_get_keycode(m_keySymbols, keySymX);
    if (!keyCodes) {
        return false;
    }
    int i = 0;
    bool success = !grab;
    while (keyCodes[i] != XCB_NO_SYMBOL) {
        xcb_keycode_t keyCodeX = keyCodes[i++];

        // Check if shift needs to be added to the grab since KKeySequenceWidget
        // can remove shift for some keys. (all the %&* and such)
        /* clang-format off */
        if (!(keyQt & Qt::SHIFT)
            && !KKeyServer::isShiftAsModifierAllowed(keyQt)
            && !(keyQt & Qt::KeypadModifier)
            && keySymX != xcb_key_symbols_get_keysym(m_keySymbols, keyCodeX, 0)
            && keySymX == xcb_key_symbols_get_keysym(m_keySymbols, keyCodeX, 1)) { /* clang-format on */
            qCDebug(KGLOBALACCELD) << "adding shift to the grab";
            keyModX |= KKeyServer::modXShift();
        }

        keyModX &= g_keyModMaskXAccel; // Get rid of any non-relevant bits in mod

        if (!keyCodeX) {
            qCDebug(KGLOBALACCELD) << "keyQt (0x" << Qt::hex << keyQt << ") was resolved to x11 keycode 0";
            continue;
        }

        // We'll have to grab 8 key modifier combinations in order to cover all
        //  combinations of CapsLock, NumLock, ScrollLock.
        // Does anyone with more X-savvy know how to set a mask on QX11Info::appRootWindow so that
        //  the irrelevant bits are always ignored and we can just make one XGrabKey
        //  call per accelerator? -- ellis
#ifndef NDEBUG
        QString sDebug = QString("\tcode: 0x%1 state: 0x%2 | ").arg(keyCodeX, 0, 16).arg(keyModX, 0, 16);
#endif
        uint keyModMaskX = ~g_keyModMaskXOnOrOff;
        QVector<xcb_void_cookie_t> cookies;
        for (uint irrelevantBitsMask = 0; irrelevantBitsMask <= 0xff; irrelevantBitsMask++) {
            if ((irrelevantBitsMask & keyModMaskX) == 0) {
#ifndef NDEBUG
                sDebug += QString("0x%3, ").arg(irrelevantBitsMask, 0, 16);
#endif
                if (grab) {
                    cookies << xcb_grab_key_checked(QX11Info::connection(),
                                                    true,
                                                    QX11Info::appRootWindow(),
                                                    keyModX | irrelevantBitsMask,
                                                    keyCodeX,
                                                    XCB_GRAB_MODE_ASYNC,
                                                    XCB_GRAB_MODE_SYNC);
                } else {
                    /* clang-format off */
                    cookies << xcb_ungrab_key_checked(QX11Info::connection(),
                                                      keyCodeX, QX11Info::appRootWindow(),
                                                      keyModX | irrelevantBitsMask);
                    /* clang-format on */
                }
            }
        }

        bool failed = false;
        if (grab) {
            for (int i = 0; i < cookies.size(); ++i) {
                QScopedPointer<xcb_generic_error_t, QScopedPointerPodDeleter> error(xcb_request_check(QX11Info::connection(), cookies.at(i)));
                if (!error.isNull()) {
                    failed = true;
                }
            }
            if (failed) {
                qCDebug(KGLOBALACCELD) << "grab failed!\n";
                for (uint m = 0; m <= 0xff; m++) {
                    if ((m & keyModMaskX) == 0) {
                        xcb_ungrab_key(QX11Info::connection(), keyCodeX, QX11Info::appRootWindow(), keyModX | m);
                    }
                }
            } else {
                success = true;
            }
        }
    }
    free(keyCodes);
    return success;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool KGlobalAccelImpl::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
#else
bool KGlobalAccelImpl::nativeEventFilter(const QByteArray &eventType, void *message, long *)
#endif
{
    if (eventType != "xcb_generic_event_t") {
        return false;
    }
    xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t *>(message);
    const uint8_t responseType = event->response_type & ~0x80;
    if (responseType == XCB_MAPPING_NOTIFY) {
        x11MappingNotify();

        // Make sure to let Qt handle it as well
        return false;
    } else if (responseType == XCB_KEY_PRESS) {
#ifdef KDEDGLOBALACCEL_TRACE
        qCDebug(KGLOBALACCELD) << "Got XKeyPress event";
#endif
        return x11KeyPress(reinterpret_cast<xcb_key_press_event_t *>(event));
    } else if (m_xkb_first_event && responseType == m_xkb_first_event) {
        const uint8_t xkbEvent = event->pad0;
        switch (xkbEvent) {
        case XCB_XKB_MAP_NOTIFY:
            x11MappingNotify();
            break;
        case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
            const xcb_xkb_new_keyboard_notify_event_t *ev = reinterpret_cast<xcb_xkb_new_keyboard_notify_event_t *>(event);
            if (ev->changed & XCB_XKB_NKN_DETAIL_KEYCODES) {
                x11MappingNotify();
            }
            break;
        }
        default:
            break;
        }

        // Make sure to let Qt handle it as well
        return false;
    } else {
        // We get all XEvents. Just ignore them.
        return false;
    }
}

void KGlobalAccelImpl::x11MappingNotify()
{
    qCDebug(KGLOBALACCELD) << "Got XMappingNotify event";

    // Maybe the X modifier map has been changed.
    // uint oldKeyModMaskXAccel = g_keyModMaskXAccel;
    // uint oldKeyModMaskXOnOrOff = g_keyModMaskXOnOrOff;

    // First ungrab all currently grabbed keys. This is needed because we
    // store the keys as qt keycodes and use KKeyServer to map them to x11 key
    // codes. After calling KKeyServer::initializeMods() they could map to
    // different keycodes.
    ungrabKeys();

    if (m_keySymbols) {
        // Force reloading of the keySym mapping
        xcb_key_symbols_free(m_keySymbols);
        m_keySymbols = nullptr;
    }

    KKeyServer::initializeMods();
    calculateGrabMasks();

    grabKeys();
}

bool KGlobalAccelImpl::x11KeyPress(xcb_key_press_event_t *pEvent)
{
    if (QWidget::keyboardGrabber() || QApplication::activePopupWidget()) {
        qCWarning(KGLOBALACCELD) << "kglobalacceld should be popup and keyboard grabbing free!";
    }

    // Keyboard needs to be ungrabed after XGrabKey() activates the grab,
    // otherwise it becomes frozen.
    xcb_connection_t *c = QX11Info::connection();
    xcb_void_cookie_t cookie = xcb_ungrab_keyboard_checked(c, XCB_TIME_CURRENT_TIME);
    xcb_flush(c);
    // xcb_flush() only makes sure that the ungrab keyboard request has been
    // sent, but is not enough to make sure that request has been fulfilled. Use
    // xcb_request_check() to make sure that the request has been processed.
    xcb_request_check(c, cookie);

    int keyQt;
    if (!KKeyServer::xcbKeyPressEventToQt(pEvent, &keyQt)) {
        qCWarning(KGLOBALACCELD) << "KKeyServer::xcbKeyPressEventToQt failed";
        return false;
    }
    // qDebug() << "keyQt=" << QString::number(keyQt, 16);

    // All that work for this hey... argh...
    if (NET::timestampCompare(pEvent->time, QX11Info::appTime()) > 0) {
        QX11Info::setAppTime(pEvent->time);
    }
    return keyPressed(keyQt);
}

void KGlobalAccelImpl::setEnabled(bool enable)
{
    if (enable && qApp->platformName() == QLatin1String("xcb")) {
        qApp->installNativeEventFilter(this);
    } else {
        qApp->removeNativeEventFilter(this);
    }
}

void KGlobalAccelImpl::syncX()
{
    xcb_connection_t *c = QX11Info::connection();
    auto *value = xcb_get_input_focus_reply(c, xcb_get_input_focus_unchecked(c), nullptr);
    free(value);
}
