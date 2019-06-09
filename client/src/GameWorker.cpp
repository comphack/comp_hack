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
#include <MessageShutdown.h>

// client Includes
#include "LoginDialog.h"

using namespace game;

GameWorker::GameWorker(QObject *pParent) : QObject(pParent), libcomp::Worker()
{
    // Connect the message queue to the Qt message system.
    connect(this, SIGNAL(SendMessageSignal(libcomp::Message::Message*)),
        this, SLOT(HandleMessageSignal(libcomp::Message::Message*)),
        Qt::QueuedConnection);

    // Setup the UI windows.
    (new LoginDialog(this))->show();
}

GameWorker::~GameWorker()
{
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
