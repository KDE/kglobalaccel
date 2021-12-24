/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2007 Andreas Hartmetz <ahartmetz@gmail.com>
    SPDX-FileCopyrightText: 2007 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalacceld.h"

#include "component.h"
#include "globalshortcut.h"
#include "globalshortcutcontext.h"
#include "globalshortcutsregistry.h"
#include "kserviceactioncomponent.h"
#include "logging_p.h"

#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QMetaMethod>
#include <QTimer>

#include "kglobalaccel.h"

struct KGlobalAccelDPrivate {
    KGlobalAccelDPrivate(KGlobalAccelD *qq)
        : q(qq)
    {
    }

    GlobalShortcut *findAction(const QStringList &actionId) const;

    /**
     * Find the action @a shortcutUnique in @a componentUnique.
     *
     * @return the action or @c nullptr if doesn't exist
     */
    GlobalShortcut *findAction(const QString &componentUnique, const QString &shortcutUnique) const;

    GlobalShortcut *addAction(const QStringList &actionId);
    KdeDGlobalAccel::Component *component(const QStringList &actionId) const;

    void splitComponent(QString &component, QString &context) const
    {
        context = QStringLiteral("default");
        if (component.indexOf('|') != -1) {
            QStringList tmp = component.split('|');
            Q_ASSERT(tmp.size() == 2);
            component = tmp.at(0);
            context = tmp.at(1);
        }
    }

    //! Timer for delayed writing to kglobalshortcutsrc
    QTimer writeoutTimer;

    //! Our holder
    KGlobalAccelD *q;
};

GlobalShortcut *KGlobalAccelDPrivate::findAction(const QStringList &actionId) const
{
    // Check if actionId is valid
    if (actionId.size() != 4) {
        qCDebug(KGLOBALACCELD) << "Invalid! '" << actionId << "'";
        return nullptr;
    }

    return findAction(actionId.at(KGlobalAccel::ComponentUnique), actionId.at(KGlobalAccel::ActionUnique));
}

GlobalShortcut *KGlobalAccelDPrivate::findAction(const QString &_componentUnique, const QString &shortcutUnique) const
{
    QString componentUnique = _componentUnique;

    KdeDGlobalAccel::Component *component;
    QString contextUnique;
    if (componentUnique.indexOf('|') == -1) {
        component = GlobalShortcutsRegistry::self()->getComponent(componentUnique);
        if (component) {
            contextUnique = component->currentContext()->uniqueName();
        }
    } else {
        splitComponent(componentUnique, contextUnique);
        component = GlobalShortcutsRegistry::self()->getComponent(componentUnique);
    }

    if (!component) {
#ifdef KDEDGLOBALACCEL_TRACE
        qCDebug(KGLOBALACCELD) << componentUnique << "not found";
#endif
        return nullptr;
    }

    GlobalShortcut *shortcut = component->getShortcutByName(shortcutUnique, contextUnique);

#ifdef KDEDGLOBALACCEL_TRACE
    if (shortcut) {
        qCDebug(KGLOBALACCELD) << componentUnique << contextUnique << shortcut->uniqueName();
    } else {
        qCDebug(KGLOBALACCELD) << "No match for" << shortcutUnique;
    }
#endif
    return shortcut;
}

KdeDGlobalAccel::Component *KGlobalAccelDPrivate::component(const QStringList &actionId) const
{
    // Get the component for the action. If we have none create a new one
    KdeDGlobalAccel::Component *component = GlobalShortcutsRegistry::self()->getComponent(actionId.at(KGlobalAccel::ComponentUnique));
    if (!component) {
        if (actionId.at(KGlobalAccel::ComponentUnique).endsWith(QLatin1String(".desktop"))) {
            component = new KdeDGlobalAccel::KServiceActionComponent(actionId.at(KGlobalAccel::ComponentUnique),
                                                                     actionId.at(KGlobalAccel::ComponentFriendly),
                                                                     GlobalShortcutsRegistry::self());
            component->activateGlobalShortcutContext(QStringLiteral("default"));
            static_cast<KdeDGlobalAccel::KServiceActionComponent *>(component)->loadFromService();
        } else {
            component = new KdeDGlobalAccel::Component(actionId.at(KGlobalAccel::ComponentUnique),
                                                       actionId.at(KGlobalAccel::ComponentFriendly),
                                                       GlobalShortcutsRegistry::self());
        }
        Q_ASSERT(component);
    }
    return component;
}

GlobalShortcut *KGlobalAccelDPrivate::addAction(const QStringList &actionId)
{
    Q_ASSERT(actionId.size() >= 4);

    QString componentUnique = actionId.at(KGlobalAccel::ComponentUnique);

    QString contextUnique = QStringLiteral("default");

    if (componentUnique.indexOf('|') != -1) {
        QStringList tmp = componentUnique.split('|');
        Q_ASSERT(tmp.size() == 2);
        componentUnique = tmp.at(0);
        contextUnique = tmp.at(1);
    }

    QStringList actionIdTmp = actionId;
    actionIdTmp.replace(KGlobalAccel::ComponentUnique, componentUnique);

    // Create the component if necessary
    KdeDGlobalAccel::Component *component = this->component(actionIdTmp);
    Q_ASSERT(component);

    // Create the context if necessary
    if (component->getShortcutContexts().count(contextUnique) == 0) {
        component->createGlobalShortcutContext(contextUnique);
    }

    Q_ASSERT(!component->getShortcutByName(componentUnique, contextUnique));

    return new GlobalShortcut(actionId.at(KGlobalAccel::ActionUnique), actionId.at(KGlobalAccel::ActionFriendly), component->shortcutContext(contextUnique));
}

Q_DECLARE_METATYPE(QStringList)

KGlobalAccelD::KGlobalAccelD(QObject *parent)
    : QObject(parent)
    , d(new KGlobalAccelDPrivate(this))
{
}

bool KGlobalAccelD::init()
{
    qDBusRegisterMetaType<QKeySequence>();
    qDBusRegisterMetaType<QList<QKeySequence>>();
    qDBusRegisterMetaType<QList<QDBusObjectPath>>();
    qDBusRegisterMetaType<QList<QStringList>>();
    qDBusRegisterMetaType<QStringList>();
    qDBusRegisterMetaType<KGlobalShortcutInfo>();
    qDBusRegisterMetaType<QList<KGlobalShortcutInfo>>();

    GlobalShortcutsRegistry *reg = GlobalShortcutsRegistry::self();
    Q_ASSERT(reg);

    d->writeoutTimer.setSingleShot(true);
    connect(&d->writeoutTimer, &QTimer::timeout, reg, &GlobalShortcutsRegistry::writeSettings);

    if (!QDBusConnection::sessionBus().registerService(QLatin1String("org.kde.kglobalaccel"))) {
        qCWarning(KGLOBALACCELD) << "Failed to register service org.kde.kglobalaccel";
        return false;
    }

    if (!QDBusConnection::sessionBus().registerObject(QStringLiteral("/kglobalaccel"), this, QDBusConnection::ExportScriptableContents)) {
        qCWarning(KGLOBALACCELD) << "Failed to register object kglobalaccel in org.kde.kglobalaccel";
        return false;
    }

    GlobalShortcutsRegistry::self()->setDBusPath(QDBusObjectPath("/"));
    GlobalShortcutsRegistry::self()->loadSettings();

    return true;
}

KGlobalAccelD::~KGlobalAccelD()
{
    GlobalShortcutsRegistry *const reg = GlobalShortcutsRegistry::self();
    if (d->writeoutTimer.isActive()) {
        d->writeoutTimer.stop();
        reg->writeSettings();
    }
    reg->deactivateShortcuts();
    delete d;
}

QList<QStringList> KGlobalAccelD::allMainComponents() const
{
    QList<QStringList> ret;
    QStringList emptyList;
    emptyList.reserve(4);
    for (int i = 0; i < 4; i++) {
        emptyList.append(QString());
    }

    const auto components = GlobalShortcutsRegistry::self()->allMainComponents();
    ret.reserve(components.size() * 4);
    for (const KdeDGlobalAccel::Component *component : components) {
        QStringList actionId(emptyList);
        actionId[KGlobalAccel::ComponentUnique] = component->uniqueName();
        actionId[KGlobalAccel::ComponentFriendly] = component->friendlyName();
        ret.append(actionId);
    }

    return ret;
}

QList<QStringList> KGlobalAccelD::allActionsForComponent(const QStringList &actionId) const
{
    //### Would it be advantageous to sort the actions by unique name?
    QList<QStringList> ret;

    KdeDGlobalAccel::Component *const component = GlobalShortcutsRegistry::self()->getComponent(actionId[KGlobalAccel::ComponentUnique]);
    if (!component) {
        return ret;
    }

    QStringList partialId(actionId[KGlobalAccel::ComponentUnique]); // ComponentUnique
    partialId.append(QString()); // ActionUnique
    // Use our internal friendlyName, not the one passed in. We should have the latest data.
    partialId.append(component->friendlyName()); // ComponentFriendly
    partialId.append(QString()); // ActionFriendly

    const auto listShortcuts = component->allShortcuts();
    for (const GlobalShortcut *const shortcut : listShortcuts) {
        if (shortcut->isFresh()) {
            // isFresh is only an intermediate state, not to be reported outside.
            continue;
        }
        QStringList actionId(partialId);
        actionId[KGlobalAccel::ActionUnique] = shortcut->uniqueName();
        actionId[KGlobalAccel::ActionFriendly] = shortcut->friendlyName();
        ret.append(actionId);
    }
    return ret;
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(5, 90)
QStringList KGlobalAccelD::action(int key) const
{
    return actionList(key);
}
#endif

QStringList KGlobalAccelD::actionList(const QKeySequence &key) const
{
    GlobalShortcut *shortcut = GlobalShortcutsRegistry::self()->getShortcutByKey(key);
    QStringList ret;
    if (shortcut) {
        ret.append(shortcut->context()->component()->uniqueName());
        ret.append(shortcut->uniqueName());
        ret.append(shortcut->context()->component()->friendlyName());
        ret.append(shortcut->friendlyName());
    }
    return ret;
}

void KGlobalAccelD::activateGlobalShortcutContext(const QString &component, const QString &uniqueName)
{
    KdeDGlobalAccel::Component *const comp = GlobalShortcutsRegistry::self()->getComponent(component);
    if (comp) {
        comp->activateGlobalShortcutContext(uniqueName);
    }
}

QList<QDBusObjectPath> KGlobalAccelD::allComponents() const
{
    QList<QDBusObjectPath> allComp;

    const auto lstMainComponents = GlobalShortcutsRegistry::self()->allMainComponents();
    for (const KdeDGlobalAccel::Component *component : lstMainComponents) {
        allComp.append(component->dbusPath());
    }

    return allComp;
}

void KGlobalAccelD::blockGlobalShortcuts(bool block)
{
#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << block;
#endif
    block ? GlobalShortcutsRegistry::self()->deactivateShortcuts(true) : GlobalShortcutsRegistry::self()->activateShortcuts();
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(5, 90)
QList<int> KGlobalAccelD::shortcut(const QStringList &action) const
{
    GlobalShortcut *shortcut = d->findAction(action);
    if (shortcut) {
        QList<int> ret;
        for (auto i : shortcut->keys()) {
            ret << i[0];
        }
        return ret;
    }
    return QList<int>();
}
#endif

QList<QKeySequence> KGlobalAccelD::shortcutKeys(const QStringList &action) const
{
    GlobalShortcut *shortcut = d->findAction(action);
    if (shortcut) {
        return shortcut->keys();
    }
    return QList<QKeySequence>();
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(5, 90)
QList<int> KGlobalAccelD::defaultShortcut(const QStringList &action) const
{
    GlobalShortcut *shortcut = d->findAction(action);
    if (shortcut) {
        QList<int> ret;
        for (auto i : shortcut->keys()) {
            ret << i[0];
        }
        return ret;
    }
    return QList<int>();
}
#endif

QList<QKeySequence> KGlobalAccelD::defaultShortcutKeys(const QStringList &action) const
{
    GlobalShortcut *shortcut = d->findAction(action);
    if (shortcut) {
        return shortcut->defaultKeys();
    }
    return QList<QKeySequence>();
}

// This method just registers the action. Nothing else. Shortcut has to be set
// later.
void KGlobalAccelD::doRegister(const QStringList &actionId)
{
#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << actionId;
#endif

    // Check because we would not want to add a action for an invalid
    // actionId. findAction returns nullptr in that case.
    if (actionId.size() < 4) {
        return;
    }

    GlobalShortcut *shortcut = d->findAction(actionId);
    if (!shortcut) {
        shortcut = d->addAction(actionId);
    } else {
        // a switch of locales is one common reason for a changing friendlyName
        if ((!actionId[KGlobalAccel::ActionFriendly].isEmpty()) && shortcut->friendlyName() != actionId[KGlobalAccel::ActionFriendly]) {
            shortcut->setFriendlyName(actionId[KGlobalAccel::ActionFriendly]);
            scheduleWriteSettings();
        }
        if ((!actionId[KGlobalAccel::ComponentFriendly].isEmpty())
            && shortcut->context()->component()->friendlyName() != actionId[KGlobalAccel::ComponentFriendly]) {
            shortcut->context()->component()->setFriendlyName(actionId[KGlobalAccel::ComponentFriendly]);
            scheduleWriteSettings();
        }
    }
}

QDBusObjectPath KGlobalAccelD::getComponent(const QString &componentUnique) const
{
#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << componentUnique;
#endif

    KdeDGlobalAccel::Component *component = GlobalShortcutsRegistry::self()->getComponent(componentUnique);

    if (component) {
        return component->dbusPath();
    } else {
        sendErrorReply("org.kde.kglobalaccel.NoSuchComponent", QStringLiteral("The component '%1' doesn't exist.").arg(componentUnique));
        return QDBusObjectPath("/");
    }
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(5, 90)
QList<KGlobalShortcutInfo> KGlobalAccelD::getGlobalShortcutsByKey(int key) const
{
    return globalShortcutsByKey(key, KGlobalAccel::MatchType::Equal);
}
#endif

QList<KGlobalShortcutInfo> KGlobalAccelD::globalShortcutsByKey(const QKeySequence &key, KGlobalAccel::MatchType type) const
{
#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << key;
#endif
    const QList<GlobalShortcut *> shortcuts = GlobalShortcutsRegistry::self()->getShortcutsByKey(key, type);

    QList<KGlobalShortcutInfo> rc;
    for (const GlobalShortcut *sc : shortcuts) {
#ifdef KDEDGLOBALACCEL_TRACE
        qCDebug(KGLOBALACCELD) << sc->context()->uniqueName() << sc->uniqueName();
#endif
        rc.append(static_cast<KGlobalShortcutInfo>(*sc));
    }

    return rc;
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(5, 90)
bool KGlobalAccelD::isGlobalShortcutAvailable(int shortcut, const QString &component) const
{
    return globalShortcutAvailable(shortcut, component);
}
#endif

bool KGlobalAccelD::globalShortcutAvailable(const QKeySequence &shortcut, const QString &component) const
{
    QString realComponent = component;
    QString context;
    d->splitComponent(realComponent, context);
    return GlobalShortcutsRegistry::self()->isShortcutAvailable(shortcut, realComponent, context);
}

void KGlobalAccelD::setInactive(const QStringList &actionId)
{
#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << actionId;
#endif

    GlobalShortcut *shortcut = d->findAction(actionId);
    if (shortcut) {
        shortcut->setIsPresent(false);
    }
}

bool KGlobalAccelD::unregister(const QString &componentUnique, const QString &shortcutUnique)
{
#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << componentUnique << shortcutUnique;
#endif

    // Stop grabbing the key
    GlobalShortcut *shortcut = d->findAction(componentUnique, shortcutUnique);
    if (shortcut) {
        shortcut->unRegister();
        scheduleWriteSettings();
    }

    return shortcut;
}

#if KGLOBALACCELPRIVATE_BUILD_DEPRECATED_SINCE(4, 3)
void KGlobalAccelD::unRegister(const QStringList &actionId)
{
#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << actionId;
#endif

    // Stop grabbing the key
    GlobalShortcut *shortcut = d->findAction(actionId);
    if (shortcut) {
        shortcut->unRegister();
        scheduleWriteSettings();
    }
}
#endif

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(5, 90)
QList<int> KGlobalAccelD::setShortcut(const QStringList &actionId, const QList<int> &keys, uint flags)
{
    QList<int> ret;
    QList<QKeySequence> input;
    for (auto i : keys) {
        input << i;
    }
    const QList<QKeySequence> list = setShortcutKeys(actionId, input, flags);
    for (auto i : list) {
        ret << i[0];
    }
    return ret;
}
#endif

QList<QKeySequence> KGlobalAccelD::setShortcutKeys(const QStringList &actionId, const QList<QKeySequence> &keys, uint flags)
{
    // spare the DBus framework some work
    const bool setPresent = (flags & SetPresent);
    const bool isAutoloading = !(flags & NoAutoloading);
    const bool isDefault = (flags & IsDefault);

    GlobalShortcut *shortcut = d->findAction(actionId);
    if (!shortcut) {
        return QList<QKeySequence>();
    }

    // default shortcuts cannot clash because they don't do anything
    if (isDefault) {
        if (shortcut->defaultKeys() != keys) {
            shortcut->setDefaultKeys(keys);
            scheduleWriteSettings();
        }
        return keys; // doesn't matter
    }

    if (isAutoloading && !shortcut->isFresh()) {
        // the trivial and common case - synchronize the action from our data
        // and exit.
        if (!shortcut->isPresent() && setPresent) {
            shortcut->setIsPresent(true);
        }
        // We are finished here. Return the list of current active keys.
        return shortcut->keys();
    }

    // now we are actually changing the shortcut of the action
    shortcut->setKeys(keys);

    if (setPresent) {
        shortcut->setIsPresent(true);
    }

    // maybe isFresh should really only be set if setPresent, but only two things should use !setPresent:
    //- the global shortcuts KCM: very unlikely to catch KWin/etc.'s actions in isFresh state
    //- KGlobalAccel::stealGlobalShortcutSystemwide(): only applies to actions with shortcuts
    //  which can never be fresh if created the usual way
    shortcut->setIsFresh(false);

    scheduleWriteSettings();

    return shortcut->keys();
}

#if KGLOBALACCEL_BUILD_DEPRECATED_SINCE(5, 90)
void KGlobalAccelD::setForeignShortcut(const QStringList &actionId, const QList<int> &keys)
{
    QList<QKeySequence> input;
    for (auto i : keys) {
        input << i;
    }
    return setForeignShortcutKeys(actionId, input);
}
#endif

void KGlobalAccelD::setForeignShortcutKeys(const QStringList &actionId, const QList<QKeySequence> &keys)
{
#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << actionId;
#endif

    GlobalShortcut *shortcut = d->findAction(actionId);
    if (!shortcut) {
        return;
    }

    QList<QKeySequence> newKeys = setShortcutKeys(actionId, keys, NoAutoloading);

    Q_EMIT yourShortcutsChanged(actionId, newKeys);
}

void KGlobalAccelD::scheduleWriteSettings() const
{
    if (!d->writeoutTimer.isActive()) {
        d->writeoutTimer.start(500);
    }
}

#include "moc_kglobalacceld.cpp"
