/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2007 Andreas Hartmetz <ahartmetz@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KGLOBALSHORTCUTTEST_H
#define KGLOBALSHORTCUTTEST_H

#include <QObject>

class QAction;

class KGlobalShortcutTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testSetShortcut();
    void testActivateShortcut();
    void testFindActionByKey();
    void testChangeShortcut();
    void testStealShortcut();
    void testSaveRestore();
    void testListActions();
    void testComponentAssignment();
    void testConfigurationActions();
    void testNotification();

    void testGetGlobalShortcut();
    void testMangle();
    void testCrop();
    void testReverse();
    void testMatch();

    // This has to be the last before forgetGlobalShortcut
    void testOverrideMainComponentData();

    void testForgetGlobalShortcut(); // clean global config altered by setupTest

public:
    KGlobalShortcutTest()
        : m_actionA(nullptr)
        , m_actionB(nullptr)
    {
    }

private:
    void setupTest(const QString &id);

    QAction *m_actionA;
    QAction *m_actionB;
    bool m_daemonInstalled;
};

#endif
