/**
 * @file server/world/src/ManagerConnection.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Manager to handle world connections to lobby and channel servers.
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

#ifndef SERVER_WORLD_SRC_MANAGERCONNECTION_H
#define SERVER_WORLD_SRC_MANAGERCONNECTION_H

// libcomp Includes
#include "BaseServer.h"
#include "InternalConnection.h"
#include "Manager.h"

// Boost ASIO Includes
#include <asio.hpp>

namespace world
{

class ManagerConnection : public libcomp::Manager
{
public:
    ManagerConnection(std::shared_ptr<libcomp::BaseServer> server);
    virtual ~ManagerConnection();

    /**
     * @brief Get the different types of messages handles by this manager.
     */
    virtual std::list<libcomp::Message::MessageType> GetSupportedTypes() const;

    /**
     * Process a message from the queue.
     */
    virtual bool ProcessMessage(const libcomp::Message::Message *pMessage);

    std::shared_ptr<libcomp::InternalConnection> GetLobbyConnection();

    /**
    * Returns true if the lobby connection is currently active.
    */
    bool LobbyConnected();

private:
    std::shared_ptr<libcomp::BaseServer> mServer;

    std::shared_ptr<libcomp::InternalConnection> mLobbyConnection;
};

} // namespace world

#endif // SERVER_WORLD_SRC_MANAGERCONNECTION_H
