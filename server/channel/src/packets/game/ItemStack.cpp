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
#include <DefinitionManager.h>
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
#include <MiItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"
#include "CharacterManager.h"

using namespace channel;

void SplitStack(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    std::pair<uint32_t, uint16_t> sourceItem, uint32_t targetSlot)
{
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();
    auto itemBox = character->GetItemBoxes(0).Get();

    size_t srcSlot = (size_t)sourceItem.first;
    uint16_t srcStack = sourceItem.second;

    auto srcItem = itemBox->GetItems(srcSlot).Get();

    bool valid = srcItem != nullptr;
    if(valid)
    {
        bool targetSpecified = targetSlot != static_cast<uint32_t>(-1);
        if(!targetSpecified)
        {
            targetSlot = 0;
            for(; targetSlot < 50; targetSlot++)
            {
                if(itemBox->GetItems(targetSlot).IsNull())
                {
                    break;
                }
            }
        }

        if(targetSlot == 50 || !itemBox->GetItems((size_t)targetSlot).IsNull())
        {
            LOG_ERROR(libcomp::String("Split stack failed because there was"
                " no empty slot available: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            valid = false;
        }

        int32_t oldStack = (int32_t)srcItem->GetStackSize() - (int32_t)srcStack;
        if(oldStack <= 0)
        {
            LOG_ERROR(libcomp::String("Split stack attempted with a"
                " source stack smaller than the required amount: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            valid = false;
        }

        if(valid)
        {
            auto destItem = std::make_shared<objects::Item>(*srcItem);

            srcItem->SetStackSize((uint16_t)oldStack);
            destItem->SetStackSize(srcStack);
            destItem->SetBoxSlot((int8_t)targetSlot);

            destItem->Register(destItem);
            itemBox->SetItems((size_t)targetSlot, destItem);

            state->SetObjectID(destItem->GetUUID(),
                server->GetNextObjectID());

            std::list<uint16_t> updatedSlots = { (uint16_t)srcSlot,
                (uint16_t)targetSlot };
            server->GetCharacterManager()->SendItemBoxData(client, itemBox,
                updatedSlots, false);

            auto dbChanges = libcomp::DatabaseChangeSet::Create(
                state->GetAccountUID());
            dbChanges->Insert(destItem);
            dbChanges->Update(srcItem);
            dbChanges->Update(itemBox);
            server->GetWorldDatabase()->QueueChangeSet(dbChanges);
        }
    }

    if(!valid)
    {
        LOG_DEBUG(libcomp::String("ItemStack split request failed. Notifying"
            " requestor: %1\n").Arg(state->GetAccountUID().ToString()));

        libcomp::Packet err;
        err.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ERROR_ITEM);
        err.WriteS32Little((int32_t)
            ClientToChannelPacketCode_t::PACKET_ITEM_STACK);
        err.WriteS32Little(-1);
        err.WriteS8(0);
        err.WriteS8(0);

        client->SendPacket(err);
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

    auto targetItem = itemBox->GetItems(targetSlot).Get();
    auto itemDef = targetItem ? server->GetDefinitionManager()
        ->GetItemData(targetItem->GetType()) : nullptr;

    bool valid = false;
    if(itemDef)
    {
        valid = true;

        int32_t maxStack = (int32_t)itemDef->GetPossession()->GetStackSize();

        auto dbChanges = libcomp::DatabaseChangeSet::Create(
            state->GetAccountUID());
        dbChanges->Update(itemBox);
        for(auto sourceItem : sourceItems)
        {
            size_t srcSlot = (size_t)sourceItem.first;
            uint16_t srcStack = sourceItem.second;

            auto srcItem = itemBox->GetItems(srcSlot).Get();
            if(srcItem)
            {
                if(srcItem->GetType() != targetItem->GetType())
                {
                    LOG_ERROR(libcomp::String("Combine stack attempted with"
                        " items of differing types: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                    valid = false;
                    break;
                }

                int32_t newStack = (int32_t)targetItem->GetStackSize() +
                    (int32_t)srcStack;
                if(newStack > maxStack)
                {
                    LOG_ERROR(libcomp::String("Combine stack attempted with a"
                        " target stack larger than the max size: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                    valid = false;
                    break;
                }

                int32_t oldStack = (int32_t)srcItem->GetStackSize() -
                    (int32_t)srcStack;
                if(oldStack < 0)
                {
                    LOG_ERROR(libcomp::String("Combine stack attempted with a"
                        " source stack smaller than the requested amount: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                    valid = false;
                    break;
                }

                if(srcItem == targetItem)
                {
                    LOG_ERROR(libcomp::String("Player '%1' on connection '%2' "
                        "tried to merge an item into itself! Killing their "
                        "connection.\n").Arg(character->GetName()).Arg(
                        client->GetName()));
                    client->Close();
                    valid = false;
                    break;
                }

                srcItem->SetStackSize((uint16_t)oldStack);

                if(srcItem->GetStackSize() == 0)
                {
                    dbChanges->Delete(srcItem);
                    itemBox->SetItems(srcSlot, NULLUUID);
                }
                else
                {
                    dbChanges->Update(srcItem);
                }

                targetItem->SetStackSize((uint16_t)newStack);

                dbChanges->Update(targetItem);
            }
            else
            {
                valid = false;
                break;
            }
        }

        // Save anything that processed correctly
        server->GetWorldDatabase()->QueueChangeSet(dbChanges);
    }

    if(!valid)
    {
        LOG_DEBUG(libcomp::String("ItemStack stack request failed. Notifying"
            " requestor: %1\n").Arg(state->GetAccountUID().ToString()));

        libcomp::Packet err;
        err.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ERROR_ITEM);
        err.WriteS32Little((int32_t)
            ClientToChannelPacketCode_t::PACKET_ITEM_STACK);
        err.WriteS32Little(-1);
        err.WriteS8(0);
        err.WriteS8(0);

        client->SendPacket(err);
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

    // A stack is being requested when the target slot contains an item.
    // If the target slot is -1, a split is being requested to the next
    // available slot.
    bool isSplit = true;
    uint32_t targetSlot = p.ReadU32Little();
    if(targetSlot != static_cast<uint32_t>(-1))
    {
        if(targetSlot >= 50)
        {
            LOG_ERROR("Invalid item box target slot specified in item"
                " stack request.");
            return false;
        }

        auto state = client->GetClientState();
        auto character = state->GetCharacterState()->GetEntity();
        auto itemBox = character->GetItemBoxes(0).Get();
        isSplit = itemBox->GetItems((size_t)targetSlot).IsNull();
    }

    if(isSplit)
    {
        server->QueueWork(SplitStack, server, client, srcItems.front(), targetSlot);
    }
    else
    {
        server->QueueWork(CombineStacks, server, client, srcItems, targetSlot);
    }

    return true;
}
