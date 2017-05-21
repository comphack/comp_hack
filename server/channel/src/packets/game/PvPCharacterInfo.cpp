/**
 * @file server/channel/src/packets/game/PvPCharacterInfo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for PvP character info.
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

bool Parsers::PvPCharacterInfo::Parse(libcomp::ManagerPacket *pPacketManager,
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
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PVP_CHARACTER_INFO);

    reply.WriteS32Little(0);    //Unknown
    reply.WriteS32Little(0);    //Unknown
    reply.WriteS8(0);    //Unknown
    reply.WriteS8(0);    //Unknown
    reply.WriteS32Little(0);    //Unknown

    for(int i = 0; i < 2; i++)
    {
        reply.WriteS32Little(0);    //Unknown
        reply.WriteS32Little(0);    //Unknown
        reply.WriteS32Little(0);    //Unknown
    }

    reply.WriteS32Little(0);    //Unknown
    reply.WriteS32Little(0);    //Unknown
    reply.WriteS32Little(0);    //Unknown

    int32_t unknownCount = 0;
    reply.WriteS32Little(unknownCount);
    for(int32_t i = 0; i < unknownCount; i++)
    {
        reply.WriteS8(0);    //Unknown
        reply.WriteS32Little(0);    //Unknown
    }

    connection->SendPacket(reply);

    return true;
}
