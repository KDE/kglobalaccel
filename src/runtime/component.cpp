/* Copyright (C) 2008 Michael Jansen <kde@michael-jansen.biz>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "component.h"

#include "globalshortcut.h"
#include "globalshortcutcontext.h"
#include "globalshortcutsregistry.h"
#include "logging_p.h"
#include <config-kglobalaccel.h>
#include "kglobalaccel_interface.h"

#include <QStringList>
#include <QKeySequence>

#if HAVE_X11
#include <QX11Info>
#endif

static QList<int> keysFromString(const QString &str)
{
    QList<int> ret;
    if (str == QLatin1String("none")) {
        return ret;
    }
    const QStringList strList = str.split('\t');
    for (const QString &s : strList) {
        int key = QKeySequence(s)[0];
        if (key != -1) {     //sanity check just in case
            ret.append(key);
        }
    }
    return ret;
}


static QString stringFromKeys(const QList<int> &keys)
{
    if (keys.isEmpty()) {
        return QStringLiteral("none");
    }
    QString ret;
    for (int key : keys) {
        ret.append(QKeySequence(key).toString());
        ret.append('\t');
    }
    ret.chop(1);
    return ret;
}

namespace KdeDGlobalAccel {

Component::Component(
            const QString &uniqueName,
            const QString &friendlyName,
            GlobalShortcutsRegistry *registry)
    :   _uniqueName(uniqueName)
        ,_friendlyName(friendlyName)
        ,_registry(registry)
    {
    // Make sure we do no get uniquenames still containing the context
    Q_ASSERT(uniqueName.indexOf("|")==-1);

    // Register ourselve with the registry
    if (_registry)
        {
        _registry->addComponent(this);
        }

    QString DEFAULT=QStringLiteral("default");
    createGlobalShortcutContext(DEFAULT, QStringLiteral("Default Context"));
    _current = _contexts.value(DEFAULT);
    }


Component::~Component()
    {
    // Remove ourselve from the registry
    if (_registry)
        {
        _registry->takeComponent(this);
        }

    // We delete all shortcuts from all contexts
    qDeleteAll(_contexts);
    }


bool Component::activateGlobalShortcutContext(
        const QString &uniqueName)
    {
    if (!_contexts.value(uniqueName))
        {
        createGlobalShortcutContext(uniqueName, "TODO4");
        return false;
        }

    // Deactivate the current contexts shortcuts
    deactivateShortcuts();

    // Switch the context
    _current = _contexts.value(uniqueName);

    return true;
    }


void Component::activateShortcuts()
    {
    for (GlobalShortcut *shortcut : qAsConst(_current->_actions))
        {
        shortcut->setActive();
        }
    }


QList<GlobalShortcut*> Component::allShortcuts(const QString &contextName) const
    {
    GlobalShortcutContext *context = _contexts.value(contextName);
    if (context)
        {
        return context->_actions.values();
        }
    else
        {
        return QList<GlobalShortcut*> ();
        }
    }


QList<KGlobalShortcutInfo> Component::allShortcutInfos(const QString &contextName) const
    {
    GlobalShortcutContext *context = _contexts.value(contextName);
    if (!context)
        {
        return QList<KGlobalShortcutInfo>();
        }

    return context->allShortcutInfos();
    }


bool Component::cleanUp()
    {
    bool changed = false;

    const auto actions = _current->_actions;
    for (GlobalShortcut *shortcut : actions)
        {
        qCDebug(KGLOBALACCELD) << _current->_actions.size();
        if (!shortcut->isPresent())
            {
            changed = true;
            shortcut->unRegister();
            }
        }

    if (changed) {
        _registry->writeSettings();
        // We could be destroyed after this call!
    }

    return changed;
    }


bool Component::createGlobalShortcutContext(
        const QString &uniqueName,
        const QString &friendlyName)
    {
    if (_contexts.value(uniqueName))
        {
        qCDebug(KGLOBALACCELD) << "Shortcut Context " << uniqueName << "already exists for component " << _uniqueName;
        return false;
        }
    _contexts.insert(uniqueName, new GlobalShortcutContext(uniqueName, friendlyName, this));
    return true;
    }


GlobalShortcutContext *Component::currentContext()
    {
    return _current;
    }


QDBusObjectPath Component::dbusPath() const
    {
    QString dbusPath = _uniqueName;
    // Clean up for dbus usage: any non-alphanumeric char should be turned into '_'
    const int len = dbusPath.length();
    for ( int i = 0; i < len; ++i )
        {
        if ( !dbusPath[i].isLetterOrNumber() || dbusPath[i].unicode() >= 0x7F )
            // DBus path can only contain ASCII characters
            dbusPath[i] = QLatin1Char('_');
        }
    // QDBusObjectPath could be a little bit easier to handle :-)
    return QDBusObjectPath( _registry->dbusPath().path() + "component/" + dbusPath);
    }


void Component::deactivateShortcuts(bool temporarily)
    {
    for (GlobalShortcut *shortcut : qAsConst(_current->_actions))
        {
        if (temporarily
                && uniqueName() == QLatin1String("kwin")
                && shortcut->uniqueName() == QLatin1String("Block Global Shortcuts"))
            {
            continue;
            }
        shortcut->setInactive();
        }
    }


void Component::emitGlobalShortcutPressed( const GlobalShortcut &shortcut )
    {
#if HAVE_X11
    // pass X11 timestamp
    long timestamp = QX11Info::appTime();
    // Make sure kglobalacceld has ungrabbed the keyboard after receiving the
    // keypress, otherwise actions in application that try to grab the
    // keyboard (e.g. in kwin) may fail to do so. There is still a small race
    // condition with this being out-of-process.
    if (_registry->_manager) {
        _registry->_manager->syncWindowingSystem();
    }
#else
    long timestamp = 0;
#endif

    // Make sure it is one of ours
    if (shortcut.context()->component() != this)
        {
        // In production mode do nothing
        return;
        }

    emit globalShortcutPressed(
            shortcut.context()->component()->uniqueName(),
            shortcut.uniqueName(),
            timestamp);
    }

void Component::invokeShortcut(const QString &shortcutName, const QString &context)
    {
        GlobalShortcut *shortcut = getShortcutByName(shortcutName, context);
        if (shortcut) emitGlobalShortcutPressed(*shortcut);
    }

QString Component::friendlyName() const
    {
    if (_friendlyName.isEmpty())
        return _uniqueName;
    return _friendlyName;
    }


GlobalShortcut *Component::getShortcutByKey(int key) const
    {
    return _current->getShortcutByKey(key);
    }


QList<GlobalShortcut *> Component::getShortcutsByKey(int key) const
    {
    QList <GlobalShortcut *> rc;
    for (GlobalShortcutContext *context : qAsConst(_contexts))
        {
        GlobalShortcut *sc = context->getShortcutByKey(key);
        if (sc) rc.append(sc);
        }
    return rc;
    }


GlobalShortcut *Component::getShortcutByName(const QString &uniqueName, const QString &context) const
    {
    if (!_contexts.value(context))
        {
        return nullptr;
        }

    return _contexts.value(context)->_actions.value(uniqueName);
    }


QStringList Component::getShortcutContexts() const
    {
    return _contexts.keys();
    }


bool Component::isActive() const
    {
    // The component is active if at least one of it's global shortcuts is
    // present.
    for (GlobalShortcut *shortcut : qAsConst(_current->_actions))
        {
        if (shortcut->isPresent()) return true;
        }
    return false;
    }


bool Component::isShortcutAvailable(
        int key,
        const QString &component,
        const QString &context) const
    {
    qCDebug(KGLOBALACCELD) << QKeySequence(key).toString() << component;

    // if this component asks for the key. only check the keys in the same
    // context
    if (component==uniqueName())
        {
        const auto actions = shortcutContext(context)->_actions;
        for (GlobalShortcut *sc : actions)
            {
            if (sc->keys().contains(key)) return false;
            }
        }
    else
        {
        for (GlobalShortcutContext *ctx : qAsConst(_contexts))
            {
            for (GlobalShortcut *sc : qAsConst(ctx->_actions))
                {
                if (sc->keys().contains(key)) return false;
                }
            }
        }
    return true;
    }

GlobalShortcut *Component::registerShortcut(const QString &uniqueName, const QString &friendlyName, const QString &shortcutString, const QString &defaultShortcutString)
    {
    // The shortcut will register itself with us
    GlobalShortcut *shortcut = new GlobalShortcut(
            uniqueName,
            friendlyName,
            currentContext());

    const QList<int> keys = keysFromString(shortcutString);
    shortcut->setDefaultKeys(keysFromString(defaultShortcutString));
    shortcut->setIsFresh(false);
    QList<int> newKeys = keys;
    for (int key : keys)
        {
        if (key != 0)
            {
            if (GlobalShortcutsRegistry::self()->getShortcutByKey(key))
                {
                // The shortcut is already used. The config file is
                // broken. Ignore the request.
                newKeys.removeAll(key);
                qCWarning(KGLOBALACCELD) << "Shortcut found twice in kglobalshortcutsrc."<<key;
                }
            }
        }
    shortcut->setKeys(keys);
    return shortcut;
    }


void Component::loadSettings(KConfigGroup &configGroup)
    {
    // GlobalShortcutsRegistry::loadSettings handles contexts.
    const auto listKeys = configGroup.keyList();
    for (const QString &confKey : listKeys)
        {
        const QStringList entry = configGroup.readEntry(confKey, QStringList());
        if (entry.size() != 3)
            {
            continue;
            }

        GlobalShortcut *shortcut = registerShortcut(confKey, entry[2], entry[0], entry[1]);
        if (configGroup.name().endsWith(QLatin1String(".desktop"))) {
            shortcut->setIsPresent(true);
        }
        }
    }


void Component::setFriendlyName(const QString &name)
    {
    _friendlyName = name;
    }


GlobalShortcutContext *Component::shortcutContext( const QString &contextName )
    {
    return _contexts.value(contextName);
    }


GlobalShortcutContext const *Component::shortcutContext( const QString &contextName ) const
    {
    return _contexts.value(contextName);
    }


QStringList Component::shortcutNames( const QString &contextName) const
    {
    GlobalShortcutContext *context = _contexts.value(contextName);
    if (!context)
        {
        return QStringList();
        }

    return context->_actions.keys();
    }


QString Component::uniqueName() const
    {
    return _uniqueName;
    }


void Component::unregisterShortcut(const QString &uniqueName)
    {
    // Now wrote all contexts
    for( GlobalShortcutContext *context : qAsConst(_contexts))
        {
        if (context->_actions.value(uniqueName))
            {
            delete context->takeShortcut(context->_actions.value(uniqueName));
            }
        }
    }


void Component::writeSettings(KConfigGroup& configGroup) const
    {
    // If we don't delete the current content global shortcut
    // registrations will never not deleted after forgetGlobalShortcut()
    configGroup.deleteGroup();


    // Now write all contexts
    for( GlobalShortcutContext *context : qAsConst(_contexts))
        {
        KConfigGroup contextGroup;

        if (context->uniqueName() == QLatin1String("default"))
            {
            contextGroup = configGroup;
            // Write the friendly name
            contextGroup.writeEntry("_k_friendly_name", friendlyName());
            }
        else
            {
            contextGroup = KConfigGroup(&configGroup, context->uniqueName());
            // Write the friendly name
            contextGroup.writeEntry("_k_friendly_name", context->friendlyName());
            }

        // qCDebug(KGLOBALACCELD) << "writing group " << _uniqueName << ":" << context->uniqueName();

        for (const GlobalShortcut *shortcut : qAsConst(context->_actions))
            {
            // qCDebug(KGLOBALACCELD) << "writing" << shortcut->uniqueName();

            // We do not write fresh shortcuts.
            // We do not write session shortcuts
            if (shortcut->isFresh() || shortcut->isSessionShortcut())
                {
                continue;
                }
            // qCDebug(KGLOBALACCELD) << "really writing" << shortcut->uniqueName();

            QStringList entry(stringFromKeys(shortcut->keys()));
            entry.append(stringFromKeys(shortcut->defaultKeys()));
            entry.append(shortcut->friendlyName());

            contextGroup.writeEntry(shortcut->uniqueName(), entry);
            }
        }
    }

} // namespace KdeDGlobalAccel
