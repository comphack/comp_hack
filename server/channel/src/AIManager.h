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

#ifndef SERVER_CHANNEL_SRC_AIMANAGER_H
#define SERVER_CHANNEL_SRC_AIMANAGER_H

// channel Includes
#include "ActiveEntityState.h"
#include "AIState.h"
#include "ClientState.h"

namespace libcomp
{
class ScriptEngine;
}

namespace channel
{

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
class AIManager
{
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
     * @param baseAIType AI type to use for the base demon AI or zero
     *  to use the default for the demon type
     * @param aggression Aggression level of a the AI, defaults to 100
     * @return true on sucess, false upon failure
     */
    bool Prepare(const std::shared_ptr<ActiveEntityState>& eState,
        const libcomp::String& aiType, uint16_t baseAIType = 0,
        uint8_t aggression = 100);

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
     * Queue a script command on the specified AIState
     * @param aiState Pointer to the AIState
     * @param functionName Name of the script function to be executed when
     *  the command is processed
     */
    void QueueScriptCommand(const std::shared_ptr<AIState> aiState,
        const libcomp::String& functionName);

    /**
     * Queue a wait command on the specified AIState
     * @param aiState Pointer to the AIState
     * @param waitTime Number of seconds the entity should wait when
     *  the command is processed
     */
    void QueueWaitCommand(const std::shared_ptr<AIState> aiState,
        uint32_t waitTime);

    /**
     * Set the entity's destination position based on the supplied
     * values and uses the current position values to set the origin.
     * Communicating that the move has taken place must be done elsewhere.
     * @param xPos X position to move to
     * @param yPos Y position to move to
     * @param now Server time to use as the origin ticks
     */
    void Move(const std::shared_ptr<ActiveEntityState>& eState,
        float xPos, float yPos, uint64_t now);
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
    bool UpdateState(const std::shared_ptr<ActiveEntityState>& eState, uint64_t now,
        bool isNight);

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
        const std::shared_ptr<objects::EnemyBase>& eBase, uint64_t now,
        bool isNight);

    /**
     * Handle normal wander actions for the supplied AI controlled enemy
     * based entity
     * @param eState Pointer to the entity state to update
     * @param eBase Pointer to the enemy base object from the entity
     */
    void Wander(const std::shared_ptr<ActiveEntityState>& eState,
        const std::shared_ptr<objects::EnemyBase>& eBase);

    /**
     * Clear an entity's AIState current target and find the next target to focus on
     * @param state Pointer to the AI controlled entity state
     * @param now Current timestamp of the server
     * @param isNight Night time indicator which affects targetting
     * @return Pointer to the entity being targeted
     */
    std::shared_ptr<ActiveEntityState> Retarget(
        const std::shared_ptr<ActiveEntityState>& state,
        uint64_t now, bool isNight);

    /**
     * Refresh the skill map of the AIState if needed
     * @param eState Pointer to the entity state to update
     * @param aiState Pointer to the AIState
     */
    void RefreshSkillMap(const std::shared_ptr<ActiveEntityState>& eState,
        const std::shared_ptr<AIState>& aiState);

    /**
     * Determine which skill the entity will use next based on their current
     * state
     * @param eState Pointer to the entity state
     * @return true if a skill was selected, false if none can be used
     */
    bool PrepareSkillUsage(const std::shared_ptr<ActiveEntityState>& state);

    /**
     * Execute a function on the script bound to an entity's AI state and return the
     * result
     * @param eState Pointer to the AI controlled entity state
     * @param functionName Name of the script function to execute
     * @param now Current timestamp of the server
     * @param result Output parameter to store the script return result in
     * @return true if the function exists and returned a result, false if it did not
     */
    template<class T> bool ExecuteScriptFunction(const std::shared_ptr<
        ActiveEntityState>& eState, const libcomp::String& functionName, uint64_t now,
        T& result)
    {
        auto aiState = eState->GetAIState();
        auto script = aiState->GetScript();
        if(!script)
        {
            return false;
        }

        Sqrat::Function f(Sqrat::RootTable(script->GetVM()), functionName.C());

        auto scriptResult = !f.IsNull() ? f.Evaluate<T>(eState, this, now) : 0;
        if(!scriptResult)
        {
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
     * @return Pointer to the new move command
     */
    std::shared_ptr<AIMoveCommand> GetMoveCommand(
        const std::shared_ptr<ActiveEntityState>& eState,
        const Point& dest, float reduce = 0.f, bool split = true) const;

    /**
     * Get a new wait command
     * @param waitTime Number of milliseconds the entity should wait when
     *  the command is processed
     * @return Pointer to the new move command
     */
    std::shared_ptr<AICommand> GetWaitCommand(uint32_t waitTime) const;

    /// Static map of scripts that have been loaded and compiled
    /// by AI type name
    static std::unordered_map<std::string,
        std::shared_ptr<libcomp::ScriptEngine>> sPreparedScripts;

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_AIMANAGER_H
