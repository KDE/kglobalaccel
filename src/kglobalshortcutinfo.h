/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KGLOBALSHORTCUTINFO_H
#define KGLOBALSHORTCUTINFO_H

#include <kglobalaccel_export.h>

#include <QList>
#include <QObject>
#include <QKeySequence>
#include <QDBusArgument>

class KGlobalShortcutInfoPrivate;

/**
 * @author Michael Jansen <kde@michael-jansen.biz>
 */
class KGLOBALACCEL_EXPORT KGlobalShortcutInfo : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.kde.kglobalaccel.KShortcutInfo")

    Q_SCRIPTABLE Q_PROPERTY(QString uniqueName READ uniqueName)
    Q_SCRIPTABLE Q_PROPERTY(QString friendlyName READ friendlyName)

    Q_SCRIPTABLE Q_PROPERTY(QString componentUniqueName READ componentUniqueName)
    Q_SCRIPTABLE Q_PROPERTY(QString componentFriendlyName READ componentFriendlyName)

    Q_SCRIPTABLE Q_PROPERTY(QString contextUniqueName READ contextUniqueName)
    Q_SCRIPTABLE Q_PROPERTY(QString contextFriendlyName READ contextFriendlyName)

    Q_SCRIPTABLE Q_PROPERTY(QList<QKeySequence> keys READ keys)
    Q_SCRIPTABLE Q_PROPERTY(QList<QKeySequence> defaultKeys READ keys)

public:

    KGlobalShortcutInfo();

    KGlobalShortcutInfo(const KGlobalShortcutInfo &rhs);

    ~KGlobalShortcutInfo();

    KGlobalShortcutInfo &operator= (const KGlobalShortcutInfo &rhs);

    QString contextFriendlyName() const;

    QString contextUniqueName() const;

    QString componentFriendlyName() const;

    QString componentUniqueName() const;

    QList<QKeySequence> defaultKeys() const;

    QString friendlyName() const;

    QList<QKeySequence> keys() const;

    QString uniqueName() const;

private:

    friend class GlobalShortcut;

    friend KGLOBALACCEL_EXPORT const QDBusArgument &operator>> (
        const QDBusArgument &argument,
        KGlobalShortcutInfo &shortcut);

    //! Implementation details
    KGlobalShortcutInfoPrivate *d;
};

KGLOBALACCEL_EXPORT QDBusArgument &operator<< (
    QDBusArgument &argument,
    const KGlobalShortcutInfo &shortcut);

KGLOBALACCEL_EXPORT const QDBusArgument &operator>> (
    const QDBusArgument &argument,
    KGlobalShortcutInfo &shortcut);

Q_DECLARE_METATYPE(KGlobalShortcutInfo)

#endif /* #ifndef KGLOBALSHORTCUTINFO_H */
