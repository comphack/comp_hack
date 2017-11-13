/**
 * @file server/channel/src/packets/game/ItemExpand.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to expand a compressed item such as
 *  macca or mag.
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
#include <ServerConstants.h>

// object Includes
#include <Item.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ItemExpand::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 9)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto state = client->GetClientState();

    int64_t itemID = p.ReadS64Little();
    int8_t boxID = p.ReadS8();
    (void)boxID;

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    int32_t responseCode = -3;   // Generic error, no message

    if(boxID != 0)
    {
        LOG_ERROR(libcomp::String("Invalid box ID encountered for ItemExpand request: %1")
            .Arg(boxID));
    }
    else if(!state->GetCharacterState()->IsAlive())
    {
        responseCode = -2;  // Cannot be used here
    }
    else if(item && item->GetStackSize() > 0 &&
        (item->GetType() == SVR_CONST.ITEM_MACCA_NOTE ||
        item->GetType() == SVR_CONST.ITEM_MAG_PRESSER))
    {
        std::shared_ptr<objects::Item> newItem;
        if(item->GetType() == SVR_CONST.ITEM_MACCA_NOTE)
        {
            // Convert to macca
            newItem = characterManager->GenerateItem(SVR_CONST.ITEM_MACCA,
                ITEM_MACCA_NOTE_AMOUNT);
        }
        else
        {
            // Convert to mag
            newItem = characterManager->GenerateItem(SVR_CONST.ITEM_MAGNETITE,
                ITEM_MAG_PRESSER_AMOUNT);
        }

        if(newItem)
        {
            std::list<std::shared_ptr<objects::Item>> inserts;
            std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> updates;

            inserts.push_back(newItem);
            updates[item] = (uint16_t)(item->GetStackSize() - 1);

            if(characterManager->UpdateItems(client, false, inserts, updates))
            {
                responseCode = 0;   // Success
            }
            else
            {
                responseCode = -1;  // Not enough space
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_EXPAND);
    reply.WriteS64Little(itemID);
    reply.WriteS8(boxID);
    reply.WriteS32Little(responseCode);

    client->SendPacket(reply);

    return true;
}
