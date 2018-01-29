/**
 * @file server/channel/src/Zone.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents a global or instanced zone on the channel.
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_ZONE_H
#define SERVER_CHANNEL_SRC_ZONE_H

 // object Includes
#include <LootBox.h>

// channel Includes
#include "ActiveEntityState.h"
#include "BazaarState.h"
#include "ChannelClientConnection.h"
#include "EnemyState.h"
#include "EntityState.h"
#include "ZoneGeometry.h"

// Standard C++11 includes
#include <map>

namespace objects
{
class ActionSpawn;
class LootBox;
class ServerNPC;
class ServerObject;
class ServerZone;
}

namespace channel
{

class ChannelClientConnection;
class ZoneInstance;

typedef EntityState<objects::LootBox> LootBoxState;
typedef EntityState<objects::ServerNPC> NPCState;
typedef EntityState<objects::ServerObject> ServerObjectState;

/**
 * Represents a server zone containing client connections, objects,
 * enemies, etc.
 */
class Zone
{
public:
    /**
     * Create a new zone. While not useful this constructor is
     * necessary for the script bindings.
     */
    Zone();

    /**
     * Create a new zone.
     * @param id Unique server ID of the zone
     * @param definition Pointer to the ServerZone definition
     */
    Zone(uint32_t id, const std::shared_ptr<objects::ServerZone>& definition);

    /**
     * Explicitly defined copy constructor necessary due to removal
     * of implicit constructor from non-copyable mutex member. This should
     * never actually be used.
     * @param other The other zone to copy
     */
    Zone(const Zone& other);

    /**
     * Clean up the zone.
     */
    ~Zone();

    /**
     * Get the unique server ID of the zone
     * @return Unique server ID of the zone
     */
    uint32_t GetID();

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
     * Get the instance the zone belongs to if on exists
     * @return Instance the zone belongs to
     */
    std::shared_ptr<ZoneInstance> GetInstance() const;

    /**
     * Set the instance the zone belongs to
     * @param instance Instance the zone belongs to
     */
    void SetInstance(const std::shared_ptr<ZoneInstance>& instance);

    /**
     * Get the dynamic map information bound to the zone
     * @return Dynamic map information bound to the zone
     */
    const std::shared_ptr<DynamicMap> GetDynamicMap() const;

    /**
     * Set the dynamic map information bound to the zone
     * @param map Dynamic maps information bound to the zone
     */
    void SetDynamicMap(const std::shared_ptr<DynamicMap>& map);

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
     * Add a bazaar to the zone
     * @param bazaar Pointer to the bazaar to add
     */
    void AddBazaar(const std::shared_ptr<BazaarState>& bazaar);

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
     * Get a bazaar instance by it's ID.
     * @param id Instance ID of the bazaar.
     * @return Pointer to the bazaar instance.
     */
    std::shared_ptr<BazaarState> GetBazaar(int32_t id);

    /**
     * Get all bazaar instances in the zone
     * @return List of all bazaar instances in the zone
     */
    const std::list<std::shared_ptr<BazaarState>> GetBazaars() const;

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
     * Check if a spawn group/location group has ever been spawned in this
     * zone or is currently spawned.
     * @param groupID Group ID of the spawn to check
     * @param aliveOnly Only count living entities in the group
     * @return true if the group has already been spawned or contains,
     *  a living enemy, false otherwise
     */
    bool GroupHasSpawned(uint32_t groupID, bool isLocation, bool aliveOnly);

    /**
     * Check if an enemy has ever spawned at the specified spot.
     * @param spotID Spot ID of the spawn to check
     * @return true if an enemy has ever spawned at the spot
     */
    bool SpawnedAtSpot(uint32_t spotID);

    /**
     * Create an encounter from a group of enemies and register them with
     * the zone. Encounter information will be retained until a check via
     * EncounterDefeated is called.
     * @param enemies List of the enemies to add to the encounter
     * @param spawnSource Optional pointer to spawn action that created the
     *  encounter. If this is specified, it will be returned as the
     *  defeatActionSource when calling EncounterDefeated
     */
    void CreateEncounter(const std::list<std::shared_ptr<EnemyState>>& enemies,
        std::shared_ptr<objects::ActionSpawn> spawnSource = nullptr);

    /**
     * Determine if an enemy encounter has been defeated and clean up the
     * encounter information for the zone.
     * @param encounterID ID of the encounter
     * @param defeatActionSource Output parameter to contain the original
     *  spawn action source that can contain defeat actions to execute
     * @return true if the encounter has been defeated, false if at least
     *  one enemy from the group is still alive
     */
    bool EncounterDefeated(uint32_t encounterID,
        std::shared_ptr<objects::ActionSpawn>& defeatActionSource);

    /**
     * Get the set of spawn location groups that need to be respawned.
     * @param now System time representing the current server time
     * @return Set of spawn location group IDs to respawn
     */
    std::set<uint32_t> GetRespawnLocations(uint64_t now);

    /**
     * Get the state of a zone flag.
     * @param key Lookup key for the flag
     * @param value Output value for the flag if it exists
     * @param worldCID CID of the character the flag belongs to, 0
     *  if it affects the entire instance
     * @return true if the flag exists, false if it does not
     */
    bool GetFlagState(int32_t key, int32_t& value, int32_t worldCID);

    /**
     * Get the state of a zone flag, returning the null default
     * if it does not exist.
     * @param key Lookup key for the flag
     * @param nullDefault Default value to return if the flag is
     *  not set
     * @param worldCID CID of the character the flag belongs to, 0
     *  if it affects the entire instance
     * @return Value of the specified flag or the nullDefault value
     *  if it does not exist
     */
    int32_t GetFlagStateValue(int32_t key, int32_t nullDefault,
        int32_t worldCID);

    /**
     * Set the state of a zone flag.
     * @param key Lookup key for the flag
     * @param value Value to set for the flag
     * @param worldCID CID of the character the flag belongs to, 0
     *  if it affects the entire instance
     */
    void SetFlagState(int32_t key, int32_t value, int32_t worldCID);

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

    /// List of pointers to bazaars instantiated for the zone
    std::list<std::shared_ptr<BazaarState>> mBazaars;

    /// List of pointers to enemies instantiated for the zone
    std::list<std::shared_ptr<EnemyState>> mEnemies;

    /// Map of spawn group IDs to pointers to enemies created from that group.
    /// Keys are never removed from this group so one time spawns can be checked.
    std::unordered_map<uint32_t, std::list<std::shared_ptr<EnemyState>>> mSpawnGroups;

    /// Map of spawn location group IDs to pointers to enemies created from the groups.
    /// Keys are never removed from this group so one time spawns can be checked.
    std::unordered_map<uint32_t, std::list<std::shared_ptr<EnemyState>>> mSpawnLocationGroups;

    /// Map of encounter IDs to enemies that belong to that encounter.
    std::unordered_map<uint32_t, std::set<std::shared_ptr<EnemyState>>> mEncounters;

    /// Map of encounter IDs to spawn actions that created the encounter
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::ActionSpawn>> mEncounterSpawnSources;

    /// Set of all spot IDs that have had an enemy spawned
    std::set<uint32_t> mSpotsSpawned;

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

    /// Map of server times to spawn location group IDs that need to be respawned
    /// at that time
    std::map<uint64_t, std::set<uint32_t>> mRespawnTimes;

    /// General use flags and associated values used for event sequences etc
    /// keyed on 0 for all characters or world CID if for a specific one
    std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>> mFlagStates;

    /// Geometry information bound to the zone
    std::shared_ptr<ZoneGeometry> mGeometry;

    /// Dynamic map information bound to the zone
    std::shared_ptr<DynamicMap> mDynamicMap;

    /// Zone instance pointer for non-global zones
    std::shared_ptr<ZoneInstance> mZoneInstance;

    /// Unique server ID of the zone
    uint32_t mID;

    /// Next ID to use for encounters registered for the zone
    uint32_t mNextEncounterID;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ZONE_H
