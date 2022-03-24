/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KGlobalAccel>
#include <QAction>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QTest>

class GlobalAction : public QAction
{
    Q_OBJECT
public:
    GlobalAction()
    {
        connect(this, &QAction::changed, this, &GlobalAction::refresh);
    }

    void refresh()
    {
        if (!m_added && shortcut().isEmpty())
            return;

        m_added = KGlobalAccel::self()->setGlobalShortcut(this, {shortcut()});

        if (!m_added) {
            qWarning() << "could not set the global shortcut" << shortcut();
        } else {
            qDebug() << "shortcut set correctly";
        }
    }

    bool m_added = false;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

    qmlRegisterType<GlobalAction>("org.kde.globalaccel", 1, 0, "GlobalAction");

    engine.load(QFINDTESTDATA("kglobalacceltest.qml"));

    return app.exec();
}

#include "kglobalacceltest.moc"
