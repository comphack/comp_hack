/**
 * @file server/world/src/WorldServer.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief World server class.
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

#ifndef SERVER_WORLD_SRC_WORLDSERVER_H
#define SERVER_WORLD_SRC_WORLDSERVER_H

 // Standard C++11 Includes
#include <map>

// libcomp Includes
#include <BaseServer.h>
#include <ChannelDescription.h>
#include <InternalConnection.h>
#include <ManagerConnection.h>
#include <Worker.h>

// object Includes
#include <WorldDescription.h>

namespace world
{

class WorldServer : public libcomp::BaseServer
{
public:
    /**
     * Create a new world server.
     * @param config Pointer to a casted WorldConfig that will contain properties
     *   every server has in addition to world specific ones.
     * @param configPath File path to the location of the config to be loaded.
     */
    WorldServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath);

    /**
     * Clean up the server.
     */
    virtual ~WorldServer();

    /**
     * Initialize the database connection and do anything else that can fail
     * to execute that needs to be handled outside of a constructor.  This
     * calls the BaseServer version as well to perform shared init steps.
     * @param self Pointer to this server to be used as a reference in
     *   packet handling code.
     * @return true on success, false on failure
     */
    virtual bool Initialize(std::weak_ptr<BaseServer>& self);

    /**
     * Get the description of the world read from the config.
     * @return Pointer to the WorldDescription
     */
    const std::shared_ptr<objects::WorldDescription> GetDescription() const;

    /**
     * Get the description of a channel currently being connected to
     * by its connection pointer.
     * @param connection Pointer to the channel's connection.
     * @return Pointer to the ChannelDescription
     */
    std::shared_ptr<objects::ChannelDescription> GetChannelDescriptionByConnection(
        const std::shared_ptr<libcomp::InternalConnection>& connection) const;

    /**
     * Get a pointer to the lobby connection.
     * @return Pointer to the lobby connection
     */
    const std::shared_ptr<libcomp::InternalConnection> GetLobbyConnection() const;

    /**
     * Set the description of a channel currently being connected to
     * via a connection.
     * @param channel Pointer to the channel's description.
     * @param connection Pointer to the channel's connection.
     */
    void SetChannelDescription(const std::shared_ptr<objects::ChannelDescription>& channel,
        const std::shared_ptr<libcomp::InternalConnection>& connection);

    /**
     * Remove the description of the channel for a connection
     * that is no longer being used.
     * @param connection Pointer to the channel's connection.
     * @return true if the description existed, false if it did not
     */
    bool RemoveChannelDescription(const std::shared_ptr<libcomp::InternalConnection>& connection);
    
    /**
     * Get the world database.
     * @return Pointer to the world's database
     */
    std::shared_ptr<libcomp::Database> GetWorldDatabase() const;
    
    /**
     * Get the lobby database.
     * @return Pointer to the lobby's database
     */
    std::shared_ptr<libcomp::Database> GetLobbyDatabase() const;

    /**
     * Set the lobby database.
     * @param database Pointer to the lobby's database
     */
    void SetLobbyDatabase(const std::shared_ptr<libcomp::Database>& database);

protected:
    /**
     * Create a connection to a newly active socket.
     * @param socket A new socket connection.
     * @return Pointer to the newly created connection
     */
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    /// A shared pointer to the world database used by the server.
    std::shared_ptr<libcomp::Database> mDatabase;

    /// A shared pointer to the world database used by the server.
    std::shared_ptr<libcomp::Database> mLobbyDatabase;

    /// Pointer to the description of the world.
    std::shared_ptr<objects::WorldDescription> mDescription;

    /// Pointer to the descriptions of connected channels by their connections.
    std::map<std::shared_ptr<libcomp::InternalConnection>,
        std::shared_ptr<objects::ChannelDescription>> mChannelDescriptions;

    /// Pointer to the manager in charge of connection messages. 
    std::shared_ptr<ManagerConnection> mManagerConnection;
};

} // namespace world

#endif // SERVER_WORLD_SRC_WORLDSERVER_H
