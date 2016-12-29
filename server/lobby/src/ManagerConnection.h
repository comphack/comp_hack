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
#include <BaseServer.h>
#include <Manager.h>
#include <World.h>

// Boost ASIO Includes
#include <asio.hpp>

namespace lobby
{
    
/**
 * Class to handle messages pertaining to connecting to
 * worlds or game clients.
 */
class ManagerConnection : public libcomp::Manager
{
public:
    /**
     * Create a new manager.
     * @param server Pointer to the server that uses this manager
     * @param service Pointer to the ASIO service to use to establish
     *  connections to world servers
     * @param messageQueue Pointer to the message queue to use
     *  when connecting to world servers
     */
    ManagerConnection(std::weak_ptr<libcomp::BaseServer> server,
        asio::io_service* service,
        std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>> messageQueue);
    
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
     * Get a list of connected worlds.
     * @return List of pointers to connected worlds
     */
    std::list<std::shared_ptr<lobby::World>> GetWorlds() const;
    
    /**
     * Get a connected world via its connection.
     * @param connection Pointer to the world's connection
     * @return Pointer to a connected world
     */
    std::shared_ptr<lobby::World> GetWorldByConnection(
        const std::shared_ptr<libcomp::InternalConnection>& connection) const;
    
    /**
     * Remove a connected world when no longer needed.
     * @param world Pointer to the world to remove
     */
    void RemoveWorld(std::shared_ptr<lobby::World>& world);

private:
    /// Static list of supported message types for the manager.
    static std::list<libcomp::Message::MessageType> sSupportedTypes;

    /// Pointer to the server that uses this manager.
    std::weak_ptr<libcomp::BaseServer> mServer;

    /// Pointer to worlds after connecting.
    std::list<std::shared_ptr<lobby::World>> mWorlds;

    /// Pointer to the ASIO service to use to establish connections
    /// to world servers.
    asio::io_service *mService;

    /// Pointer to the message queue to use when connecting to world
    /// servers.
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mMessageQueue;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_MANAGERCONNECTION_H
