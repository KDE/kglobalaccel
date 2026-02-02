/*
    SPDX-FileCopyrightText: 2026 Jakob Petsovits <jpetso@petsovits.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KGLOBALSHORTCUTTRIGGER_P_H
#define KGLOBALSHORTCUTTRIGGER_P_H

/**
 * @internal
 */

#include "kglobalshortcuttrigger.h"

#include <variant>

class KGlobalShortcutTriggerPrivate
{
public:
    struct Uninitialized {
    };
    struct Unparseable {
    };
    using TriggerVariant = std::variant<Uninitialized,
                                        Unparseable,
                                        KeyboardShortcut,
                                        TouchpadSwipeGesture,
                                        TouchpadSwipe2DGesture,
                                        TouchpadPinchGesture,
                                        TouchpadRotateGesture,
                                        TouchpadHoldGesture,
                                        ApproachScreenBorderGesture,
                                        TouchscreenSwipeGesture,
                                        TouchscreenSwipe2DGesture,
                                        TouchscreenSwipeFromEdgeGesture,
                                        TouchscreenPinchGesture,
                                        TouchscreenRotateGesture,
                                        TouchscreenHoldGesture,
                                        ScrollAxisGesture,
                                        LineShapeGesture>;
    TriggerVariant variant;
    QString serialized;

public:
    template<class Variant = Uninitialized>
    KGlobalShortcutTriggerPrivate(Variant &&trigger)
        : variant(trigger)
    {
    }

    void deserialize();
};

#endif /* #ifndef KGLOBALSHORTCUTTRIGGER_P_H */
