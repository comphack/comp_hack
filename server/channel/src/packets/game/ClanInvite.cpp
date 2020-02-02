/**
 * @file server/channel/src/packets/game/ClanInvite.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to invite another character to their current
 *  clan.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

bool Parsers::ClanInvite::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 6)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int32_t clanID = p.ReadS32Little();
    libcomp::String targetName = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_YN_REQUEST);
    request.WriteS32Little(state->GetWorldCID());
    request.WriteS32Little(clanID);
    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
        targetName, true);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

    return true;
}
