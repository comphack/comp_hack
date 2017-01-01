/**
 * @file server/channel/src/ChannelServer.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Channel server class.
 *
 * This file is part of the Channel Server (channel).
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

#include "ChannelServer.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelConnection.h"
#include "Packets.h"

// Object Includes
#include "ChannelConfig.h"

using namespace channel;

ChannelServer::ChannelServer(std::shared_ptr<objects::ServerConfig> config,
    const libcomp::String& configPath) : libcomp::BaseServer(config, configPath)
{
}

bool ChannelServer::Initialize(std::weak_ptr<BaseServer>& self)
{
    if(!BaseServer::Initialize(self))
    {
        return false;
    }

    // Connect to the world server.
    auto worldConnection = std::shared_ptr<libcomp::InternalConnection>(
        new libcomp::InternalConnection(mService));
    worldConnection->SetSelf(worldConnection);
    worldConnection->SetMessageQueue(mMainWorker.GetMessageQueue());

    mManagerConnection = std::shared_ptr<ManagerConnection>(
        new ManagerConnection(self));
    mManagerConnection->SetWorldConnection(worldConnection);

    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(mConfig);

    worldConnection->Connect(conf->GetWorldIP(), conf->GetWorldPort(), false);

    bool connected = libcomp::TcpConnection::STATUS_CONNECTED ==
        worldConnection->GetStatus();

    if(!connected)
    {
        LOG_CRITICAL("Failed to connect to the world server!\n");
        return false;
    }

    auto internalPacketManager = std::shared_ptr<libcomp::ManagerPacket>(
        new libcomp::ManagerPacket(self));
    internalPacketManager->AddParser<Parsers::SetWorldInfo>(
        to_underlying(InternalPacketCode_t::PACKET_SET_WORLD_INFO));

    //Add the managers to the main worker.
    mMainWorker.AddManager(internalPacketManager);
    mMainWorker.AddManager(mManagerConnection);

    auto clientPacketManager = std::shared_ptr<libcomp::ManagerPacket>(
        new libcomp::ManagerPacket(self));
    /// @todo: Add client side packet parsers

    // Add the managers to the generic workers.
    for(auto worker : mWorkers)
    {
        worker->AddManager(clientPacketManager);
    }

    return true;
}

ChannelServer::~ChannelServer()
{
}

const std::shared_ptr<objects::RegisteredServer> ChannelServer::GetRegisteredServer()
{
    return mRegisteredServer;
}

std::shared_ptr<objects::RegisteredServer> ChannelServer::GetWorldRegisteredServer()
{
    return mWorldRegisteredServer;
}

void ChannelServer::SetWorldRegisteredServer(const std::shared_ptr<
    objects::RegisteredServer>& registeredServer)
{
    mWorldRegisteredServer = registeredServer;
}

std::shared_ptr<libcomp::Database> ChannelServer::GetWorldDatabase() const
{
    return mWorldDatabase;
}

void ChannelServer::SetWorldDatabase(const std::shared_ptr<libcomp::Database>& database)
{
    mWorldDatabase = database;
}
   
std::shared_ptr<libcomp::Database> ChannelServer::GetLobbyDatabase() const
{
    return mLobbyDatabase;
}

void ChannelServer::SetLobbyDatabase(const std::shared_ptr<libcomp::Database>& database)
{
    mLobbyDatabase = database;
}

bool ChannelServer::RegisterServer()
{
    if(nullptr == mLobbyDatabase)
    {
        return false;
    }

    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(mConfig);

    auto registeredServer = objects::RegisteredServer::LoadRegisteredServerByParentAndID(
        mLobbyDatabase, mWorldRegisteredServer, conf->GetID());

    if(nullptr == registeredServer)
    {
        auto name = conf->GetName().IsEmpty() ? libcomp::String("Channel %1").Arg(conf->GetID())
            : conf->GetName();
        registeredServer = std::shared_ptr<objects::RegisteredServer>(new objects::RegisteredServer);
        registeredServer->SetID(conf->GetID());
        registeredServer->SetName(name);
        registeredServer->SetParent(mWorldRegisteredServer);
        registeredServer->SetStatus(objects::RegisteredServer::Status_t::ACTIVE);
        registeredServer->SetType(objects::RegisteredServer::Type_t::CHANNEL);
        if(!registeredServer->Register(registeredServer) || !registeredServer->Insert(mLobbyDatabase))
        {
            return false;
        }
    }
    else
    {
        //Some other server already connected as this ID, let it fail
        return false;
    }

    mRegisteredServer = registeredServer;

    return true;
}

std::shared_ptr<libcomp::TcpConnection> ChannelServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto connection = std::shared_ptr<libcomp::TcpConnection>(
        new libcomp::ChannelConnection(socket, CopyDiffieHellman(
            GetDiffieHellman())
        )
    );

    auto encrypted = std::dynamic_pointer_cast<
        libcomp::EncryptedConnection>(connection);
    if(AssignMessageQueue(encrypted))
    {
        // Make sure this is called after connecting.
        connection->SetSelf(connection);
        connection->ConnectionSuccess();
    }
    else
    {
        connection->Close();
        return nullptr;
    }


    return connection;
}
