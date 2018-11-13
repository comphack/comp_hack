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
class MiSkillData;
}

namespace channel
{

class Point;

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
 * This class is NOT thread safe but it should only be used by AIManager
 * which is responsible for locking.
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
     * Get the server time set when the command started
     * @return Server time in microseconds
     */
    uint64_t GetStartTime();

    /**
     * Set the command as having been started
     */
    void Start();

    /**
     * Get the ID of the entity targeted by the command
     * @return Entity ID targeted by the command or -1 if none set
     */
    int32_t GetTargetEntityID() const;

    /**
     * Set the ID of the entity targeted by the command
     * @param targetEntityID Entity ID targeted by the command or -1 if none
     */
    void SetTargetEntityID(int32_t targetEntityID);

protected:
    /// AI command type
    AICommandType_t mType;

    /// Command start time
    uint64_t mStartTime;

    /// Pre-process delay time in microseconds
    uint64_t mDelay;

    /// Entity ID being targeted by the command (can be unset as -1)
    int32_t mTargetEntityID;
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
     * Create a new move command with a targeted entity in mind
     * @param targetEntityID ID of the entity being targeted by the movement
     * @param minimumDistance Minimum distance required before the movement
     *  is complete
     * @param maximumDistance Maximum distance the move command will attempt
     *  to move before quitting
     */
    AIMoveCommand(int32_t targetEntityID, float minimumDistance,
        float maximumDistance = 0.f);

    /**
     * Clean up the move command
     */
    virtual ~AIMoveCommand();

    /**
     * Set the pathing information for the move command, using the last
     * point as the destination
     * @param pathing List of sequential points defining a movement path
     */
    std::list<Point> GetPathing() const;

    /**
     * Set the pathing information for the move command, using the last
     * point as the destination
     * @param pathing List of sequential points defining a movement path
     */
    void SetPathing(const std::list<Point>& pathing);

    /**
     * Get the x and y coordinates of the point in the pathing that will
     * be moved to next
     * @param dest Output parameter for the next movement point's coordinates
     * @return true if a current destination exists, false if one does not
     */
    bool GetCurrentDestination(Point& dest) const;

    /**
     * Get the x and y coordinates of the point at the end of the pathing
     * @param dest Output parameter for the next movement point's coordinates
     * @return true if the end destination exists, false if one does not
     */
    bool GetEndDestination(Point& dest) const;

    /**
     * Remove the current destination from the pathing and move on to the next
     * @return true if another destination exists in the pathing, false if no
     *  more points exist
     */
    bool SetNextDestination();

    /**
     * Get the min target distance required to consider the movement
     * complete or max distance it can get away before stopping
     * @param min If true, get the min, else get the max
     * @return Min/max target distance
     */
    float GetTargetDistance(bool min) const;

    /**
     * Set the min target distance required to consider the movement
     * complete or max distance it can get away before stopping
     * @param distance Min/max target distance
     * @param min If true, set the min, else set the max
     */
    void SetTargetDistance(float distance, bool min);

private:
    /// List of sequential points defining a movement path
    std::list<Point> mPathing;

    /// Maximum distance the move command will attempt to move (minus pathing)
    /// before quitting
    float mMaximumTargetDistance;

    /// Minimum distance the move command requires with the target before it
    /// is considered complete
    float mMinimumTargetDistance;
};

/**
 * AI command used to activate or execute a skill
 */
class AIUseSkillCommand : public AICommand
{
public:
    /**
     * Create a new use skill command for a skill not already activated
     * @param skillData Pointer to the definition of the skill being used
     * @param targetEntityID ID of the entity being targeted by the skill
     */
    AIUseSkillCommand(const std::shared_ptr<objects::MiSkillData>& skillData,
        int32_t targetEntityID);

    /**
     * Create a new use skill command for a skill that has already been activated
     * @param skillData Pointer to the definition of the skill being used
     * @param activated Pointer to the ActivatedAbility of a skill to execute
     */
    AIUseSkillCommand(const std::shared_ptr<objects::MiSkillData>& skillData,
        const std::shared_ptr<objects::ActivatedAbility>& activated);

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
     * Get the definition of the skill to use
     * @return Definition fo the skill to use
     */
    std::shared_ptr<objects::MiSkillData> GetSkillData() const;

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
    /// Definition of the skill being used
    std::shared_ptr<objects::MiSkillData> mSkillData;

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
