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
    auto mWorldConnection = std::shared_ptr<libcomp::InternalConnection>(
        new libcomp::InternalConnection(mService));
    mWorldConnection->SetSelf(mWorldConnection);
    mWorldConnection->SetMessageQueue(mMainWorker.GetMessageQueue());

    mManagerConnection = std::shared_ptr<ManagerConnection>(new ManagerConnection(std::shared_ptr<libcomp::BaseServer>(this)));
    mManagerConnection->SetWorldConnection(mWorldConnection);

    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(mConfig);
    mDescription.SetID(conf->GetID());
    mDescription.SetName(conf->GetName());

    mWorldConnection->Connect(conf->GetWorldIP(), conf->GetWorldPort(), false);

    bool connected = libcomp::TcpConnection::STATUS_CONNECTED ==
        mWorldConnection->GetStatus();

    if(!connected)
    {
        LOG_CRITICAL("Failed to connect to the world server!\n");
    }

    auto packetManager = std::shared_ptr<libcomp::Manager>(new ManagerPacket(std::shared_ptr<libcomp::BaseServer>(this)));

    //Add the managers to the main worker.
    mMainWorker.AddManager(packetManager);
    mMainWorker.AddManager(mManagerConnection);

    // Add the managers to the generic workers.
    mWorker.AddManager(packetManager);

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

objects::ChannelDescription ChannelServer::GetDescription()
{
    return mDescription;
}

objects::WorldDescription ChannelServer::GetWorldDescription()
{
    return mWorldDescription;
}

void ChannelServer::SetWorldDescription(objects::WorldDescription& worldDescription)
{
    mWorldDescription = worldDescription;
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
