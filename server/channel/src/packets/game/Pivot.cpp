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

// channel Includes
#include "ChannelServer.h"

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
    float x = p.ReadFloat();
    float y = p.ReadFloat();
    float rot = p.ReadFloat();
    float startTime = p.ReadFloat();
    float stopTime = p.ReadFloat();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    auto entity = state->GetEntityState(entityID);
    if(!entity)
    {
        LOG_ERROR("Player attempted to pivot an entity that does not belong"
            " to the client\n");
        state->SetLogoutSave(true);
        client->Close();
        return true;
    }

    // Make sure the request is not in the future
    ServerTime now = ChannelServer::GetServerTime();
    if(now >= startTime)
    {
        entity->SetOriginX(x);
        entity->SetOriginY(y);
        entity->SetOriginRotation(rot);
        entity->SetOriginTicks(now);
        entity->SetDestinationX(x);
        entity->SetDestinationY(y);
        entity->SetDestinationRotation(rot);
        entity->SetDestinationTicks(now);

        ServerTime stopConverted = state->ToServerTime(stopTime);
        uint64_t immobileTime = entity->GetStatusTimes(STATUS_IMMOBILE);
        if(stopConverted > immobileTime)
        {
            entity->SetStatusTimes(STATUS_IMMOBILE, stopConverted);
        }
    }

    return true;
}
