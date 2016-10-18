/**
 * @file tools/logger/src/Settings.h
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the GUI class to modify the logger settings.
 *
 * Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_LOGGER_SRC_SETTINGS_H
#define TOOLS_LOGGER_SRC_SETTINGS_H

#include <PushIgnore.h>
#include "ui_Settings.h"

#include <QDialog>
#include <PopIgnore.h>

class LoggerServer;

/**
 * User interface to change the logger settings.
 * @ingroup logger
 */
class Settings : public QDialog
{
    Q_OBJECT

public:
    /**
     * Create the user interface widget.
     * @param server Logger server to change the settings for.
     * @param parent Parent object. This should be left to it's default value
     * because the settings interface is a dialog (window).
     */
    Settings(LoggerServer *server, QWidget *parent = 0);

public slots:
    /**
     * Save all settings in the dialog and close the window.
     */
    void saveAndClose();

    /**
     * The user has clicked the add client button and should be presented with
     * a dialog to select the directory where the client executable is located.
     */
    void addClient();

    /**
     * The user has clicked the remove client button and the client install
     * should be removed from the list.
     */
    void removeClient();

    /**
     * The selection in the client list has changed and the remove client
     * button should be enabled or disabled depending on if a client is
     * selected in the list.
     */
    void selectedClient();

signals:
    /**
     * Signal to notify the @ref MainWindow of the updated client list. This
     * signal is used to update the start game sub-menu.
     * @param clientList Mapping of client names and their associated install
     * path.
     * @sa MainWindow::updateClientList
     */
    void clientListChanged(const QVariantMap& clientList);

protected:
    /// LoggerServer whose settings are to be changed.
    LoggerServer *mServer;

    /// The generated user interface class.
    Ui::Settings ui;
};

#endif // TOOLS_LOGGER_SRC_SETTINGS_H
