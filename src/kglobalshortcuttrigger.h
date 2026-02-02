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

class KGlobalShortcutTriggerPrivate;

class KGLOBALACCEL_EXPORT KeyboardShortcut
{
    Q_GADGET
public:
    QKeySequence keySequence;
    QKeySequence normalizedKeySequence;
};

class KGLOBALACCEL_EXPORT TouchpadSwipeGesture
{
    Q_GADGET
public:
    enum class Direction {
        Left = 0,
        UpLeft,
        Up,
        UpRight,
        Right,
        DownRight,
        Down,
        DownLeft,
    };
    Q_ENUM(Direction)

    int fingerCount;
    Direction direction;
};

class KGLOBALACCEL_EXPORT TouchpadSwipe2DGesture
{
    Q_GADGET
public:
    int fingerCount;
};

class KGLOBALACCEL_EXPORT TouchpadPinchGesture
{
    Q_GADGET
public:
    enum class Direction {
        Expanding = 0,
        Contracting,
    };
    Q_ENUM(Direction)

    int fingerCount;
    Direction direction;
};

class KGLOBALACCEL_EXPORT TouchpadRotateGesture
{
    Q_GADGET
public:
    enum class Direction {
        Clockwise = 0,
        CounterClockwise,
    };
    Q_ENUM(Direction)

    int fingerCount;
    Direction direction;
};

class KGLOBALACCEL_EXPORT TouchpadHoldGesture
{
    Q_GADGET
public:
    int fingerCount;
    std::chrono::milliseconds duration;
};

class KGLOBALACCEL_EXPORT ApproachScreenBorderGesture
{
    Q_GADGET
public:
    enum class Border {
        Left = 0,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
    };
    Q_ENUM(Border)

    Border border;
};

class KGLOBALACCEL_EXPORT TouchscreenSwipeGesture
{
    Q_GADGET
public:
    enum class Direction {
        Left = 0,
        UpLeft,
        Up,
        UpRight,
        Right,
        DownRight,
        Down,
        DownLeft,
    };
    Q_ENUM(Direction)

    int fingerCount;
    Direction direction;
};

class KGLOBALACCEL_EXPORT TouchscreenSwipe2DGesture
{
    Q_GADGET
public:
    int fingerCount;
};

class KGLOBALACCEL_EXPORT TouchscreenSwipeFromEdgeGesture
{
    Q_GADGET
public:
    enum class Edge {
        FromLeft = 0,
        FromTopLeft,
        FromTop,
        FromTopRight,
        FromRight,
        FromBottomRight,
        FromBottom,
        FromBottomLeft,
    };
    Q_ENUM(Edge)

    Edge edge;
};

class KGLOBALACCEL_EXPORT TouchscreenPinchGesture
{
    Q_GADGET
public:
    enum class Direction {
        Expanding = 0,
        Contracting,
    };
    Q_ENUM(Direction)

    int fingerCount;
    Direction direction;
};

class KGLOBALACCEL_EXPORT TouchscreenRotateGesture
{
    Q_GADGET
public:
    enum class Direction {
        Clockwise = 0,
        Counterclockwise,
    };
    Q_ENUM(Direction)

    int fingerCount;
    Direction direction;
};

class KGLOBALACCEL_EXPORT TouchscreenHoldGesture
{
    Q_GADGET
public:
    int fingerCount;
    std::chrono::milliseconds duration;
};

class KGLOBALACCEL_EXPORT ScrollAxisGesture
{
    Q_GADGET
public:
    enum class Direction {
        Down = 0,
        Left,
        Up,
        Right,
    };
    Q_ENUM(Direction)

    enum class MouseButtonRequirement { // TODO: evaluate if this should go into activationRequirements(), like modifier key requirements
        NoButton,
        ActivationButton,
    };
    Q_ENUM(MouseButtonRequirement)

    Direction direction;
    MouseButtonRequirement button;
};

class KGLOBALACCEL_EXPORT LineShapeGesture
{
    Q_GADGET
public:
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

    const KeyboardShortcut *asKeyboardShortcut() const;
    const TouchpadSwipeGesture *asTouchpadSwipeGesture() const;
    const TouchpadSwipe2DGesture *asTouchpadSwipe2DGesture() const;
    const TouchpadPinchGesture *asTouchpadPinchGesture() const;
    const TouchpadRotateGesture *asTouchpadRotateGesture() const;
    const TouchpadHoldGesture *asTouchpadHoldGesture() const;
    const ApproachScreenBorderGesture *asApproachScreenBorderGesture() const;
    const TouchscreenSwipeGesture *asTouchscreenSwipeGesture() const;
    const TouchscreenSwipe2DGesture *asTouchscreenSwipe2DGesture() const;
    const TouchscreenSwipeFromEdgeGesture *asTouchscreenSwipeFromEdgeGesture() const;
    const TouchscreenPinchGesture *asTouchscreenPinchGesture() const;
    const TouchscreenRotateGesture *asTouchscreenRotateGesture() const;
    const TouchscreenHoldGesture *asTouchscreenHoldGesture() const;
    const ScrollAxisGesture *asScrollAxisGesture() const;
    const LineShapeGesture *asLineShapeGesture() const;

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
