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

// lobby Includes
#include "LobbyConfig.h"
#include "LobbyServer.h"

// libcomp Includes
#include <DatabaseConfigSQLite3.h>
#include <Log.h>
#include <MessageConnectionClosed.h>
#include <MessageEncrypted.h>
#include <MessageWorldNotification.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>

using namespace lobby;

std::list<libcomp::Message::MessageType> ManagerConnection::sSupportedTypes =
    { libcomp::Message::MessageType::MESSAGE_TYPE_CONNECTION };

ManagerConnection::ManagerConnection(std::weak_ptr<libcomp::BaseServer> server,
    asio::io_service* service,
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
    return sSupportedTypes;
}

bool ManagerConnection::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    const libcomp::Message::ConnectionMessage *cMessage = dynamic_cast<
        const libcomp::Message::ConnectionMessage*>(pMessage);

    switch(cMessage->GetConnectionMessageType())
    {
        case libcomp::Message::ConnectionMessageType::CONNECTION_MESSAGE_WORLD_NOTIFICATION:
            {
                const libcomp::Message::WorldNotification *notification = dynamic_cast<
                    const libcomp::Message::WorldNotification*>(cMessage);

                uint16_t port = notification->GetPort();
                libcomp::String address = notification->GetAddress();

                LOG_DEBUG(libcomp::String(
                    "Attempting to connect back to World: %1:%2\n").Arg(
                    address).Arg(port));

                auto worldConnection = std::make_shared<
                    libcomp::InternalConnection>(*mService);

                worldConnection->SetMessageQueue(mMessageQueue);

                // Connect and stay connected until either of us shutdown
                if(worldConnection->Connect(address, port, true))
                {
                    auto world = std::make_shared<lobby::World>();
                    world->SetConnection(worldConnection);

                    LOG_INFO(libcomp::String(
                        "New World connection established: %1:%2\n").Arg(
                        address).Arg(port));

                    mUnregisteredWorlds.push_back(world);
                    return true;
                }
        
                LOG_ERROR(libcomp::String("World connection failed: %1:%2\n").Arg(address).Arg(port));
            }
            break;
        case libcomp::Message::ConnectionMessageType::CONNECTION_MESSAGE_ENCRYPTED:
            {
                const libcomp::Message::Encrypted *encrypted = dynamic_cast<
                    const libcomp::Message::Encrypted*>(cMessage);

                auto connection = encrypted->GetConnection();
                auto world = GetWorldByConnection(std::dynamic_pointer_cast<libcomp::InternalConnection>(connection));
                if (nullptr != world)
                {
                    return InitializeWorld(world);
                }
                else
                {
                    //Nothing special to do
                    return true;
                }
            }
            break;
        case libcomp::Message::ConnectionMessageType::CONNECTION_MESSAGE_CONNECTION_CLOSED:
            {
                const libcomp::Message::ConnectionClosed *closed = dynamic_cast<
                    const libcomp::Message::ConnectionClosed*>(cMessage);

                auto connection = closed->GetConnection();
                auto server = mServer.lock();
                server->RemoveConnection(connection);

                auto clientConnection = std::dynamic_pointer_cast<lobby::LobbyClientConnection>(connection);
                RemoveClientConnection(clientConnection);

                auto iConnection = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);
                auto world = GetWorldByConnection(iConnection);
                RemoveWorld(world);

                return true;
            }
            break;
    }
    
    return false;
}

bool ManagerConnection::InitializeWorld(const std::shared_ptr<lobby::World>& world)
{
    libcomp::Packet packet;
    packet.WritePacketCode(InternalPacketCode_t::PACKET_GET_WORLD_INFO);

    auto server = std::dynamic_pointer_cast<LobbyServer>(mServer.lock());
    if(!server->GetMainDatabase()->GetConfig()->SavePacket(packet, false))
    {
        return false;
    }

    world->GetConnection()->SendPacket(packet);

    return true;
}

std::list<std::shared_ptr<lobby::World>> ManagerConnection::GetWorlds() const
{
    return mWorlds;
}

std::shared_ptr<lobby::World> ManagerConnection::GetWorldByID(uint8_t id) const
{
    //Return registered worlds first
    for(auto worldList : { mWorlds, mUnregisteredWorlds })
    {
        for(auto world : worldList)
        {
            if(world->GetRegisteredWorld()->GetID() == id)
            {
                return world;
            }
        }
    }

    return nullptr;
}

std::shared_ptr<lobby::World> ManagerConnection::GetWorldByConnection(
    const std::shared_ptr<libcomp::InternalConnection>& connection) const
{
    if(nullptr == connection)
    {
        return nullptr;
    }

    //Return registered worlds first
    for(auto worldList : { mWorlds, mUnregisteredWorlds })
    {
        for(auto world : worldList)
        {
            if(world->GetConnection() == connection)
            {
                return world;
            }
        }
    }

    return nullptr;
}

const std::shared_ptr<lobby::World> ManagerConnection::RegisterWorld(
    std::shared_ptr<lobby::World>& world)
{
    auto registeredWorld = world->GetRegisteredWorld();
    if(nullptr == registeredWorld)
    {
        return nullptr;
    }

    if(std::find(mWorlds.begin(), mWorlds.end(), world) != mWorlds.end())
    {
        //Already registered, nothing to do
        return world;
    }

    //Remove previous unregistered references
    RemoveWorld(world);

    auto id = registeredWorld->GetID();
    auto connection = world->GetConnection();
    auto db = world->GetWorldDatabase();

    auto existing = GetWorldByID(id);
    if(nullptr == existing && nullptr != connection)
    {
        existing = GetWorldByConnection(connection);
    }

    if(nullptr != existing)
    {
        //Update the existing world and return that
        if(nullptr != connection)
        {
            if(nullptr == existing->GetConnection())
            {
                existing->SetConnection(connection);
            }
            else if(existing->GetConnection() != connection)
            {
                LOG_CRITICAL("Multiple world servers registered with"
                    " the same information.\n");
                return nullptr;
            }
        }

        if(nullptr == existing->GetRegisteredWorld())
        {
            RemoveWorld(existing);
            mWorlds.push_back(existing);
        }
        existing->RegisterWorld(registeredWorld);
        existing->SetWorldDatabase(db);
        return existing;
    }
    else
    {
        //New world registered
        mWorlds.push_back(world);
        return world;
    }
}

void ManagerConnection::RemoveWorld(std::shared_ptr<lobby::World>& world)
{
    if(nullptr != world)
    {
        auto iter = std::find(mWorlds.begin(), mWorlds.end(), world);
        if(iter != mWorlds.end())
        {
            auto svr = world->GetRegisteredWorld();
            auto server = std::dynamic_pointer_cast<LobbyServer>(mServer.lock());

            mWorlds.erase(iter);
            if(nullptr != svr)
            {
                LOG_INFO(libcomp::String("World connection removed: (%1) %2\n")
                    .Arg(svr->GetID()).Arg(svr->GetName()));

                server->QueueWork([](std::shared_ptr<LobbyServer> lobbyServer,
                    int8_t worldID)
                    {
                        auto accountManager = lobbyServer->GetAccountManager();
                        auto usernames = accountManager->LogoutUsersInWorld(worldID);

                        if(usernames.size() > 0)
                        {
                            LOG_WARNING(libcomp::String("%1 user(s) forcefully logged out"
                                " from world %2.\n").Arg(usernames.size()).Arg(worldID));
                        }

                        lobbyServer->SendWorldList(nullptr);
                    }, server, svr->GetID());
            }
            else
            {
                LOG_WARNING("Uninitialized world connection closed.\n");
            }

            auto db = server->GetMainDatabase();

            svr->SetStatus(objects::RegisteredWorld::Status_t::INACTIVE);
            svr->Update(db);
        }
        
        iter = std::find(mUnregisteredWorlds.begin(), mUnregisteredWorlds.end(), world);
        if(iter != mUnregisteredWorlds.end())
        {
            mUnregisteredWorlds.erase(iter);
        }
    }
}

const std::shared_ptr<LobbyClientConnection> ManagerConnection::GetClientConnection(
    const libcomp::String& username) const
{
    auto iter = mClientConnections.find(username);
    return iter != mClientConnections.end() ? iter->second : nullptr;
}

const std::list<std::shared_ptr<libcomp::TcpConnection>>
    ManagerConnection::GetClientConnections() const
{
    std::list<std::shared_ptr<libcomp::TcpConnection>> retval;
    auto connectionCopy = mClientConnections;
    for(auto kv : connectionCopy)
    {
        retval.push_back(kv.second);
    }

    return retval;
}

void ManagerConnection::SetClientConnection(const std::shared_ptr<
    LobbyClientConnection>& connection)
{
    auto state = connection->GetClientState();
    auto account = state->GetAccount().GetCurrentReference();
    if(nullptr == account)
    {
        return;
    }

    auto username = account->GetUsername();
    auto iter = mClientConnections.find(username);
    if(iter == mClientConnections.end())
    {
        mClientConnections[username] = connection;
    }
}

void ManagerConnection::RemoveClientConnection(const std::shared_ptr<
    LobbyClientConnection>& connection)
{
    if(nullptr == connection)
    {
        return;
    }

    auto state = connection->GetClientState();
    auto account = state->GetAccount().GetCurrentReference();
    if(nullptr == account)
    {
        return;
    }

    auto username = account->GetUsername();
    auto iter = mClientConnections.find(username);
    if(iter != mClientConnections.end())
    {
        mClientConnections.erase(iter);

        auto server = std::dynamic_pointer_cast<LobbyServer>(mServer.lock());
        auto accountManager = server->GetAccountManager();

        int8_t worldID;
        if(accountManager->IsLoggedIn(username, worldID) && worldID == -1)
        {
            LOG_DEBUG(libcomp::String("Logging out user: '%1'\n").Arg(username));
            accountManager->LogoutUser(username, worldID);
            server->GetSessionManager()->ExpireSession(username);
        }
    }
}
