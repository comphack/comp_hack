/**
 * @file server/channel/src/packets/game/ItemMove.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to move an item in an item box.
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
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::ItemMove::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 28)
    {
        return false;
    }

    // Since there is no fail state to send back to the client, if there is a failure
    // past this point, the client should be disconnected

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto characterManager = server->GetCharacterManager();

    int8_t sourceType = p.ReadS8();
    int64_t sourceBoxID = p.ReadS64Little();
    int64_t itemID = p.ReadS64Little();
    int8_t destType = p.ReadS8();
    int64_t destBoxID = p.ReadS64Little();
    int16_t destSlot = p.ReadS16Little();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));
    auto sourceBox = characterManager->GetItemBox(state, sourceType, sourceBoxID);
    auto destBox = characterManager->GetItemBox(state, destType, destBoxID);
    size_t sourceSlot = item ? (size_t)item->GetBoxSlot() : 0;

    if(item && sourceBox && destBox && sourceBox->GetItems(sourceSlot).Get() == item)
    {
        auto character = state->GetCharacterState()->GetEntity();
        bool sameBox = destBox == sourceBox;
        if(!sameBox)
        {
            characterManager->UnequipItem(client, item);
        }

        // Swap the items (the destination could be a null object or a real item)
        item->SetItemBox(destBox->GetUUID());
        item->SetBoxSlot((int8_t)destSlot);

        auto otherItem = destBox->GetItems((size_t)destSlot);
        destBox->SetItems((size_t)destSlot, item);
        sourceBox->SetItems(sourceSlot, otherItem);

        auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
        dbChanges->Update(item);
        dbChanges->Update(destBox);
        dbChanges->Update(sourceBox);

        if(!otherItem.IsNull())
        {
            otherItem->SetItemBox(sourceBox->GetUUID());
            otherItem->SetBoxSlot((int8_t)sourceSlot);
            dbChanges->Update(otherItem.Get());
        }

        server->GetWorldDatabase()->QueueChangeSet(dbChanges);

        // The client will handle moves just fine on its own for the most part but
        // certain simultaneous actions will cause some weirdness without sending
        // the updated slots back
        std::list<uint16_t> slots = { (uint16_t)sourceSlot };
        if(sameBox)
        {
            slots.push_back((uint16_t)destSlot);
        }

        characterManager->SendItemBoxData(client, sourceBox, slots, false);

        if(!sameBox)
        {
            slots.clear();
            slots.push_back((uint16_t)destSlot);

            characterManager->SendItemBoxData(client, destBox, slots, true);
        }
    }
    else
    {
        LOG_DEBUG(libcomp::String("ItemMove request failed. Notifying"
            " requestor: %1\n").Arg(state->GetAccountUID().ToString()));

        libcomp::Packet err;
        err.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ERROR_ITEM);
        err.WriteS32Little((int32_t)
            ClientToChannelPacketCode_t::PACKET_ITEM_MOVE);
        err.WriteS32Little(-1);
        err.WriteS8(1);
        err.WriteS8(1);

        client->SendPacket(err);
    }

    return true;
}
