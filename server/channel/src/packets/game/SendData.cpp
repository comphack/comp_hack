/**
 * @file server/channel/src/packets/SendData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update data on the server.
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
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

using namespace channel;

void SendZoneChange(const std::shared_ptr<libcomp::TcpConnection>& connection)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_ZONE_CHANGE);
    reply.WriteU32Little(0x00004E85);        //id
    reply.WriteU32Little(1);        //set
    reply.WriteFloat(0.f);          //x
    reply.WriteFloat(0.f);          //y
    reply.WriteFloat(0.f);          //rotation
    reply.WriteU32Little(0);        //unique id

    connection->SendPacket(reply);
}

bool Parsers::SendData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    /// @todo: A bunch of stuff

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_CONFIRMATION);
    reply.WriteU32Little(0x00010000); // 1.0.0

    connection->SendPacket(reply);

    SendZoneChange(connection);

    return true;
}
