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
#include <ActionZoneChange.h>
#include <ServerZone.h>

using namespace channel;

ActionManager::ActionManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
    mActionHandlers[objects::Action::ActionType_t::ZONE_CHANGE] =
        &ActionManager::ZoneChange;
}

ActionManager::~ActionManager()
{
}

void ActionManager::PerformActions(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::list<std::shared_ptr<objects::Action>>& actions)
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
        else if(!it->second(*this, client, action))
        {
            break;
        }
    }
}

bool ActionManager::ZoneChange(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::Action>& act)
{
    auto action = std::dynamic_pointer_cast<objects::ActionZoneChange>(act);

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    auto cState = client->GetClientState()->GetCharacterState();

    // Where is the character going?
    uint32_t zoneID = action->GetZoneID();
    float x = action->GetDestinationX();
    float y = action->GetDestinationY();
    float rotation = action->GetDestinationRotation();

    // The character must first leave their existing zone.
    zoneManager->LeaveZone(client);

    // Enter the new zone.
    if(!zoneManager->EnterZone(client, zoneID, x, y, rotation))
    {
        LOG_ERROR(libcomp::String("Failed to add client to zone"
            " %1. Closing the connection.\n").Arg(zoneID));

        client->Close();

        return false;
    }
    
    return true;
}
