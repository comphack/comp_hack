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
#include <CalculatedEntityState.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"
#include "ClientState.h"
#include "TokuseiManager.h"
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
        LOG_ERROR(libcomp::String("Invalid entity ID received from a move"
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

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
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

    const static bool moveCorrection = server->GetWorldSharedConfig()
        ->GetMoveCorrection();

    eState->ExpireStatusTimes(ChannelServer::GetServerTime());
    if(!eState->CanMove() || state->GetLockMovement())
    {
        destX = originX;
        destY = originY;
        positionCorrected = true;
    }
    else if(moveCorrection)
    {
        Point src(originX, originY);
        Point dest(destX, destY);

        uint8_t result = zoneManager->CorrectClientPosition(eState, src, dest,
            startTime, stopTime, true);
        if(result)
        {
            switch(result)
            {
            case 1:
                LOG_DEBUG(libcomp::String("Player movement rolled-back in"
                    " zone %1: %2\n").Arg(zone->GetDefinitionID())
                    .Arg(state->GetAccountUID().ToString()));
                break;
            case 2:
                LOG_DEBUG(libcomp::String("Player movement corrected in"
                    " zone %1: %2 ([%3, %4] => [%5, %6] to [%7, %8])\n")
                    .Arg(zone->GetDefinitionID())
                    .Arg(state->GetAccountUID().ToString())
                    .Arg(src.x).Arg(src.y).Arg(destX).Arg(destY)
                    .Arg(dest.x).Arg(dest.y));
                break;
            default:
                break;
            }

            destX = dest.x;
            destY = dest.y;
            positionCorrected = true;
        }

        // Origin may be changed for other players even if it was not
        // functionally changed for the source player
        originX = src.x;
        originY = src.y;
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
    float destRot = (float)atan2(destY - originY, destX - originX);
    eState->SetOriginRotation(originRot);
    eState->SetDestinationRotation(destRot);

    // Time to rotate while moving is nearly instantaneous
    // and kind of irrelavent so mark it right away
    eState->SetCurrentRotation(destRot);

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

    float demonX = destX;
    float demonY = destY;
    switch(eState->GetEntityType())
    {
    case EntityType_t::CHARACTER:
        // Update movement decay durability
        if(eState->GetCalculatedState()->ExistingTokuseiAspectsContains(
            (int8_t)TokuseiAspectType::EQUIP_MOVE_DECAY))
        {
            float dSquared = (float)(std::pow((originX - destX), 2)
                + std::pow((originY - destY), 2));
            server->GetTokuseiManager()->UpdateMovementDecay(client,
                std::sqrt(dSquared));
        }

        if(state->GetDemonState()->Ready())
        {
            // Only compare character/demon distance if its summoned
            demonX = state->GetDemonState()->GetDestinationX();
            demonY = state->GetDemonState()->GetDestinationY();
        }
        break;
    case EntityType_t::PARTNER_DEMON:
        // If a demon is moving while the character is hidden, warp the
        // character to the destination spot
        if(state->GetCharacterState()->GetIsHidden())
        {
            zoneManager->Warp(client, state->GetCharacterState(),
                destX, destY, 0.f);
        }
        break;
    default:
        break;
    }

    // Player character and demon being at least 6000 units from one another
    // (squared here), will cause the demon to warp to the character
    if(state->GetCharacterState()->GetDistance(demonX, demonY, true) >=
        36000000.f)
    {
        zoneManager->Warp(client, state->GetDemonState(),
            state->GetCharacterState()->GetDestinationX(),
            state->GetCharacterState()->GetDestinationY(), 0.f);
    }

    return true;
}
