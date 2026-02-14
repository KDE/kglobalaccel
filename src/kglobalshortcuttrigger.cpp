/*
    SPDX-FileCopyrightText: 2026 Jakob Petsovits <jpetso@petsovits.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalshortcuttrigger.h"
#include "kglobalshortcuttrigger_p.h"

#include "sequencehelpers_p.h"

#include <QLatin1StringView>
#include <QMetaEnum>
#include <variant>

using namespace Qt::StringLiterals;
using namespace KGlobalShortcutTriggerTypes;

constexpr QLatin1StringView NonKeyTriggerPrefix = "T:"_L1;

constexpr QLatin1StringView TouchpadSwipePrefix = "TouchpadSwipe:"_L1;
constexpr QLatin1StringView TouchpadSwipe2DPrefix = "TouchpadSwipe2D:"_L1;
constexpr QLatin1StringView TouchpadPinchPrefix = "TouchpadPinch:"_L1;
constexpr QLatin1StringView TouchpadRotatePrefix = "TouchpadRotate:"_L1;
constexpr QLatin1StringView TouchpadHoldPrefix = "TouchpadHold:"_L1;
constexpr QLatin1StringView ApproachScreenBorderPrefix = "ApproachScreenBorder:"_L1;
constexpr QLatin1StringView TouchscreenSwipePrefix = "TouchscreenSwipe:"_L1;
constexpr QLatin1StringView TouchscreenSwipe2DPrefix = "TouchscreenSwipe2D:"_L1;
constexpr QLatin1StringView TouchscreenSwipeFromEdgePrefix = "TouchscreenSwipeFromEdge:"_L1;
constexpr QLatin1StringView TouchscreenPinchPrefix = "TouchscreenPinch:"_L1;
constexpr QLatin1StringView TouchscreenRotatePrefix = "TouchscreenRotate:"_L1;
constexpr QLatin1StringView TouchscreenHoldPrefix = "TouchscreenHold:"_L1;
constexpr QLatin1StringView LineShapePrefix = "LineShape:"_L1;
constexpr QLatin1StringView PointerAxisPrefix = "PointerAxis:"_L1;

//
// Trigger type constructors

namespace KGlobalShortcutTriggerTypes
{

KeyboardShortcut::KeyboardShortcut(QKeySequence keys)
    : keySequence(keys)
    , normalizedKeySequence(Utils::normalizeSequence(keys))
{
}

// TouchpadSwipeGesture::TouchpadSwipeGesture(int fingerCount, SwipeDirection direction)
//     : fingerCount(fingerCount)
//     , direction(direction)
// {
// }
//
// TouchpadSwipe2DGesture::TouchpadSwipe2DGesture(int fingerCount)
//     : fingerCount(fingerCount)
// {
// }
//
// TouchpadPinchGesture::TouchpadPinchGesture(int fingerCount, PinchDirection direction)
//     : fingerCount(fingerCount)
//     , direction(direction)
// {
// }
//
// TouchpadRotateGesture::TouchpadRotateGesture(int fingerCount, RotateDirection direction)
//     : fingerCount(fingerCount)
//     , direction(direction)
// {
// }
//
// TouchpadHoldGesture::TouchpadHoldGesture(int fingerCount, std::chrono::milliseconds duration)
//     : fingerCount(fingerCount)
//     , duration(duration)
// {
// }
//
// ApproachScreenBorderGesture::ApproachScreenBorderGesture(ScreenBorder border)
//     : border(border)
// {
// }
//
// TouchscreenSwipeGesture::TouchscreenSwipeGesture(int fingerCount, SwipeDirection direction)
//     : fingerCount(fingerCount)
//     , direction(direction)
// {
// }
//
// TouchscreenSwipe2DGesture::TouchscreenSwipe2DGesture(int fingerCount)
//     : fingerCount(fingerCount)
// {
// }
//
// TouchscreenSwipeFromEdgeGesture::TouchscreenSwipeFromEdgeGesture(int fingerCount, EdgeSwipeDirection direction)
//     : fingerCount(fingerCount)
//     , direction(direction)
// {
// }
//
// TouchscreenPinchGesture::TouchscreenPinchGesture(int fingerCount, PinchDirection direction)
//     : fingerCount(fingerCount)
//     , direction(direction)
// {
// }
//
// TouchscreenRotateGesture::TouchscreenRotateGesture(int fingerCount, RotateDirection direction)
//     : fingerCount(fingerCount)
//     , direction(direction)
// {
// }
//
// TouchscreenHoldGesture::TouchscreenHoldGesture(int fingerCount, std::chrono::milliseconds duration)
//     : fingerCount(fingerCount)
//     , duration(duration)
// {
// }
//
// PointerAxisGesture::PointerAxisGesture(PointerAxisDirection direction, MouseButtonRequirement button)
//     : direction(direction)
//     , button(button)
// {
// }
//
// LineShapeGesture::LineShapeGesture(QList<QPointF> points)
//     : points(std::move(points))
// {
// }

} // namespace KGlobalShortcutTriggerTypes

//
// KGlobalShortcutTrigger

KGlobalShortcutTrigger::KGlobalShortcutTrigger()
    : d(new KGlobalShortcutTriggerPrivate(KGlobalShortcutTriggerPrivate::Unparseable{}))
{
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const QString &serialized)
    : d(new KGlobalShortcutTriggerPrivate(KGlobalShortcutTriggerPrivate::Uninitialized{}))
{
    d->serialized = serialized;
    // variant parsing will happen later in deserialize()
}

KGlobalShortcutTrigger::~KGlobalShortcutTrigger()
{
    delete d;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const KGlobalShortcutTrigger &rhs)
    : d(new KGlobalShortcutTriggerPrivate(rhs.d->variant))
{
    d->serialized = rhs.d->serialized;
}

KGlobalShortcutTrigger &KGlobalShortcutTrigger::operator=(const KGlobalShortcutTrigger &rhs)
{
    KGlobalShortcutTrigger tmp(rhs);
    KGlobalShortcutTriggerPrivate *swap;
    swap = d;
    d = tmp.d;
    tmp.d = swap;
    return *this;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const KeyboardShortcut &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    d->serialized = sc.keySequence.toString(QKeySequence::PortableText);
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchpadSwipeGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto direction = QString::fromLatin1(QMetaEnum::fromType<SwipeDirection>().valueToKey(static_cast<quint64>(sc.direction)));
    d->serialized = TouchpadSwipePrefix % ":"_L1 % QString::number(sc.fingerCount) % ":"_L1 % direction;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchpadSwipe2DGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    d->serialized = TouchpadSwipe2DPrefix % ":"_L1 % QString::number(sc.fingerCount);
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchpadPinchGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto direction = QString::fromLatin1(QMetaEnum::fromType<PinchDirection>().valueToKey(static_cast<quint64>(sc.direction)));
    d->serialized = TouchpadPinchPrefix % ":"_L1 % QString::number(sc.fingerCount) % ":"_L1 % direction;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchpadRotateGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto direction = QString::fromLatin1(QMetaEnum::fromType<RotateDirection>().valueToKey(static_cast<quint64>(sc.direction)));
    d->serialized = TouchpadRotatePrefix % ":"_L1 % QString::number(sc.fingerCount) % ":"_L1 % direction;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchpadHoldGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    d->serialized = TouchpadHoldPrefix % ":"_L1 % QString::number(sc.fingerCount) % ":"_L1 % QString::number(sc.duration.count());
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const ApproachScreenBorderGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto border = QString::fromLatin1(QMetaEnum::fromType<ScreenBorder>().valueToKey(static_cast<quint64>(sc.border)));
    d->serialized = ApproachScreenBorderPrefix % ":"_L1 % border;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchscreenSwipeGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto direction = QString::fromLatin1(QMetaEnum::fromType<SwipeDirection>().valueToKey(static_cast<quint64>(sc.direction)));
    d->serialized = TouchscreenSwipePrefix % ":"_L1 % QString::number(sc.fingerCount) % ":"_L1 % direction;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchscreenSwipe2DGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    d->serialized = TouchscreenSwipe2DPrefix % ":"_L1 % QString::number(sc.fingerCount);
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchscreenSwipeFromEdgeGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto edge = QString::fromLatin1(QMetaEnum::fromType<EdgeSwipeDirection>().valueToKey(static_cast<quint64>(sc.edge)));
    d->serialized = TouchscreenSwipeFromEdgePrefix % ":"_L1 % edge;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchscreenPinchGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto direction = QString::fromLatin1(QMetaEnum::fromType<PinchDirection>().valueToKey(static_cast<quint64>(sc.direction)));
    d->serialized = TouchscreenPinchPrefix % ":"_L1 % QString::number(sc.fingerCount) % ":"_L1 % direction;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchscreenRotateGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto direction = QString::fromLatin1(QMetaEnum::fromType<RotateDirection>().valueToKey(static_cast<quint64>(sc.direction)));
    d->serialized = TouchscreenRotatePrefix % ":"_L1 % QString::number(sc.fingerCount) % ":"_L1 % direction;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const TouchscreenHoldGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    d->serialized = TouchscreenHoldPrefix % ":"_L1 % QString::number(sc.fingerCount) % ":"_L1 % QString::number(sc.duration.count());
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const PointerAxisGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    auto direction = QString::fromLatin1(QMetaEnum::fromType<PointerAxisDirection>().valueToKey(static_cast<quint64>(sc.direction)));
    auto button = QString::fromLatin1(QMetaEnum::fromType<PointerAxisGesture::MouseButtonRequirement>().valueToKey(static_cast<quint64>(sc.button)));
    d->serialized = PointerAxisPrefix % ":"_L1 % direction % ":"_L1 % button;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const LineShapeGesture &sc)
    : d(new KGlobalShortcutTriggerPrivate(sc))
{
    d->serialized = LineShapePrefix;
    d->serialized.reserve(LineShapePrefix.size() + sc.points.size() * 4); // at least one char each for x, y, point delim, axis delim

    if (!sc.points.isEmpty()) {
        d->serialized += QString::number(sc.points[0].x()) % ','_L1 % QString::number(sc.points[0].y());
    }
    for (int i = 1; i < sc.points.size(); ++i) {
        d->serialized += ';'_L1 % QString::number(sc.points[i].x()) % ','_L1 % QString::number(sc.points[i].y());
    }
}

QString KGlobalShortcutTrigger::toString() const
{
    // For keyboard shortcuts, d->serialized is the key sequence.
    // For gestures/non-key triggers, d->serialized already includes the non-key trigger prefix.
    return d->serialized;
}

bool KGlobalShortcutTrigger::isEmpty() const
{
    return d->serialized.isEmpty();
}

bool KGlobalShortcutTrigger::isKnownTriggerType() const
{
    d->deserialize();
    return !std::holds_alternative<KGlobalShortcutTriggerPrivate::Unparseable>(d->variant);
}

bool KGlobalShortcutTrigger::operator==(const KGlobalShortcutTrigger &rhs) const
{
    return d->serialized == rhs.d->serialized;
}

bool KGlobalShortcutTrigger::canShadow(const KGlobalShortcutTrigger &other) const
{
    if (auto kbsc = asKeyboardShortcut(); kbsc != nullptr) {
        if (auto otherKbsc = other.asKeyboardShortcut(); otherKbsc != nullptr) {
            return Utils::contains(kbsc->normalizedKeySequence, otherKbsc->normalizedKeySequence);
        }
    }
    return false;
}

bool KGlobalShortcutTrigger::conflictsWith(const KGlobalShortcutTrigger &other) const
{
    if (isEmpty() || other.isEmpty()) {
        return false;
    }
    if (auto kbsc = asKeyboardShortcut(); kbsc != nullptr) {
        if (auto otherKbsc = other.asKeyboardShortcut(); otherKbsc != nullptr) {
            if (kbsc->keySequence.matches(otherKbsc->keySequence) == QKeySequence::ExactMatch //
                || Utils::contains(kbsc->keySequence, otherKbsc->keySequence) //
                || Utils::contains(otherKbsc->keySequence, kbsc->keySequence)) {
                return true;
            }
        }
    } else if (d->serialized == other.d->serialized) {
        return true;
    }
    return false;
}

const KeyboardShortcut *KGlobalShortcutTrigger::asKeyboardShortcut() const
{
    d->deserialize();
    return std::get_if<KeyboardShortcut>(&d->variant);
}

const TouchpadSwipeGesture *KGlobalShortcutTrigger::asTouchpadSwipeGesture() const
{
    d->deserialize();
    return std::get_if<TouchpadSwipeGesture>(&d->variant);
}

const TouchpadSwipe2DGesture *KGlobalShortcutTrigger::asTouchpadSwipe2DGesture() const
{
    d->deserialize();
    return std::get_if<TouchpadSwipe2DGesture>(&d->variant);
}

const TouchpadPinchGesture *KGlobalShortcutTrigger::asTouchpadPinchGesture() const
{
    d->deserialize();
    return std::get_if<TouchpadPinchGesture>(&d->variant);
}

const TouchpadRotateGesture *KGlobalShortcutTrigger::asTouchpadRotateGesture() const
{
    d->deserialize();
    return std::get_if<TouchpadRotateGesture>(&d->variant);
}

const TouchpadHoldGesture *KGlobalShortcutTrigger::asTouchpadHoldGesture() const
{
    d->deserialize();
    return std::get_if<TouchpadHoldGesture>(&d->variant);
}

const ApproachScreenBorderGesture *KGlobalShortcutTrigger::asApproachScreenBorderGesture() const
{
    d->deserialize();
    return std::get_if<ApproachScreenBorderGesture>(&d->variant);
}

const TouchscreenSwipeGesture *KGlobalShortcutTrigger::asTouchscreenSwipeGesture() const
{
    d->deserialize();
    return std::get_if<TouchscreenSwipeGesture>(&d->variant);
}

const TouchscreenSwipe2DGesture *KGlobalShortcutTrigger::asTouchscreenSwipe2DGesture() const
{
    d->deserialize();
    return std::get_if<TouchscreenSwipe2DGesture>(&d->variant);
}

const TouchscreenSwipeFromEdgeGesture *KGlobalShortcutTrigger::asTouchscreenSwipeFromEdgeGesture() const
{
    d->deserialize();
    return std::get_if<TouchscreenSwipeFromEdgeGesture>(&d->variant);
}

const TouchscreenPinchGesture *KGlobalShortcutTrigger::asTouchscreenPinchGesture() const
{
    d->deserialize();
    return std::get_if<TouchscreenPinchGesture>(&d->variant);
}

const TouchscreenRotateGesture *KGlobalShortcutTrigger::asTouchscreenRotateGesture() const
{
    d->deserialize();
    return std::get_if<TouchscreenRotateGesture>(&d->variant);
}

const TouchscreenHoldGesture *KGlobalShortcutTrigger::asTouchscreenHoldGesture() const
{
    d->deserialize();
    return std::get_if<TouchscreenHoldGesture>(&d->variant);
}

const PointerAxisGesture *KGlobalShortcutTrigger::asPointerAxisGesture() const
{
    d->deserialize();
    return std::get_if<PointerAxisGesture>(&d->variant);
}

const LineShapeGesture *KGlobalShortcutTrigger::asLineShapeGesture() const
{
    d->deserialize();
    return std::get_if<LineShapeGesture>(&d->variant);
}

static std::optional<KGlobalShortcutTriggerPrivate::TriggerVariant> parseSwipeGesture(QStringView triggerString)
{
    enum Type {
        Touchpad,
        Touchscreen
    } type;

    if (triggerString.startsWith(TouchpadSwipePrefix)) {
        triggerString = triggerString.sliced(TouchpadSwipePrefix.size());
        type = Touchpad;
    } else if (triggerString.startsWith(TouchscreenSwipePrefix)) {
        triggerString = triggerString.sliced(TouchscreenSwipePrefix.size());
        type = Touchscreen;
    } else {
        return std::nullopt;
    }

    QList<QStringView> params = triggerString.tokenize(u':').toContainer();
    if (params.size() < 2) {
        return std::nullopt;
    }

    bool ok = false;
    int fingerCount = params[0].toInt(&ok);
    if (!ok) {
        return std::nullopt;
    }

    ok = false;
    auto direction = static_cast<SwipeDirection>(QMetaEnum::fromType<SwipeDirection>().keyToValue(params[1].toLatin1().data(), &ok));
    if (!ok) {
        return std::nullopt;
    }

    // TODO: parse activation requirements

    if (type == Touchpad) {
        return {TouchpadSwipeGesture{.fingerCount = fingerCount, .direction = direction}};
    } else {
        return {TouchscreenSwipeGesture{.fingerCount = fingerCount, .direction = direction}};
    }
}

static std::optional<KGlobalShortcutTriggerPrivate::TriggerVariant> parsePinchGesture(QStringView triggerString)
{
    enum Type {
        Touchpad,
        Touchscreen
    } type;

    if (triggerString.startsWith(TouchpadPinchPrefix)) {
        triggerString = triggerString.sliced(TouchpadPinchPrefix.size());
        type = Touchpad;
    } else if (triggerString.startsWith(TouchscreenPinchPrefix)) {
        triggerString = triggerString.sliced(TouchscreenPinchPrefix.size());
        type = Touchscreen;
    } else {
        return std::nullopt;
    }

    QList<QStringView> params = triggerString.tokenize(u':').toContainer();
    if (params.size() < 2) {
        return std::nullopt;
    }

    bool ok = false;
    int fingerCount = params[0].toInt(&ok);
    if (!ok) {
        return std::nullopt;
    }

    ok = false;
    auto direction = static_cast<PinchDirection>(QMetaEnum::fromType<PinchDirection>().keyToValue(params[1].toLatin1().data(), &ok));
    if (!ok) {
        return std::nullopt;
    }

    if (type == Touchpad) {
        return {TouchpadPinchGesture{.fingerCount = fingerCount, .direction = direction}};
    } else {
        return {TouchscreenPinchGesture{.fingerCount = fingerCount, .direction = direction}};
    }
}

void KGlobalShortcutTriggerPrivate::deserialize()
{
    if (!std::holds_alternative<KGlobalShortcutTriggerPrivate::Uninitialized>(variant)) {
        return;
    }

    // Empty strings and key sequences are the common case, serialized without a prefix.
    // Gestures and any other triggers must be explicitly prefixed, e.g. "T:TouchpadSwipe:3:Up"
    if (serialized.isEmpty()) {
        variant = KGlobalShortcutTriggerPrivate::Unparseable{};
        return;
    } else if (!serialized.startsWith(NonKeyTriggerPrefix)) {
        // QKeySequence::fromString() will parse basically anything, no point in checking its result
        variant = KeyboardShortcut(QKeySequence::fromString(serialized, QKeySequence::PortableText));
        return;
    }

    const QStringView triggerString = QStringView(serialized).sliced(NonKeyTriggerPrefix.size());

    if (auto optVariant = parseSwipeGesture(triggerString); optVariant.has_value()) {
        variant = *optVariant;
    } else if (auto optVariant = parsePinchGesture(triggerString); optVariant.has_value()) {
        variant = *optVariant;
    } // TODO: parse more variants (and figure out activation requirements parsing)
    else {
        variant = KGlobalShortcutTriggerPrivate::Unparseable{};
    }
}

// static
QList<KGlobalShortcutTrigger> KGlobalShortcutTrigger::fromKeyboardShortcuts(const QList<QKeySequence> &keys)
{
    QList<KGlobalShortcutTrigger> result;
    std::ranges::transform(keys, std::back_inserter(result), [](const auto &key) -> KGlobalShortcutTrigger {
        return {KeyboardShortcut{key}};
    });
    return result;
}

// static
QList<QKeySequence> KGlobalShortcutTrigger::onlyKeyboardShortcuts(const QList<KGlobalShortcutTrigger> &triggers)
{
    QList<QKeySequence> result;

    for (const KGlobalShortcutTrigger &trigger : triggers) {
        if (auto kbsc = trigger.asKeyboardShortcut(); kbsc != nullptr) {
            result.append(kbsc->keySequence);
        }
    };
    return result;
}

#include "moc_kglobalshortcuttrigger.cpp"
