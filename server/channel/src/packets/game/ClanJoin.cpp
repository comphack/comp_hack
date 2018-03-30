/**
 * @file server/channel/src/packets/game/ClanJoin.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to join a clan based on an invitation.
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
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::ClanJoin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 11)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int32_t sourceCID = p.ReadS32Little();
    int32_t clanID = p.ReadS32Little();
    int8_t unknown = p.ReadS8();
    libcomp::String sourceName = p.ReadString16Little(
        state->GetClientStringEncoding(), true);
    (void)sourceCID;
    (void)unknown;

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES);
    request.WriteS32Little(state->GetWorldCID());
    request.WriteS32Little(clanID);
    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
        sourceName, true);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

    return true;
}
