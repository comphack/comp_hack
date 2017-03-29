/**
 * @file server/channel/src/packets/game/ObjectInteraction.cpp
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Request from the client to handle an object interaction (NPC).
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

#include "Packets.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>
#include <PacketCodes.h>

// object includes
#include <ServerZone.h>
#include <ServerZoneSpot.h>

// Standard C++14 Includes
#include <gsl/gsl>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

class ActionList
{
public:
    std::list<std::shared_ptr<objects::Action>> actions;
};

bool Parsers::ObjectInteraction::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    // Sanity check the packet size.
    if(sizeof(uint32_t) != p.Left())
    {
        return false;
    }

    // Read the values from the packet.
    int32_t entityID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto zone = server->GetZoneManager()->GetZoneInstance(client);
    auto zoneDef = zone->GetDefinition();

    std::list<std::shared_ptr<objects::Action>> actions;

    // Lookup the spot and see if it has actions.
    std::shared_ptr<objects::EntityStateObject> entity = zone->GetNPC(entityID);

    // Look for an object if there is no NPC by that name.
    if(!entity)
    {
        entity = zone->GetServerObject(entityID);
    }

    /// @todo Implement better checks? In range of entity?

    LOG_DEBUG(libcomp::String("Interacted with entity %1\n").Arg(entityID));

    if(entity)
    {
        // Get the action list.
        auto pActionList = new ActionList;
        pActionList->actions = entity->GetActions();

        LOG_DEBUG(libcomp::String("Got entity with %1 actions.\n").Arg(
            pActionList->actions.size()));

        // There must be at least 1 action or we are wasting our time.
        if(pActionList->actions.empty())
        {
            delete pActionList;

            return true;
        }

        // Perform the action(s) in the list.
        server->QueueWork([](
            const std::shared_ptr<ChannelServer>& serverWork,
            const std::shared_ptr<ChannelClientConnection> clientWork,
            gsl::owner<ActionList*> pActionListWork)
        {
            serverWork->GetActionManager()->PerformActions(clientWork,
                pActionListWork->actions);

            delete pActionListWork;
        }, server, client, pActionList);
    }
    else
    {
        LOG_WARNING(libcomp::String("Unknown entity %1\n").Arg(
            entityID));
    }

    return true;
}
