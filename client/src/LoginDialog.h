/**
 * @file client/src/LoginDialog.h
 * @ingroup client
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Login dialog.
 *
 * This file is part of the COMP_hack Test Client (client).
 *
 * Copyright (C) 2012-2019 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCLIENT_SRC_LOGINDIALOG_H
#define LIBCLIENT_SRC_LOGINDIALOG_H

// Qt Includes
#include "ui_LoginDialog.h"

namespace game
{

class GameWorker;

/**
 * Dialog to login the client (to the lobby).
 */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Construct the login dialog.
     * @param pWorker The GameWorker for the UI.
     * @param pParent Parent Qt widget for the dialog.
     */
    LoginDialog(GameWorker *pWorker, QWidget *pParent = nullptr);

    /**
     * Cleanup the dialog.
     */
    ~LoginDialog() override;

private slots:
    /**
     * Called when the login button is clicked.
     */
    void Login();

private:
    /// Pointer to the GameWorker.
    GameWorker *mGameWorker;

    /// UI for this dialog.
    Ui::LoginDialog ui;
};

} // namespace game

#endif // LIBCLIENT_SRC_LOGINDIALOG_H
