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

#include "LoginDialog.h"

// Qt Includes
#include <QDnsLookup>
#include <QHostAddress>
#include <QSettings>

// libcomp Includes
#include <ErrorCodes.h>

// client Includes
#include "GameWorker.h"
#include "LobbyScene.h"

// logic Messages
#include <MessageConnected.h>
#include <MessageConnectionInfo.h>

using namespace game;

using libcomp::Message::MessageClientType;

LoginDialog::LoginDialog(GameWorker *pWorker, QWidget *pParent) :
    QDialog(pParent), mGameWorker(pWorker)
{
    mDnsLookup = new QDnsLookup(this);

    ui.setupUi(this);

    mOriginalStatus = ui.statusLabel->text();

    connect(mDnsLookup, SIGNAL(finished()),
        this, SLOT(HaveDNS()));

    connect(ui.loginButton, SIGNAL(clicked(bool)),
        this, SLOT(Login()));
    connect(ui.username, SIGNAL(textChanged(const QString&)),
        this, SLOT(Validate()));
    connect(ui.password, SIGNAL(textChanged(const QString&)),
        this, SLOT(Validate()));

    connect(ui.username, SIGNAL(returnPressed()),
        this, SLOT(Login()));
    connect(ui.password, SIGNAL(returnPressed()),
        this, SLOT(Login()));
    connect(ui.host, SIGNAL(returnPressed()),
        this, SLOT(Login()));
    connect(ui.connectionID, SIGNAL(returnPressed()),
        this, SLOT(Login()));

    QSettings settings;

    ui.username->setText(settings.value("username",
        ui.username->text()).toString());
    ui.host->setText(settings.value("host",
        ui.host->text()).toString());
    ui.port->setValue(settings.value("port",
        ui.port->value()).toInt());
    ui.connectionID->setText(settings.value("connectionID",
        ui.connectionID->text()).toString());
    ui.rememberUsername->setChecked(settings.value("rememberUsername",
        ui.rememberUsername->isChecked()).toBool());
    ui.clientVersion->setValue(settings.value("clientVersion",
        (int)(ui.clientVersion->value() * 1000)).toInt() / 1000.0);

    Validate();

    if(ui.username->text().length() < 3)
    {
        ui.username->setFocus();
    }
    else
    {
        ui.password->setFocus();
    }
}

LoginDialog::~LoginDialog()
{
}

void LoginDialog::Validate()
{
    bool ok = true;

    if(ui.username->text().length() < 3)
    {
        ok = false;
    }

    if(ui.password->text().length() < 3)
    {
        ok = false;
    }

    ui.loginButton->setEnabled(ok);
}

void LoginDialog::Login()
{
    if(!ui.loginButton->isEnabled())
    {
        return;
    }

    // Disable the UI first.
    setEnabled(false);

    QString username = ui.username->text();
    QString password = ui.password->text();
    QString host = ui.host->text();
    int port = ui.port->value();
    QString connectionID = ui.connectionID->text();
    bool rememberUsername = ui.rememberUsername->isChecked();
    int clientVersion = (int)(ui.clientVersion->value() * 1000);

    QSettings settings;

    if(rememberUsername)
    {
        settings.setValue("username", username);
    }
    else
    {
        settings.setValue("username", QString());
    }

    settings.setValue("host", host);
    settings.setValue("port", port);
    settings.setValue("connectionID", connectionID);
    settings.setValue("rememberUsername", rememberUsername);
    settings.setValue("clientVersion", clientVersion);

    /// Resolve the DNS host name if needed.
    if(QHostAddress(host).isNull())
    {
        // Do a DNS lookup.
        mDnsLookup->setType(QDnsLookup::A);
        mDnsLookup->setName(host);
        mDnsLookup->lookup();

        return;
    }

    // Forward the request to the logic thread.
    mGameWorker->SendToLogic(new logic::MessageConnectToLobby(
        username.toUtf8().constData(), password.toUtf8().constData(),
        (uint32_t)clientVersion, connectionID.toUtf8().constData(),
        host.toUtf8().constData(), (uint16_t)port));
}

void LoginDialog::HaveDNS()
{
    if(mDnsLookup->error() != QDnsLookup::NoError)
    {
        /// @todo Print the error.
        setEnabled(true);

        return;
    }

    QString username = ui.username->text();
    QString password = ui.password->text();
    int port = ui.port->value();
    QString connectionID = ui.connectionID->text();
    int clientVersion = (int)(ui.clientVersion->value() * 1000);

    const auto records = mDnsLookup->hostAddressRecords();

    if(records.isEmpty())
    {
        /// @todo Print the error.
        setEnabled(true);

        return;
    }

    QString host = records.first().value().toString();

    // Forward the request to the logic thread.
    mGameWorker->SendToLogic(new logic::MessageConnectToLobby(
        username.toUtf8().constData(), password.toUtf8().constData(),
        (uint32_t)clientVersion, connectionID.toUtf8().constData(),
        host.toUtf8().constData(), (uint16_t)port));
}

bool LoginDialog::ProcessClientMessage(
    const libcomp::Message::MessageClient *pMessage)
{
    switch(to_underlying(pMessage->GetMessageClientType()))
    {
        case to_underlying(MessageClientType::CONNECTED_TO_LOBBY):
            return HandleConnectedToLobby(pMessage);
        default:
            break;
    }

    return false;
}

bool LoginDialog::HandleConnectedToLobby(
        const libcomp::Message::MessageClient *pMessage)
{
    const logic::MessageConnectedToLobby *pMsg = reinterpret_cast<
        const logic::MessageConnectedToLobby*>(pMessage);

    if(ErrorCodes_t::SUCCESS == pMsg->GetErrorCode())
    {
        // Enable the dialog again and clear the password.
        setEnabled(true);

        ui.password->setText(QString());
        ui.statusLabel->setText(mOriginalStatus);

        // Save the SID for later.
        mSID = pMsg->GetSID();

        // Show the lobby.
        mGameWorker->GetLobbyScene()->show();
        close();
    }
    else
    {
        QString errorMessage;

        // Get and display the error message.
        switch(to_underlying(pMsg->GetErrorCode()))
        {
            case to_underlying(ErrorCodes_t::BAD_USERNAME_PASSWORD):
                errorMessage = tr("Invalid username or password"); break;
            case to_underlying(ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN):
                errorMessage = tr("Account is still logged in"); break;
            case to_underlying(ErrorCodes_t::SERVER_FULL):
                errorMessage = tr("Server is full"); break;
            case to_underlying(ErrorCodes_t::WRONG_CLIENT_VERSION):
                errorMessage = tr("Please update your client"); break;
            case to_underlying(ErrorCodes_t::CONNECTION_TIMEOUT):
                errorMessage = tr("Connection to server has timed out"); break;
            default:
                errorMessage = tr("Unknown error"); break;
        }

        errorMessage = QString("<font color=\"Red\"><b>%1</b></font>").arg(
            errorMessage);

        ui.statusLabel->setText(errorMessage);

        // Some error let you try again, some do not.
        switch(to_underlying(pMsg->GetErrorCode()))
        {
            case to_underlying(ErrorCodes_t::BAD_USERNAME_PASSWORD):
            case to_underlying(ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN):
            case to_underlying(ErrorCodes_t::SERVER_FULL):
            case to_underlying(ErrorCodes_t::CONNECTION_TIMEOUT):
                setEnabled(true);

                ui.password->setFocus();
                break;
            default:
                break;
        }
    }

    return true;
}
