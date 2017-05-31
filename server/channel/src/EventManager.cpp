/**
 * @file server/channel/src/EventManager.cpp
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

#include "EventManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

// object Includes
#include <Account.h>
#include <EventChoice.h>
#include <EventHomepoint.h>
#include <EventInstance.h>
#include <EventMessage.h>
#include <EventNPCMessage.h>
#include <EventOpenMenu.h>
#include <EventPerformActions.h>
#include <EventPrompt.h>
#include <EventState.h>
#include <ServerZone.h>

using namespace channel;

EventManager::EventManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

EventManager::~EventManager()
{
}

bool EventManager::HandleEvent(
    const std::shared_ptr<ChannelClientConnection>& client,
    const libcomp::String& eventID, int32_t sourceEntityID)
{
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    auto event = serverDataManager->GetEventData(eventID);
    if(nullptr == event)
    {
        LOG_ERROR(libcomp::String("Invalid event ID encountered %1\n"
            ).Arg(eventID));
        return false;
    }
    else
    {
        auto state = client->GetClientState();
        auto eState = state->GetEventState();
        if(eState->GetCurrent() != nullptr)
        {
            eState->AppendPrevious(eState->GetCurrent());
        }

        auto instance = std::shared_ptr<objects::EventInstance>(
            new objects::EventInstance);
        instance->SetEvent(event);
        instance->SetSourceEntityID(sourceEntityID);

        eState->SetCurrent(instance);

        return HandleEvent(client, instance);
    }
}

bool EventManager::HandleResponse(const std::shared_ptr<ChannelClientConnection>& client,
    int32_t responseID)
{
    auto state = client->GetClientState();
    auto eState = state->GetEventState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto current = eState->GetCurrent();

    if(nullptr == current)
    {
        LOG_ERROR(libcomp::String("Option selected for unknown event: %1\n"
            ).Arg(character->GetAccount()->GetUsername()));
        return false;
    }

    libcomp::String nextEventID;
    auto event = current->GetEvent();
    auto eventType = event->GetEventType();
    switch(eventType)
    {
        case objects::Event::EventType_t::MESSAGE:
        case objects::Event::EventType_t::NPC_MESSAGE:
            if(responseID != 0)
            {
                LOG_ERROR("Non-zero response received for message response.\n");
            }
            else
            {
                auto e = std::dynamic_pointer_cast<objects::EventMessage>(event);

                // If there are still more messages, increment and continue the same event
                if(current->GetIndex() < (e->MessageIDsCount() - 1))
                {
                    current->SetIndex((uint8_t)(current->GetIndex() + 1));
                    HandleEvent(client, current);
                    return true;
                }
                
                nextEventID = e->GetNext();

                /// @todo: check infinite loops
            }
            break;
        case objects::Event::EventType_t::PROMPT:
            {
                auto e = std::dynamic_pointer_cast<objects::EventPrompt>(event);
                auto choice = e->GetChoices((size_t)responseID);
                if(choice == nullptr)
                {
                    LOG_ERROR(libcomp::String("Invalid choice %1 selected for event %2\n"
                        ).Arg(responseID).Arg(e->GetID()));
                }
                else
                {
                    // Set pop information now that an option has been selected
                    current->SetPop(choice->GetPop());
                    current->SetPopNext(choice->GetPopNext());

                    nextEventID = choice->GetEventID();
                }
            }
            break;
        case objects::Event::EventType_t::OPEN_MENU:
            {
                auto e = std::dynamic_pointer_cast<objects::EventOpenMenu>(event);
                // 0 seems to signify to close the menu
                if(responseID != 0)
                {
                    LOG_ERROR(libcomp::String("Non-close choice %1 received for event %2\n"
                        ).Arg(responseID).Arg(e->GetID()));
                }
            }
            break;
        default:
            LOG_ERROR(libcomp::String("Response received for invalid event of type %1\n"
                ).Arg(to_underlying(eventType)));
            break;
    }
    
    HandleNext(client, current, nextEventID);

    return true;
}

bool EventManager::HandleEvent(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    bool handled = false;
    auto server = mServer.lock();
    auto eventType = instance->GetEvent()->GetEventType();
    switch(eventType)
    {
        case objects::Event::EventType_t::MESSAGE:
            server->GetCharacterManager()->SetStatusIcon(client, 4);
            handled = Message(client, instance);
            break;
        case objects::Event::EventType_t::NPC_MESSAGE:
            server->GetCharacterManager()->SetStatusIcon(client, 4);
            handled = NPCMessage(client, instance);
            break;
        case objects::Event::EventType_t::PROMPT:
            server->GetCharacterManager()->SetStatusIcon(client, 4);
            handled = Prompt(client, instance);
            break;
        case objects::Event::EventType_t::PERFORM_ACTIONS:
            handled = PerformActions(client, instance);
            break;
        case objects::Event::EventType_t::OPEN_MENU:
            server->GetCharacterManager()->SetStatusIcon(client, 4);
            handled = OpenMenu(client, instance);
            break;
        case objects::Event::EventType_t::HOMEPOINT:
            handled = Homepoint(client, instance);
            break;
        default:
            LOG_ERROR(libcomp::String("Failed to handle event of type %1\n"
                ).Arg(to_underlying(eventType)));
            break;
    }

    if(!handled)
    {
        EndEvent(client);
    }

    return handled;
}

void EventManager::HandleNext(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& current,
    const libcomp::String& nextEventID)
{
    auto state = client->GetClientState();
    auto eState = state->GetEventState();

    if(nextEventID.IsEmpty())
    {
        auto previous = eState->PreviousCount() > 0 ? eState->GetPrevious().back() : nullptr;
        if(previous != nullptr && (current->GetPop() || previous->GetPopNext()))
        {
            eState->RemovePrevious(eState->PreviousCount() - 1);
            eState->SetCurrent(previous);
            HandleEvent(client, previous);
        }
        else
        {
            EndEvent(client);
        }
    }
    else
    {
        HandleEvent(client, nextEventID, current->GetSourceEntityID());
    }
}

bool EventManager::Message(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventMessage>(instance->GetEvent());
    auto idx = instance->GetIndex();

    instance->SetPop(e->GetPop());
    instance->SetPopNext(e->GetPopNext());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_MESSAGE);
    p.WriteS32Little(e->GetMessageIDs((size_t)idx));

    client->SendPacket(p);

    return true;
}

bool EventManager::NPCMessage(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventNPCMessage>(instance->GetEvent());
    auto idx = instance->GetIndex();
    auto unknown = e->GetUnknown((size_t)idx);

    instance->SetPop(e->GetPop());
    instance->SetPopNext(e->GetPopNext());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_NPC_MESSAGE);
    p.WriteS32Little(instance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageIDs((size_t)idx));
    p.WriteS32Little(unknown != 0 ? unknown : e->GetUnknownDefault());

    client->SendPacket(p);

    return true;
}

bool EventManager::Prompt(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventPrompt>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PROMPT);
    p.WriteS32Little(instance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageID());

    auto choiceCount = e->ChoicesCount();
    p.WriteS32Little((int32_t)choiceCount);
    for(size_t i = 0; i < choiceCount; i++)
    {
        auto choice = e->GetChoices(i);

        p.WriteS32Little((int32_t)i);
        p.WriteS32Little(choice->GetMessageID());
    }

    client->SendPacket(p);

    return true;
}

bool EventManager::OpenMenu(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventOpenMenu>(instance->GetEvent());
    auto state = client->GetClientState();

    instance->SetPop(e->GetPop());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_OPEN_MENU);
    p.WriteS32Little(instance->GetSourceEntityID());
    p.WriteS32Little(e->GetMenuType());
    p.WriteS32Little(0);    // Unknown
    p.WriteString16Little(state->GetClientStringEncoding(),
        libcomp::String(), true);

    client->SendPacket(p);

    return true;
}

bool EventManager::PerformActions(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventPerformActions>(instance->GetEvent());

    auto server = mServer.lock();
    auto actionManager = server->GetActionManager();
    auto actions = e->GetActions();

    actionManager->PerformActions(client, actions, instance->GetSourceEntityID());

    HandleNext(client, instance, e->GetNext());

    return true;
}

bool EventManager::Homepoint(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventHomepoint>(instance->GetEvent());

    auto server = mServer.lock();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto zone = server->GetZoneManager()->GetZoneInstance(client);

    instance->SetPop(e->GetPop());

    /// @todo: check for invalid zone types or positions

    auto zoneID = zone->GetDefinition()->GetID();
    auto xCoord = cState->GetDestinationX();
    auto yCoord = cState->GetDestinationY();

    character->SetHomepointZone(zoneID);
    character->SetHomepointX(xCoord);
    character->SetHomepointY(yCoord);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_HOMEPOINT_UPDATE);
    p.WriteS32Little((int32_t)zoneID);
    p.WriteFloat(xCoord);
    p.WriteFloat(yCoord);

    client->SendPacket(p);

    mServer.lock()->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

    HandleNext(client, instance, e->GetNext());

    return true;
}

bool EventManager::EndEvent(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto eState = state->GetEventState();

    eState->SetCurrent(nullptr);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_END);

    client->SendPacket(p);

    auto server = mServer.lock();
    server->GetCharacterManager()->SetStatusIcon(client);

    return true;
}
