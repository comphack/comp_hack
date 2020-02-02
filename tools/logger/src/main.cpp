/**
 * @file tools/logger/src/main.cpp
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main source file of the packet logging application.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "LoggerServer.h"

#include <PushIgnore.h>
#include <QCoreApplication>
#include <QTranslator>
#include <QFileInfo>
#include <QSettings>
#include <QDateTime>
#include <QLocale>
#include <QString>
#include <QFile>
#include <QDir>

#include <iostream>
#include <zlib.h>

#ifndef COMP_LOGGER_HEADLESS
#include <QApplication>
#include <PopIgnore.h>

#include "MainWindow.h"
#else // COMP_LOGGER_HEADLESS
#include <PopIgnore.h>
#endif // COMP_LOGGER_HEADLESS

 /**
  * This is the main function for the packet logging application. There are two
  * versions of this application: GUI and headless. The GUI version is meant
  * for periodic use by end-users. The headless version is designed to be run
  * in the background on a server or for users who are familiar with the
  * command line and do not wish to use a GUI.
  * @arg argc Number of arguments passed to the application.
  * @arg argv Array of arguments passed to the application.
  * @returns Return code when appplication exits.
  */
int main(int argc, char *argv[])
{
#ifdef COMP_LOGGER_HEADLESS
    QCoreApplication app(argc, argv);
#else // COMP_LOGGER_HEADLESS
    QApplication app(argc, argv);
#endif // COMP_LOGGER_HEADLESS

    // These settings are used to specify how the settings are stored. On
    // Windows, there settings are stored in the registry at
    // HKEY_CURRENT_USER\Software\COMP_hack\COMP_hack Logger
    // On Linux, these settings will be stored in the file
    // $HOME/.config/COMP_hack/COMP_hack Logger.conf
    // Consult the QSettings documentation in the Qt API reference for more
    // information on how the settings work (and where they are on Mac OS X).
    app.setOrganizationName("COMP_hack");
    app.setOrganizationDomain("comp.hack");
    app.setApplicationName("COMP_hack Logger");

    // We will now load the capture path from the settings file. The default
    // settings path on Windows is My Documents\Captures and on Linux is
    // $HOME/Captures.
    QString capturePath;

    {
        QSettings settings;

        // Load the capture path setting and provide a different default path
        // depending on the OS. If the setting doesn't exist the default path
        // will be used.
#ifdef Q_OS_WIN32
        QString defaultPath = QString("%1/My Documents/Captures").arg(
            QDir::homePath());
#else // Q_OS_UNIX
        QString defaultPath = QString("%1/Captures").arg(QDir::homePath());
#endif // Q_OS_WIN32

        capturePath = settings.value("General/CapturePath",
            defaultPath).toString();

        // Check if the path exists, if it doesn't attempt to create it.
        if( !QDir(capturePath).exists() )
        {
            if( !QDir(capturePath).mkpath(capturePath) )
            {
                std::cerr << "Failed to create capture directory '"
                    << capturePath.toLocal8Bit().data() << "'." << std::endl;

                return -1;
            }
        }

        // Generate a test capture file path.
        QString testPath = QDir(capturePath).absoluteFilePath("test.comp");

        QFile testFile(testPath);

        // Attempt to open the test capture file for writing.
        if( !testFile.open(QIODevice::WriteOnly) )
        {
            std::cerr << "Failed to open (write) test capture file '"
                << testPath.toLocal8Bit().data() << "'." << std::endl;

            return -2;
        }

        // Attempt to write some data into the test capture file.
        if(testFile.write("writeTest\n", 10) != 10)
        {
            std::cerr << "Failed to write to test capture file '"
                << testPath.toLocal8Bit().data() << "'." << std::endl;

            return -3;
        }

        testFile.close();

        // Make sure the test file now reports the right size.
        if(QFileInfo(testPath).size() != 10)
        {
            std::cerr << "Invalid size for test capture file '"
                << testPath.toLocal8Bit().data() << "'." << std::endl;

            return -4;
        }

        // Attempt to open the test file for reading back in.
        if( !testFile.open(QIODevice::ReadOnly) )
        {
            std::cerr << "Failed to open (read) test capture file '"
                << testPath.toLocal8Bit().data() << "'." << std::endl;

            return -5;
        }

        char buffer[12];
        memset(buffer, 0, 12);

        // Attempt to read the contents of the test file.
        if(testFile.read(buffer, 10) != 10)
        {
            std::cerr << "Failed to read test capture file '"
                << testPath.toLocal8Bit().data() << "'." << std::endl;

            return -6;
        }

        // Check that the test file contains the correct data.
        if(memcmp("writeTest\n", buffer, 10) != 0)
        {
            std::cerr << "Corrupt data read from test capture file '"
                << testPath.toLocal8Bit().data() << "'." << std::endl;

            return -7;
        }

        testFile.close();

        // Attempt to remove the test file.
        if( !testFile.remove() )
        {
            std::cerr << "Failed to delete test capture file '"
                << testPath.toLocal8Bit().data() << "'." << std::endl;

            return -8;
        }
    }

#ifdef COMP_LOGGER_HEADLESS
    // In headless mode, print the capture path to the user.
    std::cout << "----------------------------------------"
        << "----------------------------------------" << std::endl;
    std::cout << "Capture Directory: "
        << capturePath.toLocal8Bit().data() << std::endl;
    std::cout << "----------------------------------------"
        << "----------------------------------------" << std::endl;

    // Create the logger server, pass the capture path, and start the server.
    LoggerServer *server = new LoggerServer;
    server->setCapturePath(capturePath);
    server->startServer();
#else // COMP_LOGGER_HEADLESS
    // Create the main window. The GUI mode server will be started and
    // controlled from this window.
    MainWindow *main = new MainWindow(capturePath);
    main->show();
#endif // COMP_LOGGER_HEADLESS

    // Run the application until the main window has been closed or the user
    // presses Control+C (for headless mode).
    return app.exec();
}
