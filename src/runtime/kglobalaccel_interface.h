/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KGLOBALACCEL_INTERFACE_H
#define KGLOBALACCEL_INTERFACE_H

#include <QObject>

#include "kf5globalaccelprivate_export.h"

class GlobalShortcutsRegistry;

#define KGlobalAccelInterface_iid "org.kde.kglobalaccel5.KGlobalAccelInterface"

/**
 * Abstract interface for plugins to implement
 */
class KGLOBALACCELPRIVATE_EXPORT KGlobalAccelInterface : public QObject
{
    Q_OBJECT

public:
    explicit KGlobalAccelInterface(QObject *parent);
    ~KGlobalAccelInterface() override;

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
    virtual bool grabKey(int key, bool grab) = 0;

    /*
     * Enable/disable all shortcuts. There will not be any grabbed shortcuts at this point.
     */
    virtual void setEnabled(bool) = 0;

    /**
     * Allows implementing plugins to synchronize with the windowing system.
     * Default implementation does nothing.
     **/
    virtual void syncWindowingSystem();

    void setRegistry(GlobalShortcutsRegistry *registry);

protected:
    /**
     * called by the implementation to inform us about key presses
     * @returns @c true if the key was handled
     **/
    bool keyPressed(int keyQt);
    void grabKeys();
    void ungrabKeys();

    class Private;
    QScopedPointer<Private> d;
};

class KGLOBALACCELPRIVATE_EXPORT KGlobalAccelInterfaceV2 : public KGlobalAccelInterface
{
    Q_OBJECT
public:
    KGlobalAccelInterfaceV2(QObject *parent);

protected:
    /**
     * Called by the implementation to inform us about key releases
     *
     * @param keyQt the key that was just released
     *
     * @returns @c true if the key was handled
     **/
    bool keyReleased(int keyQt);
};

Q_DECLARE_INTERFACE(KGlobalAccelInterface, KGlobalAccelInterface_iid)

#endif
