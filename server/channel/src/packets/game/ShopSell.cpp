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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Account.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiPossessionData.h>
#include <ServerShop.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "Prefecture.h"

using namespace channel;

// Result values
// 0: Success
// -1: too many items
// anything else: generic error
static void SendShopSaleReply(
    const std::shared_ptr<ChannelClientConnection> client, int32_t shopID,
    int32_t result, bool queue) {
  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOP_SELL);
  reply.WriteS32Little(shopID);
  reply.WriteS32Little(result);

  if (queue) {
    client->QueuePacket(reply);
  } else {
    client->SendPacket(reply);
  }
}

static void HandleShopSale(
    const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client, int32_t shopID,
    std::list<std::pair<uint32_t, int64_t>> itemsSold) {
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto inventory = character->GetItemBoxes(0).Get();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  if (state->GetCurrentMenuShopID((int32_t)SVR_CONST.MENU_SHOP_SELL) !=
      shopID) {
    auto accountUID = state->GetAccountUID();
    LogGeneralError([shopID, accountUID]() {
      return libcomp::String(
                 "Player attempted a sale at shop %1 which they are not "
                 "currently interacting with: %2\n")
          .Arg(shopID)
          .Arg(accountUID.ToString());
    });

    SendShopSaleReply(client, shopID, -2, false);
    return;
  }

  auto shop = server->GetServerDataManager()->GetShopData((uint32_t)shopID);
  if (!shop) {
    // Invalid shop
    SendShopSaleReply(client, shopID, -2, false);
    return;
  }

  float lncAdjust = 1.f;
  if (shop->GetLNCAdjust()) {
    float lnc = (float)character->GetLNC();
    float lncCenter = shop->GetLNCCenter();
    float lncDelta = (float)fabs(lnc - lncCenter);

    lncAdjust = (lncCenter ? 1.2f : 1.1f) - lncDelta * 0.00002f;
  }

  uint64_t saleAmount = 0;
  std::list<std::shared_ptr<objects::Item>> deleteItems;
  std::map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems;
  for (auto pair : itemsSold) {
    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(
            state->GetObjectUUID(pair.second)));
    uint32_t amount = pair.first;
    if (!item || item->GetItemBox() != inventory->GetUUID() ||
        inventory->GetItems((size_t)item->GetBoxSlot()).Get() != item) {
      // Item/inventory does not match
      SendShopSaleReply(client, shopID, -2, false);
      return;
    }

    int32_t newAmount = (int32_t)item->GetStackSize() - (int32_t)amount;
    if (newAmount < 0) {
      // Too many items being sold requested
      SendShopSaleReply(client, shopID, -2, false);
      return;
    }

    auto def = definitionManager->GetItemData(item->GetType());
    if (!def || (def->GetBasic()->GetFlags() & ITEM_FLAG_SELL) == 0) {
      auto accountUID = state->GetAccountUID();
      LogItemError([item, accountUID]() {
        return libcomp::String(
                   "Player attempted to sell non-sellable item type %1: %2\n")
            .Arg(item->GetType())
            .Arg(accountUID.ToString());
      });

      SendShopSaleReply(client, shopID, -2, false);
      return;
    }

    int32_t sellPrice = def->GetBasic()->GetSellPrice();

    // Apply LNC adjust
    if (lncAdjust != 1.f) {
      sellPrice = (int32_t)floor((float)sellPrice * lncAdjust);
      if (sellPrice < 0) {
        sellPrice = 1;
      }
    }

    saleAmount = (uint64_t)(saleAmount + ((uint64_t)sellPrice * amount));
    if (newAmount == 0) {
      deleteItems.push_back(item);
    } else {
      stackAdjustItems[item] = (uint16_t)newAmount;
    }
  }

  std::list<int8_t> freeSlots;
  for (int8_t i = 0; i < 50; i++) {
    if (inventory->GetItems((size_t)i).IsNull()) {
      freeSlots.push_back(i);
    }
  }

  for (auto item : deleteItems) {
    freeSlots.push_back(item->GetBoxSlot());
  }

  freeSlots.unique();
  freeSlots.sort();

  int32_t macca = (int32_t)(saleAmount % (uint64_t)ITEM_MACCA_NOTE_AMOUNT);
  int32_t notes = (int32_t)floorl((double)(saleAmount - (uint64_t)macca) /
                                  (double)ITEM_MACCA_NOTE_AMOUNT);

  auto maxNoteStack = definitionManager->GetItemData(SVR_CONST.ITEM_MACCA_NOTE)
                          ->GetPossession()
                          ->GetStackSize();
  for (auto item : characterManager->GetExistingItems(
           character, SVR_CONST.ITEM_MACCA_NOTE, inventory)) {
    if (notes == 0) break;

    int32_t stackLeft = (int32_t)(maxNoteStack - item->GetStackSize());
    if (stackLeft <= 0) continue;

    int32_t stackAdd = (notes <= stackLeft) ? notes : stackLeft;
    stackAdjustItems[item] = (uint16_t)(item->GetStackSize() + stackAdd);
    notes = (int32_t)(notes - stackAdd);
  }

  for (auto item : characterManager->GetExistingItems(
           character, SVR_CONST.ITEM_MACCA, inventory)) {
    if (macca == 0) break;

    int32_t stackLeft =
        (int32_t)(ITEM_MACCA_NOTE_AMOUNT - item->GetStackSize());
    if (stackLeft <= 0) continue;

    int32_t stackAdd = (macca <= stackLeft) ? macca : stackLeft;
    stackAdjustItems[item] = (uint16_t)(item->GetStackSize() + stackAdd);
    macca = (int32_t)(macca - stackAdd);
  }

  // Add whatever amount is left as new items
  std::list<std::shared_ptr<objects::Item>> insertItems;
  while (notes > 0) {
    uint16_t stack = (notes > maxNoteStack) ? maxNoteStack : (uint16_t)notes;
    insertItems.push_back(
        characterManager->GenerateItem(SVR_CONST.ITEM_MACCA_NOTE, stack));
    notes = (int32_t)(notes - stack);
  }

  if (macca > 0) {
    insertItems.push_back(
        characterManager->GenerateItem(SVR_CONST.ITEM_MACCA, (uint16_t)macca));
  }

  if (freeSlots.size() < insertItems.size()) {
    SendShopSaleReply(client, shopID, -1, false);
    return;
  }

  auto changes = libcomp::DatabaseChangeSet::Create();
  std::list<uint16_t> updatedSlots;

  // Delete the full stacks of items sold
  for (auto item : deleteItems) {
    // Unequip if equipped
    characterManager->UnequipItem(client, item);

    auto slot = item->GetBoxSlot();
    inventory->SetItems((size_t)slot, NULLUUID);
    changes->Delete(item);
    updatedSlots.push_back((uint16_t)slot);
  }

  // Insert the new items
  for (auto item : insertItems) {
    auto slot = freeSlots.front();
    freeSlots.erase(freeSlots.begin());

    item->SetItemBox(inventory->GetUUID());
    item->SetBoxSlot(slot);
    inventory->SetItems((size_t)slot, item);
    changes->Insert(item);
    updatedSlots.push_back((uint16_t)slot);
  }

  // Update item stacks
  for (auto iPair : stackAdjustItems) {
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

bool Parsers::ShopSell::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() < 12) {
    return false;
  }

  int32_t shopID = p.ReadS32Little();
  int32_t cacheTime = p.ReadS32Little();
  int32_t itemCount = p.ReadS32Little();
  std::list<std::pair<uint32_t, int64_t>> itemsSold;

  (void)cacheTime;  // Not used

  if (itemCount < 0 || p.Left() != (uint32_t)(12 * itemCount)) {
    return false;
  }

  for (int32_t i = 0; i < itemCount; i++) {
    auto itemID = p.ReadS64Little();
    auto stackSize = p.ReadU32Little();

    itemsSold.push_back(std::pair<uint32_t, int64_t>(stackSize, itemID));
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();

  HandleShopSale(server, client, shopID, itemsSold);

  return true;
}
