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

// channel Includes
#include "ChannelConnection.h"
#include "ManagerPacket.h"

// Object Includes
#include "ChannelConfig.h"

using namespace channel;

ChannelServer::ChannelServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath) :
    libcomp::BaseServer(config, configPath)
{
    // Connect to the world server.
    mWorldConnection = std::shared_ptr<libcomp::InternalConnection>(
        new libcomp::InternalConnection(mService));
    mWorldConnection->SetSelf(mWorldConnection);
    mWorldConnection->SetMessageQueue(mMainWorker.GetMessageQueue());

    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(mConfig);

    mWorldConnection->Connect(conf->GetWorldIP(), conf->GetWorldPort(), false);

    bool connected = libcomp::TcpConnection::STATUS_CONNECTED ==
        mWorldConnection->GetStatus();

    if(!connected)
    {
        LOG_CRITICAL("Failed to connect to the world server!\n");
    }

    // Add the managers to the worker.
    mWorker.AddManager(std::shared_ptr<libcomp::Manager>(new ManagerPacket()));

    // Start the workers.
    mWorker.Start();
}

ChannelServer::~ChannelServer()
{
    // Make sure the worker threads stop.
    mWorker.Join();
}

void ChannelServer::Shutdown()
{
    BaseServer::Shutdown();

    /// @todo Add more workers.
    mWorker.Shutdown();
}

std::shared_ptr<libcomp::TcpConnection> ChannelServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto connection = std::shared_ptr<libcomp::TcpConnection>(
        new libcomp::ChannelConnection(socket, CopyDiffieHellman(
            GetDiffieHellman())
        )
    );

    // Assign this to the only worker available.
    std::dynamic_pointer_cast<libcomp::ChannelConnection>(
        connection)->SetMessageQueue(mWorker.GetMessageQueue());

    // Make sure this is called after connecting.
    connection->SetSelf(connection);
    connection->ConnectionSuccess();

    return connection;
}
