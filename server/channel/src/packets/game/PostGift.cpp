/**
 * @file server/channel/src/packets/game/PostGift.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for post gift information.
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

// object Includes
#include <PostItem.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::PostGift::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto lobbyDB = server->GetLobbyDatabase();

    int32_t postID = p.ReadS32Little();

    auto itemUUID = state->GetLocalObjectUUID(postID);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_POST_GIFT);
    reply.WriteS32Little(postID);

    bool success = false;
    if(!itemUUID.IsNull())
    {
        auto postItem = libcomp::PersistentObject::LoadObjectByUUID<
            objects::PostItem>(lobbyDB, itemUUID);
        if(postItem)
        {
            reply.WriteS32Little(0);
            reply.WriteS8(0);
            reply.WriteString16Little(state->GetClientStringEncoding(),
                postItem->GetFromName(), true);
            reply.WriteString16Little(state->GetClientStringEncoding(),
                postItem->GetGiftMessage(), true);

            success = true;
        }
    }

    if(!success)
    {
        reply.WriteS32Little(-1);
    }

    client->SendPacket(reply);

    return true;
}
