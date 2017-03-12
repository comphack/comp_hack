/**
 * @file server/channel/src/packets/game/ItemStack.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to stack or split stacked items in an item box.
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

/// @todo: does this work on item boxes that are not the inventory?

void SplitStack(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    std::pair<uint32_t, uint16_t> sourceItem)
{
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();
    auto itemBox = character->GetItemBoxes(0).Get();

    size_t srcSlot = (size_t)sourceItem.first;
    uint16_t srcStack = sourceItem.second;

    auto srcItem = itemBox->GetItems(srcSlot).Get();
    auto destItem = std::shared_ptr<objects::Item>(
        new objects::Item(*srcItem));

    size_t destSlot = 0;
    for(; destSlot < 50; destSlot++)
    {
        if(itemBox->GetItems(destSlot).IsNull())
        {
            break;
        }
    }

    if(destSlot == 50)
    {
        LOG_ERROR(libcomp::String("Split stack failed because there was"
            " no empty slot available for character: %1\n").Arg(
                character->GetUUID().ToString()));
        return;
    }

    srcItem->SetStackSize(static_cast<uint16_t>(
        srcItem->GetStackSize() - srcStack));
    destItem->SetStackSize(srcStack);
    destItem->SetBoxSlot((int8_t)destSlot);

    auto worldDB = server->GetWorldDatabase();
    if(destItem->Register(destItem) && destItem->Insert(worldDB) &&
        srcItem->Update(worldDB) && itemBox->SetItems(destSlot, destItem) &&
        itemBox->Update(worldDB))
    {
        state->SetObjectID(destItem->GetUUID(),
            server->GetNextObjectID());

        server->GetCharacterManager()->SendItemBoxData(client, 0);
    }
    else
    {
        LOG_ERROR(libcomp::String("Save failed during split stack operation"
            " which may have resulted in loss of data for character: %1\n")
            .Arg(character->GetUUID().ToString()));
    }
}

void CombineStacks(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    const std::list<std::pair<uint32_t, uint16_t>> sourceItems,
    uint32_t targetSlot)
{
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();
    auto itemBox = character->GetItemBoxes(0).Get();

    auto targetItem = itemBox->GetItems(targetSlot);

    std::list<std::shared_ptr<objects::Item>> deleteItems;
    for(auto sourceItem : sourceItems)
    {
        size_t srcSlot = (size_t)sourceItem.first;
        uint16_t srcStack = sourceItem.second;

        auto srcItem = itemBox->GetItems(srcSlot).Get();
        srcItem->SetStackSize(static_cast<uint16_t>(
            srcItem->GetStackSize() - srcStack));

        if(srcItem->GetStackSize() == 0)
        {
            deleteItems.push_back(srcItem);
            itemBox->SetItems(srcSlot, NULLUUID);
        }

        targetItem->SetStackSize(static_cast<uint16_t>(
            targetItem->GetStackSize() + srcStack));
    }

    if(deleteItems.size() > 0)
    {
        auto worldDB = server->GetWorldDatabase();

        auto objs = libcomp::PersistentObject::ToList<objects::Item>(
            deleteItems);
        if(!worldDB->DeleteObjects(objs) || !itemBox->Update(worldDB))
        {
            LOG_ERROR(libcomp::String("Save failed during combine stack operation"
                " which may have resulted in invalid item data for character: %1\n")
                .Arg(character->GetUUID().ToString()));
        }
    }
}

bool Parsers::ItemStack::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 14)
    {
        return false;
    }
    
    uint32_t srcItemCount = p.ReadU32Little();

    std::list<std::pair<uint32_t, uint16_t>> srcItems;
    for(uint32_t i = 0; i < srcItemCount; i++)
    {
        uint32_t slot = p.ReadU32Little();
        uint16_t stack = p.ReadU16Little();

        if(slot >= 50)
        {
            LOG_ERROR("Invalid item box source slot specified in item"
                " stack request.");
            return false;
        }

        srcItems.push_back(std::pair<uint32_t, uint16_t>(
            slot, stack));
    }

    if(srcItems.size() == 0)
    {
        LOG_ERROR("No source items defined in item stack request.");
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);

    uint32_t targetSlot = p.ReadU32Little();
    if(targetSlot == static_cast<uint32_t>(-1))
    {
        server->QueueWork(SplitStack, server, client, srcItems.front());
    }
    else
    {
        if(targetSlot >= 50)
        {
            LOG_ERROR("Invalid item box target slot specified in item"
                " stack request.");
            return false;
        }

        server->QueueWork(CombineStacks, server, client, srcItems, targetSlot);
    }

    return true;
}
