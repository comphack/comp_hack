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
#include <ScriptEngine.h>

// object Includes
#include <Action.h>

// channel Includes
#include "ChannelClientConnection.h"

namespace libcomp
{
class ScriptEngine;
}

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
     * @param zone Pointer to the current zone the action is being performed in
     * @param groupID Optional action group ID used for specific action logic
     * @param autoEventsOnly Optional parameter to force an auto-only context
     *  when processing events. Does not apply when context switching.
     */
    void PerformActions(const std::shared_ptr<ChannelClientConnection>& client,
        const std::list<std::shared_ptr<objects::Action>>& actions,
        int32_t sourceEntityID, const std::shared_ptr<Zone>& zone = nullptr,
        uint32_t groupID = 0, bool autoEventsOnly = false);

private:
    struct ActionContext
    {
        std::shared_ptr<ChannelClientConnection> Client;
        std::shared_ptr<objects::Action> Action;
        int32_t SourceEntityID = 0;
        uint32_t GroupID = 0;
        std::shared_ptr<Zone> CurrentZone;
        bool AutoEventsOnly = false;
    };

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
     * Delay action execution or flag an action for delayed usage at
     * a later time.
     * @param ctx ActionContext for the executing source information.
     * @retval false The action list should stop after this action.
     */
    bool Delay(ActionContext& ctx);

    /**
     * Move the current time trial results to the record set.
     * @param ctx ActionContext for the executing source information.
     * @param rewardItem Item obtained from the time trial
     * @param rewardItemCount Item count obtained form the time trial
     * @retval false if the trial records were not updated
     */
    bool RecordTimeTrial(ActionContext& ctx, uint32_t rewardItem,
        uint16_t rewardItemCount);

    /**
     * Get the action from the supplied context converted to the proper type.
     * If the action is configured with a transformation script, a transformed
     * copy will be returned and set on the context.
     * @param ctx ActionContext for the executing source information.
     * @param requireClient Check that the client is on the context if supplied
     *  and fail if it is not
     * @return true on success, false on failure
     */
    template <class T>
    std::shared_ptr<T> GetAction(ActionContext& ctx, bool requireClient)
    {
        if(requireClient && !VerifyClient(ctx, typeid(T).name()))
        {
            return nullptr;
        }

        auto act = ctx.Action;
        auto ptr = std::dynamic_pointer_cast<T>(ctx.Action);
        if(ptr && !ptr->GetTransformScriptID().IsEmpty())
        {
            // Make a copy and transform
            ptr = std::make_shared<T>(*ptr);

            auto engine = std::make_shared<libcomp::ScriptEngine>();
            engine->Using<T>();
            if(PrepareTransformScript(ctx, engine))
            {
                // Store the action for transformation
                Sqrat::Function f(Sqrat::RootTable(engine->GetVM()),
                    "prepare");
                auto scriptResult = !f.IsNull() ? f.Evaluate<int32_t>(ptr) : 0;

                // Apply the transformation
                if(scriptResult && *scriptResult == 0 &&
                    TransformAction(ctx, engine))
                {
                    // Set new action
                    ctx.Action = ptr;
                    return ptr;
                }
            }

            // Return failure
            return nullptr;
        }

        return ptr;
    }

    /**
     * Verify that the client is on the context and print an error message
     * if it is not.
     * @param ctx ActionContext for the executing source information
     * @param typeName Name of the action type being verified
     * @return true if the client exists, false if it does not
     */
    bool VerifyClient(ActionContext& ctx, const libcomp::String& typeName);

    /**
     * Prepare the transformation script from the action on the supplied
     * script engine.
     * @param ctx ActionContext for the executing source information
     * @param engine Script engine to prepare the script for
     * @return true on success, false on failure
     */
    bool PrepareTransformScript(ActionContext& ctx,
        std::shared_ptr<libcomp::ScriptEngine> engine);

    /**
     * Finish preparing and execute the tranformation script configured
     * for the action.
     * @param ctx ActionContext for the executing source information
     * @param engine Script engine to execute the script using
     * @return true on success, false on failure
     */
    bool TransformAction(ActionContext& ctx,
        std::shared_ptr<libcomp::ScriptEngine> engine);

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;

    /// List of action parsers.
    libcomp::EnumMap<objects::Action::ActionType_t, std::function<bool(
        ActionManager&, ActionContext&)>> mActionHandlers;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ACTIONMANAGER_H
