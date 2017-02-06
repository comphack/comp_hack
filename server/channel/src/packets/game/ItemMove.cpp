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
    const std::shared_ptr<objects::ItemBox> destBox, size_t destSlot)
{
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetCharacter();

    auto sourceItems = sourceBox->GetItems();

    size_t sourceSlot = 0;
    std::shared_ptr<objects::Item> item;
    for(; sourceSlot < 50; sourceSlot++)
    {
        if(!sourceItems[sourceSlot].IsNull() &&
            state->GetObjectID(sourceItems[sourceSlot].GetUUID()) == itemID)
        {
            item = sourceItems[sourceSlot].Get();
            break;
        }
    }

    if(nullptr == item)
    {
        LOG_ERROR(libcomp::String("Item move operation failed due to unknown supplied"
            " item ID on character: %1\n").Arg(character->GetUUID().ToString()));
        return;
    }

    destBox->SetItems(destSlot, item);

    libcomp::ObjectReference<objects::Item> empty;
    sourceBox->SetItems(sourceSlot, empty);

    // Nothing was created or deleted so just let the next save operation pick this up
}

bool Parsers::ItemMove::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 28)
    {
        return false;
    }

    int8_t sourceType = p.ReadS8();
    int64_t sourceBoxID = p.ReadS64Little();
    int64_t itemID = p.ReadS64Little();

    int8_t destType = p.ReadS8();
    int64_t destBoxID = p.ReadS64Little();
    int16_t destSlot = p.ReadS16Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto character = client->GetClientState()->
        GetCharacterState()->GetCharacter();

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
            destBox, (size_t)destSlot);
    }
    else
    {
        return false;
    }

    return true;
}
