/**
 * @file server/channel/src/ActionManager.cpp
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

#include "ActionManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

// object Includes
#include <Action.h>
#include <ActionSetNPCState.h>
#include <ActionStartEvent.h>
#include <ActionUpdateFlag.h>
#include <ActionUpdateLNC.h>
#include <ActionUpdateQuest.h>
#include <ActionZoneChange.h>
#include <ServerObject.h>
#include <ServerZone.h>

using namespace channel;

ActionManager::ActionManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
    mActionHandlers[objects::Action::ActionType_t::ZONE_CHANGE] =
        &ActionManager::ZoneChange;
    mActionHandlers[objects::Action::ActionType_t::START_EVENT] =
        &ActionManager::StartEvent;
    mActionHandlers[objects::Action::ActionType_t::SET_NPC_STATE] =
        &ActionManager::SetNPCState;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_FLAG] =
        &ActionManager::UpdateFlag;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_LNC] =
        &ActionManager::UpdateLNC;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_QUEST] =
        &ActionManager::UpdateQuest;
}

ActionManager::~ActionManager()
{
}

void ActionManager::PerformActions(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::list<std::shared_ptr<objects::Action>>& actions,
    int32_t sourceEntityID)
{
    (void)client;

    for(auto action : actions)
    {
        auto it = mActionHandlers.find(action->GetActionType());

        if(mActionHandlers.end() == it)
        {
            LOG_ERROR(libcomp::String("Failed to parse action of type %1\n"
                ).Arg(to_underlying(action->GetActionType())));
        }
        else if(!it->second(*this, client, action, sourceEntityID))
        {
            break;
        }
    }
}

bool ActionManager::StartEvent(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID)
{
    auto act = std::dynamic_pointer_cast<objects::ActionStartEvent>(action);

    auto server = mServer.lock();
    auto eventManager = server->GetEventManager();

    eventManager->HandleEvent(client, act->GetEventID(), sourceEntityID);
    
    return true;
}

bool ActionManager::ZoneChange(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID)
{
    (void)sourceEntityID;

    auto act = std::dynamic_pointer_cast<objects::ActionZoneChange>(action);

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    // Where is the character going?
    uint32_t zoneID = act->GetZoneID();
    float x = act->GetDestinationX();
    float y = act->GetDestinationY();
    float rotation = act->GetDestinationRotation();

    // Enter the new zone and always leave the old zone even if its the same.
    if(!zoneManager->EnterZone(client, zoneID, x, y, rotation, true))
    {
        LOG_ERROR(libcomp::String("Failed to add client to zone"
            " %1. Closing the connection.\n").Arg(zoneID));

        client->Close();

        return false;
    }
    
    return true;
}

bool ActionManager::SetNPCState(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID)
{
    auto act = std::dynamic_pointer_cast<objects::ActionSetNPCState>(action);

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto zone = zoneManager->GetZoneInstance(client);
    auto oNPC = zone ? zone->GetServerObject(sourceEntityID) : nullptr;
    if(oNPC)
    {
        oNPC->GetEntity()->SetState(act->GetState());

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_NPC_STATE_CHANGE);
        p.WriteS32Little(sourceEntityID);
        p.WriteU8(act->GetState());

        zoneManager->BroadcastPacket(client, p);

        return true;
    }

    return true;
}

bool ActionManager::UpdateFlag(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID)
{
    (void)sourceEntityID;

    auto act = std::dynamic_pointer_cast<objects::ActionUpdateFlag>(action);
    auto characterManager = mServer.lock()->GetCharacterManager();

    switch(act->GetFlagType())
    {
    case objects::ActionUpdateFlag::FlagType_t::MAP:
        characterManager->AddMap(client, act->GetID());
        break;
    case objects::ActionUpdateFlag::FlagType_t::PLUGIN:
        characterManager->AddPlugin(client, act->GetID());
        break;
    case objects::ActionUpdateFlag::FlagType_t::VALUABLE:
        characterManager->AddRemoveValuable(client, act->GetID(),
            act->GetRemove());
        break;
    default:
        return false;
    }

    return true;
}

bool ActionManager::UpdateLNC(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID)
{
    (void)sourceEntityID;

    auto act = std::dynamic_pointer_cast<objects::ActionUpdateLNC>(action);
    auto character = client->GetClientState()->GetCharacterState()
        ->GetEntity();
    auto characterManager = mServer.lock()->GetCharacterManager();

    int16_t lnc = character->GetLNC();
    if(act->GetIsSet())
    {
        lnc = act->GetValue();
    }
    else
    {
        lnc = (int16_t)(lnc + act->GetValue());
    }

    characterManager->UpdateLNC(client, lnc);

    return true;
}

bool ActionManager::UpdateQuest(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::Action>& action, int32_t sourceEntityID)
{
    (void)sourceEntityID;

    auto act = std::dynamic_pointer_cast<objects::ActionUpdateQuest>(action);
    auto server = mServer.lock();
    auto eventManager = server->GetEventManager();

    eventManager->UpdateQuest(client, act->GetQuestID(), act->GetPhase());

    return true;
}
