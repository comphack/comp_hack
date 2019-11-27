/**
 * @file server/channel/src/EventManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages the execution and processing of events as well
 *  as quest phase progression and condition evaluation.
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

#ifndef SERVER_CHANNEL_SRC_EVENTMANAGER_H
#define SERVER_CHANNEL_SRC_EVENTMANAGER_H

// libcomp Includes
#include <EnumMap.h>
#include <ScriptEngine.h>

// object Includes
#include <DemonQuest.h>
#include <Event.h>
#include <EventCondition.h>
#include <EventInstance.h>

// channel Includes
#include "ChannelClientConnection.h"

namespace libcomp
{
class ScriptEngine;
}

namespace objects
{
class EventConditionData;
class EventFlagCondition;
class EventInstance;
}

typedef objects::EventCondition::CompareMode_t EventCompareMode;

namespace channel
{

class ChannelServer;

/**
 * Struct containing optional parameters supplied to EventManager::HandleEvent
 * to simplify the function signature.
 */
struct EventOptions
{
    // Action group ID, set when performing a "start event" action so any later
    // sets can pick up where the others left off
    uint32_t ActionGroupID = 0;

    // Force an auto-only context, regardless of its the client is specified
    bool AutoOnly = false;

    // Disallow interruption of any events in the set. Events that are queued
    // but not started can still be interrupted if another is active.
    bool NoInterrupt = false;

    // Override any transform script params on the first event being handled.
    // If the event is not a transform event, these will be ignored.
    std::list<libcomp::String> TransformScriptParams;
};

/**
 * Manager class in charge of processing event sequences as well as quest
 * phase progression and condition evaluation. Events include things like
 * NPC dialogue, player choice prompts, cinematics and context sensitive
 * menus.  Events can be strung together and can progress as well as pop
 * back to previous events much like a dialogue tree.
 */
class EventManager
{
public:
    /**
     * Create a new EventManager.
     * @param server Pointer back to the channel server this belongs to
     */
    EventManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the EventManager.
     */
    ~EventManager();

    /**
     * Handle a new event based upon the supplied ID, relative to an
     * optional entity
     * @param client Pointer to the client the event affects, can be null
     * @param eventID ID of the event to handle
     * @param sourceEntityID Optional source of an event to focus on
     * @param zone Pointer to the zone where the event is executing. If
     *  the client is specified this will be overridden
     * @param options Optional parameters to modify event execution
     * @return true on success, false on failure
     */
    bool HandleEvent(const std::shared_ptr<ChannelClientConnection>& client,
        const libcomp::String& eventID, int32_t sourceEntityID,
        const std::shared_ptr<Zone>& zone = nullptr,
        EventOptions options = {});

    /**
     * Prepare a new event based upon the supplied ID, relative to an
     * optional entity
     * @param eventID ID of the event to handle
     * @param sourceEntityID Optional source of an event to focus on
     * @return Pointer to the new EventInstance or nullptr on failure
     */
    std::shared_ptr<objects::EventInstance> PrepareEvent(
        const libcomp::String& eventID, int32_t sourceEntityID);

    /**
     * Start a placeholder "system" event that does not end until explicitly
     * requested. Useful for certain actions that lock the player in place until
     * they automatically complete.
     * @param client Pointer to the client the event affects
     * @param sourceEntityID Optional source of an event to focus on
     * @return true if the event was started, false if it could not
     */
    bool StartSystemEvent(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t sourceEntityID);

    /**
     * Stop the client's current event and return the source entity ID if
     * one existed
     * @param client Pointer to the client
     * @return Source entity ID of the interrupted event
     */
    int32_t InterruptEvent(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Request an open menu event be started for the supplied client
     * @param client Pointer to the client connection
     * @param menuType Type of menu being opened
     * @param shopID Optional shop ID of the menu being opened
     * @param sourceEntityID Optional source of an event to focus on
     * @param allowInsert If true the menu can be requested with an event
     *  currently active. If false an an active event causes a failure.
     * @return true if request was sent to the client, false otherwise
     */
    bool RequestMenu(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t menuType, int32_t shopID = 0, int32_t sourceEntityID = 0,
        bool allowInsert = false);

    /**
     * React to a response based on the current event of a client
     * @param client Pointer to the client sending the response
     * @param responseID Value representing the player's response contextual
     *  to the current event type (or -1 for a menu "next" enable flag)
     * @param objectID Optional object ID selected as part of the client
     *  response
     * @return true on success, false on failure
     */
    bool HandleResponse(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t responseID, int64_t objectID = -1);

    /**
     * Set the supplied client's ChannelLogin active event and event index
     * so the event chain can be continued when they arrive at the other
     * channel server
     * @param client Pointer to the client connection
     * @return true on success, false on failure
     */
    bool SetChannelLoginEvent(
        const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Continue an event that was in progress when the supplied client changed
     * from another channel to the current one.
     * @param client Pointer to the client connection
     * @return true if an event was started, false if no event needed to
     *  be performed
     */
    bool ContinueChannelChangeEvent(
        const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Start, update or complete a quest based upon the quest ID and phase
     * supplied. Restrictions are enforced to disallow skipping phases, etc.
     * @param client Pointer to the client the quest is associated to
     * @param questID ID of the quest being updated
     * @param phase Phase to update the quest to. If 0 is supplied, the quest
     *  will be started. If -1 is supplied, the quest will be completed.
     *  Otherwise the quest will move to that phase.
     * @param forceUpdate Optional parameter to bypass quest state based
     *  restrictions and force the quest into the specified state
     * @param updateFlags Optional parameter to set flags on the next or current
     *  phase
     * @return true on success (or no phase update needed), false on failure
     */
    bool UpdateQuest(const std::shared_ptr<ChannelClientConnection>& client,
        int16_t questID, int8_t phase, bool forceUpdate = false,
        const std::unordered_map<int32_t, int32_t>& updateFlags = {});

    /**
     * Update the client's quest kill counts (normal and demon)
     * @param client Pointer to the client with active kill quests
     * @param kills Map of demon types to the number killed
     */
    void UpdateQuestKillCount(const std::shared_ptr<ChannelClientConnection>& client,
        const std::unordered_map<uint32_t, int32_t>& kills);

    /**
     * Update the registered set of enemy types that need to be killed to
     * complete the current quests for the supplied client
     * @param client Pointer to the client to update
     */
    void UpdateQuestTargetEnemies(
        const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Send the client's active quest list
     * @param client Pointer to the client
     */
    void SendActiveQuestList(
        const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Send the client's completed quest list
     * @param client Pointer to the client
     */
    void SendCompletedQuestList(
        const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Generate a demon quest for the supplied character and demon
     * @param cState Pointer to the character state
     * @param demon Pointer to the demon that will have the quest
     * @return Pointer to the newly generated demon quest, null if an error
     *  occurred
     */
    std::shared_ptr<objects::DemonQuest> GenerateDemonQuest(
        const std::shared_ptr<CharacterState>& cState,
        const std::shared_ptr<objects::Demon>& demon);

    /**
     * Update the target count for the client's active demon quest if it
     * matches the supplied quest type.
     * @param client Pointer to the client
     * @param questType Type of quest required to perform the update
     * @param targetType Type of target that has changed. For item quests
     *  supplying no value will recalculate the count based on the current
     *  item counts in the inventory
     * @param increment Value to increase the target count by
     * @return true if any updates occurred
     */
    bool UpdateDemonQuestCount(const std::shared_ptr<ChannelClientConnection>& client,
        objects::DemonQuest::Type_t questType, uint32_t targetType = 0,
        int32_t increment = 0);

    /**
     * Reset the quests available in from the demons in the COMP and set the
     * demon quest daily count back to zero. The caller is responsible for
     * actually saving the changes that are written to the supplied changeset.
     * @param client Pointer to the client
     * @param now Current system time to be set on the CharacterProgress
     *  records when reseting the quests
     * @param changes Pointer to the changeset to apply the changes to
     * @return true if one or more demon had quests reset, false if only
     *  the time reset occurred
     */
    bool ResetDemonQuests(const std::shared_ptr<
        ChannelClientConnection>& client, uint32_t now,
        const std::shared_ptr<libcomp::DatabaseChangeSet>& changes);

    /**
     * End the client's demon quest in success or failure. If any costs are
     * required to complete a quest, they will be paid here.
     * @param client Pointer to the client
     * @param failCode Specifies how the quest should be notified to the player
     *  should it end in failure. Values include:
     *  0) Successful completion
     *  1) Quest has timed out
     *  2) Quest has failed for a miscellaneous reason
     * @return Failure code matching the input parameter including any reasons
     *  why a requested success is actually a failure. If a non-zero value is
     *  returned, no update was performed
     */
    int8_t EndDemonQuest(const std::shared_ptr<
        ChannelClientConnection>& client, int8_t failCode = 2);

    /**
     * Handle an event instance by branching into the appropriate handler
     * function after updating the character's overhead icon if needed
     * @param client Pointer to the client the event affects
     * @param instance Event instance to handle
     * @return true on success, false on failure
     */
    bool HandleEvent(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Start a pending web-game session with a session ID received from the
     * world server. Session IDs are assumed to have been received only once
     * both the world and lobby have accepted the channel's request as valid.
     * @param client Pointer to the client connection
     * @param sessionID Session Id received from the world server
     */
    void StartWebGame(const std::shared_ptr<ChannelClientConnection>& client,
        const libcomp::String& sessionID);

    /**
     * End any web-game session currently active for the client
     * @param client Pointer to the client connection
     * @param notifyWorld Specifies if the world should be notified that the
     *  session is ending. Should only ever be false if the world requested
     *  that the session be ended
     */
    void EndWebGame(const std::shared_ptr<ChannelClientConnection>& client,
        bool notifyWorld);

    /**
     * Evaluate a list of event conditions for a client
     * @param zone Zone to evaluate the conditions in
     * @param conditions Event conditions to evaluate
     * @param client Optional pointer to the client connection
     * @return true if the event conditions evaluate to true, otherwise false
     */
    bool EvaluateEventConditions(
        const std::shared_ptr<Zone>& zone,
        const std::list<std::shared_ptr<objects::EventCondition>>& conditions,
        const std::shared_ptr<ChannelClientConnection>& client = nullptr);

private:
    struct EventContext
    {
        std::shared_ptr<ChannelClientConnection> Client;
        std::shared_ptr<Zone> CurrentZone;
        std::shared_ptr<objects::EventInstance> EventInstance;
        std::list<libcomp::String> TransformScriptParams;
        bool AutoOnly = false;
    };

    /**
     * Handle an event instance by branching into the appropriate handler
     * function after updating the character's overhead icon if needed
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool HandleEvent(EventContext& ctx);

    /**
     * Set the overhead status icon of the client entity to indicate they are
     * involved in an event
     * @param client Pointer to the client connection
     */
    void SetEventStatus(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Evaluate each of the valid condition sets required to start a quest
     * @param ctx Execution context of the event
     * @param questID ID of the quest being evaluated
     * @return true if the any of the condition sets all evaluated to true, false
     *  if no set's conditions all evaluated to true
     */
    bool EvaluateQuestConditions(EventContext& ctx, int16_t questID);

    /**
     * Evaluate an event condition
     * @param ctx Execution context of the event
     * @param condition Event condition to evaluate
     * @return true if the event condition evaluates to true, otherwise false
     */
    bool EvaluateEventCondition(EventContext& ctx,
        const std::shared_ptr<objects::EventCondition>& condition);

    /**
     * Evaluate a list of event conditions
     * @param ctx Execution context of the event
     * @param conditions Event conditions to evaluate
     * @return true if the event conditions evaluate to true, otherwise false
     */
    bool EvaluateEventConditions(EventContext& ctx,
        const std::list<std::shared_ptr<objects::EventCondition>>& conditions);

    /**
     * Evaluate a standard event condition
     * @param ctx Execution context of the event
     * @param eState Pointer to the evaluating entity state for the context.
     *  Can be null
     * @param condition Standard condition to evaluate
     * @return true if standard condition evaluates to true, otherwise false
     */
    bool EvaluateCondition(EventContext& ctx,
        const std::shared_ptr<ActiveEntityState>& eState,
        const std::shared_ptr<objects::EventConditionData>& condition,
        EventCompareMode compareMode = EventCompareMode::DEFAULT_COMPARE);

    /**
     * Evaluate each of the requirements to complete the current quest phase
     * @param client Pointer to the client to evaluate contextually to
     * @param questID ID of the quest to check
     * @param phase Current phase ID, acts as assurance that the right phase
     *  is being checked
     * @return true if all of the current phase's completion requirements have
     *  been met, false if any have not been met or the phase supplied is not
     *  the current phase
     */
    bool EvaluateQuestPhaseRequirements(const std::shared_ptr<
        ChannelClientConnection>& client, int16_t questID, int8_t phase);

    /**
     * Evaluate a partner demon based event condition
     * @param client Pointer to the client to evaluate contextually to
     * @param condition Event condition to evaluate
     * @return true if the event condition evaluates to true, otherwise false
     */
    bool EvaluatePartnerCondition(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventCondition>& condition);

    /**
     * Evaluate a quest based event condition
     * @param ctx Execution context of the event
     * @param condition Event condition to evaluate
     * @return true if the event condition evaluates to true, otherwise false
     */
    bool EvaluateQuestCondition(EventContext& ctx,
        const std::shared_ptr<objects::EventCondition>& condition);

    /**
     * Evaluate a set of flag states for a standard condition
     * @param flagStates Map of flag states to evaluate
     * @param condition Condition to evaluate
     * @return true if the flag states evaluate to true, otherwise false
     */
    bool EvaluateFlagStates(const std::unordered_map<int32_t, int32_t>& flagStates,
        const std::shared_ptr<objects::EventFlagCondition>& condition);

    /**
     * Compare values using the supplied compare mode from an event
     * condition
     * @param value1 First (or LHS) comparison value
     * @param value2 Second (or RHS) comparison value
     * @param value3 Third optional comparison value (mostly used for
     *  "between" comparisons)
     * @param compareMode Compare mode to use when evaluating the values
     * @param defaultCompare Default compare mode to use if the compareMode
     *  parameter evaluates to DEFAULT_COMPARE. If both equal this value
     *  the comparison automatically fails
     * @param validCompareSetting Valid comparison flags corresponding to
     *  the numeric values associated to EventCompareMode
     * @return true if the condition evalaultes to true, false if the condition
     *  evaluates to false or if an error occurred
     */
    bool Compare(int32_t value1, int32_t value2, int32_t value3,
        EventCompareMode compareMode, EventCompareMode defaultCompare =
        EventCompareMode::DEFAULT_COMPARE, uint16_t validCompareSetting = 63);

    /**
     * Calculate and add all rewards to the supplied demon quest
     * @param cState Pointer to the character state
     * @param demon Pointer to the demon with the quest
     * @param dQuest Pointer to the demon quest
     */
    void AddDemonQuestRewards(const std::shared_ptr<CharacterState>& cState,
        const std::shared_ptr<objects::Demon>& demon,
        std::shared_ptr<objects::DemonQuest>& dQuest);

    /**
     * Progress or pop to the next sequential event if one exists. If
     * no event is found, the event sequence is ended.
     * @param ctx Execution context of the event
     */
    void HandleNext(EventContext& ctx);

    /**
     * Send an NPC based message to the client
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool NPCMessage(EventContext& ctx);

    /**
     * Send an extended NPC based message to the client
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool ExNPCMessage(EventContext& ctx);

    /**
     * Start a multitalk event clientside
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool Multitalk(EventContext& ctx);

    /**
     * Prompt the player to choose from set options
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool Prompt(EventContext& ctx);

    /**
     * Play an in-game cinematic clientside
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool PlayScene(EventContext& ctx);

    /**
     * Open a menu to the player representing various facilities
     * and functions
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool OpenMenu(EventContext& ctx);

    /**
     * Perform one or many actions from the same types of options
     * configured for object interaction
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool PerformActions(EventContext& ctx);

    /**
     * Signify a direction to the player
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool Direction(EventContext& ctx);

    /**
     * Send the player an I-Time request
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    bool ITime(EventContext& ctx);

    /**
     * End the current event sequence
     * @param client Pointer to the client the event affects
     * @return true on success, false on failure
     */
    bool EndEvent(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Execute the appropriate (party) tri-fusion event responses
     * @param client Pointer to the client the event affects
     * @return true on success, false on failure
     */
    bool HandleTriFusion(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Handle all web-game setup steps if no session already exists
     * @param client Pointer to the client the event affects
     * @return true if the session was already active, false if it was created
     */
    bool HandleWebGame(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Get the event from the supplied context converted to the proper type.
     * If the event is configured with a transformation script, a transformed
     * copy will be returned and set on the context.
     * @param ctx Execution context of the event
     * @return true on success, false on failure
     */
    template <class T>
    std::shared_ptr<T> GetEvent(EventContext& ctx)
    {
        auto inst = ctx.EventInstance;
        auto e = inst->GetEvent();
        auto ptr = std::dynamic_pointer_cast<T>(e);
        if(ptr && !ptr->GetTransformScriptID().IsEmpty())
        {
            // Make a copy and transform
            ptr = std::make_shared<T>(*ptr);

            auto engine = std::make_shared<libcomp::ScriptEngine>();
            engine->Using<T>();
            if(PrepareTransformScript(ctx, engine))
            {
                // Store the event for transformation
                Sqrat::Function f(Sqrat::RootTable(engine->GetVM()),
                    "prepare");
                auto scriptResult = !f.IsNull() ? f.Evaluate<int32_t>(ptr) : 0;

                // Apply the transformation
                if(scriptResult && *scriptResult == 0 &&
                    TransformEvent(ctx, engine) && VerifyITime(ctx, ptr))
                {
                    // Set new event
                    inst->SetEvent(ptr);
                    return ptr;
                }
            }

            // Return failure
            return nullptr;
        }

        return VerifyITime(ctx, ptr) ? ptr : nullptr;
    }

    /**
     * Prepare the transformation script from the event on the supplied
     * script engine.
     * @param ctx Execution context of the event
     * @param engine Script engine to prepare the script for
     * @return true on success, false on failure
     */
    bool PrepareTransformScript(EventContext& ctx,
        std::shared_ptr<libcomp::ScriptEngine> engine);

    /**
     * Finish preparing and execute the tranformation script configured
     * for the event.
     * @param ctx Execution context of the event
     * @param engine Script engine to execute the script using
     * @return true on success, false on failure
     */
    bool TransformEvent(EventContext& ctx,
        std::shared_ptr<libcomp::ScriptEngine> engine);

    /**
     * Verify if the supplied event can be executed in regards to the event
     * entity's current I-Time state
     * @param ctx Execution context of the event
     * @param e Pointer to the event to be executed
     * @return true if the event can be executed, false if it is not valid
     */
    bool VerifyITime(EventContext& ctx, std::shared_ptr<objects::Event> e);

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_EVENTMANAGER_H
