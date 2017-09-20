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

    auto datastore = std::make_shared<libcomp::DataStore>("comp_map");
    auto defintions = std::make_shared<libcomp::DefinitionManager>();
    
    QSettings settings("map.ini", QSettings::IniFormat);

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
