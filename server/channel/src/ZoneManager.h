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
     * @return true if the client entered the zone properly, false if they
     *  did not
     */
    bool EnterZone(const std::shared_ptr<ChannelClientConnection>& client,
        uint32_t zoneID);

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

    void SendToRange(const std::shared_ptr<ChannelClientConnection>& client, libcomp::Packet& p, bool includeSelf = true);
    /**
     * Get a list of client connections in the zone
     * @param client Client connection to use as the "source" connection
     * @return List of client connections in the zone
     * @param includeSelf Optional parameter to include the connection being passed
     *  in as part of the return set. Defaults to true
     */
    std::list<std::shared_ptr<ChannelClientConnection>> GetZoneConnections(
        const std::shared_ptr<ChannelClientConnection>& client,
        bool includeSelf = true);
private:
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
