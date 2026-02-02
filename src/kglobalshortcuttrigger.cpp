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
constexpr QLatin1StringView LineShapePrefix = "LineShape:"_L1;
constexpr QLatin1StringView ScrollAxisPrefix = "ScrollAxis:"_L1;

KGlobalShortcutTrigger::KGlobalShortcutTrigger()
    : d(new KGlobalShortcutTriggerPrivate(KGlobalShortcutTriggerPrivate::Unparseable{}))
{
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

// static
KGlobalShortcutTrigger KGlobalShortcutTrigger::fromString(const QString &serialized)
{
    if (serialized.isEmpty()) {
        return KGlobalShortcutTrigger();
    }
    KGlobalShortcutTrigger trigger;
    trigger.d->serialized = serialized;
    // variant parsing will happen later in deserialize()
    return trigger;
}

// static
KGlobalShortcutTrigger KGlobalShortcutTrigger::fromKeyboardShortcut(QKeySequence key)
{
    // TODO: better, more de-duplicated implementation of this?
    KGlobalShortcutTrigger trigger;
    KeyboardShortcut sc;
    sc.keySequence = key;
    sc.normalizedKeySequence = Utils::normalizeSequence(key);
    trigger.d->variant = sc;
    trigger.d->serialized = key.toString(QKeySequence::PortableText);
    return trigger;
}

QString KGlobalShortcutTrigger::toString() const
{
    return d->serialized;
}

bool KGlobalShortcutTrigger::isEmpty() const
{
    return d->serialized.isEmpty();
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

const ScrollAxisGesture *KGlobalShortcutTrigger::asScrollAxisGesture() const
{
    d->deserialize();
    return std::get_if<ScrollAxisGesture>(&d->variant);
}

const LineShapeGesture *KGlobalShortcutTrigger::asLineShapeGesture() const
{
    d->deserialize();
    return std::get_if<LineShapeGesture>(&d->variant);
}

static std::optional<KGlobalShortcutTriggerPrivate::TriggerVariant> parseSwipeGesture(QStringView serialized)
{
    enum Type {
        Touchpad,
        Touchscreen
    } type;

    if (serialized.startsWith(TouchpadSwipePrefix)) {
        type = Touchpad;
        serialized.slice(TouchpadSwipePrefix.size());
    } else if (serialized.startsWith(TouchscreenSwipePrefix)) {
        type = Touchscreen;
        serialized.slice(TouchscreenSwipePrefix.size());
    } else {
        return std::nullopt;
    }

    QList<QStringView> params = serialized.tokenize(u':').toContainer();
    if (serialized.length() < 2) {
        return std::nullopt;
    }

    bool ok = false;
    int fingerCount = params[0].toInt(&ok);
    if (!ok) {
        return std::nullopt;
    }

    QMetaEnum metaEnum =
        (type == Touchpad) ? QMetaEnum::fromType<TouchpadSwipeGesture::Direction>() : QMetaEnum::fromType<TouchscreenSwipeGesture::Direction>();
    ok = false;
    int direction = metaEnum.keyToValue(params[1].toLatin1().data(), &ok);
    if (!ok) {
        return std::nullopt;
    }

    // TODO: parse activation requirements

    if (type == Touchpad) {
        return {TouchpadSwipeGesture{
            .fingerCount = fingerCount,
            .direction = static_cast<TouchpadSwipeGesture::Direction>(direction),
        }};
    } else {
        return {TouchscreenSwipeGesture{
            .fingerCount = fingerCount,
            .direction = static_cast<TouchscreenSwipeGesture::Direction>(direction),
        }};
    }
}

void KGlobalShortcutTriggerPrivate::deserialize()
{
    if (!std::holds_alternative<KGlobalShortcutTriggerPrivate::Uninitialized>(variant)) {
        return;
    }

    if (QKeySequence key = QKeySequence::fromString(serialized, QKeySequence::PortableText); !key.isEmpty()) {
        KeyboardShortcut sc;
        sc.keySequence = key;
        sc.normalizedKeySequence = Utils::normalizeSequence(key);
        variant = sc;
    } else if (auto optVariant = parseSwipeGesture(serialized); optVariant.has_value()) {
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
    std::ranges::transform(keys, std::back_inserter(result), &KGlobalShortcutTrigger::fromKeyboardShortcut);
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
