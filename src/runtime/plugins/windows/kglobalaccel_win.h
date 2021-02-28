/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KGLOBALACCEL_WIN_H
#define _KGLOBALACCEL_WIN_H

#include <QWidget>

class GlobalShortcutsRegistry;
/**
 * @internal
 *
 * The KGlobalAccel private class handles grabbing of global keys,
 * and notification of when these keys are pressed.
 */
class KGlobalAccelImpl : public QWidget
{
    Q_OBJECT

public:
    KGlobalAccelImpl(GlobalShortcutsRegistry *owner);

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

private:
    bool winEvent(MSG *message, long *result);

    GlobalShortcutsRegistry *m_owner;
    bool m_enabled;
};

#endif // _KGLOBALACCEL_WIN_H
