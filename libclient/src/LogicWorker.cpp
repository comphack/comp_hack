/**
 * @file libcomp/src/LogicWorker.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Worker for client<==>server interaction.
 *
 * This file is part of the COMP_hack Client Library (libclient).
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

#include "LogicWorker.h"

// Managers
#include "ConnectionManager.h"
#include "LobbyManager.h"

using namespace logic;

LogicWorker::LogicWorker() : libcomp::Worker()
{
    // Construct the managers.
    auto connectionManager =
        std::make_shared<ConnectionManager>(this, GetMessageQueue());
    auto lobbyManager = std::make_shared<LobbyManager>(this, GetMessageQueue());

    // Save pointers to the managers.
    mConnectionManager = connectionManager.get();
    mLobbyManager = lobbyManager.get();

    // Add the managers so they may process the queue.
    AddManager(connectionManager);
    AddManager(lobbyManager);
}

LogicWorker::~LogicWorker()
{
    mConnectionManager = nullptr;
    mLobbyManager = nullptr;
}

bool LogicWorker::SendToGame(libcomp::Message::Message *pMessage)
{
    if(mGameMessageQueue)
    {
        mGameMessageQueue->Enqueue(pMessage);

        return true;
    }
    else
    {
        return false;
    }
}

void LogicWorker::SetGameQueue(
    const std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message *>>
        &messageQueue)
{
    mGameMessageQueue = messageQueue;
}

void LogicWorker::SendPacket(libcomp::Packet &packet)
{
    mConnectionManager->SendPacket(packet);
}

void LogicWorker::SendPacket(libcomp::ReadOnlyPacket &packet)
{
    mConnectionManager->SendPacket(packet);
}

void LogicWorker::SendPackets(const std::list<libcomp::Packet *> &packets)
{
    mConnectionManager->SendPackets(packets);
}

void LogicWorker::SendPackets(
    const std::list<libcomp::ReadOnlyPacket *> &packets)
{
    mConnectionManager->SendPackets(packets);
}

bool LogicWorker::SendObject(std::shared_ptr<libcomp::Object> &obj)
{
    return mConnectionManager->SendObject(obj);
}

bool LogicWorker::SendObjects(
    const std::list<std::shared_ptr<libcomp::Object>> &objs)
{
    return mConnectionManager->SendObjects(objs);
}
