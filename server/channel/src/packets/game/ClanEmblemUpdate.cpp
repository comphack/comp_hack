/**
 * @file server/channel/src/packets/game/ClanEmblemUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update their clan's emblem.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ClanEmblemUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 12)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int32_t clanID = p.ReadS32Little();
    uint8_t base = p.ReadU8();
    uint8_t symbol = p.ReadU8();
    uint8_t r1 = p.ReadU8();
    uint8_t g1 = p.ReadU8();
    uint8_t b1 = p.ReadU8();
    uint8_t r2 = p.ReadU8();
    uint8_t g2 = p.ReadU8();
    uint8_t b2 = p.ReadU8();

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_CLAN_EMBLEM_UPDATE);
    request.WriteS32Little(state->GetWorldCID());
    request.WriteS32Little(clanID);
    request.WriteU8(base);
    request.WriteU8(symbol);
    request.WriteU8(r1);
    request.WriteU8(g1);
    request.WriteU8(b1);
    request.WriteU8(r2);
    request.WriteU8(g2);
    request.WriteU8(b2);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

    return true;
}
