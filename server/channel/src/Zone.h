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

// channel Includes
#include "ActiveEntityState.h"
#include "BazaarState.h"
#include "ChannelClientConnection.h"
#include "EnemyState.h"
#include "EntityState.h"
#include "ZoneGeometry.h"

// object Includes
#include <ServerZoneInstanceVariant.h>
#include <ZoneObject.h>

// Standard C++11 includes
#include <map>

namespace objects
{
class ActionSpawn;
class Ally;
class Loot;
class LootBox;
class ServerNPC;
class ServerObject;
class ServerZone;
class SpawnRestriction;
}

namespace channel
{

class ChannelClientConnection;
class PlasmaState;
class WorldClock;
class ZoneInstance;

typedef ActiveEntityStateImp<objects::Ally> AllyState;
typedef EntityState<objects::LootBox> LootBoxState;
typedef EntityState<objects::ServerNPC> NPCState;
typedef EntityState<objects::ServerObject> ServerObjectState;

typedef objects::ServerZoneInstanceVariant::InstanceType_t InstanceType_t;

/**
 * Represents a server zone containing client connections, objects,
 * enemies, etc.
 */
class Zone : public objects::ZoneObject
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
     * Get the defintion ID of the zone
     * @return Defintion ID of the zone
     */
    uint32_t GetDefinitionID();

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
     * Get the instance the zone belongs to if one exists
     * @return Instance the zone belongs to
     */
    std::shared_ptr<ZoneInstance> GetInstance() const;

    /**
     * Get the instance variant the zone belongs to if one exists
     * @return Instance variant the zone belongs to
     */
    InstanceType_t GetInstanceType() const;

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
     * Check if the zone has respawnable entities associated to it
     * @return true if the zone has respawnable entities associated to it
     */
    bool HasRespawns() const;

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
     * @param spawnDelay Optional parameter to update spawn groups with
     *  a timed delay (in seconds) instead of right away, should the group be
     *  empty after the change
     */
    void RemoveEntity(int32_t entityID, uint32_t spawnDelay = 0);

    /**
     * Add an ally to the zone
     * @param ally Pointer to the ally to add
     */
    void AddAlly(const std::shared_ptr<AllyState>& ally);

    /**
     * Add a bazaar to the zone
     * @param bazaar Pointer to the bazaar to add
     */
    void AddBazaar(const std::shared_ptr<BazaarState>& bazaar);

    /**
     * Add an enemy to the zone
     * @param enemy Pointer to the enemy to add
     */
    void AddEnemy(const std::shared_ptr<EnemyState>& enemy);

    /**
     * Add a loot body to the zone
     * @param box Pointer to the loot box to add
     * @param bossGroupID Optional ID of the group to bind looting to
     *  for boss boxes
     */
    void AddLootBox(const std::shared_ptr<LootBoxState>& box,
        uint32_t bossGroupID = 0);

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
     * Add a plasma grouping to the zone
     * @param object Pointer to the plasma to add
     */
    void AddPlasma(const std::shared_ptr<PlasmaState>& plasma);

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
     * Get an ally instance by it's ID.
     * @param id Instance ID of the ally.
     * @return Pointer to the ally instance.
     */
    std::shared_ptr<AllyState> GetAlly(int32_t id);

    /**
     * Get all ally instances in the zone
     * @return List of all ally instances in the zone
     */
    const std::list<std::shared_ptr<AllyState>> GetAllies() const;

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
     * Mark a boss box as belonging to the specified entity if
     * the box is not already claimed and the entity has not
     * already claimed the box
     * @param id Instance ID of the boss box.
     * @param looterID ID of the entity claiming the box
     * @return true if the box was claimed, false if it was not
     */
    bool ClaimBossBox(int32_t id, int32_t looterID);

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
     * Get a plasma instance by it's definition ID.
     * @param id Definition ID of the plasma.
     * @return Pointer to the plasma instance.
     */
    std::shared_ptr<PlasmaState> GetPlasma(uint32_t id);

    /**
     * Get all plasma instances in the zone
     * @return List of all plasma instances in the zone
     */
    const std::unordered_map<uint32_t,
        std::shared_ptr<PlasmaState>> GetPlasma() const;

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
     *  a living entity, false otherwise
     */
    bool GroupHasSpawned(uint32_t groupID, bool isLocation, bool aliveOnly);

    /**
     * Check if an entity has ever spawned at the specified spot.
     * @param spotID Spot ID of the spawn to check
     * @return true if an entity has ever spawned at the spot
     */
    bool SpawnedAtSpot(uint32_t spotID);

    /**
     * Create an encounter from a group of entities and register them with
     * the zone. Encounter information will be retained until a check via
     * EncounterDefeated is called.
     * @param entities List of the entities to add to the encounter
     * @param spawnSource Optional pointer to spawn action that created the
     *  encounter. If this is specified, it will be returned as the
     *  defeatActionSource when calling EncounterDefeated
     */
    void CreateEncounter(const std::list<std::shared_ptr<
        ActiveEntityState>>& entities,
        std::shared_ptr<objects::ActionSpawn> spawnSource = nullptr);

    /**
     * Determine if an entity encounter has been defeated and clean up the
     * encounter information for the zone.
     * @param encounterID ID of the encounter
     * @param defeatActionSource Output parameter to contain the original
     *  spawn action source that can contain defeat actions to execute
     * @return true if the encounter has been defeated, false if at least
     *  one entity from the group is still alive
     */
    bool EncounterDefeated(uint32_t encounterID,
        std::shared_ptr<objects::ActionSpawn>& defeatActionSource);

    /**
     * Get the IDs of all entities in the zone marked for despawn.
     * @return Set of all entity IDs to despawn
     */
    std::set<int32_t> GetDespawnEntities();

    /**
     * Get all spawn groups in this zone that have been marked as disabled
     * either due to time restrictions or manually
     * @return Set of disabled spawn group IDs
     */
    std::set<uint32_t> GetDisabledSpawnGroups();

    /**
     * Mark an entity for despawn in the zone. Once marked, it cannot
     * be unmarked.
     * @param entityID ID of the entity to mark for despawn
     */
    void MarkDespawn(int32_t entityID);

    /**
     * Update all spawn groups and plasma states that have time restrictions
     * based upon the supplie world clock time
     * @param clock World clock set to the current time
     * @return true if any updates were performed that the zone manager needs
     *  to react to, false if none occurred or they can be processed later
     */
    bool UpdateTimedSpawns(const WorldClock& clock);

    /**
     * Enable or disable the supplied spawn groups and also disable (or enable)
     * any spawn location groups that now have all groups disabled (or one enabled)
     * @param spawnGroupIDs Set of spawn group IDs to adjust
     * @param enable true if the group should be enabled, false if it should
     *  be disabled
     * @return true if any updates were performed that the zone manager needs
     *  to react to, false if none occurred or they can be processed later
     */
    bool EnableDisableSpawnGroups(const std::set<uint32_t>& spawnGroupIDs,
        bool enable);

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
     * Return the flag states for the zone.
     * @return Flag states for the zone.
     */
    std::unordered_map<int32_t, std::unordered_map<
        int32_t, int32_t>> GetFlagStates();

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
     * Get the XP multiplier for the zone combined with any variant
     * specific boosts.
     * @return XP multiplier for the zone
     */
    float GetXPMultiplier();

    /**
     * Take loot out of the specified loot box. This should be the only way
     * loot is removed from a box as it uses a mutex lock to ensure that the
     * loot is not duplicated for two different people. This currently does NOT
     * support splitting stacks within the loot box when the box being moved to
     * becomes full.
     * @param lBox Pointer to the loot box
     * @param slots Loot slots being requested or empty to request all
     * @param freeSlots Contextual number of free slots signifying how many
     *  loot items can be taken
     * @param stacksFree Contextual number of free stack slots used when
     *  determining how many items can be taken
     * @return Map of slot IDs to the loot taken
     */
    std::unordered_map<size_t, std::shared_ptr<objects::Loot>>
        TakeLoot(std::shared_ptr<objects::LootBox> lBox, std::set<int8_t> slots,
            size_t freeSlots, std::unordered_map<uint32_t, uint16_t> stacksFree = {});

    /**
     * Determines if the supplied path collides with anything in the zone's
     * geometry
     * @param path Line representing a path
     * @param point Output parameter to set where the intersection occurs
     * @param surface Output parameter to return the first line to be
     *  intersected by the path
     * @param shape Output parameter to return the first shape the path
     *  will collide with. This will always be the shape the surface
     *  belongs to
     * @return true if the line collides, false if it does not
     */
    bool Collides(const Line& path, Point& point,
        Line& surface, std::shared_ptr<ZoneShape>& shape) const;

    /**
     * Determines if the supplied path collides with anythin in the zone's
     * geometry
     * @param path Line representing a path
     * @param point Output parameter to set where the intersection occurs
     * @return true if the line collides, false if it does not
     */
    bool Collides(const Line& path, Point& point) const;

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

    /**
     * Determine based on the supplied clock time if a spawn restriction
     * is active or not
     * @param clock World clock set to the current time
     * @param restriction Pointer to the spawn restriction to evaluate
     * @return true if the current time is valid for the restriction,
     *  false if it is not
     */
    bool TimeRestrictionActive(const WorldClock& clock,
        const std::shared_ptr<objects::SpawnRestriction>& restriction);

    /**
     * Register a new spawned entity to the zone stored spots and group field
     * @param state Pointer to the state of the spawned entity
     * @param spotID Spot ID being spawned into
     * @param sgID Spawn group ID being spawned from
     * @param slgID Spawn location group ID being spawned from
     */
    void AddSpawnedEntity(const std::shared_ptr<ActiveEntityState>& state,
        uint32_t spotID, uint32_t sgID, uint32_t slgID);

    /**
     * Enable a set of spawn groups and update any spawn location groups
     * that previously had all groups disabled
     * @param spawnGroupIDs Set of spawn group IDs to enable
     */
    void EnableSpawnGroups(const std::set<uint32_t>& spawnGroupIDs);

    /**
     * Disable a set of spawn groups and update any spawn location groups
     * that now have all groups disabled
     * @param spawnGroupIDs Set of spawn group IDs to disable
     */
    bool DisableSpawnGroups(const std::set<uint32_t>& spawnGroupIDs);

    /// Map of world CIDs to client connections
    std::unordered_map<int32_t, std::shared_ptr<ChannelClientConnection>> mConnections;

    /// List of pointers to allies instantiated for the zone
    std::list<std::shared_ptr<AllyState>> mAllies;

    /// List of pointers to bazaars instantiated for the zone
    std::list<std::shared_ptr<BazaarState>> mBazaars;

    /// List of pointers to enemies instantiated for the zone
    std::list<std::shared_ptr<EnemyState>> mEnemies;

    /// Map of spawn group IDs to pointers to entities created from that group.
    /// Keys are never removed from this group so one time spawns can be checked.
    std::unordered_map<uint32_t,
        std::list<std::shared_ptr<ActiveEntityState>>> mSpawnGroups;

    /// Map of spawn location group IDs to pointers to entities created from the groups.
    /// Keys are never removed from this group so one time spawns can be checked.
    std::unordered_map<uint32_t,
        std::list<std::shared_ptr<ActiveEntityState>>> mSpawnLocationGroups;

    /// Map of encounter IDs to entities that belong to that encounter.
    std::unordered_map<uint32_t,
        std::set<std::shared_ptr<ActiveEntityState>>> mEncounters;

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

    /// Map of boss box group IDs to the boxes included
    std::unordered_map<uint32_t, std::set<int32_t>> mBossBoxGroups;

    /// Map of boss box group IDs to entities that have claimed part of the group
    std::unordered_map<uint32_t, std::set<int32_t>> mBossBoxOwners;

    /// Map of plasma states by definition ID
    std::unordered_map<uint32_t, std::shared_ptr<PlasmaState>> mPlasma;

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

    /// Set of entity IDs waiting to despawn. IDs are removed from this set when
    /// the entity is removed from the zone.
    std::set<int32_t> mPendingDespawnEntities;

    /// Set of spawn group IDs that have been disabled
    std::set<uint32_t> mDisabledSpawnGroups;

    /// Set of spawn location group IDs where all associated groups are disabled
    std::set<uint32_t> mDisabledSpawnLocationGroups;

    /// General use flags and associated values used for event sequences etc
    /// keyed on 0 for all characters or world CID if for a specific one
    std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>> mFlagStates;

    /// Geometry information bound to the zone
    std::shared_ptr<ZoneGeometry> mGeometry;

    /// Dynamic map information bound to the zone
    std::shared_ptr<DynamicMap> mDynamicMap;

    /// Zone instance pointer for non-global zones
    std::shared_ptr<ZoneInstance> mZoneInstance;

    /// Next ID to use for encounters registered for the zone
    uint32_t mNextEncounterID;

    /// Quick reference flag to determine if the zone has respwa
    bool mHasRespawns;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ZONE_H
