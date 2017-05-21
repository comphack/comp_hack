/**
 * @file server/channel/src/packets/game/ClanInfo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the current player's clan info.
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
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelClientConnection.h"

using namespace channel;

bool Parsers::ClanInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    /// @todo: implement non-default values

    libcomp::String empty;
    
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_INFO);
    reply.WriteS32Little(0);    // Clan ID
    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        empty, true);           // Clan name
    reply.WriteS32Little(0);    // Base zone ID

    int8_t activeMemberCount = 0;
    reply.WriteS8(activeMemberCount);
    for(int8_t i = 0; i < activeMemberCount; i++)
    {
        reply.WriteS32Little(0);    // Active member entity ID
    }

    reply.WriteS8(0);    // Clan level
    reply.WriteU8(0);    // Emblem base
    reply.WriteU8(0);    // Emblem symbol

    // Base color values
    reply.WriteU8(0);    // R
    reply.WriteU8(0);    // G
    reply.WriteU8(0);    // B

    // Symbol color values
    reply.WriteU8(0);    // R
    reply.WriteU8(0);    // G
    reply.WriteU8(0);    // B

    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        empty, true);   // Unknown

    connection->SendPacket(reply);

    return true;
}
