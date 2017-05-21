/**
 * @file server/channel/src/packets/Sync.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to sync with the server time.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::WorldTime::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    int8_t phase, hour, min;
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    server->GetWorldClockTime(phase, hour, min);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_WORLD_TIME);
    reply.WriteS8(phase);
    reply.WriteS8(hour);
    reply.WriteS8(min);

    connection->SendPacket(reply);

    return true;
}
