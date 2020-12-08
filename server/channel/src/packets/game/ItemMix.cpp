/**
 * @file server/channel/src/packets/game/ItemMix.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to mix two items into a different result.
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
#include <Randomizer.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Item.h>
#include <ItemBox.h>
#include <MiBlendData.h>
#include <MiBlendData_Item.h>
#include <MiBlendExtData.h>
#include <MiBlendExtData_DstItemChange.h>
#include <MiBlendExtData_SrcItemChange.h>
#include <MiItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::ItemMix::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 36) {
    return false;
  }

  uint32_t blendID = p.ReadU32Little();
  int64_t itemID1 = p.ReadS64Little();
  int64_t itemID2 = p.ReadS64Little();
  int64_t itemIDExt1 = p.ReadS64Little();
  int64_t itemIDExt2 = p.ReadS64Little();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto inventory = character->GetItemBoxes(0).Get();

  auto item1 = itemID1 > 0 ? std::dynamic_pointer_cast<objects::Item>(
                                 libcomp::PersistentObject::GetObjectByUUID(
                                     state->GetObjectUUID(itemID1)))
                           : nullptr;
  auto item2 = itemID2 > 0 ? std::dynamic_pointer_cast<objects::Item>(
                                 libcomp::PersistentObject::GetObjectByUUID(
                                     state->GetObjectUUID(itemID2)))
                           : nullptr;

  auto blendData = definitionManager->GetBlendData(blendID);

  bool success = item1 && item2 && blendData;

  std::list<std::shared_ptr<objects::Item>> extItems;
  std::list<std::shared_ptr<objects::MiBlendExtData>> extItemDefs;
  for (int64_t extItemID : {itemIDExt1, itemIDExt2}) {
    if (extItemID > 0) {
      auto itemExt = std::dynamic_pointer_cast<objects::Item>(
          libcomp::PersistentObject::GetObjectByUUID(
              state->GetObjectUUID(extItemID)));
      auto extData =
          itemExt ? definitionManager->GetBlendExtData(itemExt->GetType())
                  : nullptr;

      bool failure = !extData;
      if (!failure) {
        if ((extData->GetGroupID() &&
             extData->GetGroupID() != blendData->GetExtensionGroupID()) ||
            itemExt->GetItemBox() != inventory->GetUUID() ||
            itemExt->GetStackSize() == 0) {
          failure = true;
        } else {
          extItems.push_back(itemExt);
          extItemDefs.push_back(extData);
        }
      }

      if (failure) {
        LogItemError([&]() {
          return libcomp::String(
                     "ItemMix attempted with invalid extension item: %1\n")
              .Arg(state->GetAccountUID().ToString());
        });

        success = false;
      }
    }
  }

  // Check expertise requirements
  int32_t expertID = blendData ? blendData->GetExpertiseID() : 0;
  int32_t requiredClass = blendData ? blendData->GetRequiredClass() : 0;
  int32_t requiredRank = blendData ? blendData->GetRequiredRank() : 0;
  if (extItemDefs.size() > 0) {
    float classAdjust = 1.f;
    float rankAdjust = 1.f;
    for (auto blendExtData : extItemDefs) {
      if (blendExtData->GetExpertiseID() != -1) {
        expertID = blendExtData->GetExpertiseID();
      }

      classAdjust -= (1.f - blendExtData->GetRequiredClass());
      rankAdjust -= (1.f - blendExtData->GetRequiredRank());
    }

    requiredClass = (int32_t)ceil((float)requiredClass * classAdjust);
    requiredRank = (int32_t)ceil((float)requiredRank * rankAdjust);
  }

  uint8_t expertRank =
      expertID ? cState->GetExpertiseRank((uint32_t)expertID, definitionManager)
               : 0;
  if (success && expertID && (requiredClass * 10 + requiredRank) > expertRank) {
    LogItemError([&]() {
      return libcomp::String(
                 "ItemMix attempted without required expertise rank: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    success = false;
  }

  // Check additional validations
  if (success && (item1->GetItemBox() != inventory->GetUUID() ||
                  item2->GetItemBox() != inventory->GetUUID())) {
    LogItemError([&]() {
      return libcomp::String(
                 "ItemMix attempted with an item not in the inventory: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    success = false;
  }

  // Stage the updates if still valid
  std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> updateItems;
  std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> originalStacks;
  if (success) {
    auto itemA = item1;
    auto itemB = item2;

    auto inputItem1 = blendData->GetInputItems(0);
    auto inputItem2 = blendData->GetInputItems(1);

    uint32_t item1Type = inputItem1->GetItemID();
    uint32_t item2Type = inputItem2->GetItemID();

    uint16_t item1Min = inputItem1->GetMin();
    uint16_t item2Min = inputItem2->GetMin();

    // Apply extension transformations
    if (extItemDefs.size() > 0) {
      float min1Scale = 1.f;
      float min2Scale = 1.f;
      for (auto blendExtData : extItemDefs) {
        auto mod1 = blendExtData->GetSrcItems(0);
        auto mod2 = blendExtData->GetSrcItems(1);

        item1Type = mod1->GetItemID() != static_cast<uint32_t>(-1)
                        ? mod1->GetItemID()
                        : item1Type;
        item2Type = mod2->GetItemID() != static_cast<uint32_t>(-1)
                        ? mod2->GetItemID()
                        : item2Type;

        min1Scale -= (1.f - mod1->GetMinScale());
        min2Scale -= (1.f - mod2->GetMinScale());
      }

      if (min1Scale < 0.f) min1Scale = 0.f;
      if (min2Scale < 0.f) min2Scale = 0.f;

      item1Min = (uint16_t)floor((float)item1Min * min1Scale);
      item2Min = (uint16_t)floor((float)item2Min * min2Scale);

      // Apply minimum of 1
      if (item1Min == 0) {
        item1Min = 1;
      }

      if (item2Min == 0) {
        item2Min = 1;
      }
    }

    if (item2->GetType() == item1Type && item1->GetType() == item2Type) {
      // Reverse order
      itemA = item2;
      itemB = item1;
    } else if (item1->GetType() != item1Type || item2->GetType() != item2Type) {
      LogItemError([&]() {
        return libcomp::String(
                   "ItemMix supplied item types do not match definition: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      success = false;
    }

    if (success) {
      if (itemA->GetStackSize() < item1Min ||
          itemB->GetStackSize() < item2Min) {
        LogItemError([&]() {
          return libcomp::String(
                     "ItemMix supplied without enough items of each type "
                     "required: %1\n")
              .Arg(state->GetAccountUID().ToString());
        });

        success = false;
      } else {
        originalStacks[itemA] = itemA->GetStackSize();
        originalStacks[itemB] = itemB->GetStackSize();

        updateItems[itemA] = (uint16_t)(itemA->GetStackSize() - item1Min);
        updateItems[itemB] = (uint16_t)(itemB->GetStackSize() - item2Min);

        for (auto extItem : extItems) {
          updateItems[extItem] = (uint16_t)(extItem->GetStackSize() - 1);
        }
      }
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_MIX);

  if (success) {
    // Mix valid, determine outcome
    std::shared_ptr<objects::Item> newItem;
    std::list<std::shared_ptr<objects::Item>> insertItems;

    uint32_t successRate = blendData->GetProbabilities(0);
    uint32_t gSuccessRate = blendData->GetProbabilities(1);

    int32_t expSuccessBoost = blendData->GetExpertSuccessBoost();
    int32_t expGSuccessBoost = blendData->GetExpertGreatSuccessBoost();

    int32_t expSuccessUp = blendData->GetSuccessExpertUp();
    int32_t expGSuccessUp = blendData->GetGreatSuccessExpertUp();
    int32_t expFailUp = blendData->GetFailExpertUp();

    float lossRate = blendData->GetMaterialLossRate();

    auto outItem1 = blendData->GetResultItems(0);
    auto outItem2 = blendData->GetResultItems(1);

    uint32_t item1Type = outItem1->GetItemID();
    uint32_t item2Type = outItem2->GetItemID() != static_cast<uint32_t>(-1)
                             ? outItem2->GetItemID()
                             : 0;

    uint16_t item1Min = outItem1->GetMin();
    uint16_t item2Min = item2Type ? outItem2->GetMin() : 0;

    uint16_t item1Max = outItem1->GetMax();
    uint16_t item2Max = item2Type ? outItem2->GetMax() : 0;

    // Apply extension transformations
    if (extItemDefs.size() > 0) {
      float successRateScale = 1.f;
      float gSuccessRateScale = 1.f;
      float expSuccessBoostScale = 1.f;
      float expGSuccessBoostScale = 1.f;
      float expSuccessUpScale = 1.f;
      float expGSuccessUpScale = 1.f;
      float expFailUpScale = 1.f;
      float lossRateScale = 1.f;
      float item1MinScale = 1.f;
      float item2MinScale = 1.f;
      float item1MaxScale = 1.f;
      float item2MaxScale = 1.f;
      for (auto blendExtData : extItemDefs) {
        successRateScale *= blendExtData->GetProbabilities(0);
        gSuccessRateScale *= blendExtData->GetProbabilities(1);

        expSuccessBoostScale *= blendExtData->GetExpertSuccessBoost();
        expGSuccessBoostScale *= blendExtData->GetExpertGreatSuccessBoost();

        expSuccessUpScale *= blendExtData->GetSuccessExpertUp();
        expGSuccessUpScale *= blendExtData->GetGreatSuccessExpertUp();
        expFailUpScale *= blendExtData->GetFailExpertUp();

        lossRateScale *= blendExtData->GetMaterialLoss();

        auto mod1 = blendExtData->GetDstItems(0);
        auto mod2 = blendExtData->GetDstItems(1);

        item1Type = mod1->GetItemID() != static_cast<uint32_t>(-1)
                        ? mod1->GetItemID()
                        : item1Type;
        item2Type = mod2->GetItemID() != static_cast<uint32_t>(-1)
                        ? mod2->GetItemID()
                        : item2Type;

        item1MinScale *= mod1->GetMinScale();
        item2MinScale *= mod2->GetMinScale();

        item1MaxScale *= mod1->GetMaxScale();
        item2MaxScale *= mod2->GetMaxScale();
      }

      if (successRateScale < 0.f) successRateScale = 0.f;
      if (gSuccessRateScale < 0.f) gSuccessRateScale = 0.f;
      if (expSuccessBoostScale < 0.f) expSuccessBoostScale = 0.f;
      if (expGSuccessBoostScale < 0.f) expGSuccessBoostScale = 0.f;
      if (expGSuccessUpScale < 0.f) expGSuccessUpScale = 0.f;
      if (expFailUpScale < 0.f) expFailUpScale = 0.f;
      if (lossRateScale < 0.f) lossRateScale = 0.f;
      if (item1MinScale < 0.f) item1MinScale = 0.f;
      if (item2MinScale < 0.f) item2MinScale = 0.f;
      if (item1MaxScale < 0.f) item1MaxScale = 0.f;
      if (item2MaxScale < 0.f) item2MaxScale = 0.f;

      successRate = (uint32_t)ceil((float)successRate * successRateScale);
      gSuccessRate = (uint32_t)ceil((float)gSuccessRate * gSuccessRateScale);

      expSuccessBoost =
          (int32_t)ceil((float)expSuccessBoost * expSuccessBoostScale);
      expGSuccessBoost =
          (int32_t)ceil((float)expGSuccessBoost * expGSuccessBoostScale);

      expSuccessUp = (int32_t)ceil((float)expSuccessUp * expSuccessUpScale);
      expGSuccessUp = (int32_t)ceil((float)expGSuccessUp * expGSuccessUpScale);
      expFailUp = (int32_t)ceil((float)expFailUp * expFailUpScale);

      lossRate = lossRate * lossRateScale;

      item1Min = (uint16_t)floor((float)item1Min * item1MinScale);
      item2Min = (uint16_t)floor((float)item2Min * item2MinScale);

      item1Max = (uint16_t)floor((float)item1Max * item1MaxScale);
      item2Max = (uint16_t)floor((float)item2Max * item2MaxScale);
    }

    // Apply expertise boosts
    successRate = successRate + (uint32_t)(expSuccessBoost * expertRank);

    // You cannot get a great success without a great success item
    gSuccessRate =
        item2Type ? (gSuccessRate + (uint32_t)(expGSuccessBoost * expertRank))
                  : 0;

    int32_t outcome = 0;  // Failure
    uint16_t itemCount = 0;
    int32_t expertUp = expFailUp;
    if (successRate >= 10000 || RNG(uint32_t, 1, 10000) < successRate) {
      uint32_t itemType = item1Type;

      outcome = 1;  // Success
      expertUp = expSuccessUp;

      if (gSuccessRate >= 10000 || RNG(uint32_t, 1, 10000) < gSuccessRate) {
        outcome = 2;  // Great success
        expertUp = expGSuccessUp;

        itemType = item2Type;
        itemCount = RNG(uint16_t, item2Min, item2Max);
      } else {
        itemCount = RNG(uint16_t, item1Min, item1Max);
      }

      auto itemData = definitionManager->GetItemData(itemType);
      if (!itemData) {
        LogItemError([&]() {
          return libcomp::String(
                     "ItemMix resulted in an invalid item with item type '%1' "
                     "from recipe '%2': %3\n")
              .Arg(itemType)
              .Arg(blendID)
              .Arg(state->GetAccountUID().ToString());
        });

        client->Close();

        return true;
      }

      // Add to an existing stack if possible
      for (auto existing :
           characterManager->GetExistingItems(character, itemType, inventory)) {
        uint16_t stackSize = existing->GetStackSize();

        // Make sure we don't cancel an existing change
        auto it = updateItems.find(existing);
        if (it != updateItems.end()) {
          stackSize = it->second;
        }

        if (stackSize == 0) {
          // Do not reuse an item if its being removed (covers
          // anything equippable or with an expiration too)
          continue;
        }

        if ((uint16_t)(stackSize + itemCount) <=
            itemData->GetPossession()->GetStackSize()) {
          newItem = existing;
          updateItems[newItem] = (uint16_t)(stackSize + itemCount);
          break;
        }
      }

      // If no stack can hold all the new items, create new
      if (!newItem) {
        newItem = characterManager->GenerateItem(itemType, itemCount);
        insertItems.push_back(newItem);
      }
    } else {
      // Adjust stack size consumed for failure rate
      for (auto pair : originalStacks) {
        uint16_t lossCount = (uint16_t)floor(
            (float)(pair.second - updateItems[pair.first]) * lossRate);
        updateItems[pair.first] = (uint16_t)(pair.second - lossCount);
      }
    }

    std::list<uint16_t> updatedSlots;
    for (auto& pair : updateItems) {
      updatedSlots.push_back((uint16_t)pair.first->GetBoxSlot());
    }

    if (characterManager->UpdateItems(client, false, insertItems, updateItems,
                                      false)) {
      if (insertItems.size() > 0) {
        updatedSlots.push_back((uint16_t)newItem->GetBoxSlot());
      }
    }

    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_MIXED);
    notify.WriteS32Little(cState->GetEntityID());
    notify.WriteS32Little(outcome);

    server->GetZoneManager()->BroadcastPacket(client, notify);

    reply.WriteS32Little(0);  // Success
    reply.WriteU32Little(blendID);
    reply.WriteS64Little(itemID1);
    reply.WriteS64Little(itemID2);
    reply.WriteS64Little(itemIDExt1);
    reply.WriteS64Little(itemIDExt2);
    reply.WriteU32Little(newItem ? newItem->GetType() : 0);
    reply.WriteU16Little(itemCount);
    reply.WriteS8(newItem ? newItem->GetBoxSlot() : -1);

    client->QueuePacket(reply);

    characterManager->SendItemBoxData(client, inventory, updatedSlots);

    // If expertise should be increased, apply that now
    if (expertID && expertUp) {
      std::list<std::pair<uint8_t, int32_t>> expPoints;
      expertUp = (int32_t)(
          (double)(expertUp * 50) / (double)((expertRank / 10) + 1) /
          (double)((expertRank % 10) + 1) *
          (double)cState->GetCorrectValue(CorrectTbl::RATE_EXPERTISE) * 0.01);

      expPoints.push_back(std::make_pair(expertID, expertUp));

      characterManager->UpdateExpertisePoints(client, expPoints);
    }

    client->FlushOutgoing();
  } else {
    reply.WriteS32Little(-1);  // Error
    reply.WriteU32Little(blendID);
    reply.WriteS64Little(itemID1);
    reply.WriteS64Little(itemID2);
    reply.WriteS64Little(itemIDExt1);
    reply.WriteS64Little(itemIDExt2);
    reply.WriteU32Little(0);  // Result item
    reply.WriteU16Little(0);  // Stack
    reply.WriteS8(-1);        // Target slot

    client->SendPacket(reply);
  }

  return true;
}
