/**
 * @file server/channel/src/packets/game/PostList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to retrieve items currently in the player's
 *  Post.
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

// object Includes
#include <AccountWorldData.h>
#include <PostItem.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::PostList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto worldDB = server->GetWorldDatabase();

    int32_t slotsRemaining = p.ReadS32Little();
    int32_t itemIdx = p.ReadS32Little();

    // Always reload items and the post
    auto worldData = objects::AccountWorldData::LoadAccountWorldDataByAccount(worldDB,
        state->GetAccountUID());
    auto postItems = objects::PostItem::LoadPostItemListByAccount(worldDB,
        state->GetAccountUID());

    // Adjust the index to only return the ones the client doesn't
    // know about as the client only wants new items
    itemIdx = (int32_t)(itemIdx + (21 - slotsRemaining));

    // Pull the items starting at the index
    std::list<std::shared_ptr<objects::PostItem>> items;
    for(int32_t i = itemIdx; i < (int32_t)(itemIdx + slotsRemaining); i++)
    {
        auto item = worldData->GetPost((size_t)i);
        if(!item.IsNull())
        {
            items.push_back(item.Get());
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_POST_LIST);
    reply.WriteS32Little(0);    // Unknown
    reply.WriteS32Little((int32_t)items.size());

    for(auto item : items)
    {
        reply.WriteS32Little(state->GetLocalObjectID(item->GetUUID()));
        reply.WriteS8(0);           // Item flags (limited time etc)
        reply.WriteS32Little(-1);   // Unknown
        reply.WriteS32Little((int32_t)item->GetType());
        reply.WriteS32Little((int32_t)item->GetTimestamp());
        reply.WriteS32Little(1);    // Unknown
    }

    reply.WriteS32Little(itemIdx);
    reply.WriteS32Little((int32_t)worldData->PostCount());

    connection->SendPacket(reply);

    return true;
}
