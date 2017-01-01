/**
 * @file server/lobby/src/World.h
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief World definition in regards to the lobby containing an active connection to the world server.
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

#ifndef SERVER_LOBBY_SRC_WORLD_H
#define SERVER_LOBBY_SRC_WORLD_H

// libcomp Includes
#include <Database.h>
#include <InternalConnection.h>

// object Includes
#include <RegisteredServer.h>

namespace lobby
{

/**
 * Associates a world connection to @ref RegisteredServer of the world
 * and its channels for the lobby.
 */
class World
{
public:
    /**
     * Create a new world from a world connection.
     */
    World();

    /**
     * Clean up the world.
     */
    virtual ~World();

    /**
     * Get a pointer to the world's connection.
     * @return Pointer to the world's connection
     */
    std::shared_ptr<libcomp::InternalConnection> GetConnection() const;

    /**
     * Set the pointer to the world's connection.
     * @param connection Pointer to the world's connection
     */
    void SetConnection(const std::shared_ptr<libcomp::InternalConnection>& connection);

    /**
     * Get a list of pointers to the world's channel RegisteredServers.
     * @return List of pointers to the world's channel RegisteredServers
     */
    const std::list<std::shared_ptr<objects::RegisteredServer>> GetChannels() const;

    /**
     * Get a pointer to a channel RegisteredServer by its ID.
     * @param id ID of a channel RegisteredServer to the world
     * @return Pointer to the matching channel RegisteredServer
     */
    std::shared_ptr<objects::RegisteredServer> GetChannelByID(uint8_t id) const;

    /**
     * Remove a channel RegisteredServer by its ID.
     * @param id ID of a channel associated to the world
     * @return true if the RegisteredServer existed, false if it did not exist
     */
    bool RemoveChannelByID(uint8_t id);

    /**
     * Get the world database.
     * @return Pointer to the world's database
     */
    std::shared_ptr<libcomp::Database> GetWorldDatabase() const;

    /**
     * Set the world database.
     * @param database Pointer to the world's database
     */
    void SetWorldDatabase(const std::shared_ptr<libcomp::Database>& database);

    /**
     * Set a channel RegisteredServer.
     * @param channel Pointer to a channel's RegisteredServer
     */
    void RegisterChannel(const std::shared_ptr<objects::RegisteredServer>& channel);
    
    /**
     * Get the registered server.
     * @return Pointer to the RegisteredServer
     */
    const std::shared_ptr<objects::RegisteredServer> GetRegisteredServer() const;

    /**
     * Set the registered server.
     * @param registeredServer Pointer to the RegisteredServer
     */
    void SetRegisteredServer(const std::shared_ptr<objects::RegisteredServer>& registeredServer);

private:
    /// Pointer to the world's connection
    std::shared_ptr<libcomp::InternalConnection> mConnection;

    /// Pointer to the RegisteredServer
    std::shared_ptr<objects::RegisteredServer> mRegisteredServer;

    /// A shared pointer to the world database used by the server
    std::shared_ptr<libcomp::Database> mDatabase;

    /// List of pointers to the channel RegisteredServers
    std::list<std::shared_ptr<objects::RegisteredServer>> mChannels;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_WORLD_H
