/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>
    SPDX-FileCopyrightText: 2006 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2007 Andreas Hartmetz <ahartmetz@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KGLOBALACCEL_P_H
#define KGLOBALACCEL_P_H

#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QStringList>

#include "kglobalaccel.h"
#include "kglobalaccel_component_interface.h"
#include "kglobalaccel_interface.h"
#include "kglobalshortcutinfo.h"
#include "kglobalshortcuttrigger.h"

enum SetShortcutFlag {
    SetPresent = 2,
    NoAutoloading = 4,
    IsDefault = 8,
    SupportsOneToOneGesture = KGlobalShortcutInfo::Feature::SupportsOneToOneGesture,
    SupportsFreeform2DGesture = KGlobalShortcutInfo::Feature::SupportsFreeform2DGesture,
};
enum SetInverseActionFlag {
    InverseActionCouplingIsMandatory = KGlobalShortcutInfo::Feature::InverseActionCouplingIsMandatory,
};

class KGlobalAccelPrivate
{
public:
    enum ShortcutState {
        Pressed,
        Repeated,
        Released
    };
    enum ShortcutType {
        /// The shortcut will immediately become active but may be reset to "default".
        ActiveShortcut = 0x1,
        /// The shortcut is a default shortcut - it becomes active when somebody decides to
        /// reset shortcuts to default.
        DefaultShortcut = 0x2,
    };

    Q_DECLARE_FLAGS(ShortcutTypes, ShortcutType)
    enum Removal {
        SetInactive = 0, ///< Forget the action in this class and mark it as not present in the KDED module
        UnRegister, ///< Remove any trace of the action in this class and in the KDED module
    };
    KGlobalAccelPrivate(KGlobalAccel *);

    int ifaceVersion();

    /// Propagate any shortcut changes to the KDED module that does the bookkeeping
    /// and the key grabbing.
    ///@todo KF6
    void updateGlobalShortcut(/*const would be better*/ QAction *action,
                              KGlobalAccelPrivate::ShortcutTypes actionFlags,
                              KGlobalAccel::GestureSupportFlags gestureSupport,
                              KGlobalAccel::GlobalShortcutLoading globalFlags);

    /// Register the action in this class and in the KDED module
    bool doRegister(QAction *action); //"register" is a C keyword :p
    /// cf. the RemoveAction enum
    void remove(QAction *action, Removal r);

    //"private" helpers
    QString componentUniqueForAction(const QAction *action);
    QString componentFriendlyForAction(const QAction *action);
    QStringList makeActionId(const QAction *action);
    QStringList makeActionReference(const QAction *action);
    QList<QKeySequence> shortcutFromIntList(const QList<int> &list);

    void cleanup();

    // private slot implementations
    QAction *findAction(const QString &, const QString &);
    void invokeAction(const QString &, const QString &, qlonglong, ShortcutState wasHeld);
    void invokeDeactivate(const QString &, const QString &);
    void shortcutTriggersChanged(const QStringList &, const QList<KGlobalShortcutTrigger> &);
    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void reRegisterAll();

    // for all actions with (isEnabled() && globalShortcutAllowed())
    QMultiHash<QString, QAction *> nameToAction;
    QSet<QAction *> actions;

    org::kde::KGlobalAccel *iface();

    //! Get the component @p componentUnique. If @p remember is true the instance is cached and we
    //! subscribe to signals about changes to the component.
    org::kde::kglobalaccel::Component *getComponent(const QString &componentUnique, bool remember);

    //! Our owner
    KGlobalAccel *q;

    //! The components the application is using
    QHash<QString, org::kde::kglobalaccel::Component *> components;
    QMap<const QAction *, QList<KGlobalShortcutTrigger>> actionDefaultTriggers;
    QMap<const QAction *, QList<KGlobalShortcutTrigger>> actionTriggers;

    bool setShortcutWithDefault(QAction *action,
                                const QList<QKeySequence> &keys,
                                const QList<KGlobalShortcutTrigger> &extraTriggers,
                                KGlobalAccel::GestureSupportFlags gestureSupport,
                                KGlobalAccel::GlobalShortcutLoading loadFlag);

    void unregister(const QStringList &actionId);
    void setInactive(const QStringList &actionId);

private:
    org::kde::KGlobalAccel *m_iface = nullptr;
    std::optional<int> m_iface_version;
    QPointer<QAction> m_lastActivatedAction;
    QDBusServiceWatcher *m_watcher;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KGlobalAccelPrivate::ShortcutTypes)

#endif
