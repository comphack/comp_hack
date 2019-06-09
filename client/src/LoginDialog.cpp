/**
 * @file client/src/LoginDialog.cpp
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

#include "LoginDialog.h"

// client Includes
#include "GameWorker.h"

// libclient Includes
#include <MessageConnectionInfo.h>

using namespace game;

LoginDialog::LoginDialog(GameWorker *pWorker, QWidget *pParent) :
    QDialog(pParent), mGameWorker(pWorker)
{
    ui.setupUi(this);

    connect(ui.loginButton, SIGNAL(clicked(bool)),
        this, SLOT(Login()));
}

LoginDialog::~LoginDialog()
{
}

void LoginDialog::Login()
{
    // Disable the UI first.
    setEnabled(false);

    // Forward the request to the logic thread.
    mGameWorker->SendToLogic(new logic::MessageConnectToLobby("lobby@1"));
}
