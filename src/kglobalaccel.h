/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>
    SPDX-FileCopyrightText: 2006 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2007 Andreas Hartmetz <ahartmetz@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KGLOBALACCEL_H_
#define _KGLOBALACCEL_H_

#include "kglobalshortcutinfo.h"
#include "kglobalshortcuttrigger.h"
#include <kglobalaccel_export.h>

#include <QKeySequence>
#include <QList>
#include <QObject>
#include <qflags.h>
#include <qtmetamacros.h>

class QAction;
class OrgKdeKglobalaccelComponentInterface;

/*!
 * \class KGlobalAccel
 * \inmodule KGlobalAccel
 * \brief Configurable global shortcut support.
 *
 * KGlobalAccel allows you to have global accelerators that are independent of
 * the focused window.  Unlike regular shortcuts, the application's window does not need focus
 * for them to be activated.
 *
 * Here's an example how to register the Meta + E global shortcut:
 *
 * \code
 * QAction *action = new QAction();
 * action->setObjectName(QStringLiteral("actionId"));
 * action->setText(QStringLiteral("human readable name"));
 * KGlobalAccel::self()->setDefaultShortcut(action, QList<QKeySequence>() << (Qt::META | Qt::Key_E));
 * KGlobalAccel::self()->setShortcut(action, QList<QKeySequence>() << (Qt::META | Qt::Key_E));
 * connect(action, &QAction::triggered, []() {
 *     qDebug() << "global shortcut trigerred";
 * });
 * \endcode
 *
 * \sa KKeySequenceRecorder
 */
class KGLOBALACCEL_EXPORT KGlobalAccel : public QObject
{
    Q_OBJECT

public:
    /*!
     * \enum KGlobalAccel::GlobalShortcutLoading
     *
     * An enum about global shortcut setter semantics
     *
     * \value Autoloading
     *        Look up the action in global settings (using its main component's name and text()) and
     *        set the shortcut as saved there.
     *        \sa setGlobalShortcut()
     * \value NoAutoloading
     *        Prevent autoloading of saved global shortcut for action.
     */
    enum GlobalShortcutLoading {
        Autoloading = 0x0,
        NoAutoloading = 0x4,
    };

    /*!
     * \enum KGlobalAccel::actionIdFields
     *
     * Index for actionId QStringLists
     *
     * \value ComponentUnique
     *        Components Unique Name (ID).
     * \value ActionUnique
     *        Actions Unique Name (ID).
     * \value ComponentFriendly
     *        Components Friendly Translated Name
     * \value ActionFriendly
     *        Actions Friendly Translated Name
     */
    enum actionIdFields {
        ComponentUnique = 0,
        ActionUnique = 1,
        ComponentFriendly = 2,
        ActionFriendly = 3,
    };

    /*!
     * \enum KGlobalAccel::MatchType
     *
     * Keysequence match semantics.
     *
     * Assuming we have an Emacs-style shortcut, for example (Alt+B, Alt+F, Alt+G) already assigned,
     * how a new shortcut is compared depends on which value of the enum is used.
     *
     * \value Equal Exact matching: (Alt+B, Alt+F, Alt+G)
     * \value Shadows Sequence shadowing: (Alt+B, Alt+F), (Alt+F, Alt+G)
     * \value Shadowed Sequence being shadowed: (Alt+B, Alt+F, Alt+G, <any key>), (<any key>, Alt+B, Alt+F, Alt+G)
     *
     * \since 5.90
     */
    enum MatchType {
        Equal,
        Shadows,
        Shadowed,
    };
    Q_ENUM(MatchType)

    /*!
     * \enum KGlobalAccel::InverseActionCoupling
     *
     * An enum about the relationship of two inverse actions
     *
     * \value OptionalCoupling
     *        When the user (re)assigns or unassigns a shortcut trigger for one of the two actions,
     *        a settings application may prompt the user if the trigger (of the same trigger type)
     *        for its inverse action should likewise be (re)assigned or unassigned.
     *        This is merely a convenience; either action is sufficient by itself.
     * \value MandatoryCoupling
     *        When the user (re)assigns a shortcut trigger for one of the two actions, a settings
     *        application should ensure that the trigger (of the same trigger type) for its
     *        inverse action is also (re)assigned. Likewise, if the user unassigns a trigger,
     *        the equivalent trigger for the inverse action should also be unassigned.
     *        KGlobalAccel may enforce this by activating either both or none of the shortcuts.
     *
     * \sa setInverseShortcutActions()
     */
    enum InverseActionCoupling {
        OptionalCoupling = 0,
        MandatoryCoupling = KGlobalShortcutInfo::Feature::InverseActionCouplingIsMandatory,
    };
    Q_ENUM(InverseActionCoupling)

    /*!
     * \enum KGlobalAccel::GestureSupport
     *
     * An enum about custom gesture integration behaviors that are supported for a shortcut action
     *
     * \value SupportsOneShotTriggerOnly
     *        Completing an assigned gesture will trigger the action just like a keyboard shortcut
     *        would. There are no custom behaviors for in-progress gestures. This is the default
     *        and supported for every action.
     * \value SupportsOneToOneGesture
     *        The component that provides the action is able to recognize in-progress gestures
     *        and provide visual feedback before the action is triggered. Progress in this case
     *        is measured on a linear scale from 0% to 100%. Opting into such gesture APIs may
     *        only be available to components with access to compositor internals.
     * \value SupportsFreeform2DGesture
     *        The component that provides the action is able to recognize free-form gestures that
     *        cannot be mapped to a simple on/off or 0%-to-100% linear scale. Starting the gesture
     *        will enter a dedicated mode, for example to select items in a 2D arrangement.
     *        (If free-form gestures are supported, it should usually be for an action that would
     *        otherwise simply open the dedicated mode.)
     *        A settings UI that assigns a free-form gesture should take care to unassign other
     *        potentially conflicting gestures. Primarily, this means that a free-form 2D swipe
     *        gesture is incompatible with any one-dimensional swipe gestures.
     *
     * \sa setShortcut(), setDefaultShortcut(), setGlobalShortcut()
     */
    enum GestureSupport {
        SupportsOneShotTriggerOnly = 0,
        SupportsOneToOneGesture = KGlobalShortcutInfo::Feature::SupportsOneToOneGesture,
        SupportsFreeform2DGesture = KGlobalShortcutInfo::Feature::SupportsFreeform2DGesture,
    };
    Q_DECLARE_FLAGS(GestureSupportFlags, GestureSupport)
    Q_FLAG(GestureSupportFlags)

    /*!
     * Returns (and creates if necessary) the singleton instance
     */
    static KGlobalAccel *self();

    /*!
     * Take away the given shortcut \a seq from the named action it belongs to.
     * This applies to all actions with global shortcuts in any KDE application.
     *
     * \sa promptStealShortcutSystemwide()
     */
    static void stealShortcutSystemwide(const QKeySequence &seq);

    /*!
     * Take away the given shortcut \a trigger from the named action it belongs to.
     * This applies to all actions with global shortcuts in any KDE application.
     *
     * \sa promptStealShortcutSystemwide()
     */
    static void stealShortcutSystemwide(const KGlobalShortcutTrigger &trigger);

    /*!
     * Clean the shortcuts for component \a componentUnique.
     *
     * If the component is not active all global shortcut registrations are
     * purged and the component is removed completely.
     *
     * If the component is active all global shortcut registrations not in use
     * will be purged. If there is no shortcut registration left the component
     * is purged too.
     *
     * If a purged component or shortcut is activated the next time it will
     * reregister itself. All you probably will lose on wrong usage are the
     * user's set shortcuts.
     *
     * If you make sure your component is running and all global shortcuts it
     * has are active this function can be used to clean up the registry.
     *
     * Handle with care!
     *
     * If the method return \c true at least one shortcut was purged so handle
     * all previously acquired information with care.
     */
    static bool cleanComponent(const QString &componentUnique);

    /*!
     * Returns \c true if the component with the given \a componentName exists; otherwise returns \c false.
     */
    static bool isComponentActive(const QString &componentName);

    /*!
     * Returns a list of global shortcuts registered for the shortcut \a seq using
     * the given matching \a type.
     *
     * If the list contains more that one entry it means the component
     * that registered the shortcuts uses global shortcut contexts. All
     * returned shortcuts belong to the same component.
     *
     * \since 5.90
     */
    static QList<KGlobalShortcutInfo> globalShortcutsByKey(const QKeySequence &seq, MatchType type = Equal);

    /*!
     * Returns a list of global shortcuts registered for the shortcut \a trigger
     * using the given match \a type.
     *
     * If the list contains more that one entry it means the component
     * that registered the shortcuts uses global shortcut contexts.
     * All returned shortcuts belong to the same component.
     *
     * \since 6.24 // TODO: update once approved for merging
     */
    static QList<KGlobalShortcutInfo> globalShortcutsByTrigger(const KGlobalShortcutTrigger &trigger, MatchType type = Equal);

    /*!
     * Returns \c true if the shortcut \a seq is available for the \a component; otherwise returns
     * \c false.
     *
     * The component is only of interest if the current application uses global shortcut
     * contexts. In that case a global shortcut by \a component in an inactive
     * global shortcut contexts does not block the \a seq for us.
     *
     * \since 4.2
     */
    static bool isGlobalShortcutAvailable(const QKeySequence &seq, const QString &component = QString());

    /*!
     * Returns \c true if the shortcut \a trigger is available for the \a component;
     * otherwise returns \c false.
     *
     * The component is only of interest if the current application uses global shortcut
     * contexts. In that case a global shortcut by \a component in an inactive
     * global shortcut contexts does not block the \a trigger for us.
     *
     * \since 6.24 // TODO: update version once approved
     */
    static bool isGlobalShortcutTriggerAvailable(const KGlobalShortcutTrigger &trigger, const QString &comp);

    /*!
     * Show a messagebox to inform the user that a global shortcut is already occupied,
     * and ask to take it away from its current action(s). This is GUI only, so nothing will
     * be actually changed.
     *
     * Returns \c true if user confirms that it is okay to re-assign the global shortcut;
     * otherwise returns \c false.
     *
     * \sa stealShortcutSystemwide()
     *
     * \since 4.2
     */
    static bool promptStealShortcutSystemwide(QWidget *parent, const QList<KGlobalShortcutInfo> &shortcuts, const QKeySequence &seq);

    /*!
     * Assign a default global \a shortcut for a given \a action.
     *
     * For more information about global shortcuts and \a loadFlag, see setShortcut().
     *
     * Upon shortcut change the globalShortcutChanged() will be triggered so other applications get notified.
     *
     * Returns \c true if the shortcut has been set successfully; otherwise returns \c false.
     *
     * \sa globalShortcutChanged()
     *
     * \since 5.0
     */
    bool setDefaultShortcut(QAction *action, const QList<QKeySequence> &shortcut, GlobalShortcutLoading loadFlag = Autoloading);

    /*!
     * Assign default global shortcut \a keys and \a triggers for a given \a action.
     *
     * For more information about global shortcuts and \a loadFlag, see setShortcut().
     *
     * \a supportFlags specifies which interaction behaviors are supported for this shortcut.
     * Most shortcuts will simply execute a one-shot action when triggered. Some shortcuts will
     * also support deeper integration with in-progress gestures. A settings UI can use this
     * information to determine shortcut assignment behaviors; apart from that, KGlobalAccel does
     * not provide an API for tracking in-progress gestures.
     *
     * Upon shortcut change the globalShortcutChanged() and globalShortcutTriggersChanged() signals
     * will be triggered so other applications get notified.
     *
     * Returns \c true if the shortcut has been set successfully; otherwise returns \c false.
     *
     * \sa globalShortcutChanged()
     * \sa globalShortcutTriggersChanged()
     *
     * \since 6.24 // TODO: update version once approved
     */
    bool setDefaultShortcut(QAction *action,
                            const QList<QKeySequence> &keys,
                            const QList<KGlobalShortcutTrigger> &triggers,
                            GestureSupportFlags gestureSupport = SupportsOneShotTriggerOnly,
                            GlobalShortcutLoading loadFlag = Autoloading);

    /*!
     * Assign a global shortcut for the given action.
     *
     * Global shortcuts allow an action to respond to key shortcuts independently of the focused window,
     * i.e. the action will trigger if the keys were pressed no matter where in the session.
     *
     * The action must have a per main component unique
     * action->objectName() to enable cross-application bookkeeping. If the action->objectName() is
     * empty this method will do nothing and will return false.
     *
     * It is mandatory that the action->objectName() doesn't change once the shortcut has been
     * successfully registered.
     *
     * \note KActionCollection::insert(name, action) will set action's objectName to name so you often
     * don't have to set an objectName explicitly.
     *
     * When an action, identified by main component name and objectName(), is assigned
     * a global shortcut for the first time on a KDE installation the assignment will
     * be saved. The shortcut will then be restored every time setGlobalShortcut() is
     * called with \a loadFlag == \c KGlobalAccel::Autoloading.
     *
     * If you actually want to change the global shortcut you have to set
     * \a loadFlag to NoAutoloading. The new shortcut will be automatically saved again.
     *
     * \a action specifies the action for which the shortcut will be assigned.
     *
     * \a shortcut specifies the global shortcut(s) to assign. Will be ignored unless \a loadFlag
     * is set to \c KGlobalAccel::NoAutoloading or this is the first time ever you call this method
     * (see above).
     *
     * If \a loadFlag is \c KGlobalAccel::Autoloading, assign the global shortcut this action has
     * previously had if any. That way user preferences and changes made to avoid clashes will be
     * conserved. If \c KGlobalAccel::NoAutoloading the given shortcut will be assigned without
     * looking up old values. You should only do this if the user wants to change the shortcut or
     * if you have another very good reason. Key combinations that clash with other shortcuts will be
     * dropped.
     *
     * \note the default shortcut will never be influenced by autoloading - it will be set as given.
     * \sa shortcut(), globalShortcutChanged()
     * \since 5.0
     */
    bool setShortcut(QAction *action, const QList<QKeySequence> &shortcut, GlobalShortcutLoading loadFlag = Autoloading);

    /*!
     * Assign global shortcut \a keys and \a extraTriggers for the given action.
     *
     * Most applications should limit themselves to keyboard shortcuts, and should use the
     * other overload without the \a extraTriggers parameter.
     *
     * Global shortcuts allow an action to respond to key and gesture shortcuts independently of
     * the focused window, i.e. the action will trigger if the keys were pressed no matter where
     * in the session.
     *
     * The action must have a per main component unique
     * action->objectName() to enable cross-application bookkeeping. If the action->objectName() is
     * empty this method will do nothing and will return false.
     *
     * It is mandatory that the action->objectName() doesn't change once the shortcut has been
     * successfully registered.
     *
     * \note KActionCollection::insert(name, action) will set action's objectName to name so you often
     * don't have to set an objectName explicitly.
     *
     * When an action, identified by main component name and objectName(), is assigned
     * a global shortcut for the first time on a KDE installation the assignment will
     * be saved. The shortcut will then be restored every time setGlobalShortcut() is
     * called with \a loadFlag == \c KGlobalAccel::Autoloading.
     *
     * If you actually want to change the global shortcut you have to set
     * \a loadFlag to NoAutoloading. The new shortcut will be automatically saved again.
     *
     * \a action specifies the action for which the shortcut will be assigned.
     *
     * \a keys specifies the global shortcut(s) to assign. Will be ignored unless \a loadFlag is
     * set to \c KGlobalAccel::NoAutoloading or this is the first time ever you call this method
     * (see above).
     *
     * \a extraTriggers specifies additional global shortcut triggers to assign. A compositor may
     * or may support various types of extra triggers. Will be ignored unless \a loadFlag is set to
     * \c NoAutoloading or this is the first time ever you call this method (see above).
     *
     * If \a loadFlag is KGlobalAccel::Autoloading, assign the global shortcut this action has
     * previously had if any. That way user preferences and changes made to avoid clashes will be
     * conserved. If KGlobalAccel::NoAutoloading the given shortcut will be assigned without
     * looking up old values. You should only do this if the user wants to change the shortcut or
     * if you have another very good reason. Key combinations that clash with other shortcuts will be
     * dropped.
     *
     * \note the default shortcut will never be influenced by autoloading - it will be set as given.
     * \sa shortcut(), globalShortcutChanged(), globalShortcutTriggersChanged()
     * \since 6.24 // TODO: update version once approved
     */
    bool setShortcut(QAction *action,
                     const QList<QKeySequence> &keys,
                     const QList<KGlobalShortcutTrigger> &extraTriggers,
                     GlobalShortcutLoading loadFlag = Autoloading);

    /*!
     * Sets both active and default shortcut \a shortcuts for the given \a action.
     *
     * If more control for loading the shortcuts is needed use the variants offering more control.
     *
     * Returns \c true if the shortcut has been set successfully; otherwise returns \c false.
     *
     * \sa setShortcut(), setDefaultShortcut()
     * \since 5.0
     */
    static bool setGlobalShortcut(QAction *action, const QList<QKeySequence> &shortcuts);

    /*!
     * Sets both active and default \a shortcut for the given \a action.
     *
     * This method is suited for the case that only one shortcut is to be configured.
     *
     * If more control for loading the shortcuts is needed, use the variants offering more control.
     *
     * Returns \c true if the shortcut has been set successfully; otherwise returns \c false.
     *
     * \sa setShortcut(), setDefaultShortcut()
     * \since 5.0
     */
    static bool setGlobalShortcut(QAction *action, const QKeySequence &shortcut);

    /*!
     * Sets both active and default shortcut \a keys and \a extraTriggers for the given \a action.
     *
     * If more control for loading the shortcuts is needed, use the variants offering more control.
     *
     * Returns \c true if the shortcut has been set successfully; otherwise returns \c false.
     *
     * \sa setShortcut(), setDefaultShortcut()
     * \since 6.24 // TODO: update version once approved
     */
    static bool setGlobalShortcut(QAction *action,
                                  const QList<QKeySequence> &keys,
                                  const QList<KGlobalShortcutTrigger> &extraTriggers,
                                  GestureSupportFlags gestureSupport = SupportsOneShotTriggerOnly);

    /*!
     * Designates \a forwardAction and \a backwardAction as inverse actions of each other.
     *
     * Both actions must have been set with \c setDefaultShortcut() or \a setGlobalShortcut()
     * before calling this function. Their component name (e.g. "kwin") must be the same.
     *
     * When configuring a trigger for one of these two actions, the settings application should
     * prompt the user to also (re)assign or modify a trigger of the same kind for its
     * inverse action.
     *
     * Both actions may be shown next to each other in trigger lists.
     *
     * \a coupling specifies if one shortcut requires the its counterpart to be available as well.
     *
     * Returns \c true if the shortcut has been set successfully; otherwise returns \c false.
     *
     * \sa setShortcut(), setDefaultShortcut(), setGlobalShortcut(), InverseActionCoupling
     * \since 6.24 // TODO: update version once approved
     */
    static bool setInverseShortcutActions(QAction *forwardAction, QAction *backwardAction, InverseActionCoupling coupling = MandatoryCoupling);

    /*!
     * Get the global default keyboard shortcut for this \a action, if one exists. Global shortcuts
     * allow your actions to respond to accellerators independently of the focused window.
     * Unlike regular shortcuts, the application's window does not need focus
     * for them to be activated.
     *
     * \sa setDefaultShortcut()
     * \since 5.0
     */
    QList<QKeySequence> defaultShortcut(const QAction *action) const;

    /*!
     * Get the global keyboard shortcut for this \a action, if one exists. Global shortcuts
     * allow your actions to respond to accelerators independently of the focused window.
     * Unlike regular shortcuts, the application's window does not need focus
     * for them to be activated.
     *
     * \note that this method only works together with setShortcut() because the action pointer
     * is used to retrieve the result. If you would like to retrieve the shortcut as stored
     * in the global settings, use the globalShortcut(componentName, actionId) instead.
     *
     * \sa setShortcut()
     * \since 5.0
     */
    QList<QKeySequence> shortcut(const QAction *action) const;

    /*!
     * Retrieves the keyboard shortcut as defined in global settings by
     * \a componentName (e.g. "kwin") and \a actionId (e.g. "Kill Window").
     *
     * \since 5.10
     */
    QList<QKeySequence> globalShortcut(const QString &componentName, const QString &actionId) const;

    /*!
     * Unregister and remove all defined global shortcuts for the given \a action.
     *
     * \since 5.0
     */
    void removeAllShortcuts(QAction *action);

    /*!
     * Returns true if a shortcut or a default shortcut has been registered for the given \a action.
     *
     * \since 5.0
     */
    bool hasShortcut(const QAction *action) const;

Q_SIGNALS:
    // KF7 TODO: change "QAction *" to "const QAction *" in all the signals below,
    //           and any remaining functions above

    /*!
     * Emitted when the global shortcut is changed. A global shortcut is subject to be changed by
     * the global shortcuts kcm.
     *
     * \a action specifies the action for which the changed shortcut was registered.
     *
     * \a seq indicates the key sequence that corresponds to the changed shortcut.
     *
     * \sa setGlobalShortcut(), setDefaultShortcut()
     * \since 5.0
     */
    void globalShortcutChanged(QAction *action, const QKeySequence &seq);

    /*!
     * Emitted when a global shortcut is changed. A global shortcut is subject to be
     * changed by a System Settings KCM such as Shortcuts, Mouse & Touchpad, or Screen Edges.
     *
     * \a action specifies the action for which the changed shortcut was registered.
     *
     * \a trigger indicates the key sequence that corresponds to the changed shortcut.
     *
     * \sa setGlobalShortcut(), setDefaultShortcut() // TODO: should there be trigger versions of these as well? I guess so
     * \since 6.24 // TODO: set this correctly before merging
     */
    void globalShortcutTriggersChanged(QAction *action, const QList<KGlobalShortcutTrigger> &trigger);

    /*!
     * Emitted when a global shortcut for the given \a action is activated or deactivated.
     *
     * The global shortcut will be activated when the keys are pressed and deactivated when the
     * keys are released. For gesture triggers, the global shortcut will be activated when the
     * gesture starts and deactivated when it is triggered or cancelled. \a active indicates
     * whether the global shortcut is active.
     *
     * \since 5.94
     */
    void globalShortcutActiveChanged(QAction *action, bool active);

private:
    KGLOBALACCEL_NO_EXPORT KGlobalAccel();
    KGLOBALACCEL_NO_EXPORT ~KGlobalAccel() override;

    KGLOBALACCEL_NO_EXPORT OrgKdeKglobalaccelComponentInterface *getComponent(const QString &componentUnique);

    class KGlobalAccelPrivate *const d;

    friend class KGlobalAccelSingleton;
};

KGLOBALACCEL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KGlobalAccel::MatchType &type);
KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalAccel::MatchType &type);

#endif // _KGLOBALACCEL_H_
