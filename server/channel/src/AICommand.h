/**
 * @file server/channel/src/AICommand.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents a command for an AI controllable entity on
 *  the channel.
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

#ifndef SERVER_CHANNEL_SRC_AICOMMAND_H
#define SERVER_CHANNEL_SRC_AICOMMAND_H

// libcomp Includes
#include <CString.h>

namespace objects
{
class ActivatedAbility;
}

namespace channel
{

/**
 * Type of AI command used to specify what should happen to an AI controlled
 * entity upon state update.
 */
enum AICommandType_t : uint8_t
{
    NONE = 0,   //!< No special action is taken but the entity may still wait
    MOVE,       //!< Entity moves from one point to another
    USE_SKILL,  //!< Entity is either activating or executing a skill
    SCRIPTED,   //!< Entity is executing a function on
};

/**
 * Base class for all AI commands to be handled by the AIManager on
 * server ticks. Any command can be configured to delay before execution
 * so this doubles up as a request to "wait" when no other action is queued.
 */
class AICommand
{
public:
    /**
     * Create a new AI command
     */
    AICommand();

    /**
     * Clean up the AI command
     */
    virtual ~AICommand();

    /**
     * Get the AI command type
     * @return AI command type
     */
    AICommandType_t GetType() const;

    /**
     * Get the delay to process before the command itself
     * @return Pre-process delay time in microseconds
     */
    uint64_t GetDelay() const;

    /**
     * Set the delay to process before the command itself
     * @param delay Pre-process delay time in microseconds
     */
    void SetDelay(uint64_t delay);

    /**
     * Check if the command has been marked as started
     * @return true if the command has started, false if it
     *  has not
     */
    bool Started();

    /**
     * Set the command as having been started
     */
    void Start();

protected:
    /// AI command type
    AICommandType_t mType;

    /// Pre-process delay time in microseconds
    uint64_t mDelay;

    /// Indicates that the command has been started
    bool mStarted;
};

/**
 * AI command defining where the entity will move and contains pathing
 * information about how to get there
 */
class AIMoveCommand : public AICommand
{
public:
    /**
     * Create a new move command
     */
    AIMoveCommand();

    /**
     * Clean up the move command
     */
    virtual ~AIMoveCommand();

    /**
     * Set the pathing information for the move command, using the last
     * point as the destination
     * @param pathing List of sequential points defining a movement path
     */
    void SetPathing(const std::list<std::pair<float, float>>& pathing);

    /**
     * Get the x and y coordinates of the point in the pathing that will
     * be moved to next
     * @param x Output parameter for the next movement point's X coordinate
     * @param y Output parameter for the next movement point's Y coordinate
     * @return true if a current destination exists, false if one does not
     */
    bool GetCurrentDestination(float& x, float& y) const;
    
    /**
     * Get the x and y coordinates of the point at the end of the pathing
     * @param x Output parameter for the final movement point's X coordinate
     * @param y Output parameter for the final movement point's Y coordinate
     * @return true if the end destination exists, false if one does not
     */
    bool GetEndDestination(float& x, float& y) const;

    /**
     * Remove the current destination from the pathing and move on to the next
     * @return true if another destination exists in the pathing, false if no
     *  more points exist
     */
    bool SetNextDestination();

private:
    /// List of sequential points defining a movement path
    std::list<std::pair<float, float>> mPathing;
};

/**
 * AI command used to activate or execute a skill
 */
class AIUseSkillCommand : public AICommand
{
public:
    /**
     * Create a new use skill command for a skill not already activated
     * @param skillID ID of the skill being used
     * @param targetObjectID ID of the object being targeted by the skill
     */
    AIUseSkillCommand(uint32_t skillID, int64_t targetObjectID);

    /**
     * Create a new use skill command for a skill that has already been activated
     * @param activated Pointer to the ActivatedAbility of a skill to execute
     */
    AIUseSkillCommand(const std::shared_ptr<objects::ActivatedAbility>& activated);

    /**
     * Clean up the use skill command
     */
    virtual ~AIUseSkillCommand();

    /**
     * Get the ID of the skill to use
     * @return ID fo the skill to use
     */
    uint32_t GetSkillID() const;

    /**
     * Get the ID of the object that was targeted by the skill
     * @return ID of the object that was targeted by the skill
     */
    int64_t GetTargetObjectID() const;

    /**
     * Set the ActivatedAbility pointer after the skill has been activated
     * @param activated Pointer to the ActivatedAbility of a skill to execute
     */
    void SetActivatedAbility(const std::shared_ptr<
        objects::ActivatedAbility>& activated);

    /**
     * Get the pointer to the ActivatedAbility of a skill to execute
     * @return Pointer to the ActivatedAbility of a skill to execute
     */
    std::shared_ptr<objects::ActivatedAbility> GetActivatedAbility() const;

private:
    /// ID of the skill being used
    uint32_t mSkillID;

    /// ID of the object being targeted by the skill
    int64_t mTargetObjectID;

    /// Pointer to the ActivatedAbility of a skill to execute
    std::shared_ptr<objects::ActivatedAbility> mActivated;
};

/**
 * AI command used to execute a script function bound to an AIState
 */
class AIScriptedCommand : public AICommand
{
public:
    /**
     * Create a new scripted command
     * @param functionName Name of the script function to execute
     */
    AIScriptedCommand(const libcomp::String& functionName);

    /**
     * Clean up the scripted command
     */
    virtual ~AIScriptedCommand();

    /**
     * Get the name of the script function to execute
     * @return Name of the script function to execute
     */
    libcomp::String GetFunctionName() const;

private:
    /// Name of the script function to execute
    libcomp::String mFunctionName;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_AICOMMAND_H
