/**
 * @file server/lobby/src/LobbyServer.h
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

#ifndef SERVER_LOBBY_SRC_LOBBYSERVER_H
#define SERVER_LOBBY_SRC_LOBBYSERVER_H

// libcomp Includes
#include <BaseServer.h>
#include <Database.h>
#include <Worker.h>

// channel Includes
#include "LobbyConfig.h"

namespace lobby
{

class LobbyServer : public libcomp::BaseServer
{
public:
    LobbyServer(std::shared_ptr<objects::LobbyConfig> config, const libcomp::String& configPath);
    virtual ~LobbyServer();

    virtual void Shutdown();

protected:
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    std::shared_ptr<objects::LobbyConfig> mConfig;

    /// @todo Replace this with many worker threads.
    libcomp::Worker mWorker;

    std::shared_ptr<libcomp::Database> mDatabase;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_LOBBYSERVER_H
