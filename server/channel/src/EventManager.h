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

#ifndef SERVER_CHANNEL_SRC_EVENTMANAGER_H
#define SERVER_CHANNEL_SRC_EVENTMANAGER_H

// libcomp Includes
#include <EnumMap.h>

// object Includes
#include <Event.h>

// channel Includes
#include "ChannelClientConnection.h"

namespace objects
{
class EventCondition;
class EventConditionData;
class EventInstance;
}

namespace channel
{

class ChannelServer;

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
     * @param client Pointer to the client the event affects
     * @param eventID ID of the event to handle
     * @param sourceEntityID Optional source of an event to focus on
     * @return true on success, false on failure
     */
    bool HandleEvent(const std::shared_ptr<ChannelClientConnection>& client,
        const libcomp::String& eventID, int32_t sourceEntityID);

    /**
     * React to a response based on the current event of a client
     * @param client Pointer to the client sending the response
     * @param responseID Value representing the player's response
     *  contextual to the current event type
     * @return true on success, false on failure
     */
    bool HandleResponse(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t responseID);

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
     * Update the client's quest kill counts
     * @param client Pointer to the client with active kill quests
     * @param kills Map of demon types to the number killed
     */
    void UpdateQuestKillCount(const std::shared_ptr<ChannelClientConnection>& client,
        const std::unordered_map<uint32_t, int32_t>& kills);

    /**
     * Evaluate each of the valid condition sets required to start a quest
     * @param client Pointer to the client to evaluate contextually to
     * @param questID ID of the quest being evaluated
     * @return true if the any of the condition sets all evaluated to true, false
     *  if no set's conditions all evaluated to true
     */
    bool EvaluateQuestConditions(const std::shared_ptr<ChannelClientConnection>& client,
        int16_t questID);

    /**
     * Evaluate an event condition
     * @param client Pointer to the client to evaluate contextually to
     * @param conditions List of event conditions to evaluate
     * @return true if the event condition evaluates to true, otherwise false
     */
    bool EvaluateEventCondition(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventCondition>& condition);

    /**
     * Evaluate a list of event conditions
     * @param client Pointer to the client to evaluate contextually to
     * @param condition Event condition to evaluate
     * @return true if the event conditions evaluate to true, otherwise false
     */
    bool EvaluateEventConditions(const std::shared_ptr<ChannelClientConnection>& client,
        const std::list<std::shared_ptr<objects::EventCondition>>& conditions);

    /**
     * Evaluate a standard event condition
     * @param client Pointer to the client to evaluate contextually to
     * @param condition Standard condition to evaluate
     * @return true if standard condition evaluates to true, otherwise false
     */
    bool EvaluateCondition(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventConditionData>& condition);

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

private:
    /**
     * Evaluate a set of flag states for a standard condition
     * @param flagStates Map of flag states to evaluate
     * @param condition Standard condition to evaluate
     * @return true if the flag states evaluate to true, otherwise false
     */
    bool EvaluateFlagStates(const std::unordered_map<int32_t, int32_t>& flagStates,
        const std::shared_ptr<objects::EventCondition>& condition);

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
     * Progress or pop to the next sequential event if one exists. If
     * no event is found, the event sequence is ended.
     * @param client Pointer to the client the event affects
     * @param current Current event instance to process relative to
     */
    void HandleNext(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& current);

    /**
     * Send a message to the client
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool Message(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Send an NPC based message to the client
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool NPCMessage(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Send an extended NPC based message to the client
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool ExNPCMessage(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Start a multitalk event clientside
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool Multitalk(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Prompt the player to choose from set options
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool Prompt(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Play an in-game cinematic clientside
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool PlayScene(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Open a menu to the player representing various facilities
     * and functions
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool OpenMenu(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Inform the player that items have been obtained
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool GetItems(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Perform one or many actions from the same types of options
     * configured for object interaction
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool PerformActions(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Inform the client that their character homepoint has been updated
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool Homepoint(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Render a stage effect on the client
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool StageEffect(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Signify a direction to the player
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool Direction(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Signify a special direction to the player
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool SpecialDirection(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Play a sound effect clientside
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool PlaySoundEffect(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Play a BGM track clientside
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool PlayBGM(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Stop a BGM track clientside
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool StopBGM(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * End the current event sequence
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool EndEvent(const std::shared_ptr<ChannelClientConnection>& client);

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_EVENTMANAGER_H
