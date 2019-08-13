/**
 * @file libcomp/src/GameWorker.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Worker for client UI and scene interaction.
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

#include "GameWorker.h"

// libcomp Includes
#include <EnumUtils.h>
#include <Log.h>
#include <MessageClient.h>
#include <MessageShutdown.h>

// client Includes
#include "LobbyScene.h"
#include "LoginDialog.h"

using namespace game;

using libcomp::Message::MessageType;

GameWorker::GameWorker(QObject *pParent) : QObject(pParent),
    libcomp::Worker(), libcomp::Manager()
{
    // Connect the message queue to the Qt message system.
    connect(this, SIGNAL(SendMessageSignal(libcomp::Message::Message*)),
        this, SLOT(HandleMessageSignal(libcomp::Message::Message*)),
        Qt::QueuedConnection);

    // Setup the UI windows.
    mLobbyScene = new LobbyScene(this);
    mLoginDialog = new LoginDialog(this);

    // Register the client message managers.
    AddClientManager(mLobbyScene);
    AddClientManager(mLoginDialog);

    // Show the login dialog.
    mLoginDialog->show();
}

GameWorker::~GameWorker()
{
}

void GameWorker::AddClientManager(logic::ClientManager *pManager)
{
    if(pManager)
    {
        mClientManagers.insert(pManager);
    }
}

bool GameWorker::SendToLogic(libcomp::Message::Message *pMessage)
{
    if(mLogicMessageQueue)
    {
        mLogicMessageQueue->Enqueue(pMessage);

        return true;
    }
    else
    {
        return false;
    }
}

void GameWorker::SetLogicQueue(const std::shared_ptr<libcomp::MessageQueue<
    libcomp::Message::Message*>>& messageQueue)
{
    mLogicMessageQueue = messageQueue;
}

std::list<libcomp::Message::MessageType> GameWorker::GetSupportedTypes() const
{
    return {
        // MessageType::MESSAGE_TYPE_CONNECTION,
        MessageType::MESSAGE_TYPE_CLIENT,
    };
}

bool GameWorker::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    switch(to_underlying(pMessage->GetType()))
    {
        // case to_underlying(MessageType::MESSAGE_TYPE_CONNECTION):
        //     return ProcessConnectionMessage(
        //         (const libcomp::Message::ConnectionMessage*)pMessage);
        case to_underlying(MessageType::MESSAGE_TYPE_CLIENT):
            return ProcessClientMessage(
                (const libcomp::Message::MessageClient*)pMessage);
        default:
            break;
    }

    return false;
}

void GameWorker::Run(libcomp::MessageQueue<
    libcomp::Message::Message*> *pMessageQueue)
{
    // Add the manager after construction to avoid problems.
    AddManager(shared_from_this());

    libcomp::Worker::Run(pMessageQueue);
}

LoginDialog* GameWorker::GetLoginDialog() const
{
    return mLoginDialog;
}

LobbyScene* GameWorker::GetLobbyScene() const
{
    return mLobbyScene;
}

void GameWorker::HandleMessageSignal(libcomp::Message::Message *pMessage)
{
    libcomp::Worker::HandleMessage(pMessage);
}

void GameWorker::HandleMessage(libcomp::Message::Message *pMessage)
{
    libcomp::Message::Shutdown *pShutdown = dynamic_cast<
        libcomp::Message::Shutdown*>(pMessage);

    if(pShutdown)
    {
        libcomp::Worker::HandleMessage(pMessage);
    }
    else
    {
        emit SendMessageSignal(pMessage);
    }
}

bool GameWorker::ProcessClientMessage(
    const libcomp::Message::MessageClient *pMessage)
{
    bool didProcess = false;

    for(auto pManager : mClientManagers)
    {
        if(pManager->ProcessClientMessage(pMessage))
        {
            didProcess = true;
        }
    }

    if(!didProcess)
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Failed to process client message:\n%1\n")
                .Arg(pMessage->Dump());
        });
    }

    return true;
}
