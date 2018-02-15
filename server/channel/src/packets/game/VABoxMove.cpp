/**
 * @file server/channel/src/packets/game/VABoxMove.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to move a VA item within the closet.
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

using namespace channel;

bool Parsers::VABoxMove::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 14)
    {
        return false;
    }

    int32_t unused = p.ReadS32Little(); // Always 0
    int8_t slot1 = p.ReadS8();
    uint32_t itemType1 = p.ReadU32Little();
    int8_t slot2 = p.ReadS8();
    uint32_t itemType2 = p.ReadU32Little();
    (void)unused;

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    bool item1Null = itemType1 == static_cast<uint32_t>(-1);
    bool item2Null = itemType2 == static_cast<uint32_t>(-1);

    bool success = false;
    if(((item1Null && character->GetVACloset((size_t)slot1) == 0) ||
        (!item1Null && character->GetVACloset((size_t)slot1) == itemType1)) &&
        ((item2Null && character->GetVACloset((size_t)slot2) == 0) ||
        (!item2Null && character->GetVACloset((size_t)slot2) == itemType2)))
    {
        character->SetVACloset((size_t)slot1, item2Null ? 0 : itemType2);
        character->SetVACloset((size_t)slot2, item1Null ? 0 : itemType1);

        success = true;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_VA_BOX_MOVE);
    reply.WriteS32Little(success ? 0 : -1);
    reply.WriteS32Little(0);    // Unknown
    reply.WriteS8(slot1);
    reply.WriteU32Little(itemType2);
    reply.WriteS8(slot2);
    reply.WriteU32Little(itemType1);

    client->SendPacket(reply);

    if(success)
    {
        server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());
    }

    return true;
}
