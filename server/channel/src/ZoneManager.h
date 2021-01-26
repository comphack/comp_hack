/**
 * @file server/channel/src/ZoneManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages zone objects and connections.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

// object Includes
#include <ServerZoneTrigger.h>

// libcomp Includes
#include <Mutex.h>

// libobjgen Includes
#include <UUID.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "Zone.h"
#include "ZoneGeometry.h"
#include "ZoneInstance.h"

namespace libcomp {
class Packet;
}

namespace objects {
class ActionSpawn;
class MiZoneData;
class PvPInstanceVariant;
class Spawn;
}  // namespace objects

namespace channel {

class ChannelServer;
class WorldClock;
class WorldClockTime;

typedef objects::ServerZoneTrigger::Trigger_t ZoneTrigger_t;

/**
 * Manager to handle zone focused actions.
 */
class ZoneManager {
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
   * Load all QMP zone geometry files and prepare them to be bound
   * to zones as they are instantiated. If a specific file fails to
   * load, an error will be returned but the zone will still be
   * accessible without server side collision support.
   */
  void LoadGeometry();

  /**
   * Instantiate all global zones the server is responsible for
   * hosting. This should be called only once, after a valid world
   * connection has been established but before any clients connect.
   */
  void InstanceGlobalZones();

  /**
   * Get the zone associated to a client connection
   * @param client Client connection connected to a zone
   * @return Pointer to a zone
   */
  std::shared_ptr<Zone> GetCurrentZone(
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Get the zone associated to a character entity ID
   * @param worldCID World CID of the character associated to a client
   *  connection
   * @return Pointer to a zone
   */
  std::shared_ptr<Zone> GetCurrentZone(int32_t worldCID);

  /**
   * Get a zone by definition ID and dynamic map ID
   * @param zoneID Definition ID of a zone to retrieve
   * @param dynamicMapID Dynamic Map ID of the zone to retrieve
   * @return Pointer to the zone associated to the supplied IDs
   */
  std::shared_ptr<Zone> GetGlobalZone(uint32_t zoneID, uint32_t dynamicMapID);

  /**
   * Get a zone by definition ID, dynamic map ID and (optionally)
   * instance ID
   * @param zoneID Definition ID of a zone to retrieve
   * @param dynamicMapID Dynamic Map ID of the zone to retrieve
   * @param instanceID Instance ID of a zone to retrieve
   * @return Pointer to the zone associated to the supplied IDs
   */
  std::shared_ptr<Zone> GetExistingZone(uint32_t zoneID, uint32_t dynamicMapID,
                                        uint32_t instanceID = 0);

  /**
   * Perform all zone entry setup and related actions for a player
   * associated to the supplied client using the default starting point
   * @param client Client connection for the player entering the zone
   * @param zoneID Definition ID of a zone to add the client to
   * @param dynamicMapID Dynamic Map ID of the zone to add the client to.
   *  if specified as 0, the first instance of the zone will be used
   * @return true if the player entered the zone properly, false if they
   *  did not
   */
  bool EnterZone(const std::shared_ptr<ChannelClientConnection>& client,
                 uint32_t zoneID, uint32_t dynamicMapID);

  /**
   * Perform all zone entry setup and related actions for a player
   * associated to the supplied client using an explicit entry point
   * @param client Client connection for the player entering the zone
   * @param zoneID Definition ID of a zone to add the client to
   * @param dynamicMapID Dynamic Map ID of the zone to add the client to.
   *  if specified as 0, the first instance of the zone will be used
   * @param xCoord X-coordinate to send character to.
   * @param yCoord Y-coordinate to send character to.
   * @param rotation Character rotation upon entering the zone.
   * @param forceLeave Optional param that when set to true will force
   *  a call to LeaveZone even if the zone they are moving to is the same
   * @return true if the player entered the zone properly, false if they
   *  did not
   */
  bool EnterZone(const std::shared_ptr<ChannelClientConnection>& client,
                 uint32_t zoneID, uint32_t dynamicMapID, float xCoord,
                 float yCoord, float rotation, bool forceLeave = false);

  /**
   * Remove a client connection from a zone
   * @param client Client connection to remove from any associated zone
   * @param logOut If true, special logout actions will be performed
   * @param newZoneID ID of the next zone the client is moving to, used
   *  for retaining zone instances that are not being cleaned up
   * @param newDynamicMapID Dynamic map ID of the next zone the client
   *  is moving to
   */
  void LeaveZone(const std::shared_ptr<ChannelClientConnection>& client,
                 bool logOut, uint32_t newZoneID = 0,
                 uint32_t newDynamicMapID = 0);

  /**
   * Create a zone instance with access granted for the supplied CIDs
   * @param access Pointer to the access definition
   * @return 0 if it failed to create, 1 if it created local, 2 if a request
   *  was sent to the world to create the instance
   */
  uint8_t CreateInstance(
      const std::shared_ptr<objects::InstanceAccess>& access);

  /**
   * Expire and remove an instance matching the supplied values
   * @param instanceID ID of the instance to expire
   * @param timeOut Disconnect time out to check on the instance to make
   *  sure no one has reconnected
   */
  void ExpireInstance(uint32_t instanceID, uint64_t timeOut);

  /**
   * Get a zone instance by ID
   * @param instanceID ID of the instance to retrieve
   * @return Pointer to the zone instance associated to the supplied ID
   */
  std::shared_ptr<ZoneInstance> GetInstance(uint32_t instanceID);

  /**
   * Get the zone instance access available to the supplied world CID
   * @param worldCID Character world CID
   * @return Pointer to the zone access, null if none exist
   */
  std::shared_ptr<objects::InstanceAccess> GetInstanceAccess(int32_t worldCID);

  /**
   * Get or create a zone in the supplied instance. Useful for creating
   * zones before anyone enters
   * @param instance Pointer ot the instance to get the zone from
   * @param zoneID Zone definition ID to get
   * @param dynamicMapID Zone dynamic map ID to get
   * @return Pointer to the zone, null if it failed to create
   */
  std::shared_ptr<Zone> GetInstanceZone(
      const std::shared_ptr<ZoneInstance>& instance, uint32_t zoneID,
      uint32_t dynamicMapID);

  /**
   * Get information about the correct zone and channel to log into for
   * the supplied character
   * @param character Pointer to the character attempting to log in
   * @param zoneID Output parameter for the zone ID to log into
   * @param dynamicMapID Output parameter for the dynamic map ID to log into
   * @param channelID Output parameter for the channel to log into or
   *  -1 if the channel does not matter
   * @param x X coordinate for the zone login
   * @param y Y coordinate for the zone login
   * @param rot Rotation for the zone login (in radians)
   * @return true if the zone was determined correctly, false if it was not
   */
  bool GetLoginZone(const std::shared_ptr<objects::Character>& character,
                    uint32_t& zoneID, uint32_t& dynamicMapID, int8_t& channelID,
                    float& x, float& y, float& rot);

  /**
   * Get or create the first zone in the supplied instance
   * @param instance Pointer ot the instance to get the zone from
   * @return Pointer to the zone, null if it failed to creat
   */
  std::shared_ptr<Zone> GetInstanceStartingZone(
      const std::shared_ptr<ZoneInstance>& instance);

  /**
   * Get the coordinates of a match starting point in the supplied zone for
   * the client
   * @param client Pointer to the client connection entering the match
   * @param zone Pointer to the zone the client is in
   * @param x Output parameter containing the starting position x coord
   * @param y Output parameter containing the starting position y coord
   * @param rot Output parameter containing the starting position rotation
   * @return true if the spot was found, false if it was not
   */
  bool GetMatchStartPosition(
      const std::shared_ptr<ChannelClientConnection>& client,
      const std::shared_ptr<Zone>& zone, float& x, float& y, float& rot);

  /**
   * Move the supplied client to the starting zone of the instance they
   * are entering
   * @param client Pointer to the client connection entering the instance
   * @param access Pointer to the access definition, will be loaded from the
   *  client if not specified
   * @param diasporaEnter Optional indicator required to be set if the client
   *  is entering a Diaspora instance as it's entrance criteria is unique
   * @return true if the move was successful, false if it was not
   */
  bool MoveToInstance(const std::shared_ptr<ChannelClientConnection>& client,
                      std::shared_ptr<objects::InstanceAccess> access = nullptr,
                      bool diasporaEnter = false);

  /**
   * Move the supplied client to the lobby zone of the zone they are
   * currently in
   * @param client Pointer to the client connection to move
   * @return true if the move was successful, false if it was not
   */
  bool MoveToLobby(const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Send data about entities that exist in a zone to a new connection and
   * update any existing connections with information about the new one.
   * @param client Client connection that was added to a zone
   * @return true if the client is in a zone, false if they are not
   */
  bool SendPopulateZoneData(
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Send a request to a client to show an entity.
   * @param client Pointer to the client connection
   * @param entityID ID of the entity to show
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void ShowEntity(const std::shared_ptr<ChannelClientConnection>& client,
                  int32_t entityID, bool queue = false);

  /**
   * Send a request to all clients in a zone to show an entity.
   * @param zone Pointer to the zone to show the entities to
   * @param entityID ID of the entity to show
   */
  void ShowEntityToZone(const std::shared_ptr<Zone>& zone, int32_t entityID);

  /**
   * Send a request to all clients in a zone to show a demon entity
   * and also relay any zone-in information to the source client.
   * @param zone Pointer to the zone to show the demon to
   * @param client Pointer to the source client connection
   */
  void ShowDemonToZone(const std::shared_ptr<Zone>& zone,
                       const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Send a request to a list of client to show an entity.
   * @param clients List of pointers to client connections
   * @param entityID ID of the entity to show
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void ShowEntity(
      const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
      int32_t entityID, bool queue = false);

  /**
   * Send a request to a client to prepare an entity for display.
   * @param client Pointer to the client connection
   * @param entityID ID of the entity to prep
   * @param type Client-side entity type identifier
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void PopEntityForProduction(
      const std::shared_ptr<ChannelClientConnection>& client, int32_t entityID,
      int32_t type, bool queue = false);

  /**
   * Send a request to all clients in a zone to prepare an entity
   * for display.
   * @param zone Pointer to the zone to prep to show entities to
   * @param entityID ID of the entity to prep
   * @param type Client-side entity type identifier
   */
  void PopEntityForZoneProduction(const std::shared_ptr<Zone>& zone,
                                  int32_t entityID, int32_t type);

  /**
   * Send a request to a list of clients to prepare an entity for display.
   * @param clients List of pointers to client connections
   * @param entityID ID of the entity to prep
   * @param type Client-side entity type identifier
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void PopEntityForProduction(
      const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
      int32_t entityID, int32_t type, bool queue = false);

  /**
   * Send a request to all game clients in a zone to remove an entity.
   * @param zone Pointer to the zone to remove the entities from
   * @param entityIDs IDs of the entities to remove
   * @param removalMode Optional preset removal mode
   *  0) Immediate removal
   *  2) Store partner demon
   *  3) Demon egg contract complete
   *  4) Replacing enemy with lootable body
   *  5) Replacing enemy with demon egg
   *  6) Replacing enemy with gift box
   *  7) Enemy despawn
   *  8) Enemy running away upon skill hit recovery
   *  12) Partner demon 2-way fusion
   *  13) Lootable body despawn
   *  15) Partner demon crystallized
   *  16) Partner demon 3-way fusion
   *  Seen but not unknown (1, 10)
   * @param queue true if the packet should be queued
   */
  void RemoveEntitiesFromZone(const std::shared_ptr<Zone>& zone,
                              const std::list<int32_t>& entityIDs,
                              int32_t removalMode = 0, bool queue = false);

  /**
   * Send a request to the specified game clients to remove an entity.
   * @param clients List of pointers to client connections
   * @param entityIDs IDs of the entities to remove
   * @param removalMode Optional preset removal mode. See
   *  ZoneManager::RemoveEntitiesFromZone for options
   * @param queue true if the packet should be queued
   */
  void RemoveEntities(
      const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
      const std::list<int32_t>& entityIDs, int32_t removalMode = 0,
      bool queue = false);

  /**
   * Send data about the supplied NPC to the zone or supplied clients
   * @param zone Pointer to the zone the NPC exists in
   * @param clients List of pointers to client connections
   * @param npcState State of the NPC to show
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void ShowNPC(
      const std::shared_ptr<Zone>& zone,
      const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
      const std::shared_ptr<NPCState>& npcState, bool queue = false);

  /**
   * Send data about the supplied object NPC to the zone or supplied clients
   * @param zone Pointer to the zone the object NPC exists in
   * @param clients List of pointers to client connections
   * @param objState State of the object NPC to show
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void ShowObject(
      const std::shared_ptr<Zone>& zone,
      const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
      const std::shared_ptr<ServerObjectState>& objState, bool queue = false);

  /**
   * Stop and fix an entity at their current position for the specified
   * amount of time.
   * @param eState Pointer to the entity to stop
   * @param fixUntil Server timestamp of when the entity can move again
   * @param now Current server time
   */
  void FixCurrentPosition(const std::shared_ptr<ActiveEntityState>& eState,
                          uint64_t fixUntil, uint64_t now = 0);

  /**
   * Schedule the delayed removal of one or more entities in the specified zone.
   * @param time Server time to schedule entity removal for
   * @param zone Pointer to the zone the entities belong to
   * @param entityIDs List of IDs associated to entities to remove
   * @param removeMode Optional parameter to modify the way the entity is
   * removed in regards to the client
   */
  void ScheduleEntityRemoval(uint64_t time, const std::shared_ptr<Zone>& zone,
                             const std::list<int32_t>& entityIDs,
                             int32_t removeMode = 0);

  /**
   * Send the details of a loot box in the specified client's zone to the client
   * specified or all clients in that zone
   * @param client Pointer to the client connection
   * @param lState Entity state of the loot box
   * @param eState Entity state of the entity that created the loot box,
   * specified when the loot box is first created
   * @param sendToAll true if all clients in the zone should be sent the data
   *  false if just the supplied client should be sent to
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void SendLootBoxData(const std::shared_ptr<ChannelClientConnection>& client,
                       const std::shared_ptr<LootBoxState>& lState,
                       const std::shared_ptr<ActiveEntityState>& eState,
                       bool sendToAll, bool queue = false);

  /**
   * Send information about the current market matching the supplied ID in a
   * bazaar to clients in the same zone
   * @param zone Pointer to the zone to send the data to
   * @param bState Entity state of the bazaar
   * @param marketID Market ID to send information about
   */
  void SendBazaarMarketData(const std::shared_ptr<Zone>& zone,
                            const std::shared_ptr<BazaarState>& bState,
                            uint32_t marketID);

  /**
   * Send information about the supplied culture machine state to clients in
   * the same zone
   * @param zone Pointer to the zone to send the data to
   * @param cmState Culture machine state to send data about
   */
  void SendCultureMachineData(
      const std::shared_ptr<Zone>& zone,
      const std::shared_ptr<CultureMachineState>& cmState);

  /**
   * Expire and inactivate bazaar markets and culture machines that are no
   * longer active in the specified zone
   * @param zone Pointer to a zone
   * @param expireBefore System time to expire rentals before. If not
   *  specified, the current system time will be used.
   */
  void ExpireRentals(const std::shared_ptr<Zone>& zone,
                     uint32_t expireBefore = 0);

  /**
   * Send a packet to every connection in the zone or all but the client
   * specified
   * @param client Client connection to use as the "source" connection
   * @param p Packet to send to the zone
   * @param includeSelf Optional parameter to include the connection being
   * passed in when sending the packets. Defaults to true
   */
  void BroadcastPacket(const std::shared_ptr<ChannelClientConnection>& client,
                       libcomp::Packet& p, bool includeSelf = true);

  /**
   * Send a packet to every connection in the specified zone
   * @param zone Pointer to the zone to send the packet to
   * @param p Packet to send to the zone
   */
  void BroadcastPacket(const std::shared_ptr<Zone>& zone, libcomp::Packet& p);

  /**
   * sends a packet to a specified range
   * @param client Client connection to use as the "source" connection
   * @param p Packet to send to the zone
   * @param includeSelf Optional parameter to include the connection being
   * passed in when sending the packets. Defaults to true
   */
  void SendToRange(const std::shared_ptr<ChannelClientConnection>& client,
                   libcomp::Packet& p, bool includeSelf = true);

  /**
   * Get a list of client connections in the zone
   * @param client Client connection to use as the "source" connection
   * @param includeSelf Optional parameter to include the connection being
   * passed in as part of the return set. Defaults to true
   * @return List of client connections in the zone
   */
  std::list<std::shared_ptr<ChannelClientConnection>> GetZoneConnections(
      const std::shared_ptr<ChannelClientConnection>& client,
      bool includeSelf = true);

  /**
   * Spawn an enemy in the specified zone at set coordinates
   * @param zone Pointer to the zone where the enemy should be spawned
   * @param demonID Demon/enemy type ID to spawn
   * @param x X coordinate to render the enemy at
   * @param y Y coordinate to render the enemy at
   * @param rot Rotation to render the enemy with
   * @param aiType Optional parameter to specify the type of AI
   *  script to bind to the enemy by its type name
   * @return true if the enemy was spawned, false if it was not
   */
  bool SpawnEnemy(const std::shared_ptr<Zone>& zone, uint32_t demonID, float x,
                  float y, float rot, const libcomp::String& aiType = "");

  /**
   * Create an ally in the specified zone at set coordinates but do not
   * add it to the zone yet
   * @param zone Pointer to the zone where the entity should be spawned
   * @param demonID Demon/enemy type ID to spawn
   * @param spotID Optional spot ID to add the enemy to
   * @param x X coordinate to render the ally at
   * @param y Y coordinate to render the ally at
   * @param rot Rotation to render the ally with
   * @return Pointer to the new ActiveEntityState
   */
  std::shared_ptr<ActiveEntityState> CreateAlly(
      const std::shared_ptr<Zone>& zone, uint32_t demonID, uint32_t spotID,
      float x, float y, float rot);

  /**
   * Create an enemy (or ally) in the specified zone at set coordinates
   * but do not add it to the zone yet
   * @param zone Pointer to the zone where the entity should be spawned
   * @param demonID Demon/enemy type ID to spawn
   * @param spawnID Optional zone definition spawn ID
   * @param spotID Optional spot ID to add the enemy to
   * @param x X coordinate to render the enemy at
   * @param y Y coordinate to render the enemy at
   * @param rot Rotation to render the enemy with
   * @param responsibleEntity Entity responsible for creating this entity.
   * @return Pointer to the new ActiveEntityState
   */
  std::shared_ptr<ActiveEntityState> CreateEnemy(
      const std::shared_ptr<Zone>& zone, uint32_t demonID, uint32_t spawnID,
      uint32_t spotID, float x, float y, float rot,
      const libobjgen::UUID& responsibleEntity = {});

  /**
   * Copy the current skills and stats from one entity onto an enemy or ally
   * and recalculate their stats. This must be done before adding it to a
   * zone.
   * @param eState Enemy or Ally state to copy to
   * @param copyState Entity state to copy from
   * @return false if an error occurred
   */
  bool CopyToEnemy(const std::shared_ptr<ActiveEntityState>& eState,
                   const std::shared_ptr<ActiveEntityState>& copyState);

  /**
   * Copy the skills and stats from a demon onto an enemy or ally and
   * recalculate their stats. This must be done before adding it to a zone.
   * @param eState Enemy or Ally state to copy to
   * @param copyDemon Demon to copy from
   * @param copyEntityID Optional entity ID to use to mimic boosts that only
   *  affect the entity being copied
   * @return false if an error occurred
   */
  bool CopyDemon(const std::shared_ptr<ActiveEntityState>& eState,
                 const std::shared_ptr<objects::Demon>& copyDemon,
                 int32_t copyEntityID = 0);

  /**
   * Add enemies (or allies) already created in the zone, prepare AI and
   * fire spawn triggers. Spawns can optionally be staggered and/or created
   * as a single encounter that will fire the supplied action source actions
   * when defeated.
   * @param eStates Pointer to enemy or ally states to add to the zone
   * @param zone Pointer to the zone where the entities should be added
   * @param staggerSpawn If true the spawns will be staggered as they appear,
   *  if false they will all appear at once
   * @param asEncounter If true the enemies will be created as an encounter
   *  and the supplied defeatActions will fire when the last one is defeated,
   *  if false they will be normal spawns
   * @param defeatActions List of defeat actions to fire when the enemies
   *  are killed, should the be spawned in as an encounter
   * @return true if the enemies were added successfully, false if an error
   *  occurred
   */
  bool AddEnemiesToZone(
      const std::list<std::shared_ptr<ActiveEntityState>>& eStates,
      const std::shared_ptr<Zone>& zone, bool staggerSpawn, bool asEncounter,
      const std::list<std::shared_ptr<objects::Action>>& defeatActions);

  /**
   * Add enemies (or allies) already created in the zone, prepare AI and
   * fire spawn triggers. Spawns can optionally be staggered and/or created
   * as a single encounter that will fire the supplied action source actions
   * when defeated.
   * @param eStates Pointer to enemy or ally states to add to the zone
   * @param zone Pointer to the zone where the entities should be added
   * @param staggerSpawn If true the spawns will be staggered as they appear,
   *  if false they will all appear at once
   * @param asEncounter If true the enemies will be created as an encounter
   *  and the supplied defeatEventID will fire when the last one is defeated,
   *  if false they will be normal spawns
   * @param defeatEventID Event ID to fire from an action when the encounter
   *  is defeated
   * @return true if the enemies were added successfully, false if an error
   *  occurred
   */
  bool AddEnemiesToZone(std::list<std::shared_ptr<ActiveEntityState>> eStates,
                        const std::shared_ptr<Zone>& zone, bool staggerSpawn,
                        bool asEncounter, const libcomp::String& defeatEventID);

  /**
   * Move the supplied entity from its current position to the supplied
   * x/y coordinates in their current zone. If any collision is hit, the
   * entity will be placed at the collision point. No movement for this
   * entity will be communicated to the client so this is primarily useful
   * in repositioning entities not yet in the zone.
   * @param eState Pointer to the entity to reposition
   * @param x New X coordinate
   * @param y New Y coordinate
   * @return false if the entity hit a collision in the zone or could not
   *  be moved at all
   */
  bool LinearReposition(const std::shared_ptr<ActiveEntityState>& eState,
                        float x, float y);

  /**
   * Update the specified zone's spawns from SpawnLocationGroups or
   * SpawnGroups directly updated at specific spots
   * @param zone Pointer to the zone where the groups should be updated
   * @param refreshAll true if each group should be filled, false if only
   *  spawn groups with an elapsed refresh timer should be updated
   * @param now Current server time
   * @param actionSource Optional action source of the spawn
   * @return true if any enemy was spawned, false if no enemy was spawned
   */
  bool UpdateSpawnGroups(
      const std::shared_ptr<Zone>& zone, bool refreshAll, uint64_t now = 0,
      std::shared_ptr<objects::ActionSpawn> actionSource = nullptr);

  /**
   * Update the specified zone's staggered spawns, showing them to the
   * zone if any are ready
   * @param zone Pointer to the zone
   * @param now Current server time
   * @return true if any enemy was spawned, false if no enemy was spawned
   */
  bool UpdateStaggeredSpawns(const std::shared_ptr<Zone>& zone, uint64_t now);

  /**
   * Update the specified zone's PlasmaSpawns
   * @param zone Pointer to the zone where the plasma should be updated
   * @param now Current server time
   * @return true if any plasma was spawned or despawned
   */
  bool UpdatePlasma(const std::shared_ptr<Zone>& zone, uint64_t now = 0);

  /**
   * Fail the supplied client's current plasma minigame
   * @param client Pointer to the client connection
   * @param plasmaID Entity ID of the plasma currently being accessed
   * @param pointID Optional specifier of the point being accessed. If
   *  not specified, the current one will be found.
   */
  void FailPlasma(const std::shared_ptr<ChannelClientConnection>& client,
                  int32_t plasmaID, int8_t pointID = 0);

  /**
   * Updates the current states of entities in the zone.  Enemy AI is
   * processed from here as well as updating the current state of moving
   * entities.
   */
  void UpdateActiveZoneStates();

  /**
   * Update the state of status effects in the supplied zone, adding
   * and updating existing effects, expiring old effects and applying
   * T-damage and regen to entities with applicable effects
   * @param zone Pointer to the zone to update status effects for
   * @param now System time representing the current server time to use
   *  for event checking
   * @param entities List of pointers to entities to have their status
   *  effects updated
   */
  void UpdateStatusEffectStates(
      const std::shared_ptr<Zone>& zone, uint32_t now,
      const std::list<std::shared_ptr<ActiveEntityState>>& entities);

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
            const std::shared_ptr<ActiveEntityState>& eState, float xPos,
            float yPos, float rot);

  /**
   * Perform time-based actions depending on the current world clock time.
   * @param clock World clock set to the current time
   * @param lastTrigger Last time the zone time triggers were processed
   *  according to the server. Only events between this and the current
   *  clock will be executed.
   */
  void HandleTimedActions(const WorldClock& clock,
                          const WorldClockTime& lastTrigger);

  /**
   * Start the timer currently assigned to the supplied zone instance
   * @param instance Pointer to a zone instance
   * @return false if an error occurred
   */
  bool StartInstanceTimer(const std::shared_ptr<ZoneInstance>& instance);

  /**
   * Extend the timer currently assigned to the supplied zone instance.
   * Only certain variants are valid. If the timer is successfully extended,
   * the already assigned expiration event ID will be used
   * @param instance Pointer to a zone instance
   * @param seconds Number of seconds the timer will be extended by
   * @return false if an error occurred
   */
  bool ExtendInstanceTimer(const std::shared_ptr<ZoneInstance>& instance,
                           uint32_t seconds);

  /**
   * Stop the timer currently assigned to the supplied zone instance
   * @param instance Pointer to a zone instance
   * @param stopTime Explicit stop time to use instead of the current
   *  server time. This is useful for making sure the stop time does
   *  not undershoot the actual expiration
   * @return false if an error occurred
   */
  bool StopInstanceTimer(const std::shared_ptr<ZoneInstance>& instance,
                         uint64_t stopTime = false);

  /**
   * Send the timer currently assigned to the supplied zone instance to
   * one or all clients currenctly connected to it
   * @param instance Pointer to a zone instance
   * @param client Optional pointer to the client that should be sent
   *  the timer information. If not specified the entire zone will be
   *  sent the information.
   * @param queue true if the message should be queued, false if
   *  it should send right away
   * @param extension Optional parameter to include as a one time "extension"
   *  already added to the timer sent for certain packets
   */
  void SendInstanceTimer(
      const std::shared_ptr<ZoneInstance>& instance,
      const std::shared_ptr<ChannelClientConnection>& client = nullptr,
      bool queue = false, uint32_t extension = 0);

  /**
   * Update the death time-out associated to the client and handle
   * scheduling expiration and communication to the players
   * @param state Pointer to state of the entity to time-out
   * @param time Time-out value to assign to the entity. If -1, the
   *  time-out will be cleared. If 0, the current value will be sent.
   * @param client Optional pointer to the client that should be sent
   *  the time-out packets. If not specified the entire zone will be
   *  sent the information.
   */
  void UpdateDeathTimeOut(
      ClientState* state, int32_t time,
      const std::shared_ptr<ChannelClientConnection>& client = nullptr);

  /**
   * Handle instance specific death time-out scheduling and processing
   * @param instance Pointer to a zone instance the entity is in
   * @param client Pointer to the client the death timer is active for
   * @param deathTime If supplied the death time assigned to the client
   *  will be checked and upon matching the client will be scheduled for
   *  instance removal. If not supplied, the death time assigned to the
   *  client will be used to schedule removal by calling this function
   *  again.
   */
  void HandleDeathTimeOut(
      const std::shared_ptr<ZoneInstance>& instance,
      const std::shared_ptr<ChannelClientConnection>& client,
      uint64_t deathTime);

  /**
   * End the timer currently assigned to the supplied zone instance for
   * a specific client and contextually process anything that should occur
   * upon this action happening. Both successes and failures are handled
   * based upon actions like timer stop, timer expiration and zone change
   * while the timer is still active.
   * @param instance Pointer to a zone instance
   * @param client Pointer to the client the timer is ending for
   * @param isSuccess true if the reason the timer is being ended should
   *  be considered a success state
   * @param queue Optional parameter to queue the packets being sent
   */
  void EndInstanceTimer(const std::shared_ptr<ZoneInstance>& instance,
                        const std::shared_ptr<ChannelClientConnection>& client,
                        bool isSuccess, bool queue = false);

  /**
   * Update any zone specific entity tracking/state information and if
   * the zone is marked for tracking and fall back to team tracking only
   * if the whole zone should not be tracked. An example of a tracked
   * zone is a Diaspora instance zone.
   * @param zone Pointer to a zone to track
   * @param team Optional pointer to the team to track if the zone is not
   *  being tracked in itself
   * @return true if the zone (or team) tracking was updated successfully,
   *  false if it did not apply
   */
  bool UpdateTrackedZone(const std::shared_ptr<Zone>& zone,
                         const std::shared_ptr<objects::Team>& team = nullptr);

  /**
   * Update any team specific entity tracking/state information
   * @param team Pointer to the team being tracked
   * @param zone Optional pointer to the only zone the tracking information
   *  should be updated for
   * @return true if the team tracking was updated successfully, false if
   *  it did not apply
   */
  bool UpdateTrackedTeam(const std::shared_ptr<objects::Team>& team,
                         const std::shared_ptr<Zone>& zone = nullptr);

  /**
   * Update the loot assigned to the destiny box assigned to a specific
   * world CID
   * @param instance Pointer to the instance the box belongs to
   * @param worldCID World CID that has access to the box
   * @param add List of pointers to loot to add
   * @param remove Optional set of slots to clear before adding loot
   * @return true if the updates occurred, false if they did not
   */
  bool UpdateDestinyBox(const std::shared_ptr<ZoneInstance>& instance,
                        int32_t worldCID,
                        const std::list<std::shared_ptr<objects::Loot>>& add,
                        std::set<uint8_t> remove = {});

  /**
   * Send the current state of the destiny box a client has access to
   * @param client Pointer to the client connection
   * @param eventMenu true if the box is being viewed via the lotto event
   *  menu, false if it is being viewed normally
   * @param queue If true the packet will be queued instead of sent directly
   */
  void SendDestinyBox(const std::shared_ptr<ChannelClientConnection>& client,
                      bool eventMenu, bool queue = false);

  /**
   * Perform any updates related to multi-zone bosses being killed
   * @param zone Pointer to the zone the bosses were killed in
   * @param sourceState Pointer to the state of the player that killed the
   *  bosses
   * @param types List of enemy types for the bosses that
   */
  void MultiZoneBossKilled(const std::shared_ptr<Zone>& zone,
                           ClientState* sourceState,
                           const std::list<uint32_t>& types);

  /**
   * Trigger specific zone actions for the specified zone for a set of
   * source entities
   * @param zone Pointer to a zone
   * @param entities List of pointers to entities that will be used as
   *  source entities for each action
   * @param trigger Type of trigger to execute
   * @param client Pointer to the client to use for the executing action
   *  contexts
   * @return true if at least one action trigger was fired, false if no
   *  action triggers were fired
   */
  bool TriggerZoneActions(
      const std::shared_ptr<Zone>& zone,
      std::list<std::shared_ptr<ActiveEntityState>> entities,
      ZoneTrigger_t trigger,
      const std::shared_ptr<ChannelClientConnection>& client = nullptr);

  /**
   * Gather zone specific triggers
   * @param zone Pointer to a zone
   * @param trigger Type of trigger to retrieve
   * @return List of pointers to the matching zone triggers
   */
  std::list<std::shared_ptr<objects::ServerZoneTrigger>> GetZoneTriggers(
      const std::shared_ptr<Zone>& zone, ZoneTrigger_t trigger);

  /**
   * Handle zone specific actions from triggers for either the entire zone
   * or one specific entity
   * @param zone Pointer to a zone
   * @param triggers List of pointers to triggers from the zone
   * @param entity Optional pointer to the entity to trigger the actions for.
   *  If not specified the entire zone will be used (with no source).
   * @param client Optional pointer to the entity's client
   * @return true if at least one action trigger was fired, false if no
   *  action triggers were fired
   */
  bool HandleZoneTriggers(
      const std::shared_ptr<Zone>& zone,
      const std::list<std::shared_ptr<objects::ServerZoneTrigger>>& triggers,
      const std::shared_ptr<ActiveEntityState>& entity = nullptr,
      const std::shared_ptr<ChannelClientConnection>& client = nullptr);

  /**
   * Start an event contextual to the supplied zone with no source entity
   * @param zone Pointer to the zone to start the event for
   * @param eventID ID of the event to start
   * @return true if the event started successfully, false if it did not
   */
  bool StartZoneEvent(const std::shared_ptr<Zone>& zone,
                      const libcomp::String& eventID);

  /**
   * Update zone geometry information bound to a server object in the
   * supplied zone
   * @param zone Pointer to a zone
   * @param elemObject Pointer to a server object that may be bound to
   *  zone geometry
   * @return true if the geometry state was updated, false if it was not
   */
  bool UpdateGeometryElement(const std::shared_ptr<Zone>& zone,
                             std::shared_ptr<objects::ServerObject> elemObject);

  /**
   * Get the X/Y coordinates and rotation of the center point of a spot.
   * @param dynamicMapID Dynamic map ID of the zone containing the spot
   * @param spotID Spot ID to find the center of
   * @param x Default X position to use if the spot is not found, changes
   *  to the spot center X coordinate if found
   * @param y Default Y position to use if the spot is not found, changes
   *  to the spot center Y coordinate if found
   * @param rot Default rotation to use if the spot is not found, changes
   *  to the spot rotation if found
   * @return true if the spot was found, false it was not
   */
  bool GetSpotPosition(uint32_t dynamicMapID, uint32_t spotID, float& x,
                       float& y, float& rot) const;

  /**
   * Get a random point within the specified width and height representing
   * a rectangular area in a zone.
   * @param width Width of the area to generate a random point within
   * @param height Height of the area to generate a random point within
   * @return X, Y coordinates of the random point
   */
  Point GetRandomPoint(float width, float height) const;

  /**
   * Get a random point within the supplied zone spot.
   * @param spot Pointer to the spot to get a random point within
   * @param zoneData Optional pointer to the zone definition that, when
   *  supplied, will force the random point chosen to be in the proper
   *  zone boundaries
   * @return X, Y coordinates of the random point
   */
  Point GetRandomSpotPoint(
      const std::shared_ptr<objects::MiSpotData>& spot,
      const std::shared_ptr<objects::MiZoneData>& zoneData = nullptr);

  /**
   * Get a random rotation value (in radians)
   * @return Rotation value (in radians)
   */
  static float GetRandomRotation();

  /**
   * Get a point directly away or directly towards two specified points.
   * @param sourceX Point 1 X coordinate
   * @param sourceY Point 1 Y coordinate
   * @param targetX Point 2 X coordinate
   * @param targetY Point 2 Y coordinate
   * @param distance Distance to move
   * @param away true if the point calculated should be futher from the
   *  target point, false if it should be closer to it
   * @param zone Optional pointer to the zone to check against collision
   * @return X, Y coordinates at the specified point not adjusted for
   *  collisions
   */
  Point GetLinearPoint(float sourceX, float sourceY, float targetX,
                       float targetY, float distance, bool away,
                       const std::shared_ptr<Zone>& zone = nullptr);

  /**
   * Set an entity's destination position at a distace directly away or
   * directly towards the specified point. Communicating that the move
   * has taken place must be done elsewhere.
   * @param eState Pointer to the entity state
   * @param targetX X coordinate to move relative to
   * @param targetY Y coordinate to move relative to
   * @param distance Distance to move
   * @param away true if the entity should move away from the point,
   *  false if it should move toward it
   * @param now Server time to use as the origin ticks
   * @param endTime Server time to use as the destination ticks
   * @return Point the entity will move to
   */
  Point MoveRelative(const std::shared_ptr<ActiveEntityState>& eState,
                     float targetX, float targetY, float distance, bool away,
                     uint64_t now, uint64_t endTime);

  /**
   * Correct a client supplied entity position to ensure it does not move
   * out of bounds. No communication to the client takes place within.
   * @param eState Pointer to the entity state
   * @param dest Destination point
   * @return true if the destination was corrected and should be communicated
   *  to the client
   */
  bool CorrectClientPosition(const std::shared_ptr<ActiveEntityState>& eState,
                             Point& dest);

  /**
   * Correct a client supplied entity position to ensure it does not move
   * out of bounds. No communication to the client takes place within.
   * @param eState Pointer to the entity state
   * @param src Source movement point (ignored if not moving)
   * @param dest Destination point
   * @param startTime Server time representing when the position is starting
   *  to change (ignore if not moving)
   * @param stopTime Server time representing when the position is ending
   * @param isMove Designates if the entity is moving or not
   * @return 0 if no change of note took place, 1 if the movement was rolled
   *  back, 2 if only the end point was corrected
   */
  uint8_t CorrectClientPosition(
      const std::shared_ptr<ActiveEntityState>& eState, Point& src, Point& dest,
      ServerTime& startTime, ServerTime& stopTime, bool isMove = false);

  /**
   * Adjust a destination point based on a source point and the point where
   * it collides with the nearest zone geometry point
   * @param src Source point
   * @param collidePoint Point where the path collides with geometry
   * @param collideLine Line the path collides with geometry
   * @return Adjusted linear destination point
   */
  Point CollisionAdjust(const Point& src, const Point& collidePoint,
                        const Line& collideLine);

  /**
   * Calculate the shortest path between the supplied source and destination
   * points for the specified zone
   * @param zone Pointer to the zone
   * @param source Source point
   * @param dest Destination point
   * @param maxDistance Optional max distance for the path
   * @return List of the full shortest path (starting with the source and
   *  ending with the destination) or empty if no path is possible
   */
  std::list<Point> GetShortestPath(const std::shared_ptr<Zone>& zone,
                                   const Point& source, const Point& dest,
                                   float maxDistance = 0.f);

  /**
   * Determine the shortest distance from a point to a line segment
   * @param line Line segment to measure distance to
   * @param point Point to measure distance from
   * @return Shortest distance between the point and line
   */
  static float GetPointToLineDistance(const Line& line, const Point& point);

  /**
   * Determine the point on a line segment that is closest to another
   * supplied point
   * @param line Line segment to get the nearest point from
   * @param point Point to get the nearest point for
   * @return Point on the line segment that is nearest
   */
  static Point GetNearestPoint(const Line& line, const Point& point);

  /**
   * Determine if the specified point is within a polygon defined by vertices
   * @param p Point to check
   * @param vertices List of points representing a polygon's vertices
   * @param overlapRadius Optional modifier to handle the point as a circle
   *  using this value as the radius and checking if it overlaps anywhere
   * @return true if the point is within the polygon, false if it is not
   */
  static bool PointInPolygon(const Point& p, const std::list<Point> vertices,
                             float overlapRadius = 0.f);

  /**
   * Filter the list of supplied entities to only those visible in the
   * specified field of view
   * @param entities List of entities to filter to visible only
   * @param x X coordinate of the FoV origin
   * @param y Y coordinate of the FoV origin
   * @param rot Rotation in radians for the center of the FoV
   * @param maxAngle Maximum angle in radians for either side of the FoV
   * @param useHitbox If true, the entities' hitboxes will be used to
   *  determine if they are in the FoV, even if the center point is not
   * @return Filtered list of entities visible
   */
  static std::list<std::shared_ptr<ActiveEntityState>> GetEntitiesInFoV(
      const std::list<std::shared_ptr<ActiveEntityState>>& entities, float x,
      float y, float rot, float maxAngle, bool useHitbox = false);

  /**
   * Rotate a point around an origin point by the specified radians amount
   * @param p Point to rotate
   * @param origin Origin point to rotate around
   * @param radians Number of radians to rotate around the origin
   * @return Transformed rotation point
   */
  static Point RotatePoint(const Point& p, const Point& origin, float radians);

  /**
   * Sync updates and removals to InstanceAccess definitions that belong
   * to this server or any other one
   * @param updates List of updated InstanceAccess definitions
   * @param removes List of removed InstanceAccess definitions
   */
  void SyncInstanceAccess(
      std::list<std::shared_ptr<objects::InstanceAccess>> updates,
      std::list<std::shared_ptr<objects::InstanceAccess>> removes);

 private:
  /**
   * Select a spot for a spawn group and get it's location.
   * @param useSpotID If the spot ID should be used.
   * @param spotID Selected spot ID.
   * @param spot Selected spot.
   * @param location Location of the selected spot.
   * @param dynamicMap Dynamic map data for the zone.
   * @param zoneDef Definition for the zone.
   * @param locations Spawn locations.
   * @returns true if the spot was found; false on error. Upon error the
   *  spotID is NOT cleared so it can be used for error reporting.
   * @note This is called by @ref UpdateSpawnGroups.
   */
  bool SelectSpotAndLocation(
      bool useSpotID, uint32_t& spotID, const std::set<uint32_t>& spotIDs,
      std::shared_ptr<channel::ZoneSpotShape>& spot,
      std::shared_ptr<objects::SpawnLocation>& location,
      std::shared_ptr<DynamicMap>& dynamicMap,
      std::shared_ptr<objects::ServerZone>& zoneDef,
      std::list<std::shared_ptr<objects::SpawnLocation>>& locations);

  /**
   * Move the supplied client from the current channel to another based upon
   * the zone being moved to. This will cause the client to log out upon
   * success.
   * @param client Pointer to the client connection
   * @param zoneID ID of the zone being entered
   * @param dynamicMapID ID of the dynamic map being entered
   * @param toInstance If supplied, the zone and dynamic map belong to an
   *  instance and the access should be used
   * @param x X coordinate to use in the new zone, ignored for instances
   * @param y Y coordinate to use in the new zone, ignored for instances
   * @param rot Rotation to use in the new zone (in radians), ignored for
   *  instances
   * @return true if the client was moved, false if they either didn't
   *  need to move or the move failed
   */
  bool MoveToZoneChannel(
      const std::shared_ptr<ChannelClientConnection>& client, uint32_t zoneID,
      uint32_t dynamicMapID,
      const std::shared_ptr<objects::InstanceAccess>& toInstance, float x = 0.f,
      float y = 0.f, float rot = 0.f);

  /**
   * Calculate the shortest path between the supplied source and destination
   * Qmp points in the same zone geometry. Path optimization between points
   * is up to the caller to perform.
   * @param geometry Pointer to a zone geometry definition
   * @param sourceID Source point ID
   * @param destID Destination point ID
   * @return List of the shortest path of points to move to in order
   */
  std::list<uint32_t> GetShortestPath(
      const std::shared_ptr<ZoneGeometry>& geometry, uint32_t sourceID,
      uint32_t destID);

  /**
   * Create an enemy (or ally) in the specified zone at set coordinates
   * but do not add it to the zone yet
   * @param zone Pointer to the zone where the entity should be spawned
   * @param demonID Demon/enemy type ID to spawn
   * @param spawn Optional pointer to the entity's spawn definition
   * @param x X coordinate to render the enemy at
   * @param y Y coordinate to render the enemy at
   * @param rot Rotation to render the enemy with
   * @param asAlly Optional override to create an ally regardless of
   *  spawn category, defaults to false
   * @param responsibleEntity Entity responsible for creating this entity.
   * @return Pointer to the new ActiveEntityState
   */
  std::shared_ptr<ActiveEntityState> CreateEnemy(
      const std::shared_ptr<Zone>& zone, uint32_t demonID,
      const std::shared_ptr<objects::Spawn>& spawn, float x, float y, float rot,
      bool asAlly = false, const libobjgen::UUID& responsibleEntity = {});

  /**
   * Send entity information about an enemy in a zone
   * @param enemyState Enemy state to use to report enemy data to the clients
   * @param client Pointer to the client connection to send to if specified
   *  instead of the whole zone
   * @param zone Pointer to the zone where the enemy exists
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void SendEnemyData(const std::shared_ptr<EnemyState>& enemyState,
                     const std::shared_ptr<ChannelClientConnection>& client,
                     const std::shared_ptr<Zone>& zone, bool queue = false);

  /**
   * Send entity information about an ally in a zone
   * @param allyState Ally state to use to report ally data to the clients
   * @param client Pointer to the client connection to send to if specified
   *  instead of the whole zone
   * @param zone Pointer to the zone where the ally exists
   * @param queue true if the message should be queued, false if
   *  it should send right away
   */
  void SendAllyData(const std::shared_ptr<AllyState>& allyState,
                    const std::shared_ptr<ChannelClientConnection>& client,
                    const std::shared_ptr<Zone>& zone, bool queue = false);

  /**
   * Schedule instance access time-out and remove the instance if no
   * player accesses it before then
   * @param instance Pointer to the zone instance to update
   */
  void ScheduleInstanceAccessTimeOut(
      const std::shared_ptr<ZoneInstance>& instance);

  /**
   * Schedule the expiration of a zone instance timer
   * @param instance Pointer to the zone instance with a timer set
   */
  void ScheduleTimerExpiration(const std::shared_ptr<ZoneInstance>& instance);

  /**
   * Determine if the supplied boss EnemyState is valid to add to a
   * multi-zone boss group
   * @param enemyState Pointer to the boss EnemyState
   * @return true if the boss can be added, false if it cannot
   */
  bool ValidateBossGroup(const std::shared_ptr<EnemyState>& enemyState);

  /**
   * Send the current multi-zone boss statuses in the supplied group
   * @param groupID Multi-zone boss group to update
   */
  void SendMultiZoneBossStatus(uint32_t groupID);

  /**
   * Perform all necessary despawns for the supplied zone.
   * @param zone Pointer to zone do perform despawns for
   */
  void HandleDespawns(const std::shared_ptr<Zone>& zone);

  /**
   * Update the state of status effects in the supplied zone, adding
   * and updating existing effects, expiring old effects and applying
   * T-damage and regen to entities with applicable effects
   * @param zone Pointer to the zone to update status effects for
   * @param now System time representing the current server time to use
   *  for event checking
   */
  void UpdateStatusEffectStates(const std::shared_ptr<Zone>& zone,
                                uint32_t now);

  /**
   * Handle all instance specific zone population actions such as entity
   * hiding and timer updating
   * @param client Pointer to the client connection entering the zone
   * @param zone Pointer to the zone the client is entering
   */
  void HandleSpecialInstancePopulate(
      const std::shared_ptr<ChannelClientConnection>& client,
      const std::shared_ptr<Zone>& zone);

  /**
   * Get a zone by zone definition ID. If the zone does not exist and is
   * not a global zone, it will be created.
   * @param zoneID Zone definition ID to get a zone for
   * @param dynamicMapID Dynamic Map ID of the zone to to get a zone for
   * @param client Pointer to the client connection to use to decide whether
   *  a new zone should be created if the zone is private
   * @param currentInstanceID Optional instance ID of the zone being moved from
   * @return Pointer to the new or existing zone
   */
  std::shared_ptr<Zone> GetZone(
      uint32_t zoneID, uint32_t dynamicMapID,
      const std::shared_ptr<ChannelClientConnection>& client,
      uint32_t currentInstanceID = 0);

  /**
   * Create a new zone based off of the supplied definition
   * @param definition Pointer to a zone definition
   * @param instance Optional pointer to the instance the zone will belong to
   * @return Pointer to a new zone
   */
  std::shared_ptr<Zone> CreateZone(
      const std::shared_ptr<objects::ServerZone>& definition,
      const std::shared_ptr<ZoneInstance>& instance = nullptr);

  /**
   * All all PvP bases defined in a zone from the variant
   * @param zone Pointer to the zone
   * @param variant Pointer to the PvP instance variant
   */
  void AddPvPBases(const std::shared_ptr<Zone>& zone,
                   const std::shared_ptr<objects::PvPInstanceVariant>& variant);

  /**
   * All all Diaspora bases defined in a zone from the variant
   * @param zone Pointer to the zone
   */
  void AddDiasporaBases(const std::shared_ptr<Zone>& zone);

  /**
   * Determine if the suppplied client has access to enter a restricted
   * zone
   * @param client Pointer to the client attempting to enter the zone
   * @param zone Pointer to the restricted zone
   */
  bool CanEnterRestrictedZone(
      const std::shared_ptr<ChannelClientConnection>& client,
      const std::shared_ptr<Zone>& zone);

  /**
   * Send InstanceAccess create or join messages to either one player or
   * all players that have access to it
   * @param access Pointer to the InstanceAccess
   * @param joined If true the join message will be sent, if false the
   *  create message will be sent
   * @param client Pointer to a client connection that has access to the
   *  instance. If null all players that have access to the instance will
   *  be sent the message instead
   */
  void SendAccessMessage(
      const std::shared_ptr<objects::InstanceAccess>& access, bool joined,
      const std::shared_ptr<ChannelClientConnection>& client = nullptr);

  /**
   * Remove and clean up all zone information for a specific zone
   * @param zone Pointer to the zone
   * @param freezeOnly If true, the zone will only be frozen and removed
   *  from all affected cache data members, if false it will be fully
   *  removed
   */
  void RemoveZone(const std::shared_ptr<Zone>& zone, bool freezeOnly);

  /**
   * Remove a zone instance by unique ID if no characters have access
   * and no characters are currently in any zone.
   * @param instanceID Unique ID of the instance to remove
   * @return true if the instance was removed, false it was not
   */
  bool RemoveInstance(uint32_t instanceID);

  /**
   * Determine if the server object supplied is in a state that would
   * disabled geometry collision.
   * @param obj Pointer to a server object
   * @return true if the object is in a collision disabled state
   */
  bool IsGeometryDisabled(const std::shared_ptr<objects::ServerObject>& obj);

  /**
   * Register time restricted actions for a zone with the manager
   * such as time conditional spawn groups or plasma. Times registered
   * with the manager are also registered with the server.
   * @param zone Pointer to the zone to register
   * @param definition Pointer to the zone definition
   * @return true if any times were registered, false if no times
   *  were registered
   */
  bool RegisterTimeRestrictions(
      const std::shared_ptr<Zone>& zone,
      const std::shared_ptr<objects::ServerZone>& definition);

  /**
   * Gather all world clock representations of the supplied time triggers
   * @param triggers List of pointers to time triggers
   * @return List of world clock representations
   */
  std::list<WorldClockTime> GetTriggerTimes(
      const std::list<std::shared_ptr<objects::ServerZoneTrigger>>& triggers);

  /// Map of zones by unique ID
  std::unordered_map<uint32_t, std::shared_ptr<Zone>> mZones;

  /// Map of global zone definition IDs to map of zone unique IDs by
  /// dynamic map ID
  std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>>
      mGlobalZoneMap;

  /// Map of zone instances by unique ID
  std::unordered_map<uint32_t, std::shared_ptr<ZoneInstance>> mZoneInstances;

  /// Map of world CIDs to the zone instance access definitions. The
  /// moment a character enters the zone, they are removed from this map
  /// as they are not able to intentionally re-enter
  std::unordered_map<int32_t, std::shared_ptr<objects::InstanceAccess>>
      mZoneInstanceAccess;

  /// Map of world CIDs to zone unique IDs
  std::unordered_map<int32_t, uint32_t> mEntityMap;

  /// Map of QMP filenames to the geometry structures built from them
  std::unordered_map<std::string, std::shared_ptr<ZoneGeometry>> mZoneGeometry;

  /// Map of dynamic map IDs to geometry information built from their
  /// corresponding binary definitions
  std::unordered_map<uint32_t, std::shared_ptr<DynamicMap>> mDynamicMaps;

  /// Map of global boss group IDs to zones in that group on the server
  std::unordered_map<uint32_t, std::set<uint32_t>> mGlobalBossZones;

  /// Set of all zones that should be considered active when
  /// updating states. When a zone is removed from this set but
  /// not removed entirely, its AI etc will be frozen until a player
  /// enters the zone again.
  std::set<uint32_t> mActiveZones;

  /// Set of IDs for all zones that are both active and configured for
  /// team, full zone or boss tracking
  std::set<uint32_t> mActiveTrackedZones;

  /// Set of all zone IDs, active or not, that have had a time restricted
  /// update performed on them. This can be thought of as temporarily
  /// "unfreezing" a zone if it is not currently active. Once the zone's
  /// updates have been performed, it is removed from this set.
  std::set<uint32_t> mTimeRestrictUpdatedZones;

  /// Set of all zone IDs with at least one time restricted action bound
  /// to them
  std::set<uint32_t> mAllTimeRestrictZones;

  /// Map of times bound to zone IDs to register with the world for world
  /// time triggering
  std::map<WorldClockTime, std::set<uint32_t>> mSpawnTimeRestrictZones;

  /// List of zone-detached global time triggers that fire independent of
  /// any zone
  std::list<std::shared_ptr<objects::ServerZoneTrigger>> mGlobalTimeTriggers;

  /// Next server time that tracked zones will be refreshed during
  ServerTime mTrackingRefresh;

  /// Next available zone unique ID
  uint32_t mNextZoneID;

  /// Next available zone instance unique ID
  uint32_t mNextZoneInstanceID;

  /// Server lock for shared resources
  libcomp::Mutex mLock;

  /// Server lock for creating or getting existing zones in an instance
  libcomp::Mutex mInstanceZoneLock;

  /// Pointer to the channel server
  std::weak_ptr<ChannelServer> mServer;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_ZONEMANAGER_H
