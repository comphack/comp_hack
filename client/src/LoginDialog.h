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

#ifndef LIBCLIENT_SRC_LOGINDIALOG_H
#define LIBCLIENT_SRC_LOGINDIALOG_H

// Qt Includes
#include "ui_LoginDialog.h"

// libclient Includes
#include <ClientManager.h>

// Qt Forward Declarations
class QDnsLookup;

namespace game
{

class GameWorker;

/**
 * Dialog to login the client (to the lobby).
 */
class LoginDialog : public QDialog, public logic::ClientManager
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

    /**
     * Process a client message.
     * @param pMessage Client message to process.
     */
    bool ProcessClientMessage(
        const libcomp::Message::MessageClient *pMessage);

private slots:
    /**
     * Called when the login button is clicked.
     */
    void Login();

    /**
     * Validate the form when it changes.
     */
    void Validate();

    /**
     * Called after a DNS record has been resolved (or on error).
     */
    void HaveDNS();

private:
    /**
     * Handle the authentication reply.
     * @param pMessage Client message to process.
     */
    bool HandleConnectedToLobby(
        const libcomp::Message::MessageClient *pMessage);

    /// Pointer to the GameWorker.
    GameWorker *mGameWorker;

    /// Handles DNS record lookups.
    QDnsLookup *mDnsLookup;

    /// Original status message.
    QString mOriginalStatus;

    /// Session ID for this connection.
    libcomp::String mSID;

    /// UI for this dialog.
    Ui::LoginDialog ui;
};

} // namespace game

#endif // LIBCLIENT_SRC_LOGINDIALOG_H
