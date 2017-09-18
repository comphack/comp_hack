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

#ifndef SERVER_CHANNEL_SRC_AISTATE_H
#define SERVER_CHANNEL_SRC_AISTATE_H

// libcomp Includes
#include <ScriptEngine.h>

// channel Includes
#include "AICommand.h"

// Standard C++11 includes
#include <mutex>

namespace channel
{

/// AI skill type for close ranged attacks
const uint16_t AI_SKILL_TYPE_CLSR = 0x01;

/// AI skill type for long ranged attacks
const uint16_t AI_SKILL_TYPE_LNGR = 0x02;

/// AI skill type for defensive skills
const uint16_t AI_SKILL_TYPE_DEF = 0x04;

/// AI skill type for healing skills
const uint16_t AI_SKILL_TYPE_HEAL = 0x08;

/// AI skill type for support skills
const uint16_t AI_SKILL_TYPE_SUPPORT = 0x10;

/// AI skill type mask for enemy affecting skills
const uint16_t AI_SKILL_TYPES_ENEMY = 0x03;

/// AI skill type mask for ally affecting skills
const uint16_t AI_SKILL_TYPES_ALLY = 0x1C;

/// AI skill type mask for all skills
const uint16_t AI_SKILL_TYPES_ALL = 0x1F;

/**
 * Possible AI statuses for an active AI controlled entity.
 */
enum AIStatus_t : uint8_t
{
    IDLE = 0,   //!< Entity is either stationary or otherwise not active
    WANDERING,  //!< Enemy entity is wandering around its spawn location
    AGGRO,  //!< Entity is not in combat yet but is pursuing a target
    COMBAT, //!< Entity is engaged in combat with one or more opponent
};

/**
 * Contains the state of an entity's AI information when controlled
 * by the channel.
 */
class AIState
{
public:
    /**
     * Create a new AI state.
     */
    AIState();

    /**
     * Clean up the AI state.
     */
    virtual ~AIState();

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
     */
    void SetStatus(AIStatus_t status, bool isDefault = false);

    /**
     * Check if the status is set to idle
     * @return true if the status is set to idle
     */
    bool IsIdle() const;

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
     * @return Point to the bound AI script or null if not bound
     */
    std::shared_ptr<libcomp::ScriptEngine> GetScript() const;

    /**
     * Bind an AI script to the AI controlled entity
     * @param aiScript Script to bind to the AI controlled entity
     */
    void SetScript(const std::shared_ptr<libcomp::ScriptEngine>& aiScript);

    /**
     * Get the current command or next command that has not been started
     * to process for the entity
     * @return Pointer to the current/next command
     */
    std::shared_ptr<AICommand> GetCurrentCommand() const;

    /**
     * Queue a command to process for the AI controlled entity
     * @param command Pointer to a command to queue for the entity
     */
    void QueueCommand(const std::shared_ptr<AICommand>& command);

    /**
     * Clear all queued commands
     */
    void ClearCommands();

    /**
     * Pop the first command off the command queue
     * @return Pointer to the command that was removed or null if
     *  there were no commands
     */
    std::shared_ptr<AICommand> PopCommand();

    /**
     * Entity ID of the the currently targeted entity or 0 if none
     * is set
     * @return Entity ID of the AI controlled entity's target
     */
    int32_t GetTarget() const;

    /**
     * Set the entity ID of the currently targeted entity
     * @param Entity ID of the currently targeted entity
     */
    void SetTarget(int32_t targetEntityID);

    /**
     * Get the settings mask that specifies what kinds of skills
     * the AI controlled entity should use
     * @return skillSettings Skill settings mask
     */
    uint16_t GetSkillSettings() const;

    /**
     * Set the settings mask that specifies what kinds of skills
     * the AI controlled entity should use
     * @param skillSettings Skill settings mask
     */
    void SetSkillSettings(uint16_t skillSettings);

    /**
     * Check if the AI controlled entity's skill map has been set
     * @return true if the skills have been mapped, false if they
     *  have not
     */
    bool SkillsMapped() const;

    /**
     * Mark the skill map as needing a refresh
     */
    void ResetSkillsMapped();

    /**
     * Get the mapped skills of the AI controlled entity
     * @return Map of AI skill types to active skill IDs the entity can use
     */
    std::unordered_map<uint16_t,
        std::vector<uint32_t>> GetSkillMap() const;

    /**
     * Set the mapped skills of the AI controlled entity
     * @param skillMap Map of AI skill types to active skill IDs the entity
     *  can use
     */
    void SetSkillMap(const std::unordered_map<uint16_t,
        std::vector<uint32_t>>& skillMap);

    /**
     * Override a default server side processing action with a
     * function in the script bound to the entity.
     * @param action Name of the action to override
     * @param functionName Name of the script function that will
     *  override the action. If this is specified as blank, a
     *  function with the action name specified will be called
     *  when the action is hit instead.
     */
    void OverrideAction(const libcomp::String& action,
        const libcomp::String& functionName);

    /**
     * Check if the specified AI action has been overridden
     * @param action Name of the action to check
     * @return true if the action has been overridden
     */
    bool IsOverriden(const libcomp::String& action);

    /**
     * Get the script function name set to override the AI action
     * @param action Name of the action to check
     * @return Name of the function set to override the AI action
     */
    libcomp::String GetScriptFunction(const libcomp::String& action);

private:
    /// List of all AI commands to be processed, starting with the current
    /// command and ending with the last to be processed
    std::list<std::shared_ptr<AICommand>> mCommandQueue;

    /// Pointer to the current AI command or the next to process if not started
    std::shared_ptr<AICommand> mCurrentCommand;

    /// Map of AI actions to script function names to call instead
    std::unordered_map<std::string, libcomp::String> mActionOverrides;

    /// Map of AI skill types to active skill IDs the entity can use
    std::unordered_map<uint16_t, std::vector<uint32_t>> mSkillMap;

    /// Pointer to the AI script to use for the AI controlled entity
    std::shared_ptr<libcomp::ScriptEngine> mAIScript;

    /// Entity ID of another entity in the zone being targeted independent
    /// of a skill being used
    int32_t mTargetEntityID;

    /// Mask of AI skill type flags the entity will use
    uint16_t mSkillSettings;

    /// Current AI status of the entity
    AIStatus_t mStatus;

    /// Previous AI status of the entity
    AIStatus_t mPreviousStatus;

    /// Default AI status of the entity
    AIStatus_t mDefaultStatus;

    /// Specifies that the status has changed and hasn't been checked yet
    bool mStatusChanged;

    /// Speifies that the entity's skills need to be mapped for use
    bool mSkillsMapped;

    /// Server lock for shared resources (represented by pointer for ease
    /// of Sqrat use)
    std::mutex* mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_AISTATE_H
