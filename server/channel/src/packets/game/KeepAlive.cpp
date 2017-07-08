/**
 * @file server/channel/src/packets/game/KeepAlive.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to keep the connection active.
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

bool Parsers::KeepAlive::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    // Keep alive requests should occur once every 10 seconds
    // The client can miss only a few requests before the server kills the connection
    client->RefreshTimeout(server->GetServerTime());

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_KEEP_ALIVE);
    reply.WriteU32Little(p.ReadU32Little());

    client->SendPacket(reply);

    return true;
}