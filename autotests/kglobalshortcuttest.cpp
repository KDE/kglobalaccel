/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2007 Andreas Hartmetz <ahartmetz@gmail.com>
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalshortcuttest.h"
#include "sequencehelpers_p.h"
#include <QAction>
#include <QDBusInterface>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QThread>
#include <kglobalaccel.h>

#include <qplatformdefs.h>

#include <QDBusConnectionInterface>

#ifdef HAVE_XCB_XTEST
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <X11/keysymdef.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xtest.h>
#endif

/*
 * NOTE
 * ----
 * These tests could be better. They don't include actually triggering actions,
 * and we just choose very improbable shortcuts to avoid conflicts with real
 * applications' shortcuts.
 *
 */

const QKeySequence sequenceA = QKeySequence(Qt::SHIFT | Qt::META | Qt::CTRL | Qt::ALT | Qt::Key_F12);
const QKeySequence sequenceB = QKeySequence(Qt::Key_F29);
const QKeySequence sequenceC = QKeySequence(Qt::SHIFT | Qt::META | Qt::CTRL | Qt::Key_F28);
const QKeySequence sequenceD = QKeySequence(Qt::META | Qt::ALT | Qt::Key_F30);
const QKeySequence sequenceE = QKeySequence(Qt::META | Qt::Key_F29);
const QKeySequence sequenceF = QKeySequence(Qt::META | Qt::Key_F27);

// we need a GUI so that the implementation can grab keys
QTEST_MAIN(KGlobalShortcutTest)

void KGlobalShortcutTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    m_daemonInstalled = true;

    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus->isServiceRegistered(QStringLiteral("org.kde.kglobalaccel"))) {
        QDBusReply<void> reply = bus->startService(QStringLiteral("org.kde.kglobalaccel"));
        if (!reply.isValid()) {
            m_daemonInstalled = false;
        }
    }
}

/**
 * NOTE: This method alters a KDE config file in your home directory.
 *
 * KDE4: ~/.kde4/share/config/kglobalshortcutsrc
 * KF5: ~/.config/kglobalshortcutsrc
 *
 * At least on KDE4 this file cannot be modified by hand if the Plasma session
 * is running. So in case you have to clean the file of spurious entries
 * (e.g. because of broken or failing test cases)
 * you have to logout from Plasma.
 *
 * The following sections are created and normally removed automatically:
 * [qttest]
 */
void KGlobalShortcutTest::setupTest(const QString &id)
{
    QString componentName = "qttest";

    if (m_actionA) {
        KGlobalAccel::self()->removeAllShortcuts(m_actionA);
        delete m_actionA;
    }

    if (m_actionB) {
        KGlobalAccel::self()->removeAllShortcuts(m_actionB);
        delete m_actionB;
    }

    // Ensure that the previous test did cleanup correctly
#if KGLOBALACCEL_ENABLE_DEPRECATED_SINCE(4, 2)
    KGlobalAccel *kga = KGlobalAccel::self();
    QList<QStringList> components = kga->allMainComponents();
    QStringList componentId;
    componentId << componentName << QString() << "KDE Test Program" << QString();
    // QVERIFY(!components.contains(componentId));
#endif

    m_actionA = new QAction("Text For Action A", this);
    m_actionA->setObjectName("Action A:" + id);
    m_actionA->setProperty("componentName", componentName);
    m_actionA->setProperty("componentDisplayName", "KDE Test Program");
    KGlobalAccel::self()->setShortcut(m_actionA, QList<QKeySequence>() << sequenceA << sequenceB, KGlobalAccel::NoAutoloading);
    KGlobalAccel::self()->setDefaultShortcut(m_actionA, QList<QKeySequence>() << sequenceA << sequenceB, KGlobalAccel::NoAutoloading);

    m_actionB = new QAction("Text For Action B", this);
    m_actionB->setObjectName("Action B:" + id);
    m_actionB->setProperty("componentName", componentName);
    m_actionB->setProperty("componentDisplayName", "KDE Test Program");
    KGlobalAccel::self()->setShortcut(m_actionB, QList<QKeySequence>(), KGlobalAccel::NoAutoloading);
    KGlobalAccel::self()->setDefaultShortcut(m_actionB, QList<QKeySequence>(), KGlobalAccel::NoAutoloading);
}

void KGlobalShortcutTest::testSetShortcut()
{
    setupTest("testSetShortcut");

    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }

    // Just ensure that the desired values are set for both actions
    QList<QKeySequence> cutA;
    cutA << sequenceA << sequenceB;

    QCOMPARE(KGlobalAccel::self()->shortcut(m_actionA), cutA);
    QCOMPARE(KGlobalAccel::self()->defaultShortcut(m_actionA), cutA);

    QVERIFY(KGlobalAccel::self()->shortcut(m_actionB).isEmpty());
    QVERIFY(KGlobalAccel::self()->defaultShortcut(m_actionB).isEmpty());
}

void KGlobalShortcutTest::testActivateShortcut()
{
#ifdef HAVE_XCB_XTEST
    if (!QX11Info::isPlatformX11()) {
        QSKIP("This test can only be run on platform xcb");
    }

    setupTest("testActivateShortcut");
    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }
    QSignalSpy actionASpy(m_actionA, &QAction::triggered);
    QVERIFY(actionASpy.isValid());

    xcb_connection_t *c = QX11Info::connection();
    xcb_window_t w = QX11Info::appRootWindow();

    xcb_key_symbols_t *syms = xcb_key_symbols_alloc(c);
    auto getCode = [syms](int code) {
        xcb_keycode_t *keyCodes = xcb_key_symbols_get_keycode(syms, code);
        const xcb_keycode_t ret = keyCodes[0];
        free(keyCodes);
        return ret;
    };
    const xcb_keycode_t shift = getCode(XK_Shift_L);
    const xcb_keycode_t meta = getCode(XK_Super_L);
    const xcb_keycode_t control = getCode(XK_Control_L);
    const xcb_keycode_t alt = getCode(XK_Alt_L);
    const xcb_keycode_t f12 = getCode(XK_F12);
    xcb_key_symbols_free(syms);

    xcb_test_fake_input(c, XCB_KEY_PRESS, meta, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_test_fake_input(c, XCB_KEY_PRESS, control, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_test_fake_input(c, XCB_KEY_PRESS, alt, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_test_fake_input(c, XCB_KEY_PRESS, shift, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_test_fake_input(c, XCB_KEY_PRESS, f12, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);

    xcb_test_fake_input(c, XCB_KEY_RELEASE, f12, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_test_fake_input(c, XCB_KEY_RELEASE, shift, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_test_fake_input(c, XCB_KEY_RELEASE, meta, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_test_fake_input(c, XCB_KEY_RELEASE, control, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_test_fake_input(c, XCB_KEY_RELEASE, alt, XCB_TIME_CURRENT_TIME, w, 0, 0, 0);
    xcb_flush(c);

    QVERIFY(actionASpy.wait());
    QCOMPARE(actionASpy.count(), 1);
#else
    QSKIP("This test requires to be compiled with XCB-XTEST");
#endif
}

// Current state
// m_actionA: (sequenceA, sequenceB)
// m_actionB: (,)

void KGlobalShortcutTest::testFindActionByKey()
{
    // Skip this. The above testcase hasn't changed the actions
    setupTest("testFindActionByKey");
    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }

#if KGLOBALACCEL_ENABLE_DEPRECATED_SINCE(5, 90)
    QList<KGlobalShortcutInfo> actionId = KGlobalAccel::self()->getGlobalShortcutsByKey(sequenceB);
#else
    QList<KGlobalShortcutInfo> actionId = KGlobalAccel::self()->globalShortcutsByKey(sequenceB);
#endif

    QCOMPARE(actionId.size(), 1);

    QString actionIdAComponentUniqueName("qttest");
    QString actionIdAUniqueName("Action A:testFindActionByKey");
    QString actionIdAComponentFriendlyName("KDE Test Program");
    QString actionIdAFriendlyName("Text For Action A");

    QCOMPARE(actionId.first().componentUniqueName(), actionIdAComponentUniqueName);
    QCOMPARE(actionId.first().uniqueName(), actionIdAUniqueName);
    QCOMPARE(actionId.first().componentFriendlyName(), actionIdAComponentFriendlyName);
    QCOMPARE(actionId.first().friendlyName(), actionIdAFriendlyName);

#if KGLOBALACCEL_ENABLE_DEPRECATED_SINCE(5, 90)
    actionId = KGlobalAccel::self()->getGlobalShortcutsByKey(sequenceA);
#else
    actionId = KGlobalAccel::self()->globalShortcutsByKey(sequenceA);
#endif

    QCOMPARE(actionId.size(), 1);

    QCOMPARE(actionId.first().componentUniqueName(), actionIdAComponentUniqueName);
    QCOMPARE(actionId.first().uniqueName(), actionIdAUniqueName);
    QCOMPARE(actionId.first().componentFriendlyName(), actionIdAComponentFriendlyName);
    QCOMPARE(actionId.first().friendlyName(), actionIdAFriendlyName);
}

void KGlobalShortcutTest::testChangeShortcut()
{
    // Skip this. The above testcase hasn't changed the actions
    setupTest("testChangeShortcut");

    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }
    // Change the shortcut
    KGlobalAccel::self()->setShortcut(m_actionA, QList<QKeySequence>() << sequenceC, KGlobalAccel::NoAutoloading);
    // Ensure that the change is active
    QCOMPARE(KGlobalAccel::self()->shortcut(m_actionA), QList<QKeySequence>() << sequenceC);
    // We haven't changed the default shortcut, ensure it is unchanged
    QList<QKeySequence> cutA;
    cutA << sequenceA << sequenceB;
    QCOMPARE(KGlobalAccel::self()->defaultShortcut(m_actionA), cutA);

    // Try to set a already take shortcut
    QList<QKeySequence> cutB;
    cutB << KGlobalAccel::self()->shortcut(m_actionA).first() << QKeySequence(sequenceE);
    KGlobalAccel::self()->setShortcut(m_actionB, cutB, KGlobalAccel::NoAutoloading);
    // Ensure that no change was made to the primary active shortcut
    QVERIFY(KGlobalAccel::self()->shortcut(m_actionB).first().isEmpty());
    // Ensure that the change to the secondary active shortcut was made
    QCOMPARE(KGlobalAccel::self()->shortcut(m_actionB).at(1), QKeySequence(sequenceE));
    // Ensure that the default shortcut is still empty
    QVERIFY(KGlobalAccel::self()->defaultShortcut(m_actionB).isEmpty()); // unchanged

    // Only change the active shortcut
    cutB[0] = sequenceD;
    KGlobalAccel::self()->setShortcut(m_actionB, cutB, KGlobalAccel::NoAutoloading);
    // Check that the change went through
    QCOMPARE(KGlobalAccel::self()->shortcut(m_actionB), cutB);
    // Check that the default shortcut is not active
    QVERIFY(KGlobalAccel::self()->defaultShortcut(m_actionB).isEmpty()); // unchanged

    struct SeqInfo {
        QKeySequence seqA;
        QKeySequence seqB;
        bool expected;
    };

    static const SeqInfo values[] = {
        {QKeySequence('A', 'B', 'C'), QKeySequence('A', 'B', 'C'), true},
        {QKeySequence('A', 'B', 'C'), QKeySequence('B', 'C'), true},
        {QKeySequence('A', 'B', 'C'), QKeySequence('A', 'B'), true},
        {QKeySequence('A', 'B', 'C'), QKeySequence('B'), true},
        {QKeySequence('A', 'B', 'C'), QKeySequence('D', 'B', 'C'), false},
        {QKeySequence('A', 'B', 'C'), QKeySequence('A', 'B', 'D'), false},
        {QKeySequence('A', 'B', 'C'), QKeySequence('D'), false},
        {QKeySequence('A', 'B', 'B', 'C'), QKeySequence('B', 'B'), true},
        {QKeySequence('A', 'B', 'C', 'D'), QKeySequence('A', 'B'), true},
        {QKeySequence('A', 'B', 'C', 'D'), QKeySequence('B', 'C'), true},
        {QKeySequence('A', 'B', 'C', 'D'), QKeySequence('C', 'D'), true},
    };

    for (const auto &info : values) {
        KGlobalAccel::self()->removeAllShortcuts(m_actionA);
        KGlobalAccel::self()->setGlobalShortcut(m_actionA, info.seqA);

        cutB.clear();
        cutB << info.seqB;
        KGlobalAccel::self()->removeAllShortcuts(m_actionB);
        KGlobalAccel::self()->setGlobalShortcut(m_actionB, info.seqB);

        QList<QKeySequence> seq = KGlobalAccel::self()->shortcut(m_actionB);
        if (!info.expected) {
            QCOMPARE(seq, cutB);
        } else {
            QVERIFY(seq.first().isEmpty());
        }
    }
}

void KGlobalShortcutTest::testStealShortcut()
{
    setupTest("testStealShortcut");
    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }

    // Steal a shortcut from an action. First ensure the initial state is
    // correct
    QList<QKeySequence> cutA;
    cutA << sequenceA << sequenceB;
    QCOMPARE(KGlobalAccel::self()->shortcut(m_actionA), cutA);
    QCOMPARE(KGlobalAccel::self()->defaultShortcut(m_actionA), cutA);

    KGlobalAccel::stealShortcutSystemwide(sequenceA);
    // let DBus do its thing in case it happens asynchronously
    QTest::qWait(200);
    QList<QKeySequence> shortcuts = KGlobalAccel::self()->shortcut(m_actionA);
    QVERIFY(!shortcuts.isEmpty());
    QVERIFY(shortcuts.first().isEmpty());
}

void KGlobalShortcutTest::testSaveRestore()
{
    setupTest("testSaveRestore");
    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }

    // It /would be nice/ to test persistent storage. That is not so easy...
    QList<QKeySequence> cutA = KGlobalAccel::self()->shortcut(m_actionA);
    // Delete the action
    delete m_actionA;

    // Recreate it
    m_actionA = new QAction("Text For Action A", this);
    m_actionA->setObjectName("Action A:testSaveRestore");
    m_actionA->setProperty("componentName", "qttest");
    m_actionA->setProperty("componentDisplayName", "KDE Test Program");

    // Now it's empty
    QVERIFY(KGlobalAccel::self()->shortcut(m_actionA).isEmpty());

    KGlobalAccel::self()->setShortcut(m_actionA, QList<QKeySequence>());
    // Now it's restored
    QCOMPARE(KGlobalAccel::self()->shortcut(m_actionA), cutA);

    // And again
    delete m_actionA;
    m_actionA = new QAction("Text For Action A", this);
    m_actionA->setObjectName("Action A:testSaveRestore");
    m_actionA->setProperty("componentName", "qttest");
    m_actionA->setProperty("componentDisplayName", "KDE Test Program");
    KGlobalAccel::self()->setShortcut(m_actionA, QList<QKeySequence>() << QKeySequence() << (cutA.isEmpty() ? QKeySequence() : cutA.first()));
    QCOMPARE(KGlobalAccel::self()->shortcut(m_actionA), cutA);
}

// Duplicated again!
enum actionIdFields {
    ComponentUnique = 0,
    ActionUnique = 1,
    ComponentFriendly = 2,
    ActionFriendly = 3,
};

void KGlobalShortcutTest::testListActions()
{
    setupTest("testListActions");
    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }

    // As in kdebase/workspace/kcontrol/keys/globalshortcuts.cpp
#if KGLOBALACCEL_ENABLE_DEPRECATED_SINCE(4, 2)
    KGlobalAccel *kga = KGlobalAccel::self();
    QList<QStringList> components = kga->allMainComponents();
    // qDebug() << components;
    QStringList componentId;
    componentId << "qttest" << QString() << "KDE Test Program" << QString();
    QVERIFY(components.contains(componentId));
#endif

#if KGLOBALACCEL_ENABLE_DEPRECATED_SINCE(4, 2)
    QList<QStringList> actions = kga->allActionsForComponent(componentId);
    QVERIFY(!actions.isEmpty());
    QStringList actionIdA;
    actionIdA << "qttest"
              << "Action A:testListActions"
              << "KDE Test Program"
              << "Text For Action A";
    QStringList actionIdB;
    actionIdB << "qttest"
              << "Action B:testListActions"
              << "KDE Test Program"
              << "Text For Action B";
    // qDebug() << actions;
    QVERIFY(actions.contains(actionIdA));
    QVERIFY(actions.contains(actionIdB));
#endif
}

void KGlobalShortcutTest::testComponentAssignment()
{
    // We don't use them here
    // setupTest();

    QString otherComponent("test_component1");
    QList<QKeySequence> cutB;
    /************************************************************
     * Ensure that the actions get a correct component assigned *
     ************************************************************/
    // Action without component name get the global component
    {
        QAction action("Text For Action A", nullptr);
        action.setObjectName("Action C");

        QCOMPARE(action.property("componentName").toString(), QString());
        KGlobalAccel::self()->setShortcut(&action, cutB, KGlobalAccel::NoAutoloading);
        QCOMPARE(action.property("componentName").toString(), QString());
        // cleanup
        KGlobalAccel::self()->removeAllShortcuts(&action);
    }

    // Action with component name keeps its component name
    {
        QAction action("Text for Action C", nullptr);
        action.setObjectName("Action C");
        action.setProperty("componentName", otherComponent);

        QCOMPARE(action.property("componentName").toString(), otherComponent);
        KGlobalAccel::self()->setShortcut(&action, cutB, KGlobalAccel::NoAutoloading);
        QCOMPARE(action.property("componentName").toString(), otherComponent);
        // cleanup
        KGlobalAccel::self()->removeAllShortcuts(&action);
    }
}

void KGlobalShortcutTest::testConfigurationActions()
{
    setupTest("testConfigurationActions");
    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }

    // Create a configuration action
    QAction cfg_action("Text For Action A", nullptr);
    cfg_action.setObjectName("Action A:testConfigurationActions");
    cfg_action.setProperty("isConfigurationAction", true);
    cfg_action.setProperty("componentName", "qttest");
    cfg_action.setProperty("componentDisplayName", "KDE Test Program");
    KGlobalAccel::self()->setShortcut(&cfg_action, QList<QKeySequence>());

    // Check that the configuration action has the correct shortcuts
    QCOMPARE(KGlobalAccel::self()->shortcut(m_actionA), KGlobalAccel::self()->shortcut(&cfg_action));

    // TODO:
    // - change shortcut from configuration action and test for
    //   yourShortcutGotChanged
    // - Ensure that the config action doesn't trigger(how?)
    // - Ensure that the original action is still working when the
    //   configuration action is deleted
}

void KGlobalShortcutTest::testOverrideMainComponentData()
{
    setupTest("testOverrideMainComponentData");

    QString otherComponent("test_component1");
    QList<QKeySequence> cutB;

    // Action without component name
    QAction *action = new QAction("Text For Action A", this);
    QCOMPARE(action->property("componentName").toString(), QString());
    action->setObjectName("Action A");
    KGlobalAccel::self()->setShortcut(action, cutB, KGlobalAccel::NoAutoloading);
    QCOMPARE(action->property("componentName").toString(), QString());

    // Action with component name
    KGlobalAccel::self()->removeAllShortcuts(action);
    delete action;
    action = new QAction("Text For Action A", this);
    action->setObjectName("Action A");
    action->setProperty("componentName", otherComponent);
    QCOMPARE(action->property("componentName").toString(), otherComponent);
    KGlobalAccel::self()->setShortcut(action, cutB, KGlobalAccel::NoAutoloading);
    QCOMPARE(action->property("componentName").toString(), otherComponent);

    // cleanup
    KGlobalAccel::self()->removeAllShortcuts(action);
}

void KGlobalShortcutTest::testNotification()
{
    setupTest("testNotification");

    // Action without component name
    QAction *action = new QAction("Text For Action A", this);
    QCOMPARE(action->property("componentName").toString(), QString());
    action->setObjectName("Action A");
    QList<QKeySequence> cutB;
    KGlobalAccel::self()->setShortcut(action, cutB, KGlobalAccel::NoAutoloading);
    QCOMPARE(action->property("componentName").toString(), QString());

    // kglobalacceld collects registrations and shows the together. Give it
    // time to kick in.
    QThread::sleep(2);

    KGlobalAccel::self()->removeAllShortcuts(action);
}

void KGlobalShortcutTest::testGetGlobalShortcut()
{
    setupTest("testLoadShortcutFromGlobalSettings"); // cleanup see testForgetGlobalShortcut
    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }

    // retrieve shortcut list
    auto shortcutList = KGlobalAccel::self()->globalShortcut("qttest", "Action A:testLoadShortcutFromGlobalSettings");
    QCOMPARE(shortcutList.count(), 2); // see setupTest

    // test for a real shortcut:
    //     shortcutList = KGlobalAccel::self()->shortcut("kwin", "Kill Window");
    //     QCOMPARE(shortcutList.count(), 1);
}

void KGlobalShortcutTest::testMangle()
{
    setupTest("testMangle");
    static const std::pair<QKeySequence, QKeySequence> values[] = {
        {QKeySequence('A', 'B', 'C', 'D'), QKeySequence('A', 'B', 'C', 'D')},
        {QKeySequence("Shift+A,B,C,D"), QKeySequence("Shift+A,B,C,D")},
        {QKeySequence("Shift+Tab,B,C,D"), QKeySequence("Shift+Tab,B,C,D")},
        {QKeySequence("Ctrl+Shift+Tab,B,C,D"), QKeySequence("Ctrl+Shift+Tab,B,C,D")},
        {QKeySequence("Shift+BackTab,B,C,D"), QKeySequence("Shift+Tab,B,C,D")},
        {QKeySequence("Shift+BackTab,Shift+BackTab,Shift+BackTab,Shift+BackTab"), QKeySequence("Shift+Tab,Shift+Tab,Shift+Tab,Shift+Tab")},
        {QKeySequence(), QKeySequence()},
    };

    for (const auto &pair : values) {
        QCOMPARE(Utils::mangleKey(pair.first), pair.second);
    }
}

void KGlobalShortcutTest::testCrop()
{
    setupTest("testCrop");

    struct SeqInfo {
        QKeySequence seq;
        int count = 0;
        QKeySequence expected;
    };

    static const SeqInfo values[] = {
        {QKeySequence('A', 'B', 'C', 'D'), 1, QKeySequence('B', 'C', 'D')},
        {QKeySequence('A', 'B', 'C', 'D'), 2, QKeySequence('C', 'D')},
        {QKeySequence('A', 'B', 'C'), 1, QKeySequence('B', 'C')},
        {QKeySequence('A', 'B', 'C'), 2, QKeySequence('C')},
        {QKeySequence('A'), 1, QKeySequence()},
        {QKeySequence('A'), 2, QKeySequence()},
        {QKeySequence(), 1, QKeySequence()},
        {QKeySequence(), 2, QKeySequence()},
    };

    for (const auto &s : values) {
        QCOMPARE(Utils::cropKey(s.seq, s.count), s.expected);
    }
}

void KGlobalShortcutTest::testReverse()
{
    setupTest("testReverse");
    static const std::pair<QKeySequence, QKeySequence> values[] = {
        {QKeySequence('A', 'B', 'C', 'D'), QKeySequence('D', 'C', 'B', 'A')},
        {QKeySequence('A', 'B', 'C'), QKeySequence('C', 'B', 'A')},
        {QKeySequence('A', 'B'), QKeySequence('B', 'A')},
        {QKeySequence('A'), QKeySequence('A')},
        {QKeySequence(), QKeySequence()},
    };

    for (const auto &pair : values) {
        QCOMPARE(Utils::reverseKey(pair.first), pair.second);
    }
}

void KGlobalShortcutTest::testMatch()
{
    setupTest("Match");

    struct Info {
        QList<QKeySequence> seqList;
        bool isMatch = false;
    };

    static const Info values[] = {
        {QList{QKeySequence('A', 'B', 'C')}, true},
        {QList{QKeySequence('A', 'B', 'D')}, false},
        {QList{QKeySequence('D')}, false},
        {QList{QKeySequence('A', 'B', 'C', 'D')}, true},
        {QList{QKeySequence('D', 'A', 'B', 'C')}, true},
        {QList{QKeySequence('D', 'A', 'B', 'D')}, false},
        {QList{QKeySequence('A', 'B')}, true},
        {QList{QKeySequence('B', 'C')}, true},
        {QList{QKeySequence('A', 'C')}, false},
    };

    const QKeySequence seq = QKeySequence('A', 'B', 'C');
    for (const auto &i : values) {
        QCOMPARE(Utils::matchSequences(seq, i.seqList), i.isMatch);
    }
}

void KGlobalShortcutTest::testForgetGlobalShortcut()
{
    setupTest("testForgetGlobalShortcut");

    // Ensure that forgetGlobalShortcut can be called on any action.
    QAction a("Test", nullptr);
    KGlobalAccel::self()->removeAllShortcuts(&a);
    if (!m_daemonInstalled) {
        QSKIP("kglobalaccel not installed");
    }

    // We forget these two shortcuts and check that the component is gone
    // after that. If not it can mean the forgetGlobalShortcut() call is
    // broken OR someone messed up these tests to leave an additional global
    // shortcut behind.
    KGlobalAccel::self()->removeAllShortcuts(m_actionB);
    KGlobalAccel::self()->removeAllShortcuts(m_actionA);
    // kglobalaccel writes asynchronous.
    QThread::sleep(1);

#if KGLOBALACCEL_ENABLE_DEPRECATED_SINCE(4, 2)
    KGlobalAccel *kga = KGlobalAccel::self();
    QList<QStringList> components = kga->allMainComponents();
    QStringList componentId;
    componentId << "qttest" << QString() << "KDE Test Program" << QString();
    QVERIFY(!components.contains(componentId));
#endif
}
