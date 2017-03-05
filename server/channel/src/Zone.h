/**
 * @file server/channel/src/Zone.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents an instance of a zone.
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_ZONE_H
#define SERVER_CHANNEL_SRC_ZONE_H

// channel Includes
#include "ChannelClientConnection.h"
#include "EntityState.h"

namespace objects
{
class ServerNPC;
class ServerObject;
class ServerZone;
}

namespace channel
{

class ChannelClientConnection;

typedef EntityState<objects::ServerNPC> NPCState;
typedef EntityState<objects::ServerObject> ServerObjectState;

/**
 * Represents an instance of a zone containing client connections, objects,
 * enemies, etc.
 */
class Zone
{
public:
    /**
     * Create a new zone instance.
     * @param id Unique instance ID of the zone
     * @param definition Pointer to the ServerZone definition
     */
    Zone(uint64_t id, const std::shared_ptr<objects::ServerZone>& definition);

    /**
     * Clean up the zone instance.
     */
    ~Zone();

    /**
     * Get the unique instance ID of the zone
     * @return Unique instance ID of the zone
     */
    uint64_t GetID();

    /**
     * Add a client connection to the zone and register its primary entity ID
     * @param client Pointer to a client connection to add
     */
    void AddConnection(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Remove a client connection from the zone and unregister its primary entity ID
     * @param client Pointer to a client connection to remove
     */
    void RemoveConnection(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Add an NPC to the zone
     * @param npc Pointer to the definition of an NPC to add
     * @return Pointer to the instantiated NPC for the zone
     */
    std::shared_ptr<NPCState> AddNPC(const std::shared_ptr<objects::ServerNPC>& npc);

    /**
     * Add an object to the zone
     * @param object Pointer to the definition of an object to add
     * @return Pointer to the instantiated object for the zone
     */
    std::shared_ptr<ServerObjectState> AddObject(
        const std::shared_ptr<objects::ServerObject>& object);

    /**
     * Get all client connections in the zone mapped by primary entity ID
     * @return Map of all client connections in the zone by primary entity ID
     */
    std::unordered_map<int32_t,
        std::shared_ptr<ChannelClientConnection>> GetConnections();

    /**
     * Get all NPC instances in the zone
     * @return List of all NPC instances in the zone
     */
    const std::list<std::shared_ptr<NPCState>> GetNPCs();

    /**
     * Get all object instances in the zone
     * @return List of all object instances in the zone
     */
    const std::list<std::shared_ptr<ServerObjectState>> GetObjects();

    /**
     * Get the definition of the zone
     * @return Pointer to the definition of the zone
     */
    const std::shared_ptr<objects::ServerZone> GetDefinition();

private:
    void BuildZone();

    /// Pointer to the ServerZone definition
    std::shared_ptr<objects::ServerZone> mServerZone;

    /// Map of primarty entity IDs to client connections
    std::unordered_map<int32_t, std::shared_ptr<ChannelClientConnection>> mConnections;

    /// List of pointers to NPCs instantiated for the zone
    std::list<std::shared_ptr<NPCState>> mNPCs;

    /// List of pointers to objects instantiated for the zone
    std::list<std::shared_ptr<ServerObjectState>> mObjects;

    /// Unique instance ID of the zone
    uint64_t mID;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ZONE_H
