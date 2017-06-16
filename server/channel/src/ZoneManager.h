/**
 * @file server/channel/src/ZoneManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages zone instance objects and connections.
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

#ifndef SERVER_CHANNEL_SRC_ZONEMANAGER_H
#define SERVER_CHANNEL_SRC_ZONEMANAGER_H

// channel Includes
#include "ChannelClientConnection.h"
#include "Zone.h"

namespace libcomp
{
class Packet;
}

namespace channel
{

class ChannelServer;

/**
 * Manager to handle zone focused actions.
 */
class ZoneManager
{
public:
    /**
     * Create a new ZoneManager.
     * @param server Pointer back to the channel server this
     *  belongs to
     */
    ZoneManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the ZoneManager.
     */
    ~ZoneManager();

    /**
     * Get the zone instance associated to a client connection
     * @param client Client connection connected to a zone
     * @return Pointer to a zone instance
     */
    std::shared_ptr<Zone> GetZoneInstance(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Get the zone instance associated to a character entity ID
     * @param primaryEntityID Character entity ID associated to a client
     *  connection as the primary entity
     * @return Pointer to a zone instance
     */
    std::shared_ptr<Zone> GetZoneInstance(int32_t primaryEntityID);

    /**
     * Associate a client connection to a zone
     * @param client Client connection to connect to a zone
     * @param zoneID Definition ID of a zone to add the client to
     * @param xCoord X-coordinate to send character to.
     * @param yCoord Y-coordinate to send character to.
     * @param rotation Character rotation upon entering the zone.
     * @param forceLeave Optional param that when set to true will force
     *  a call to LeaveZone even if the zone they are moving to is the same
     * @return true if the client entered the zone properly, false if they
     *  did not
     */
    bool EnterZone(const std::shared_ptr<ChannelClientConnection>& client,
        uint32_t zoneID, float xCoord, float yCoord, float rotation,
        bool forceLeave = false);

    /**
     * Remove a client connection from a zone
     * @param client Client connection to remove from any associated zone
     */
    void LeaveZone(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Send data about entities that exist in a zone to a new connection and
     * update any existing connections with information about the new one.
     * @param client Client connection that was added to a zone
     */
    void SendPopulateZoneData(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Tell the game client to show an entity.
     * @param client Pointer to the client connection
     * @param entityID ID of the entity to show
     * @param queue true if the message should be queued, false if
     *  it should send right away
     */
    void ShowEntity(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t entityID, bool queue = false);

    /**
     * Tell all game clients in a zone to show an entity.
     * @param client Pointer to the client connection with an entity
     *  to show to other clients in the same zone.
     * @param entityID ID of the entity to show
     */
    void ShowEntityToZone(const std::shared_ptr<
        ChannelClientConnection>& client, int32_t entityID);

    /**
     * Tell the game client to prepare an entity for display.
     * @param client Pointer to the client connection
     * @param entityID ID of the entity to prep
     * @param type Client-side entity type identifier
     * @param queue true if the message should be queued, false if
     *  it should send right away
     */
    void PopEntityForProduction(const std::shared_ptr<
        ChannelClientConnection>& client, int32_t entityID, int32_t type, bool queue = false);

    /**
     * Tell all game clients to prepare an entity for display.
     * @param client Pointer to the client connection with an entity
     *  to prep to other clients in the same zone
     * @param entityID ID of the entity to prep
     * @param type Client-side entity type identifier
     */
    void PopEntityForZoneProduction(const std::shared_ptr<
        ChannelClientConnection>& client, int32_t entityID, int32_t type);

    /**
     * Tell all game clients in a zone to remove an entity.
     * @param zone Pointer to the zone to remove the entities from
     * @param entityIDs IDs of the entities to remove
     */
    void RemoveEntitiesFromZone(const std::shared_ptr<Zone>& zone,
        const std::list<int32_t>& entityIDs);

    /**
     * Send a packet to every connection in the zone or all but the client specified
     * @param client Client connection to use as the "source" connection
     * @param p Packet to send to the zone
     * @param includeSelf Optional parameter to include the connection being passed
     *  in when sending the packets. Defaults to true
     */
    void BroadcastPacket(const std::shared_ptr<ChannelClientConnection>& client,
        libcomp::Packet& p, bool includeSelf = true);

    /**
    * sends a packet to a specified range
    * @param client Client connection to use as the "source" connection
    * @param p Packet to send to the zone
    * @param includeSelf Optional parameter to include the connection being passed
    *  in when sending the packets. Defaults to true
    */
    void SendToRange(const std::shared_ptr<ChannelClientConnection>& client, libcomp::Packet& p, bool includeSelf = true);

    /**
     * Get a list of client connections in the zone
     * @param client Client connection to use as the "source" connection
     * @param includeSelf Optional parameter to include the connection being passed
     *  in as part of the return set. Defaults to true
     * @return List of client connections in the zone
     */
    std::list<std::shared_ptr<ChannelClientConnection>> GetZoneConnections(
        const std::shared_ptr<ChannelClientConnection>& client,
        bool includeSelf = true);

    /**
     * Spawn an enemy in the specified zone instance at set coordinates
     * @param zone Pointer to the zone instance where the enemy should be spawned
     * @param demonID Demon/enemy type ID to spawn
     * @param x X coordinate to render the enemy at
     * @param y Y coordinate to render the enemy at
     * @param rot Rotation to render the enemy with
     * @param aiType Optional parameter to specify the type of AI
     *  script to bind to the enemy by its type name
     * @return true if the enemy was spawned, false if it was not
     */
    bool SpawnEnemy(const std::shared_ptr<Zone>& zone, uint32_t demonID, float x,
        float y, float rot, const libcomp::String& aiType = "default");

    /**
     * Updates the current states of entities in the zone.  Enemy AI is
     * processed from here as well as updating the current state of moving
     * entities.
     */
    void UpdateActiveZoneStates();

    /**
     * Warp an entity to the specified location immediately.
     * @param client Pointer to the client connection to use for gathering zone
     *  information
     * @param eState Pointer to the state of the entity being warped
     * @param xPos X coordinate to warp the entity to
     * @param yPos Y coordinate to warp the entity to
     * @param rot Rotation to set at the target coordinates
     */
    void Warp(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<ActiveEntityState>& eState, float xPos, float yPos,
        float rot);

private:
    /**
     * Send entity information about an enemy in a zone
     * @param client Pointer to the client connection to send to or use as
     *  an identifier for the zone to send to
     * @param enemyState Enemy state to use to report enemy data to the clients
     * @param zone Pointer to the zone instance where the enemy exists
     * @param sendToAll true if all clients in the zone should be sent the data
     *  false if just the supplied client should be sent to
     * @param queue true if the message should be queued, false if
     *  it should send right away
     */
    void SendEnemyData(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<EnemyState>& enemyState,
        const std::shared_ptr<Zone>& zone, bool sendToAll, bool queue = false);

    /**
     * Send a packet to every connection in the specified zone
     * @param zone Pointer to the zone to send the packet to
     * @param p Packet to send to the zone
     */
    void BroadcastPacket(const std::shared_ptr<Zone>& zone, libcomp::Packet& p);

    /**
     * Get a zone instance by zone definition ID. This function is responsible for
     * deciding if a non-public zone should have an additional instance created.
     * @param zoneID Zone definition ID to get an instance for
     * @return Pointer to a matching zone instance
     */
    std::shared_ptr<Zone> GetZone(uint32_t zoneID);

    /**
     * Create a new zone instance based off of the supplied definition
     * @param definition Pointer to a zone definition
     * @return Pointer to a new zone instance
     */
    std::shared_ptr<Zone> CreateZoneInstance(
        const std::shared_ptr<objects::ServerZone>& definition);

    /// Map of zone intances by instance ID
    std::unordered_map<uint32_t, std::shared_ptr<Zone>> mZones;

    /// Map of zone definition IDs to zone instance IDs
    std::unordered_map<uint32_t, std::set<uint32_t>> mZoneMap;

    /// Map of primary entity IDs to zone instance IDs
    std::unordered_map<int32_t, uint32_t> mEntityMap;

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;

    /// Next available zone instance ID
    uint32_t mNextZoneInstanceID;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ZONEMANAGER_H
