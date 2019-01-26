/**
 * @file server/channel/src/packets/game/Pivot.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to pivot a player entity in place
 *  for a specified amount of time. Used primarily by skill execution
 *  to sync animation timing.
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

#include "Packets.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>

// object Includes
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::Pivot::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 24)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();

    auto eState = state->GetEntityState(entityID);
    if(!eState)
    {
        LOG_ERROR(libcomp::String("Invalid entity ID received from a pivot"
            " request: %1\n").Arg(state->GetAccountUID().ToString()));
        client->Close();
        return true;
    }
    else if(!eState->Ready(true))
    {
        // Nothing to do, the entity is not currently active
        return true;
    }

    auto zone = eState->GetZone();
    if(!zone)
    {
        // Not actually in a zone
        return true;
    }

    float x = p.ReadFloat();
    float y = p.ReadFloat();
    float rot = p.ReadFloat();
    float startTime = p.ReadFloat();
    float stopTime = p.ReadFloat();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
    auto zoneManager = server->GetZoneManager();

    const static bool moveCorrection = server->GetWorldSharedConfig()
        ->GetMoveCorrection();
    if(moveCorrection)
    {
        Point dest(x, y);
        if(zoneManager->CorrectClientPosition(eState, dest))
        {
            LOG_DEBUG(libcomp::String("Player pivot corrected in"
                " zone %1: %2\n").Arg(zone->GetDefinitionID())
                .Arg(state->GetAccountUID().ToString()));

            x = dest.x;
            y = dest.y;
        }
    }

    // Make sure the request is not in the future
    ServerTime now = ChannelServer::GetServerTime();
    if(now >= state->ToServerTime(startTime))
    {
        eState->SetOriginX(x);
        eState->SetOriginY(y);
        eState->SetOriginRotation(rot);
        eState->SetOriginTicks(now);
        eState->SetDestinationX(x);
        eState->SetDestinationY(y);
        eState->SetDestinationRotation(rot);
        eState->SetDestinationTicks(now);

        ServerTime stopConverted = state->ToServerTime(stopTime);
        uint64_t immobileTime = eState->GetStatusTimes(STATUS_IMMOBILE);
        if(stopConverted > immobileTime)
        {
            eState->SetStatusTimes(STATUS_IMMOBILE, stopConverted);
        }
    }

    return true;
}
