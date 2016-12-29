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
#include <ChannelDescription.h>
#include <InternalConnection.h>

// object Includes
#include <WorldDescription.h>

namespace lobby
{
    
/**
 * Associates a world connection to a description of the world
 * and its channels for the lobby.
 */
class World
{
public:
    /**
     * Create a new world from a world connection.
     * @param connection An active world connection
     */
    World(std::shared_ptr<libcomp::InternalConnection> connection);
    
    /**
     * Clean up the world.
     */
    virtual ~World();
    
    /**
     * Gather necessary information for the world by sending a
     * request for the world description.
     * @return true on success, false on failure
     */
    bool Initialize();
    
    /**
     * Get a pointer to the world's connection.
     * @return Pointer to the world's connection
     */
    std::shared_ptr<libcomp::InternalConnection> GetConnection() const;
    
    /**
     * Get a pointer to the world's description.
     * @return Pointer to the world's description
     */
    std::shared_ptr<objects::WorldDescription> GetWorldDescription() const;
    
    /**
     * Get a list of pointers to the world's channel descriptions.
     * @return List of pointers to the world's channel descriptions
     */
    const std::list<std::shared_ptr<objects::ChannelDescription>> GetChannelDescriptions() const;
    
    /**
     * Get a pointer to a channel description by its ID.
     * @param id ID of a channel associated to the world
     * @return Pointer to the matching channel description
     */
    std::shared_ptr<objects::ChannelDescription> GetChannelDescriptionByID(uint8_t id) const;
    
    /**
     * Remove a channel description by its ID.
     * @param id ID of a channel associated to the world
     * @return true if the description existed, false if it did not exist
     */
    bool RemoveChannelDescriptionByID(uint8_t id);
    
    /**
     * Set the world description.
     * @param worldDescription Pointer to the world's description
     */
    void SetWorldDescription(const std::shared_ptr<objects::WorldDescription>& worldDescription);
    
    /**
     * Set a channel description.
     * @param channelDescription Pointer to a channel's description
     */
    void SetChannelDescription(const std::shared_ptr<objects::ChannelDescription>& channelDescription);

private:
    /// Pointer to the world's connection
    std::shared_ptr<libcomp::InternalConnection> mConnection;

    /// Pointer to the world's description
    std::shared_ptr<objects::WorldDescription> mWorldDescription;

    /// List of pointers to the channel descriptions
    std::list<std::shared_ptr<objects::ChannelDescription>> mChannelDescriptions;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_WORLD_H
