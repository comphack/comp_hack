/**
 * @file tools/cathedral/src/main.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main source file for the Cathedral of Content.
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

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // These settings are used to specify how the settings are stored. On
  // Windows, there settings are stored in the registry at
  // HKEY_CURRENT_USER\Software\COMP_hack\COMP_hack Cathedral of Content
  // On Linux, these settings will be stored in the file
  // $HOME/.config/COMP_hack/COMP_hack Cathedral of Content.conf
  // Consult the QSettings documentation in the Qt API reference for more
  // information on how the settings work (and where they are on Mac OS X).
  app.setOrganizationName("COMP_hack");
  app.setOrganizationDomain("comp.hack");
  app.setApplicationName("COMP_hack Cathedral of Content");
  // app.setStyle("fusion");

  MainWindow *pWindow = new MainWindow;

  if (pWindow->Init()) {
    pWindow->show();
  } else {
    return EXIT_SUCCESS;
  }

  return app.exec();
}
