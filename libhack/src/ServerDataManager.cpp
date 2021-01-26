/**
 * @file libhack/src/ServerDataManager.cpp
 * @ingroup libhack
 *
 * @author HACKfrost
 *
 * @brief Manages loading and storing server data objects.
 *
 * This file is part of the COMP_hack Library (libhack).
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

#include "ServerDataManager.h"

#ifndef EXOTIC_PLATFORM

// libhack Includes
#include "ScriptEngine.h"

// libcomp Includes
#include "DefinitionManager.h"
#include "Log.h"

// object Includes
#include <AILogicGroup.h>
#include <Action.h>
#include <ActionCreateLoot.h>
#include <ActionDelay.h>
#include <ActionSpawn.h>
#include <ActionStartEvent.h>
#include <ActionZoneChange.h>
#include <ActionZoneInstance.h>
#include <DemonFamiliarityType.h>
#include <DemonPresent.h>
#include <DemonQuestReward.h>
#include <DropSet.h>
#include <EnchantSetData.h>
#include <EnchantSpecialData.h>
#include <Event.h>
#include <EventChoice.h>
#include <EventITime.h>
#include <EventOpenMenu.h>
#include <EventPerformActions.h>
#include <EventPrompt.h>
#include <FusionMistake.h>
#include <ItemDrop.h>
#include <MiSItemData.h>
#include <MiSStatusData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <PlasmaSpawn.h>
#include <PvPInstanceVariant.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerShop.h>
#include <ServerShopProduct.h>
#include <ServerShopTab.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <ServerZoneInstanceVariant.h>
#include <ServerZonePartial.h>
#include <ServerZoneSpot.h>
#include <ServerZoneTrigger.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocationGroup.h>
#include <Tokusei.h>

// Standard C Includes
#include <cmath>

using namespace libcomp;
using namespace libhack;

ServerDataManager::ServerDataManager() {}

ServerDataManager::~ServerDataManager() {}

const std::shared_ptr<objects::ServerZone> ServerDataManager::GetZoneData(
    uint32_t id, uint32_t dynamicMapID, bool applyPartials,
    std::set<uint32_t> extraPartialIDs) {
  std::shared_ptr<objects::ServerZone> zone;

  auto iter = mZoneData.find(id);
  if (iter != mZoneData.end()) {
    if (dynamicMapID != 0) {
      auto dIter = iter->second.find(dynamicMapID);
      zone = (dIter != iter->second.end()) ? dIter->second : nullptr;
    } else {
      // Return first
      zone = iter->second.begin()->second;
    }
  }

  if (applyPartials && zone) {
    std::set<uint32_t> partialIDs;

    // Gather all auto-applied partials
    auto partialIter = mZonePartialMap.find(zone->GetDynamicMapID());
    if (partialIter != mZonePartialMap.end()) {
      partialIDs = partialIter->second;
    }

    // Gather and verify all extra partials
    for (uint32_t partialID : extraPartialIDs) {
      auto partial = GetZonePartialData(partialID);
      if (partial && !partial->GetAutoApply() &&
          (partial->DynamicMapIDsCount() == 0 ||
           partial->DynamicMapIDsContains(zone->GetDynamicMapID()))) {
        partialIDs.insert(partialID);
      }
    }

    if (partialIDs.size() > 0) {
      // Copy the definition and apply changes
      libcomp::String zoneStr = libcomp::String("%1%2").Arg(id).Arg(
          id != dynamicMapID ? libcomp::String(" (%1)").Arg(dynamicMapID) : "");

      zone = std::make_shared<objects::ServerZone>(*zone);
      for (uint32_t partialID : partialIDs) {
        if (!ApplyZonePartial(zone, partialID)) {
          // Errored, no zone should be returned
          return nullptr;
        }
      }

      // Now validate spawn information and correct as needed
      std::set<uint32_t> sgRemoves;
      for (auto sgPair : zone->GetSpawnGroups()) {
        std::set<uint32_t> missingSpawns;
        for (auto sPair : sgPair.second->GetSpawns()) {
          if (!zone->SpawnsKeyExists(sPair.first)) {
            missingSpawns.insert(sPair.first);
          }
        }

        if (missingSpawns.size() > 0) {
          if (missingSpawns.size() < sgPair.second->SpawnsCount()) {
            // Copy the group and edit the spawns
            auto sg = std::make_shared<objects::SpawnGroup>(*sgPair.second);
            for (uint32_t remove : sgRemoves) {
              sg->RemoveSpawns(remove);
            }

            zone->SetSpawnGroups(sgPair.first, sg);
          } else {
            sgRemoves.insert(sgPair.first);
          }
        }
      }

      for (uint32_t sgRemove : sgRemoves) {
        LogServerDataManagerDebug([&]() {
          return libcomp::String(
                     "Removing empty spawn group %1 when generating zone: %2\n")
              .Arg(sgRemove)
              .Arg(zoneStr);
        });

        zone->RemoveSpawnGroups(sgRemove);
      }

      std::set<uint32_t> slgRemoves;
      for (auto slgPair : zone->GetSpawnLocationGroups()) {
        std::set<uint32_t> missingGroups;
        for (uint32_t sgID : slgPair.second->GetGroupIDs()) {
          if (!zone->SpawnGroupsKeyExists(sgID)) {
            missingGroups.insert(sgID);
          }
        }

        if (missingGroups.size() > 0) {
          if (missingGroups.size() < slgPair.second->GroupIDsCount()) {
            // Copy the group and edit the spawns
            auto slg =
                std::make_shared<objects::SpawnLocationGroup>(*slgPair.second);
            for (uint32_t remove : sgRemoves) {
              slg->RemoveGroupIDs(remove);
            }

            zone->SetSpawnLocationGroups(slgPair.first, slg);
          } else {
            slgRemoves.insert(slgPair.first);
          }
        }
      }

      for (uint32_t slgRemove : slgRemoves) {
        LogServerDataManagerDebug([&]() {
          return String(
                     "Removing empty spawn location group %1 when generating "
                     "zone: %2\n")
              .Arg(slgRemove)
              .Arg(zoneStr);
        });

        zone->RemoveSpawnLocationGroups(slgRemove);
      }
    }
  }

  return zone;
}

const std::list<std::pair<uint32_t, uint32_t>>
ServerDataManager::GetFieldZoneIDs() {
  return mFieldZoneIDs;
}

const std::unordered_map<uint32_t, std::set<uint32_t>>
ServerDataManager::GetAllZoneIDs() {
  std::unordered_map<uint32_t, std::set<uint32_t>> zoneIDs;
  for (auto pair : mZoneData) {
    for (auto dPair : pair.second) {
      zoneIDs[pair.first].insert(dPair.first);
    }
  }

  return zoneIDs;
}

const std::shared_ptr<objects::ServerZoneInstance>
ServerDataManager::GetZoneInstanceData(uint32_t id) {
  return GetObjectByID<uint32_t, objects::ServerZoneInstance>(
      id, mZoneInstanceData);
}

const std::set<uint32_t> ServerDataManager::GetAllZoneInstanceIDs() {
  std::set<uint32_t> instanceIDs;
  for (auto pair : mZoneInstanceData) {
    instanceIDs.insert(pair.first);
  }

  return instanceIDs;
}

bool ServerDataManager::ExistsInInstance(uint32_t instanceID, uint32_t zoneID,
                                         uint32_t dynamicMapID) {
  auto instDef = GetZoneInstanceData(instanceID);
  if (instDef) {
    for (size_t i = 0; i < instDef->ZoneIDsCount(); i++) {
      if (instDef->GetZoneIDs(i) == zoneID &&
          (!dynamicMapID || instDef->GetDynamicMapIDs(i) == dynamicMapID)) {
        return true;
      }
    }
  }

  return false;
}

const std::shared_ptr<objects::ServerZoneInstanceVariant>
ServerDataManager::GetZoneInstanceVariantData(uint32_t id) {
  return GetObjectByID<uint32_t, objects::ServerZoneInstanceVariant>(
      id, mZoneInstanceVariantData);
}

std::set<uint32_t> ServerDataManager::GetStandardPvPVariantIDs(
    uint8_t type) const {
  auto it = mStandardPvPVariantIDs.find(type);
  return it != mStandardPvPVariantIDs.end() ? it->second : std::set<uint32_t>();
}

bool ServerDataManager::VerifyPvPInstance(
    uint32_t instanceID, DefinitionManager* definitionManager) {
  auto instanceDef = GetZoneInstanceData(instanceID);
  if (instanceDef && definitionManager) {
    for (uint32_t zoneID : instanceDef->GetZoneIDs()) {
      auto zoneDef = definitionManager->GetZoneData(zoneID);
      if (!zoneDef || zoneDef->GetBasic()->GetType() !=
                          objects::MiZoneBasicData::Type_t::PVP) {
        LogServerDataManagerError([&]() {
          return String(
                     "Instance contains non-PvP zones and cannot be used for "
                     "PvP: %1\n")
              .Arg(instanceID);
        });

        return false;
      }
    }

    return true;
  }

  LogServerDataManagerError([&]() {
    return String("Failed to verify PvP instance: %1\n").Arg(instanceID);
  });

  return false;
}

const std::shared_ptr<objects::ServerZonePartial>
ServerDataManager::GetZonePartialData(uint32_t id) {
  return GetObjectByID<uint32_t, objects::ServerZonePartial>(id,
                                                             mZonePartialData);
}

const std::shared_ptr<objects::Event> ServerDataManager::GetEventData(
    const libcomp::String& id) {
  return GetObjectByID<std::string, objects::Event>(id.C(), mEventData);
}

const std::shared_ptr<objects::ServerShop> ServerDataManager::GetShopData(
    uint32_t id) {
  return GetObjectByID<uint32_t, objects::ServerShop>(id, mShopData);
}

std::list<uint32_t> ServerDataManager::GetCompShopIDs() const {
  return mCompShopIDs;
}

const std::shared_ptr<objects::AILogicGroup> ServerDataManager::GetAILogicGroup(
    uint16_t id) {
  return GetObjectByID<uint16_t, objects::AILogicGroup>(id, mAILogicGroups);
}

const std::shared_ptr<objects::DemonFamiliarityType>
ServerDataManager::GetDemonFamiliarityTypeData(int32_t id) {
  return GetObjectByID<int32_t, objects::DemonFamiliarityType>(
      id, mDemonFamiliarityTypeData);
}

const std::shared_ptr<objects::DemonPresent>
ServerDataManager::GetDemonPresentData(uint32_t id) {
  return GetObjectByID<uint32_t, objects::DemonPresent>(id, mDemonPresentData);
}

std::unordered_map<uint32_t, std::shared_ptr<objects::DemonQuestReward>>
ServerDataManager::GetDemonQuestRewardData() {
  return mDemonQuestRewardData;
}

const std::shared_ptr<objects::DropSet> ServerDataManager::GetDropSetData(
    uint32_t id) {
  return GetObjectByID<uint32_t, objects::DropSet>(id, mDropSetData);
}

std::unordered_map<uint32_t, std::shared_ptr<objects::FusionMistake>>
ServerDataManager::GetFusionMistakeData() {
  return mFusionMistakeData;
}

const std::shared_ptr<objects::DropSet> ServerDataManager::GetGiftDropSetData(
    uint32_t giftBoxID) {
  auto it = mGiftDropSetLookup.find(giftBoxID);
  return it != mGiftDropSetLookup.end() ? GetDropSetData(it->second) : nullptr;
}

const std::shared_ptr<ServerScript> ServerDataManager::GetScript(
    const libcomp::String& name) {
  return GetObjectByID<std::string, ServerScript>(name.C(), mScripts);
}

const std::shared_ptr<ServerScript> ServerDataManager::GetAIScript(
    const libcomp::String& name) {
  return GetObjectByID<std::string, ServerScript>(name.C(), mAIScripts);
}

bool ServerDataManager::LoadData(DataStore* pDataStore,
                                 DefinitionManager* definitionManager) {
  bool failure = false;

  if (definitionManager) {
    // Load definition dependent server definitions from path or file
    if (!failure) {
      LogServerDataManagerDebugMsg(
          "Loading AI logic group server definitions...\n");

      failure = !LoadObjects<objects::AILogicGroup>(
          pDataStore, "/data/ailogicgroup", definitionManager, true, true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg(
          "Loading demon familiarity type server definitions...\n");

      failure = !LoadObjects<objects::DemonFamiliarityType>(
          pDataStore, "/data/demonfamiliaritytype", definitionManager, true,
          true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg(
          "Loading demon present server definitions...\n");

      failure = !LoadObjects<objects::DemonPresent>(
          pDataStore, "/data/demonpresent", definitionManager, true, true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg(
          "Loading demon quest reward server definitions...\n");

      failure = !LoadObjects<objects::DemonQuestReward>(
          pDataStore, "/data/demonquestreward", definitionManager, true, true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg("Loading drop set server definitions...\n");

      failure = !LoadObjects<objects::DropSet>(pDataStore, "/data/dropset",
                                               definitionManager, true, true);
      if (!failure) {
        ApplyPendingDrops();
      }
    }

    if (!failure) {
      LogServerDataManagerDebugMsg(
          "Loading enchant set server definitions...\n");

      failure = !LoadObjects<objects::EnchantSetData>(
          pDataStore, "/data/enchantset", definitionManager, true, true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg(
          "Loading enchant special server definitions...\n");

      failure = !LoadObjects<objects::EnchantSpecialData>(
          pDataStore, "/data/enchantspecial", definitionManager, true, true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg(
          "Loading fusion mistake server definitions...\n");

      failure = !LoadObjects<objects::FusionMistake>(
          pDataStore, "/data/fusionmistake", definitionManager, true, true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg("Loading s-item server definitions...\n");

      failure = !LoadObjects<objects::MiSItemData>(
          pDataStore, "/data/sitemextended", definitionManager, true, true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg("Loading s-status server definitions...\n");

      failure = !LoadObjects<objects::MiSStatusData>(
          pDataStore, "/data/sstatus", definitionManager, true, true);
    }

    if (!failure) {
      LogServerDataManagerDebugMsg("Loading tokusei server definitions...\n");

      failure = !LoadObjects<objects::Tokusei>(pDataStore, "/data/tokusei",
                                               definitionManager, true, true);
    }
  }

  if (!failure) {
    LogServerDataManagerDebugMsg("Loading zone server definitions...\n");

    failure = !LoadObjects<objects::ServerZone>(
        pDataStore, "/zones", definitionManager, false, false);
  }

  if (!failure) {
    LogServerDataManagerDebugMsg(
        "Loading zone partial server definitions...\n");

    failure = !LoadObjects<objects::ServerZonePartial>(
        pDataStore, "/zones/partial", definitionManager, true, false);
  }

  if (!failure) {
    LogServerDataManagerDebugMsg("Loading event server definitions...\n");

    failure = !LoadObjects<objects::Event>(pDataStore, "/events",
                                           definitionManager, true, false);
  }

  if (!failure) {
    LogServerDataManagerDebugMsg(
        "Loading zone instance server definitions...\n");

    failure = !LoadObjects<objects::ServerZoneInstance>(
        pDataStore, "/data/zoneinstance", definitionManager, true, true);
  }

  if (!failure) {
    LogServerDataManagerDebugMsg(
        "Loading zone instance variant server definitions...\n");

    failure = !LoadObjects<objects::ServerZoneInstanceVariant>(
        pDataStore, "/data/zoneinstancevariant", definitionManager, true, true);
  }

  if (!failure) {
    LogServerDataManagerDebugMsg("Loading shop server definitions...\n");

    failure = !LoadObjects<objects::ServerShop>(pDataStore, "/shops",
                                                definitionManager, true, false);
  }

  if (!failure) {
    LogServerDataManagerDebugMsg("Loading server scripts...\n");

    failure =
        !LoadScripts(pDataStore, "/scripts", &ServerDataManager::LoadScript);
  }

  return !failure;
}

bool ServerDataManager::VerifyDataIntegrity(
    DefinitionManager* definitionManager) {
  bool valid = VerifyEventIntegrity();
  ;

  if (definitionManager && !VerifyItemReferences(definitionManager)) {
    valid = false;
  }

  return valid;
}

bool ServerDataManager::VerifyEventIntegrity() {
  bool valid = true;

  // Gather all sources of actions
  for (auto ePair : mEventData) {
    // Check all direct event references, ignoring invalid types here
    // as those need to be explicitly defined to differ anyway
    std::set<libcomp::String> refIDs;
    std::set<libcomp::String> invalidEventIDs;

    // Keep track of where "next" IDs can be
    std::list<std::shared_ptr<objects::EventSequence>> seqList;
    if (!ePair.second->GetSkipInvalid()) {
      seqList.push_back(ePair.second);
    }

    bool hasITimeGifts = false;
    switch (ePair.second->GetEventType()) {
      case objects::Event::EventType_t::ITIME:
      case objects::Event::EventType_t::PROMPT: {
        auto e = std::dynamic_pointer_cast<objects::EventPrompt>(ePair.second);
        if (e) {
          if (ePair.second->GetEventType() ==
              objects::Event::EventType_t::PROMPT) {
            // Prompt base paths are never taken
            if (!e->GetNext().IsEmpty()) {
              LogServerDataManagerWarning([e]() {
                return String(
                           "'next' property set on event that will never be "
                           "used %1\n")
                    .Arg(e->GetID());
              });
            }

            if (!e->GetQueueNext().IsEmpty()) {
              LogServerDataManagerWarning([e]() {
                return String(
                           "'queueNext' property set on event that will never "
                           "be used %1\n")
                    .Arg(e->GetID());
              });
            }

            if (e->BranchesCount() > 0) {
              LogServerDataManagerWarning([e]() {
                return String(
                           "'branches' property set on event that will never "
                           "be used %1\n")
                    .Arg(e->GetID());
              });
            }

            seqList.clear();
          } else {
            // Get "start actions" event
            auto iTime = std::dynamic_pointer_cast<objects::EventITime>(e);
            if (iTime) {
              if (!iTime->GetStartActions().IsEmpty()) {
                refIDs.insert(iTime->GetStartActions());
              }

              hasITimeGifts = iTime->GiftIDsCount() > 0;
            }
          }

          for (auto& choice : e->GetChoices()) {
            if (choice->GetSkipInvalid()) continue;

            seqList.push_back(choice);
          }
        }
      } break;
      case objects::Event::EventType_t::OPEN_MENU: {
        auto e =
            std::dynamic_pointer_cast<objects::EventOpenMenu>(ePair.second);
        if (e && !e->GetUseNext().IsEmpty()) {
          refIDs.insert(e->GetUseNext());
        }
      } break;
      case objects::Event::EventType_t::PERFORM_ACTIONS: {
        auto e = std::dynamic_pointer_cast<objects::EventPerformActions>(
            ePair.second);
        if (e) {
          auto actions = GetAllActions(e->GetActions());
          for (auto eventID : GetInvalidEventIDs(actions)) {
            invalidEventIDs.insert(eventID);
          }
        }
      } break;
      default:
        break;
    }

    for (auto& seq : seqList) {
      if (!seq->GetNext().IsEmpty()) {
        refIDs.insert(seq->GetNext());
      }

      if (!seq->GetQueueNext().IsEmpty()) {
        refIDs.insert(seq->GetQueueNext());
      }

      bool noConditionBranch = false;
      for (auto branch : seq->GetBranches()) {
        if (!branch->GetNext().IsEmpty()) {
          refIDs.insert(branch->GetNext());
        }

        if (!branch->GetQueueNext().IsEmpty()) {
          refIDs.insert(branch->GetQueueNext());
        }

        noConditionBranch |= branch->ConditionsCount() == 0;
      }

      if (noConditionBranch && !hasITimeGifts &&
          seq->GetBranchScriptID().IsEmpty()) {
        auto eventID = ePair.first;
        LogServerDataManagerError([eventID]() {
          return String(
                     "Event encountered with one or more inaccessible "
                     "branches: %1\n")
              .Arg(eventID);
        });

        valid = false;
      }
    }

    for (auto refID : refIDs) {
      if (mEventData.find(refID.C()) == mEventData.end()) {
        invalidEventIDs.insert(refID.C());
      }
    }

    for (auto eventID : invalidEventIDs) {
      LogServerDataManagerError([ePair, eventID]() {
        return String(
                   "Invalid event ID reference encountered on event %1: %2\n")
            .Arg(ePair.first)
            .Arg(eventID);
      });
    }

    valid &= invalidEventIDs.size() == 0;
  }

  // Validate zone references
  for (auto& zdPair : mZoneData) {
    for (auto& zPair : zdPair.second) {
      auto actions = GetAllZoneActions(zPair.second, true);
      for (auto eventID : GetInvalidEventIDs(actions)) {
        LogServerDataManagerError([zPair, eventID]() {
          return String(
                     "Invalid event ID reference encountered on zone %1: %2\n")
              .Arg(zPair.first)
              .Arg(eventID);
        });

        valid = false;
      }
    }
  }

  // Validate zone partial references
  for (auto& zPair : mZonePartialData) {
    for (auto action : GetAllZonePartialActions(zPair.second, true)) {
      auto actions = GetAllZonePartialActions(zPair.second, true);
      for (auto eventID : GetInvalidEventIDs(actions)) {
        LogServerDataManagerError([zPair, eventID]() {
          return String(
                     "Invalid event ID reference encountered on zone partial "
                     "%1: %2\n")
              .Arg(zPair.first)
              .Arg(eventID);
        });

        valid = false;
      }
    }
  }

  // Check instance events
  for (auto& iPair : mZoneInstanceData) {
    auto instance = iPair.second;
    for (auto eventID :
         {instance->GetCreateEventID(), instance->GetToLobbyEventID()}) {
      if (!eventID.IsEmpty() &&
          mEventData.find(eventID.C()) == mEventData.end()) {
        LogServerDataManagerError([instance, eventID]() {
          return String(
                     "Invalid event ID reference encountered on zone instance "
                     "%1: %2\n")
              .Arg(instance->GetID())
              .Arg(eventID);
        });

        valid = false;
      }
    }
  }

  // Check instance variant expiration timers
  for (auto& varPair : mZoneInstanceVariantData) {
    auto variant = varPair.second;
    auto eventID = variant->GetTimerExpirationEventID();
    if (!eventID.IsEmpty() &&
        mEventData.find(eventID.C()) == mEventData.end()) {
      LogServerDataManagerError([variant, eventID]() {
        return String(
                   "Invalid event ID reference encountered on zone instance "
                   "variant %1: %2\n")
            .Arg(variant->GetID())
            .Arg(eventID);
      });

      valid = false;
    }
  }

  return valid;
}

bool ServerDataManager::VerifyItemReferences(
    DefinitionManager* definitionManager) {
  if (!definitionManager) {
    // No issues found, not checked either
    return true;
  }

  bool valid = true;

  // Verify shop products
  for (auto& shopPair : mShopData) {
    for (auto& tab : shopPair.second->GetTabs()) {
      for (auto prod : tab->GetProducts()) {
        if (!definitionManager->GetShopProductData(prod->GetProductID())) {
          auto shop = shopPair.second;
          LogServerDataManagerError([shop, prod]() {
            return String(
                       "Invalid shop product ID encountered in shop %1: %2\n")
                .Arg(shop->GetShopID())
                .Arg(prod->GetProductID());
          });

          valid = false;
        }
      }
    }
  }

  // Verify dropsets
  for (auto& dropsetPair : mDropSetData) {
    for (auto drop : dropsetPair.second->GetDrops()) {
      if (!definitionManager->GetItemData(drop->GetItemType())) {
        auto dropset = dropsetPair.second;
        LogServerDataManagerError([dropset, drop]() {
          return String("Invalid item ID encountered in dropset %1: %2\n")
              .Arg(dropset->GetID())
              .Arg(drop->GetItemType());
        });

        valid = false;
      }
    }
  }

  // Verify demon presents
  for (auto& pPair : mDemonPresentData) {
    std::set<uint32_t> itemIDs;
    for (uint32_t itemID : pPair.second->GetCommonItems()) {
      itemIDs.insert(itemID);
    }

    for (uint32_t itemID : pPair.second->GetUncommonItems()) {
      itemIDs.insert(itemID);
    }

    for (uint32_t itemID : pPair.second->GetRareItems()) {
      itemIDs.insert(itemID);
    }

    for (uint32_t itemID : itemIDs) {
      if (!definitionManager->GetItemData(itemID)) {
        LogServerDataManagerWarning([pPair, itemID]() {
          return String("Invalid item ID encountered in demon present %1: %2\n")
              .Arg(pPair.first)
              .Arg(itemID);
        });
      }
    }
  }

  // Verify direct drops (warn of invalid dropset uses)
  for (auto& ePair : mEventData) {
    if (ePair.second->GetEventType() ==
        objects::Event::EventType_t::PERFORM_ACTIONS) {
      auto e =
          std::dynamic_pointer_cast<objects::EventPerformActions>(ePair.second);
      if (e) {
        for (auto& action : GetAllActions(
                 e->GetActions(),
                 (int8_t)objects::Action::ActionType_t::CREATE_LOOT)) {
          auto act =
              std::dynamic_pointer_cast<objects::ActionCreateLoot>(action);
          if (!act) continue;

          for (auto& drop : act->GetDrops()) {
            if (!definitionManager->GetItemData(drop->GetItemType())) {
              LogServerDataManagerError([ePair, drop]() {
                return String(
                           "Invalid item ID encountered on drop within event "
                           "%1: %2\n")
                    .Arg(ePair.first)
                    .Arg(drop->GetItemType());
              });

              valid = false;
            }
          }
        }
      }
    }
  }

  for (auto& zdPair : mZoneData) {
    for (auto& zPair : zdPair.second) {
      std::set<uint32_t> dropTypes;

      auto dropSetIDs = zPair.second->GetDropSetIDs();
      for (auto& sPair : zPair.second->GetSpawns()) {
        for (auto& drop : sPair.second->GetDrops()) {
          dropTypes.insert(drop->GetItemType());
        }

        for (auto& gift : sPair.second->GetGifts()) {
          dropTypes.insert(gift->GetItemType());
        }

        for (uint32_t dropsetID : sPair.second->GetDropSetIDs()) {
          dropSetIDs.insert(dropsetID);
        }

        for (uint32_t giftSetID : sPair.second->GetGiftSetIDs()) {
          dropSetIDs.insert(giftSetID);
        }
      }

      for (auto& pPair : zPair.second->GetPlasmaSpawns()) {
        dropSetIDs.insert(pPair.second->GetDropSetID());
      }

      for (auto& action :
           GetAllActions(GetAllZoneActions(zPair.second, false),
                         (int8_t)objects::Action::ActionType_t::CREATE_LOOT)) {
        auto act = std::dynamic_pointer_cast<objects::ActionCreateLoot>(action);
        if (!act) continue;

        for (auto& drop : act->GetDrops()) {
          dropTypes.insert(drop->GetItemType());
        }
      }

      for (uint32_t itemType : dropTypes) {
        if (!definitionManager->GetItemData(itemType)) {
          LogServerDataManagerError([zPair, itemType]() {
            return String(
                       "Invalid item ID encountered on drop in zone %1: %2\n")
                .Arg(zPair.first)
                .Arg(itemType);
          });

          valid = false;
        }
      }

      for (uint32_t dropSetID : dropSetIDs) {
        // Make sure any that are specified are not invalid
        if (dropSetID && mDropSetData.find(dropSetID) == mDropSetData.end()) {
          LogServerDataManagerWarning([zPair, dropSetID]() {
            return String("Invalid dropset ID encountered in zone %1: %2\n")
                .Arg(zPair.first)
                .Arg(dropSetID);
          });
        }
      }
    }
  }

  for (auto& zPair : mZonePartialData) {
    std::set<uint32_t> dropTypes;

    auto dropSetIDs = zPair.second->GetDropSetIDs();
    for (auto& sPair : zPair.second->GetSpawns()) {
      for (auto& drop : sPair.second->GetDrops()) {
        dropTypes.insert(drop->GetItemType());
      }

      for (auto& gift : sPair.second->GetGifts()) {
        dropTypes.insert(gift->GetItemType());
      }

      for (uint32_t dropsetID : sPair.second->GetDropSetIDs()) {
        dropSetIDs.insert(dropsetID);
      }

      for (uint32_t giftSetID : sPair.second->GetGiftSetIDs()) {
        dropSetIDs.insert(giftSetID);
      }
    }

    for (auto& pPair : zPair.second->GetPlasmaSpawns()) {
      dropSetIDs.insert(pPair.second->GetDropSetID());
    }

    for (auto& action :
         GetAllActions(GetAllZonePartialActions(zPair.second, false),
                       (int8_t)objects::Action::ActionType_t::CREATE_LOOT)) {
      auto act = std::dynamic_pointer_cast<objects::ActionCreateLoot>(action);
      if (!act) continue;

      for (auto& drop : act->GetDrops()) {
        dropTypes.insert(drop->GetItemType());
      }
    }

    for (uint32_t itemType : dropTypes) {
      if (!definitionManager->GetItemData(itemType)) {
        LogServerDataManagerError([zPair, itemType]() {
          return String(
                     "Invalid item ID encountered on drop in zone partial %1: "
                     "%2\n")
              .Arg(zPair.first)
              .Arg(itemType);
        });

        valid = false;
      }
    }

    for (uint32_t dropSetID : dropSetIDs) {
      // Make sure any that are specified are not invalid
      if (dropSetID && mDropSetData.find(dropSetID) == mDropSetData.end()) {
        LogServerDataManagerWarning([zPair, dropSetID]() {
          return String(
                     "Invalid dropset ID encountered in zone partial %1: %2\n")
              .Arg(zPair.first)
              .Arg(dropSetID);
        });
      }
    }
  }

  for (auto& rPair : mDemonQuestRewardData) {
    std::set<uint32_t> dropSetIDs;
    for (uint32_t dropSetID : rPair.second->GetNormalDropSets()) {
      dropSetIDs.insert(dropSetID);
    }

    for (uint32_t dropSetID : rPair.second->GetBonusDropSets()) {
      dropSetIDs.insert(dropSetID);
    }

    for (uint32_t dropSetID : rPair.second->GetChanceDropSets()) {
      dropSetIDs.insert(dropSetID);
    }

    for (uint32_t dropSetID : dropSetIDs) {
      if (mDropSetData.find(dropSetID) == mDropSetData.end()) {
        LogServerDataManagerWarning([rPair, dropSetID]() {
          return String(
                     "Invalid dropset ID encountered in demon quest reward %1: "
                     "%2\n")
              .Arg(rPair.first)
              .Arg(dropSetID);
        });
      }
    }
  }

  return valid;
}

std::list<std::shared_ptr<ServerScript>> ServerDataManager::LoadScripts(
    DataStore* pDataStore, const libcomp::String& path, bool& success,
    bool store) {
  std::list<std::shared_ptr<ServerScript>> scripts;

  auto scriptsOld = mScripts;
  auto aiScriptsOld = mAIScripts;

  success = LoadScripts(pDataStore, path, &ServerDataManager::LoadScript);

  // Return only ones that just loaded
  for (auto& pair : mScripts) {
    if (scriptsOld.find(pair.first) == scriptsOld.end()) {
      scripts.push_back(pair.second);
    }
  }

  for (auto& pair : mAIScripts) {
    if (aiScriptsOld.find(pair.first) == aiScriptsOld.end()) {
      scripts.push_back(pair.second);
    }
  }

  if (!store) {
    mScripts = scriptsOld;
    mAIScripts = aiScriptsOld;
  }

  return scripts;
}

bool ServerDataManager::ApplyZonePartial(
    std::shared_ptr<objects::ServerZone> zone, uint32_t partialID) {
  if (!zone || !partialID) {
    return false;
  }

  uint32_t id = zone->GetID();
  uint32_t dynamicMapID = zone->GetDynamicMapID();

  auto originDef = GetZoneData(id, dynamicMapID, false);
  if (originDef == zone) {
    LogServerDataManagerError([&]() {
      return String(
                 "Attempted to apply partial definition to original zone "
                 "definition: %1%2\n")
          .Arg(id)
          .Arg(id != dynamicMapID ? String(" (%1)").Arg(dynamicMapID) : "");
    });

    return false;
  }

  auto partial = GetZonePartialData(partialID);
  if (!partial) {
    LogServerDataManagerError([&]() {
      return String("Invalid zone partial ID encountered: %1\n").Arg(partialID);
    });

    return false;
  }

  ApplyZonePartial(zone, partial, true);

  return true;
}

void ServerDataManager::ApplyZonePartial(
    std::shared_ptr<objects::ServerZone> zone,
    const std::shared_ptr<objects::ServerZonePartial>& partial,
    bool positionReplace) {
  // Add valid team types
  for (int8_t teamType : partial->GetValidTeamTypes()) {
    zone->InsertValidTeamTypes(teamType);
  }

  // Add dropsets
  for (uint32_t dropSetID : partial->GetDropSetIDs()) {
    zone->InsertDropSetIDs(dropSetID);
  }

  // Add gift sets
  for (uint32_t giftSetID : partial->GetGiftSetIDs()) {
    zone->InsertGiftSetIDs(giftSetID);
  }

  // Add whitelist skills
  for (uint32_t skillID : partial->GetSkillWhitelist()) {
    zone->InsertSkillWhitelist(skillID);
  }

  // Add blacklist skills
  for (uint32_t skillID : partial->GetSkillBlacklist()) {
    zone->InsertSkillBlacklist(skillID);
  }

  auto npcs = zone->GetNPCs();
  auto objects = zone->GetObjects();
  if (positionReplace) {
    // Remove any NPCs/objects that share the same spot ID or are within
    // 10 units from the new one (X or Y). Do this before adding new as
    // the partial is allowed to place multiple at the same location just
    // like the zone can
    for (auto& npc : partial->GetNPCs()) {
      npcs.remove_if([npc](const std::shared_ptr<objects::ServerNPC>& oNPC) {
        return (npc->GetSpotID() && oNPC->GetSpotID() == npc->GetSpotID()) ||
               (!npc->GetSpotID() && !oNPC->GetSpotID() &&
                fabs(oNPC->GetX() - npc->GetX()) < 10.f &&
                fabs(oNPC->GetY() - npc->GetY()) < 10.f);
      });
    }

    for (auto& obj : partial->GetObjects()) {
      objects.remove_if(
          [obj](const std::shared_ptr<objects::ServerObject>& oObj) {
            return (obj->GetSpotID() &&
                    oObj->GetSpotID() == obj->GetSpotID()) ||
                   (!obj->GetSpotID() && !oObj->GetSpotID() &&
                    fabs(oObj->GetX() - obj->GetX()) < 10.f &&
                    fabs(oObj->GetY() - obj->GetY()) < 10.f);
          });
    }
  }

  // Build new NPC set
  for (auto& npc : partial->GetNPCs()) {
    // Removes supported via 0 ID
    if (npc->GetID()) {
      npcs.push_back(npc);
    }
  }
  zone->SetNPCs(npcs);

  // Build new object set
  for (auto& obj : partial->GetObjects()) {
    // Removes supported via 0 ID
    if (obj->GetID()) {
      objects.push_back(obj);
    }
  }
  zone->SetObjects(objects);

  // Update plasma spawns
  for (auto& pPair : partial->GetPlasmaSpawns()) {
    zone->SetPlasmaSpawns(pPair.first, pPair.second);
  }

  // Update spawns
  for (auto& sPair : partial->GetSpawns()) {
    auto spawn = sPair.second;
    if (spawn->GetEnemyType()) {
      // Insert/update spawn
      zone->SetSpawns(sPair.first, spawn);
    } else if (zone->SpawnsKeyExists(sPair.first)) {
      // Not redefining, append lists to existing
      auto existing = zone->GetSpawns(sPair.first);
      for (auto dropSetID : spawn->GetDropSetIDs()) {
        existing->AppendDropSetIDs(dropSetID);
      }

      for (auto drop : spawn->GetDrops()) {
        existing->AppendDrops(drop);
      }

      for (auto giftSetID : spawn->GetGiftSetIDs()) {
        existing->AppendGiftSetIDs(giftSetID);
      }

      for (auto gift : spawn->GetGifts()) {
        existing->AppendGifts(gift);
      }
    }
  }

  // Update spawn groups
  for (auto& sgPair : partial->GetSpawnGroups()) {
    zone->SetSpawnGroups(sgPair.first, sgPair.second);
  }

  // Update spawn location groups
  for (auto& slgPair : partial->GetSpawnLocationGroups()) {
    zone->SetSpawnLocationGroups(slgPair.first, slgPair.second);
  }

  // Update spots
  for (auto& spotPair : partial->GetSpots()) {
    zone->SetSpots(spotPair.first, spotPair.second);
  }

  // Add triggers
  for (auto& trigger : partial->GetTriggers()) {
    zone->AppendTriggers(trigger);
  }
}

std::list<std::shared_ptr<objects::Action>> ServerDataManager::GetAllActions(
    std::list<std::shared_ptr<objects::Action>> actions, int8_t actionType) {
  std::list<std::shared_ptr<objects::Action>> allActions;

  auto currentActions = actions;

  std::list<std::shared_ptr<objects::Action>> newActions;
  while (currentActions.size() > 0) {
    // Actions can't nest forever so loop until we're done
    for (auto action : currentActions) {
      allActions.push_back(action);

      switch (action->GetActionType()) {
        case objects::Action::ActionType_t::DELAY: {
          auto act = std::dynamic_pointer_cast<objects::ActionDelay>(action);
          for (auto act2 : act->GetActions()) {
            newActions.push_back(act2);
          }
        } break;
        case objects::Action::ActionType_t::SPAWN: {
          auto act = std::dynamic_pointer_cast<objects::ActionSpawn>(action);
          for (auto act2 : act->GetDefeatActions()) {
            newActions.push_back(act2);
          }
        } break;
        default:
          break;
      }
    }

    currentActions = newActions;
    newActions.clear();
  }

  if (actionType >= 0) {
    allActions.remove_if(
        [actionType](const std::shared_ptr<objects::Action>& action) {
          return (int8_t)action->GetActionType() != actionType;
        });
  }

  return allActions;
}

std::list<std::shared_ptr<objects::Action>>
ServerDataManager::GetAllZoneActions(
    const std::shared_ptr<objects::ServerZone>& zone, bool includeNested) {
  std::list<std::list<std::shared_ptr<objects::Action>>> actionLists;

  for (auto npc : zone->GetNPCs()) {
    actionLists.push_back(npc->GetActions());
  }

  for (auto obj : zone->GetObjects()) {
    actionLists.push_back(obj->GetActions());
  }

  for (auto sgPair : zone->GetSpawnGroups()) {
    actionLists.push_back(sgPair.second->GetSpawnActions());
    actionLists.push_back(sgPair.second->GetDefeatActions());
  }

  for (auto pPair : zone->GetPlasmaSpawns()) {
    actionLists.push_back(pPair.second->GetSuccessActions());
    actionLists.push_back(pPair.second->GetFailActions());
  }

  for (auto spotPair : zone->GetSpots()) {
    actionLists.push_back(spotPair.second->GetActions());
    actionLists.push_back(spotPair.second->GetLeaveActions());
  }

  for (auto trigger : zone->GetTriggers()) {
    actionLists.push_back(trigger->GetActions());
  }

  std::list<std::shared_ptr<objects::Action>> allActions;
  for (auto& actionList : actionLists) {
    for (auto action : includeNested ? GetAllActions(actionList) : actionList) {
      allActions.push_back(action);
    }
  }

  return allActions;
}

std::list<std::shared_ptr<objects::Action>>
ServerDataManager::GetAllZonePartialActions(
    const std::shared_ptr<objects::ServerZonePartial>& partial,
    bool includeNested) {
  std::list<std::list<std::shared_ptr<objects::Action>>> actionLists;

  for (auto npc : partial->GetNPCs()) {
    actionLists.push_back(npc->GetActions());
  }

  for (auto obj : partial->GetObjects()) {
    actionLists.push_back(obj->GetActions());
  }

  for (auto sgPair : partial->GetSpawnGroups()) {
    actionLists.push_back(sgPair.second->GetSpawnActions());
    actionLists.push_back(sgPair.second->GetDefeatActions());
  }

  for (auto pPair : partial->GetPlasmaSpawns()) {
    actionLists.push_back(pPair.second->GetSuccessActions());
    actionLists.push_back(pPair.second->GetFailActions());
  }

  for (auto spotPair : partial->GetSpots()) {
    actionLists.push_back(spotPair.second->GetActions());
    actionLists.push_back(spotPair.second->GetLeaveActions());
  }

  for (auto trigger : partial->GetTriggers()) {
    actionLists.push_back(trigger->GetActions());
  }

  std::list<std::shared_ptr<objects::Action>> allActions;
  for (auto& actionList : actionLists) {
    for (auto action : includeNested ? GetAllActions(actionList) : actionList) {
      allActions.push_back(action);
    }
  }

  return allActions;
}

bool ServerDataManager::LoadScripts(
    gsl::not_null<DataStore*> pDataStore, const libcomp::String& datastorePath,
    std::function<bool(ServerDataManager&, const libcomp::String&,
                       const libcomp::String&)>
        handler) {
  std::list<libcomp::String> files;
  std::list<libcomp::String> dirs;
  std::list<libcomp::String> symLinks;

  (void)pDataStore->GetListing(datastorePath, files, dirs, symLinks, true,
                               true);

  for (auto path : files) {
    if (path.Matches("^.*\\.nut$")) {
      std::vector<char> data = pDataStore->ReadFile(path);
      if (!handler(*this, path, std::string(data.begin(), data.end()))) {
        LogServerDataManagerError([&]() {
          return String("Failed to load script file: %1\n").Arg(path);
        });

        return false;
      }

      LogServerDataManagerInfo(
          [&]() { return String("Loaded script file: %1\n").Arg(path); });
    }
  }

  return true;
}

namespace libcomp {
template <>
BaseScriptEngine& BaseScriptEngine::Using<ServerScript>() {
  if (!BindingExists("ServerScript", true)) {
    Sqrat::Class<ServerScript> binding(mVM, "ServerScript");
    binding.Var("Name", &ServerScript::Name);
    binding.Var("Type", &ServerScript::Type);
    binding.Var("Instantiated", &ServerScript::Instantiated);
    Bind<ServerScript>("ServerScript", binding);
  }

  return *this;
}

}  // namespace libcomp

namespace libhack {

template <>
bool ServerDataManager::LoadObject<objects::ServerZone>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  auto zone = std::shared_ptr<objects::ServerZone>(new objects::ServerZone);
  if (!zone->Load(doc, *objNode)) {
    return false;
  }

  auto id = zone->GetID();
  auto dynamicMapID = zone->GetDynamicMapID();

  libcomp::String zoneStr = libcomp::String("%1%2").Arg(id).Arg(
      id != dynamicMapID ? libcomp::String(" (%1)").Arg(dynamicMapID) : "");

  bool isField = false;
  if (definitionManager) {
    auto def = definitionManager->GetZoneData(id);
    if (!def) {
      LogServerDataManagerWarning(
          [&]() { return String("Skipping unknown zone: %1\n").Arg(zoneStr); });

      return true;
    }

    isField =
        def->GetBasic()->GetType() == objects::MiZoneBasicData::Type_t::FIELD;
  }

  if (mZoneData.find(id) != mZoneData.end() &&
      mZoneData[id].find(dynamicMapID) != mZoneData[id].end()) {
    LogServerDataManagerError([&]() {
      return String("Duplicate zone encountered: %1\n").Arg(zoneStr);
    });

    return false;
  }

  // Make sure spawns are valid
  if (definitionManager) {
    for (auto sPair : zone->GetSpawns()) {
      if (definitionManager->GetDevilData(sPair.second->GetEnemyType()) ==
          nullptr) {
        LogServerDataManagerError([&]() {
          return String("Invalid spawn enemy type encountered in zone %1: %2\n")
              .Arg(zoneStr)
              .Arg(sPair.second->GetEnemyType());
        });

        return false;
      } else if (sPair.second->GetBossGroup() &&
                 sPair.second->GetCategory() !=
                     objects::Spawn::Category_t::BOSS) {
        LogServerDataManagerError([&]() {
          return String("Invalid spawn boss group encountered in zone %1: %2\n")
              .Arg(zoneStr)
              .Arg(sPair.first);
        });

        return false;
      }
    }
  }

  for (auto sgPair : zone->GetSpawnGroups()) {
    auto sg = sgPair.second;

    for (auto sPair : sg->GetSpawns()) {
      if (!zone->SpawnsKeyExists(sPair.first)) {
        LogServerDataManagerError([&]() {
          return String(
                     "Invalid spawn group spawn ID encountered in zone %1: "
                     "%2\n")
              .Arg(zoneStr)
              .Arg(sPair.first);
        });

        return false;
      }
    }

    if (!ValidateActions(sg->GetDefeatActions(),
                         libcomp::String("Zone %1, SG %2 Defeat")
                             .Arg(zoneStr)
                             .Arg(sg->GetID()),
                         false) ||
        !ValidateActions(sg->GetSpawnActions(),
                         libcomp::String("Zone %1, SG %2 Spawn")
                             .Arg(zoneStr)
                             .Arg(sg->GetID()),
                         false)) {
      return false;
    }
  }

  for (auto slgPair : zone->GetSpawnLocationGroups()) {
    for (uint32_t sgID : slgPair.second->GetGroupIDs()) {
      if (!zone->SpawnGroupsKeyExists(sgID)) {
        LogServerDataManagerError([&]() {
          return String(
                     "Invalid spawn location group spawn group ID encountered "
                     "in zone %1: %2\n")
              .Arg(zoneStr)
              .Arg(sgID);
        });

        return false;
      }
    }
  }

  mZoneData[id][dynamicMapID] = zone;

  if (isField) {
    mFieldZoneIDs.push_back(std::pair<uint32_t, uint32_t>(id, dynamicMapID));
  }

  for (auto npc : zone->GetNPCs()) {
    if (!ValidateActions(
            npc->GetActions(),
            libcomp::String("Zone %1, NPC %2").Arg(zoneStr).Arg(npc->GetID()),
            false)) {
      return false;
    }
  }

  for (auto obj : zone->GetObjects()) {
    if (!ValidateActions(obj->GetActions(),
                         libcomp::String("Zone %1, Object %2")
                             .Arg(zoneStr)
                             .Arg(obj->GetID()),
                         false)) {
      return false;
    }
  }

  for (auto& pPair : zone->GetPlasmaSpawns()) {
    auto plasma = pPair.second;
    if (!ValidateActions(
            plasma->GetSuccessActions(),
            libcomp::String("Zone %1, Plasma %2").Arg(zoneStr).Arg(pPair.first),
            false) ||
        !ValidateActions(
            plasma->GetFailActions(),
            libcomp::String("Zone %1, Plasma %2").Arg(zoneStr).Arg(pPair.first),
            false)) {
      return false;
    }
  }

  for (auto& spotPair : zone->GetSpots()) {
    auto spot = spotPair.second;
    if (!ValidateActions(spot->GetActions(),
                         libcomp::String("Zone %1, Spot %2")
                             .Arg(zoneStr)
                             .Arg(spotPair.first),
                         false) ||
        !ValidateActions(spot->GetLeaveActions(),
                         libcomp::String("Zone %1, Spot %2")
                             .Arg(zoneStr)
                             .Arg(spotPair.first),
                         false)) {
      return false;
    }
  }

  for (auto t : zone->GetTriggers()) {
    if (!ValidateActions(t->GetActions(),
                         libcomp::String("Zone %1 trigger").Arg(zoneStr),
                         TriggerIsAutoContext(t))) {
      return false;
    }
  }

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::ServerZonePartial>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto prt = std::shared_ptr<objects::ServerZonePartial>(
      new objects::ServerZonePartial);
  if (!prt->Load(doc, *objNode)) {
    return false;
  }

  auto id = prt->GetID();
  if (mZonePartialData.find(id) != mZonePartialData.end()) {
    LogServerDataManagerError([&]() {
      return String("Duplicate zone partial encountered: %1\n").Arg(id);
    });

    return false;
  }

  if (id == 0) {
    // Warn about any unsupported parts (spawns are used for global
    // spawn skills)
    if (prt->DynamicMapIDsCount() || prt->NPCsCount() || prt->ObjectsCount() ||
        prt->SpotsCount()) {
      LogServerDataManagerWarningMsg(
          "Direct global partial zone definitions specified but will be "
          "ignored\n");
    }
  } else {
    // Make sure spawns are valid
    if (definitionManager) {
      for (auto sPair : prt->GetSpawns()) {
        // Enemy type can be zero if appending sub-lists
        auto enemyType = sPair.second->GetEnemyType();
        if (enemyType &&
            definitionManager->GetDevilData(enemyType) == nullptr) {
          LogServerDataManagerError([&]() {
            return String(
                       "Invalid spawn enemy type encountered in zone partial "
                       "%1: %2\n")
                .Arg(id)
                .Arg(enemyType);
          });

          return false;
        } else if (sPair.second->GetBossGroup() &&
                   sPair.second->GetCategory() !=
                       objects::Spawn::Category_t::BOSS) {
          LogServerDataManagerError([&]() {
            return String(
                       "Invalid spawn boss group encountered in zone paritial "
                       "%1: %2\n")
                .Arg(id)
                .Arg(sPair.first);
          });

          return false;
        }
      }
    }

    if (prt->GetAutoApply()) {
      for (uint32_t dynamicMapID : prt->GetDynamicMapIDs()) {
        mZonePartialMap[dynamicMapID].insert(id);
      }
    }
  }

  mZonePartialData[id] = prt;

  for (auto sgPair : prt->GetSpawnGroups()) {
    auto sg = sgPair.second;
    if (!ValidateActions(sg->GetDefeatActions(),
                         libcomp::String("Partial %1, SG %2 Defeat")
                             .Arg(id)
                             .Arg(sg->GetID()),
                         false) ||
        !ValidateActions(
            sg->GetSpawnActions(),
            libcomp::String("Partial %1, SG %2 Spawn").Arg(id).Arg(sg->GetID()),
            false)) {
      return false;
    }
  }

  for (auto npc : prt->GetNPCs()) {
    if (!ValidateActions(
            npc->GetActions(),
            libcomp::String("Partial %1, NPC %2").Arg(id).Arg(npc->GetID()),
            false)) {
      return false;
    }
  }

  for (auto obj : prt->GetObjects()) {
    if (!ValidateActions(
            obj->GetActions(),
            libcomp::String("Partial %1, Object %2").Arg(id).Arg(obj->GetID()),
            false)) {
      return false;
    }
  }

  for (auto& spotPair : prt->GetSpots()) {
    auto spot = spotPair.second;
    if (!ValidateActions(
            spot->GetActions(),
            libcomp::String("Partial %1, Spot %2").Arg(id).Arg(spotPair.first),
            false) ||
        !ValidateActions(
            spot->GetLeaveActions(),
            libcomp::String("Partial %1, Spot %2").Arg(id).Arg(spotPair.first),
            false)) {
      return false;
    }
  }

  for (auto t : prt->GetTriggers()) {
    if (!ValidateActions(t->GetActions(),
                         libcomp::String("Partial %1 trigger").Arg(id),
                         TriggerIsAutoContext(t))) {
      return false;
    }
  }

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::Event>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto event =
      objects::Event::InheritedConstruction(objNode->Attribute("name"));
  if (event == nullptr || !event->Load(doc, *objNode)) {
    return false;
  }

  if (event->GetID().IsEmpty()) {
    LogServerDataManagerErrorMsg("Event with no ID encountered\n");

    return false;
  }

  auto id = std::string(event->GetID().C());
  if (mEventData.find(id) != mEventData.end()) {
    LogServerDataManagerError(
        [&]() { return String("Duplicate event encountered: %1\n").Arg(id); });

    return false;
  }

  mEventData[id] = event;

  if (event->GetEventType() == objects::Event::EventType_t::PERFORM_ACTIONS) {
    auto e = std::dynamic_pointer_cast<objects::EventPerformActions>(event);
    if (e) {
      if (!ValidateActions(e->GetActions(), e->GetID(), false, true)) {
        return false;
      }
    }
  }

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::ServerZoneInstance>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  auto inst = std::shared_ptr<objects::ServerZoneInstance>(
      new objects::ServerZoneInstance);
  if (!inst->Load(doc, *objNode)) {
    return false;
  }

  auto id = inst->GetID();
  if (definitionManager &&
      !definitionManager->GetZoneData(inst->GetLobbyID())) {
    LogServerDataManagerWarning([&]() {
      return String("Skipping zone instance %1 with unknown lobby %2\n")
          .Arg(inst->GetID())
          .Arg(inst->GetLobbyID());
    });

    return true;
  }

  // Zone and dynamic map IDs should be parallel lists
  size_t zoneIDCount = inst->ZoneIDsCount();
  if (zoneIDCount != inst->DynamicMapIDsCount()) {
    LogServerDataManagerError([&]() {
      return libcomp::String(
                 "Zone instance %1 encountered with zone and dynamic map "
                 "counts that do not match\n")
          .Arg(inst->GetID());
    });

    return false;
  }

  for (size_t i = 0; i < zoneIDCount; i++) {
    uint32_t zoneID = inst->GetZoneIDs(i);
    uint32_t dynamicMapID = inst->GetDynamicMapIDs(i);

    if (mZoneData.find(zoneID) == mZoneData.end() ||
        mZoneData[zoneID].find(dynamicMapID) == mZoneData[zoneID].end()) {
      LogServerDataManagerError([&]() {
        return String("Invalid zone encountered for instance: %1 (%2)\n")
            .Arg(zoneID)
            .Arg(dynamicMapID);
      });

      return false;
    }
  }

  if (mZoneInstanceData.find(id) != mZoneInstanceData.end()) {
    LogServerDataManagerError([&]() {
      return String("Duplicate zone instance encountered: %1\n").Arg(id);
    });

    return false;
  }

  mZoneInstanceData[id] = inst;

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::ServerZoneInstanceVariant>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto variant = objects::ServerZoneInstanceVariant::InheritedConstruction(
      objNode->Attribute("name"));
  if (!variant->Load(doc, *objNode)) {
    return false;
  }

  auto id = variant->GetID();
  if (mZoneInstanceVariantData.find(id) != mZoneInstanceVariantData.end()) {
    LogServerDataManagerError([&]() {
      return String("Duplicate zone instance variant encountered: %1\n")
          .Arg(id);
    });

    return false;
  }

  size_t timeCount = variant->TimePointsCount();
  switch (variant->GetInstanceType()) {
    case objects::ServerZoneInstanceVariant::InstanceType_t::TIME_TRIAL:
      if (timeCount != 4) {
        LogServerDataManagerError([&]() {
          return String(
                     "Time trial zone instance variant encountered without 4 "
                     "time points specified: %1\n")
              .Arg(id);
        });

        return false;
      }
      break;
    case objects::ServerZoneInstanceVariant::InstanceType_t::PVP:
      if (timeCount != 2 && timeCount != 3) {
        LogServerDataManagerError([&]() {
          return String(
                     "PVP zone instance variant encountered without 2 or 3 "
                     "time points specified: %1\n")
              .Arg(id);
        });

        return false;
      }
      break;
    case objects::ServerZoneInstanceVariant::InstanceType_t::DEMON_ONLY:
      if (timeCount != 3 && timeCount != 4) {
        LogServerDataManagerError([&]() {
          return String(
                     "Demon only zone instance variant encountered without 3 "
                     "or 4 time points specified: %1\n")
              .Arg(id);
        });

        return false;
      }
      break;
    case objects::ServerZoneInstanceVariant::InstanceType_t::DIASPORA:
      if (timeCount != 2) {
        LogServerDataManagerError([&]() {
          return String(
                     "Diaspora zone instance variant encountered without 2 "
                     "time points specified: %1\n")
              .Arg(id);
        });

        return false;
      }
      break;
    case objects::ServerZoneInstanceVariant::InstanceType_t::MISSION:
      if (timeCount != 1) {
        LogServerDataManagerError([&]() {
          return String(
                     "Mission zone instance variant encountered without time "
                     "point specified: %1\n")
              .Arg(id);
        });

        return false;
      }
      break;
    case objects::ServerZoneInstanceVariant::InstanceType_t::PENTALPHA:
      if (variant->GetSubID() >= 5) {
        LogServerDataManagerError([&]() {
          return String(
                     "Pentalpha zone instance variant encountered with invalid "
                     "sub ID: %1\n")
              .Arg(id);
        });

        return false;
      }
      break;
    default:
      break;
  }

  auto pvpVar = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(variant);
  if (pvpVar) {
    if (definitionManager && pvpVar->GetDefaultInstanceID() &&
        !VerifyPvPInstance(pvpVar->GetDefaultInstanceID(), definitionManager)) {
      return false;
    }

    if (!pvpVar->GetSpecialMode() &&
        pvpVar->GetMatchType() !=
            objects::PvPInstanceVariant::MatchType_t::CUSTOM) {
      mStandardPvPVariantIDs[(uint8_t)pvpVar->GetMatchType()].insert(id);
    }
  }

  mZoneInstanceVariantData[id] = variant;

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::ServerShop>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto shop = std::shared_ptr<objects::ServerShop>(new objects::ServerShop);
  if (!shop->Load(doc, *objNode)) {
    return false;
  }

  uint32_t id = (uint32_t)shop->GetShopID();
  if (mShopData.find(id) != mShopData.end()) {
    LogServerDataManagerError(
        [&]() { return String("Duplicate shop encountered: %1\n").Arg(id); });

    return false;
  }

  // Tab count cannot exceed max s8, apply lower arbitrary limit
  if (shop->TabsCount() > 100) {
    LogServerDataManagerError([&]() {
      return String("Shop with more than 100 tabs encountered: %1\n").Arg(id);
    });

    return false;
  }

  mShopData[id] = shop;

  if (shop->GetType() == objects::ServerShop::Type_t::COMP_SHOP) {
    mCompShopIDs.push_back(id);
  }

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::AILogicGroup>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto grp = std::shared_ptr<objects::AILogicGroup>(new objects::AILogicGroup);
  if (!grp->Load(doc, *objNode)) {
    return false;
  }

  uint16_t id = grp->GetID();
  if (mAILogicGroups.find(id) != mAILogicGroups.end()) {
    LogServerDataManagerError([&]() {
      return String("Duplicate AI logic group entry encountered: %1\n").Arg(id);
    });

    return false;
  }

  mAILogicGroups[id] = grp;

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::DemonFamiliarityType>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto fType = std::make_shared<objects::DemonFamiliarityType>();
  if (!fType->Load(doc, *objNode)) {
    return false;
  }

  int32_t id = fType->GetID();
  if (mDemonFamiliarityTypeData.find(id) != mDemonFamiliarityTypeData.end()) {
    LogServerDataManagerError([&]() {
      return String("Duplicate demon familiarity type entry encountered: %1\n")
          .Arg(id);
    });

    return false;
  }

  mDemonFamiliarityTypeData[id] = fType;

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::DemonPresent>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto present =
      std::shared_ptr<objects::DemonPresent>(new objects::DemonPresent);
  if (!present->Load(doc, *objNode)) {
    return false;
  }

  uint32_t id = present->GetID();
  if (mDemonPresentData.find(id) != mDemonPresentData.end()) {
    LogServerDataManagerError([&]() {
      return String("Duplicate demon present entry encountered: %1\n").Arg(id);
    });

    return false;
  }

  mDemonPresentData[id] = present;

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::DemonQuestReward>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto reward =
      std::shared_ptr<objects::DemonQuestReward>(new objects::DemonQuestReward);
  if (!reward->Load(doc, *objNode)) {
    return false;
  }

  uint32_t id = reward->GetID();
  if (mDemonQuestRewardData.find(id) != mDemonQuestRewardData.end()) {
    LogServerDataManagerError([&]() {
      return String("Duplicate demon quest reward entry encountered: %1\n")
          .Arg(id);
    });

    return false;
  }

  mDemonQuestRewardData[id] = reward;

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::DropSet>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto dropSet = std::shared_ptr<objects::DropSet>(new objects::DropSet);
  if (!dropSet->Load(doc, *objNode)) {
    return false;
  }

  uint32_t id = dropSet->GetID();

  if (dropSet->GetType() == objects::DropSet::Type_t::REDEFINE) {
    // hold onto redefinitions until the actual dropset loads
    mRedefineDropSetData[id] = dropSet;
  } else if (dropSet->GetType() == objects::DropSet::Type_t::APPEND) {
    // Hold onto the drops until the actual dropset loads
    for (auto drop : dropSet->GetDrops()) {
      mPendingMergeDrops[id].push_back(drop);
    }
  } else {
    uint32_t giftBoxID = dropSet->GetGiftBoxID();
    if (mDropSetData.find(id) != mDropSetData.end()) {
      LogServerDataManagerError([id]() {
        return String("Duplicate drop set encountered: %1\n").Arg(id);
      });

      return false;
    }

    if (giftBoxID) {
      if (mGiftDropSetLookup.find(giftBoxID) != mGiftDropSetLookup.end()) {
        LogServerDataManagerError([giftBoxID]() {
          return String("Duplicate drop set gift box ID encountered: %1\n")
              .Arg(giftBoxID);
        });

        return false;
      }

      mGiftDropSetLookup[giftBoxID] = id;
    }

    mDropSetData[id] = dropSet;
  }

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::EnchantSetData>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  auto eSet =
      std::shared_ptr<objects::EnchantSetData>(new objects::EnchantSetData);
  if (!eSet->Load(doc, *objNode)) {
    return false;
  }

  return definitionManager &&
         definitionManager->RegisterServerSideDefinition(eSet);
}

template <>
bool ServerDataManager::LoadObject<objects::EnchantSpecialData>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  auto eSpecial = std::shared_ptr<objects::EnchantSpecialData>(
      new objects::EnchantSpecialData);
  if (!eSpecial->Load(doc, *objNode)) {
    return false;
  }

  return definitionManager &&
         definitionManager->RegisterServerSideDefinition(eSpecial);
}

template <>
bool ServerDataManager::LoadObject<objects::FusionMistake>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  auto mistake =
      std::shared_ptr<objects::FusionMistake>(new objects::FusionMistake);
  if (!mistake->Load(doc, *objNode)) {
    return false;
  }

  uint32_t id = mistake->GetID();
  if (mFusionMistakeData.find(id) != mFusionMistakeData.end()) {
    LogServerDataManagerError([id]() {
      return String("Duplicate fusion mistake entry encountered: %1\n").Arg(id);
    });

    return false;
  } else if (mistake->ResultRaceIDsCount() == 0 &&
             mistake->ResultTypesCount() == 0) {
    LogServerDataManagerError([id]() {
      return String("Fusion mistake entry with no result encountered: %1\n")
          .Arg(id);
    });

    return false;
  }

  if (definitionManager) {
    for (auto type : mistake->GetResultTypes()) {
      if (!definitionManager->GetDevilData(type)) {
        LogServerDataManagerError([type, id]() {
          return String(
                     "Invalid result type %1 encountered on fusion mistake: "
                     "%2\n")
              .Arg(type)
              .Arg(id);
        });

        return false;
      }
    }
  }

  if ((!mistake->GetOnFailure() && !mistake->GetOnMaxSuccess() &&
       !mistake->GetOnSuccess() && !mistake->GetOnZeroFailure()) ||
      (!mistake->GetSourceAny() && mistake->SourceRaceIDsCount() > 3) ||
      (!mistake->GetSourceAny() && mistake->SourceTypesCount() > 3)) {
    LogServerDataManagerWarning([id]() {
      return String("Inaccssible fusion mistake definition encountered: %1\n")
          .Arg(id);
    });
  }

  mFusionMistakeData[id] = mistake;

  return true;
}

template <>
bool ServerDataManager::LoadObject<objects::MiSItemData>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  auto sItem = std::shared_ptr<objects::MiSItemData>(new objects::MiSItemData);
  if (!sItem->Load(doc, *objNode)) {
    return false;
  }

  return definitionManager &&
         definitionManager->RegisterServerSideDefinition(sItem);
}

template <>
bool ServerDataManager::LoadObject<objects::MiSStatusData>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  auto sStatus =
      std::shared_ptr<objects::MiSStatusData>(new objects::MiSStatusData);
  if (!sStatus->Load(doc, *objNode)) {
    return false;
  }

  return definitionManager &&
         definitionManager->RegisterServerSideDefinition(sStatus);
}

template <>
bool ServerDataManager::LoadObject<objects::Tokusei>(
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement* objNode,
    DefinitionManager* definitionManager) {
  auto tokusei = std::shared_ptr<objects::Tokusei>(new objects::Tokusei);
  if (!tokusei->Load(doc, *objNode)) {
    return false;
  }

  return definitionManager &&
         definitionManager->RegisterServerSideDefinition(tokusei);
}
}  // namespace libhack

bool ServerDataManager::LoadScript(const libcomp::String& path,
                                   const libcomp::String& source) {
  ScriptEngine engine;
  engine.Using<ServerScript>();
  if (!engine.Eval(source)) {
    LogServerDataManagerError([&]() {
      return String("Improperly formatted script encountered: %1\n").Arg(path);
    });

    return false;
  }

  auto root = Sqrat::RootTable(engine.GetVM());
  auto fDef = root.GetFunction("define");
  if (fDef.IsNull()) {
    LogServerDataManagerError(
        [&]() { return String("Invalid script encountered: %1\n").Arg(path); });

    return false;
  }

  auto script = std::make_shared<ServerScript>();
  auto result = fDef.Evaluate<int>(script);
  if (!result || *result != 0 || script->Name.IsEmpty() ||
      script->Type.IsEmpty()) {
    LogServerDataManagerError([&]() {
      return String("Script is not properly defined: %1\n").Arg(path);
    });

    return false;
  }

  script->Path = path;
  script->Source = source;

  if (script->Type.ToLower() == "ai") {
    if (mAIScripts.find(script->Name.C()) != mAIScripts.end()) {
      LogServerDataManagerError([&]() {
        return String("Duplicate AI script encountered: %1\n")
            .Arg(script->Name.C());
      });

      return false;
    }

    fDef = root.GetFunction("prepare");
    if (fDef.IsNull()) {
      LogServerDataManagerError([&]() {
        return String("AI script encountered with no 'prepare' function: %1\n")
            .Arg(script->Name.C());
      });

      return false;
    }

    mAIScripts[script->Name.C()] = script;
  } else {
    if (mScripts.find(script->Name.C()) != mScripts.end()) {
      LogServerDataManagerError([&]() {
        return String("Duplicate script encountered: %1\n")
            .Arg(script->Name.C());
      });

      return false;
    }

    // Check supported types here
    auto type = script->Type.ToLower();
    if (type == "eventcondition" || type == "eventbranchlogic") {
      fDef = root.GetFunction("check");
      if (fDef.IsNull()) {
        LogServerDataManagerError([&]() {
          return String(
                     "Event conditional script encountered with no 'check' "
                     "function: %1\n")
              .Arg(script->Name.C());
        });

        return false;
      }
    } else if (type == "actiontransform" || type == "eventtransform") {
      fDef = root.GetFunction("transform");
      if (fDef.IsNull()) {
        LogServerDataManagerError([&]() {
          return String(
                     "Transform script encountered with no 'transform' "
                     "function: %1\n")
              .Arg(script->Name.C());
        });

        return false;
      }

      fDef = root.GetFunction("prepare");
      if (!fDef.IsNull()) {
        LogServerDataManagerError([&]() {
          return String(
                     "Transform script encountered with reserved function name "
                     "'prepare': %1\n")
              .Arg(script->Name.C());
        });

        return false;
      }
    } else if (type == "actioncustom") {
      fDef = root.GetFunction("run");
      if (fDef.IsNull()) {
        LogServerDataManagerError([&]() {
          return String(
                     "Custom action script encountered with no 'run' function: "
                     "%1\n")
              .Arg(script->Name.C());
        });

        return false;
      }
    } else if (type == "skilllogic") {
      fDef = root.GetFunction("prepare");
      if (fDef.IsNull()) {
        LogServerDataManagerError([&]() {
          return String(
                     "Skill logic script encountered with no 'prepare' "
                     "function: %1\n")
              .Arg(script->Name.C());
        });

        return false;
      }
    } else if (type == "webapp") {
      fDef = root.GetFunction("prepare");
      if (fDef.IsNull()) {
        LogServerDataManagerError([&]() {
          return String(
                     "Web app script encountered with no 'prepare' function: "
                     "%1\n")
              .Arg(script->Name.C());
        });

        return false;
      }
    } else if (type == "webgame") {
      fDef = root.GetFunction("start");
      if (fDef.IsNull()) {
        LogServerDataManagerError([&]() {
          return String(
                     "Web game script encountered with no 'start' function: "
                     "%1\n")
              .Arg(script->Name.C());
        });

        return false;
      }
    } else {
      LogServerDataManagerError([&]() {
        return String("Invalid script type encountered: %1\n")
            .Arg(script->Type.C());
      });

      return false;
    }

    mScripts[script->Name.C()] = script;
  }

  return true;
}

void ServerDataManager::ApplyPendingDrops() {
  // Redefine dropsets first
  for (auto& pair : mRedefineDropSetData) {
    auto id = pair.first;

    auto it = mDropSetData.find(id);
    if (it == mDropSetData.end()) {
      LogServerDataManagerWarning([id]() {
        return String(
                   "Skipping redefined drop set for an ID that has not been "
                   "loaded: %1\n")
            .Arg(id);
      });
    } else {
      LogServerDataManagerDebug([id]() {
        return String("Redefining drops on drop set: %1\n").Arg(id);
      });

      it->second->SetDrops(pair.second->GetDrops());
    }
  }

  // Now append new drops
  for (auto& pair : mPendingMergeDrops) {
    auto it = mDropSetData.find(pair.first);
    if (it != mDropSetData.end()) {
      LogServerDataManagerDebug([pair]() {
        return String("Appending %1 drop(s) to drop set %2\n")
            .Arg(pair.second.size())
            .Arg(pair.first);
      });

      for (auto drop : pair.second) {
        it->second->AppendDrops(drop);
      }
    } else {
      LogServerDataManagerWarning([pair]() {
        return String("Failed to append drops to unknown drop set %1\n")
            .Arg(pair.first);
      });
    }
  }

  mRedefineDropSetData.clear();
  mPendingMergeDrops.clear();
}

bool ServerDataManager::ValidateActions(
    const std::list<std::shared_ptr<objects::Action>>& actions,
    const libcomp::String& source, bool autoContext, bool inEvent) {
  size_t current = 0;
  size_t count = actions.size();
  for (auto action : actions) {
    current++;

    if (current != count && !inEvent) {
      bool warn = false;
      switch (action->GetActionType()) {
        case objects::Action::ActionType_t::ZONE_CHANGE: {
          auto act =
              std::dynamic_pointer_cast<objects::ActionZoneChange>(action);
          warn = act && act->GetZoneID() != 0;
        } break;
        case objects::Action::ActionType_t::ZONE_INSTANCE: {
          auto act =
              std::dynamic_pointer_cast<objects::ActionZoneInstance>(action);
          if (act) {
            switch (act->GetMode()) {
              case objects::ActionZoneInstance::Mode_t::JOIN:
              case objects::ActionZoneInstance::Mode_t::CLAN_JOIN:
              case objects::ActionZoneInstance::Mode_t::TEAM_JOIN:
              case objects::ActionZoneInstance::Mode_t::TEAM_PVP:
                warn = true;
                break;
              default:
                break;
            }
          }
        } break;
        default:
          break;
      }

      if (warn) {
        LogServerDataManagerWarning([&]() {
          return String(
                     "Zone change action encountered mid-action set in a "
                     "context outside of an event. This can cause unexpected "
                     "behavior for multi-channel setups. Move to the end of "
                     "the set to avoid errors: %1\n")
              .Arg(source);
        });
      }
    }

    bool autoCtx =
        autoContext && (action->GetSourceContext() ==
                            objects::Action::SourceContext_t::ENEMIES ||
                        action->GetSourceContext() ==
                            objects::Action::SourceContext_t::SOURCE);
    switch (action->GetActionType()) {
      case objects::Action::ActionType_t::DELAY: {
        auto act = std::dynamic_pointer_cast<objects::ActionDelay>(action);
        if (!ValidateActions(act->GetActions(),
                             libcomp::String("%1 => Delay Actions").Arg(source),
                             autoCtx)) {
          return false;
        }
      } break;
      case objects::Action::ActionType_t::SPAWN: {
        auto act = std::dynamic_pointer_cast<objects::ActionSpawn>(action);
        if (!ValidateActions(
                act->GetDefeatActions(),
                libcomp::String("%1 => Defeat Actions").Arg(source), autoCtx)) {
          return false;
        }
      } break;
      case objects::Action::ActionType_t::ADD_REMOVE_ITEMS:
      case objects::Action::ActionType_t::DISPLAY_MESSAGE:
      case objects::Action::ActionType_t::GRANT_SKILLS:
      case objects::Action::ActionType_t::GRANT_XP:
      case objects::Action::ActionType_t::PLAY_BGM:
      case objects::Action::ActionType_t::PLAY_SOUND_EFFECT:
      case objects::Action::ActionType_t::SET_HOMEPOINT:
      case objects::Action::ActionType_t::SPECIAL_DIRECTION:
      case objects::Action::ActionType_t::STAGE_EFFECT:
      case objects::Action::ActionType_t::UPDATE_COMP:
      case objects::Action::ActionType_t::UPDATE_FLAG:
      case objects::Action::ActionType_t::UPDATE_LNC:
      case objects::Action::ActionType_t::UPDATE_QUEST:
      case objects::Action::ActionType_t::ZONE_CHANGE:
      case objects::Action::ActionType_t::ZONE_INSTANCE:
        if (autoCtx) {
          LogServerDataManagerError([&]() {
            return String(
                       "Non-player context with player required action type %1 "
                       "encountered: %2\n")
                .Arg((int32_t)action->GetActionType())
                .Arg(source);
          });

          return false;
        }
        break;
      case objects::Action::ActionType_t::ADD_REMOVE_STATUS:
      case objects::Action::ActionType_t::CREATE_LOOT:
      case objects::Action::ActionType_t::RUN_SCRIPT:
      case objects::Action::ActionType_t::SET_NPC_STATE:
      case objects::Action::ActionType_t::START_EVENT:
      case objects::Action::ActionType_t::UPDATE_POINTS:
      case objects::Action::ActionType_t::UPDATE_ZONE_FLAGS:
      default:
        // Nothing special needed
        break;
    }
  }

  return true;
}

std::list<libcomp::String> ServerDataManager::GetInvalidEventIDs(
    const std::list<std::shared_ptr<objects::Action>>& actions) const {
  std::list<libcomp::String> invalidRefs;
  for (auto& action : actions) {
    switch (action->GetActionType()) {
      case objects::Action::ActionType_t::START_EVENT: {
        auto act = std::dynamic_pointer_cast<objects::ActionStartEvent>(action);
        if (act && !act->GetEventID().IsEmpty() &&
            mEventData.find(act->GetEventID().C()) == mEventData.end()) {
          invalidRefs.push_back(act->GetEventID());
        }
      } break;
      case objects::Action::ActionType_t::ZONE_INSTANCE: {
        auto act =
            std::dynamic_pointer_cast<objects::ActionZoneInstance>(action);
        if (act && !act->GetTimerExpirationEventID().IsEmpty() &&
            mEventData.find(act->GetTimerExpirationEventID().C()) ==
                mEventData.end()) {
          invalidRefs.push_back(act->GetTimerExpirationEventID());
        }
      } break;
      default:
        break;
    }

    if (!action->GetOnFailureEvent().IsEmpty() &&
        mEventData.find(action->GetOnFailureEvent().C()) == mEventData.end()) {
      invalidRefs.push_back(action->GetOnFailureEvent());
    }
  }

  return invalidRefs;
}

bool ServerDataManager::TriggerIsAutoContext(
    const std::shared_ptr<objects::ServerZoneTrigger>& trigger) {
  // Most triggers use auto-only contexts
  switch (trigger->GetTrigger()) {
    case objects::ServerZoneTrigger::Trigger_t::ON_DEATH:
    case objects::ServerZoneTrigger::Trigger_t::ON_DIASPORA_BASE_CAPTURE:
    case objects::ServerZoneTrigger::Trigger_t::ON_FLAG_SET:
    case objects::ServerZoneTrigger::Trigger_t::ON_LOGIN:
    case objects::ServerZoneTrigger::Trigger_t::ON_PVP_BASE_CAPTURE:
    case objects::ServerZoneTrigger::Trigger_t::ON_PVP_COMPLETE:
    case objects::ServerZoneTrigger::Trigger_t::ON_REVIVAL:
    case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_IN:
    case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_OUT:
      return false;
    default:
      return true;
  }
}

namespace libcomp {
template <>
BaseScriptEngine& BaseScriptEngine::Using<ServerDataManager>() {
  if (!BindingExists("ServerDataManager")) {
    Sqrat::Class<ServerDataManager> binding(mVM, "ServerDataManager");
    Bind<ServerDataManager>("ServerDataManager", binding);

    // These are needed for some methods.
    Using<DefinitionManager>();

    binding.Func("LoadData",
                 &ServerDataManager::LoadData);  // Last call to binding
  }

  return *this;
}
}  // namespace libcomp

#endif  // !EXOTIC_PLATFORM
