/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalaccel_interface.h"
#include "globalshortcutsregistry.h"

class KGlobalAccelInterface::Private
{
public:
    Private(GlobalShortcutsRegistry *owner)
        : owner(owner)
    {
    }
    GlobalShortcutsRegistry *owner;
};

KGlobalAccelInterface::KGlobalAccelInterface(QObject *owner)
    : QObject(owner)
    , d(new Private(qobject_cast<GlobalShortcutsRegistry *>(owner)))
{
}

KGlobalAccelInterface::~KGlobalAccelInterface() = default;

void KGlobalAccelInterface::setRegistry(GlobalShortcutsRegistry *registry)
{
    setParent(registry);
    d->owner = registry;
}

void KGlobalAccelInterface::syncWindowingSystem()
{
}

bool KGlobalAccelInterface::keyPressed(int keyQt)
{
    return d->owner->keyPressed(keyQt);
}

void KGlobalAccelInterface::grabKeys()
{
    d->owner->grabKeys();
}

void KGlobalAccelInterface::ungrabKeys()
{
    d->owner->ungrabKeys();
}

KGlobalAccelInterfaceV2::KGlobalAccelInterfaceV2(QObject *parent)
    : KGlobalAccelInterface(parent)
{
}

bool KGlobalAccelInterfaceV2::keyReleased(int keyQt)
{
    return d->owner->keyReleased(keyQt);
}
