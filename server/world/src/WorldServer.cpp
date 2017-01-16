/**
 * @file server/world/src/WorldServer.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief World server class.
 *
 * This file is part of the World Server (world).
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

#include "WorldServer.h"

// libcomp Includes
#include <DatabaseConfigCassandra.h>
#include <DatabaseConfigSQLite3.h>
#include <InternalConnection.h>
#include <LobbyConnection.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <MessageWorldNotification.h>
#include <PacketCodes.h>

// Object Includes
#include "Packets.h"
#include "WorldConfig.h"

using namespace world;

WorldServer::WorldServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath) :
    libcomp::BaseServer(config, configPath)
{
}

bool WorldServer::Initialize(std::weak_ptr<BaseServer>& self)
{
    if(!BaseServer::Initialize(self))
    {
        return false;
    }

    auto conf = std::dynamic_pointer_cast<objects::WorldConfig>(mConfig);
    
    libcomp::EnumMap<objects::ServerConfig::DatabaseType_t,
        std::shared_ptr<objects::DatabaseConfig>> configMap;

    configMap[objects::ServerConfig::DatabaseType_t::SQLITE3]
        = conf->GetSQLite3Config().Get();

    configMap[objects::ServerConfig::DatabaseType_t::CASSANDRA]
        = conf->GetCassandraConfig().Get();

    mDatabase = GetDatabase(configMap, true);

    if(nullptr == mDatabase)
    {
        return false;
    }

    return true;
}

void WorldServer::FinishInitialize()
{
    auto conf = std::dynamic_pointer_cast<objects::WorldConfig>(mConfig);

    asio::io_service service;

    // Connect to the world server.
    std::shared_ptr<libcomp::LobbyConnection> lobbyConnection(
        new libcomp::LobbyConnection(service,
            libcomp::LobbyConnection::ConnectionMode_t::MODE_WORLD_UP));

    std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>>
        messageQueue(new libcomp::MessageQueue<libcomp::Message::Message*>());

    lobbyConnection->SetSelf(lobbyConnection);
    lobbyConnection->SetMessageQueue(messageQueue);

    lobbyConnection->Connect(conf->GetLobbyIP(), conf->GetLobbyPort(), false);

    std::thread serviceThread([&service]()
    {
        service.run();
    });

    bool connected = libcomp::TcpConnection::STATUS_CONNECTED ==
        lobbyConnection->GetStatus();

    if(!connected)
    {
        LOG_CRITICAL("Failed to connect to the lobby server!\n");

        return;
    }

    libcomp::Message::Message *pMessage = messageQueue->Dequeue();

    if(nullptr == dynamic_cast<libcomp::Message::WorldNotification*>(pMessage))
    {
        LOG_CRITICAL("Lobby server did not accept the world server "
            "notification.\n");

        return;
    }

    delete pMessage;

    mManagerConnection = std::shared_ptr<ManagerConnection>(new ManagerConnection(mSelf));

    lobbyConnection->Close();
    serviceThread.join();
    lobbyConnection.reset();
    messageQueue.reset();

    auto connectionManager = std::dynamic_pointer_cast<libcomp::Manager>(mManagerConnection);

    auto packetManager = std::shared_ptr<libcomp::ManagerPacket>(new libcomp::ManagerPacket(mSelf));
    packetManager->AddParser<Parsers::GetWorldInfo>(to_underlying(
        InternalPacketCode_t::PACKET_GET_WORLD_INFO));
    packetManager->AddParser<Parsers::SetChannelInfo>(to_underlying(
        InternalPacketCode_t::PACKET_SET_CHANNEL_INFO));
    packetManager->AddParser<Parsers::AccountLogin>(to_underlying(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGIN));
    packetManager->AddParser<Parsers::AccountLogout>(to_underlying(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT));

    //Add the managers to the main worker.
    mMainWorker.AddManager(packetManager);
    mMainWorker.AddManager(connectionManager);

    // Add the managers to the generic workers.
    for(auto worker : mWorkers)
    {
        worker->AddManager(packetManager);
        worker->AddManager(connectionManager);
    }
}

WorldServer::~WorldServer()
{
}

const std::shared_ptr<objects::RegisteredWorld> WorldServer::GetRegisteredWorld() const
{
    return mRegisteredWorld;
}

std::shared_ptr<objects::RegisteredChannel> WorldServer::GetChannel(
    const std::shared_ptr<libcomp::InternalConnection>& connection) const
{
    auto iter = mRegisteredChannels.find(connection);
    if(iter != mRegisteredChannels.end())
    {
        return iter->second;
    }

    return nullptr;
}

uint8_t WorldServer::GetNextChannelID() const
{
    std::set<uint8_t> registeredIDs;
    for(auto pair : mRegisteredChannels)
    {
        registeredIDs.insert(pair.second->GetID());
    }

    //If an ID drops, fill in the gap
    for(uint8_t i = 0; i < (uint8_t)mRegisteredChannels.size(); i++)
    {
        if(registeredIDs.find(i) == registeredIDs.end())
        {
            return i;
        }
    }

    //If there are no breaks in the numbering sequence, return max + 1 (or 0)
    return static_cast<uint8_t>(mRegisteredChannels.size());
}

std::shared_ptr<objects::RegisteredChannel> WorldServer::GetLoginChannel() const
{
    /// @todo: fix this once channels are registered with public/private zones
    return mRegisteredChannels.size() > 0 ? mRegisteredChannels.begin()->second : nullptr;
}

const std::shared_ptr<libcomp::InternalConnection> WorldServer::GetLobbyConnection() const
{
    return mManagerConnection->GetLobbyConnection();
}

void WorldServer::RegisterChannel(const std::shared_ptr<objects::RegisteredChannel>& channel,
    const std::shared_ptr<libcomp::InternalConnection>& connection)
{
    mRegisteredChannels[connection] = channel;
}

bool WorldServer::RemoveChannel(const std::shared_ptr<libcomp::InternalConnection>& connection)
{
    auto iter = mRegisteredChannels.find(connection);
    if(iter != mRegisteredChannels.end())
    {
        mRegisteredChannels.erase(iter);
        return true;
    }

    return false;
}

std::shared_ptr<libcomp::Database> WorldServer::GetWorldDatabase() const
{
    return mDatabase;
}
   
std::shared_ptr<libcomp::Database> WorldServer::GetLobbyDatabase() const
{
    return mLobbyDatabase;
}

void WorldServer::SetLobbyDatabase(const std::shared_ptr<libcomp::Database>& database)
{
    mLobbyDatabase = database;
}

bool WorldServer::RegisterServer()
{
    if(nullptr == mLobbyDatabase)
    {
        return false;
    }

    //Delete all the channels currently registered
    auto channelServers = libcomp::PersistentObject::LoadAll<
        objects::RegisteredChannel>(mDatabase);

    if(channelServers.size() > 0)
    {
        LOG_DEBUG("Clearing the registered channels from the previous execution.\n");
        auto objs = libcomp::PersistentObject::ToList<objects::RegisteredChannel>(
            channelServers);
        if(!mDatabase->DeleteObjects(objs))
        {
            LOG_CRITICAL("Registered channel deletion failed.\n");
            return false;
        }
    }

    auto conf = std::dynamic_pointer_cast<objects::WorldConfig>(mConfig);

    auto registeredWorld = objects::RegisteredWorld::LoadRegisteredWorldByID(
        mLobbyDatabase, conf->GetID());

    if(nullptr == registeredWorld)
    {
        auto name = conf->GetName().IsEmpty() ? libcomp::String("World %1").Arg(conf->GetID())
            : conf->GetName();
        registeredWorld = std::shared_ptr<objects::RegisteredWorld>(new objects::RegisteredWorld);
        registeredWorld->SetID(conf->GetID());
        registeredWorld->SetName(name);
        registeredWorld->SetStatus(objects::RegisteredWorld::Status_t::ACTIVE);
        if(!registeredWorld->Register(registeredWorld) || !registeredWorld->Insert(mLobbyDatabase))
        {
            return false;
        }
    }
    else if(registeredWorld->GetStatus() == objects::RegisteredWorld::Status_t::ACTIVE)
    {
        //Some other server already connected as this ID, let it fail
        return false;
    }
    else
    {
        auto name = conf->GetName();
        if(!name.IsEmpty())
        {
            registeredWorld->SetName(name);
        }
        registeredWorld->SetStatus(objects::RegisteredWorld::Status_t::ACTIVE);
        if(!registeredWorld->Update(mLobbyDatabase))
        {
            return false;
        }
    }

    mRegisteredWorld = registeredWorld;

    return true;
}

AccountManager* WorldServer::GetAccountManager()
{
    return &mAccountManager;
}

std::shared_ptr<libcomp::TcpConnection> WorldServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto connection = std::shared_ptr<libcomp::TcpConnection>(
        new libcomp::InternalConnection(socket, CopyDiffieHellman(
            GetDiffieHellman())
        )
    );

    // Make sure this is called after connecting.
    connection->SetSelf(connection);

    if(!mManagerConnection->LobbyConnected())
    {
        // Assign this to the main worker.
        std::dynamic_pointer_cast<libcomp::InternalConnection>(
            connection)->SetMessageQueue(mMainWorker.GetMessageQueue());

        connection->ConnectionSuccess();
    }
    else if(true)  /// @todo: ensure that channels can start connecting
    {
        auto encrypted = std::dynamic_pointer_cast<
            libcomp::EncryptedConnection>(connection);
        if(AssignMessageQueue(encrypted))
        {
            connection->ConnectionSuccess();
        }
        else
        {
            connection->Close();
            return nullptr;
        }
    }
    else
    {
        /// @todo: send a "connection refused" error message to the client
        connection->Close();
        return nullptr;
    }

    return connection;
}
