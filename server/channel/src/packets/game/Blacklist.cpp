/**
 * @file server/channel/src/packets/game/Blacklist.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the character's blacklist.
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

// object Includes
#include <AccountWorldData.h>

// channel Includes
#include "ChannelClientConnection.h"

using namespace channel;

bool Parsers::Blacklist::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 4)
    {
        return false;
    }

    int32_t unknown = p.ReadS32Little();    // Always 0
    (void)unknown;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto worldData = state->GetAccountWorldData().Get();
    auto blacklist = worldData->GetBlacklist();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BLACKLIST);
    reply.WriteS32Little(0);    // Unknown, always 0
    reply.WriteS32Little(0);    // Unknown, always 0

    reply.WriteS32Little((int32_t)blacklist.size());
    for(auto& entry : blacklist)
    {
        reply.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
            entry, true);
    }

    client->SendPacket(reply);

    return true;
}
