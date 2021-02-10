/**
 * @file server/channel/src/packets/game/TradeFinish.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to finish the trade.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <Character.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ClientState.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::TradeFinish::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 0) {
    return false;
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto exchangeSession = state->GetExchangeSession();

  auto otherCState = exchangeSession
                         ? std::dynamic_pointer_cast<CharacterState>(
                               exchangeSession->GetOtherCharacterState())
                         : nullptr;
  auto otherClient = otherCState
                         ? server->GetManagerConnection()->GetEntityClient(
                               otherCState->GetEntityID(), false)
                         : nullptr;
  if (!otherClient) {
    characterManager->EndExchange(client);
    return true;
  }

  bool success = false;

  auto otherState = otherClient->GetClientState();
  auto otherSession = otherState->GetExchangeSession();
  if (otherSession) {
    success = true;

    exchangeSession->SetFinished(true);

    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_FINISHED);

    otherClient->SendPacket(notify);
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_FINISH);
  reply.WriteS32Little(success ? 0 : -1);

  client->SendPacket(reply);

  // Wait on the other player
  if (!otherSession->GetFinished()) {
    return true;
  }

  auto character = cState->GetEntity();
  auto inventory = character->GetItemBoxes(0).Get();

  auto otherCharacter = otherCState->GetEntity();
  auto otherInventory = otherCharacter->GetItemBoxes(0).Get();

  auto freeSlots = characterManager->GetFreeSlots(client, inventory);

  std::vector<std::shared_ptr<objects::Item>> tradeItems;
  for (auto tradeItem : exchangeSession->GetItems()) {
    if (!tradeItem.IsNull()) {
      if (tradeItem->GetItemBox() == inventory->GetUUID()) {
        // Attempting to trade away a phantom item. End the trade.
        characterManager->EndExchange(client, 1);
        characterManager->EndExchange(otherClient, 1);
        client->Kill();

        LogTradeWarning([&]() {
          return libcomp::String(
                     "Player attempted to trade away a phantom item: %1\n")
              .Arg(state->GetAccountUID().ToString());
        });

        return true;
      } else {
        tradeItems.push_back(tradeItem.Get());
        freeSlots.insert((size_t)tradeItem->GetBoxSlot());
      }
    }
  }

  auto otherFreeSlots =
      characterManager->GetFreeSlots(otherClient, otherInventory);

  std::vector<std::shared_ptr<objects::Item>> otherTradeItems;
  for (auto tradeItem : otherSession->GetItems()) {
    if (!tradeItem.IsNull()) {
      if (tradeItem->GetItemBox() == otherInventory->GetUUID()) {
        // Attempting to trade away a phantom item. End the trade.
        characterManager->EndExchange(client, 1);
        characterManager->EndExchange(otherClient, 1);
        otherClient->Kill();

        LogTradeWarning([&]() {
          return libcomp::String(
                     "Player attempted to trade away a phantom item: %1\n")
              .Arg(otherState->GetAccountUID().ToString());
        });

        return true;
      } else {
        otherTradeItems.push_back(tradeItem.Get());
        otherFreeSlots.insert((size_t)tradeItem->GetBoxSlot());
      }
    }
  }

  if (tradeItems.size() > otherFreeSlots.size()) {
    characterManager->EndExchange(client, 3);
    characterManager->EndExchange(otherClient, 2);
    return true;
  } else if (otherTradeItems.size() > freeSlots.size()) {
    characterManager->EndExchange(client, 2);
    characterManager->EndExchange(otherClient, 3);
    return true;
  }

  // Trade is valid so process it

  // Step 1: Unequip all equipment being traded
  std::list<uint16_t> updatedSlots;
  for (auto item : tradeItems) {
    characterManager->UnequipItem(client, item);

    updatedSlots.push_back((uint16_t)item->GetBoxSlot());
    inventory->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
  }

  std::list<uint16_t> otherUpdatedSlots;
  for (auto item : otherTradeItems) {
    characterManager->UnequipItem(otherClient, item);

    otherUpdatedSlots.push_back((uint16_t)item->GetBoxSlot());
    otherInventory->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
  }

  // Step 2: Transfer items and prepare changes
  auto changes = libcomp::DatabaseChangeSet::Create();

  changes->Update(inventory);

  auto itemIter = otherTradeItems.begin();
  for (size_t i = 0;
       i < inventory->ItemsCount() && itemIter != otherTradeItems.end(); i++) {
    if (freeSlots.find(i) != freeSlots.end()) {
      auto item = *itemIter;
      itemIter++;

      inventory->SetItems(i, item);
      item->SetBoxSlot((int8_t)i);
      item->SetItemBox(inventory->GetUUID());
      updatedSlots.push_back((uint16_t)i);
      changes->Update(item);
    }
  }

  changes->Update(otherInventory);

  itemIter = tradeItems.begin();
  for (size_t i = 0;
       i < otherInventory->ItemsCount() && itemIter != tradeItems.end(); i++) {
    if (otherFreeSlots.find(i) != otherFreeSlots.end()) {
      auto item = *itemIter;
      itemIter++;

      otherInventory->SetItems(i, item);
      item->SetBoxSlot((int8_t)i);
      item->SetItemBox(otherInventory->GetUUID());
      otherUpdatedSlots.push_back((uint16_t)i);
      changes->Update(item);
    }
  }

  // Step 3: Handle transaction
  auto db = server->GetWorldDatabase();
  if (!db->ProcessChangeSet(changes)) {
    LogTradeErrorMsg("Trade failed to save.\n");

    client->Close();
    otherClient->Close();
  } else {
    characterManager->SendItemBoxData(client, inventory, updatedSlots);
    characterManager->SendItemBoxData(otherClient, otherInventory,
                                      otherUpdatedSlots);

    characterManager->EndExchange(client, 0);
    characterManager->EndExchange(otherClient, 0);

    LogTradeDebug([cState, otherCState, tradeItems, otherTradeItems]() {
      std::list<libcomp::String> itemList1;
      std::list<libcomp::String> itemList2;

      for (auto item : tradeItems) {
        if (item->GetStackSize() > 1) {
          itemList1.push_back(libcomp::String("%1 x%2")
                                  .Arg(item->GetType())
                                  .Arg(item->GetStackSize()));
        } else {
          itemList1.push_back(libcomp::String("%1").Arg(item->GetType()));
        }
      }

      for (auto item : otherTradeItems) {
        if (item->GetStackSize() > 1) {
          itemList2.push_back(libcomp::String("%1 x%2")
                                  .Arg(item->GetType())
                                  .Arg(item->GetStackSize()));
        } else {
          itemList2.push_back(libcomp::String("%1").Arg(item->GetType()));
        }
      }

      return libcomp::String(
                 "Successfully traded %1 from character %2 for %3 from "
                 "character %4.\n")
          .Arg(itemList1.size() > 0 ? libcomp::String::Join(itemList1, ", ")
                                    : "no items")
          .Arg(cState->GetEntityUUID().ToString())
          .Arg(itemList2.size() > 0 ? libcomp::String::Join(itemList2, ", ")
                                    : "no items")
          .Arg(otherCState->GetEntityUUID().ToString());
    });
  }

  return true;
}
