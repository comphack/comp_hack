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
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include <BaseServer.h>
#include <ConnectionMessage.h>
#include <InternalConnection.h>
#include <Manager.h>

// Boost ASIO Includes
#include <asio.hpp>

namespace world
{

/**
 * Class to handle messages pertaining to connecting to
 * the lobby or channels.
 */
class ManagerConnection : public libcomp::Manager
{
public:
    /**
     * Create a new manager.
     * @param server Pointer to the server that uses this manager
     */
    ManagerConnection(std::weak_ptr<libcomp::BaseServer> server);

    /**
     * Clean up the manager.
     */
    virtual ~ManagerConnection();

    /**
     * Get the different types of messages handled by this manager.
     * @return List of supported message types
     */
    virtual std::list<libcomp::Message::MessageType> GetSupportedTypes() const;

    /**
     * Process a message from the queue.
     * @param pMessage Message to be processed
     * @return true on success, false on failure
     */
    virtual bool ProcessMessage(const libcomp::Message::Message *pMessage);

    /**
     * Get a pointer to the lobby connection.
     * @return Pointer to the lobby connection
     */
    std::shared_ptr<libcomp::InternalConnection> GetLobbyConnection();

    /**
     * Check if the lobby connection is currently active.
     * @return true if connected, false if not connected
     */
    bool LobbyConnected();

    /**
     * Remove a connection and any associated channel
     * when no longer needed.  This should always be a channel
     * connection but should be called regardless.
     * @param connection Pointer to an internal connection
     */
    void RemoveConnection(std::shared_ptr<libcomp::InternalConnection>& connection);

private:
    /// Static list of supported message types for the manager.
    static std::list<libcomp::Message::MessageType> sSupportedTypes;

    /// Pointer to the server that uses this manager.
    std::weak_ptr<libcomp::BaseServer> mServer;

    /// Pointer to the lobby connection after connecting.
    std::shared_ptr<libcomp::InternalConnection> mLobbyConnection;
};

} // namespace world

#endif // SERVER_WORLD_SRC_MANAGERCONNECTION_H
