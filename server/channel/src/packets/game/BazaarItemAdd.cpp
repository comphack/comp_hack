/**
 * @file server/channel/src/packets/game/BazaarItemAdd.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to add an item to the player's bazaar market.
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

// objects Includes
#include <Item.h>
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::BazaarItemAdd::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 13)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int8_t slot = p.ReadS8();
    int64_t itemID = p.ReadS64Little();
    int32_t price = p.ReadS32Little();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_ITEM_ADD);
    reply.WriteS8(slot);
    reply.WriteS64Little(itemID);
    reply.WriteS32Little(price);

    int8_t oldSlot = item ? item->GetBoxSlot() : -1;
    auto box = item ? item->GetItemBox().Get() : nullptr;

    auto dbChanges = libcomp::DatabaseChangeSet::Create();
    auto bState = state->GetBazaarState();
    if(bState && bState->AddItem(state, slot, itemID, price, dbChanges))
    {
        // Unequip if its equipped
        server->GetCharacterManager()->UnequipItem(client, item);

        if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
        {
            LOG_ERROR(libcomp::String("BazaarItemAdd failed to save: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            state->SetLogoutSave(false);
            client->Close();
            return true;
        }

        std::list<uint16_t> updatedSlots = { (uint16_t)oldSlot };
        server->GetCharacterManager()->SendItemBoxData(client, box, updatedSlots);

        reply.WriteS32Little(0); // Success
    }
    else
    {
        reply.WriteS32Little(-1); // Failure
    }

    client->SendPacket(reply);

    return true;
}
