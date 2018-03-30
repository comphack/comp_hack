/**
 * @file server/channel/src/packets/game/TitleList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the list of available titles.
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
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelClientConnection.h"

using namespace channel;

bool Parsers::TitleList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    /// @todo: implement non-default values
    
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TITLE_LIST);
    reply.WriteS32Little(0);   // Unknown
    reply.WriteS8(0);   // Unknown

    int16_t unknownCount = 128;
    reply.WriteS16Little(unknownCount);
    for(int16_t i = 0; i < unknownCount; i++)
    {
        // Unknown
        reply.WriteS8(0);
    }
    
    int32_t unknownCount2 = 0;
    reply.WriteS32Little(unknownCount2);
    for(int32_t i = 0; i < unknownCount2; i++)
    {
        // Unknown
        reply.WriteS16Little(0);
    }
    
    for(int32_t i = 0; i < 5; i++)
    {
        reply.WriteS32Little(i);
        reply.WriteS16Little(-1);   // Unknown
    }

    reply.WriteU8(1);   // Unknown bool

    connection->SendPacket(reply);

    return true;
}
