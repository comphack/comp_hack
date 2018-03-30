/**
 * @file server/channel/src/packets/game/ClanData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update data related to their clan
 *  member information.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::ClanData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 8)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int32_t clanID = p.ReadS32Little();
    int8_t unknown1 = p.ReadS8();
    int8_t unknown2 = p.ReadS8();
    libcomp::String message = p.ReadString16Little(
        state->GetClientStringEncoding(), true);
    (void)unknown1;
    (void)unknown2;

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_UPDATE);
    request.WriteS32Little(state->GetWorldCID());
    request.WriteS32Little(clanID);
    request.WriteS8((int8_t)CharacterLoginStateFlag_t::CHARLOGIN_MESSAGE);
    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
        message, true);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

    return true;
}
