/**
 * @file server/channel/src/packets/game/ShopSell.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to sell an item to a shop.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Account.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

// Result values
// 0: Success
// -1: too many items
// anything else: generic error
void SendShopSaleReply(const std::shared_ptr<ChannelClientConnection> client,
    int32_t shopID, int32_t result, bool queue)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOP_SELL);
    reply.WriteS32Little(shopID);
    reply.WriteS32Little(result);

    if(queue)
    {
        client->QueuePacket(reply);
    }
    else
    {
        client->SendPacket(reply);
    }
}

void HandleShopSale(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client, int32_t shopID,
    int32_t cacheID, std::list<std::pair<uint32_t, int64_t>> itemsSold)
{
    (void)cacheID;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();
    auto characterManager = server->GetCharacterManager();
    auto defnitionManager = server->GetDefinitionManager();

    uint64_t saleAmount = 0;
    std::list<std::shared_ptr<objects::Item>> deleteItems;
    std::map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems;
    for(auto pair : itemsSold)
    {
        auto item = std::dynamic_pointer_cast<objects::Item>(
            libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(
                pair.second)));
        auto amount = pair.first;
        if(!item || item->GetItemBox().Get() != inventory)
        {
            SendShopSaleReply(client, shopID, -2, false);
            return;
        }

        auto def = defnitionManager->GetItemData(item->GetType());
        if(!def)
        {
            SendShopSaleReply(client, shopID, -2, false);
            return;
        }

        saleAmount = (uint64_t)(saleAmount +
            ((uint64_t)def->GetBasic()->GetSellPrice() * amount));
        if(item->GetStackSize() == amount)
        {
            deleteItems.push_back(item);
        }
        else
        {
            stackAdjustItems[item] = (uint16_t)(item->GetStackSize() - amount);
        }
    }
    
    std::list<int8_t> freeSlots;
    for(int8_t i = 0; i < 50; i++)
    {
        if(inventory->GetItems((size_t)i).IsNull())
        {
            freeSlots.push_back(i);
        }
    }

    for(auto item : deleteItems)
    {
        freeSlots.push_back(item->GetBoxSlot());
    }

    freeSlots.unique();
    freeSlots.sort();

    int32_t macca = (int32_t)(saleAmount % (uint64_t)ITEM_MACCA_NOTE_AMOUNT);
    int32_t notes = (int32_t)floorl((double)(saleAmount - (uint64_t)macca) /
        (double)ITEM_MACCA_NOTE_AMOUNT);

    auto maxNoteStack = defnitionManager->GetItemData(SVR_CONST.ITEM_MACCA_NOTE)
        ->GetPossession()->GetStackSize();
    for(auto item : characterManager->GetExistingItems(character,
        SVR_CONST.ITEM_MACCA_NOTE, inventory))
    {
        if(notes == 0) break;

        int32_t stackLeft = (int32_t)(maxNoteStack - item->GetStackSize());
        if(stackLeft <= 0) continue;

        int32_t stackAdd = (notes <= stackLeft) ? notes : stackLeft;
        stackAdjustItems[item] = (uint16_t)(item->GetStackSize() + stackAdd);
        notes = (int32_t)(notes - stackAdd);
    }

    for(auto item : characterManager->GetExistingItems(character,
        SVR_CONST.ITEM_MACCA, inventory))
    {
        if(macca == 0) break;

        int32_t stackLeft = (int32_t)(ITEM_MACCA_NOTE_AMOUNT - item->GetStackSize());
        if(stackLeft <= 0) continue;

        int32_t stackAdd = (macca <= stackLeft) ? macca : stackLeft;
        stackAdjustItems[item] = (uint16_t)(item->GetStackSize() + stackAdd);
        macca = (int32_t)(macca - stackAdd);
    }

    // Add whatever amount is left as new items
    std::list<std::shared_ptr<objects::Item>> insertItems;
    while(notes > 0)
    {
        uint16_t stack = (notes > maxNoteStack) ? maxNoteStack : (uint16_t)notes;
        insertItems.push_back(characterManager->GenerateItem(
            SVR_CONST.ITEM_MACCA_NOTE, stack));
        notes = (int32_t)(notes - stack);
    }

    if(macca > 0)
    {
        insertItems.push_back(characterManager->GenerateItem(
            SVR_CONST.ITEM_MACCA, (uint16_t)macca));
    }

    if(freeSlots.size() < insertItems.size())
    {
        SendShopSaleReply(client, shopID, -1, false);
        return;
    }

    auto changes = libcomp::DatabaseChangeSet::Create();
    std::list<uint16_t> updatedSlots;

    // Delete the full stacks of items sold
    for(auto item : deleteItems)
    {
        // Unequip if equipped
        characterManager->UnequipItem(client, item);

        auto slot = item->GetBoxSlot();
        inventory->SetItems((size_t)slot, NULLUUID);
        changes->Delete(item);
        updatedSlots.push_back((uint16_t)slot);
    }

    // Insert the new items
    for(auto item : insertItems)
    {
        auto slot = freeSlots.front();
        freeSlots.erase(freeSlots.begin());

        item->SetItemBox(inventory);
        item->SetBoxSlot(slot);
        inventory->SetItems((size_t)slot, item);
        changes->Insert(item);
        updatedSlots.push_back((uint16_t)slot);
    }

    // Update item stacks
    for(auto iPair : stackAdjustItems)
    {
        auto item = iPair.first;
        item->SetStackSize(iPair.second);
        changes->Update(item);
        updatedSlots.push_back((uint16_t)item->GetBoxSlot());
    }

    changes->Update(inventory);

    // Queue the changes up and notify the client of the changes
    server->GetWorldDatabase()->QueueChangeSet(changes);

    SendShopSaleReply(client, shopID, 0, true);

    updatedSlots.unique();
    updatedSlots.sort();
    characterManager->SendItemBoxData(client, inventory, updatedSlots);
}

bool Parsers::ShopSell::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 12)
    {
        return false;
    }

    int32_t shopID = p.ReadS32Little();
    int32_t cacheID = p.ReadS32Little();
    int32_t itemCount = p.ReadS32Little();
    std::list<std::pair<uint32_t, int64_t>> itemsSold;

    if(itemCount < 0 || p.Left() != (uint32_t)(12 * itemCount))
    {
        return false;
    }

    for(int32_t i = 0; i < itemCount; i++)
    {
        auto itemID = p.ReadS64Little();
        auto stackSize = p.ReadU32Little();

        itemsSold.push_back(std::pair<uint32_t, int64_t>(
            stackSize, itemID));
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    server->QueueWork(HandleShopSale, server, client, shopID, cacheID, itemsSold);

    return true;
}
