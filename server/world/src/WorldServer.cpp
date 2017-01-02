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
    libcomp::BaseServer(config, configPath), mDescription(new objects::WorldDescription)
{
}

bool WorldServer::Initialize(std::weak_ptr<BaseServer>& self)
{
    if(!BaseServer::Initialize(self))
    {
        return false;
    }

    auto conf = std::dynamic_pointer_cast<objects::WorldConfig>(mConfig);
    mDescription->SetID(conf->GetID());
    mDescription->SetName(conf->GetName());
    
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

const std::shared_ptr<objects::WorldDescription> WorldServer::GetDescription() const
{
    return mDescription;
}

std::shared_ptr<objects::ChannelDescription> WorldServer::GetChannelDescriptionByConnection(
    const std::shared_ptr<libcomp::InternalConnection>& connection) const
{
    auto iter = mChannelDescriptions.find(connection);
    if(iter != mChannelDescriptions.end())
    {
        return iter->second;
    }

    return nullptr;
}

const std::shared_ptr<libcomp::InternalConnection> WorldServer::GetLobbyConnection() const
{
    return mManagerConnection->GetLobbyConnection();
}

void WorldServer::SetChannelDescription(const std::shared_ptr<objects::ChannelDescription>& channel,
    const std::shared_ptr<libcomp::InternalConnection>& connection)
{
    mChannelDescriptions[connection] = channel;
}

bool WorldServer::RemoveChannelDescription(const std::shared_ptr<libcomp::InternalConnection>& connection)
{
    auto iter = mChannelDescriptions.find(connection);
    if(iter != mChannelDescriptions.end())
    {
        mChannelDescriptions.erase(iter);
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
