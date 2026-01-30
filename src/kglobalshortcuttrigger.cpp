/*
    SPDX-FileCopyrightText: 2026 Jakob Petsovits <jpetso@petsovits.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalshortcuttrigger.h"
#include "kglobalshortcuttrigger_p.h"

#include "sequencehelpers_p.h"

KGlobalShortcutTrigger::KGlobalShortcutTrigger()
    : d(new KGlobalShortcutTriggerPrivate)
{
}

KGlobalShortcutTrigger::~KGlobalShortcutTrigger()
{
    delete d;
}

KGlobalShortcutTrigger::KGlobalShortcutTrigger(const KGlobalShortcutTrigger &rhs)
    : d(new KGlobalShortcutTriggerPrivate)
{
    d->variant = rhs.d->variant;
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
    KGlobalShortcutTrigger trigger;

    if (serialized.isEmpty()) {
        return KGlobalShortcutTrigger();
    }

    trigger.d->serialized = serialized;

    if (QKeySequence key = QKeySequence::fromString(serialized, QKeySequence::PortableText); !key.isEmpty()) {
        KeyboardShortcut sc;
        sc.keySequence = key;
        sc.normalizedKeySequence = Utils::normalizeSequence(key);
        trigger.d->variant = sc;
    }
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

const KeyboardShortcut *KGlobalShortcutTrigger::asKeyboardShortcut() const
{
    return std::get_if<KeyboardShortcut>(&d->variant);
}

const TouchpadSwipeGesture *KGlobalShortcutTrigger::asTouchpadSwipe() const
{
    return nullptr;
}

const TouchpadSwipe2DGesture *KGlobalShortcutTrigger::asTouchpadSwipe2D() const
{
    return nullptr;
}

const TouchpadPinchGesture *KGlobalShortcutTrigger::asTouchpadPinch() const
{
    return nullptr;
}

const TouchpadRotateGesture *KGlobalShortcutTrigger::asTouchpadRotate() const
{
    return nullptr;
}

const TouchpadHoldGesture *KGlobalShortcutTrigger::asTouchpadHold() const
{
    return nullptr;
}

const ApproachScreenBorderGesture *KGlobalShortcutTrigger::asApproachScreenBorder() const
{
    return nullptr;
}

const TouchscreenSwipeGesture *KGlobalShortcutTrigger::asTouchscreenSwipe() const
{
    return nullptr;
}

const TouchscreenSwipe2DGesture *KGlobalShortcutTrigger::asTouchscreenSwipe2D() const
{
    return nullptr;
}

const TouchscreenSwipeFromEdgeGesture *KGlobalShortcutTrigger::asTouchscreenSwipeFromEdge() const
{
    return nullptr;
}

const TouchscreenPinchGesture *KGlobalShortcutTrigger::asTouchscreenPinch() const
{
    return nullptr;
}

const TouchscreenRotateGesture *KGlobalShortcutTrigger::asTouchscreenRotate() const
{
    return nullptr;
}

const TouchscreenHoldGesture *KGlobalShortcutTrigger::asTouchscreenHold() const
{
    return nullptr;
}

const AxisGesture *KGlobalShortcutTrigger::asAxisGesture() const
{
    return nullptr;
}

const StrokeGesture *KGlobalShortcutTrigger::asStrokeGesture() const
{
    return nullptr;
}

#include "moc_kglobalshortcuttrigger.cpp"
