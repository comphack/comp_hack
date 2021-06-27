/**
 * @file server/channel/src/packets/game/Barter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to barter for items or other materials.
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
#include <Demon.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiAcquisitionData.h>
#include <MiDevilData.h>
#include <MiGrowthData.h>
#include <MiItemData.h>
#include <MiNPCBarterConditionData.h>
#include <MiNPCBarterConditionDataEntry.h>
#include <MiNPCBarterData.h>
#include <MiNPCBarterGroupData.h>
#include <MiNPCBarterGroupEntry.h>
#include <MiNPCBarterItemData.h>
#include <MiNPCBasicData.h>
#include <MiPossessionData.h>
#include <MiSkillData.h>
#include <MiUnionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "Prefecture.h"

using namespace channel;

static bool BarterConditionValid(
    const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<CharacterState>& cState,
    const std::shared_ptr<DemonState>& dState,
    const std::shared_ptr<objects::MiNPCBarterData>& barterData,
    const std::shared_ptr<objects::MiNPCBarterConditionDataEntry>& condition) {
  switch (condition->GetType()) {
    case objects::MiNPCBarterConditionDataEntry::Type_t::CHARACTER_LEVEL: {
      // Character level between range
      auto lvl = (int32_t)cState->GetLevel();
      return (!condition->GetValue1() || lvl >= condition->GetValue1()) &&
             (!condition->GetValue2() || lvl <= condition->GetValue2());
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::DEMON_LEVEL: {
      // Demon level between range
      auto lvl = (int32_t)dState->GetLevel();
      return (!condition->GetValue1() || lvl >= condition->GetValue1()) &&
             (!condition->GetValue2() || lvl <= condition->GetValue2());
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::CHARACTER_SKILL: {
      // Character has skill
      auto character = cState->GetEntity();
      return character &&
             character->LearnedSkillsContains((uint32_t)condition->GetValue1());
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::DEMON_SKILL: {
      // Demon has skill (learned)
      auto demon = dState->GetEntity();
      if (demon) {
        for (uint32_t skillID : demon->GetAcquiredSkills()) {
          if (skillID == (uint32_t)condition->GetValue1()) {
            return true;
          }
        }
      }

      return false;
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::
        DEMON_SKILL_INHERITANCE: {
      // Demon has inheritence type for skill
      auto devilData = dState->GetDevilData();
      auto skillData = server->GetDefinitionManager()->GetSkillData(
          (uint32_t)condition->GetValue1());
      if (devilData && skillData) {
        auto dRestr = devilData->GetGrowth()->GetInheritanceRestrictions();
        auto sRestr = skillData->GetAcquisition()->GetInheritanceRestriction();
        return (dRestr & (uint16_t)(1 << sRestr)) != 0;
      }

      return false;
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::CHARACTER_NO_STATUS: {
      // Character does not have status
      return !cState->StatusEffectActive((uint32_t)condition->GetValue1());
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::DEMON_NO_STATUS: {
      // Demon does not have status
      return !dState->StatusEffectActive((uint32_t)condition->GetValue1());
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::CHARACTER_NO_SKILL: {
      // Exchange target (character or demon) does not have skill
      bool targetCharacter = false;
      bool targetDemon = false;
      for (auto itemData : barterData->GetResultItems()) {
        auto type = itemData->GetType();
        switch (type) {
          case objects::MiNPCBarterItemData::Type_t::STATUS_CHARACTER:
          case objects::MiNPCBarterItemData::Type_t::SKILL_CHARACTER:
            targetCharacter = true;
            break;
          case objects::MiNPCBarterItemData::Type_t::STATUS_DEMON:
          case objects::MiNPCBarterItemData::Type_t::SKILL_DEMON:
            targetDemon = true;
            break;
          case objects::MiNPCBarterItemData::Type_t::STATUS_CHARACTER_AND_DEMON:
            targetCharacter = true;
            targetDemon = true;
            break;
          default:
            break;
        }
      }

      if (targetCharacter) {
        auto character = cState->GetEntity();
        if (!character || character->LearnedSkillsContains(
                              (uint32_t)condition->GetValue1())) {
          return false;
        }
      }

      // Defaults to demon if no target
      if (targetDemon || !targetCharacter) {
        auto demon = dState->GetEntity();
        if (demon) {
          // Skill existing anywhere on the demon makes it invalid
          for (uint32_t skillID : demon->GetAcquiredSkills()) {
            if (skillID == (uint32_t)condition->GetValue1()) {
              return false;
            }
          }

          for (uint32_t skillID : demon->GetLearnedSkills()) {
            if (skillID == (uint32_t)condition->GetValue1()) {
              return false;
            }
          }

          for (auto iSkillRef : demon->GetInheritedSkills()) {
            auto iSkill = iSkillRef.Get();
            if (iSkill &&
                iSkill->GetSkill() == (uint32_t)condition->GetValue1()) {
              return false;
            }
          }
        }
      }

      return true;
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::DEMON_TYPE: {
      // Demon has specific type
      auto devilData = dState->GetDevilData();
      return devilData &&
             devilData->GetBasic()->GetID() == (uint32_t)condition->GetValue1();
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::DEMON_BASE_TYPE: {
      // Demon has base type
      auto devilData = dState->GetDevilData();
      return devilData && devilData->GetUnionData()->GetBaseDemonID() ==
                              (uint32_t)condition->GetValue1();
    }
    case objects::MiNPCBarterConditionDataEntry::Type_t::NONE:
    default:
      break;
  }

  return true;
}

static void HandleBarter(const std::shared_ptr<ChannelServer> server,
                         const std::shared_ptr<ChannelClientConnection> client,
                         uint16_t barterID) {
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto character = cState->GetEntity();
  auto demon = dState->GetEntity();

  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  auto barterData = definitionManager->GetNPCBarterData(barterID);
  auto barterGroupData = definitionManager->GetNPCBarterGroupData(
      (uint16_t)state->GetCurrentMenuShopID((int32_t)SVR_CONST.MENU_BARTER));
  auto barterConditions = barterGroupData
                              ? definitionManager->GetNPCBarterConditionData(
                                    barterGroupData->GetID())
                              : nullptr;

  int32_t spAdjust = 0;
  int64_t coinAdjust = 0;
  std::unordered_map<uint32_t, int32_t> itemAdjustments;

  int32_t cowrieAdjust = 0;
  bool includesBethel = false;
  std::array<int32_t, 5> bethelAdjustments = {{0, 0, 0, 0, 0}};

  bool failed = barterData == nullptr || barterGroupData == nullptr;

  std::shared_ptr<objects::MiNPCBarterGroupEntry> groupEntry;
  if (!failed) {
    for (auto entry : barterGroupData->GetEntries()) {
      if (entry->GetBarterID() == barterID) {
        groupEntry = entry;
        break;
      }
    }

    failed = groupEntry == nullptr;
  }

  if (!failed && barterConditions) {
    for (auto condition : barterConditions->GetConditions()) {
      if (!BarterConditionValid(server, cState, dState, barterData,
                                condition)) {
        auto accountUID = state->GetAccountUID();
        LogBarterError([barterID, accountUID]() {
          return libcomp::String(
                     "One or more barter conditions invalid for player on "
                     "barter ID %1: %2\n")
              .Arg(barterID)
              .Arg(accountUID.ToString());
        });

        failed = true;
        break;
      }
    }
  }

  if (!failed) {
    for (auto itemData : barterData->GetTradeItems()) {
      auto type = itemData->GetType();
      switch (type) {
        case objects::MiNPCBarterItemData::Type_t::ITEM: {
          uint32_t itemType = (uint32_t)itemData->GetSubtype();
          if (itemAdjustments.find(itemType) == itemAdjustments.end()) {
            itemAdjustments[itemType] = -itemData->GetAmount();
          } else {
            itemAdjustments[itemType] =
                (int32_t)(itemAdjustments[itemType] - itemData->GetAmount());
          }
        } break;
        case objects::MiNPCBarterItemData::Type_t::SOUL_POINT:
          spAdjust = (int32_t)(spAdjust - itemData->GetSubtype());

          if (!demon || (demon->GetSoulPoints() < -spAdjust)) {
            failed = true;
          }
          break;
        case objects::MiNPCBarterItemData::Type_t::BETHEL: {
          includesBethel = true;
          size_t bethelType = (size_t)(itemData->GetSubtype());
          if (bethelType == 0) {
            // It's cowries
            cowrieAdjust = (int32_t)(cowrieAdjust - itemData->GetAmount());

            if (character->GetProgress()->GetCowrie() < -cowrieAdjust) {
              failed = true;
            }
          } else if (bethelType > 0 && bethelType < 6) {
            // It's a colored bethel; subtract the type by 1 and handle it as
            // such in its adjustment array
            bethelType -= 1;
            bethelAdjustments[bethelType] = (int32_t)(
                bethelAdjustments[bethelType] - itemData->GetAmount());

            if (character->GetProgress()->GetBethel(bethelType) <
                -bethelAdjustments[bethelType]) {
              failed = true;
            }
          } else {
            failed = true;
          }
        } break;
        case objects::MiNPCBarterItemData::Type_t::COIN: {
          int64_t total = (int64_t)((int64_t)itemData->GetSubtype() * 1000000 +
                                    (int64_t)itemData->GetAmount());
          coinAdjust = (int64_t)(coinAdjust - total);

          if (character->GetProgress()->GetCoins() < -coinAdjust) {
            failed = true;
          }
        } break;
        case objects::MiNPCBarterItemData::Type_t::NONE:
          break;
        case objects::MiNPCBarterItemData::Type_t::ONE_TIME_VALUABLE:
        case objects::MiNPCBarterItemData::Type_t::STATUS_CHARACTER:
        case objects::MiNPCBarterItemData::Type_t::EVENT_COUNTER:
        case objects::MiNPCBarterItemData::Type_t::COOLDOWN:
        case objects::MiNPCBarterItemData::Type_t::STATUS_DEMON:
        case objects::MiNPCBarterItemData::Type_t::STATUS_CHARACTER_AND_DEMON:
        case objects::MiNPCBarterItemData::Type_t::SKILL_CHARACTER:
        case objects::MiNPCBarterItemData::Type_t::SKILL_DEMON:
        case objects::MiNPCBarterItemData::Type_t::PLUGIN:
        default:
          LogBarterError([type]() {
            return libcomp::String(
                       "Invalid barter trade item type encountered: %1\n")
                .Arg((uint8_t)type);
          });

          failed = true;
          break;
      }
    }
  }

  std::list<int32_t> characterSkills;
  std::list<int32_t> demonSkills;
  std::list<int32_t> pluginIDs;
  std::set<uint16_t> oneTimeValuables;
  StatusEffectChanges characterStatus;
  StatusEffectChanges demonStatus;
  std::unordered_map<int32_t, uint32_t> cooldowns;
  std::unordered_map<int32_t, int32_t> eCounters;
  if (!failed) {
    for (auto itemData : barterData->GetResultItems()) {
      auto type = itemData->GetType();
      switch (type) {
        case objects::MiNPCBarterItemData::Type_t::ITEM: {
          uint32_t itemType = (uint32_t)itemData->GetSubtype();
          if (itemAdjustments.find(itemType) == itemAdjustments.end()) {
            itemAdjustments[itemType] = itemData->GetAmount();
          } else {
            itemAdjustments[itemType] =
                (int32_t)(itemAdjustments[itemType] + itemData->GetAmount());
          }
        } break;
        case objects::MiNPCBarterItemData::Type_t::ONE_TIME_VALUABLE: {
          uint16_t valuableID = (uint16_t)itemData->GetSubtype();
          oneTimeValuables.insert(valuableID);

          if (CharacterManager::HasValuable(character, valuableID)) {
            LogBarterError([valuableID]() {
              return libcomp::String(
                         "Player attempted to perform barter with a one-time "
                         "valuable they already have: %1\n")
                  .Arg(valuableID);
            });

            failed = true;
          }
        } break;
        case objects::MiNPCBarterItemData::Type_t::STATUS_CHARACTER:
          characterStatus[(uint32_t)itemData->GetSubtype()] =
              StatusEffectChange((uint32_t)itemData->GetSubtype(),
                                 (int8_t)itemData->GetAmount(), true);
          break;
        case objects::MiNPCBarterItemData::Type_t::STATUS_DEMON:
          demonStatus[(uint32_t)itemData->GetSubtype()] =
              StatusEffectChange((uint32_t)itemData->GetSubtype(),
                                 (int8_t)itemData->GetAmount(), true);
          break;
        case objects::MiNPCBarterItemData::Type_t::STATUS_CHARACTER_AND_DEMON:
          characterStatus[(uint32_t)itemData->GetSubtype()] =
              demonStatus[(uint32_t)itemData->GetSubtype()] =
                  StatusEffectChange((uint32_t)itemData->GetSubtype(),
                                     (int8_t)itemData->GetAmount(), true);
          break;
        case objects::MiNPCBarterItemData::Type_t::SOUL_POINT:
          spAdjust = (int32_t)(spAdjust + itemData->GetSubtype());
          break;
        case objects::MiNPCBarterItemData::Type_t::EVENT_COUNTER:
          // Negate for system types
          eCounters[-itemData->GetSubtype()] = itemData->GetAmount();
          break;
        case objects::MiNPCBarterItemData::Type_t::COOLDOWN:
          // Calculate the cooldown(s) below (negate for system types)
          cooldowns[-itemData->GetSubtype()] = 0;
          break;
        case objects::MiNPCBarterItemData::Type_t::BETHEL: {
          includesBethel = true;
          size_t bethelType = (size_t)(itemData->GetSubtype());
          if (bethelType == 0) {
            // It's cowries
            cowrieAdjust = (int32_t)(cowrieAdjust + itemData->GetAmount());
          } else if (bethelType > 0 && bethelType < 6) {
            // It's a colored bethel; subtract the type by 1 and handle it as
            // such in its adjustment array
            bethelType -= 1;
            bethelAdjustments[bethelType] = (int32_t)(
                bethelAdjustments[bethelType] + itemData->GetAmount());
          } else {
            failed = true;
          }
        } break;
        case objects::MiNPCBarterItemData::Type_t::SKILL_CHARACTER:
          characterSkills.push_back(itemData->GetSubtype());
          break;
        case objects::MiNPCBarterItemData::Type_t::SKILL_DEMON:
          if (!demon) {
            auto accountUID = state->GetAccountUID();
            LogBarterError([accountUID]() {
              return libcomp::String(
                         "Attempted to add a barter demon skill to a player "
                         "without a demon summoned: %1\n")
                  .Arg(accountUID.ToString());
            });

            failed = true;
          } else {
            demonSkills.push_back(itemData->GetSubtype());
          }
          break;
        case objects::MiNPCBarterItemData::Type_t::PLUGIN:
          pluginIDs.push_back(itemData->GetSubtype());
          break;
        case objects::MiNPCBarterItemData::Type_t::COIN: {
          int64_t total = (int64_t)((int64_t)itemData->GetSubtype() * 1000000 +
                                    (int64_t)itemData->GetAmount());
          coinAdjust = (int64_t)(coinAdjust + total);
        } break;
        case objects::MiNPCBarterItemData::Type_t::NONE:
          break;
        default:
          LogBarterError([type]() {
            return libcomp::String(
                       "Invalid barter result item type encountered: %1\n")
                .Arg((uint8_t)type);
          });

          failed = true;
          break;
      }
    }
  }

  if (cooldowns.size() > 0 && !failed) {
    // Fail if any cooldowns are active, otherwise calculate new times
    uint32_t now = (uint32_t)std::time(0);

    cState->RefreshActionCooldowns(false, now);
    for (auto& pair : cooldowns) {
      if (!cState->ActionCooldownActive(pair.first, false)) {
        auto it = SVR_CONST.BARTER_COOLDOWNS.find(-pair.first);
        if (it != SVR_CONST.BARTER_COOLDOWNS.end()) {
          pair.second = (uint32_t)(now + it->second);
        }
      } else {
        int32_t cooldownID = pair.first;
        auto accountUID = state->GetAccountUID();
        LogBarterError([cooldownID, accountUID]() {
          return libcomp::String(
                     "Attempted to execute barter with active action cooldown "
                     "type %1: %2\n")
              .Arg(-cooldownID)
              .Arg(accountUID.ToString());
        });

        failed = true;
        break;
      }
    }
  }

  if (includesBethel) {
    auto progress = character->GetProgress().Get();
    for (size_t i = 0; i < 5; i++) {
      if (bethelAdjustments[i] < 0 &&
          (progress->GetBethel(i) + bethelAdjustments[i]) < 0) {
        auto accountUID = state->GetAccountUID();
        LogBarterError([accountUID]() {
          return libcomp::String(
                     "Attempted to execute barter without enough bethel: %1\n")
              .Arg(accountUID.ToString());
        });

        failed = true;
        break;
      }
    }
  }

  if (!failed) {
    // If there have not been failures yet, determine item adjustments
    // and apply all changes
    auto inventory = character->GetItemBoxes(0).Get();

    std::list<std::shared_ptr<objects::Item>> insertItems;
    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t>
        stackAdjustItems;
    for (auto itemPair : itemAdjustments) {
      auto itemData = definitionManager->GetItemData(itemPair.first);
      if (!itemData) {
        LogBarterError([&]() {
          return libcomp::String(
                     "Invalid item type encountered for barter request: %1\n")
              .Arg(itemPair.first);
        });

        failed = true;
        break;
      }

      int32_t qtyLeft = itemPair.second;

      auto existing = characterManager->GetExistingItems(
          character, itemPair.first, inventory);
      if (qtyLeft > 0) {
        // Update existing stacks first if we aren't adding a full stack
        int32_t maxStack = (int32_t)itemData->GetPossession()->GetStackSize();
        if (qtyLeft < maxStack) {
          for (auto item : existing) {
            if (qtyLeft == 0) break;

            uint16_t stackLeft = (uint16_t)(maxStack - item->GetStackSize());
            if (stackLeft == 0) continue;

            uint16_t stackAdd =
                (uint16_t)((qtyLeft <= stackLeft) ? qtyLeft : stackLeft);

            if (stackAdjustItems.find(item) == stackAdjustItems.end()) {
              stackAdjustItems[item] = item->GetStackSize();
            }

            stackAdjustItems[item] =
                (uint16_t)(stackAdjustItems[item] + stackAdd);
            qtyLeft = (int32_t)(qtyLeft - stackAdd);
          }
        }

        // If there are still more to create, add as new items
        while (qtyLeft > 0) {
          uint16_t stack =
              (uint16_t)((qtyLeft > maxStack) ? maxStack : qtyLeft);
          insertItems.push_back(
              characterManager->GenerateItem(itemPair.first, stack));
          qtyLeft = (int32_t)(qtyLeft - stack);
        }
      } else if (qtyLeft < 0) {
        // Remove from the last stack first and reverse the quantity
        // for easier comparison
        existing.reverse();
        qtyLeft = qtyLeft * -1;

        for (auto item : existing) {
          if (qtyLeft == 0) break;

          if (qtyLeft <= (int32_t)item->GetStackSize()) {
            stackAdjustItems[item] = (uint16_t)(item->GetStackSize() - qtyLeft);
            qtyLeft = 0;
          } else {
            stackAdjustItems[item] = 0;
            qtyLeft = qtyLeft - item->GetStackSize();
          }
        }

        if (qtyLeft > 0) {
          failed = true;
          break;
        }
      }
    }

    // Attempt to pay/convert bethel first
    if (includesBethel) {
      failed |= !characterManager->UpdateCowrieBethel(client, cowrieAdjust,
                                                      bethelAdjustments);
    }

    // Update items first as they're the only thing that can actually
    // fail past this point when everything is working right
    if (!failed && (stackAdjustItems.size() > 0 || insertItems.size() > 0)) {
      failed |= !characterManager->UpdateItems(client, false, insertItems,
                                               stackAdjustItems);
    }

    // Now apply the rest of the updates
    if (!failed) {
      if (spAdjust != 0) {
        characterManager->UpdateSoulPoints(client, spAdjust, true);
      }

      if (coinAdjust != 0) {
        characterManager->UpdateCoinTotal(client, coinAdjust, true);
      }

      if (characterSkills.size() > 0) {
        int32_t characterEntityID = state->GetCharacterState()->GetEntityID();
        for (int32_t skillID : characterSkills) {
          failed |= !characterManager->LearnSkill(client, characterEntityID,
                                                  (uint32_t)skillID);
        }
      }

      if (demonSkills.size() > 0) {
        int32_t demonEntityID = state->GetDemonState()->GetEntityID();
        for (int32_t skillID : demonSkills) {
          failed |= !characterManager->LearnSkill(client, demonEntityID,
                                                  (uint32_t)skillID);
        }
      }

      if (characterStatus.size() > 0) {
        cState->AddStatusEffects(characterStatus, definitionManager);
      }

      if (demonStatus.size() > 0 && state->GetDemonState()->GetEntity()) {
        state->GetDemonState()->AddStatusEffects(demonStatus,
                                                 definitionManager);
      }

      for (int32_t pluginID : pluginIDs) {
        failed |= !characterManager->AddPlugin(client, (uint16_t)pluginID);
      }

      for (uint16_t valuableID : oneTimeValuables) {
        failed |=
            !characterManager->AddRemoveValuable(client, valuableID, false);
      }

      if (!failed) {
        bool updated = false;
        for (auto& pair : cooldowns) {
          if (pair.second) {
            character->SetActionCooldowns(pair.first, pair.second);
            updated = true;
          }
        }

        if (updated) {
          server->GetWorldDatabase()->QueueUpdate(character,
                                                  state->GetAccountUID());
        }
      }

      if (!failed) {
        for (auto& pair : eCounters) {
          failed |= !characterManager->UpdateEventCounter(client, pair.first,
                                                          pair.second, true);
        }
      }
    }
  }

  // Certain exchanges need to force the menu to close so pre-barter checks
  // can run again
  bool autoCloseEnabled = oneTimeValuables.size() > 0 || cooldowns.size() > 0 ||
                          eCounters.size() > 0 ||
                          (groupEntry && (groupEntry->GetFlags() & 0x01) != 0);

  int8_t result = failed ? -1 : 0;
  if (result == 0) {
    auto accountUID = state->GetAccountUID();
    LogBarterDebug([barterID, accountUID]() {
      return libcomp::String("Barter %1 successful for player: %2\n")
          .Arg(barterID)
          .Arg(accountUID.ToString());
    });
  }

  if (!failed && autoCloseEnabled) {
    // Close menu and wait for next event or event end
    result = 1;
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BARTER);
  reply.WriteS8(result);
  reply.WriteU16Little(barterID);

  client->QueuePacket(reply);

  if (result == 1) {
    // When auto-close occurs, support "next" event handling via a system
    // response
    server->GetEventManager()->HandleResponse(client, -1);
  }

  client->FlushOutgoing();
}

bool Parsers::Barter::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 2) {
    return false;
  }

  uint16_t barterID = p.ReadU16Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();

  HandleBarter(server, client, barterID);

  return true;
}
