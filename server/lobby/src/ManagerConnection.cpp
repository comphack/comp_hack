/**
 * @file server/lobby/src/ManagerConnection.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Manager to handle lobby connections to world servers.
 *
 * This file is part of the Lobby Server (lobby).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#include "ManagerConnection.h"

// libcomp Includes
#include "Log.h"
#include "MessageConnectionClosed.h"
#include "MessageEncrypted.h"
#include "MessageWorldNotification.h"

#include <thread>

using namespace lobby;

ManagerConnection::ManagerConnection(std::shared_ptr<libcomp::BaseServer> server,
    std::shared_ptr<asio::io_service> service,
    std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>> messageQueue)
{
    mServer = server;
    mService = service;
    mMessageQueue = messageQueue;
}

ManagerConnection::~ManagerConnection()
{
}

std::list<libcomp::Message::MessageType>
ManagerConnection::GetSupportedTypes() const
{
    std::list<libcomp::Message::MessageType> supportedTypes;

    supportedTypes.push_back(
        libcomp::Message::MessageType::MESSAGE_TYPE_CONNECTION);

    return supportedTypes;
}

bool ManagerConnection::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    const libcomp::Message::WorldNotification *notification = dynamic_cast<
        const libcomp::Message::WorldNotification*>(pMessage);

    if(nullptr != notification)
    {
        uint16_t port = notification->GetPort();
        libcomp::String address = notification->GetAddress();

        LOG_DEBUG(libcomp::String("Attempting to connect back to World: %1:%2\n").Arg(address).Arg(port));

        std::shared_ptr<libcomp::InternalConnection> worldConnection(
            new libcomp::InternalConnection(*mService));

        worldConnection->SetSelf(worldConnection);
        worldConnection->SetMessageQueue(mMessageQueue);

        // Connect and stay connected until either of us shutdown
        if(worldConnection->Connect(address, port, true))
        {
            auto world = std::shared_ptr<lobby::World>(new lobby::World(worldConnection));
            LOG_INFO(libcomp::String("New World connection established: %1:%2\n").Arg(address).Arg(port));
            mWorlds.push_back(world);
            return true;
        }
        
        LOG_ERROR(libcomp::String("World connection failed: %1:%2\n").Arg(address).Arg(port));
    }

    const libcomp::Message::Encrypted *encrypted = dynamic_cast<
        const libcomp::Message::Encrypted*>(pMessage);

    if(nullptr != encrypted)
    {
        auto connection = encrypted->GetConnection();
        for(auto world : mWorlds)
        {
            if (world->GetConnection() == connection)
            {
                return world->Initialize();
            }
        }
    }

    const libcomp::Message::ConnectionClosed *closed = dynamic_cast<
        const libcomp::Message::ConnectionClosed*>(pMessage);

    if(nullptr != closed)
    {
        auto connection = std::shared_ptr<libcomp::TcpConnection>(closed->GetConnection());

        mServer->RemoveConnection(connection);

        //If this is from an internal connection, it is a world connection, else it is a client connection
        auto iConnection = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);
        if(nullptr != iConnection)
        {
            auto world = GetWorldByConnection(std::shared_ptr<libcomp::InternalConnection>(iConnection));
            if(nullptr != world)
            {
                RemoveWorld(world);
            }
        }

        return true;
    }
    
    return false;
}

std::list<std::shared_ptr<lobby::World>> ManagerConnection::GetWorlds()
{
    return mWorlds;
}

std::shared_ptr<lobby::World> ManagerConnection::GetWorldByConnection(const std::shared_ptr<libcomp::InternalConnection>& connection)
{
    for (auto world : mWorlds)
    {
        if (world->GetConnection() == connection)
        {
            return world;
        }
    }

    return nullptr;
}

void ManagerConnection::RemoveWorld(std::shared_ptr<lobby::World>& world)
{
    auto desc = world->GetWorldDescription();
    LOG_INFO(libcomp::String("World connection removed: (%1) %2\n").Arg(desc.GetID()).Arg(desc.GetName()));

    auto iter = std::find(mWorlds.begin(), mWorlds.end(), world);
    if(iter != mWorlds.end())
    {
        mWorlds.erase(iter);
    }
}