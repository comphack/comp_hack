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

 // object Includes
#include <LootBox.h>

// channel Includes
#include "ActiveEntityState.h"
#include "ChannelClientConnection.h"
#include "EnemyState.h"
#include "EntityState.h"
#include "ZoneGeometry.h"

// Standard C++11 includes
#include <map>

namespace objects
{
class LootBox;
class ServerNPC;
class ServerObject;
class ServerZone;
}

namespace channel
{

class ChannelClientConnection;

typedef EntityState<objects::LootBox> LootBoxState;
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
    Zone(uint32_t id, const std::shared_ptr<objects::ServerZone>& definition);

    /**
     * Clean up the zone instance.
     */
    ~Zone();

    /**
     * Get the unique instance ID of the zone
     * @return Unique instance ID of the zone
     */
    uint32_t GetID();

    /**
     * Get the owner ID of the zone (only applies to non-global zones)
     * @return Owner ID of the zone
     */
    int32_t GetOwnerID() const;

    /**
     * Set the owner ID of the zone (only applies to non-global zones)
     * @param ownerID Owner ID of the zone
     */
    void SetOwnerID(int32_t ownerID);

    /**
     * Get the geometry information bound to the zone
     * @return Geometry information bound to the zone
     */
    const std::shared_ptr<ZoneGeometry> GetGeometry() const;

    /**
     * Set the geometry information bound to the zone
     * @param geometry Geometry information bound to the zone
     */
    void SetGeometry(const std::shared_ptr<ZoneGeometry>& geometry);

    /**
     * Add a client connection to the zone and register its world CID
     * @param client Pointer to a client connection to add
     */
    void AddConnection(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Remove a client connection from the zone and unregister its world CID
     * @param client Pointer to a client connection to remove
     */
    void RemoveConnection(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Remove an entity from the zone. For player entities, use RemoveConnection
     * instead.
     * @param entityID ID of the entity to remove
     * @param updateSpawns Optional parameter to update spawn groups
     *  now instead of waiting for the cleanup of loot boxes etc
     */
    void RemoveEntity(int32_t entityID, bool updateSpawns = false);

    /**
     * Add an enemy to the zone
     * @param enemy Pointer to the enemy to add
     */
    void AddEnemy(const std::shared_ptr<EnemyState>& enemy);

    /**
     * Add a loot body to the zone
     * @param box Pointer to the loot box to add
     */
    void AddLootBox(const std::shared_ptr<LootBoxState>& box);

    /**
     * Add an NPC to the zone
     * @param npc Pointer to the NPC to add
     */
    void AddNPC(const std::shared_ptr<NPCState>& npc);

    /**
     * Add an object to the zone
     * @param object Pointer to the object to add
     */
    void AddObject(const std::shared_ptr<ServerObjectState>& object);

    /**
     * Get all client connections in the zone mapped by world CID
     * @return Map of all client connections in the zone by world CID
     */
    std::unordered_map<int32_t,
        std::shared_ptr<ChannelClientConnection>> GetConnections();

    /**
     * Get all client connections in the zone as a list
     * @return List of all client connections in the zone
     */
    std::list<std::shared_ptr<ChannelClientConnection>> GetConnectionList();

    /**
     * Get an active entity in the zone by ID
     * @param entityID ID of the active entity to retrieve
     * @return Pointer to the active entity or null if does not exist or is
     *  not active
     */
    const std::shared_ptr<ActiveEntityState> GetActiveEntity(int32_t entityID);

    /**
     * Get all active entities in the zone
     * @return List of pointers to all active entities
     */
    const std::list<std::shared_ptr<ActiveEntityState>> GetActiveEntities();

    /**
     * Get all active entities in the zone within a supplied radius
     * @param x X coordinate of the center of the radius
     * @param y Y coordinate of the center of the radius
     * @param radius Radius to check for entities
     * @return List of pointers to active entities in the radius
     */
    const std::list<std::shared_ptr<ActiveEntityState>>
        GetActiveEntitiesInRadius(float x, float y, double radius);

    /**
     * Get an entity instance by it's ID.
     * @param id Instance ID of the entity.
     * @return Pointer to the entity instance.
     */
    std::shared_ptr<objects::EntityStateObject> GetEntity(int32_t id);

    /**
     * Get an entity instance with a specified actor ID.
     * @param actorID Actor ID of the entity.
     * @return Pointer to the entity instance.
     */
    std::shared_ptr<objects::EntityStateObject> GetActor(int32_t actorID);

    /**
     * Get an enemy instance by it's ID.
     * @param id Instance ID of the enemy.
     * @return Pointer to the enemy instance.
     */
    std::shared_ptr<EnemyState> GetEnemy(int32_t id);

    /**
     * Get all enemy instances in the zone
     * @return List of all enemy instances in the zone
     */
    const std::list<std::shared_ptr<EnemyState>> GetEnemies() const;

    /**
     * Get a loot box instance by it's ID.
     * @param id Instance ID of the loot box.
     * @return Pointer to the loot box instance.
     */
    std::shared_ptr<LootBoxState> GetLootBox(int32_t id);

    /**
     * Get all loot box instances in the zone
     * @return List of all loot box instances in the zone
     */
    const std::list<std::shared_ptr<LootBoxState>> GetLootBoxes() const;

    /**
     * Get an NPC instance by it's ID.
     * @param id Instance ID of the NPC.
     * @return Pointer to the NPC instance.
     */
    std::shared_ptr<NPCState> GetNPC(int32_t id);

    /**
     * Get all NPC instances in the zone
     * @return List of all NPC instances in the zone
     */
    const std::list<std::shared_ptr<NPCState>> GetNPCs() const;

    /**
     * Get an object instance by it's ID.
     * @param id Instance ID of the object.
     * @return Pointer to the object instance.
     */
    std::shared_ptr<ServerObjectState> GetServerObject(int32_t id);

    /**
     * Get all object instances in the zone
     * @return List of all object instances in the zone
     */
    const std::list<std::shared_ptr<ServerObjectState>> GetServerObjects() const;

    /**
     * Get the definition of the zone
     * @return Pointer to the definition of the zone
     */
    const std::shared_ptr<objects::ServerZone> GetDefinition();

    /**
     * Set the next status effect event time associated to an entity
     * in the zone
     * @param time Time of the next status effect event time
     * @param entityID ID of the entity with a status effect event
     *  at the specified time
     */
    void SetNextStatusEffectTime(uint32_t time, int32_t entityID);

    /**
     * Get the list of entities that have had registered status effect
     * event times that have passed since the specified time
     * @param now System time representing the current server time
     * @return List of entities that have had registered status effect
     *  event times that have passed
     */
    std::list<std::shared_ptr<ActiveEntityState>>
        GetUpdatedStatusEffectEntities(uint32_t now);

    /**
     * Check if a spawn group has ever been spawned in this zone or is
     * currently spawned.
     * @param spawnGroupID Group ID of the spawn to check
     * @param aliveOnly Only count living entities in the group
     * @return true if the group has already been spawned or contains,
     *  a living enemy, false otherwise
     */
    bool GroupHasSpawned(uint32_t spawnGroupID, bool aliveOnly);

    /**
     * Get the set of spawn groups that have room for another enemy spawn
     * that have also had their respawn time elapsed.
     * @param now System time representing the current server time
     * @return Map of spawn group IDs to the difference between the number
     *  of enemies still alive and the max allowed number
     */
    std::unordered_map<uint32_t, uint16_t> GetReinforceableSpawnGroups(uint64_t now);

    /**
     * Take loot out of the specified loot box. This should be the only way
     * loot is removed from a box as it uses a mutex lock to ensure that the
     * loot is not duplicated for two different people. This currently does NOT
     * support splitting stacks within the loot box when the box being moved to
     * becomes full.
     * @param lState Entity state of the loot box
     * @param slots Loot slots being requested or empty to request all
     * @param freeSlots Contextual number of free slots signifying how many
     *  loot items can be taken
     * @param stacksFree Contextual number of free stack slots used when
     *  determining how many items can be taken
     * @return Map of slot IDs to the loot taken
     */
    std::unordered_map<size_t, std::shared_ptr<objects::Loot>>
        TakeLoot(std::shared_ptr<LootBoxState> lState, std::set<int8_t> slots,
            size_t freeSlots, std::unordered_map<uint32_t, uint16_t> stacksFree = {});

    /**
     * Perform pre-deletion cleanup actions
     */
    void Cleanup();

private:
    /**
     * Register an entity as one that currently exists in the zone
     * @param state Pointer to an entity state in the zone
     */
    void RegisterEntityState(const std::shared_ptr<objects::EntityStateObject>& state);

    /**
     * Remove an entity that no longer exists in the zone by its ID
     * @param entityID ID of an entity to remove
     */
    void UnregisterEntityState(int32_t entityID);

    /// Pointer to the ServerZone definition
    std::shared_ptr<objects::ServerZone> mServerZone;

    /// Map of world CIDs to client connections
    std::unordered_map<int32_t, std::shared_ptr<ChannelClientConnection>> mConnections;

    /// List of pointers to enemies instantiated for the zone
    std::list<std::shared_ptr<EnemyState>> mEnemies;

    /// Map of spawn group IDs to pointers to enemies created from that group.
    /// Keys are never removed from this group so one time spawns can be cheked.
    std::unordered_map<uint32_t, std::list<std::shared_ptr<EnemyState>>> mSpawnGroups;

    /// List of pointers to NPCs instantiated for the zone
    std::list<std::shared_ptr<NPCState>> mNPCs;

    /// List of pointers to objects instantiated for the zone
    std::list<std::shared_ptr<ServerObjectState>> mObjects;

    /// List of pointers to lootable boxes for the zone
    std::list<std::shared_ptr<LootBoxState>> mLootBoxes;

    /// Map of entities in the zone by their ID
    std::unordered_map<int32_t, std::shared_ptr<objects::EntityStateObject>> mAllEntities;

    /// Map of entities in the zone with a specified actor ID for used
    /// when referencing in actions or events
    std::unordered_map<int32_t, std::shared_ptr<objects::EntityStateObject>> mActors;

    /// Map of system times to active entities with status effects that need
    /// handling at that time
    std::map<uint32_t, std::set<int32_t>> mNextEntityStatusTimes;

    /// Map of server times to spawn group IDs that need to be reinforced
    /// at that time
    std::map<uint64_t, std::set<uint32_t>> mSpawnGroupReinforceTimes;

    /// Geometry information bound to the zone
    std::shared_ptr<ZoneGeometry> mGeometry;

    /// Unique instance ID of the zone
    uint32_t mID;

    /// If the zone is private, this is the world CID of the character who
    /// instantiated it
    int32_t mOwnerID;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ZONE_H
