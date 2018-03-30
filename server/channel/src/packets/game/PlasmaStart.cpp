/**
 * @file server/channel/src/packets/game/PlasmaStart.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to start the plasma picking minigame.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "PlasmaState.h"

using namespace channel;

bool Parsers::PlasmaStart::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 5)
    {
        return false;
    }

    int32_t plasmaID = p.ReadS32Little();
    int8_t pointID = p.ReadS8();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto zone = cState->GetZone();
    auto pState = std::dynamic_pointer_cast<PlasmaState>(zone->GetEntity(plasmaID));

    std::shared_ptr<PlasmaPoint> point;

    bool success = false;
    if(pState && pointID)
    {
        point = pState->PickPoint((uint32_t)pointID, state->GetWorldCID());
        success = point != nullptr;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_START);
    reply.WriteS32Little(plasmaID);
    reply.WriteS8(pointID);
    reply.WriteS32Little(success ? 0 : -1);

    client->QueuePacket(reply);

    if(success)
    {
        characterManager->SetStatusIcon(client, 4);
    }

    client->FlushOutgoing();

    return true;
}
