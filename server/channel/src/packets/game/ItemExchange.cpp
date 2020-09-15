/**
 * @file server/channel/src/packets/game/ItemExchange.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to exchange an item for something else.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// object Includes
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <Item.h>
#include <MiCategoryData.h>
#include <MiExchangeData.h>
#include <MiExchangeObjectData.h>
#include <MiExchangeOptionData.h>
#include <MiItemData.h>
#include <MiSkillItemStatusCommonData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::ItemExchange::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 9) {
    return false;
  }

  int64_t itemID = p.ReadS64Little();
  int8_t optionID = p.ReadS8();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  auto item = std::dynamic_pointer_cast<objects::Item>(
      libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

  int32_t responseCode = -3;  // Generic error, no message

  auto itemDef =
      item ? definitionManager->GetItemData(item->GetType()) : nullptr;
  auto exchangeDef =
      item ? definitionManager->GetExchangeData(item->GetType()) : nullptr;
  auto optionDef =
      exchangeDef ? exchangeDef->GetOptions((size_t)optionID) : nullptr;

  if (!itemDef || !optionDef) {
    LogItemError([item, optionID]() {
      return libcomp::String(
                 "Invalid exchange ID encountered for ItemExchange request: "
                 "%1, %2\n")
          .Arg(item ? item->GetType() : 0)
          .Arg(optionID);
    });
  } else if (!state->GetCharacterState()->IsAlive()) {
    responseCode = -2;  // Cannot be used here
  } else if (itemDef->GetCommon()->GetCategory()->GetSubCategory() ==
             ITEM_SUBCATEGORY_EXCHANGE_ITEM) {
    std::list<std::shared_ptr<objects::Item>> inserts;
    for (auto obj : optionDef->GetItems()) {
      uint32_t itemType = obj->GetID();
      uint16_t stackSize = obj->GetStackSize();
      if (itemType) {
        auto newItem = characterManager->GenerateItem(itemType, stackSize);
        if (newItem) {
          inserts.push_back(newItem);
        }
      }
    }

    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> updates;
    updates[item] = (uint16_t)(item->GetStackSize() - 1);

    if (characterManager->UpdateItems(client, false, inserts, updates)) {
      responseCode = 0;  // Success

      auto accountUID = state->GetAccountUID();
      LogItemDebug([item, optionID, accountUID]() {
        return libcomp::String(
                   "Player exchanged item type %1 for item option %2: %3\n")
            .Arg(item->GetType())
            .Arg(optionID)
            .Arg(accountUID.ToString());
      });
    } else {
      responseCode = -1;  // Not enough space
    }
  } else if (itemDef->GetCommon()->GetCategory()->GetSubCategory() ==
             ITEM_SUBCATEGORY_EXCHANGE_DEMON) {
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();
    auto comp = character->GetCOMP().Get();

    uint8_t maxSlots = progress->GetMaxCOMPSlots();

    size_t freeCount = 0;
    for (uint8_t i = 0; i < maxSlots; i++) {
      auto slot = comp->GetDemons((size_t)i);
      if (slot.IsNull()) {
        freeCount++;
      }
    }

    std::list<uint32_t> demonTypes;
    for (auto obj : optionDef->GetItems()) {
      uint32_t demonType = obj->GetID();
      uint16_t count = obj->GetStackSize();
      if (demonType) {
        for (uint16_t i = 0; i < count; i++) {
          demonTypes.push_back(demonType);
        }
      }
    }

    if (demonTypes.size() <= freeCount) {
      std::list<std::shared_ptr<objects::Item>> empty;

      std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> updates;
      updates[item] = (uint16_t)(item->GetStackSize() - 1);

      if (characterManager->UpdateItems(client, false, empty, updates)) {
        responseCode = 0;  // Success

        for (uint32_t demonType : demonTypes) {
          auto demonData = definitionManager->GetDevilData(demonType);
          characterManager->ContractDemon(client, demonData,
                                          cState->GetEntityID(), 3000);
        }

        auto accountUID = state->GetAccountUID();
        LogItemDebug([item, optionID, accountUID]() {
          return libcomp::String(
                     "Player exchanged item type %1 for demon option %2: %3\n")
              .Arg(item->GetType())
              .Arg(optionID)
              .Arg(accountUID.ToString());
        });
      }
    } else {
      LogItemError([demonTypes, freeCount]() {
        return libcomp::String(
                   "Attempted to add '%1' demon(s) from ItemExchange request "
                   "but only had room for %2\n")
            .Arg(demonTypes.size())
            .Arg(freeCount);
      });
    }
  } else {
    LogItemError([itemDef]() {
      return libcomp::String(
                 "Invalid source item sub-category encountered for "
                 "ItemExchange request: %1, %2\n")
          .Arg(itemDef->GetCommon()->GetCategory()->GetSubCategory());
    });
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_EXCHANGE);
  reply.WriteS64Little(itemID);
  reply.WriteS8(optionID);
  reply.WriteS32Little(responseCode);

  client->SendPacket(reply);

  return true;
}
