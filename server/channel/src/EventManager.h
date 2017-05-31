/**
 * @file server/channel/src/EventManager.h
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manages the execution and processing of events.
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
class EventInstance;
}

namespace channel
{

class ChannelServer;

/**
 * Manager class in charge of processing event sequences. Events include
 * things like NPC dialogue, player choice prompts, cinematics and context
 * sensitive menus.  Events can be strung together and can progress as well
 * as pop back to previous events much like a dialogue tree.
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

private:
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
     * @param nextEventID ID of the next event to process. If this is
     *  empty, the event will attempt to pop to the previous event.
     */
    void HandleNext(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& current,
        const libcomp::String& nextEventID);

    /**
     * Send a message to the client
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool Message(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::EventInstance>& instance);

    /**
     * Send a NPC based message to the client
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool NPCMessage(const std::shared_ptr<ChannelClientConnection>& client,
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
     * Open a menu to the player representing various facilities
     * and functions
     * @param client Pointer to the client the event affects
     * @param instance Current event instance to process relative to
     * @return true on success, false on failure
     */
    bool OpenMenu(const std::shared_ptr<ChannelClientConnection>& client,
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
