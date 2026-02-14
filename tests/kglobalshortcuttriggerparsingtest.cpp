/*
    SPDX-FileCopyrightText: 2026 Muhammad Ahmad

    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kglobalshortcuttrigger.h"

#include <QTest>

using namespace Qt::StringLiterals;
using namespace KGlobalShortcutTriggerTypes;

class KGlobalShortcutTriggerParsingTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void parsesTouchpadPinch();
    void rejectsInvalidTouchpadPinch();
    void parsesTouchpadSwipe();
    void requiresTPrefixForNonKeyTriggers(); // new regression test
};

void KGlobalShortcutTriggerParsingTest::parsesTouchpadPinch()
{
    const KGlobalShortcutTrigger t(u"T:TouchpadPinch:3:Expanding"_s);

    QVERIFY(!t.isEmpty());

    const auto pinch = t.asTouchpadPinchGesture();
    QVERIFY(pinch != nullptr);
    QCOMPARE(pinch->fingerCount, 3);
    QCOMPARE(pinch->direction, PinchDirection::Expanding);
}

void KGlobalShortcutTriggerParsingTest::rejectsInvalidTouchpadPinch()
{
    const KGlobalShortcutTrigger t1(u"T:TouchpadPinch:foo:Expanding"_s);
    QVERIFY(t1.asTouchpadPinchGesture() == nullptr);

    const KGlobalShortcutTrigger t2(u"T:TouchpadPinch:3:Zoom"_s);
    QVERIFY(t2.asTouchpadPinchGesture() == nullptr);

    const KGlobalShortcutTrigger t3(u"T:TouchpadPinch:3"_s);
    QVERIFY(t3.asTouchpadPinchGesture() == nullptr);
}

void KGlobalShortcutTriggerParsingTest::parsesTouchpadSwipe()
{
    const KGlobalShortcutTrigger t(u"T:TouchpadSwipe:3:Left"_s);

    QVERIFY(!t.isEmpty());

    const auto swipe = t.asTouchpadSwipeGesture();
    QVERIFY(swipe != nullptr);
    QCOMPARE(swipe->fingerCount, 3);
    QCOMPARE(swipe->direction, SwipeDirection::Left);
}

void KGlobalShortcutTriggerParsingTest::requiresTPrefixForNonKeyTriggers()
{
    // Without "T:", non-key triggers must NOT be interpreted as gestures.
    const KGlobalShortcutTrigger tSwipe(u"TouchpadSwipe:3:Left"_s);
    QVERIFY(tSwipe.asTouchpadSwipeGesture() == nullptr);

    const KGlobalShortcutTrigger tPinch(u"TouchpadPinch:3:Expanding"_s);
    QVERIFY(tPinch.asTouchpadPinchGesture() == nullptr);

    // Without "T:", these strings are parsed as keyboard shortcuts.
    // QKeySequence is useless at rejecting invalid sequences, so we expect *something*
    // to be parsed, even if it makes no sense.
    QVERIFY(!tSwipe.isEmpty());
    QVERIFY(!tPinch.isEmpty());
    QVERIFY(tSwipe.asKeyboardShortcut() != nullptr);
    QVERIFY(tPinch.asKeyboardShortcut() != nullptr);
}

QTEST_MAIN(KGlobalShortcutTriggerParsingTest)
#include "kglobalshortcuttriggerparsingtest.moc"
