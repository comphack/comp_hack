/**
 * @file tools/capgrep/src/main.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main source file for the utility.
 *
 * This tool will display the contents of game packets.
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

#include <QApplication>

// Stop ignoring warnings
#include <PopIgnore.h>

/**
 * This is the main function for the packet analysis application. This
 * application displays channel packet captures produced by the logger.
 * @arg argc Number of arguments passed to the application.
 * @arg argv Array of arguments passed to the application.
 * @returns Return code when appplication exits.
 */
int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // These settings are used to specify how the settings are stored. On
  // Windows, there settings are stored in the registry at
  // HKEY_CURRENT_USER\Software\COMP_hack\COMP_hack Capture Grep
  // On Linux, these settings will be stored in the file
  // $HOME/.config/COMP_hack/COMP_hack Capture Grep.conf
  // Consult the QSettings documentation in the Qt API reference for more
  // information on how the settings work (and where they are on Mac OS X).
  app.setOrganizationName("COMP_hack");
  app.setOrganizationDomain("comp.hack");
  app.setApplicationName("COMP_hack Capture Grep");

  // Remove the border around widgets added to the status bar.
  app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");

  // Create and display the main window.
  (new MainWindow)->show();

  // Run the main application event loop.
  return app.exec();
}
