/**
 * @file server/channel/src/packets/game/Move.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to move an entity or game object.
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

// Standard C++11 Includes
#include <math.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"
#include "ClientState.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::Move::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int32_t entityID = p.ReadS32Little();

    auto eState = state->GetEntityState(entityID, false);
    if(nullptr == eState)
    {
        LOG_ERROR(libcomp::String("Invalid entity ID received from a move request: %1\n")
            .Arg(entityID));
        return false;
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

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto zoneManager = server->GetZoneManager();

    float destX = p.ReadFloat();
    float destY = p.ReadFloat();
    float originX = p.ReadFloat();
    float originY = p.ReadFloat();
    float ratePerSec = p.ReadFloat();
    ClientTime start = (ClientTime)p.ReadFloat();
    ClientTime stop = (ClientTime)p.ReadFloat();

    ServerTime startTime = state->ToServerTime(start);
    ServerTime stopTime = state->ToServerTime(stop);

    /// @todo: Determine if the player's movement was valid (collisions, triggers etc)
    bool positionCorrected = false;

    eState->ExpireStatusTimes(ChannelServer::GetServerTime());
    if(!eState->CanMove())
    {
        zoneManager->FixCurrentPosition(eState, stopTime, startTime);
        return true;
    }

    /*float deltaX = destX - originX;
    float deltaY = destY - originY;
    float dist2 = (deltaX * deltaX) + (deltaY * deltaY);

    // Delta time in seconds
    double deltaTime = static_cast<double>((double)(stopTime - startTime) / 1000000.0);
    double maxDist = static_cast<double>(deltaTime * ratePerSec);
    double dist = (double)sqrtl(dist2);

    LOG_DEBUG(libcomp::String("Rate: %1 | Dist: %2 | Max Dist: %3 | Time: %4\n").Arg(
        ratePerSec).Arg(dist).Arg(maxDist).Arg(std::to_string(deltaTime)));*/

    eState->SetOriginX(originX);
    eState->SetCurrentX(originX);
    eState->SetOriginY(originY);
    eState->SetCurrentY(originY);
    eState->SetOriginTicks(startTime);
    eState->SetDestinationX(destX);
    eState->SetDestinationY(destY);
    eState->SetDestinationTicks(stopTime);

    float originRot = eState->GetCurrentRotation();
    float destRot = (float)atan2(originY - destY, originX - destX);
    eState->SetOriginRotation(originRot);
    eState->SetDestinationRotation(destRot);

    // Time to rotate while moving is nearly instantaneous
    // and kind of irrelavent so mark it right away
    eState->SetCurrentRotation(destRot);

    /// @todo: Fire zone triggers

    // If the entity is still visible to others, relay info
    std::list<std::shared_ptr<ChannelClientConnection>> zConnections;
    if(eState->IsClientVisible())
    {
        zConnections = zoneManager->GetZoneConnections(client, false);
    }

    // If the move was invalid, send correction to self
    if(positionCorrected)
    {
        zConnections.push_back(client);
    }

    if(zConnections.size() > 0)
    {
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MOVE);
        reply.WriteS32Little(entityID);
        reply.WriteFloat(destX);
        reply.WriteFloat(destY);
        reply.WriteFloat(originX);
        reply.WriteFloat(originY);
        reply.WriteFloat(ratePerSec);

        RelativeTimeMap timeMap;
        timeMap[reply.Size()] = startTime;
        timeMap[reply.Size() + 4] = stopTime;

        ChannelClientConnection::SendRelativeTimePacket(zConnections, reply, timeMap);
    }

    // If a demon is moving while the character is hidden, warp the
    // character to the destination spot
    if(eState == state->GetDemonState() &&
        state->GetCharacterState()->GetIsHidden())
    {
        zoneManager->Warp(client, state->GetCharacterState(),
            destX, destY, 0.f);
    }

    /// @todo: lower movement durability

    return true;
}
