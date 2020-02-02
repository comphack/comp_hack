/**
 * @file server/channel/src/packets/game/VABox.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for all items contained in the VA closet.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"

using namespace channel;

bool Parsers::VABox::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 4)
    {
        return false;
    }

    int32_t unused = p.ReadS32Little();
    (void)unused;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    std::set<size_t> slots;
    for(size_t i = 0; i < 50; i++)
    {
        if(character->GetVACloset(i) != 0)
        {
            slots.insert(i);
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_VA_BOX);
    reply.WriteS32Little(0);
    reply.WriteS32Little(0);
    reply.WriteS32Little((int32_t)slots.size());
    for(size_t slot : slots)
    {
        reply.WriteS8((int8_t)slot);
        reply.WriteU32Little(character->GetVACloset(slot));
    }

    client->SendPacket(reply);

    return true;
}
