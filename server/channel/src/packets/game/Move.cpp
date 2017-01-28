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
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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
#include "CharacterState.h"
#include "ClientState.h"

using namespace channel;

bool Parsers::Move::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();

    int32_t entityID = p.ReadS32Little();

    std::shared_ptr<objects::EntityStateObject> eState;
    if(cState->GetEntityID() == entityID)
    {
        eState = std::dynamic_pointer_cast<objects::EntityStateObject>(cState);
    }
    else if(nullptr != dState && dState->GetEntityID() == entityID)
    {
        eState = std::dynamic_pointer_cast<objects::EntityStateObject>(dState);
    }
    else
    {
        LOG_ERROR(libcomp::String("Invalid entity ID received from a move request: %1\n")
            .Arg(entityID));
        return false;
    }

    float destX = p.ReadFloat();
    float destY = p.ReadFloat();
    float originX = p.ReadFloat();
    float originY = p.ReadFloat();
    float ratePerSec = p.ReadFloat();
    ClientTime start = (ClientTime)p.ReadFloat();
    ClientTime stop = (ClientTime)p.ReadFloat();

    ServerTime startTime = state->ToServerTime(start);
    ServerTime stopTime = state->ToServerTime(stop);

    (void)ratePerSec;

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
    eState->SetOriginY(originY);
    eState->SetOriginTicks(startTime);
    eState->SetDestinationX(destX);
    eState->SetDestinationY(destY);
    eState->SetDestinationTicks(stopTime);

    eState->SetOriginRotation(eState->GetDestinationRotation());
    eState->SetDestinationRotation((float)atan2(destY, destX));

    /// @todo: Fire zone triggers

    /// @todo: Send to the whole zone, including the player this sent from if they
    /// need to be corrected
    /*libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MOVE_RESPONSE);
    reply.WriteS32Little(entityID);
    reply.WriteFloat(destX);
    reply.WriteFloat(destY);
    reply.WriteFloat(originX);
    reply.WriteFloat(originY);
    reply.WriteFloat(ratePerSec);
    reply.WriteFloat(start);
    reply.WriteFloat(stop);

    connection->SendPacket(reply);*/

    return true;
}
