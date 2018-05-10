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
struct ActionContext;

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
     * @param zone Pointer to the current zone the action is being performed in
     * @param groupID Optional action group ID used for specific action logic
     */
    void PerformActions(const std::shared_ptr<ChannelClientConnection>& client,
        const std::list<std::shared_ptr<objects::Action>>& actions,
        int32_t sourceEntityID, const std::shared_ptr<Zone>& zone = nullptr,
        uint32_t groupID = 0);

private:
    /**
     * Start an event sequence for the client.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool StartEvent(ActionContext& ctx);

    /**
     * Perform the zone change action on behalf of the client. If
     * no zoneID is specified, they will be sent to their homepoint.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool ZoneChange(ActionContext& ctx);

    /**
     * Set the homepoint for the client character.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool SetHomepoint(ActionContext& ctx);

    /**
     * Change the state of the source entity in the zone.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool SetNPCState(ActionContext& ctx);

    /**
     * Add or remove items to the client character's inventory.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool AddRemoveItems(ActionContext& ctx);

    /**
     * Add or remove status effects to the client's character or
     * partner demon.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool AddRemoveStatus(ActionContext& ctx);

    /**
     * Add/remove demons from the COMP and/or set the max slots available.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool UpdateCOMP(ActionContext& ctx);

    /**
     * Grant XP to the source client character and/or partner demon.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool GrantXP(ActionContext& ctx);

    /**
     * Grant skills or skill points to the source client character
     * or partner demon.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool GrantSkills(ActionContext& ctx);

    /**
     * Display a message for the client that no response is returned from.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool DisplayMessage(ActionContext& ctx);

    /**
     * Display a stage effect for the client that no response is
     * returned from.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool StageEffect(ActionContext& ctx);

    /**
     * Display a special direction effect for the client.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool SpecialDirection(ActionContext& ctx);

    /**
     * Play or stop a BGM for the client.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool PlayBGM(ActionContext& ctx);

    /**
     * Play a sound effect for the client.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool PlaySoundEffect(ActionContext& ctx);

    /**
     * Update flags related to character maps, valuables or plugins.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool UpdateFlag(ActionContext& ctx);

    /**
     * Update the client character's LNC alignment.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool UpdateLNC(ActionContext& ctx);

    /**
     * Update various point values associated to the client.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool UpdatePoints(ActionContext& ctx);

    /**
     * Update a quest related to the current character.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool UpdateQuest(ActionContext& ctx);

    /**
     * Update one or more flags in the current zone.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool UpdateZoneFlags(ActionContext& ctx);

    /**
     * Update the instance related to the current character.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool UpdateZoneInstance(ActionContext& ctx);

    /**
     * Spawn an enemy spawn group by ID in the client's current zone.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool Spawn(ActionContext& ctx);

    /**
     * Create one or more loot boxes at the specified location.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool CreateLoot(ActionContext& ctx);

    /**
     * Move the current time trial results to the record set.
     * @param ctx ActionContext for the executing source information.
     * @param rewardItem Item obtained from the time trial
     * @param rewardItemCount Item count obtained form the time trial
     * @retval false if the trial records were not updated
     */
    bool RecordTimeTrial(ActionContext& ctx, uint32_t rewardItem,
        uint16_t rewardItemCount);

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;

    /// List of action parsers.
    libcomp::EnumMap<objects::Action::ActionType_t, std::function<bool(
        ActionManager&, ActionContext&)>> mActionHandlers;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ACTIONMANAGER_H
