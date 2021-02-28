/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalaccel_win.h"

#include "kkeyserver_win.h"

#include <QWidgetList>
#ifdef Q_OS_WIN

#include "globalshortcutsregistry.h"
#include "kglobalaccel.h"
#include "logging_p.h"

#include <QDebug>

#include <windows.h>

KGlobalAccelImpl::KGlobalAccelImpl(GlobalShortcutsRegistry *owner)
    : m_owner(owner)
    , m_enabled(false)
{
}

bool KGlobalAccelImpl::grabKey(int keyQt, bool grab)
{
    if (!keyQt) {
        qCWarning(KGLOBALACCELD) << "Tried to grab key with null code.";
        return false;
    }

    uint keyCodeW;
    uint keyModW;
    KKeyServer::keyQtToCodeWin(keyQt, &keyCodeW);
    KKeyServer::keyQtToModWin(keyQt, &keyModW);

    ATOM id = GlobalAddAtom(MAKEINTATOM(keyQt));
    bool b;
    if (grab) {
        b = RegisterHotKey(reinterpret_cast<HWND>(winId()), id, keyModW, keyCodeW);
    } else {
        b = UnregisterHotKey(reinterpret_cast<HWND>(winId()), id);
    }

    return b;
}

void KGlobalAccelImpl::setEnabled(bool enable)
{
    m_enabled = enable;
}

bool KGlobalAccelImpl::winEvent(MSG *message, long *result)
{
    if (message->message == WM_HOTKEY) {
        uint keyCodeW = HIWORD(message->lParam);
        uint keyModW = LOWORD(message->lParam);

        int keyCodeQt, keyModQt;
        KKeyServer::codeWinToKeyQt(keyCodeW, &keyCodeQt);
        KKeyServer::modWinToKeyQt(keyModW, &keyModQt);

        return m_owner->keyPressed(keyCodeQt | keyModQt);
    }
    return false;
}

#endif // Q_OS_WIN
