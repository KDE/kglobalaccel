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
                                        KGlobalShortcutTriggerTypes::KeyboardShortcut,
                                        KGlobalShortcutTriggerTypes::TouchpadSwipeGesture,
                                        KGlobalShortcutTriggerTypes::TouchpadSwipe2DGesture,
                                        KGlobalShortcutTriggerTypes::TouchpadPinchGesture,
                                        KGlobalShortcutTriggerTypes::TouchpadRotateGesture,
                                        KGlobalShortcutTriggerTypes::TouchpadHoldGesture,
                                        KGlobalShortcutTriggerTypes::ApproachScreenBorderGesture,
                                        KGlobalShortcutTriggerTypes::TouchscreenSwipeGesture,
                                        KGlobalShortcutTriggerTypes::TouchscreenSwipe2DGesture,
                                        KGlobalShortcutTriggerTypes::TouchscreenSwipeFromEdgeGesture,
                                        KGlobalShortcutTriggerTypes::TouchscreenPinchGesture,
                                        KGlobalShortcutTriggerTypes::TouchscreenRotateGesture,
                                        KGlobalShortcutTriggerTypes::TouchscreenHoldGesture,
                                        KGlobalShortcutTriggerTypes::PointerAxisGesture,
                                        KGlobalShortcutTriggerTypes::LineShapeGesture>;
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
