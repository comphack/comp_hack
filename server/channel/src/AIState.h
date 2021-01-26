/**
 * @file server/channel/src/AIState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Contains AI related data for an active entity on the channel.
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

#ifndef SERVER_CHANNEL_SRC_AISTATE_H
#define SERVER_CHANNEL_SRC_AISTATE_H

// libcomp Includes
#include <Constants.h>
#include <ScriptEngine.h>

// object Includes
#include <AIStateObject.h>

// channel Includes
#include "AICommand.h"

namespace objects {
class MiSkillData;
}

namespace channel {

/// AI skill type mask for enemy affecting skills
const uint16_t AI_SKILL_TYPES_ENEMY =
    (uint16_t)(AI_SKILL_TYPE_CLSR | AI_SKILL_TYPE_LNGR);

/// AI skill type mask for ally affecting skills
const uint16_t AI_SKILL_TYPES_ALLY =
    (uint16_t)(AI_SKILL_TYPE_DEF | AI_SKILL_TYPE_HEAL | AI_SKILL_TYPE_SUPPORT);

/// AI skill type mask for all skills
const uint16_t AI_SKILL_TYPES_ALL =
    (uint16_t)(AI_SKILL_TYPES_ENEMY | AI_SKILL_TYPES_ALLY);

typedef std::pair<std::shared_ptr<objects::MiSkillData>, uint16_t>
    AISkillWeight_t;
typedef std::unordered_map<uint16_t, std::list<AISkillWeight_t>> AISkillMap_t;

/**
 * Possible AI statuses for an active AI controlled entity.
 */
enum AIStatus_t : uint8_t {
  IDLE = 0,   //!< Entity is either stationary or otherwise not active
  WANDERING,  //!< Enemy entity is wandering around its spawn location
  FOLLOWING,  //!< Entity is following its follow target (if possible)
  AGGRO,      //!< Entity is not in combat yet but is pursuing a target
  COMBAT,     //!< Entity is engaged in combat with one or more opponent
};

/**
 * Contains the state of an entity's AI information when controlled
 * by the channel.
 */
class AIState : public objects::AIStateObject {
 public:
  /**
   * Create a new AI state.
   */
  AIState();

  /**
   * Get the status
   * @return Status of the AI state
   */
  AIStatus_t GetStatus() const;

  /**
   * Get the previous status
   * @return Previous status of the AI state
   */
  AIStatus_t GetPreviousStatus() const;

  /**
   * Get the default status
   * @return Default status of the AI state
   */
  AIStatus_t GetDefaultStatus() const;

  /**
   * Set the status and optionally set it as the default as well
   * @param status Status to update the state to
   * @param isDefault Optional parameter to set the status as the
   *  state's default status as well
   * @return true if it was set, false if it was not
   */
  bool SetStatus(AIStatus_t status, bool isDefault = false);

  /**
   * Check if the status is set to combat
   * @return true if the status is set to combat
   */
  bool InCombat() const;

  /**
   * Check if the status is set to aggro (or optionally combat)
   * @param includeCombat Optional parameter to include the combat state too
   * @return true if the status is set to aggro (or optionally combat)
   */
  bool IsAggro(bool includeCombat = true) const;

  /**
   * Check if the status is set to following (which means they are either
   * currently following a target or were and have not updated)
   * @return true if the status is set to following
   */
  bool IsFollowing() const;

  /**
   * Check if the status is set to idle
   * @return true if the status is set to idle
   */
  bool IsIdle() const;

  /**
   * Check if the status is set to wandering
   * @return true if the status is set to wandering
   */
  bool IsWandering() const;

  /**
   * Check if the entity has a follow entity target
   * @return true if a follow entity target is assigned
   */
  bool HasFollowTarget() const;

  /**
   * Check if the status has changed since the last server refresh
   * @return true if the status has changed since the last server refresh
   */
  bool StatusChanged() const;

  /**
   * Reset the status changed flag
   */
  void ResetStatusChanged();

  /**
   * Get the bound AI script
   * @return Pointer to the bound AI script or null if not bound
   */
  std::shared_ptr<libhack::ScriptEngine> GetScript() const;

  /**
   * Bind an AI script to the AI controlled entity
   * @param aiScript Script to bind to the AI controlled entity
   */
  void SetScript(const std::shared_ptr<libhack::ScriptEngine>& aiScript);

  /**
   * Get the AI's aggro value from its base AI definition representing
   * day, night and enemy casting distances and FoVs
   * @param mode AI aggro type:
   *  0) Normal
   *  1) Night
   *  2) Enemy skill casting (any time)
   * @param fov true if the FoV value corresponding to the mode should
   *  be retrieved, false if the distance should be retrieved
   * @param defaultVal Default value to return should the requested
   *  value not exist
   * @return Aggro distance or FoV corresponding to the requested mode
   */
  float GetAggroValue(uint8_t mode, bool fov, float defaultVal);

  /**
   * Get the AI's de-aggro distnace from its base AI definition.
   * @param isNight If true, the entity's night search distance will be
   *  used for the calculation. If false, the normal distance will be used.
   * @return De-aggro distance for the entity
   */
  float GetDeaggroDistance(bool isNight);

  /**
   * Get the current command or next command that has not been started
   * to process for the entity
   * @return Pointer to the current/next command
   */
  std::shared_ptr<AICommand> GetCurrentCommand() const;

  /**
   * Queue a command to process for the AI controlled entity
   * @param command Pointer to a command to queue for the entity
   * @param interrupt If true the command will become the new current
   *  command. If false the command will be added to the end.
   */
  void QueueCommand(const std::shared_ptr<AICommand>& command,
                    bool interrupt = false);

  /**
   * Clear all queued commands
   */
  void ClearCommands();

  /**
   * Pop the first command off the command queue
   * @param specific Optional pointer to the specific command to remove
   *  in case its not the first command
   * @return Pointer to the command that was removed or null if
   *  there were no commands
   */
  std::shared_ptr<AICommand> PopCommand(
      const std::shared_ptr<AICommand>& specific = nullptr);

  /**
   * Mark the skill map as needing a refresh
   */
  void ResetSkillsMapped();

  /**
   * Get the mapped skills of the AI controlled entity
   * @return Map of AI skill types to active skill definitions the entity
   *  can use
   */
  AISkillMap_t GetSkillMap() const;

  /**
   * Set the mapped skills of the AI controlled entity
   * @param skillMap Map of AI skill types to active skill definitions
   *  the entity can use
   */
  void SetSkillMap(const AISkillMap_t& skillMap);

 private:
  /// List of all AI commands to be processed, starting with the current
  /// command and ending with the last to be processed
  std::list<std::shared_ptr<AICommand>> mCommandQueue;

  /// Pointer to the current AI command or the next to process if not started
  std::shared_ptr<AICommand> mCurrentCommand;

  /// Map of AI skill types to active skill definitions the entity can use
  /// with definitions and assigned weights that recalculate whenever the
  /// set or costs are adjusted
  AISkillMap_t mSkillMap;

  /// Pointer to the AI script to use for the AI controlled entity
  std::shared_ptr<libhack::ScriptEngine> mAIScript;

  /// Current AI status of the entity
  AIStatus_t mStatus;

  /// Previous AI status of the entity
  AIStatus_t mPreviousStatus;

  /// Default AI status of the entity
  AIStatus_t mDefaultStatus;

  /// Specifies that the status has changed and hasn't been checked yet
  bool mStatusChanged;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_AISTATE_H
