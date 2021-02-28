/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "globalshortcutsregistry.h"
#include "component.h"
#include "globalshortcut.h"
#include "globalshortcutcontext.h"
#include "kglobalaccel_interface.h"
#include "kserviceactioncomponent.h"
#include "logging_p.h"
#include <config-kglobalaccel.h>

#include <KDesktopFile>
#include <KPluginLoader>
#include <KPluginMetaData>
#include <QDir>
#include <QGuiApplication>
#include <QJsonArray>
#include <QStandardPaths>

#include <QDBusConnection>
#include <QKeySequence>

static bool checkPlatform(const QJsonObject &metadata, const QString &platformName)
{
    const QJsonArray platforms = metadata.value(QStringLiteral("MetaData")).toObject().value(QStringLiteral("platforms")).toArray();
    return std::any_of(platforms.begin(), platforms.end(), [&platformName](const QJsonValue &value) {
        return QString::compare(platformName, value.toString(), Qt::CaseInsensitive) == 0;
    });
}

static KGlobalAccelInterface *loadPlugin(GlobalShortcutsRegistry *parent)
{
    QString platformName = QString::fromLocal8Bit(qgetenv("KGLOBALACCELD_PLATFORM"));
    if (platformName.isEmpty()) {
        platformName = QGuiApplication::platformName();
    }

    const QVector<QStaticPlugin> staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &staticPlugin : staticPlugins) {
        const QJsonObject metadata = staticPlugin.metaData();
        if (metadata.value(QLatin1String("IID")) != QLatin1String(KGlobalAccelInterface_iid)) {
            continue;
        }
        if (checkPlatform(metadata, platformName)) {
            KGlobalAccelInterface *interface = qobject_cast<KGlobalAccelInterface *>(staticPlugin.instance());
            if (interface) {
                qCDebug(KGLOBALACCELD) << "Loaded a static plugin for platform" << platformName;
                interface->setRegistry(parent);
                return interface;
            }
        }
    }

    const QVector<KPluginMetaData> candidates = KPluginLoader::findPlugins(QStringLiteral("org.kde.kglobalaccel5.platforms"));
    for (const KPluginMetaData &candidate : candidates) {
        QPluginLoader loader(candidate.fileName());
        if (checkPlatform(loader.metaData(), platformName)) {
            KGlobalAccelInterface *interface = qobject_cast<KGlobalAccelInterface *>(loader.instance());
            if (interface) {
                qCDebug(KGLOBALACCELD) << "Loaded plugin" << candidate.fileName() << "for platform" << platformName;
                interface->setRegistry(parent);
                return interface;
            }
        }
    }

    qCWarning(KGLOBALACCELD) << "Could not find any platform plugin";
    return nullptr;
}

GlobalShortcutsRegistry::GlobalShortcutsRegistry()
    : QObject()
    , _active_keys()
    , _components()
    , _manager(loadPlugin(this))
    , _config(qEnvironmentVariableIsSet("KGLOBALACCEL_TEST_MODE") ? QString() : QStringLiteral("kglobalshortcutsrc"), KConfig::SimpleConfig)
{
    if (_manager) {
        _manager->setEnabled(true);
    }
}

GlobalShortcutsRegistry::~GlobalShortcutsRegistry()
{
    if (_manager) {
        _manager->setEnabled(false);

        // Ungrab all keys. We don't go over GlobalShortcuts because
        // GlobalShortcutsRegistry::self() doesn't work anymore.
        const auto listKeys = _active_keys.keys();
        for (const int key : listKeys) {
            _manager->grabKey(key, false);
        }
    }
    _active_keys.clear();
}

KdeDGlobalAccel::Component *GlobalShortcutsRegistry::addComponent(KdeDGlobalAccel::Component *component)
{
    if (_components.value(component->uniqueName())) {
        Q_ASSERT_X(false, "GlobalShortcutsRegistry::addComponent", "component already registered?!?!");
        return _components.value(component->uniqueName());
    }

    _components.insert(component->uniqueName(), component);
    QDBusConnection conn(QDBusConnection::sessionBus());

    conn.registerObject(component->dbusPath().path(), component, QDBusConnection::ExportScriptableContents);
    return component;
}

void GlobalShortcutsRegistry::activateShortcuts()
{
    for (KdeDGlobalAccel::Component *component : qAsConst(_components)) {
        component->activateShortcuts();
    }
}

QList<KdeDGlobalAccel::Component *> GlobalShortcutsRegistry::allMainComponents() const
{
    return _components.values();
}

void GlobalShortcutsRegistry::clear()
{
    for (KdeDGlobalAccel::Component *component : qAsConst(_components)) {
        delete component;
    }
    _components.clear();

    // The shortcuts should have deregistered themselves
    Q_ASSERT(_active_keys.isEmpty());
}

QDBusObjectPath GlobalShortcutsRegistry::dbusPath() const
{
    return _dbusPath;
}

void GlobalShortcutsRegistry::deactivateShortcuts(bool temporarily)
{
    for (KdeDGlobalAccel::Component *component : qAsConst(_components)) {
        component->deactivateShortcuts(temporarily);
    }
}

GlobalShortcut *GlobalShortcutsRegistry::getActiveShortcutByKey(int key) const
{
    return _active_keys.value(key);
}

KdeDGlobalAccel::Component *GlobalShortcutsRegistry::getComponent(const QString &uniqueName)
{
    return _components.value(uniqueName);
}

GlobalShortcut *GlobalShortcutsRegistry::getShortcutByKey(int key) const
{
    for (KdeDGlobalAccel::Component *component : qAsConst(_components)) {
        GlobalShortcut *rc = component->getShortcutByKey(key);
        if (rc)
            return rc;
    }
    return nullptr;
}

QList<GlobalShortcut *> GlobalShortcutsRegistry::getShortcutsByKey(int key) const
{
    QList<GlobalShortcut *> rc;

    for (KdeDGlobalAccel::Component *component : qAsConst(_components)) {
        rc = component->getShortcutsByKey(key);
        if (!rc.isEmpty())
            return rc;
    }
    return rc;
}

bool GlobalShortcutsRegistry::isShortcutAvailable(int shortcut, const QString &componentName, const QString &contextName) const
{
    for (KdeDGlobalAccel::Component *component : qAsConst(_components)) {
        if (!component->isShortcutAvailable(shortcut, componentName, contextName))
            return false;
    }
    return true;
}

Q_GLOBAL_STATIC(GlobalShortcutsRegistry, _self)
GlobalShortcutsRegistry *GlobalShortcutsRegistry::self()
{
    return _self;
}

bool GlobalShortcutsRegistry::keyPressed(int keyQt)
{
    GlobalShortcut *shortcut = getShortcutByKey(keyQt);
    if (!shortcut) {
        // This can happen for example with the ALT-Print shortcut of kwin.
        // ALT+PRINT is SYSREQ on my keyboard. So we grab something we think
        // is ALT+PRINT but symXToKeyQt and modXToQt make ALT+SYSREQ of it
        // when pressed (correctly). We can't match that.
#ifdef KDEDGLOBALACCEL_TRACE
        qCDebug(KGLOBALACCELD) << "Got unknown key" << QKeySequence(keyQt).toString();
#endif

        // In production mode just do nothing.
        return false;
    } else if (!shortcut->isActive()) {
#ifdef KDEDGLOBALACCEL_TRACE
        qCDebug(KGLOBALACCELD) << "Got inactive key" << QKeySequence(keyQt).toString();
#endif

        // In production mode just do nothing.
        return false;
    }

#ifdef KDEDGLOBALACCEL_TRACE
    qCDebug(KGLOBALACCELD) << QKeySequence(keyQt).toString() << "=" << shortcut->uniqueName();
#endif

    QStringList data(shortcut->context()->component()->uniqueName());
    data.append(shortcut->uniqueName());
    data.append(shortcut->context()->component()->friendlyName());
    data.append(shortcut->friendlyName());

    // Make sure kglobalacceld has ungrabbed the keyboard after receiving the
    // keypress, otherwise actions in application that try to grab the
    // keyboard (e.g. in kwin) may fail to do so. There is still a small race
    // condition with this being out-of-process.
    if (_manager) {
        _manager->syncWindowingSystem();
    }

    // 1st Invoke the action
    shortcut->context()->component()->emitGlobalShortcutPressed(*shortcut);

    return true;
}

void GlobalShortcutsRegistry::loadSettings()
{
    const auto groupList = _config.groupList();
    for (const QString &groupName : groupList) {
        qCDebug(KGLOBALACCELD) << "Loading group " << groupName;

        Q_ASSERT(groupName.indexOf('\x1d') == -1);

        // loadSettings isn't designed to be called in between. Only at the
        // beginning.
        Q_ASSERT(!getComponent(groupName));

        KConfigGroup configGroup(&_config, groupName);

        // We previously stored the friendly name in a separate group. migrate
        // that
        const QString friendlyName = configGroup.readEntry("_k_friendly_name");

        // Create the component
        KdeDGlobalAccel::Component *component = nullptr;
        if (groupName.endsWith(QLatin1String(".desktop"))) {
            component = new KdeDGlobalAccel::KServiceActionComponent(groupName, friendlyName, this);
        } else {
            component = new KdeDGlobalAccel::Component(groupName, friendlyName, this);
        }

        // Now load the contexts
        const auto groupList = configGroup.groupList();
        for (const QString &context : groupList) {
            // Skip the friendly name group
            if (context == QLatin1String("Friendly Name"))
                continue;

            KConfigGroup contextGroup(&configGroup, context);
            QString contextFriendlyName = contextGroup.readEntry("_k_friendly_name");
            component->createGlobalShortcutContext(context, contextFriendlyName);
            component->activateGlobalShortcutContext(context);
            component->loadSettings(contextGroup);
        }

        // Load the default context
        component->activateGlobalShortcutContext("default");
        component->loadSettings(configGroup);
    }

    // Load the configured KServiceActions
    const QStringList desktopPaths =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("kglobalaccel"), QStandardPaths::LocateDirectory);
    for (const QString &path : desktopPaths) {
        QDir dir(path);
        if (!dir.exists()) {
            continue;
        }
        const QStringList patterns = {QStringLiteral("*.desktop")};
        const auto lstDesktopFiles = dir.entryList(patterns);
        for (const QString &desktopFile : lstDesktopFiles) {
            if (_components.contains(desktopFile)) {
                continue;
            }

            KDesktopFile f(dir.filePath(desktopFile));
            if (f.noDisplay()) {
                continue;
            }

            KdeDGlobalAccel::KServiceActionComponent *component = new KdeDGlobalAccel::KServiceActionComponent(desktopFile, f.readName(), this);
            component->activateGlobalShortcutContext(QStringLiteral("default"));
            component->loadFromService();
        }
    }
}

void GlobalShortcutsRegistry::grabKeys()
{
    activateShortcuts();
}

bool GlobalShortcutsRegistry::registerKey(int key, GlobalShortcut *shortcut)
{
    if (!_manager) {
        return false;
    }
    if (key == 0) {
        qCDebug(KGLOBALACCELD) << shortcut->uniqueName() << ": Key '" << QKeySequence(key).toString() << "' already taken by "
                               << _active_keys.value(key)->uniqueName() << ".";
        return false;
    } else if (_active_keys.value(key)) {
        qCDebug(KGLOBALACCELD) << shortcut->uniqueName() << ": Attempt to register key 0.";
        return false;
    }

    qCDebug(KGLOBALACCELD) << "Registering key" << QKeySequence(key).toString() << "for" << shortcut->context()->component()->uniqueName() << ":"
                           << shortcut->uniqueName();

    _active_keys.insert(key, shortcut);
    return _manager->grabKey(key, true);
}

void GlobalShortcutsRegistry::setAccelManager(KGlobalAccelInterface *manager)
{
    _manager = manager;
}

void GlobalShortcutsRegistry::setDBusPath(const QDBusObjectPath &path)
{
    _dbusPath = path;
}

KdeDGlobalAccel::Component *GlobalShortcutsRegistry::takeComponent(KdeDGlobalAccel::Component *component)
{
    QDBusConnection conn(QDBusConnection::sessionBus());
    conn.unregisterObject(component->dbusPath().path());
    return _components.take(component->uniqueName());
}

void GlobalShortcutsRegistry::ungrabKeys()
{
    deactivateShortcuts();
}

bool GlobalShortcutsRegistry::unregisterKey(int key, GlobalShortcut *shortcut)
{
    if (!_manager) {
        return false;
    }
    if (_active_keys.value(key) != shortcut) {
        // The shortcut doesn't own the key or the key isn't grabbed
        return false;
    }

    qCDebug(KGLOBALACCELD) << "Unregistering key" << QKeySequence(key).toString() << "for" << shortcut->context()->component()->uniqueName() << ":"
                           << shortcut->uniqueName();

    _manager->grabKey(key, false);
    _active_keys.take(key);
    return true;
}

void GlobalShortcutsRegistry::writeSettings() const
{
    const auto lst = GlobalShortcutsRegistry::self()->allMainComponents();
    for (const KdeDGlobalAccel::Component *component : lst) {
        KConfigGroup configGroup(&_config, component->uniqueName());
        if (component->allShortcuts().isEmpty()) {
            configGroup.deleteGroup();
            delete component;
        } else {
            component->writeSettings(configGroup);
        }
    }

    _config.sync();
}

#include "moc_globalshortcutsregistry.cpp"
