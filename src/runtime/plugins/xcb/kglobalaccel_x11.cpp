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
#include <QSocketNotifier>

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
#include <xcb/record.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcbext.h>
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
    : KGlobalAccelInterfaceV2(parent)
    , m_keySymbols(nullptr)
    , m_xkb_first_event(0)
{
    Q_ASSERT(QX11Info::connection());

    int events = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
    xcb_change_window_attributes(QX11Info::connection(), QX11Info::appRootWindow(), XCB_CW_EVENT_MASK, &events);

    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(QX11Info::connection(), &xcb_xkb_id);
    if (reply && reply->present) {
        m_xkb_first_event = reply->first_event;
    }

    // We use XRecord to get the released keys because we need a way to get notified about
    // them without needing to hold a grab
    // Holding a grab would be a problem for the cases when a process (looking at you KWin's
    // toolbox) replies to a global shortcut trigger with another grab
    m_display = XOpenDisplay(nullptr);
    auto connection = xcb_connect(XDisplayString((Display *)m_display), nullptr);
    auto context = xcb_generate_id(connection);
    xcb_record_range_t range;
    memset(&range, 0, sizeof(range));
    range.device_events.first = XCB_KEY_RELEASE;
    range.device_events.last = XCB_KEY_RELEASE;
    xcb_record_client_spec_t cs = XCB_RECORD_CS_ALL_CLIENTS;
    xcb_record_create_context(connection, context, 0, 1, 1, &cs, &range);
    auto cookie = xcb_record_enable_context(connection, context);
    xcb_flush(connection);

    m_xrecordCookieSequence = cookie.sequence;

    auto m_notifier = new QSocketNotifier(xcb_get_file_descriptor(connection), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, [this, connection] {
        xcb_generic_event_t *event;
        while ((event = xcb_poll_for_event(connection))) {
            std::free(event);
        }

        xcb_record_enable_context_reply_t *reply = nullptr;
        xcb_generic_error_t *error = nullptr;
        while (m_xrecordCookieSequence && xcb_poll_for_reply(connection, m_xrecordCookieSequence, (void **)&reply, &error)) {
            // xcb_poll_for_reply may set both reply and error to null if connection has error.
            // break if xcb_connection has error, no point to continue anyway.
            if (xcb_connection_has_error(connection)) {
                break;
            }

            if (error) {
                std::free(error);
                break;
            }

            if (!reply) {
                continue;
            }

            QScopedPointer<xcb_record_enable_context_reply_t, QScopedPointerPodDeleter> data(reinterpret_cast<xcb_record_enable_context_reply_t *>(reply));
            xcb_key_press_event_t *events = reinterpret_cast<xcb_key_press_event_t *>(xcb_record_enable_context_data(reply));
            int nEvents = xcb_record_enable_context_data_length(reply) / sizeof(xcb_key_press_event_t);
            for (xcb_key_press_event_t *e = events; e < events + nEvents; e++) {
                Q_ASSERT(e->response_type == XCB_KEY_RELEASE);
                qCDebug(KGLOBALACCELD) << "Got XKeyRelease event";
                x11KeyRelease(e);
            }
        }
    });
    m_notifier->setEnabled(true);

    calculateGrabMasks();
}

KGlobalAccelImpl::~KGlobalAccelImpl()
{
    XCloseDisplay((Display *)m_display);
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
        QString sDebug = QStringLiteral("\tcode: 0x%1 state: 0x%2 | ").arg(keyCodeX, 0, 16).arg(keyModX, 0, 16);
#endif
        uint keyModMaskX = ~g_keyModMaskXOnOrOff;
        QVector<xcb_void_cookie_t> cookies;
        for (uint irrelevantBitsMask = 0; irrelevantBitsMask <= 0xff; irrelevantBitsMask++) {
            if ((irrelevantBitsMask & keyModMaskX) == 0) {
#ifndef NDEBUG
                sDebug += QStringLiteral("0x%3, ").arg(irrelevantBitsMask, 0, 16);
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
        qCDebug(KGLOBALACCELD) << "Got XKeyPress event";
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

bool KGlobalAccelImpl::x11KeyRelease(xcb_key_press_event_t *pEvent)
{
    if (QWidget::keyboardGrabber() || QApplication::activePopupWidget()) {
        qCWarning(KGLOBALACCELD) << "kglobalacceld should be popup and keyboard grabbing free!";
    }

    int keyQt;
    if (!KKeyServer::xcbKeyPressEventToQt(pEvent, &keyQt)) {
        return false;
    }
    return keyReleased(keyQt);
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
