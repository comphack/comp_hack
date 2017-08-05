/**
 * @file server/channel/src/packets/game/ClanList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to list member details. This will be
 *  requested multiple times until every member is described.
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

bool Parsers::ClanList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 5)
    {
        return false;
    }

    std::list<int32_t> worldCIDs;

    int32_t unknown = p.ReadS32Little();
    int8_t cidCount = p.ReadS8();
    (void)unknown;

    if(p.Left() != (uint32_t)(cidCount * 4))
    {
        return false;
    }

    for(int8_t i = 0; i < cidCount; i++)
    {
        worldCIDs.push_back(p.ReadS32Little());
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LIST);
    request.WriteS32Little(state->GetWorldCID());
    request.WriteU8(1); // Member level info
    request.WriteU16Little((uint16_t)cidCount);
    for(int32_t worldCID : worldCIDs)
    {
        request.WriteS32Little(worldCID);
    }

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

    return true;
}
