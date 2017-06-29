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
#include <MiItemBasicData.h>
#include <MiItemData.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

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

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));
    if(nullptr == item)
    {
        LOG_ERROR("Item move failed due to unknown item ID.\n");
        state->SetLogoutSave(true);
        client->Close();
        return true;
    }

    int8_t destType = p.ReadS8();
    int64_t destBoxID = p.ReadS64Little();
    int16_t destSlot = p.ReadS16Little();

    auto sourceBox = characterManager->GetItemBox(state, sourceType, sourceBoxID);
    auto destBox = characterManager->GetItemBox(state, destType, destBoxID);

    if(nullptr != sourceBox && nullptr != destBox)
    {
        auto character = state->GetCharacterState()->GetEntity();

        size_t sourceSlot = (size_t)item->GetBoxSlot();
        if(sourceBox->GetItems(sourceSlot).Get() != item)
        {
            LOG_ERROR("Item move operation failed.\n");
            state->SetLogoutSave(true);
            client->Close();
            return true;
        }

        if(destBox != sourceBox)
        {
            characterManager->UnequipItem(client, item);
        }

        // Swap the items (the destination could be a null object or a real item)
        item->SetItemBox(destBox);
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
            otherItem->SetItemBox(sourceBox);
            otherItem->SetBoxSlot((int8_t)sourceSlot);
            dbChanges->Update(otherItem.Get());
        }

        server->GetWorldDatabase()->QueueChangeSet(dbChanges);
    }
    else
    {
        LOG_ERROR("Item move failed due to invalid source or destination box.\n");
        state->SetLogoutSave(true);
        client->Close();
    }

    return true;
}
