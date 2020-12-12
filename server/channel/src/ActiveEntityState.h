/**
 * @file server/channel/src/ActiveEntityState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an active entity on the channel.
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

#ifndef SERVER_CHANNEL_SRC_ACTIVEENTITYSTATE_H
#define SERVER_CHANNEL_SRC_ACTIVEENTITYSTATE_H

// libcomp Includes
#include <EnumMap.h>

// objects Includes
#include <ActiveEntityStateObject.h>
#include <EntityStats.h>
#include <MiCorrectTbl.h>
#include <StatusEffect.h>
#include <TokuseiCondition.h>

// Standard C++11 includes
#include <map>

/// Effect cancelled upon logout
const uint8_t EFFECT_CANCEL_LOGOUT = 0x01;

/// Effect cancelled upon leaving a zone
const uint8_t EFFECT_CANCEL_ZONEOUT = 0x04;

/// Effect cancelled upon death
const uint8_t EFFECT_CANCEL_DEATH = 0x08;

/// Effect cancelled upon being hit
const uint8_t EFFECT_CANCEL_HIT = 0x10;

/// Effect cancelled upon receiving any damage
const uint8_t EFFECT_CANCEL_DAMAGE = 0x20;

/// Effect cancelled upon being knocked back
const uint8_t EFFECT_CANCEL_KNOCKBACK = 0x40;

/// Effect cancelled upon performing a skill
const uint8_t EFFECT_CANCEL_SKILL = 0x80;

/// Recalculation resulted in a locally visible stat change
const uint8_t ENTITY_CALC_STAT_LOCAL = 0x01;

/// Recalculation resulted in a stat change visible to the world
const uint8_t ENTITY_CALC_STAT_WORLD = 0x02;

/// Recalculation resulted in a modified movement speed
const uint8_t ENTITY_CALC_MOVE_SPEED = 0x04;

/// Recalculation resulted in a modified skill set (characters only)
const uint8_t ENTITY_CALC_SKILL = 0x08;

/// Entity is immobile for no specific reason
const uint8_t STATUS_IMMOBILE = 0x01;

/// Entity is mid-hit stun
const uint8_t STATUS_HIT_STUN = 0x02;

/// Entity is being knocked back
const uint8_t STATUS_KNOCKBACK = 0x04;

/// Entity is charging a skill
const uint8_t STATUS_CHARGING = 0x08;

/// Entity is using the rest command or will not attempt to act (AI only)
const uint8_t STATUS_RESTING = 0x10;

/// Entity is still affected by a skill's lockout time
const uint8_t STATUS_LOCKOUT = 0x20;

/// Entity is waiting (used by AI controlled entity)
const uint8_t STATUS_WAITING = 0x40;

/// Entity is ignoring other entities and will not be seen by searching AI
const uint8_t STATUS_IGNORE = 0x80;

namespace libcomp {
class DefinitionManager;
}

namespace objects {
class EnemyBase;
class MiSkillData;
class MiStatusData;
}  // namespace objects

typedef objects::MiCorrectTbl::ID_t CorrectTbl;
typedef objects::ActiveEntityStateObject::DisplayState_t ActiveDisplayState_t;

namespace channel {

class AIState;
class Zone;

/**
 * Represents a request to add or remove a status effect with any applicable
 * modifications to the normal definition.
 */
class StatusEffectChange {
 public:
  /**
   * Create a new StatusEffectChange.
   */
  StatusEffectChange()
      : Type(0), Stack(0), IsReplace(false), IsConstant(false), Duration(0) {}

  /**
   * Create a new StatusEffectChange with supplied options.
   * @param type Type of status effect
   * @param stack Stack size of the statuse effect
   * @param isReplace Indicates if the status effect params should replace
   *  the effect if it already exists
   */
  StatusEffectChange(uint32_t type, int8_t stack, bool isReplace)
      : Type(type),
        Stack(stack),
        IsReplace(isReplace),
        IsConstant(false),
        Duration(0) {}

  /// Status effect type
  uint32_t Type;

  /// Stack size of the effect. If this is not a replace, the stack can be
  /// negative to reduce the stack.
  int8_t Stack;

  /// Indicates if the status effect params should replace the effect if it
  /// already exists
  bool IsReplace;

  /// Indicates if the status effect is a constant effect that is granted
  /// from a renewing source like a tokusei
  bool IsConstant;

  /// Explicit duration (in MS) to be used for the status effect. Overrides
  /// any default values and allows NONE (constant) status effects to be
  /// added with expirations.
  uint32_t Duration;
};

typedef std::unordered_map<uint32_t, StatusEffectChange> StatusEffectChanges;

/**
 * Represents an active entity on the channel server. An entity is
 * active if it can move or perform actions independent of other entities.
 * Active entities have stats and status effects in addition to the usual
 * current zone position shared with non-active entities.
 */
class ActiveEntityState : public objects::ActiveEntityStateObject {
 public:
  /**
   * Create a new active entity state
   */
  ActiveEntityState();

  /**
   * Get a label string formatted with the entity type, ID and demon type
   * when applicable. Used for logging.
   * @return Entity label string
   */
  libcomp::String GetEntityLabel() const;

  /**
   * Get the adjusted correct table value associated to the entity.
   * @param tableID ID of the correct table value to retrieve
   * @param calcState Override CalculatedEntityState to use instead of
   *  the entity's default
   * @return Adjusted correct table value
   */
  int16_t GetCorrectValue(
      CorrectTbl tableID,
      std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr);

  /**
   * Recalculate the entity's stats, adjusted by equipment and
   * effects.
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how effects and items interact with the entity
   * @param calcState Override CalculatedEntityState to use instead of the
   *  entity's default. If a value is supplied for this, the entity's stats
   *  will not be updated as a result of the calculation, instead the
   *  supplied state will be updated to simulate "effective stats".
   * @param contextSkill Optional contextual skill to apply adjustments from.
   *  Essentially adjusts stats "while a skill is being used" which can
   *  vary between scenarios.
   * @return Flags indicating if the calculation resulted in a change that
   *  should be communicated to the client or world:
   *  0x01) ENTITY_CALC_STAT_LOCAL = locallly visible stats were changed
   *  0x02) ENTITY_CALC_STAT_WORLD = stats changed that are visible to the world
   *  0x04) ENTITY_CALC_SKILL = skill set has changed (character only)
   */
  virtual uint8_t RecalculateStats(
      libcomp::DefinitionManager* definitionManager,
      std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr,
      std::shared_ptr<objects::MiSkillData> contextSkill = nullptr);

  /**
   * Copy the current skills and stats from the entity onto an enemy or ally.
   * @param eState Enemy or Ally state to copy to
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how effects and items interact with the entity
   * @return false if an error occurred
   */
  virtual bool CopyToEnemy(const std::shared_ptr<ActiveEntityState>& eState,
                           libcomp::DefinitionManager* definitionManager);

  /**
   * Check if the entity has the supplied skill learned and not currently
   * disabled.
   * @param skillID ID of the skill to check
   * @return true if the skill is available, false if it is not
   */
  bool SkillAvailable(uint32_t skillID);

  /**
   * Get a numeric representation (also stored in constants) of the
   * entity's current alignment
   * @return Numeric representation of the entity's current alignment
   */
  virtual uint8_t GetLNCType();

  /**
   * Check if the entity matches the supplied LNC type.
   * @param lncType LNC tpe to compare to. This can be in two formats based
   *  upon the invertFlag parameter.
   * @param invertFlag If true compare using the inverted flag format
   * @return true if the LNC matches, false if it does not
   */
  bool IsLNCType(uint8_t lncType, bool invertFlag);

  /**
   * Get a numeric representation entity's gender. The values are:
   * 0) Male
   * 1) Female
   * 2) None/unspecified
   * @return Numeric representation of the entity's gender
   */
  virtual int8_t GetGender();

  /**
   * Get all skills that the entity currently has available.
   * @param definitionManager Pointer to the definition manager to use
   *  for skill source definitions
   * @param includeTokusei false if tokusei skills should not be included
   * @return Set of all skill IDs the entity currently has available
   */
  virtual std::set<uint32_t> GetAllSkills(
      libcomp::DefinitionManager* definitionManager, bool includeTokusei);

  /**
   * Get the core stats associated to the active entity.
   * @return Pointer to the active entity's core stats
   */
  virtual std::shared_ptr<objects::EntityStats> GetCoreStats();

  /**
   * Get the current level of the active entity.
   * @return Active entity's current level
   */
  virtual int8_t GetLevel();

  /**
   * Get the entity UUID associated to the entity this state represents.
   * @return Entity UUID or null UUID if not specified
   */
  virtual const libobjgen::UUID GetEntityUUID();

  /**
   * Get the world CID associated to the entity
   * @return World CID, 0 if not applicable
   */
  int32_t GetWorldCID();

  /**
   * Check if the entity is associated to a current event. Auto-only
   * events do not apply here. This affects both types of player entities.
   * @return true if an event exists, false if one does not
   */
  bool HasActiveEvent() const;

  /**
   * Get assigned entity as its EnemyBase representation or null
   * if it does not apply.
   * @return Pointer to the EnemyBase object
   */
  virtual std::shared_ptr<objects::EnemyBase> GetEnemyBase() const;

  /**
   * Set the entity's destination position based on the supplied
   * values and uses the current position values to set the origin.
   * Communicating that the move has taken place must be done elsewhere.
   * @param xPos X position to move to
   * @param yPos Y position to move to
   * @param now Server time to use as the origin ticks
   */
  void Move(float xPos, float yPos, uint64_t now);

  /**
   * Set the entity's destination rotation based on the supplied
   * values and uses the current rotation values to set the origin.
   * Communicating that the rotation has taken place must be done elsewhere.
   * @param rot New rotation value to set
   * @param now Server time to use as the origin ticks
   */
  void Rotate(float rot, uint64_t now);

  /**
   * Stop the entity's movement based on the current postion information.
   * Communicating that the movement has stopped must be done elsewhere.
   * @param now Server time to use as the origin ticks
   */
  void Stop(uint64_t now);

  /**
   * Check if the entity is currently alive
   * @return true if the entity is alive, false if they are not
   */
  bool IsAlive() const;

  /**
   * Check if the entity is currently not at their destination position
   * @return true if the entity is moving, false if they are not
   */
  bool IsMoving() const;

  /**
   * Check if the entity is currently not at their destination rotation
   * @return true if the entity is rotating, false if they are not
   */
  bool IsRotating() const;

  /**
   * Check if the entity is able to move or activate a skill
   * @return true if the entity can act, false if it cannot
   */
  bool CanAct();

  /**
   * Check if the entity is able to move
   * @param ignoreSkill Determines if all move conditions should be checked
   *  or just ones that don't pertain to a skill being used
   * @return true if the entity can move, false if it cannot
   */
  bool CanMove(bool ignoreSkill = false);

  /**
   * Check if the entity and another entity are in the same faction
   * @param other Pointer to the other entity
   * @param ignoreGroup If true only the faction will be checked.
   *  If false the faction group will also be checked.
   * @return true if the entities are in the same faction
   */
  bool SameFaction(std::shared_ptr<ActiveEntityState> other,
                   bool ignoreGroup = false);

  /**
   * Corrects rotation values that have exceeded the minimum
   * or maximum allowed range.
   * @param rot Original rotation value to correct
   * @return Equivalent but corrected rotation value
   */
  static float CorrectRotation(float rot);

  /**
   * Calculate the distance between the entity and the specified X
   * and Y coordiates
   * @param x X coordinate to calculate the distance for
   * @param y Y coordinate to calculate the distance for
   * @param squared Optional parameter to return the distance squared
   *  for faster radius comparisons
   * @return Distance between the entity and the specified point
   */
  float GetDistance(float x, float y, bool squared = false);

  /**
   * Calculate the distance between this entity and another.
   * @param other Pointer to the other entity
   * @param squared Optional parameter to return the distance squared
   *  for faster radius comparisons
   * @return Distance between this entity and the other
   */
  float GetDistance(std::shared_ptr<ActiveEntityState> other,
                    bool squared = false);

  /**
   * Determine if the entity has line of sight to another, taking into
   * account normal and one way barriers.
   * @param other Pointer to the other entity
   * @param now Current timestamp of the server
   * @return true if the entity has line of sight
   */
  bool HasLineOfSight(std::shared_ptr<ActiveEntityState> other,
                      uint64_t now = 0);

  /**
   * Get the entity's adjusted movement speed.
   * @param ignoreSkill If true the current activated ability will be ignored
   *  for determining current speed state
   * @param altSpeed true if the alternate speed used for walking should
   *  be retrieved instead of the default run speed
   * @return Movement speed of the entity
   */
  float GetMovementSpeed(bool ignoreSkill = false, bool altSpeed = false);

  /**
   * Get the hitbox size for the current entity as defined by MiDevilData.
   * The value returned is not converted to relative server units.
   * @return Hitbox size
   */
  uint32_t GetHitboxSize() const;

  /**
   * Update the entity's current position and rotation values based
   * upon the source/destination ticks and the current time.  If now
   * matches the last refresh time, no work is done.
   * @param now Current timestamp of the server
   */
  void RefreshCurrentPosition(uint64_t now);

  /**
   * Update the entity's current pending combatants, removing any that
   * are no longer valid and adding the supplied values if the time supplied
   * is no later than the simultaneous hit window (100ms) of the earliest
   * value already set. Once a value is added here, the corresponding pending
   * skill can be queued to execute. If the value is not added, the skill
   * should be retried again later.
   * @param entityID ID of the entity that is attacking
   * @param executeTime Pending execution time of the skill if it were
   *  allowed to hit
   * @return true if the skill hit was added, false if it was not
   */
  bool UpdatePendingCombatants(int32_t entityID, uint64_t executeTime);

  /**
   * Expire any status times that have passed (including skill cooldowns)
   * @param now Current timestamp of the server
   */
  void ExpireStatusTimes(uint64_t now);

  /**
   * Get the AIState of the entity
   * @return Pointer to the AIState of the entity or null if the
   *  entity is not AI controlled
   */
  std::shared_ptr<channel::AIState> GetAIState() const;

  /**
   * Set the AIState of the entity
   * @param Pointer to the AIState of the entity
   */
  void SetAIState(const std::shared_ptr<channel::AIState>& aiState);

  /**
   * Get or update the entity's current knockback value based on the last
   * ticks associated to the value and the supplied time. If the value
   * reaches or exceeds the maximum knockback resistance, the max
   * value will be used and the last update tick will be cleared.
   * @param time Timestamp from the server
   * @param recoveryBoost Recovery rate increase percentage
   * @param setValue Optional parameter to disable setting the value
   *  and only retrun the estimated value
   * @return Updated knockback value
   */
  float RefreshKnockback(uint64_t time, float recoveryBoost,
                         bool setValue = true);

  /**
   * Refresh and then reduce the entity's knockback value. If the value
   * goes under zero, it will be set to zero.
   * @param now Current timestamp of the server
   * @param decrease Value to decrease the knockback value by or -1.f
   *  to indicate a full decrease
   * @param recoveryBoost Recovery rate increase percentage
   */
  float UpdateKnockback(uint64_t now, float decrease, float recoveryBoost);

  /**
   * Check if the entity state has everything needed to start
   * being used.
   * @param ignoreDisplayState Optional parameter to ignore the display state
   *  of the entity and return true if the entity is at least set, defaults
   *  to false
   * @return true if the state is ready to use, otherwise false
   */
  virtual bool Ready(bool ignoreDisplayState = false);

  /**
   * Check if the entity state has an entity set currently. Under normal
   * circumstances, this will always be true for all but partner demons.
   * @return true if an entity is set, otherwise false
   */
  virtual bool EntityIsSet() const;

  /**
   * Check if the entity state is visible to any game client and should
   * be included when sending entity definitions or movement actions
   * @return true if the entity is visible, false if it is not
   */
  virtual bool IsClientVisible();

  /**
   * Check if the entity has a mount status indicating they are either
   * a character or demon that is mounted
   * @return true if the entity is mounted, false if it is not
   */
  bool IsMounted();

  /**
   * Get the zone the entity currently exists in.
   * @return Pointer to the entity's current zone, nullptr if they
   *  are not currently in a zone
   */
  std::shared_ptr<Zone> GetZone() const;

  /**
   * Set the entity's current zone.
   * @param zone Pointer to the zone the entity has entered
   * @param updatePrevious true if the previous zone should be notified
   *  of the change, false if this is being handled elsewhere
   */
  void SetZone(const std::shared_ptr<Zone>& zone, bool updatePrevious = true);

  /**
   * Set the HP and/or MP of the entity to either a specified or adjusted
   * value. Use this instead of a raw EntityStats function to avoid
   * conflicting updats running at the same time.
   * @param hp Specified or adjusted HP value to apply
   * @param mp Specified or adjusted MP value to apply
   * @param adjust If true the supplied HP and MP values will be added to
   *  the current value instead of being set explicitly
   * @param canOverflow true if the entity can be changed from alive to
   *  dead and vice-versa
   * @return If adjust and canOverflow are both true, the returned value
   *  will be whether the entity's HP has overflowed.  If one of those params
   *  is false, the returned value represents if the HP or MP were changed
   */
  bool SetHPMP(int32_t hp, int32_t mp, bool adjust, bool canOverflow = false);

  /**
   * Set the HP and/or MP of the entity to either a specified or adjusted
   * value. Use this instead of a raw EntityStats function to avoid
   * conflicting updats running at the same time.
   * @param hp Specified or adjusted HP value to apply
   * @param mp Specified or adjusted MP value to apply
   * @param adjust If true the supplied HP and MP values will be added to
   *  the current value instead of being set explicitly
   * @param canOverflow true if the entity can be changed from alive to
   *  dead and vice-versa
   * @param clenchChance Chance to survive a lethal hit from more than 1
   *  starting HP, represented as a 2 decimal precision value (10000 = 100%)
   * @param hpAdjusted If not adjusting, this will be the HP damage dealt,
   *  otherwise it matches the value supplied
   * @param mpAdjusted If not adjusting, this will be the MP damage dealt,
   *  otherwise it matches the value supplied
   * @return If adjust and canOverflow are both true, the returned value
   *  will be whether the entity's HP has overflowed.  If one of those params
   *  is false, the returned value represents if the HP or MP were changed
   */
  bool SetHPMP(int32_t hp, int32_t mp, bool adjust, bool canOverflow,
               int32_t clenchChance, int32_t& hpAdjusted, int32_t& mpAdjusted);

  /**
   * Get the current status effect map
   * @return Map of current status effects by type ID
   */
  const std::unordered_map<uint32_t, std::shared_ptr<objects::StatusEffect>>&
  GetStatusEffects() const;

  /**
   * Get the list of current status effects
   * @return List of status effects
   */
  std::list<std::shared_ptr<objects::StatusEffect>> GetStatusEffectsList();

  /**
   * Determine if the supplied status effect is active on the entity
   * @param effectType Status effect type to look for
   * @return true if the status effect is active, false if it is not
   */
  bool StatusEffectActive(uint32_t effectType);

  /**
   * Retrieve the time remaining on the supplied status effect
   * @param effectType Status effect type to look for
   * @return Expiration time which is based upon the supplied time
   *  for relative duration effects
   */
  uint32_t StatusEffectTimeLeft(uint32_t effectType);

  /**
   * Set the status effects currently on the entity
   * @param List of status effects currently on the entity
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how the effects behave
   */
  void SetStatusEffects(
      const std::list<std::shared_ptr<objects::StatusEffect>>& effects,
      libcomp::DefinitionManager* definitionManager);

  /**
   * Add new status effects to the entity and activate them. If there are
   * status effects that are replaced by the application of the new ones
   * they will be expired and returned.
   * @param effects Map of status effects being changed
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how the effects behave
   * @param now Current system timestamp to use when activating the effects,
   *  defaults to the current time if not specified
   * @param queueChanges If true and the entity is in an effect active state,
   *  the effect inserts, updates and removes will be queued as the next entity
   *  time-based events to be processed by the next server tick. Set to false
   *  if the changes will be communicated by some other means
   * @return Set of expired effect IDs if any added effects cancel existing
   *  effects
   */
  std::set<uint32_t> AddStatusEffects(
      const StatusEffectChanges& effects,
      libcomp::DefinitionManager* definitionManager, uint32_t now = 0,
      bool queueChanges = true);

  /**
   * Expire existing status effects by effect type ID. The expire event
   * will be queued up for processing on the next server tick.
   * @param effectTypes Set of effect type IDs to expire
   * @return Set of cancelled status effect types that will not be queued
   */
  std::set<uint32_t> ExpireStatusEffects(const std::set<uint32_t>& effectTypes);

  /**
   * Cancel existing status effects via cancel event flags. The expire
   * event will be queued up for processing on the next server tick unless
   * effects are not active.
   * @param cancelFlags Flags indicating conditions that can cause status
   *  effects to be cancelled. The set of valid status conditions are listed
   *  as constants on ActiveEntityState
   * @param keepEffects Optional set of status effects to keep
   * @return Set of cancelled status effect types that will not be queued
   */
  std::set<uint32_t> CancelStatusEffects(
      uint8_t cancelFlags, const std::set<uint32_t>& keepEffects = {});

  /**
   * Cancel existing status effects via cancel event flags. The expire
   * event will be queued up for processing on the next server tick unless
   * effects are not active.
   * @param cancelFlags Flags indicating conditions that can cause status
   *  effects to be cancelled. The set of valid status conditions are listed
   *  as constants on ActiveEntityState
   * @param cancelled Output parameter stating whether any effects were
   *  cancelled, independent of queuing
   * @param keepEffects Optional set of status effects to keep
   * @return Set of cancelled status effect types that will not be queued
   */
  std::set<uint32_t> CancelStatusEffects(
      uint8_t cancelFlags, bool& cancelled,
      const std::set<uint32_t>& keepEffects = {});

  /**
   * Activate or deactivate the entity's status effect states. By activating
   * effects, they will be registered with the ActiveEntityState and its current
   * zone via their absolute server time expiration in addition to setting any
   * quick access properties on the entity. By deactivating effects, they will
   * be unregistered from the zone, the quick access properties will be cleared
   * and the expiration time of relative effects will be updated.
   * @param activate true if the effects will activated, else they will be
   *  deactivated
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how the effects behave
   * @param now Current system timestamp to use when activating the effects,
   *  defaults to the current time if not specified
   */
  void SetStatusEffectsActive(bool activate,
                              libcomp::DefinitionManager* definitionManager,
                              uint32_t now = 0);

  /**
   * Pop effect events that have occurred past the specified time off the
   * event mapping for the entity and their current zone.
   * @param time System timestamp to use for comparing effect events that
   *  have passed
   * @param hpTDamage Output parameter to store the amount of HP time
   *  damage that should be dealt
   * @param mpTDamage Output parameter to store the amount of MP time
   *  damage that should be dealt
   * @param tUpkeep Output parameter to store skill upkeep cost that
   *  is being paid
   * @param added Output set to store the effect type IDs of newly added
   *  effects that have been queued since the specified time
   * @param updated Output set to store the effect type IDs of updated
   *  effects that have been queued since the specified time
   * @param removed Output set to store the effect type IDs of removed
   *  effects that have been queued since the specified time
   * @return Flag mask with the following flags:
   *  1) At least one effect tick passed
   *  2) Special T-damage effect occurred
   */
  uint8_t PopEffectTicks(uint32_t time, int32_t& hpTDamage, int32_t& mpTDamage,
                         int32_t& tUpkeep, std::set<uint32_t>& added,
                         std::set<uint32_t>& updated,
                         std::set<uint32_t>& removed);

  /**
   * Check if the entity currently has active T-damage effects
   * @return true if the entity has special T-damage effects, false if it
   *  does not
   */
  virtual bool HasSpecialTDamage();

  /**
   * Get the UUID of the entity responsible for creating this entity.
   * @returns UUID of entity responsible for creating this entity.
   */
  virtual libobjgen::UUID GetResponsibleEntity() const;

  /**
   * Clear or reset any active skill upkeep costs timer
   * @return true if an upkeep is active, false if there is no active upkeep
   */
  bool ResetUpkeep();

  /**
   * Get a snapshot of status effects currently on the entity with their
   * corresponding expiration time which is based upon the supplied time
   * for relative duration effects
   * @param now Current system timestamp to use for caculating relative duration
   *  expirations remaining, if nothing is specified, the current time will be
   * used
   * @return List of current status effects paired with their expiration time
   *  remaining
   */
  std::list<std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t>>
  GetCurrentStatusEffectStates(uint32_t now = 0);

  /**
   * Get the entity IDs of opponents this entity is in combat against.
   * @return Set of opponent entity IDs
   */
  std::set<int32_t> GetOpponentIDs() const;

  /**
   * Check if the entity has an opponent with the specified entity ID.
   * @param opponentID Entity ID of an opponent to find or 0 to check for any
   * @return true if the entity ID is associated as an opponent to this entity
   */
  bool HasOpponent(int32_t opponentID = 0);

  /**
   * Add or remove an opponent with the specified entity ID.
   * @param add true if the opponent will be added, false if it will be removed
   * @param opponentID Entity ID of an opponent to add or remove
   * @return Number of opponents associated to the entity after the operation
   */
  size_t AddRemoveOpponent(bool add, int32_t opponentID);

  /**
   * Get the entity's chance to null, reflect or absorb the specified affinity.
   * @param nraIdx Correct table index for affinity NRA
   *  1) Null
   *  2) Reflect
   *  3) Absorb
   *  Defines exist in the Constants header for each of these.
   * @param type Correct table type of the affinity to retrieve
   * @param calcState Override CalculatedEntityState to use instead of the
   *  entity's default
   * @return Integer representation of the chance to avoid affinity damage, 0
   *  if the supplied values are invalid
   */
  int16_t GetNRAChance(
      uint8_t nraIdx, CorrectTbl type,
      std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr);

  /**
   * Determine if the supplied NRA type/index effect shield is active on the
   * entity and optionally decrease the effect by one if it is.
   * @param nraIdx Correct table index for affinity NRA
   *  1) Null
   *  2) Reflect
   *  3) Absorb
   *  Defines exist in the Constants header for each of these.
   * @param type Correct table type of the affinity to retrieve
   * @param reduce If true, the first matching NRA shield will be reduced
   *  by one
   * @return true if a shield effect existed, false if one did not
   */
  bool GetNRAShield(uint8_t nraIdx, CorrectTbl type, bool reduce);

  /**
   * Get the next activated ability ID from 0 to 127.
   * @return The next activated ability ID for the client
   */
  int8_t GetNextActivatedAbilityID();

 protected:
  /**
   * Remove the set of supplied status effects from all registered
   * collections. This function should not be called directly in place of
   * being expired first.
   * @param effectTypes Set of effect type IDs to remove
   */
  void RemoveStatusEffects(const std::set<uint32_t>& effectTypes);

  /**
   * Activate a status effect added to the entity's current status effect set.
   * By activating an effect, it will be registered with the ActiveEntityState
   * and its current zone via their absolute server time expiration in addition
   * to setting any quick access properties on the entity.
   * @param effect Pointer to the effect to activate
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how the effect behaves
   * @param now Current system timestamp to use when activating the effect
   * @param mode Activation mode for what to register. Modes include:
   *  1) Only the time will be registered, should only be used if the
   *     expiration is being updated
   *  2) Check if the effect is already registered and register if it is not
   *  All other values will fully register the effect.
   */
  void ActivateStatusEffect(
      const std::shared_ptr<objects::StatusEffect>& effect,
      libcomp::DefinitionManager* definitionManager, uint32_t now,
      uint8_t mode);

  /**
   * Determine if the supplied status effect type can cause the entity to
   * be ignored by AI entities
   * @param effectType Effect type to check
   * @param true if the effect is an ignore effect
   */
  bool IsIgnoreEffect(uint32_t effectType) const;

  /**
   * Reset the AIIgnored value on the entity based on their current state
   */
  void ResetAIIgnored();

  /**
   * Set the next effect event time for a specified effect type for the entity's
   * event map. The zone itself must be updated using RegisterNextEffectTime
   * after each effect is set using this function.
   * @param effectType Effect type ID to register an event for
   * @param time Absolute system time to register for the event
   */
  void SetNextEffectTime(uint32_t effectType, uint32_t time);

  /**
   * Set a status effect currently on the entity and register its cancel
   * conditions
   * @param effect Pointer to a status effect to add to the entity
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how the effect behaves
   */
  void RegisterStatusEffect(
      const std::shared_ptr<objects::StatusEffect>& effect,
      libcomp::DefinitionManager* definitionManager);

  /**
   * Register the entity's next effect event time with the current zone.
   */
  void RegisterNextEffectTime();

  /**
   * Calculate the relative or absolute expiration for an effect based upon
   * the current system time and an absolute system time.
   * @param effect Pointer to the effect to calculate the expiration of
   * @param nextTime Absolute system time to use when calculating the expiration
   * @param now Current system timestamp to use when activating the effect
   * @return Calculated expiration for the effect
   */
  uint32_t GetCurrentExpiration(
      const std::shared_ptr<objects::StatusEffect>& effect, uint32_t nextTime,
      uint32_t now);

  /**
   * Adjust the supplied correct table stat values based upon adjustments from
   * equipment or status effects.
   * @param adjustments List of adjustments to the correct table values supplied
   * @param stats Output map parameter of base or calculated stats to adjust for
   *  the current entity
   * @param calcState Override CalculatedEntityState to use instead of the
   *  entity's default
   * @param baseMode If true only base stat correct table types will be
   * adjusted, if false only the non-base stat correct table types will be
   * adjusted and NRA values will be updated immediately.
   */
  void AdjustStats(
      const std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments,
      libcomp::EnumMap<CorrectTbl, int32_t>& stats,
      std::shared_ptr<objects::CalculatedEntityState> calcState, bool baseMode);

  /**
   * Update the entity's calculated NRA chances for each affinity from base and
   * equipment values. NRA chances will further be modified for status effects
   * within AdjustStats during the calculated stat mode.
   * @param stats Map containing the base NRA values
   * @param calcState Override CalculatedEntityState to use instead of the
   *  entity's default
   * @param adjustments List of adjustments to the correct table values supplied
   *  by equipment
   */
  void UpdateNRAChances(
      libcomp::EnumMap<CorrectTbl, int32_t>& stats,
      std::shared_ptr<objects::CalculatedEntityState> calcState,
      const std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments =
          {});

  /**
   * Get the correct table value adjustments from the entity's current skills
   * and status effects.
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how the skills and effects behave
   * @param calcState Override CalculatedEntityState to use instead of the
   *  entity's default
   * @param adjustments Output list parameter to add the adjustments to
   * @param contextSkill Optional contextual skill to apply adjustments from.
   *  Essentially adjusts stats "while a skill is being used" which can
   *  vary between scenarios.
   */
  void GetAdditionalCorrectTbls(
      libcomp::DefinitionManager* definitionManager,
      std::shared_ptr<objects::CalculatedEntityState> calcState,
      std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments,
      std::shared_ptr<objects::MiSkillData> contextSkill = nullptr);

  /**
   * Apply a set of skill correct table value adjustments.
   * @param skillIDs Set of skill IDs on the entity
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how the skills behave
   * @param adjustments Output list parameter to add the adjustments to
   */
  void ApplySkillCorrectTbls(
      const std::set<uint32_t>& skillIDs,
      libcomp::DefinitionManager* definitionManager,
      std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments);

  /**
   * Recalculate a demon or enemy entity's stats.
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how effects and items interact with the entity
   * @param stats Map of correct table IDs to calculated stats to set on the
   *  entity. The values already in the map should be pre-dependent stat
   *  calculated values.
   * @param adjustments List of adjustments to correct table values without
   *  standard adjustments added
   * @param calcState Override CalculatedEntityState to use instead of the
   *  entity's default
   * @param contextSkill Contextual skill to apply adjustments from.
   *  Essentially adjusts stats "while a skill is being used" which can
   *  vary between scenarios.
   * @return 1 if the calculation resulted in a change to the stats that should
   *  be sent to the client, 2 if one of the changes should be communicated to
   *  the world (for party members etc), 0 otherwise
   */
  uint8_t RecalculateDemonStats(
      libcomp::DefinitionManager* definitionManager,
      libcomp::EnumMap<CorrectTbl, int32_t>& stats,
      std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments,
      std::shared_ptr<objects::CalculatedEntityState> calcState,
      std::shared_ptr<objects::MiSkillData> contextSkill);

  /**
   * Recalculate a entity's stats for an enemy or ally which have all types
   * of state information in common
   * @param definitionManager Pointer to the DefinitionManager to use when
   *  determining how effects and items interact with the entity
   * @param calcState Override CalculatedEntityState to use instead of the
   *  entity's default
   * @param contextSkill Contextual skill to apply adjustments from.
   *  Essentially adjusts stats "while a skill is being used" which can
   *  vary between scenarios.
   * @return 1 if the calculation resulted in a change to the stats that should
   *  be sent to the client, 2 if one of the changes should be communicated to
   *  the world (for party members etc), 0 otherwise
   */
  uint8_t RecalculateEnemyStats(
      libcomp::DefinitionManager* definitionManager,
      std::shared_ptr<objects::CalculatedEntityState> calcState,
      std::shared_ptr<objects::MiSkillData> contextSkill);

  /**
   * Get all skills that the enemy or ally entity currently has available.
   * @param definitionManager Pointer to the definition manager to use
   *  for skill source definitions
   * @param includeTokusei false if tokusei skills should not be included
   * @return Set of all skill IDs the entity currently has available
   */
  std::set<uint32_t> GetAllEnemySkills(
      libcomp::DefinitionManager* definitionManager, bool includeTokusei);

  /**
   * Calculate the numeric representation (also stored in constants)
   * of the entity's alignment based off the supplied LNC point value
   * @param lncPoints LNC points representing degrees of alignment
   * @return Numeric representation of the entity's current alignment
   */
  uint8_t CalculateLNCType(int16_t lncPoints) const;

  /**
   * Remove any switch skills marked as active that are no longer available
   * to the entity.
   */
  void RemoveInactiveSwitchSkills();

  /**
   * Get the set of skill IDs granted by effective tokusei.
   * @param definitionManager Pointer to the definition manager to use
   *  for tokusei definitions
   * @return Set of skill IDs granted by effective tokusei
   */
  std::set<uint32_t> GetEffectiveTokuseiSkills(
      libcomp::DefinitionManager* definitionManager);

  /**
   * Compare and set the entity's current stats and also keep track of if
   * a change occurred.
   * @param stats Map of correct table IDs to calculated stats to set on the
   *  entity
   * @param dependentBase If true, only dependent stat base values will be
   *  checked and set. If false, final stats will be checked and set.
   * @return 1 if the calculation resulted in a change to the stats that should
   *  be sent to the client, 2 if one of the changes should be communicated to
   *  the world (for party members etc), 0 if no change resulted from the
   *  recalculation
   */
  uint8_t CompareAndResetStats(libcomp::EnumMap<CorrectTbl, int32_t>& stats,
                               bool dependentBase);

  /**
   * Compare and set the entity's current stats and also keep track of if
   * a change occurred.
   * @param stats Map of correct table IDs to calculated stats to set on the
   *  entity
   * @return 1 if the calculation resulted in a change to the stats that should
   *  be sent to the client, 2 if one of the changes should be communicated to
   *  the world (for party members etc), 0 if no change resulted from the
   *  recalculation
   */
  uint8_t CompareAndResetStats(libcomp::EnumMap<CorrectTbl, int32_t>& stats);

  /// Map of active status effects by effect type ID
  std::unordered_map<uint32_t, std::shared_ptr<objects::StatusEffect>>
      mStatusEffects;

  /// Map of active status effect definitions by effect type ID
  std::unordered_map<uint32_t, std::shared_ptr<objects::MiStatusData>>
      mStatusEffectDefs;

  /// IDs of status effects currently active that deal time damage for
  /// quick access
  std::set<uint32_t> mTimeDamageEffects;

  /// Active effect type IDs mapped to cancel condition flags for quick
  /// access
  std::unordered_map<uint8_t, std::set<uint32_t>> mCancelConditions;

  /// Map of server system times mapped to the next event time associated
  /// to each active status effect. System effects are stored here as
  /// a 0. Actual effects will be stored here as reserved values 1
  /// (inticating a new effect was added), 2 (indicating an efect has
  /// been updated) or 3 (indicating an effect has expired). Any other
  /// value stored will be an absolute system time when the regen, cost or
  /// T-Damage will be applied or the effect associated will be expired.
  std::map<uint32_t, std::set<uint32_t>> mNextEffectTimes;

  /// Pointer to the current zone the entity is in
  std::shared_ptr<Zone> mCurrentZone;

  /// Set of entity IDs representing opponents that the entity is currently
  /// fighting. If an entity is in this set, this entity should be in their
  /// set as well.
  std::set<int32_t> mOpponentIDs;

  /// Map of effect type IDs to NRA types and NRA indexes. This represents
  /// a 100% NRA result whenever a skill of the corresponding type is used
  /// which will decrease the effect stack until being cancelled out should
  /// it occur before the effect's expiration time.
  std::unordered_map<uint32_t, libcomp::EnumMap<CorrectTbl, std::set<uint8_t>>>
      mNRAShields;

  /// true if the status effects have been activated for the current zone
  bool mEffectsActive;

  /// Signifies that the entity is alive
  bool mAlive;

  /// false if the entity has been assigned but never calculated
  bool mInitialCalc;

  /// Signifies that the entity has the cloak status active
  bool mCloaked;

  /// Last timestamp the entity's state was refreshed
  uint64_t mLastRefresh;

  /// Next system time when regen and timed damage will occur
  uint32_t mNextRegenSync;

  /// Next system time when skill upkeep costs will occur
  uint32_t mNextUpkeep;

  /// Next available activated ability ID
  int8_t mNextActivatedAbilityID;

  /// Pointer to the AI state information bound to the entity
  std::shared_ptr<AIState> mAIState;

  /// Server lock for shared resources
  std::mutex mLock;
};

/**
 * Contains the state of an active entity related to a channel.
 */
template <typename T>
class ActiveEntityStateImp : public ActiveEntityState {
 public:
  /**
   * Create a new active entity state.
   */
  ActiveEntityStateImp();

  /**
   * Clean up the active entity state.
   */
  virtual ~ActiveEntityStateImp() {}

  /**
   * Get the active entity
   * @return Pointer to the active entity
   */
  std::shared_ptr<T> GetEntity() const { return mEntity; }

  /**
   * Set the active entity
   * @param entity Pointer to the active entity
   * @param definitionManager Pointer to the definition manager to use
   *  for definining entity definitions
   */
  void SetEntity(const std::shared_ptr<T>& entity,
                 libcomp::DefinitionManager* definitionManager);

  virtual std::shared_ptr<objects::EntityStats> GetCoreStats() {
    return mEntity ? mEntity->GetCoreStats().Get() : nullptr;
  }

  virtual int8_t GetLevel() {
    auto cs = GetCoreStats();
    return cs ? cs->GetLevel() : 0;
  }

  virtual bool Ready(bool ignoreDisplayState = false) {
    return mEntity != nullptr &&
           (ignoreDisplayState ||
            (GetDisplayState() == ActiveDisplayState_t::ACTIVE &&
             !GetIsHidden()));
  }

  virtual bool EntityIsSet() const { return mEntity != nullptr; }

  virtual bool IsClientVisible() {
    auto displayState = GetDisplayState();
    return mEntity != nullptr &&
           (displayState == ActiveDisplayState_t::ACTIVE ||
            displayState == ActiveDisplayState_t::MOUNT);
  }

 private:
  /// Pointer to the active entity
  std::shared_ptr<T> mEntity;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_ACTIVEENTITYSTATE_H
