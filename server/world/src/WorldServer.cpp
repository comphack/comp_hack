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
#include <InternalConnection.h>
#include <LobbyConnection.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <MessageWorldNotification.h>

// Object Includes
#include "WorldConfig.h"

using namespace world;

WorldServer::WorldServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath) :
    libcomp::BaseServer(config, configPath)
{
    asio::io_service service;

    // Connect to the world server.
    std::shared_ptr<libcomp::LobbyConnection> lobbyConnection(
        new libcomp::LobbyConnection(service,
            libcomp::LobbyConnection::ConnectionMode_t::MODE_WORLD_UP));

    std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>>
        messageQueue(new libcomp::MessageQueue<libcomp::Message::Message*>());

    lobbyConnection->SetSelf(lobbyConnection);
    lobbyConnection->SetMessageQueue(messageQueue);

    auto conf = std::dynamic_pointer_cast<objects::WorldConfig>(mConfig);
    mDescription.SetID(conf->GetID());
    mDescription.SetName(conf->GetName());

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
    }

    libcomp::Message::Message *pMessage = messageQueue->Dequeue();

    if(nullptr == dynamic_cast<libcomp::Message::WorldNotification*>(pMessage))
    {
        LOG_CRITICAL("Lobby server did not accept the world server "
            "notification.\n");
    }

    delete pMessage;

    mManagerConnection = std::shared_ptr<ManagerConnection>(new ManagerConnection());

    lobbyConnection->Close();
    serviceThread.join();
    lobbyConnection.reset();
    messageQueue.reset();

    auto connectionManager = std::shared_ptr<libcomp::Manager>(mManagerConnection);
    auto packetManager = std::shared_ptr<libcomp::Manager>(new ManagerPacket(std::shared_ptr<libcomp::BaseServer>(this)));

    //Add the managers to the main worker.
    mMainWorker.AddManager(packetManager);
    mMainWorker.AddManager(connectionManager);

    // Add the managers to the generic workers.
    mWorker.AddManager(packetManager);
    mWorker.AddManager(connectionManager);

    // Start the worker.
    mWorker.Start();
}

WorldServer::~WorldServer()
{
}

objects::WorldDescription WorldServer::GetDescription()
{
    return mDescription;
}

bool WorldServer::GetChannelDescriptionByConnection(std::shared_ptr<libcomp::InternalConnection> connection, objects::ChannelDescription& outChannel)
{
    if (mChannelDescriptions.find(connection) != mChannelDescriptions.end())
    {
        outChannel = mChannelDescriptions[connection];
        return true;
    }

    return false;
}

std::shared_ptr<libcomp::InternalConnection> WorldServer::GetLobbyConnection()
{
    return mManagerConnection->GetLobbyConnection();
}

void WorldServer::SetChannelDescription(objects::ChannelDescription channel, std::shared_ptr<libcomp::InternalConnection> connection)
{
    mChannelDescriptions[connection] = channel;
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
    else if(true)  //todo: ensure that channels can start connecting
    {
        // Assign this to the only worker available.
        std::dynamic_pointer_cast<libcomp::InternalConnection>(
            connection)->SetMessageQueue(mWorker.GetMessageQueue());

        connection->ConnectionSuccess();
    }
    else
    {
        //todo: print an error message
        connection->Close();
    }

    return connection;
}
