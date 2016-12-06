/**
 * @file server/lobby/src/ManagerConnection.h
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Manager to handle lobby connections to world servers.
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

#ifndef SERVER_LOBBY_SRC_MANAGERCONNECTION_H
#define SERVER_LOBBY_SRC_MANAGERCONNECTION_H

// libcomp Includes
#include "Manager.h"
#include "World.h"

// Boost ASIO Includes
#include <asio.hpp>

namespace lobby
{

class ManagerConnection : public libcomp::Manager
{
public:
    ManagerConnection(std::shared_ptr<asio::io_service> service,
        std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>> messageQueue);
    virtual ~ManagerConnection();

    /**
     * @brief Get the different types of messages handles by this manager.
     */
    virtual std::list<libcomp::Message::MessageType> GetSupportedTypes() const;

    /**
     * Process a message from the queue.
     */
    virtual bool ProcessMessage(const libcomp::Message::Message *pMessage);

    std::list<std::shared_ptr<lobby::World>> GetWorlds();

    std::shared_ptr<lobby::World> GetWorldByConnection(std::shared_ptr<libcomp::InternalConnection> connection);

private:
    std::list<std::shared_ptr<lobby::World>> mWorlds;

    std::shared_ptr<asio::io_service> mService;
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mMessageQueue;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_MANAGERCONNECTION_H
