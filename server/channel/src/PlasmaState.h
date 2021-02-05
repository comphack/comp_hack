/**
 * @file server/channel/src/PlasmaState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a plasma spawn on the channel.
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

#ifndef SERVER_CHANNEL_SRC_PLASMASTATE_H
#define SERVER_CHANNEL_SRC_PLASMASTATE_H

// objects Includes
#include <EntityState.h>
#include <PlasmaSpawn.h>

namespace libcomp {
class Packet;
}

namespace objects {
class LootBox;
}

namespace channel {

/**
 * Represents a specific point within a plasma state.
 */
class PlasmaPoint : public objects::ServerObjectBase {
  friend class PlasmaState;

 public:
  /**
   * Create a plasma point.
   */
  PlasmaPoint();

  /**
   * Reset the point to its default state for re-use
   */
  void Refresh();

  /**
   * Get a calculated state value per looter, taking into account
   * if the point is hidden or has been opened
   * @param looterID Optional ID of a looter to check against any
   *  existing looter
   * @return Calcualted state value
   */
  int32_t GetState(int32_t looterID = -1);

  /**
   * Get loot associated to an open point
   * @return Loot associated to the point
   */
  std::shared_ptr<objects::LootBox> GetLoot() const;

 private:
  /// Pointer to loot generated for an open point
  std::shared_ptr<objects::LootBox> mLoot;

  /// ID of the character looting (or attemping to loot) the
  /// point. Once set, no one else can loot the point.
  int32_t mLooterID;

  /// Server time specifying when an open point should be hidden
  /// and queued for respawn
  uint64_t mHideTime;

  /// true if point is currently hidden, false if it is visible
  bool mHidden;

  /// true if the point is open, false if it has not been opened
  bool mOpen;
};

/**
 * Contains the state of an plasma spawn related to a channel.
 */
class PlasmaState : public EntityState<objects::PlasmaSpawn> {
 public:
  /**
   * Create a plasma state based off of a server definition.
   * @param plasma Pointer to the server definition
   */
  PlasmaState(const std::shared_ptr<objects::PlasmaSpawn>& plasma);

  /**
   * Clean up the plasma state.
   */
  virtual ~PlasmaState() {}

  /**
   * Create plasma points corresponding to each one required by
   * the definition
   * @return true if the points were created successfully
   */
  bool CreatePoints();

  /**
   * Get a plasma point corresponding to the point ID
   * @param pointID ID of the point to retrieve
   * @return Pointer to the plasma point
   */
  std::shared_ptr<PlasmaPoint> GetPoint(uint32_t pointID);

  /**
   * Get all active plasma points
   * @return List of all active plasma points
   */
  std::list<std::shared_ptr<PlasmaPoint>> GetActivePoints();

  /**
   * Determine if the plasma is marked as active or any points are
   * not currently hidden.
   * @param visible If true, the plasma is only considered active if
   *  a single point is not currently hidden. If false, the plasma is
   *  only considered active if its deactivated parameter is not set.
   * @return true if active or points are not hidden, false otherwise
   */
  bool IsActive(bool visible = false);

  /**
   * Enable/disable (or activate/deactivate) the plasma set. Enabling plasma
   * causes it to appear in the zone on next server tick. Disabling plasma
   * causes it to despawn on next server tick. Deactivated plasma will
   * neither spawn nor despawn until it is activated again. For most non-time
   * trigger processes, modifying if the plasma is active is the normal
   * use case.
   * @param enable If true the points will be enabled, if false they
   *  will be disabled and de-pop if active
   * @param activation If true the points will be activated or deactivated
   *  instead of enabled or disabled. Defaults to false.
   */
  void Toggle(bool enable, bool activation = false);

  /**
   * Check if there is plasma pending a hide or respawn update
   * @param respawn true if points pending respawn should be
   *  retrieved, false if points pending hiding should be
   *  retrieved
   * @param now Current server time
   * @return true if a point in the specified state exists
   */
  bool HasStateChangePoints(bool respawn, uint64_t now = 0);

  /**
   * Get a list of plasma points that have respawned
   * and prepare them to be shown within the state
   * @param now Current server time
   * @return List of plasma points that have respawned
   */
  std::list<std::shared_ptr<PlasmaPoint>> PopRespawnPoints(uint64_t now = 0);

  /**
   * Get a list of plasma points that have become hidden
   * and prepare them to be hidden within the state
   * @param now Current server time
   * @return List of plasma points that have become hidden
   */
  std::list<std::shared_ptr<PlasmaPoint>> PopHidePoints(uint64_t now = 0);

  /**
   * Claim a point for a specific character and begin
   * "picking" plasma. This is responsible for locking
   * players out if they attempt to access the point after
   * another player has already claimed it
   * @param point Pointer to PlasmaPoint to pick
   * @param looterID ID of the character attempting to loot
   *  the point
   * @return True if the looter can pick it,  false if they cannot
   */
  bool PickPoint(std::shared_ptr<PlasmaPoint> point, int32_t looterID);

  /**
   * Update a plasma point with the result of the picking
   * minigame
   * @param pointID ID of the point to update
   * @param looterID ID of the character who is updating the
   *  point. If this does not match the looter on the point
   *  already, nothing will be changed
   * @param result Client supplied result of the minigame
   *  containing a positive value for a success or a negative
   *  for a failure. This represents the distance from the
   *  "goal" in the minigame with a negative being outside
   *  of the success area
   * @return Pointer to the point if the update was valid,
   *  null if it was not
   */
  std::shared_ptr<PlasmaPoint> SetPickResult(uint32_t pointID, int32_t looterID,
                                             int8_t result);

  /**
   * Hide the supplied plasma point  if the point's loot box is empty.
   * @param point Pointer to the plasma point to hide
   * @return true if the point was empty and was marked as hidden,
   *  false if it was not empty
   */
  bool HideIfEmpty(const std::shared_ptr<PlasmaPoint>& point);

  /**
   * Set the loot associated to a specific point
   * @param pointID ID of the point to update
   * @param looterID ID of the character who is updating the
   *  point. If this does not match the looter on the point
   *  already, nothing will be changed
   * @param loot Pointer to the generated loot for the point
   * @return true if the update was completed, otherwise false
   */
  bool SetLoot(uint32_t pointID, int32_t looterID,
               const std::shared_ptr<objects::LootBox>& loot);

  /**
   * Write data to the supplied packet containing a specific
   * point's status
   * @param p Packet to write to
   * @param pointID ID of the point to describe
   * @param looterID Optional parameter to retrive the character
   *  relative point state
   */
  void GetPointStatusData(libcomp::Packet& p, uint32_t pointID,
                          int32_t looterID = -1);

  /**
   * Write data to the supplied packet containing specific
   * points' statuses
   * @param p Packet to write to
   * @param pointIDs IDs of the points to describe
   * @param looterID Optional parameter to retrive the character
   *  relative point state
   */
  void GetPointStatusData(libcomp::Packet& p, std::list<uint32_t> pointIDs,
                          int32_t looterID = -1);

 private:
  /**
   * Hide a point and set it up for respawn
   * @param point Pointer to the plasma point to hide
   */
  void HidePoint(const std::shared_ptr<PlasmaPoint>& point);

  /// Map of points by definition ID
  std::unordered_map<uint32_t, std::shared_ptr<PlasmaPoint>> mPoints;

  /// Map of point IDs to server times when that point should
  /// be respawned
  std::unordered_map<uint32_t, uint64_t> mPointRespawns;

  /// Map of point IDs to server times when that point should be
  /// hidden to respawn later
  std::unordered_map<uint32_t, uint64_t> mPointHides;

  /// Indicates if the plasma set is deactivated and will not re-enable
  /// until it has been activated again
  bool mDeactivated;

  /// Indicates if the plasma set is disabled and no points will spawn
  bool mDisabled;

  /// Server lock for shared resources
  std::mutex mLock;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_PLASMASTATE_H
