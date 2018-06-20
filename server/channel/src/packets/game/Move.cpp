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

// object Includes
#include <WorldSharedConfig.h>

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

    bool positionCorrected = false;

    const static bool checkCollision = server->GetWorldSharedConfig()
        ->GetMoveCorrection();

    eState->ExpireStatusTimes(ChannelServer::GetServerTime());
    if(!eState->CanMove())
    {
        destX = originX;
        destY = originY;
        positionCorrected = true;
    }
    else if(checkCollision)
    {
        auto geometry = zone->GetGeometry();
        if(geometry)
        {
            Line path(Point(originX, originY), Point(destX, destY));

            Point collidePoint;
            Line outSurface;
            std::shared_ptr<ZoneShape> outShape;
            if(zone->Collides(path, collidePoint, outSurface, outShape))
            {
                // Back off by 10
                collidePoint = zoneManager->GetLinearPoint(collidePoint.x,
                    collidePoint.y, originX, originY, 10.f, false);

                destX = collidePoint.x;
                destY = collidePoint.y;
                positionCorrected = true;

                // Monitor for ne'er-do-wells
                LOG_DEBUG(libcomp::String("Player movement corrected: %1"
                    " (%2, %2)\n").Arg(state->GetAccountUID().ToString())
                    .Arg(destX).Arg(destY));
            }
        }
    }

    eState->SetOriginX(originX);
    eState->SetCurrentX(originX);
    eState->SetOriginY(originY);
    eState->SetCurrentY(originY);
    eState->SetOriginTicks(startTime);
    eState->SetDestinationX(destX);
    eState->SetDestinationY(destY);
    eState->SetDestinationTicks(stopTime);

    // Calculate rotation from origin and destination
    float originRot = eState->GetCurrentRotation();
    float destRot = (float)atan2(originY - destY, originX - destX);
    eState->SetOriginRotation(originRot);
    eState->SetDestinationRotation(destRot);

    // Time to rotate while moving is nearly instantaneous
    // and kind of irrelavent so mark it right away
    eState->SetCurrentRotation(destRot);

    /// @todo: Fire zone triggers

    if(positionCorrected)
    {
        // Sending the move response back to the player can still be
        // forced through, warp back to the corrected point
        zoneManager->Warp(client, eState, destX, destY, destRot);
    }

    // If the entity is still visible to others, relay info
    if(eState->IsClientVisible())
    {
        auto zConnections = zoneManager->GetZoneConnections(client, false);
        if(zConnections.size() > 0)
        {
            libcomp::Packet notify;
            notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MOVE);
            notify.WriteS32Little(entityID);
            notify.WriteFloat(destX);
            notify.WriteFloat(destY);
            notify.WriteFloat(originX);
            notify.WriteFloat(originY);
            notify.WriteFloat(ratePerSec);

            RelativeTimeMap timeMap;
            timeMap[notify.Size()] = startTime;
            timeMap[notify.Size() + 4] = stopTime;

            ChannelClientConnection::SendRelativeTimePacket(zConnections,
                notify, timeMap);
        }
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
