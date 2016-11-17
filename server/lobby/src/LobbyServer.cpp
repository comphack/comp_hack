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

// libcomp Includes
#include <DatabaseCassandra.h>
#include <Log.h>
#include <ManagerConnection.h>
#include <ManagerPacket.h>

// Object Includes
#include "LobbyConfig.h"

using namespace lobby;

LobbyServer::LobbyServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath) :
    libcomp::BaseServer(config, configPath)
{
    /// @todo Setup the database type based on the config.
    /// @todo Consider moving this into the base server.
    mDatabase = std::shared_ptr<libcomp::Database>(
        new libcomp::DatabaseCassandra);

    auto conf = std::dynamic_pointer_cast<objects::LobbyConfig>(mConfig);

    // Open the database.
    if(!mDatabase->Open(conf->GetDatabaseIP()) || !mDatabase->IsOpen())
    {
        LOG_CRITICAL("Failed to open database.\n");

        return;
    }

    // Setup the database.
    /// @todo Only if the database does not exist and call Use() instead!
    if(!mDatabase->Setup())
    {
        LOG_CRITICAL("Failed to init database.\n");

        return;
    }

    auto connectionManager = std::shared_ptr<libcomp::Manager>(new ManagerConnection(std::shared_ptr<asio::io_service>(&mService), mMainWorker.GetMessageQueue()));

    //Add the managers to the main worker.
    mMainWorker.AddManager(connectionManager);

    // Add the managers to the generic workers.
    mWorker.AddManager(std::shared_ptr<libcomp::Manager>(new ManagerPacket()));
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
