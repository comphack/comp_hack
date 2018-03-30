/**
 * @file server/channel/src/packets/game/ChannelList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the list of channels connected to
 *	the server.
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
#include "ChannelClientConnection.h"
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ChannelList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto channels = server->GetAllRegisteredChannels();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHANNEL_LIST);
    reply.WriteS8((int8_t)channels.size());

    for(auto channel : channels)
    {
        reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
            channel->GetName(), true);

        /*
         * Server status is as follows:
         * 0-24 Comfortable
         * 25-39 Normal
         * 40-98 Conjested
         * 99 Full (White Text)
         * 100+ Full (Red Text)
         */
        reply.WriteU8(1);   // Show in list (always return true)
        reply.WriteS8(0);   // Percent full
        reply.WriteS8(0);   // 0 = visible, 2 = PvP
    }

    connection->SendPacket(reply);

    return true;
}
