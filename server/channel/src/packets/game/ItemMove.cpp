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
#include <Item.h>
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

void MoveItem(const std::shared_ptr<ChannelClientConnection>& client,
    int64_t itemID, const std::shared_ptr<objects::ItemBox> sourceBox,
    const std::shared_ptr<objects::ItemBox> destBox, size_t destSlot,
    const std::shared_ptr<ChannelServer>& server)
{
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();
    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(
            state->GetObjectUUID(itemID)));

    size_t sourceSlot = (size_t)item->GetBoxSlot();

    if(sourceBox->GetItems(sourceSlot).Get() != item)
    {
        LOG_ERROR(libcomp::String("Item move operation failed due to unknown "
            "supplied item ID %1 on character: %2\n").Arg(itemID).Arg(
            character->GetUUID().ToString()));

        return;
    }

    // Swap the items (the destination could be a null object or a real item).
    item->SetBoxSlot((int8_t)destSlot);
    auto otherItem = destBox->GetItems(destSlot);
    destBox->SetItems(destSlot, item);
    sourceBox->SetItems(sourceSlot, otherItem);

    auto db = server->GetWorldDatabase();

    // Save the item boxes.
    bool dbWrite = destBox->Update(db);

    if(destBox.get() != sourceBox.get())
    {
        dbWrite &= sourceBox->Update(db);
    }

    if(!dbWrite)
    {
        LOG_ERROR(libcomp::String("Save failed during item move operation "
            "which may have resulted in invalid item data for character: %1\n")
            .Arg(character->GetUUID().ToString()));
    }
}

bool Parsers::ItemMove::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 28)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);

    int8_t sourceType = p.ReadS8();
    int64_t sourceBoxID = p.ReadS64Little();
    int64_t itemID = p.ReadS64Little();

    auto uuid = client->GetClientState()->GetObjectUUID(itemID);

    if(uuid.IsNull() || nullptr == std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(uuid)))
    {
        return false;
    }

    int8_t destType = p.ReadS8();
    int64_t destBoxID = p.ReadS64Little();
    int16_t destSlot = p.ReadS16Little();
    auto character = client->GetClientState()->
        GetCharacterState()->GetEntity();

    std::shared_ptr<objects::ItemBox> sourceBox;
    if(sourceType == 0 && sourceBoxID == 0)
    {
        sourceBox = character->GetItemBoxes(0).Get();
    }
    else
    {
        /// @todo
        LOG_ERROR("Item move request sent using a non-inventory item box.\n");
        return false;
    }
    
    std::shared_ptr<objects::ItemBox> destBox;
    if(destType == 0 && destBoxID == 0)
    {
        destBox = character->GetItemBoxes(0).Get();
    }
    else
    {
        /// @todo
        LOG_ERROR("Item move request sent using a non-inventory item box.\n");
        return false;
    }

    if(nullptr != sourceBox && nullptr != destBox)
    {
        server->QueueWork(MoveItem, client, itemID, sourceBox,
            destBox, (size_t)destSlot, server);
    }
    else
    {
        return false;
    }

    return true;
}
