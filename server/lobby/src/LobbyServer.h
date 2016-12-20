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
#include <Worker.h>

// lobby Includes
#include "ManagerConnection.h"
#include "World.h"

namespace lobby
{

class LobbyServer : public libcomp::BaseServer
{
public:
    LobbyServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath);
    virtual ~LobbyServer();

    virtual bool Initialize(std::weak_ptr<BaseServer>& self);

    virtual void Shutdown();

    std::list<std::shared_ptr<lobby::World>> GetWorlds();

    std::shared_ptr<lobby::World> GetWorldByConnection(std::shared_ptr<libcomp::InternalConnection> connection);

protected:
    void CreateFirstAccount();
    void PromptCreateAccount();

    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    /// @todo Replace this with many worker threads.
    libcomp::Worker mWorker;

    std::shared_ptr<ManagerConnection> mManagerConnection;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_LOBBYSERVER_H
