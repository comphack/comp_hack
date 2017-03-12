/**
 * @file server/channel/src/packets/game/HotbarData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for info about a hotbar page.
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
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>
#include <Hotbar.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

void SendHotbarData(const std::shared_ptr<ChannelClientConnection> client,
    size_t page)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto hotbar = character->GetHotbars(page).Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_HOTBAR_DATA);
    reply.WriteS8((int8_t)page);
    reply.WriteS32(0);

    for(size_t i = 0; i < 16; i++)
    {
        auto type = hotbar != nullptr ? hotbar->GetItemTypes(i) : (int8_t)0;
        auto item = hotbar != nullptr ? hotbar->GetItems(i).Get() : nullptr;
        auto itemID = (int64_t)(hotbar != nullptr ? hotbar->GetItemIDs(i) : 0);

        if(nullptr != item)
        {
            itemID = state->GetObjectID(item->GetUUID());
        }

        reply.WriteS8(itemID != 0 ? type : 0);
        reply.WriteS64(itemID);
    }

    client->SendPacket(reply);
}

bool Parsers::HotbarData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    int8_t page = p.ReadS8();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    server->QueueWork(SendHotbarData, client, (size_t)page);

    return true;
}
