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

// lobby Includes
#include "LobbyClientConnection.h"

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
     * Request information about the world and send database
     * connection information to the main DB.
     * @param world World to intialize
     * @return true on success, false on failure
     */
    bool InitializeWorld(const std::shared_ptr<lobby::World>& world);

    /**
     * Get a list of registered worlds.
     * @return List of pointers to registered worlds
     */
    std::list<std::shared_ptr<lobby::World>> GetWorlds() const;

    /**
     * Get a registered world via its ID.
     * @param id The world's ID
     * @return Pointer to a registered world
     */
    std::shared_ptr<lobby::World> GetWorldByID(uint8_t id) const;

    /**
     * Get a registered world via its connection.
     * @param connection Pointer to the world's connection
     * @return Pointer to a registered world
     */
    std::shared_ptr<lobby::World> GetWorldByConnection(
        const std::shared_ptr<libcomp::InternalConnection>& connection) const;

    /**
     * Register a world and remove previous entries.  This will
     * fail if the world does not contain registered server information.
     * @param world Pointer to the world to register
     * @return Pointer to the new registered server or nullptr
     *  on failure
     */
    const std::shared_ptr<lobby::World> RegisterWorld(
        std::shared_ptr<lobby::World>& world);

    /**
     * Remove a registered world when no longer needed.
     * @param world Pointer to the world to remove
     */
    void RemoveWorld(std::shared_ptr<lobby::World>& world);

    /**
     * Get client connection by username.
     * @param username Client account username
     * @return Pointer to the client connection
     */
    const std::shared_ptr<LobbyClientConnection> GetClientConnection(
        const libcomp::String& username) const;

    /**
     * Get all client connections.
     * @return List of pointers to all client connections
     */
    const std::list<std::shared_ptr<libcomp::TcpConnection>>
        GetClientConnections() const;

    /**
     * Set an active client connection after its account has
     * been detected.
     * @param connection Pointer to the client connection
     */
    void SetClientConnection(const std::shared_ptr<
        LobbyClientConnection>& connection);

    /**
     * Remove a client connection.
     * @param connection Pointer to the client connection
     */
    void RemoveClientConnection(const std::shared_ptr<
        LobbyClientConnection>& connection);

private:
    /// Static list of supported message types for the manager
    static std::list<libcomp::Message::MessageType> sSupportedTypes;

    /// Pointer to the server that uses this manager
    std::weak_ptr<libcomp::BaseServer> mServer;

    /// List of pointers to registered worlds
    std::list<std::shared_ptr<lobby::World>> mWorlds;

    /// List of pointers to unregistered worlds
    std::list<std::shared_ptr<lobby::World>> mUnregisteredWorlds;

    /// Map of active client connections by account username
    std::unordered_map<libcomp::String,
        std::shared_ptr<LobbyClientConnection>> mClientConnections;

    /// Pointer to the ASIO service to use to establish connections
    /// to world servers
    asio::io_service *mService;

    /// Pointer to the message queue to use when connecting to world
    /// servers
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mMessageQueue;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_MANAGERCONNECTION_H
