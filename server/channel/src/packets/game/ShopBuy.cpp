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
#include <ServerDataManager.h>

// object Includes
#include <Account.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiPossessionData.h>
#include <MiShopProductData.h>
#include <PostItem.h>
#include <ServerShop.h>
#include <ServerShopProduct.h>
#include <ServerShopTab.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"

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
    int32_t shopID, int32_t cacheID, int32_t productID, int32_t quantity,
    libcomp::String gifteeName, libcomp::String giftMessage)
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

    // Initially it was thought that the CP cost on MiShopProductData indicated
    // if the item had a CP cost or not but there are entries that sell for CP
    // in the UI with a cost of zero here so use the flag on the item instead
    bool cpPurchase = characterManager->IsCPItem(def);

    bool success = false;
    if(!cpPurchase)
    {
        // Non-CP purchases go to the inventory
        std::list<std::shared_ptr<objects::Item>> insertItems;
        std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems;

        price = price * quantity;

        if(!characterManager->CalculateMaccaPayment(client, (uint64_t)price,
            insertItems, stackAdjustItems))
        {
            LOG_ERROR(libcomp::String("Attempted to buy an item the player could"
                " not afford: %1\n").Arg(state->GetAccountUID().ToString()));
            SendShopPurchaseReply(client, shopID, productID, -2, false);
            return;
        }

        int32_t qtyLeft = (int32_t)quantity;
        int32_t maxStack = (int32_t)def->GetPossession()->GetStackSize();

        // Update existing stacks first if we aren't adding a full stack
        if(qtyLeft < maxStack)
        {
            for(auto item : characterManager->GetExistingItems(character,
                product->GetItem(), inventory))
            {
                if(qtyLeft == 0) break;

                uint16_t stackLeft = (uint16_t)(maxStack - item->GetStackSize());
                if(stackLeft <= 0) continue;

                uint16_t stackAdd = (uint16_t)((qtyLeft <= stackLeft) ? qtyLeft : stackLeft);

                if(stackAdjustItems.find(item) == stackAdjustItems.end())
                {
                    stackAdjustItems[item] = item->GetStackSize();
                }

                stackAdjustItems[item] = (uint16_t)(stackAdjustItems[item] + stackAdd);
                qtyLeft = (int32_t)(qtyLeft - stackAdd);
            }
        }

        // If there are still more to create, add as new items
        while(qtyLeft > 0)
        {
            uint16_t stack = (uint16_t)((qtyLeft > maxStack) ? maxStack : qtyLeft);
            insertItems.push_back(characterManager->GenerateItem(
                product->GetItem(), stack));
            qtyLeft = (int32_t)(qtyLeft - stack);
        }

        success = characterManager->UpdateItems(client, false, insertItems,
            stackAdjustItems);
    }
    else
    {
        // CP purchases always go to the post instead of the inventory
        auto lobbyDB = server->GetLobbyDatabase();
        auto targetCharacter = character;

        // Check for gift parameters
        if(!gifteeName.IsEmpty())
        {
            auto worldDB = server->GetWorldDatabase();
            targetCharacter = objects::Character::LoadCharacterByName(worldDB,
                gifteeName);

            if(targetCharacter == nullptr)
            {
                LOG_ERROR(libcomp::String("Invalid gift target character"
                    " name: '%1'\n").Arg(gifteeName));
                SendShopPurchaseReply(client, shopID, productID, -2, false);
                return;
            }
        }

        quantity = (int32_t)product->GetStack();

        auto postItems = objects::PostItem::LoadPostItemListByAccount(
            lobbyDB, targetCharacter->GetAccount());
        if(((int32_t)postItems.size() + 1) >= MAX_POST_ITEM_COUNT)
        {
            SendShopPurchaseReply(client, shopID, productID, -1, false);
            return;
        }

        auto account = libcomp::PersistentObject::LoadObjectByUUID<objects::Account>(
            server->GetLobbyDatabase(), character->GetAccount(), true);
        uint32_t cp = account->GetCP();
        if(cp < (uint32_t)price)
        {
            SendShopPurchaseReply(client, shopID, productID, -2, false);
            return;
        }

        auto opChangeset = std::make_shared<libcomp::DBOperationalChangeSet>();
        auto expl = std::make_shared<libcomp::DBExplicitUpdate>(account);
        expl->SubtractFrom<int64_t>("CP", price, (int32_t)cp);
        opChangeset->AddOperation(expl);

        uint32_t timestamp = (uint32_t)std::time(0);

        auto postItem = libcomp::PersistentObject::New<objects::PostItem>(true);
        postItem->SetType(product->GetID());
        postItem->SetTimestamp(timestamp);
        postItem->SetAccount(targetCharacter->GetAccount());

        if(!gifteeName.IsEmpty())
        {
            postItem->SetSource(objects::PostItem::Source_t::GIFT);
            postItem->SetFromName(character->GetName());
            postItem->SetGiftMessage(giftMessage);
        }

        opChangeset->Insert(postItem);

        if(!lobbyDB->ProcessChangeSet(opChangeset))
        {
            LOG_ERROR(libcomp::String("Attempted to buy an item exceeding the"
                " player's CP amount: %1\n").Arg(state->GetAccountUID().ToString()));
            SendShopPurchaseReply(client, shopID, productID, -2, false);
            return;
        }
        else
        {
            success = true;

            auto syncManager = server->GetChannelSyncManager();
            if(syncManager->UpdateRecord(account, "Account"))
            {
                syncManager->SyncOutgoing();
            }
        }
    }

    SendShopPurchaseReply(client, shopID, productID, success  ? 0 : -1, false);
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

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    if(p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little())))
    {
        return false;
    }

    libcomp::String gifteeName = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    if(p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little())))
    {
        return false;
    }

    libcomp::String giftMessage = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    if(quantity <= 0)
    {
        // Nothing to do
        SendShopPurchaseReply(client, shopID, productID, 0, false);
        return true;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    server->QueueWork(HandleShopPurchase, server, client, shopID, cacheID,
        productID, quantity, gifteeName, giftMessage);

    return true;
}
