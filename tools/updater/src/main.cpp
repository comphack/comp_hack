/**
 * @file tools/updater/src/main.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to update the game client.
 *
 * This tool will update the game client.
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <PushIgnore.h>
#ifdef COMP_HACK_HEADLESS
#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>

#include "Downloader.h"
#else // COMP_HACK_HEADLESS
#include <QApplication>

#include <QFile>
#include <QProcess>
#include <QFileInfo>
#include <QtPlugin>
#include <PopIgnore.h>

#ifdef STATIC
Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qgif)
#endif // STATIC

#ifdef Q_OS_WIN32
#include <windows.h>
#endif // Q_OS_WIN32

#include "Updater.h"
#endif // COMP_HACK_HEADLESS

int main(int argc, char *argv[])
{
#ifdef COMP_HACK_HEADLESS
    QCoreApplication app(argc, argv);

    QString settingsPath = "ImagineUpdate.dat";

    if(QFileInfo("ImagineUpdate-user.dat").exists())
    {
        settingsPath = "ImagineUpdate-user.dat";
    }

    QSettings settings(settingsPath, QSettings::IniFormat);
    QString url = settings.value("Setting/BaseURL1").toString();

    Downloader *dl = new Downloader(url);
    dl->startUpdate();

    QObject::connect(dl, SIGNAL(updateFinished()), &app, SLOT(quit()));

    return app.exec();
#else // COMP_HACK_HEADLESS
    QApplication app(argc, argv);

#ifdef Q_OS_WIN32
    QString path = QApplication::applicationFilePath();
    QString exe = QFileInfo(path).fileName();
    path = QFileInfo(path).path();

    Q_ASSERT( !path.isEmpty() && !exe.isEmpty() );

    bool isCopy = false;
    if(exe.left(1) == "_")
    {
        exe = exe.mid(1);
        isCopy = true;
    }

    QString normal = QString("%1/%2").arg(path).arg(exe);
    QString copy = QString("%1/_%2").arg(path).arg(exe);

    if( app.arguments().contains("--delete") )
        Sleep(3000);

    // Delete the copy
    if(!isCopy)
        QFile(copy).remove();

    // If we just want to delete the copy, exit now
    if( app.arguments().contains("--delete") )
        return 0;

    // Make the copy, start it, and exit
    if(!isCopy)
    {
        // Copy the updater
        QFile(normal).copy(copy);

        // Start the copy
        QProcess::startDetached(copy, QStringList(), path);

        // Exit
        return 0;
    }
#endif // Q_OS_WIN32

    (new Updater)->show();

    int ret = app.exec();

#ifdef Q_OS_WIN32
    // Delete the copy now
    if(isCopy)
        QProcess::startDetached(normal, QStringList() << "--delete", path);
#endif // Q_OS_WIN32

    return ret;
#endif // COMP_HACK_HEADLESS
}
