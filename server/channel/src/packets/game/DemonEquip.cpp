/**
 * @file server/channel/src/packets/game/DemonEquip.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to equip or unequip an item on the
 *  currently summoned demon that can replace a trait skill.
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

// object Includes
#include <Item.h>
#include <ItemBox.h>
#include <MiDevilData.h>
#include <MiDevilEquipmentData.h>
#include <MiDevilEquipmentItemData.h>
#include <MiGrowthData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"

using namespace channel;

bool Parsers::DemonEquip::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 27) {
    return false;
  }

  int64_t demonID = p.ReadS64Little();
  uint8_t actionType = p.ReadU8();
  uint8_t sourceSlot = p.ReadU8();
  int64_t sourceItemID = p.ReadS64Little();
  uint8_t targetSlot = p.ReadU8();
  int64_t targetItemID = p.ReadS64Little();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();

  auto character = cState->GetEntity();
  auto inventory = character ? character->GetItemBoxes(0).Get() : nullptr;
  if (!inventory) {
    return true;
  }

  auto demon = demonID ? std::dynamic_pointer_cast<objects::Demon>(
                             libcomp::PersistentObject::GetObjectByUUID(
                                 state->GetObjectUUID(demonID)))
                       : nullptr;
  if (!demon || dState->GetEntity() != demon) {
    LogDemonError([&]() {
      return libcomp::String("Invalid demon requested for DemonEquip: %1.\n")
          .Arg(state->GetAccountUID().ToString());
    });

    return true;
  }

  auto equip = sourceItemID ? std::dynamic_pointer_cast<objects::Item>(
                                  libcomp::PersistentObject::GetObjectByUUID(
                                      state->GetObjectUUID(sourceItemID)))
                            : nullptr;
  auto unequip = targetItemID ? std::dynamic_pointer_cast<objects::Item>(
                                    libcomp::PersistentObject::GetObjectByUUID(
                                        state->GetObjectUUID(targetItemID)))
                              : nullptr;

  int8_t demonSlot = -1;
  int8_t inventorySlot = -1;

  bool removeAll = false;
  switch (actionType) {
    case 0:  // Equip from inventory
    case 2:  // Replace (drag to demon)
      demonSlot = (int8_t)targetSlot;
      inventorySlot = (int8_t)sourceSlot;
      break;
    case 1:  // Unequip to inventory
    case 3:  // Replace (drag from demon)
    {
      // "Equip" item is actually being unequipped so swap everything
      auto swap = equip;
      equip = unequip;
      unequip = swap;

      demonSlot = (int8_t)sourceSlot;
      inventorySlot = (int8_t)targetSlot;
    } break;
    case 4:
      removeAll = true;
      break;
    default:
      LogDemonError([&]() {
        return libcomp::String(
                   "Unknown DemonEquip action type encountered: %1\n")
            .Arg(actionType);
      });

      return false;
  }

  bool success = true;

  if (!removeAll && !equip && !unequip) {
    LogDemonError([&]() {
      return libcomp::String(
                 "DemonEquip equip action attempted with no valid item "
                 "supplied: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    success = false;
  }

  std::set<uint16_t> exclusionGroups;
  if (success && equip) {
    if (equip->GetBoxSlot() != inventorySlot ||
        equip->GetItemBox() != inventory->GetUUID()) {
      LogDemonError([&]() {
        return libcomp::String(
                   "DemonEquip equip action attempted with incorrect inventory "
                   "item slot supplied: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      success = false;
    }

    if (demonSlot >= 4) {
      LogDemonError([&]() {
        return libcomp::String(
                   "DemonEquip equip action attempted with invalid target "
                   "slot: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      success = false;
    }

    // Verify that the item is valid for demon equipment
    auto demonEquipData =
        definitionManager->GetDevilEquipmentItemData(equip->GetType());
    if (demonEquipData) {
      auto equipData = definitionManager->GetDevilEquipmentData(
          demonEquipData->GetSkillID());
      for (auto exGroup : equipData->GetExclusionGroup()) {
        if (exGroup) {
          exclusionGroups.insert(exGroup);
        }
      }
    } else {
      LogDemonError([&]() {
        return libcomp::String(
                   "DemonEquip equip action attempted with item that is not "
                   "demon equipment: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      success = false;
    }

    // Verify restrictions on items that will remain
    auto growth = dState->GetDevilData()->GetGrowth();

    for (size_t i = 0; i < 4; i++) {
      uint32_t skillID = growth->GetTraits(i);
      if ((int8_t)i == demonSlot) {
        // Make sure we're not covering a fixed trait
        auto equipData = definitionManager->GetDevilEquipmentData(skillID);
        if (equipData && equipData->GetFixed()) {
          LogDemonError([&]() {
            return libcomp::String(
                       "DemonEquip attempted on fixed demon trait: %1\n")
                .Arg(state->GetAccountUID().ToString());
          });

          success = false;
          break;
        }

        // Skip the slot being equipped to otherwise
        continue;
      }

      auto item = demon->GetEquippedItems(i).Get();
      if (item) {
        // Item currently equipped
        if (item != unequip) {
          auto equipData =
              definitionManager->GetDevilEquipmentItemData(item->GetType());
          skillID = equipData ? equipData->GetSkillID() : 0;
        } else {
          skillID = 0;
        }
      }

      if (skillID) {
        auto equipData = definitionManager->GetDevilEquipmentData(skillID);
        if (equipData) {
          for (auto exGroup : equipData->GetExclusionGroup()) {
            if (exGroup &&
                exclusionGroups.find(exGroup) != exclusionGroups.end()) {
              LogDemonError([&]() {
                return libcomp::String(
                           "DemonEquip exclusion group restriction failed: "
                           "%1\n")
                    .Arg(state->GetAccountUID().ToString());
              });

              success = false;
              break;
            }
          }
        }
      }
    }
  }

  if (success && unequip && inventorySlot >= 50) {
    LogDemonError([&]() {
      return libcomp::String(
                 "DemonEquip unequip action attempted with invalid target "
                 "slot: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    success = false;
  }

  std::set<size_t> removeAllSlots;
  if (success && removeAll) {
    removeAllSlots = characterManager->GetFreeSlots(client, inventory);

    uint8_t equipCount = 0;
    for (size_t i = 0; i < 4; i++) {
      auto item = demon->GetEquippedItems(i).Get();
      if (item) {
        equipCount++;
      }
    }

    if ((size_t)equipCount > removeAllSlots.size()) {
      LogDemonError([&]() {
        return libcomp::String(
                   "DemonEquip unequip all action attempted with insufficient "
                   "inventory space available: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      success = false;
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_EQUIP);
  reply.WriteS64Little(demonID);

  if (success) {
    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());

    std::list<uint16_t> updatedSlots;
    if (inventorySlot >= 0) {
      updatedSlots.push_back((uint16_t)inventorySlot);
    }

    if (removeAll) {
      for (size_t i = 0; i < 4; i++) {
        auto item = demon->GetEquippedItems(i).Get();
        if (item) {
          size_t slot = *removeAllSlots.begin();
          removeAllSlots.erase(slot);

          demon->SetEquippedItems(i, NULLUUID);

          inventory->SetItems((size_t)slot, item);
          item->SetBoxSlot((int8_t)slot);
          item->SetItemBox(inventory->GetUUID());
          dbChanges->Update(item);

          updatedSlots.push_back((uint16_t)slot);
        }
      }
    } else {
      demon->SetEquippedItems((size_t)demonSlot, equip);
      inventory->SetItems((size_t)inventorySlot, unequip);

      if (equip) {
        equip->SetBoxSlot(-1);
        equip->SetItemBox(NULLUUID);
        dbChanges->Update(equip);
      }

      if (unequip) {
        unequip->SetBoxSlot(inventorySlot);
        unequip->SetItemBox(inventory->GetUUID());
        dbChanges->Update(unequip);
      }
    }

    dbChanges->Update(demon);
    dbChanges->Update(inventory);

    reply.WriteU8(0);  // Success

    for (size_t i = 0; i < 4; i++) {
      auto item = demon->GetEquippedItems(i).Get();
      if (item) {
        reply.WriteS64Little(state->GetObjectID(item->GetUUID()));
        reply.WriteU32Little(item->GetType());
      } else {
        reply.WriteS64Little(-1);
        reply.WriteU32Little(static_cast<uint32_t>(-1));
      }
    }

    reply.WriteU8(0);  // Always 0

    client->QueuePacket(reply);

    // Send inventory slots
    if (updatedSlots.size() > 0) {
      characterManager->SendItemBoxData(client, inventory, updatedSlots);
    }

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);

    // Always recalc
    server->GetTokuseiManager()->Recalculate(
        cState, true, std::set<int32_t>{dState->GetEntityID()});
    characterManager->RecalculateStats(dState, client);
  } else {
    reply.WriteU8(1);  // Failure
    reply.WriteBlank(49);

    client->SendPacket(reply);
  }

  return true;
}
