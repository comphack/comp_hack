/**
 * @file server/channel/src/EnemyState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an enemy on the channel.
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

#ifndef SERVER_CHANNEL_SRC_ENEMYSTATE_H
#define SERVER_CHANNEL_SRC_ENEMYSTATE_H

// objects Includes
#include <ActiveEntityState.h>
#include <Enemy.h>

#include <ScriptEngine.h>

namespace libcomp
{
class ServerDataManager;
}

namespace channel
{

/**
 * Contains the state of an enemy related to a channel as well
 * as functionality to be used by the scripting engine for AI.
 */
class EnemyState : public ActiveEntityStateImp<objects::Enemy>
{
public:
    /**
     * Create a new enemy state.
     */
    EnemyState();

    /**
     * Clean up the enemy state.
     */
    virtual ~EnemyState() { }

    /**
     * Prepare the enemy for use following the setting of all necessary
     * data.
     * @param self Pointer back to the enemy state
     * @param aiType AI script type to bind to the enemy
     * @param dataManager Pointer to the server's data manager for loading
     *  the AI script new if needed
     * @return true on sucess false if something failed
     */
    bool Prepare(const std::weak_ptr<EnemyState>& self,
        const libcomp::String& aiType, libcomp::ServerDataManager* dataManager);

    /**
     * Update the state of the enemy, processing AI and performing other
     * related actions.
     * @param now Current timestamp of the server
     * @return true on success, false otherwise
     */
    bool UpdateState(uint64_t now);

    /**
     * Retrieves a timestamp associated to an enemy specific action.
     * @param action Name of the action to retrieve information from
     * @return Timestamp associated to the action or 0 if not found
     */
    uint64_t GetActionTime(const libcomp::String& action);

    /**
     * Stores a timestamp associated to an enemy specific action.
     * @param action Name of the action to store
     * @param time Timestamp of the specified action
     */
    void SetActionTime(const libcomp::String& action, uint64_t time);

private:
    /// Static map of scripts that have been loaded and compiled
    /// by AI type name
    static std::unordered_map<std::string,
        std::shared_ptr<libcomp::ScriptEngine>> sPreparedScripts;

    /// Map of timestamps associated to enemy specific actions
    std::unordered_map<std::string, uint64_t> mActionTimes;

    /// Pointer to the AI script bound to the enemy
    std::shared_ptr<libcomp::ScriptEngine> mAIScript;

    /// Pointer back to the entity
    std::weak_ptr<EnemyState> mSelf;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ENEMYSTATE_H
