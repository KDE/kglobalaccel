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
    using TriggerVariant = std::variant<KeyboardShortcut,
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
                                        AxisGesture,
                                        StrokeGesture>;
    TriggerVariant variant;
    QString serialized;
};

#endif /* #ifndef KGLOBALSHORTCUTTRIGGER_P_H */
