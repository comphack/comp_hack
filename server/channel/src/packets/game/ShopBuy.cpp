/**
 * @file server/channel/src/packets/game/ShopBuy.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to buy an item from a shop.
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
#include <ServerConstants.h>

// object Includes
#include <Account.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemData.h>
#include <MiPossessionData.h>
#include <MiShopProductData.h>
#include <ServerShop.h>
#include <ServerShopProduct.h>
#include <ServerShopTab.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

// Result values
// 0: Success
// -1: too many items
// anything else: error dialog
void SendShopPurchaseReply(const std::shared_ptr<ChannelClientConnection> client,
    int32_t shopID, int32_t productID, int32_t result, bool queue)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOP_BUY);
    reply.WriteS32Little(shopID);
    reply.WriteS32Little(productID);
    reply.WriteS32Little(result);
    reply.WriteS8(1);   // Unknown
    reply.WriteS32Little(0);   // Unknown

    if(queue)
    {
        client->QueuePacket(reply);
    }
    else
    {
        client->SendPacket(reply);
    }
}

void HandleShopPurchase(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int32_t shopID, int32_t cacheID, int32_t productID, int32_t quantity)
{
    (void)cacheID;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();
    auto characterManager = server->GetCharacterManager();
    auto defnitionManager = server->GetDefinitionManager();

    auto shop = server->GetServerDataManager()->GetShopData((uint32_t)shopID);
    auto product = defnitionManager->GetShopProductData((uint32_t)productID);
    auto def = product != nullptr
        ? defnitionManager->GetItemData(product->GetItem()) : nullptr;
    if(!shop || !product || !def)
    {
        LOG_ERROR(libcomp::String("Invalid shop purchase: shopID=%1, productID=%2\n")
            .Arg(shopID).Arg(productID));
        SendShopPurchaseReply(client, shopID, productID, -2, false);
        return;
    }

    int32_t price = 0;
    bool productFound = false;
    for(auto tab : shop->GetTabs())
    {
        for(auto p : tab->GetProducts())
        {
            if(p->GetProductID() == productID)
            {
                productFound = true;
                price = p->GetBasePrice();
                break;
            }
        }

        if(productFound)
        {
            break;
        }
    }

    if(!productFound)
    {
        LOG_ERROR(libcomp::String("Shop '%1' does not contain product '%2'\n")
            .Arg(shopID).Arg(productID));
        SendShopPurchaseReply(client, shopID, productID, -2, false);
        return;
    }

    // Sanity check price
    if(price <= 0)
    {
        price = 1;
    }

    // CP purchases are only partially defined in the server files
    bool cpPurchase = product->GetCPCost() > 0;
    if(cpPurchase)
    {
        quantity = (int32_t)product->GetStack();
    }

    price = price * quantity;

    uint16_t stackDecrease = 0;
    std::shared_ptr<objects::Item> updateItem;
    std::list<std::shared_ptr<objects::Item>> insertItems;
    std::list<std::shared_ptr<objects::Item>> deleteItems;
    if(!cpPurchase)
    {
        // Macca purchase
        auto macca = characterManager->GetExistingItems(character, SVR_CONST.ITEM_MACCA,
            character->GetItemBoxes(0).Get());
        auto maccaNotes = characterManager->GetExistingItems(character,
            SVR_CONST.ITEM_MACCA_NOTE, character->GetItemBoxes(0).Get());

        uint64_t totalMacca = 0;
        for(auto m : macca)
        {
            totalMacca += m->GetStackSize();
        }

        for(auto m : maccaNotes)
        {
            totalMacca += (uint64_t)(m->GetStackSize() * ITEM_MACCA_NOTE_AMOUNT);
        }

        if((uint64_t)price > totalMacca)
        {
            LOG_ERROR(libcomp::String("Attempted to buy an item the player could"
                " not afford: %1\n").Arg(state->GetAccountUID().ToString()));
            SendShopPurchaseReply(client, shopID, productID, -2, false);
            return;
        }

        // Remove last first, starting with macca
        macca.reverse();
        maccaNotes.reverse();

        uint16_t priceLeft = (uint16_t)price;
        for(auto m : macca)
        {
            if (priceLeft == 0) break;

            auto stack = m->GetStackSize();
            if(stack > priceLeft)
            {
                stackDecrease = (uint16_t)(stack - priceLeft);
                priceLeft = 0;
                updateItem = m;
            }
            else
            {
                priceLeft = (uint16_t)(priceLeft - stack);
                deleteItems.push_back(m);
            }
        }

        for(auto m : maccaNotes)
        {
            if (priceLeft == 0) break;

            auto stack = m->GetStackSize();
            uint16_t stackAmount = (uint16_t)(stack * ITEM_MACCA_NOTE_AMOUNT);
            if(stackAmount > priceLeft)
            {
                auto delta = stackAmount - priceLeft;
                uint16_t maccaLeft = (uint16_t)(delta % ITEM_MACCA_NOTE_AMOUNT);

                stackDecrease = (uint16_t)(stack - (delta - maccaLeft) / ITEM_MACCA_NOTE_AMOUNT);
                priceLeft = 0;
                updateItem = m;

                if(maccaLeft)
                {
                    insertItems.push_back(
                        characterManager->GenerateItem(SVR_CONST.ITEM_MACCA, maccaLeft));
                }
            }
            else
            {
                priceLeft = (uint16_t)(priceLeft - stack);
                deleteItems.push_back(m);
            }
        }
    }

    // Determine how many slots are free and how many are needed.
    // Natively shops will only sell a full stack at a time but this
    // logic handles more complex possibilities just in case.
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

    uint16_t qtyLeft = (uint16_t)quantity;
    uint16_t maxStack = def->GetPossession()->GetStackSize();

    // Update existing stacks first if we aren't adding a full stack
    std::map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems;
    if(qtyLeft < maxStack)
    {
        for(auto item : characterManager->GetExistingItems(character,
            product->GetItem(), inventory))
        {
            if(qtyLeft == 0) break;

            uint16_t stackLeft = (uint16_t)(maxStack - item->GetStackSize());
            if(stackLeft <= 0) continue;

            uint16_t stackAdd = (qtyLeft <= stackLeft) ? qtyLeft : stackLeft;
            stackAdjustItems[item] = (uint16_t)(item->GetStackSize() + stackAdd);
            qtyLeft = (uint16_t)(qtyLeft - stackAdd);
        }
    }

    // If there are still more to create, add as new items
    while(qtyLeft > 0)
    {
        uint16_t stack = (qtyLeft > maxStack) ? maxStack : qtyLeft;
        insertItems.push_back(characterManager->GenerateItem(
            product->GetItem(), stack));
        qtyLeft = (uint16_t)(qtyLeft - stack);
    }

    if(freeSlots.size() < insertItems.size())
    {
        SendShopPurchaseReply(client, shopID, productID, -1, false);
        return;
    }

    // Purchase is valid
    if(cpPurchase)
    {
        auto account = libcomp::PersistentObject::LoadObjectByUUID<objects::Account>(
            server->GetLobbyDatabase(), character->GetAccount().GetUUID(), true);
        auto lobbyDB = server->GetLobbyDatabase();

        auto opChangeset = std::make_shared<libcomp::DBOperationalChangeSet>();
        auto expl = std::make_shared<libcomp::DBExplicitUpdate>(account);
        expl->Subtract<int64_t>("CP", price);
        opChangeset->AddOperation(expl);

        if(!lobbyDB->ProcessChangeSet(opChangeset))
        {
            LOG_ERROR(libcomp::String("Attempted to buy an item exceeding the"
                " player's CP amount: %1\n").Arg(state->GetAccountUID().ToString()));
            SendShopPurchaseReply(client, shopID, productID, -2, false);
            return;
        }
    }

    auto changes = libcomp::DatabaseChangeSet::Create();
    std::list<uint16_t> updatedSlots;

    // Delete the items consumed by the payment
    for(auto item : deleteItems)
    {
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

    // Update the decreased payment item
    if(updateItem)
    {
        updateItem->SetStackSize(stackDecrease);
        changes->Update(updateItem);
        updatedSlots.push_back((uint16_t)updateItem->GetBoxSlot());
    }

    // Increase the stacks of existing items of the same type
    for(auto iPair : stackAdjustItems)
    {
        auto item = iPair.first;
        item->SetStackSize(iPair.second);
        changes->Update(item);
        updatedSlots.push_back((uint16_t)item->GetBoxSlot());
    }

    changes->Update(inventory);

    // Process all changes as a transaction
    auto worldDB = server->GetWorldDatabase();
    if(worldDB->ProcessChangeSet(changes))
    {
        SendShopPurchaseReply(client, shopID, productID, 0, true);

        updatedSlots.unique();
        updatedSlots.sort();
        characterManager->SendItemBoxData(client, inventory, updatedSlots);
    }
    else
    {
        if(cpPurchase)
        {
            // Roll back the CP cost
            auto account = character->GetAccount().Get();
            auto lobbyDB = server->GetLobbyDatabase();

            auto opChangeset = std::make_shared<libcomp::DBOperationalChangeSet>();
            auto expl = std::make_shared<libcomp::DBExplicitUpdate>(account);
            expl->Add<int64_t>("CP", price);
            opChangeset->AddOperation(expl);

            if(!lobbyDB->ProcessChangeSet(opChangeset))
            {
                // Hopefully this never happens
                LOG_CRITICAL("Account CP decrease could not be rolled back following"
                    " a failed NPC shop purchase!\n");
            }
        }
    }
}

bool Parsers::ShopBuy::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 22)
    {
        return false;
    }

    int32_t shopID = p.ReadS32Little();
    int32_t cacheID = p.ReadS32Little();
    int32_t productID = p.ReadS32Little();
    int32_t quantity = p.ReadS32Little();
    /// @todo: handle present purchases

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    if(quantity <= 0)
    {
        // Nothing to do
        SendShopPurchaseReply(client, shopID, productID, 0, false);
        return true;
    }

    server->QueueWork(HandleShopPurchase, server, client, shopID, cacheID,
        productID, quantity);

    return true;
}
