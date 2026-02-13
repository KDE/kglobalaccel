/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KGLOBALSHORTCUTINFO_H
#define KGLOBALSHORTCUTINFO_H

#include "kglobalshortcuttrigger.h"

#include <kglobalaccel_export.h>

#include <QDBusArgument>
#include <QKeySequence>
#include <QList>
#include <QObject>

class KGlobalShortcutInfoPrivate;
class KGlobalShortcutInfoWrapperV3;

class KGLOBALACCEL_EXPORT KGlobalShortcutInfo : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.kde.kglobalaccel.KShortcutInfo")

    /* clang-format off */
    Q_SCRIPTABLE Q_PROPERTY(QString uniqueName READ uniqueName)
    Q_SCRIPTABLE Q_PROPERTY(QString friendlyName READ friendlyName)

    Q_SCRIPTABLE Q_PROPERTY(QString componentUniqueName READ componentUniqueName)
    Q_SCRIPTABLE Q_PROPERTY(QString componentFriendlyName READ componentFriendlyName)

    Q_SCRIPTABLE Q_PROPERTY(QString contextUniqueName READ contextUniqueName)
    Q_SCRIPTABLE Q_PROPERTY(QString contextFriendlyName READ contextFriendlyName)

    Q_SCRIPTABLE Q_PROPERTY(QList<QKeySequence> keys READ keys)
    Q_SCRIPTABLE Q_PROPERTY(QList<QKeySequence> defaultKeys READ keys)

    // Q_SCRIPTABLE Q_PROPERTY(QList<KGlobalShortcutTrigger> triggers READ triggers)
    // Q_SCRIPTABLE Q_PROPERTY(QList<KGlobalShortcutTrigger> defaultTriggers READ defaultTriggers)

public:
    KGlobalShortcutInfo();
    /* clang-format on */

    KGlobalShortcutInfo(const KGlobalShortcutInfo &rhs);
    KGlobalShortcutInfo(KGlobalShortcutInfo &&rhs);

    ~KGlobalShortcutInfo() override;

    KGlobalShortcutInfo &operator=(const KGlobalShortcutInfo &rhs);
    KGlobalShortcutInfo &operator=(KGlobalShortcutInfo &&rhs);

    QString contextFriendlyName() const;

    QString contextUniqueName() const;

    QString componentFriendlyName() const;

    QString componentUniqueName() const;

    QList<QKeySequence> defaultKeys() const;

    QString friendlyName() const;

    QList<QKeySequence> keys() const;

    QList<KGlobalShortcutTrigger> triggers() const;

    QList<KGlobalShortcutTrigger> defaultTriggers() const;

    QString uniqueName() const;

    bool hasInverseAction() const;
    QString inverseActionUniqueName() const;
    bool inverseActionCouplingIsMandatory() const;

    bool supportsOneToOneGesture() const;
    bool supportsFreeform2DGesture() const;

    enum Feature {
        InverseActionCouplingIsMandatory = 16,
        SupportsOneToOneGesture = 32,
        SupportsFreeform2DGesture = 64,
    };
    Q_DECLARE_FLAGS(FeatureFlags, Feature)
    Q_FLAG(FeatureFlags)

    uint featureFlags() const;

private:
    friend class GlobalShortcut;

    friend KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutInfo &shortcut);
    friend KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutInfoWrapperV3 &shortcut);
    friend KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, QKeySequence &sequence);

    //! Implementation details
    KGlobalShortcutInfoPrivate *d;
};

/*!
 * \class KGlobalShortcutInfoWrapperV3
 * \inmodule KGlobalAccel
 * \brief A wrapper around KGlobalShortcutInfo, with different D-Bus marshalling that includes triggers.
 */
class KGLOBALACCEL_EXPORT KGlobalShortcutInfoWrapperV3
{
public:
    KGlobalShortcutInfoWrapperV3();
    KGlobalShortcutInfoWrapperV3(KGlobalShortcutInfo wrapped);

    const KGlobalShortcutInfo &value() const;
    KGlobalShortcutInfo &value();

    static QList<KGlobalShortcutInfo> unwrap(QList<KGlobalShortcutInfoWrapperV3> &&listOfWrappers);

private:
    KGlobalShortcutInfo m_wrapped;
};

KGLOBALACCEL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KGlobalShortcutInfo &shortcut);
KGLOBALACCEL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const KGlobalShortcutInfoWrapperV3 &shortcut);
KGLOBALACCEL_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const QKeySequence &sequence);

KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutInfo &shortcut);
KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, KGlobalShortcutInfoWrapperV3 &shortcut);
KGLOBALACCEL_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, QKeySequence &sequence);

Q_DECLARE_METATYPE(KGlobalShortcutInfo)
Q_DECLARE_METATYPE(KGlobalShortcutInfoWrapperV3)

#endif /* #ifndef KGLOBALSHORTCUTINFO_H */
