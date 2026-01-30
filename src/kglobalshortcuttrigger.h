/*
    SPDX-FileCopyrightText: 2026 Jakob Petsovits <jpetso@petsovits.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KGLOBALSHORTCUTTRIGGER_H
#define KGLOBALSHORTCUTTRIGGER_H

#include <kglobalaccel_export.h>

#include <QDBusArgument>
#include <QKeySequence>
#include <QList>
#include <QPointF>
#include <QString>

#include <chrono>
#include <optional>

class KGlobalShortcutTriggerPrivate;

struct KGLOBALACCEL_EXPORT KeyboardShortcut {
    QKeySequence keySequence;
    QKeySequence normalizedKeySequence;
};

struct KGLOBALACCEL_EXPORT TouchpadSwipeGesture {
    enum class Direction : uint8_t {
        Left = 0,
        UpLeft,
        Up,
        UpRight,
        Right,
        DownRight,
        Down,
        DownLeft,
    };

    int fingerCount;
    Direction direction;
};

struct KGLOBALACCEL_EXPORT TouchpadSwipe2DGesture {
    int fingerCount;
};

struct KGLOBALACCEL_EXPORT TouchpadPinchGesture {
    enum class Direction : uint8_t {
        Expanding = 0,
        Contracting,
    };

    int fingerCount;
    Direction direction;
};

struct KGLOBALACCEL_EXPORT TouchpadRotateGesture {
    enum class Direction : uint8_t {
        Clockwise = 0,
        CounterClockwise,
    };

    int fingerCount;
    Direction direction;
};

struct KGLOBALACCEL_EXPORT TouchpadHoldGesture {
    int fingerCount;
    std::chrono::milliseconds duration;
};

struct KGLOBALACCEL_EXPORT ApproachScreenBorderGesture {
    enum class Border : uint8_t {
        Left = 0,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
    };

    Border border;
};

struct KGLOBALACCEL_EXPORT TouchscreenSwipeGesture {
    enum class Direction : uint8_t {
        Left = 0,
        UpLeft,
        Up,
        UpRight,
        Right,
        DownRight,
        Down,
        DownLeft,
    };

    int fingerCount;
    Direction direction;
};

struct KGLOBALACCEL_EXPORT TouchscreenSwipe2DGesture {
    int fingerCount;
};

struct KGLOBALACCEL_EXPORT TouchscreenSwipeFromEdgeGesture {
    enum class Edge : uint8_t {
        FromLeft = 0,
        FromTopLeft,
        FromTop,
        FromTopRight,
        FromRight,
        FromBottomRight,
        FromBottom,
        FromBottomLeft,
    };

    Edge edge;
};

struct KGLOBALACCEL_EXPORT TouchscreenPinchGesture {
    enum class Direction : uint8_t {
        Expanding = 0,
        Contracting,
    };

    int fingerCount;
    Direction direction;
};

struct KGLOBALACCEL_EXPORT TouchscreenRotateGesture {
    enum class Direction : uint8_t {
        Clockwise = 0,
        Counterclockwise,
    };

    int fingerCount;
    Direction direction;
};

struct KGLOBALACCEL_EXPORT TouchscreenHoldGesture {
    int fingerCount;
    std::chrono::milliseconds duration;
};

struct KGLOBALACCEL_EXPORT AxisGesture {
    enum class Direction : uint8_t {
        Down = 0,
        Left,
        Up,
        Right,
    };
    enum class MouseButtonRequirement : uint8_t { // TODO: evaluate if this should go into activationRequirements()
        NoButton,
        ActivationButton,
    };

    Direction direction;
    MouseButtonRequirement button;
};

struct KGLOBALACCEL_EXPORT StrokeGesture {
    QList<QPointF> points;
};

/*!
 * \class KGlobalShortcutTrigger
 * \inmodule KGlobalAccel
 * \brief Description of an event that can trigger an action, such as a keyboard shortcut or touch gesture.
 */
class KGLOBALACCEL_EXPORT KGlobalShortcutTrigger
{
public:
    struct KGLOBALACCEL_EXPORT ActivationRequirement {
        QString type;
        QString condition;
    };

public:
    KGlobalShortcutTrigger();
    ~KGlobalShortcutTrigger();

    KGlobalShortcutTrigger(const KGlobalShortcutTrigger &rhs);
    KGlobalShortcutTrigger &operator=(const KGlobalShortcutTrigger &rhs);

    /*!
     * Create a trigger value from a string that was previously exported via toString().
     *
     * Passing an empty string will produce an empty KGlobalShortcutTrigger value,
     * i.e. \c {isEmpty() == true}.
     *
     * For a string that cannot be interpreted by this version of KGlobalAccel, the created
     * KGlobalShortcutTrigger is not considered empty. The string is retained in the value
     * so it can later be written to a config file again unmodified.
     *
     * \sa toString()
     */
    static KGlobalShortcutTrigger fromString(const QString &serialized);

    //! Mainly for unit testing in plasma/kglobalacceld
    static KGlobalShortcutTrigger fromKeyboardShortcut(QKeySequence key);

    /*!
     * A serialized string representation of this trigger.
     *
     * Any characters may be used other than '\n', '\r' or '\t'.
     */
    QString toString() const;

    //! Returns true if created with the parameter-less default constructor, or from an empty string.
    bool isEmpty() const;

    /*!
     * A list of rules which determine whether this trigger is applicable in the current context.
     *
     * For example, this list can limit the trigger to a set of pressed modifiers
     * or active windows.
     *
     * A caller who is looking to recognize triggers must check that all of these requirements
     * are satisfied. The trigger should not be recognized, and a gesture trigger should not even
     * be started, if:
     *
     * \list
     *   \li any of the conditions do not apply in the current context, or,
     *   \li any of the conditions are not fully supported for evaluation by the caller,
     *   \li any of the activation requirement types are unknown to the caller.
     * \endlist
     *
     * A UI for listing and managing triggers should hide a trigger if the caller cannot visualize
     * all of its activation requirements. Hidden triggers, however, should be preserved when
     * storing a modified set of triggers.
     */
    QList<ActivationRequirement> activationRequirements() const;

    /*!
     * Return true if this trigger would shadow the \a other one if both are active.
     *
     * \sa KGlobalAccel::MatchType
     */
    bool canShadow(const KGlobalShortcutTrigger &other) const;

    /*!
     * Return a trigger that inverses this one into the opposite direction.
     *
     * If this trigger has no notion of an inverse, return std::nullopt.
     */
    std::optional<KGlobalShortcutTrigger> inverse();

    const KeyboardShortcut *asKeyboardShortcut() const;
    const TouchpadSwipeGesture *asTouchpadSwipe() const;
    const TouchpadSwipe2DGesture *asTouchpadSwipe2D() const;
    const TouchpadPinchGesture *asTouchpadPinch() const;
    const TouchpadRotateGesture *asTouchpadRotate() const;
    const TouchpadHoldGesture *asTouchpadHold() const;
    const ApproachScreenBorderGesture *asApproachScreenBorder() const;
    const TouchscreenSwipeGesture *asTouchscreenSwipe() const;
    const TouchscreenSwipe2DGesture *asTouchscreenSwipe2D() const;
    const TouchscreenSwipeFromEdgeGesture *asTouchscreenSwipeFromEdge() const;
    const TouchscreenPinchGesture *asTouchscreenPinch() const;
    const TouchscreenRotateGesture *asTouchscreenRotate() const;
    const TouchscreenHoldGesture *asTouchscreenHold() const;
    const AxisGesture *asAxisGesture() const;
    const StrokeGesture *asStrokeGesture() const;

    bool operator==(const KGlobalShortcutTrigger &rhs) const;

private:
    friend KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutTrigger &shortcut);

    //! Implementation details
    KGlobalShortcutTriggerPrivate *d;
};

KGLOBALACCEL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KGlobalShortcutTrigger &trigger);
KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutTrigger &trigger);

Q_DECLARE_METATYPE(KGlobalShortcutTrigger)

#endif /* #ifndef KGLOBALSHORTCUTTRIGGER_H */
