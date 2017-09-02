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
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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
class EnemyState;
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
     * @return true on sucess, false upon failure
     */
    bool Prepare(const std::shared_ptr<ActiveEntityState>& eState,
        const libcomp::String& aiType = "");

    /**
     * Update the AI state of all active AI controlled entities in the
     * specified zone
     * @param zone Pointer to the zone to update
     * @param now Current server time when the 
     */
    void UpdateActiveStates(const std::shared_ptr<Zone>& zone, uint64_t now);

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
     * @return true if the entity state should be communicated to the zone,
     *  false otherwise
     */
    bool UpdateState(const std::shared_ptr<ActiveEntityState>& eState, uint64_t now);

    /**
     * Update the state of an enemy, processing AI directly or queuing commands
     * to be procssed on next update
     * @param eState Pointer to the enemy state to update
     * @param now Current timestamp of the server
     * @return true if the enemy state should be communicated to the zone,
     *  false otherwise
     */
    bool UpdateEnemyState(const std::shared_ptr<EnemyState>& eState, uint64_t now);

    /**
     * Refresh the skill map of the AIState if needed
     * @param eState Pointer to the entity state to update
     * @param aiState Pointer to the AIState
     */
    void RefreshSkillMap(const std::shared_ptr<ActiveEntityState>& eState,
        const std::shared_ptr<AIState>& aiState);

    /**
     * Execute a function on the script bound to an entity's AI state and return the
     * result
     * @param eState Pointer to the AI controlled entity state
     * @param functionName Name of the script function to execute
     * @param now Current timestamp of the server
     * @param result Output parameter to store the script return result in
     * @result true if the function exists and returned a result, false if it did not
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
     * @param source Starting point
     * @param dest End point
     * @param reduce Reduces the final movement path by a set amount so the entity
     *  ends up that amount of units away
     * @return Pointer to the new move command
     */
    std::shared_ptr<AIMoveCommand> GetMoveCommand(const std::shared_ptr<Zone>& zone,
        const Point& source, const Point& dest, float reduce = 0.f) const;

    /**
     * Get a new wait command
     * @param waitTime Number of seconds the entity should wait when
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
