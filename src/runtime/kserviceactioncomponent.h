/*
    SPDX-FileCopyrightText: 2008 Michael Jansen <kde@michael-jansen.biz>
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSERVICEACTIONCOMPONENT_H
#define KSERVICEACTIONCOMPONENT_H

#include "component.h"

#include <KDesktopFile>

#include <memory>

/**
 * @author Michael Jansen <kde@michael-jansen.biz>
 */
class KServiceActionComponent : public Component
{
    Q_OBJECT

public:
    ~KServiceActionComponent() override;

    void loadFromService();
    void emitGlobalShortcutPressed(const GlobalShortcut &shortcut) override;

    bool cleanUp() override;

private:
    friend class ::GlobalShortcutsRegistry;
    //! Constructs a KServiceActionComponent. This is a private constuctor, to create
    //! a KServiceActionComponent, use GlobalShortcutsRegistry::self()->createServiceActionComponent().
    KServiceActionComponent(const QString &serviceStorageId, const QString &friendlyName);

    void runProcess(const KConfigGroup &group, const QString &token);

    QString m_serviceStorageId;
    std::unique_ptr<KDesktopFile> m_desktopFile;
    bool m_isInApplicationsDir = false;
};

#endif /* #ifndef KSERVICEACTIONCOMPONENT_H */
