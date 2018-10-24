/**
 * @file server/channel/src/packets/game/Rotate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to rotate an entity or game object.
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
#include <Decrypt.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"
#include "ClientState.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::Rotate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 16)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int32_t entityID = p.ReadS32Little();

    auto eState = state->GetEntityState(entityID, false);
    if(nullptr == eState)
    {
        LOG_ERROR(libcomp::String("Invalid entity ID received from a rotate"
            " request: %1\n").Arg(state->GetAccountUID().ToString()));
        client->Close();
        return true;
    }
    else if(!eState->Ready(true))
    {
        // Nothing to do, the entity is not currently active
        return true;
    }

    float rotation = p.ReadFloat();
    ClientTime start = (ClientTime)p.ReadFloat();
    ClientTime stop = (ClientTime)p.ReadFloat();

    ServerTime startTime = state->ToServerTime(start);
    ServerTime stopTime = state->ToServerTime(stop);

    // Rotating does not update the X and Y position
    eState->RefreshCurrentPosition(server->GetServerTime());
    float x = eState->GetCurrentX();
    float y = eState->GetCurrentY();
    eState->SetOriginX(x);
    eState->SetOriginY(y);
    eState->SetDestinationX(x);
    eState->SetDestinationY(y);

    eState->SetOriginTicks(startTime);
    eState->SetDestinationTicks(stopTime);

    eState->SetOriginRotation(eState->GetCurrentRotation());
    eState->SetDestinationRotation(rotation);

    // If the entity is still visible to others, relay info
    if(eState->IsClientVisible())
    {
        auto zConnections = server->GetZoneManager()->GetZoneConnections(
            client, false);

        if(zConnections.size() > 0)
        {
            libcomp::Packet reply;
            reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ROTATE);
            reply.WriteS32Little(entityID);
            reply.WriteFloat(rotation);

            RelativeTimeMap timeMap;
            timeMap[reply.Size()] = startTime;
            timeMap[reply.Size() + 4] = stopTime;

            ChannelClientConnection::SendRelativeTimePacket(zConnections,
                reply, timeMap);
        }
    }

    return true;
}
