/**
 * @file server/channel/src/packets/game/SearchEntryInfo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the current player's search entry
 *  types.
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

// object Includes
#include <SearchEntry.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelSyncManager.h"

using namespace channel;

bool Parsers::SearchEntryInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto syncManager = server->GetChannelSyncManager();
    int32_t worldCID = client->GetClientState()->GetWorldCID();

    std::set<int8_t> types;

    auto entries = syncManager->GetSearchEntries();
    for(auto ePair : entries)
    {
        for(auto entry : ePair.second)
        {
            if(entry->GetSourceCID() == worldCID)
            {
                types.insert((int8_t)ePair.first);
                break;
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_INFO);
    reply.WriteS8((int8_t)types.size());
    for(int8_t type : types)
    {
        reply.WriteS8(type);
    }

    client->SendPacket(reply);

    return true;
}
