/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick.Controls 2.15
import org.kde.kquickcontrols 2.0
import org.kde.globalaccel 1.0

ApplicationWindow
{
    visible: true

    GlobalAction {
        text: "Hola"
        onTriggered: console.log("woah")
        objectName: "org.kde.globalaccel.test.globalacceltest"
        shortcut: sequenceItem.keySequence
    }

    KeySequenceItem
    {
        modifierlessAllowed: true
        id: sequenceItem

    }
}
