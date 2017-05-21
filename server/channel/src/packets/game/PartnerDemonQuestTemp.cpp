/**
 * @file server/channel/src/packets/game/PartnerDemonQuestTemp.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Unknown
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

bool Parsers::PartnerDemonQuestTemp::Parse(libcomp::ManagerPacket *pPacketManager,
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
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_DEMON_QUEST_TEMP);

    bool unknown = false;
    reply.WriteU8(unknown ? 1 : 0);

    if(unknown)
    {
        reply.WriteS64Little(0);    //Unknown
        reply.WriteS8(0);    //Unknown

        int8_t unknown2 = 0;
        reply.WriteS8(unknown2);    //Unknown
        if(unknown2 != 0)
        {
            reply.WriteU32Little(0);    //Unknown
            reply.WriteS32Little(0);    //Unknown
        }
        
        int8_t unknown3 = 0;
        reply.WriteS8(unknown3);    //Unknown
        if(unknown3 != 0)
        {
            reply.WriteU32Little(0);    //Unknown
            reply.WriteU16Little(0);    //Unknown
        }

        reply.WriteS32Little(0);    //Unknown
        
        int8_t unknown4 = 0;
        reply.WriteS8(unknown4);    //Unknown
        if(unknown4 != 0)
        {
            reply.WriteS8(0);    //Unknown
            reply.WriteU32Little(0);    //Unknown
            reply.WriteU16Little(0);    //Unknown
        }

        reply.WriteS16Little(0);    //Unknown
        reply.WriteU32Little(0);    //Unknown
        reply.WriteU32Little(0);    //Unknown
    }

    connection->SendPacket(reply);

    return true;
}
