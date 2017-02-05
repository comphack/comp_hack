/**
 * @file server/channel/src/packets/game/ItemBox.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for info about a specific item box.
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
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>
#include <Item.h>
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

void SendItemBox(const std::shared_ptr<ChannelClientConnection>& client,
    int64_t boxID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetCharacter();
    auto box = character->GetItemBoxes((size_t)boxID);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_BOX);
    reply.WriteS8(box->GetType());
    reply.WriteS64(state->GetObjectID(box->GetUUID()));
    reply.WriteS32(0);  //Unknown
    reply.WriteU16Little(50); // Max Item Count
    reply.WriteS32Little(0); // Unknown

    int32_t usedSlots = 0;
    for(auto item : box->GetItems())
    {
        if(!item.IsNull())
        {
            usedSlots++;
        }
    }

    reply.WriteS32Little(usedSlots);

    for(uint16_t i = 0; i < 50; i++)
    {
        auto item = box->GetItems(i);

        if(item.IsNull()) continue;

        reply.WriteU16Little(i);    //Slot
        reply.WriteS64Little(
            state->GetObjectID(item->GetUUID()));
        reply.WriteU32Little(item->GetType());
        reply.WriteU16Little(item->GetStackSize());
        reply.WriteU16Little(item->GetDurability());
        reply.WriteS8(item->GetMaxDurability());

        reply.WriteS16Little(item->GetTarot());
        reply.WriteS16Little(item->GetSoul());

        for(auto modSlot : item->GetModSlots())
        {
            reply.WriteU16Little(modSlot);
        }

        reply.WriteS32Little(0);    //Unknown
        /*reply.WriteU8(0);   //Unknown
        reply.WriteS16Little(0);   //Unknown
        reply.WriteS16Little(0);   //Unknown
        reply.WriteU8(0); // Failed Item Fuse 0 = OK | 1 = FAIL*/

        auto basicEffect = item->GetBasicEffect();
        if(basicEffect)
        {
            reply.WriteU32Little(basicEffect);
        }
        else
        {
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }

        auto specialEffect = item->GetSpecialEffect();
        if(specialEffect)
        {
            reply.WriteU32Little(specialEffect);
        }
        else
        {
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }

        for(auto bonus : item->GetFuseBonuses())
        {
            reply.WriteS8(bonus);
        }
    }

    client->SendPacket(reply);
}

bool Parsers::ItemBox::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 9)
    {
        return false;
    }

    int8_t type = p.ReadS8();
    int64_t boxID = p.ReadS64Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    if(type == 0 && boxID == 0)
    {
        server->QueueWork(SendItemBox, client, boxID);
    }
    else
    {
        /// @todo
        return false;
    }

    return true;
}
