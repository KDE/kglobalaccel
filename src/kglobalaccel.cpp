/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>
    SPDX-FileCopyrightText: 2006 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2007 Andreas Hartmetz <ahartmetz@gmail.com>
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalaccel.h"
#include "kglobalaccel_debug.h"
#include "kglobalaccel_p.h"

#include <memory>

#include <QAction>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPushButton>
#include <config-kglobalaccel.h>
#if HAVE_X11
#include <QX11Info>
#endif

org::kde::kglobalaccel::Component *KGlobalAccelPrivate::getComponent(const QString &componentUnique, bool remember = false)
{
    // Check if we already have this component
    {
        auto component = components.value(componentUnique);
        if (component) {
            return component;
        }
    }

    // Connect to the kglobalaccel daemon
    org::kde::KGlobalAccel kglobalaccel(QStringLiteral("org.kde.kglobalaccel"), QStringLiteral("/kglobalaccel"), QDBusConnection::sessionBus());
    if (!kglobalaccel.isValid()) {
        qCDebug(KGLOBALACCEL_LOG) << "Failed to connect to the kglobalaccel daemon" << QDBusConnection::sessionBus().lastError();
        return nullptr;
    }

    // Get the path for our component. We have to do that because
    // componentUnique is probably not a valid dbus object path
    QDBusReply<QDBusObjectPath> reply = kglobalaccel.getComponent(componentUnique);
    if (!reply.isValid()) {
        if (reply.error().name() == QLatin1String("org.kde.kglobalaccel.NoSuchComponent")) {
            // No problem. The component doesn't exists. That's normal
            return nullptr;
        }

        // An unknown error.
        qCDebug(KGLOBALACCEL_LOG) << "Failed to get dbus path for component " << componentUnique << reply.error();
        return nullptr;
    }

    // Now get the component
    org::kde::kglobalaccel::Component *component =
        new org::kde::kglobalaccel::Component(QStringLiteral("org.kde.kglobalaccel"), reply.value().path(), QDBusConnection::sessionBus(), q);

    // No component no cleaning
    if (!component->isValid()) {
        qCDebug(KGLOBALACCEL_LOG) << "Failed to get component" << componentUnique << QDBusConnection::sessionBus().lastError();
        return nullptr;
    }

    if (remember) {
        // Connect to the signals we are interested in.
        q->connect(component,
                   &org::kde::kglobalaccel::Component::globalShortcutPressed,
                   q,
                   [this](const QString &componentUnique, const QString &shortcutUnique, qlonglong timestamp) {
                       _k_invokeAction(componentUnique, shortcutUnique, timestamp);
                   });

        components[componentUnique] = component;
    }

    return component;
}

namespace
{
QString serviceName()
{
    return QStringLiteral("org.kde.kglobalaccel");
}
}

void KGlobalAccelPrivate::cleanup()
{
    qDeleteAll(components);
    delete m_iface;
    m_iface = nullptr;
    delete m_watcher;
    m_watcher = nullptr;
}

KGlobalAccelPrivate::KGlobalAccelPrivate(KGlobalAccel *q)
    : q(q)
{
    m_watcher = new QDBusServiceWatcher(serviceName(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, q);
    q->connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged, q, [this](const QString &serviceName, const QString &oldOwner, const QString &newOwner) {
        _k_serviceOwnerChanged(serviceName, oldOwner, newOwner);
    });
}

org::kde::KGlobalAccel *KGlobalAccelPrivate::iface()
{
    if (!m_iface) {
        m_iface = new org::kde::KGlobalAccel(serviceName(), QStringLiteral("/kglobalaccel"), QDBusConnection::sessionBus());
        // Make sure kglobalaccel is running. The iface declaration above somehow works anyway.
        QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
        if (bus && !bus->isServiceRegistered(serviceName())) {
            QDBusReply<void> reply = bus->startService(serviceName());
            if (!reply.isValid()) {
                qCritical() << "Couldn't start kglobalaccel from org.kde.kglobalaccel.service:" << reply.error();
            }
        }
        q->connect(m_iface, &org::kde::KGlobalAccel::yourShortcutGotChanged, q, [this](const QStringList &actionId, const QList<int> &newKeys) {
            _k_shortcutGotChanged(actionId, newKeys);
        });
    }
    return m_iface;
}

KGlobalAccel::KGlobalAccel()
    : d(new KGlobalAccelPrivate(this))
{
    qDBusRegisterMetaType<QList<int>>();
    qDBusRegisterMetaType<QList<QStringList>>();
    qDBusRegisterMetaType<KGlobalShortcutInfo>();
    qDBusRegisterMetaType<QList<KGlobalShortcutInfo>>();
}

KGlobalAccel::~KGlobalAccel()
{
    delete d;
}

void KGlobalAccel::activateGlobalShortcutContext(const QString &contextUnique, const QString &contextFriendly, const QString &programName)
{
    Q_UNUSED(contextFriendly);
    // TODO: provide contextFriendly
    self()->d->iface()->activateGlobalShortcutContext(programName, contextUnique);
}

// static
bool KGlobalAccel::cleanComponent(const QString &componentUnique)
{
    org::kde::kglobalaccel::Component *component = self()->getComponent(componentUnique);
    if (!component) {
        return false;
    }

    return component->cleanUp();
}

// static
bool KGlobalAccel::isComponentActive(const QString &componentUnique)
{
    org::kde::kglobalaccel::Component *component = self()->getComponent(componentUnique);
    if (!component) {
        return false;
    }

    return component->isActive();
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(4, 4)
bool KGlobalAccel::isEnabled() const
{
    return d->enabled;
}
#endif

org::kde::kglobalaccel::Component *KGlobalAccel::getComponent(const QString &componentUnique)
{
    return d->getComponent(componentUnique);
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(4, 4)
void KGlobalAccel::setEnabled(bool enabled)
{
    d->enabled = enabled;
}
#endif

class KGlobalAccelSingleton
{
public:
    KGlobalAccelSingleton();

    KGlobalAccel instance;
};

Q_GLOBAL_STATIC(KGlobalAccelSingleton, s_instance)

KGlobalAccelSingleton::KGlobalAccelSingleton()
{
    qAddPostRoutine([]() {
        s_instance->instance.d->cleanup();
    });
}

KGlobalAccel *KGlobalAccel::self()
{
    return &s_instance()->instance;
}

bool KGlobalAccelPrivate::doRegister(QAction *action)
{
    if (!action || action->objectName().isEmpty() || action->objectName().startsWith(QLatin1String("unnamed-"))) {
        qWarning() << "Attempt to set global shortcut for action without objectName()."
                      " Read the setGlobalShortcut() documentation.";
        return false;
    }

    const bool isRegistered = actions.contains(action);
    if (isRegistered) {
        return true;
    }

    QStringList actionId = makeActionId(action);

    nameToAction.insert(actionId.at(KGlobalAccel::ActionUnique), action);
    actions.insert(action);
    iface()->doRegister(actionId);

    QObject::connect(action, &QObject::destroyed, [this, action](QObject *) {
        if (actions.contains(action) && (actionShortcuts.contains(action) || actionDefaultShortcuts.contains(action))) {
            remove(action, KGlobalAccelPrivate::SetInactive);
        }
    });

    return true;
}

void KGlobalAccelPrivate::remove(QAction *action, Removal removal)
{
    if (!action || action->objectName().isEmpty()) {
        return;
    }

    const bool isRegistered = actions.contains(action);
    if (!isRegistered) {
        return;
    }

    QStringList actionId = makeActionId(action);

    nameToAction.remove(actionId.at(KGlobalAccel::ActionUnique), action);
    actions.remove(action);

    if (removal == UnRegister) {
        // Complete removal of the shortcut is requested
        // (forgetGlobalShortcut)
        unregister(actionId);
    } else {
        // If the action is a configurationAction wen only remove it from our
        // internal registry. That happened above.

        // If we are merely marking a callback as inactive there is nothing for kglobalaccel to do if kglobalaccel is not running
        // this can happen on shutdown where all apps and kglobalaccel are all torn down at once
        // For this reason we turn off the autostart flag in the DBus message call

        if (!action->property("isConfigurationAction").toBool()) {
            // If it's a session shortcut unregister it.
            action->objectName().startsWith(QLatin1String("_k_session:")) ? unregister(actionId) : setInactive(actionId);
        }
    }

    actionDefaultShortcuts.remove(action);
    actionShortcuts.remove(action);
}

void KGlobalAccelPrivate::unregister(const QStringList &actionId)
{
    const auto component = actionId.at(KGlobalAccel::ComponentUnique);
    const auto action = actionId.at(KGlobalAccel::ActionUnique);

    auto message = QDBusMessage::createMethodCall(iface()->service(), iface()->path(), iface()->interface(), QStringLiteral("unregister"));
    message.setArguments({component, action});
    message.setAutoStartService(false);
    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
}

void KGlobalAccelPrivate::setInactive(const QStringList &actionId)
{
    auto message = QDBusMessage::createMethodCall(iface()->service(), iface()->path(), iface()->interface(), QStringLiteral("setInactive"));
    message.setArguments({actionId});
    message.setAutoStartService(false);
    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
}

void KGlobalAccelPrivate::updateGlobalShortcut(/*const would be better*/ QAction *action,
                                               ShortcutTypes actionFlags,
                                               KGlobalAccel::GlobalShortcutLoading globalFlags)
{
    // No action or no objectname -> Do nothing
    if (!action || action->objectName().isEmpty()) {
        return;
    }

    QStringList actionId = makeActionId(action);

    uint setterFlags = 0;
    if (globalFlags & NoAutoloading) {
        setterFlags |= NoAutoloading;
    }

    if (actionFlags & ActiveShortcut) {
        const QList<QKeySequence> activeShortcut = actionShortcuts.value(action);
        bool isConfigurationAction = action->property("isConfigurationAction").toBool();
        uint activeSetterFlags = setterFlags;

        // setPresent tells kglobalaccel that the shortcut is active
        if (!isConfigurationAction) {
            activeSetterFlags |= SetPresent;
        }

        // Sets the shortcut, returns the active/real keys
        const auto result = iface()->setShortcut(actionId, intListFromShortcut(activeShortcut), activeSetterFlags);

        // Make sure we get informed about changes in the component by kglobalaccel
        getComponent(componentUniqueForAction(action), true);

        // Create a shortcut from the result
        const QList<QKeySequence> scResult(shortcutFromIntList(result));

        if (isConfigurationAction && (globalFlags & NoAutoloading)) {
            // If this is a configuration action and we have set the shortcut,
            // inform the real owner of the change.
            // Note that setForeignShortcut will cause a signal to be sent to applications
            // even if it did not "see" that the shortcut has changed. This is Good because
            // at the time of comparison (now) the action *already has* the new shortcut.
            // We called setShortcut(), remember?
            // Also note that we will see our own signal so we may not need to call
            // setActiveGlobalShortcutNoEnable - _k_shortcutGotChanged() does it.
            // In practice it's probably better to get the change propagated here without
            // DBus delay as we do below.
            iface()->setForeignShortcut(actionId, result);
        }
        if (scResult != activeShortcut) {
            // If kglobalaccel returned a shortcut that differs from the one we
            // sent, use that one. There must have been clashes or some other problem.
            actionShortcuts.insert(action, scResult);
            Q_EMIT q->globalShortcutChanged(action, scResult.isEmpty() ? QKeySequence() : scResult.first());
        }
    }

    if (actionFlags & DefaultShortcut) {
        const QList<QKeySequence> defaultShortcut = actionDefaultShortcuts.value(action);
        iface()->setShortcut(actionId, intListFromShortcut(defaultShortcut), setterFlags | IsDefault);
    }
}

QStringList KGlobalAccelPrivate::makeActionId(const QAction *action)
{
    QStringList ret(componentUniqueForAction(action)); // Component Unique Id ( see actionIdFields )
    Q_ASSERT(!ret.at(KGlobalAccel::ComponentUnique).isEmpty());
    Q_ASSERT(!action->objectName().isEmpty());
    ret.append(action->objectName()); // Action Unique Name
    ret.append(componentFriendlyForAction(action)); // Component Friendly name
    const QString actionText = action->text().replace(QLatin1Char('&'), QStringLiteral(""));
    ret.append(actionText); // Action Friendly Name
    return ret;
}

QList<int> KGlobalAccelPrivate::intListFromShortcut(const QList<QKeySequence> &cut)
{
    QList<int> ret;
    for (const QKeySequence &sequence : cut) {
        ret.append(sequence[0]);
    }
    while (!ret.isEmpty() && ret.last() == 0) {
        ret.removeLast();
    }
    return ret;
}

QList<QKeySequence> KGlobalAccelPrivate::shortcutFromIntList(const QList<int> &list)
{
    QList<QKeySequence> ret;
    for (int i : list) {
        ret.append(i);
    }
    return ret;
}

QString KGlobalAccelPrivate::componentUniqueForAction(const QAction *action)
{
    if (!action->property("componentName").isValid()) {
        return QCoreApplication::applicationName();
    } else {
        return action->property("componentName").toString();
    }
}

QString KGlobalAccelPrivate::componentFriendlyForAction(const QAction *action)
{
    QString property = action->property("componentDisplayName").toString();
    if (!property.isEmpty()) {
        return property;
    }
    if (!QGuiApplication::applicationDisplayName().isEmpty()) {
        return QGuiApplication::applicationDisplayName();
    }
    return QCoreApplication::applicationName();
}

#if HAVE_X11
int _k_timestampCompare(unsigned long time1_, unsigned long time2_) // like strcmp()
{
    quint32 time1 = time1_;
    quint32 time2 = time2_;
    if (time1 == time2) {
        return 0;
    }
    return quint32(time1 - time2) < 0x7fffffffU ? 1 : -1; // time1 > time2 -> 1, handle wrapping
}
#endif

void KGlobalAccelPrivate::_k_invokeAction(const QString &componentUnique, const QString &actionUnique, qlonglong timestamp)
{
    QAction *action = nullptr;
    const QList<QAction *> candidates = nameToAction.values(actionUnique);
    for (QAction *const a : candidates) {
        if (componentUniqueForAction(a) == componentUnique) {
            action = a;
        }
    }

    // We do not trigger if
    // - there is no action
    // - the action is not enabled
    // - the action is an configuration action
    if (!action || !action->isEnabled() || action->property("isConfigurationAction").toBool()) {
        return;
    }

#if HAVE_X11
    // Update this application's X timestamp if needed.
    // TODO The 100%-correct solution should probably be handling this action
    // in the proper place in relation to the X events queue in order to avoid
    // the possibility of wrong ordering of user events.
    if (QX11Info::isPlatformX11()) {
        if (_k_timestampCompare(timestamp, QX11Info::appTime()) > 0) {
            QX11Info::setAppTime(timestamp);
        }
        if (_k_timestampCompare(timestamp, QX11Info::appUserTime()) > 0) {
            QX11Info::setAppUserTime(timestamp);
        }
    }
#endif
    action->setProperty("org.kde.kglobalaccel.activationTimestamp", timestamp);

    action->trigger();
}

void KGlobalAccelPrivate::_k_shortcutGotChanged(const QStringList &actionId, const QList<int> &keys)
{
    QAction *action = nameToAction.value(actionId.at(KGlobalAccel::ActionUnique));
    if (!action) {
        return;
    }

    const QList<QKeySequence> shortcuts = shortcutFromIntList(keys);
    actionShortcuts.insert(action, shortcuts);
    Q_EMIT q->globalShortcutChanged(action, keys.isEmpty() ? QKeySequence() : shortcuts.first());
}

void KGlobalAccelPrivate::_k_serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner);
    if (name == QLatin1String("org.kde.kglobalaccel") && !newOwner.isEmpty()) {
        // kglobalaccel was restarted
        qCDebug(KGLOBALACCEL_LOG) << "detected kglobalaccel restarting, re-registering all shortcut keys";
        reRegisterAll();
    }
}

void KGlobalAccelPrivate::reRegisterAll()
{
    // We clear all our data, assume that all data on the other side is clear too,
    // and register each action as if it just was allowed to have global shortcuts.
    // If the kded side still has the data it doesn't matter because of the
    // autoloading mechanism. The worst case I can imagine is that an action's
    // shortcut was changed but the kded side died before it got the message so
    // autoloading will now assign an old shortcut to the action. Particularly
    // picky apps might assert or misbehave.
    const QSet<QAction *> allActions = actions;
    nameToAction.clear();
    actions.clear();
    for (QAction *const action : allActions) {
        if (doRegister(action)) {
            updateGlobalShortcut(action, ActiveShortcut, KGlobalAccel::Autoloading);
        }
    }
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(4, 2)
QList<QStringList> KGlobalAccel::allMainComponents()
{
    return d->iface()->allMainComponents();
}
#endif

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(4, 2)
QList<QStringList> KGlobalAccel::allActionsForComponent(const QStringList &actionId)
{
    return d->iface()->allActionsForComponent(actionId);
}
#endif

// static
#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(4, 2)
QStringList KGlobalAccel::findActionNameSystemwide(const QKeySequence &seq)
{
    return self()->d->iface()->action(seq[0]);
}
#endif

QList<KGlobalShortcutInfo> KGlobalAccel::getGlobalShortcutsByKey(const QKeySequence &seq)
{
    return self()->d->iface()->getGlobalShortcutsByKey(seq[0]);
}

bool KGlobalAccel::isGlobalShortcutAvailable(const QKeySequence &seq, const QString &comp)
{
    return self()->d->iface()->isGlobalShortcutAvailable(seq[0], comp);
}

// static
#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(4, 2)
bool KGlobalAccel::promptStealShortcutSystemwide(QWidget *parent, const QStringList &actionIdentifier, const QKeySequence &seq)
{
    if (actionIdentifier.size() < 4) {
        return false;
    }
    QString title = tr("Conflict with Global Shortcut");
    QString message = tr("The '%1' key combination has already been allocated "
                         "to the global action \"%2\" in %3.\n"
                         "Do you want to reassign it from that action to the current one?")
                          .arg(seq.toString(), actionIdentifier.at(KGlobalAccel::ActionFriendly), actionIdentifier.at(KGlobalAccel::ComponentFriendly));

    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(message);
    box.addButton(QMessageBox::Ok)->setText(tr("Reassign"));
    box.addButton(QMessageBox::Cancel);

    return box.exec() == QMessageBox::Ok;
}
#endif

// static
bool KGlobalAccel::promptStealShortcutSystemwide(QWidget *parent, const QList<KGlobalShortcutInfo> &shortcuts, const QKeySequence &seq)
{
    if (shortcuts.isEmpty()) {
        // Usage error. Just say no
        return false;
    }

    QString component = shortcuts[0].componentFriendlyName();

    QString message;
    if (shortcuts.size() == 1) {
        message = tr("The '%1' key combination is registered by application %2 for action %3:").arg(seq.toString(), component, shortcuts[0].friendlyName());
    } else {
        QString actionList;
        for (const KGlobalShortcutInfo &info : shortcuts) {
            actionList += tr("In context '%1' for action '%2'\n").arg(info.contextFriendlyName(), info.friendlyName());
        }
        message = tr("The '%1' key combination is registered by application %2.\n%3").arg(seq.toString(), component, actionList);
    }

    QString title = tr("Conflict With Registered Global Shortcut");

    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(message);
    box.addButton(QMessageBox::Ok)->setText(tr("Reassign"));
    box.addButton(QMessageBox::Cancel);

    return box.exec() == QMessageBox::Ok;
}

// static
void KGlobalAccel::stealShortcutSystemwide(const QKeySequence &seq)
{
    // get the shortcut, remove seq, and set the new shortcut
    const QStringList actionId = self()->d->iface()->action(seq[0]);
    if (actionId.size() < 4) { // not a global shortcut
        return;
    }
    QList<int> sc = self()->d->iface()->shortcut(actionId);

    for (int i = 0; i < sc.count(); i++)
        if (sc[i] == seq[0]) {
            sc[i] = 0;
        }

    self()->d->iface()->setForeignShortcut(actionId, sc);
}

bool checkGarbageKeycode(const QList<QKeySequence> &shortcut)
{
    // protect against garbage keycode -1 that Qt sometimes produces for exotic keys;
    // at the moment (~mid 2008) Multimedia PlayPause is one of those keys.
    for (const QKeySequence &sequence : shortcut) {
        for (int i = 0; i < 4; i++) {
            if (sequence[i] == -1) {
                qWarning() << "Encountered garbage keycode (keycode = -1) in input, not doing anything.";
                return true;
            }
        }
    }
    return false;
}

bool KGlobalAccel::setDefaultShortcut(QAction *action, const QList<QKeySequence> &shortcut, GlobalShortcutLoading loadFlag)
{
    if (checkGarbageKeycode(shortcut)) {
        return false;
    }

    if (!d->doRegister(action)) {
        return false;
    }

    d->actionDefaultShortcuts.insert(action, shortcut);
    d->updateGlobalShortcut(action, KGlobalAccelPrivate::DefaultShortcut, loadFlag);
    return true;
}

bool KGlobalAccel::setShortcut(QAction *action, const QList<QKeySequence> &shortcut, GlobalShortcutLoading loadFlag)
{
    if (checkGarbageKeycode(shortcut)) {
        return false;
    }

    if (!d->doRegister(action)) {
        return false;
    }

    d->actionShortcuts.insert(action, shortcut);
    d->updateGlobalShortcut(action, KGlobalAccelPrivate::ActiveShortcut, loadFlag);
    return true;
}

QList<QKeySequence> KGlobalAccel::defaultShortcut(const QAction *action) const
{
    return d->actionDefaultShortcuts.value(action);
}

QList<QKeySequence> KGlobalAccel::shortcut(const QAction *action) const
{
    return d->actionShortcuts.value(action);
}

QList<QKeySequence> KGlobalAccel::globalShortcut(const QString &componentName, const QString &actionId) const
{
    // see also d->updateGlobalShortcut(action, KGlobalAccelPrivate::ActiveShortcut, KGlobalAccel::Autoloading);

    // how componentName and actionId map to QAction, e.g:
    // action->setProperty("componentName", "kwin");
    // action->setObjectName("Kill Window");

    const QList<int> result = self()->d->iface()->shortcut({componentName, actionId, QString(), QString()});
    const QList<QKeySequence> scResult(d->shortcutFromIntList(result));
    return scResult;
}

void KGlobalAccel::removeAllShortcuts(QAction *action)
{
    d->remove(action, KGlobalAccelPrivate::UnRegister);
}

bool KGlobalAccel::hasShortcut(const QAction *action) const
{
    return d->actionShortcuts.contains(action) || d->actionDefaultShortcuts.contains(action);
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(5, 9)
bool KGlobalAccel::eventFilter(QObject *watched, QEvent *event)
{
    return QObject::eventFilter(watched, event);
}
#endif

bool KGlobalAccel::setGlobalShortcut(QAction *action, const QList<QKeySequence> &shortcut)
{
    KGlobalAccel *g = KGlobalAccel::self();
    return g->d->setShortcutWithDefault(action, shortcut, Autoloading);
}

bool KGlobalAccel::setGlobalShortcut(QAction *action, const QKeySequence &shortcut)
{
    return KGlobalAccel::setGlobalShortcut(action, QList<QKeySequence>() << shortcut);
}

bool KGlobalAccelPrivate::setShortcutWithDefault(QAction *action, const QList<QKeySequence> &shortcut, KGlobalAccel::GlobalShortcutLoading loadFlag)
{
    if (checkGarbageKeycode(shortcut)) {
        return false;
    }

    if (!doRegister(action)) {
        return false;
    }

    actionDefaultShortcuts.insert(action, shortcut);
    actionShortcuts.insert(action, shortcut);
    updateGlobalShortcut(action, KGlobalAccelPrivate::DefaultShortcut | KGlobalAccelPrivate::ActiveShortcut, loadFlag);
    return true;
}

#include "moc_kglobalaccel.cpp"
