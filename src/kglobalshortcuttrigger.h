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
#include <QObject>
#include <QPointF>

#include <chrono>
#include <optional>

namespace KGlobalShortcutTriggerTypes
{
Q_NAMESPACE_EXPORT(KGLOBALACCEL_EXPORT)

enum class SwipeDirection : unsigned int {
    Left = 0,
    UpLeft,
    Up,
    UpRight,
    Right,
    DownRight,
    Down,
    DownLeft,
};
Q_ENUM_NS(SwipeDirection)

enum class EdgeSwipeDirection : unsigned int {
    FromLeft = 0,
    FromTopLeft,
    FromTop,
    FromTopRight,
    FromRight,
    FromBottomRight,
    FromBottom,
    FromBottomLeft,
};
Q_ENUM_NS(EdgeSwipeDirection)

enum class PinchDirection : unsigned int {
    Expanding = 0,
    Contracting,
};
Q_ENUM_NS(PinchDirection)

enum class RotateDirection : unsigned int {
    Clockwise = 0,
    CounterClockwise,
};
Q_ENUM_NS(RotateDirection)

enum class ScreenBorder : unsigned int {
    Left = 0,
    TopLeft,
    Top,
    TopRight,
    Right,
    BottomRight,
    Bottom,
    BottomLeft,
};
Q_ENUM_NS(ScreenBorder)

enum class PointerAxisDirection : unsigned int {
    Down = 0,
    Left,
    Up,
    Right,
};
Q_ENUM_NS(PointerAxisDirection)

class KGLOBALACCEL_EXPORT KeyboardShortcut
{
    Q_GADGET
public:
    QKeySequence keySequence;
    QKeySequence normalizedKeySequence;

    explicit KeyboardShortcut(QKeySequence);
    KeyboardShortcut(const KeyboardShortcut &other) = default;
    KeyboardShortcut &operator=(const KeyboardShortcut &other) = default;
};

class KGLOBALACCEL_EXPORT TouchpadSwipeGesture
{
    Q_GADGET
public:
    int fingerCount;
    SwipeDirection direction;

    // explicit TouchpadSwipeGesture(int fingerCount, SwipeDirection);
};

class KGLOBALACCEL_EXPORT TouchpadSwipe2DGesture
{
    Q_GADGET
public:
    int fingerCount;

    // explicit TouchpadSwipe2DGesture(int fingerCount);
};

class KGLOBALACCEL_EXPORT TouchpadPinchGesture
{
    Q_GADGET
public:
    int fingerCount;
    PinchDirection direction;

    // explicit TouchpadPinchGesture(int fingerCount, PinchDirection);
};

class KGLOBALACCEL_EXPORT TouchpadRotateGesture
{
    Q_GADGET
public:
    int fingerCount;
    RotateDirection direction;

    // explicit TouchpadRotateGesture(int fingerCount, RotateDirection);
};

class KGLOBALACCEL_EXPORT TouchpadHoldGesture
{
    Q_GADGET
public:
    int fingerCount;
    std::chrono::milliseconds duration;

    // explicit TouchpadHoldGesture(int fingerCount, std::chrono::milliseconds duration);
};

class KGLOBALACCEL_EXPORT ApproachScreenBorderGesture
{
    Q_GADGET
public:
    ScreenBorder border;

    // explicit ApproachScreenBorderGesture(ScreenBorder);
};

class KGLOBALACCEL_EXPORT TouchscreenSwipeGesture
{
    Q_GADGET
public:
    int fingerCount;
    SwipeDirection direction;

    // explicit TouchscreenSwipeGesture(int fingerCount, SwipeDirection);
};

class KGLOBALACCEL_EXPORT TouchscreenSwipe2DGesture
{
    Q_GADGET
public:
    int fingerCount;

    // explicit TouchscreenSwipe2DGesture(int fingerCount);
};

class KGLOBALACCEL_EXPORT TouchscreenSwipeFromEdgeGesture
{
    Q_GADGET
public:
    EdgeSwipeDirection edge;

    // explicit TouchscreenSwipeFromEdgeGesture(EdgeSwipeDirection);
};

class KGLOBALACCEL_EXPORT TouchscreenPinchGesture
{
    Q_GADGET
public:
    int fingerCount;
    PinchDirection direction;

    // explicit TouchscreenPinchGesture(int fingerCount, PinchDirection);
};

class KGLOBALACCEL_EXPORT TouchscreenRotateGesture
{
    Q_GADGET
public:
    int fingerCount;
    RotateDirection direction;

    // explicit TouchscreenRotateGesture(int fingerCount, RotateDirection);
};

class KGLOBALACCEL_EXPORT TouchscreenHoldGesture
{
    Q_GADGET
public:
    int fingerCount;
    std::chrono::milliseconds duration;

    // explicit TouchscreenHoldGesture(int fingerCount, std::chrono::milliseconds duration);
};

class KGLOBALACCEL_EXPORT PointerAxisGesture
{
    Q_GADGET
public:
    enum class MouseButtonRequirement : unsigned int { // TODO: evaluate if this should go into activationRequirements(), like modifier key requirements
        NoButton,
        ActivationButton,
    };
    Q_ENUM(MouseButtonRequirement)

    PointerAxisDirection direction;
    MouseButtonRequirement button;

    // explicit PointerAxisGesture(PointerAxisDirection, MouseButtonRequirement);
};

class KGLOBALACCEL_EXPORT LineShapeGesture
{
    Q_GADGET
public:
    QList<QPointF> points;

    // explicit LineShapeGesture(QList<QPointF> points);
};

} // namespace KGlobalShortcutTriggerTypes

class KGlobalShortcutTriggerPrivate;

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
    //! Create an empty trigger value, i.e. \c {isEmpty() == true}.
    KGlobalShortcutTrigger();

    ~KGlobalShortcutTrigger();

    KGlobalShortcutTrigger(const KGlobalShortcutTrigger &rhs);
    KGlobalShortcutTrigger &operator=(const KGlobalShortcutTrigger &rhs);

    //! Create a keyboard shortcut trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::KeyboardShortcut &);

    //! Create a touchpad swipe gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchpadSwipeGesture &);

    //! Create a freeform 2D touchpad swipe gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchpadSwipe2DGesture &);

    //! Create a touchpad pinch gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchpadPinchGesture &);

    //! Create a touchpad rotate gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchpadRotateGesture &);

    //! Create a touchpad hold gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchpadHoldGesture &);

    //! Create a gesture trigger for approaching a screen border with the pointer.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::ApproachScreenBorderGesture &);

    //! Create a touchscreen swipe gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchscreenSwipeGesture &);

    //! Create a freeform 2D touchscreen swipe gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchscreenSwipe2DGesture &);

    //! Create a touchscreen swipe-from-edge gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchscreenSwipeFromEdgeGesture &);

    //! Create a touchscreen pinch gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchscreenPinchGesture &);

    //! Create a touchscreen rotate gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchscreenRotateGesture &);

    //! Create a touchscreen hold gesture trigger.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::TouchscreenHoldGesture &);

    //! Create a pointer axis gesture trigger, most commonly actived using a scroll wheel.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::PointerAxisGesture &);

    //! Create a line shape gesture trigger, also known as mouse gesture.
    KGlobalShortcutTrigger(const KGlobalShortcutTriggerTypes::LineShapeGesture &);

    /*!
     * Create a trigger value from a string that was previously exported via toString().
     *
     * Passing an empty string will produce an empty KGlobalShortcutTrigger value,
     * i.e. \c {isEmpty() == true}.
     *
     * A non-empty string will produce a non-empty KGlobalShortcutTrigger, regardless of whether
     * the string can be interpreted by this version of KGlobalAccel. The serialized string is
     * retained verbatim so it be written back unmodified to a config file with toString().
     *
     * \sa toString()
     * \sa isKnownTriggerType()
     */
    explicit KGlobalShortcutTrigger(const QString &serialized);

    /*!
     * A serialized string representation of this trigger.
     *
     * Any characters may be used other than '\n', '\r' or '\t'.
     */
    QString toString() const;

    //! Returns true if created with the parameter-less default constructor, or from an empty string.
    bool isEmpty() const;

    /*!
     * Returns true if this value represents a trigger type known by this version of KGlobalAccel.
     *
     * Returns false for an empty trigger value. Also returns false if the object was initialized
     * from a serialized string that this version of KGlobalAccel cannot parse.
     *
     * If the serialized string cannot be parsed, it may still be valid but may have been generated
     * by a future version of KGlobalAccel.
     */
    bool isKnownTriggerType() const;

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
     * Return true if this trigger should not be active if the \a other one is already
     * active in the same context.
     *
     * \sa KGlobalAccel::MatchType
     */
    bool conflictsWith(const KGlobalShortcutTrigger &other) const;

    /*!
     * Return a trigger that inverses this one into the opposite direction.
     *
     * If this trigger has no notion of an inverse, return std::nullopt.
     */
    std::optional<KGlobalShortcutTrigger> inverse();

    const KGlobalShortcutTriggerTypes::KeyboardShortcut *asKeyboardShortcut() const;
    const KGlobalShortcutTriggerTypes::TouchpadSwipeGesture *asTouchpadSwipeGesture() const;
    const KGlobalShortcutTriggerTypes::TouchpadSwipe2DGesture *asTouchpadSwipe2DGesture() const;
    const KGlobalShortcutTriggerTypes::TouchpadPinchGesture *asTouchpadPinchGesture() const;
    const KGlobalShortcutTriggerTypes::TouchpadRotateGesture *asTouchpadRotateGesture() const;
    const KGlobalShortcutTriggerTypes::TouchpadHoldGesture *asTouchpadHoldGesture() const;
    const KGlobalShortcutTriggerTypes::ApproachScreenBorderGesture *asApproachScreenBorderGesture() const;
    const KGlobalShortcutTriggerTypes::TouchscreenSwipeGesture *asTouchscreenSwipeGesture() const;
    const KGlobalShortcutTriggerTypes::TouchscreenSwipe2DGesture *asTouchscreenSwipe2DGesture() const;
    const KGlobalShortcutTriggerTypes::TouchscreenSwipeFromEdgeGesture *asTouchscreenSwipeFromEdgeGesture() const;
    const KGlobalShortcutTriggerTypes::TouchscreenPinchGesture *asTouchscreenPinchGesture() const;
    const KGlobalShortcutTriggerTypes::TouchscreenRotateGesture *asTouchscreenRotateGesture() const;
    const KGlobalShortcutTriggerTypes::TouchscreenHoldGesture *asTouchscreenHoldGesture() const;
    const KGlobalShortcutTriggerTypes::PointerAxisGesture *asPointerAxisGesture() const;
    const KGlobalShortcutTriggerTypes::LineShapeGesture *asLineShapeGesture() const;

    bool operator==(const KGlobalShortcutTrigger &rhs) const;

    // convenience functions to convert from and to previous API types
    static QList<KGlobalShortcutTrigger> fromKeyboardShortcuts(const QList<QKeySequence> &keys);
    static QList<QKeySequence> onlyKeyboardShortcuts(const QList<KGlobalShortcutTrigger> &triggers);

private:
    friend KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutTrigger &shortcut);

    //! Implementation details
    KGlobalShortcutTriggerPrivate *d;
};

KGLOBALACCEL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KGlobalShortcutTrigger &trigger);
KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutTrigger &trigger);

Q_DECLARE_METATYPE(KGlobalShortcutTrigger)

#endif /* #ifndef KGLOBALSHORTCUTTRIGGER_H */
