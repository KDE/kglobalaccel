/*
    This file is part of the KDE project

    SPDX-FileCopyrightText: 2007 Andreas Hartmetz <ahartmetz@gmail.com>
    SPDX-FileCopyrightText: 2007 Michael Jansen <kde@michael-jansen.biz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kglobalaccel_version.h"
#include "kglobalacceld.h"
#include "logging_p.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <QCommandLineParser>
#include <QGuiApplication>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

static bool isEnabled()
{
    // TODO: Check if kglobalaccel can be disabled
    return true;
}

extern "C" Q_DECL_EXPORT int main(int argc, char **argv)
{
    // Disable Session Management the right way (C)
    //
    // ksmserver has global shortcuts. disableSessionManagement() does not prevent Qt from
    // registering the app with the session manager. We remove the address to make sure we do not
    // get a hang on kglobalaccel restart (kglobalaccel tries to register with ksmserver,
    // ksmserver tries to register with kglobalaccel).
    qunsetenv("SESSION_MANAGER");

    QGuiApplication::setDesktopSettingsAware(false);
    QGuiApplication app(argc, argv);
    KAboutData aboutdata(QStringLiteral("kglobalaccel"),
                         QObject::tr("KDE Global Shortcuts Service"),
                         QStringLiteral(KGLOBALACCEL_VERSION_STRING),
                         QObject::tr("KDE Global Shortcuts Service"),
                         KAboutLicense::LGPL,
                         QStringLiteral("(C) 2007-2009  Andreas Hartmetz, Michael Jansen"));
    aboutdata.addAuthor(QStringLiteral("Andreas Hartmetz"), QObject::tr("Maintainer"), QStringLiteral("ahartmetz@gmail.com"));
    aboutdata.addAuthor(QStringLiteral("Michael Jansen"), QObject::tr("Maintainer"), QStringLiteral("kde@michael-jansen.biz"));

    KAboutData::setApplicationData(aboutdata);

    {
        QCommandLineParser parser;
        aboutdata.setupCommandLine(&parser);
        parser.process(app);
        aboutdata.processCommandLine(&parser);
    }

    // check if kglobalaccel is disabled
    if (!isEnabled()) {
        qCDebug(KGLOBALACCELD) << "kglobalaccel is disabled!";
        return 0;
    }

#ifdef Q_OS_UNIX
    // It's possible that kglobalaccel gets started as the wrong user by
    // accident, e.g. kdesu dolphin leads to dbus activation. It then installs
    // its grabs and the actions are run as the wrong user.
    bool isUidset = false;
    const int sessionuid = qEnvironmentVariableIntValue("KDE_SESSION_UID", &isUidset);
    if(isUidset && static_cast<uid_t>(sessionuid) != getuid()) {
        qCWarning(KGLOBALACCELD) << "kglobalaccel running as wrong user, exiting.";
        return 0;
    }
#endif

    KDBusService service(KDBusService::Unique);

    app.setQuitOnLastWindowClosed(false);

    // Not needed in Qt6, the feature and the workaround (i.e. setFallbackSessionManagementEnabled)
    // are both gone. See:
    // https://code.qt.io/cgit/qt/qtbase.git/commit/?id=7f878c6217754dd2b63401bf1c006476e5dc27eb
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#ifndef QT_NO_SESSIONMANAGER
    QGuiApplication::setFallbackSessionManagementEnabled(false);
#endif
#endif

    // Restart on a crash
    KCrash::setFlags(KCrash::AutoRestart);

    KGlobalAccelD globalaccel;
    if (!globalaccel.init()) {
        return -1;
    }

    return app.exec();
}
