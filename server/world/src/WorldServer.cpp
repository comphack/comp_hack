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
#include <DatabaseConfigMariaDB.h>
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

WorldServer::WorldServer(const char *szProgram,
    std::shared_ptr<objects::ServerConfig> config,
    std::shared_ptr<libcomp::ServerCommandLineParser> commandLine) :
    libcomp::BaseServer(szProgram, config, commandLine)
{
}

bool WorldServer::Initialize()
{
    auto self = std::dynamic_pointer_cast<WorldServer>(shared_from_this());

    if(!BaseServer::Initialize())
    {
        return false;
    }

    auto conf = std::dynamic_pointer_cast<objects::WorldConfig>(mConfig);

    libcomp::EnumMap<objects::ServerConfig::DatabaseType_t,
        std::shared_ptr<objects::DatabaseConfig>> configMap;

    configMap[objects::ServerConfig::DatabaseType_t::SQLITE3]
        = conf->GetSQLite3Config();

    configMap[objects::ServerConfig::DatabaseType_t::MARIADB]
        = conf->GetMariaDBConfig();

    mDatabase = GetDatabase(configMap, true);

    if(nullptr == mDatabase)
    {
        return false;
    }

    mCharacterManager = new CharacterManager(self);

    return true;
}

void WorldServer::FinishInitialize()
{
    auto conf = std::dynamic_pointer_cast<objects::WorldConfig>(mConfig);

    mManagerConnection = std::make_shared<ManagerConnection>(
        shared_from_this());

    auto connectionManager = std::dynamic_pointer_cast<libcomp::Manager>(mManagerConnection);

    // Build the lobby manager
    auto packetManager = std::make_shared<libcomp::ManagerPacket>(
        shared_from_this());
    packetManager->AddParser<Parsers::GetWorldInfo>(to_underlying(
        InternalPacketCode_t::PACKET_GET_WORLD_INFO));
    packetManager->AddParser<Parsers::SetChannelInfo>(to_underlying(
        InternalPacketCode_t::PACKET_SET_CHANNEL_INFO));
    packetManager->AddParser<Parsers::AccountLogin>(to_underlying(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGIN));
    packetManager->AddParser<Parsers::AccountLogout>(to_underlying(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT));

    // Add the managers to the main worker.
    mMainWorker.AddManager(packetManager);
    mMainWorker.AddManager(connectionManager);

    // Build the channel manager
    packetManager = std::make_shared<libcomp::ManagerPacket>(
        shared_from_this());
    packetManager->AddParser<Parsers::GetWorldInfo>(to_underlying(
        InternalPacketCode_t::PACKET_GET_WORLD_INFO));
    packetManager->AddParser<Parsers::SetChannelInfo>(to_underlying(
        InternalPacketCode_t::PACKET_SET_CHANNEL_INFO));
    packetManager->AddParser<Parsers::AccountLogin>(to_underlying(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGIN));
    packetManager->AddParser<Parsers::AccountLogout>(to_underlying(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT));
    packetManager->AddParser<Parsers::Relay>(to_underlying(
        InternalPacketCode_t::PACKET_RELAY));
    packetManager->AddParser<Parsers::CharacterLogin>(to_underlying(
        InternalPacketCode_t::PACKET_CHARACTER_LOGIN));
    packetManager->AddParser<Parsers::FriendsUpdate>(to_underlying(
        InternalPacketCode_t::PACKET_FRIENDS_UPDATE));
    packetManager->AddParser<Parsers::PartyUpdate>(to_underlying(
        InternalPacketCode_t::PACKET_PARTY_UPDATE));
    packetManager->AddParser<Parsers::ClanUpdate>(to_underlying(
        InternalPacketCode_t::PACKET_CLAN_UPDATE));

    // Add the managers to the generic workers.
    for(auto worker : mWorkers)
    {
        worker->AddManager(packetManager);
        worker->AddManager(connectionManager);
    }

    // Now Connect to the lobby server.
    asio::io_service service;

    auto lobbyConnection = std::make_shared<libcomp::LobbyConnection>(service,
            libcomp::LobbyConnection::ConnectionMode_t::MODE_WORLD_UP);

    auto messageQueue = std::make_shared<libcomp::MessageQueue<
        libcomp::Message::Message*>>();

    lobbyConnection->SetMessageQueue(messageQueue);
    lobbyConnection->SetName("lobby_notify");

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

    lobbyConnection->Close();
    serviceThread.join();
    lobbyConnection.reset();
    messageQueue.reset();
}

WorldServer::~WorldServer()
{
    delete mCharacterManager;
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

std::shared_ptr<libcomp::InternalConnection> WorldServer::GetChannelConnectionByID(
    int8_t channelID) const
{
    for(auto cPair : mRegisteredChannels)
    {
        if(cPair.second->GetID() == channelID)
        {
            return cPair.first;
        }
    }

    return nullptr;
}

std::map<std::shared_ptr<libcomp::InternalConnection>,
    std::shared_ptr<objects::RegisteredChannel>> WorldServer::GetChannels() const
{
    return mRegisteredChannels;
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

CharacterManager* WorldServer::GetCharacterManager()
{
    return mCharacterManager;
}

uint32_t WorldServer::GetRelayPacket(libcomp::Packet& p,
    const std::list<int32_t>& targetCIDs, int32_t sourceCID)
{
    p.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
    p.WriteS32Little(sourceCID);

    p.WriteU8((uint8_t)PacketRelayMode_t::RELAY_CIDS);
    if(targetCIDs.size() > 0)
    {
        p.WriteU16Little((uint16_t)targetCIDs.size());
        for(auto targetCID : targetCIDs)
        {
            p.WriteS32Little(targetCID);
        }
    }

    // Return the target ID packet offset
    return 5;
}

void WorldServer::GetRelayPacket(libcomp::Packet& p, int32_t targetCID,
    int32_t sourceCID)
{
    std::list<int32_t> cids = { targetCID };
    GetRelayPacket(p, cids, sourceCID);
}

std::shared_ptr<libcomp::TcpConnection> WorldServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    static int connectionID = 0;

    auto connection = std::make_shared<libcomp::InternalConnection>(
        socket, CopyDiffieHellman(GetDiffieHellman()));

    if(!mManagerConnection->LobbyConnected())
    {
        // Assign this to the main worker.
        connection->SetMessageQueue(mMainWorker.GetMessageQueue());
        connection->ConnectionSuccess();
        connection->SetName(libcomp::String("%1:lobby").Arg(connectionID++));
    }
    else if(true)  /// @todo: ensure that channels can start connecting
    {
        connection->SetName(libcomp::String("%1:channel").Arg(
            connectionID++));

        if(AssignMessageQueue(connection))
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
