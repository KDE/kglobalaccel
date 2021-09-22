/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef GLOBALSHORTCUTSREGISTRY_H
#define GLOBALSHORTCUTSREGISTRY_H

#include "kglobalaccel.h"

#include <KSharedConfig>

#include <QDBusObjectPath>
#include <QHash>
#include <QKeySequence>
#include <QObject>

class GlobalShortcut;
class KGlobalAccelInterface;

namespace KdeDGlobalAccel
{
class Component;
}

/**
 * Global Shortcut Registry.
 *
 * Shortcuts are registered by component. A component is for example kmail or
 * amarok.
 *
 * A component can have contexts. Currently on plasma is planned to support
 * that feature. A context enables plasma to keep track of global shortcut
 * settings when switching containments.
 *
 * A shortcut (WIN+x) can be registered by one component only. The component
 * is allowed to register it more than once in different contexts.
 *
 * @author Michael Jansen <kde@michael-jansen.biz>
 */
class GlobalShortcutsRegistry : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.kde.KdedGlobalAccel.GlobalShortcutsRegistry")

public:
    /**
     * Use GlobalShortcutsRegistry::self()
     *
     * @internal
     */
    GlobalShortcutsRegistry();
    ~GlobalShortcutsRegistry() override;

    /**
     * Activate all shortcuts having their application present.
     */
    void activateShortcuts();

    /**
     * Return a list of all main components
     */
    QList<KdeDGlobalAccel::Component *> allMainComponents() const;

    /**
     * Return the root dbus path for the registry.
     */
    QDBusObjectPath dbusPath() const;

    /**
     * Deactivate all currently active shortcuts.
     */
    void deactivateShortcuts(bool temporarily = false);

    /**
     * Get the shortcut corresponding to key. Only active shortcut are
     * considered.
     */
    GlobalShortcut *getActiveShortcutByKey(const QKeySequence &key) const;

    /**
     */
    KdeDGlobalAccel::Component *getComponent(const QString &uniqueName);

    /**
     * Get the shortcut corresponding to key. Active and inactive shortcuts
     * are considered. But if the matching application uses contexts only one
     * shortcut is returned.
     *
     * @see getShortcutsByKey(int key)
     */
    GlobalShortcut *getShortcutByKey(const QKeySequence &key, KGlobalAccel::MatchType type = KGlobalAccel::MatchType::Equal) const;

    /**
     * Get the shortcuts corresponding to key. Active and inactive shortcuts
     * are considered.
     *
     * @see getShortcutsByKey(int key)
     */
    QList<GlobalShortcut *> getShortcutsByKey(const QKeySequence &key, KGlobalAccel::MatchType type) const;

    /**
     * Checks if @p shortcut is available for @p component.
     *
     * It is available if not used by another component in any context or used
     * by @p component only in not active contexts.
     */
    bool isShortcutAvailable(const QKeySequence &shortcut, const QString &component, const QString &context) const;

    static GlobalShortcutsRegistry *self();

    bool registerKey(const QKeySequence &key, GlobalShortcut *shortcut);

    void setAccelManager(KGlobalAccelInterface *manager);

    void setDBusPath(const QDBusObjectPath &path);

    bool unregisterKey(const QKeySequence &key, GlobalShortcut *shortcut);

public Q_SLOTS:

    void clear();

    void loadSettings();

    void writeSettings() const;

    // Grab the keys
    void grabKeys();

    // Ungrab the keys
    void ungrabKeys();

private:
    friend class KdeDGlobalAccel::Component;
    friend class KGlobalAccelInterface;

    KdeDGlobalAccel::Component *addComponent(KdeDGlobalAccel::Component *component);
    KdeDGlobalAccel::Component *takeComponent(KdeDGlobalAccel::Component *component);

    // called by the implementation to inform us about key presses
    // returns true if the key was handled
    bool keyPressed(int keyQt);

    QHash<QKeySequence, GlobalShortcut *> _active_keys;
    QKeySequence _active_sequence;
    QHash<int, int> _keys_count;
    QHash<QString, KdeDGlobalAccel::Component *> _components;

    KGlobalAccelInterface *_manager;

    mutable KConfig _config;

    QDBusObjectPath _dbusPath;
};

#endif /* #ifndef GLOBALSHORTCUTSREGISTRY_H */
