/**
 * @file server/channel/src/ActionManager.h
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage actions when triggering a spot or interacting with
 * an object/NPC.
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

#ifndef SERVER_CHANNEL_SRC_ACTIONMANAGER_H
#define SERVER_CHANNEL_SRC_ACTIONMANAGER_H

// libcomp Includes
#include <EnumMap.h>

// object Includes
#include <Action.h>

// channel Includes
#include "ChannelClientConnection.h"

namespace channel
{

class ChannelServer;

/**
 * Class to manage actions when triggering a spot or interacting with
 * an object/NPC.
 */
class ActionManager
{
public:
    /**
     * Create a new ActionManager.
     * @param server Pointer back to the channel server this belongs to.
     */
    ActionManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the ActionManager.
     */
    ~ActionManager();

    /**
     * Perform the list of actions on behalf of the client.
     * @param client Client to perform the actions for.
     * @param actions List of actions to perform.
     * @param sourceEntityID ID of the entity performing the actions.
     */
    void PerformActions(const std::shared_ptr<ChannelClientConnection>& client,
        const std::list<std::shared_ptr<objects::Action>>& actions,
        int32_t sourceEntityID);

private:
    /**
     * Start an event sequence for the client.
     * @param client Client to perform the actions for.
     * @param action Action to perform.
     * @param sourceEntityID ID of the entity to use as the event source.
     * @retval false The action list should stop after this action.
     */
    bool StartEvent(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID);

    /**
     * Perform the zone change action on behalf of the client.
     * @param client Client to perform the actions for.
     * @param action Action to perform.
     * @param sourceEntityID ID of the entity causing the zone change.
     * @retval false The action list should stop after this action.
     */
    bool ZoneChange(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID);

    /**
     * Change the state of the source entity in the zone.
     * @param client Client to perform the actions for.
     * @param action Action to perform.
     * @param sourceEntityID ID of the entity that is changing state.
     * @retval false The action list should stop after this action.
     */
    bool SetNPCState(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID);

    /**
     * Update flags related to character maps, valuables or plugins.
     * @param client Client to perform the actions for.
     * @param action Action to perform.
     * @param sourceEntityID ID of the source entity.
     * @retval false The action list should stop after this action.
     */
    bool UpdateFlag(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID);

    /**
     * Update the client character's LNC alignment.
     * @param client Client to perform the actions for.
     * @param action Action to perform.
     * @param sourceEntityID ID of the source entity.
     * @retval false The action list should stop after this action.
     */
    bool UpdateLNC(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID);

    /**
     * Update a quest related to the current character.
     * @param client Client to perform the actions for.
     * @param action Action to perform.
     * @param sourceEntityID ID of the source entity.
     * @retval false The action list should stop after this action.
     */
    bool UpdateQuest(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID);

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;

    /// List of action parsers.
    libcomp::EnumMap<objects::Action::ActionType_t, std::function<bool(
        ActionManager&,
        const std::shared_ptr<channel::ChannelClientConnection>&,
        const std::shared_ptr<objects::Action>&, int32_t)>> mActionHandlers;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ACTIONMANAGER_H
