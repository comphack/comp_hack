/**
 * @file server/lobby/src/LobbyServer.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby server class.
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

#include "LobbyServer.h"

// lobby Includes
#include "LobbyConnection.h"
#include "Packets.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>
#include <PacketCodes.h>

// Object Includes
#include "LobbyConfig.h"

using namespace lobby;

LobbyServer::LobbyServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath) :
    libcomp::BaseServer(config, configPath)
{
    auto conf = std::dynamic_pointer_cast<objects::LobbyConfig>(mConfig);

    mManagerConnection = std::shared_ptr<ManagerConnection>(new ManagerConnection(mSelf, std::shared_ptr<asio::io_service>(&mService), mMainWorker.GetMessageQueue()));

    auto connectionManager = std::shared_ptr<libcomp::Manager>(mManagerConnection);

    auto internalPacketManager = std::shared_ptr<libcomp::ManagerPacket>(new libcomp::ManagerPacket(mSelf));
    internalPacketManager->AddParser<Parsers::SetWorldDescription>(PACKET_SET_WORLD_DESCRIPTION);
    internalPacketManager->AddParser<Parsers::SetChannelDescription>(PACKET_SET_CHANNEL_DESCRIPTION);

    //Add the managers to the main worker.
    mMainWorker.AddManager(internalPacketManager);
    mMainWorker.AddManager(connectionManager);

    auto clientPacketManager = std::shared_ptr<libcomp::ManagerPacket>(new libcomp::ManagerPacket(mSelf));
    clientPacketManager->AddParser<Parsers::Login>(PACKET_LOGIN);
    clientPacketManager->AddParser<Parsers::Auth>(PACKET_AUTH);
    clientPacketManager->AddParser<Parsers::StartGame>(PACKET_START_GAME);
    clientPacketManager->AddParser<Parsers::CharacterList>(PACKET_CHARACTER_LIST);
    clientPacketManager->AddParser<Parsers::WorldList>(PACKET_WORLD_LIST);
    clientPacketManager->AddParser<Parsers::CreateCharacter>(PACKET_CREATE_CHARACTER);
    clientPacketManager->AddParser<Parsers::DeleteCharacter>(PACKET_DELETE_CHARACTER);
    clientPacketManager->AddParser<Parsers::QueryPurchaseTicket>(PACKET_QUERY_PURCHASE_TICKET);
    clientPacketManager->AddParser<Parsers::PurchaseTicket>(PACKET_PURCHASE_TICKET);

    // Add the managers to the generic workers.
    mWorker.AddManager(clientPacketManager);
    mWorker.AddManager(connectionManager);

    // Start the worker.
    mWorker.Start();
}

LobbyServer::~LobbyServer()
{
    // Make sure the worker threads stop.
    mWorker.Join();
}

void LobbyServer::Shutdown()
{
    BaseServer::Shutdown();

    /// @todo Add more workers.
    mWorker.Shutdown();
}

std::list<std::shared_ptr<lobby::World>> LobbyServer::GetWorlds()
{
    return mManagerConnection->GetWorlds();
}

std::shared_ptr<lobby::World> LobbyServer::GetWorldByConnection(std::shared_ptr<libcomp::InternalConnection> connection)
{
    return mManagerConnection->GetWorldByConnection(connection);
}

std::shared_ptr<libcomp::TcpConnection> LobbyServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto connection = std::shared_ptr<libcomp::TcpConnection>(
        new libcomp::LobbyConnection(socket, CopyDiffieHellman(
            GetDiffieHellman())
        )
    );

    // Assign this to the only worker available.
    std::dynamic_pointer_cast<libcomp::LobbyConnection>(
        connection)->SetMessageQueue(mWorker.GetMessageQueue());

    // Make sure this is called after connecting.
    connection->SetSelf(connection);
    connection->ConnectionSuccess();

    return connection;
}
