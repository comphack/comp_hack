/**
 * @file server/channel/src/AIManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Class to manage all server side AI related actions.
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

#ifndef SERVER_CHANNEL_SRC_AIMANAGER_H
#define SERVER_CHANNEL_SRC_AIMANAGER_H

// channel Includes
#include "AIState.h"
#include "ActiveEntityState.h"
#include "ClientState.h"

namespace libcomp {
class ScriptEngine;
}

namespace objects {
class MiSkillData;
}

namespace channel {

class AICommand;
class AIMoveCommand;
class ChannelServer;
class EnemyBase;
class Point;
class Zone;

/**
 * Class to manage actions when triggering a spot or interacting with
 * an object/NPC.
 */
class AIManager {
 public:
  /**
   * Create a new AIManager with no associated server. This should not
   * be used but is necessary for use with Sqrat.
   */
  AIManager();

  /**
   * Create a new AIManager
   * @param server Pointer back to the channel server this belongs to.
   */
  AIManager(const std::weak_ptr<ChannelServer>& server);

  /**
   * Clean up the AIManager
   */
  ~AIManager();

  /**
   * Prepare an entity for AI control following the setting of all
   * other necessary data
   * @param eState Pointer to the entity state
   * @param aiType AI script type to bind to the entity
   * @param baseAIType Optional base AI type to default to. Setting this
   *  to a value with a bound AI script will override the supplied type.
   * @return true on sucess, false upon failure
   */
  bool Prepare(const std::shared_ptr<ActiveEntityState>& eState,
               const libcomp::String& aiType, uint16_t baseAIType = 0);

  /**
   * Update the AI state of all active AI controlled entities in the
   * specified zone
   * @param zone Pointer to the zone to update
   * @param now Current server time
   * @param isNight Specifies that the server time is currently night
   *  which is between 1800 and 0599
   */
  void UpdateActiveStates(const std::shared_ptr<Zone>& zone, uint64_t now,
                          bool isNight);

  /**
   * Handler for any AI controlled entities that get hit by a combat skill
   * from another entity. This is executed immediately after a skill is
   * processed and reported to the clients.
   * @param entities List of pointers to AI controlled entities
   * @param source Pointer to the source of the skill. Can be an ally
   *  but is never the same entity being hit
   * @param skillData Pointer to the definition of the skill being used
   */
  void CombatSkillHit(
      const std::list<std::shared_ptr<ActiveEntityState>>& entities,
      const std::shared_ptr<ActiveEntityState>& source,
      const std::shared_ptr<objects::MiSkillData>& skillData);

  /**
   * Handler for an AI controlled entity completing a skill responsible
   * for timing post skill use actions
   * @param eState Pointer to the entity state
   * @param activated Pointer to the skill's activation state
   * @param skillData Pointer to the definition of the skill being used
   * @param target Pointer to the primary target of the skill. Can be
   *  an ally or even the entity itself if a reflect occurred. The original
   *  target can be retrieved from the ActivatedAbility
   * @param hit true if the skill hit the target entity in a normal fasion
   *  (not avoided, NRA'd etc)
   */
  void CombatSkillComplete(
      const std::shared_ptr<ActiveEntityState>& eState,
      const std::shared_ptr<objects::ActivatedAbility>& activated,
      const std::shared_ptr<objects::MiSkillData>& skillData,
      const std::shared_ptr<ActiveEntityState>& target, bool hit);

  /**
   * Queue a use move command on the specified AI controlled entity.
   * The shortest possible path will be used when creating the command.
   * @param eState Pointer to the entity state
   * @param x X coordiate to move to
   * @param y Y coordiate to move to
   * @param interrupt If true the command will interrupt whatever
   *  the current command is
   * @return true if the command was queued, false if it was not
   */
  bool QueueMoveCommand(const std::shared_ptr<ActiveEntityState>& eState,
                        float x, float y, bool interrupt = false);

  /**
   * Queue a use skill command on the specified AI controlled entity
   * @param eState Pointer to the entity state
   * @param skillID Skill ID to use
   * @param targetEntityID Entity ID of the target to use the skill on
   * @param advance If true, a pre-skill movement command will be queued
   *  first
   * @return true if the skill queued correctly, false if it did not
   */
  bool QueueUseSkillCommand(const std::shared_ptr<ActiveEntityState>& eState,
                            uint32_t skillID, int32_t targetEntityID,
                            bool advance);

  /**
   * Queue a script command on the specified AIState
   * @param aiState Pointer to the AIState
   * @param functionName Name of the script function to be executed when
   *  the command is processed
   * @param interrupt If true the command will interrupt whatever
   *  the current command is
   */
  void QueueScriptCommand(const std::shared_ptr<AIState>& aiState,
                          const libcomp::String& functionName,
                          bool interrupt = false);

  /**
   * Queue a wait command on the specified AIState
   * @param aiState Pointer to the AIState
   * @param waitTime Number of seconds the entity should wait when
   *  the command is processed
   * @param interrupt If true the command will interrupt whatever
   *  the current command is
   */
  void QueueWaitCommand(const std::shared_ptr<AIState>& aiState,
                        uint32_t waitTime, bool interrupt = false);

  /**
   * Start an event with the supplied entity as the source entity
   * @param eState Pointer to the entity state
   * @param eventID Event ID to start
   * @return true if the event started, false if it did not
   */
  bool StartEvent(const std::shared_ptr<ActiveEntityState>& eState,
                  const libcomp::String& eventID);

  /**
   * Set or remove the aggro target on an AI controlled entity
   * @param eState Pointer to the entity state
   * @param targetID Entity ID representing the new AI target or
   *  -1 for no target
   */
  void UpdateAggro(const std::shared_ptr<ActiveEntityState>& eState,
                   int32_t targetID);

  /**
   * Clear all commands assigned to an AI controlled entity
   * @param eState Pointer to the entity state
   * @return true if any commands were removed
   */
  bool ClearCommands(const std::shared_ptr<ActiveEntityState>& eState);

  /**
   * Begin using a Diaspora quake skill from the supplied source entity.
   * Notifications will be sent to players in the zone that the skill
   * has started and another when it hits.
   * @param source Pointer to the source of the skill
   * @param skillID ID of the quake skill being used. The function ID
   *  of this skill must match the correct type
   * @param delay Delay from start to completion for the skill in seconds
   * @return true if the skill started properly, false if it did not
   */
  bool UseDiasporaQuake(const std::shared_ptr<ActiveEntityState>& source,
                        uint32_t skillID, float delay);

  /**
   * Warp an AI controlled entity to a different point in the same zone
   * @param eState Pointer to the entity state
   * @param spotID Spot ID in the zone
   * @param x x coordinate to use if no spot ID specified
   * @param y y coordinate to use if no spot ID specified
   * @param rot Rotation to use if no spot ID specified
   * @param clearCommands If true, all current commands will be cleared
   * @return true if the entity was warped
   */
  bool Warp(const std::shared_ptr<ActiveEntityState>& eState, uint32_t spotID,
            float x, float y, float rot);

  /**
   * Create a move command that chases a specified target in the same
   * zone.
   * @param eState Pointer to the entity state
   * @param targetEntityID Entity ID of the target to chase
   * @param minDistance Optional minimum distance needed before stopping,
   *  0 to ignore
   * @param maxDistance Optional maximum distance allowed before stopping,
   *  0 to ignore
   * @param interrupt If true the command will interrupt whatever
   *  the current command is
   * @param allowLazy If true and AILazyPathing is enabled, attempt to reach
   *  the destination via a linear path only. If false, navigate around
   *  obstacles via the shortest path.
   * @return true if the command was queued, false if it was not
   */
  bool Chase(const std::shared_ptr<ActiveEntityState>& eState,
             int32_t targetEntityID, float minDistance = 0.f,
             float maxDistance = 0.f, bool interrupt = false,
             bool allowLazy = false);

  /**
   * Create a move command that causes the entity to retreat from a
   * target point in a straight line up to a min distance. Only fails
   * if something was invalid or the entity would not retreat further
   * from the command.
   * @param eState Pointer to the entity state
   * @param x X coordiate to retreat from
   * @param y Y coordiate to retreat from
   * @param interrupt If true the command will interrupt whatever
   *  the current command is
   * @param distance Distance to put between the entity and its target
   * @return true if the command was queued, false if it was not
   */
  bool Retreat(const std::shared_ptr<ActiveEntityState>& eState, float x,
               float y, float distance, bool interrupt = false);

  /**
   * Create a move command that causes the entity to retreat from a
   * target point and circle them when far enough away. If they are
   * further away from the target than the designated distance, they
   * will move in first.
   * @param eState Pointer to the entity state
   * @param x X coordiate to circle
   * @param y Y coordiate to circle
   * @param interrupt If true the command will interrupt whatever
   *  the current command is
   * @param distance Distance to put between the entity and its target
   * @return true if the command was queued, false if it was not
   */
  bool Circle(const std::shared_ptr<ActiveEntityState>& eState, float x,
              float y, bool interrupt = false, float distance = 800.f);

 private:
  /**
   * Update the state of an entity, processing AI and performing other
   * related actions.
   * @param eState Pointer to the entity state to update
   * @param now Current timestamp of the server
   * @param isNight Night time indicator which affects targetting
   * @return true if the entity state should be communicated to the zone,
   *  false otherwise
   */
  bool UpdateState(const std::shared_ptr<ActiveEntityState>& eState,
                   uint64_t now, bool isNight);

  /**
   * Update the state of an enemy or ally, processing AI directly or queuing
   * commands to be procssed on next update
   * @param eState Pointer to the entity state to update
   * @param eBase Pointer to the enemy base object from the entity
   * @param now Current timestamp of the server
   * @param isNight Night time indicator which affects targetting
   * @return true if the entity state should be communicated to the zone,
   *  false otherwise
   */
  bool UpdateEnemyState(const std::shared_ptr<ActiveEntityState>& eState,
                        const std::shared_ptr<objects::EnemyBase>& eBase,
                        uint64_t now, bool isNight);

  /**
   * Determine any follow state specific logic for the supplied entity
   * @param eState Pointer to the entity with a follow entity
   * @param now Current timestamp of the server
   * @return true if follow specific actions were performed and the
   *  entity state has completed being updated, false otherwise
   */
  bool Follow(const std::shared_ptr<ActiveEntityState>& eState, uint64_t now);

  /**
   * Set the entity's destination position based on the supplied
   * values and uses the current position values to set the origin.
   * Communicating that the move has taken place must be done elsewhere.
   * @param eState Pointer to the entity state
   * @param dest Position to move to
   * @param now Server time to use as the origin ticks
   */
  void Move(const std::shared_ptr<ActiveEntityState>& eState, Point dest,
            uint64_t now);

  /**
   * Handle normal wander actions for the supplied AI controlled enemy
   * based entity
   * @param eState Pointer to the entity state to update
   * @param eBase Pointer to the enemy base object from the entity
   */
  void Wander(const std::shared_ptr<ActiveEntityState>& eState,
              const std::shared_ptr<objects::EnemyBase>& eBase);

  /**
   * Queue a command that will move the entity into range with its target
   * @param eState Pointer to the entity state
   * @param skillData Pointer to the skill that will be used
   * @param distOverride Optional target distance to use as the minimum
   *  distance in place of the normal skill range
   * @return 0 if the entity queued a move command to the target, 1 if the
   *  move was not possible, 2 if no move was needed
   */
  uint8_t SkillAdvance(const std::shared_ptr<ActiveEntityState>& eState,
                       const std::shared_ptr<objects::MiSkillData>& skillData,
                       float distOverride = 0.f);

  /**
   * Clear an entity's AIState current target and find the next target to focus
   * on
   * @param state Pointer to the AI controlled entity state
   * @param now Current timestamp of the server
   * @param isNight Night time indicator which affects targetting
   * @return Pointer to the entity being targeted
   */
  std::shared_ptr<ActiveEntityState> Retarget(
      const std::shared_ptr<ActiveEntityState>& state, uint64_t now,
      bool isNight);

  /**
   * Refresh the skill map of the AIState if needed
   * @param eState Pointer to the entity state to update
   * @param aiState Pointer to the AIState
   */
  void RefreshSkillMap(const std::shared_ptr<ActiveEntityState>& eState,
                       const std::shared_ptr<AIState>& aiState);

  /**
   * Determine if a skill can be used at a base level
   * @param skillData Pointer to the skill definition
   * @return true if it is valid, false if it is not
   */
  bool SkillIsValid(const std::shared_ptr<objects::MiSkillData>& skillData);

  /**
   * Determine if a skill that has already attempted activation can
   * be used again
   * @param eState Pointer to the entity state
   * @param activated Pointer to the skill's activation state
   * @return true if the skill can be reactivated, false if it cannot
   *  and should be cancelled
   */
  bool CanRetrySkill(
      const std::shared_ptr<ActiveEntityState>& eState,
      const std::shared_ptr<objects::ActivatedAbility>& activated);

  /**
   * Determine which skill the entity will use next based on their current
   * state
   * @param eState Pointer to the entity state
   * @return true if a skill was selected, false if none can be used
   */
  bool PrepareSkillUsage(const std::shared_ptr<ActiveEntityState>& state);

  /**
   * Execute a function on the script bound to an entity's AI state and return
   * the result
   * @param eState Pointer to the AI controlled entity state
   * @param functionName Name of the script function to execute
   * @param now Current timestamp of the server
   * @param result Output parameter to store the script return result in
   * @return true if the function exists and returned a result, false if it did
   * not
   */
  template <class T>
  bool ExecuteScriptFunction(const std::shared_ptr<ActiveEntityState>& eState,
                             const libcomp::String& functionName, uint64_t now,
                             T& result) {
    auto aiState = eState->GetAIState();
    auto script = aiState->GetScript();
    if (!script) {
      return false;
    }

    Sqrat::Function f(Sqrat::RootTable(script->GetVM()), functionName.C());

    auto scriptResult = !f.IsNull() ? f.Evaluate<T>(eState, this, now) : 0;
    if (!scriptResult) {
      return false;
    }

    result = *scriptResult;
    return true;
  }

  /**
   * Get a new move command from one point to another, calculating pathing and
   * adjusting for collisions
   * @param eState Pointer to the entity state to move
   * @param dest End point
   * @param reduce Reduces the final movement path by a set amount so the entity
   *  ends up that amount of units away
   * @param split If true, movements will be split into smaller segments to
   *  be more accurate to original movement. Disable for less important
   *  actions such as wandering with no target for less packet traffic.
   * @param allowLazy If true and AILazyPathing is enabled, attempt to reach
   *  the destination via a linear path only. If false, navigate around
   *  obstacles via the shortest path.
   * @return Pointer to the new move command
   */
  std::shared_ptr<AIMoveCommand> GetMoveCommand(
      const std::shared_ptr<ActiveEntityState>& eState, const Point& dest,
      float reduce = 0.f, bool split = true, bool allowLazy = false);

  /**
   * Get a new wait command
   * @param waitTime Number of milliseconds the entity should wait when
   *  the command is processed
   * @return Pointer to the new move command
   */
  std::shared_ptr<AICommand> GetWaitCommand(uint32_t waitTime) const;

  /**
   * Add or remove a specific entity ID from the aggro targets on the
   * supplied entity
   * @param eState Pointer to the entity state
   * @param targetID Entity ID representing the target entity
   * @param remove If true, remove the entity, otherwise add it
   */
  void AddRemoveAggro(const std::shared_ptr<ActiveEntityState>& eState,
                      int32_t targetID, bool remove);

  /**
   * Get the other entity that shares aggro with the supplied entity or
   * null if none exists or the server is not configured to use shared
   * aggro
   * @param eState Pointer to the entity state
   * @return Pointer to the other entity
   */
  std::shared_ptr<ActiveEntityState> GetSharedAggroEntity(
      const std::shared_ptr<ActiveEntityState>& eState);

  /**
   * Determine if combat stagger is enabled for the server to adjust
   * timing of post charge and follow up attacks.
   * @return true if combat stagger is enabled, false if it is not
   */
  bool CombatStaggerEnabled();

  /**
   * Determine if the AI does not bother to navigate to a target that is
   * not in direct line of sight. Only affects specific movement types.
   * @return true if lazy pathing is enabled, false if it is not
   */
  bool LazyPathingEnabled();

  /// Static map of scripts that have been loaded and compiled
  /// by AI type name
  static std::unordered_map<std::string, std::shared_ptr<libcomp::ScriptEngine>>
      sPreparedScripts;

  /// Pointer to the channel server.
  std::weak_ptr<ChannelServer> mServer;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_AIMANAGER_H
