/**
 * @file tools/map/src/main.cpp
 * @ingroup map
 *
 * @author HACKfrost
 *
 * @brief Main source file for the map manager.
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // These settings are used to specify how the settings are stored. On
    // Windows, there settings are stored in the registry at
    // HKEY_CURRENT_USER\Software\COMP_hack\COMP_hack Map
    // On Linux, these settings will be stored in the file
    // $HOME/.config/COMP_hack/COMP_hack Map.conf
    // Consult the QSettings documentation in the Qt API reference for more
    // information on how the settings work (and where they are on Mac OS X).
    app.setOrganizationName("COMP_hack");
    app.setOrganizationDomain("comp.hack");
    app.setApplicationName("COMP_hack Map");

    auto datastore = std::make_shared<libcomp::DataStore>("comp_map");
    auto defintions = std::make_shared<libcomp::DefinitionManager>();

    QSettings settings;

    QString settingVal = settings.value("datastore", "error").toString();

    bool settingPath = false;
    if(settingVal == "error")
    {
        settingVal = QFileDialog::getExistingDirectory(nullptr, "Datastore path",
            "", QFileDialog::ShowDirsOnly);

        if(settingVal.isEmpty())
        {
            // Cancelling
            return EXIT_SUCCESS;
        }
        settingPath = true;
    }

    std::string err;
    if(!datastore->AddSearchPath(settingVal.toStdString()))
    {
        err = "Failed to add datastore search path from map.ini.";
    }
    else if(!defintions->LoadZoneData(&*datastore))
    {
        err = "Failed to load zone data.";
    }
    else if(!defintions->LoadDynamicMapData(&*datastore))
    {
        err = "Failed to load dynamic map data.";
    }
    else if(!defintions->LoadDevilData(&*datastore))
    {
        err = "Failed to load devil data.";
    }

    if(err.length() > 0)
    {
        QMessageBox Msgbox;
        Msgbox.setText(err.c_str());
        Msgbox.exec();

        return EXIT_FAILURE;
    }

    if(settingPath)
    {
        // Save the new ini now that we know its valid
        settings.setValue("datastore", settingVal);
        settings.sync();
    }

    (new MainWindow(datastore, defintions))->show();

    return app.exec();
}
