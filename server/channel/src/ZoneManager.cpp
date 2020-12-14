/**
 * @file server/channel/src/ZoneManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages zone instance objects and connections.
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

#include "ZoneManager.h"

// libcomp Includes
#include <Constants.h>
#include <DefinitionManager.h>
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ScriptEngine.h>
#include <ServerConstants.h>
#include <ServerDataManager.h>

// objects Include
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <ActionSpawn.h>
#include <ActionStartEvent.h>
#include <ActivatedAbility.h>
#include <Ally.h>
#include <ChannelLogin.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <DestinyBox.h>
#include <DiasporaBase.h>
#include <DigitalizeState.h>
#include <Enemy.h>
#include <EnemyExtension.h>
#include <EntityStats.h>
#include <InstanceAccess.h>
#include <Item.h>
#include <ItemDrop.h>
#include <Loot.h>
#include <LootBox.h>
#include <Match.h>
#include <MiAIData.h>
#include <MiDevilData.h>
#include <MiDynamicMapData.h>
#include <MiGrowthData.h>
#include <MiONPCData.h>
#include <MiSpotData.h>
#include <MiTimeLimitData.h>
#include <MiUraFieldTowerData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <MiZoneFileData.h>
#include <Party.h>
#include <PentalphaEntry.h>
#include <PlasmaSpawn.h>
#include <PlayerExchangeSession.h>
#include <PvPBase.h>
#include <PvPInstanceStats.h>
#include <PvPInstanceVariant.h>
#include <PvPMatch.h>
#include <QmpElement.h>
#include <QmpNavPoint.h>
#include <ServerBazaar.h>
#include <ServerCultureMachineSet.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <ServerZoneInstanceVariant.h>
#include <ServerZonePartial.h>
#include <ServerZoneSpot.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocation.h>
#include <SpawnLocationGroup.h>
#include <SpawnRestriction.h>
#include <Team.h>
#include <UBMatch.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "AIManager.h"
#include "AIState.h"
#include "AccountManager.h"
#include "ActionManager.h"
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"
#include "CultureMachineState.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "MatchManager.h"
#include "PerformanceTimer.h"
#include "PlasmaState.h"
#include "SkillManager.h"
#include "TokuseiManager.h"
#include "Zone.h"
#include "ZoneGeometryLoader.h"
#include "ZoneInstance.h"

// C++ Standard Includes
#include <cmath>

using namespace channel;

namespace libcomp {
template <>
ScriptEngine& ScriptEngine::Using<ZoneManager>() {
  if (!BindingExists("ZoneManager", true)) {
    Using<Zone>();

    Sqrat::Class<ZoneManager, Sqrat::NoConstructor<ZoneManager>> binding(
        mVM, "ZoneManager");
    binding.Func("GetGlobalZone", &ZoneManager::GetGlobalZone)
        .Func("GetExistingZone", &ZoneManager::GetExistingZone)
        .Func("GetInstanceStartingZone", &ZoneManager::GetInstanceStartingZone)
        .Func("CopyToEnemy", &ZoneManager::CopyToEnemy)
        .Func("CopyDemon", &ZoneManager::CopyDemon)
        .Func("CreateAlly", &ZoneManager::CreateAlly)
        .Func<std::shared_ptr<ActiveEntityState> (ZoneManager::*)(
            const std::shared_ptr<Zone>&, uint32_t, uint32_t, uint32_t, float,
            float, float, const libobjgen::UUID&)>("CreateEnemy",
                                                   &ZoneManager::CreateEnemy)
        .Func<bool (ZoneManager::*)(
            std::list<std::shared_ptr<ActiveEntityState>>,
            const std::shared_ptr<Zone>&, bool, bool, const libcomp::String&)>(
            "AddEnemiesToZone", &ZoneManager::AddEnemiesToZone)
        .Func("LinearReposition", &ZoneManager::LinearReposition)
        .Func("StartZoneEvent", &ZoneManager::StartZoneEvent);

    Bind<ZoneManager>("ZoneManager", binding);
  }

  return *this;
}
}  // namespace libcomp

ZoneManager::ZoneManager(const std::weak_ptr<ChannelServer>& server)
    : mTrackingRefresh(0),
      mNextZoneID(1),
      mNextZoneInstanceID(1),
      mServer(server) {}

ZoneManager::~ZoneManager() {
  for (auto zPair : mZones) {
    zPair.second->Cleanup();
  }
}

void ZoneManager::LoadGeometry() {
  auto server = mServer.lock();
  auto sharedConfig = server->GetWorldSharedConfig();
  uint8_t channelID = server->GetChannelID();

  auto definitionManager = server->GetDefinitionManager();
  auto serverDataManager = server->GetServerDataManager();

  std::unordered_map<uint32_t, std::set<uint32_t>> localZoneIDs;
  for (auto zonePair : serverDataManager->GetAllZoneIDs()) {
    uint32_t zoneID = zonePair.first;
    for (auto dynamicMapID : zonePair.second) {
      auto zoneData = serverDataManager->GetZoneData(zoneID, dynamicMapID);
      if (!sharedConfig->ChannelDistributionCount() ||
          sharedConfig->GetChannelDistribution(zoneData->GetGroupID()) ==
              channelID) {
        localZoneIDs[zoneID].insert(dynamicMapID);
      }
    }
  }

  for (uint32_t instanceID : serverDataManager->GetAllZoneInstanceIDs()) {
    auto instDef = serverDataManager->GetZoneInstanceData(instanceID);
    if (!sharedConfig->ChannelDistributionCount() ||
        sharedConfig->GetChannelDistribution(instDef->GetGroupID()) ==
            channelID) {
      for (size_t i = 0; i < instDef->ZoneIDsCount(); i++) {
        localZoneIDs[instDef->GetZoneIDs(i)].insert(
            instDef->GetDynamicMapIDs(i));
      }
    }
  }

  // Build zone geometry from QMP files
  ZoneGeometryLoader loader;
  mZoneGeometry = loader.LoadQMP(localZoneIDs, server);

  // Build any existing zone spots as polygons
  // Loop through a second time instead of handling in the first loop
  // because dynamic map/QMP file combos are not the same on all zones
  for (auto zonePair : localZoneIDs) {
    uint32_t zoneID = zonePair.first;
    auto zoneData = definitionManager->GetZoneData(zoneID);

    for (auto dynamicMapID : zonePair.second) {
      auto serverZone = serverDataManager->GetZoneData(zoneID, dynamicMapID);
      if (zoneData && serverZone) {
        auto dynamicMap = definitionManager->GetDynamicMapData(dynamicMapID);
        if (dynamicMap &&
            mDynamicMaps.find(dynamicMapID) == mDynamicMaps.end()) {
          auto dMap = std::make_shared<DynamicMap>();
          auto spots = definitionManager->GetSpotData(dynamicMapID);
          for (auto spotPair : spots) {
            Point center(spotPair.second->GetCenterX(),
                         spotPair.second->GetCenterY());
            float rot = spotPair.second->GetRotation();

            float x1 = center.x - spotPair.second->GetSpanX();
            float y1 = center.y - spotPair.second->GetSpanY();

            float x2 = center.x + spotPair.second->GetSpanX();
            float y2 = center.y + spotPair.second->GetSpanY();

            // Build the unrotated rectangle
            std::vector<Point> points;
            points.push_back(Point(x1, y1));
            points.push_back(Point(x2, y1));
            points.push_back(Point(x2, y2));
            points.push_back(Point(x1, y2));

            auto shape = std::make_shared<ZoneSpotShape>();

            // Rotate each point around the center
            for (auto& p : points) {
              p = RotatePoint(p, center, rot);
              shape->Vertices.push_back(p);
            }

            shape->Definition = spotPair.second;
            shape->Lines.push_back(Line(points[0], points[1]));
            shape->Lines.push_back(Line(points[1], points[2]));
            shape->Lines.push_back(Line(points[2], points[3]));
            shape->Lines.push_back(Line(points[3], points[0]));

            // Determine the boundaries of the completed shape
            std::list<float> xVals;
            std::list<float> yVals;

            for (Line& line : shape->Lines) {
              for (const Point& p : {line.first, line.second}) {
                xVals.push_back(p.x);
                yVals.push_back(p.y);
              }
            }

            xVals.sort([](const float& a, const float& b) { return a < b; });

            yVals.sort([](const float& a, const float& b) { return a < b; });

            shape->Boundaries[0] = Point(xVals.front(), yVals.front());
            shape->Boundaries[1] = Point(xVals.back(), yVals.back());

            dMap->Spots[spotPair.first] = shape;
            dMap->SpotTypes[(uint8_t)spotPair.second->GetType()].push_back(
                shape);
          }

          mDynamicMaps[dynamicMapID] = dMap;
        }
      }
    }
  }
}

void ZoneManager::InstanceGlobalZones() {
  auto server = mServer.lock();
  auto sharedConfig = server->GetWorldSharedConfig();
  auto serverDataManager = server->GetServerDataManager();

  uint8_t channelID = server->GetChannelID();

  // Gather all global zone definitions
  std::list<std::shared_ptr<objects::ServerZone>> zoneDefs;
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    for (auto zonePair : serverDataManager->GetAllZoneIDs()) {
      uint32_t zoneID = zonePair.first;

      auto it = mGlobalZoneMap.find(zoneID);
      for (uint32_t dynamicMapID : zonePair.second) {
        auto zoneData =
            serverDataManager->GetZoneData(zoneID, dynamicMapID, true);
        if (zoneData->GetGlobal() &&
            (it == mGlobalZoneMap.end() ||
             it->second.find(dynamicMapID) == it->second.end())) {
          // Zone is a valid global zone but check to make sure
          // this channel owns it
          if (sharedConfig->ChannelDistributionCount() == 0 ||
              sharedConfig->GetChannelDistribution(zoneData->GetGroupID()) ==
                  channelID) {
            zoneDefs.push_back(zoneData);
          }
        }
      }
    }
  }

  // Build zones from definitions
  for (auto zoneData : zoneDefs) {
    uint32_t zoneID = zoneData->GetID();
    uint32_t dynamicMapID = zoneData->GetDynamicMapID();

    auto zone = CreateZone(zoneData);

    std::lock_guard<libcomp::Mutex> lock(mLock);
    mGlobalZoneMap[zoneID][dynamicMapID] = zone->GetID();
    if (zoneData->GetGlobalBossGroup()) {
      mGlobalBossZones[zoneData->GetGlobalBossGroup()].insert(zone->GetID());
    }
  }

  // Register the global partial time triggers
  auto globalDef = serverDataManager->GetZonePartialData(0);
  if (globalDef) {
    for (auto trigger : globalDef->GetTriggers()) {
      switch (trigger->GetTrigger()) {
        case ZoneTrigger_t::ON_TIME:
        case ZoneTrigger_t::ON_SYSTEMTIME:
        case ZoneTrigger_t::ON_MOONPHASE:
          mGlobalTimeTriggers.push_back(trigger);
          break;
        default:
          break;
      }
    }

    for (auto& t : GetTriggerTimes(mGlobalTimeTriggers)) {
      server->RegisterClockEvent(t, 4, false);
    }
  }
}

std::shared_ptr<Zone> ZoneManager::GetCurrentZone(
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto worldCID = client->GetClientState()->GetWorldCID();
  return GetCurrentZone(worldCID);
}

std::shared_ptr<Zone> ZoneManager::GetCurrentZone(int32_t worldCID) {
  std::lock_guard<libcomp::Mutex> lock(mLock);
  auto iter = mEntityMap.find(worldCID);
  if (iter != mEntityMap.end()) {
    return mZones[iter->second];
  }

  return nullptr;
}

std::shared_ptr<Zone> ZoneManager::GetGlobalZone(uint32_t zoneID,
                                                 uint32_t dynamicMapID) {
  return GetExistingZone(zoneID, dynamicMapID, 0);
}

std::shared_ptr<Zone> ZoneManager::GetExistingZone(uint32_t zoneID,
                                                   uint32_t dynamicMapID,
                                                   uint32_t instanceID) {
  if (instanceID) {
    auto instance = GetInstance(instanceID);
    return instance ? instance->GetZone(zoneID, dynamicMapID) : nullptr;
  } else {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    auto iter = mGlobalZoneMap.find(zoneID);
    if (iter != mGlobalZoneMap.end()) {
      auto subIter = iter->second.find(dynamicMapID);
      if (subIter != iter->second.end()) {
        return mZones[subIter->second];
      }
    }

    return nullptr;
  }
}

bool ZoneManager::EnterZone(
    const std::shared_ptr<ChannelClientConnection>& client, uint32_t zoneID,
    uint32_t dynamicMapID) {
  auto zoneDef =
      mServer.lock()->GetServerDataManager()->GetZoneData(zoneID, dynamicMapID);

  float xCoord = zoneDef ? zoneDef->GetStartingX() : 0.f;
  float yCoord = zoneDef ? zoneDef->GetStartingY() : 0.f;
  float rot = zoneDef ? zoneDef->GetStartingRotation() : 0.f;

  return EnterZone(client, zoneID, zoneDef ? zoneDef->GetDynamicMapID() : 0,
                   xCoord, yCoord, rot, true);
}

bool ZoneManager::EnterZone(
    const std::shared_ptr<ChannelClientConnection>& client, uint32_t zoneID,
    uint32_t dynamicMapID, float xCoord, float yCoord, float rotation,
    bool forceLeave) {
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto worldCID = state->GetWorldCID();

  auto server = mServer.lock();
  auto sharedConfig = server->GetWorldSharedConfig();

  auto currentZone = cState->GetZone();
  auto currentInstance = currentZone ? currentZone->GetInstance() : nullptr;

  bool changeChannel = false;
  std::shared_ptr<objects::ServerZone> zoneDef;
  std::shared_ptr<objects::ServerZoneInstanceVariant> variantDef;
  if (sharedConfig->ChannelDistributionCount()) {
    // Check if the zone is handled by a different channel
    auto serverDataManager = server->GetServerDataManager();
    zoneDef = serverDataManager->GetZoneData(zoneID, dynamicMapID);
    if (zoneDef) {
      uint32_t groupID = zoneDef->GetGroupID();
      if (!zoneDef->GetGlobal()) {
        // Check if its in the player's current instance access
        auto instAccess = GetInstanceAccess(state->GetWorldCID());
        auto instDef = instAccess ? serverDataManager->GetZoneInstanceData(
                                        instAccess->GetDefinitionID())
                                  : nullptr;

        if (instDef && serverDataManager->ExistsInInstance(
                           instDef->GetID(), zoneID, dynamicMapID)) {
          groupID = instDef->GetGroupID();
          variantDef = serverDataManager->GetZoneInstanceVariantData(
              instAccess->GetVariantID());
        } else {
          groupID = 0;
        }
      }

      changeChannel = groupID && sharedConfig->GetChannelDistribution(
                                     groupID) != server->GetChannelID();
    }
  }

  std::shared_ptr<Zone> nextZone;
  std::shared_ptr<ZoneInstance> nextInstance;
  if (!changeChannel) {
    nextZone = GetZone(zoneID, dynamicMapID, client,
                       currentInstance ? currentInstance->GetID() : 0);
    if (nextZone == nullptr) {
      return false;
    } else {
      if (nextZone->GetDefinition()->GetRestricted() &&
          !CanEnterRestrictedZone(client, nextZone)) {
        return false;
      }
    }

    zoneDef = nextZone->GetDefinition();
    nextInstance = nextZone->GetInstance();
    variantDef = nextInstance ? nextInstance->GetVariant() : nullptr;
  }

  if (variantDef &&
      variantDef->GetInstanceType() == InstanceType_t::DEMON_ONLY &&
      (!dState->GetEntity() || !dState->IsAlive())) {
    LogZoneManagerError([&]() {
      return libcomp::String(
                 "Request to enter a demon only zone received with no living "
                 "demon summoned: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    return false;
  }

  if (changeChannel) {
    if (!MoveToZoneChannel(client, zoneID, dynamicMapID, nullptr, xCoord,
                           yCoord, rotation)) {
      return false;
    }

    return true;
  } else if (forceLeave || (currentZone && currentZone != nextZone)) {
    // Trigger zone-out actions
    if (currentZone) {
      TriggerZoneActions(currentZone, {cState, dState},
                         ZoneTrigger_t::ON_ZONE_OUT, client);
    }

    LeaveZone(client, false, zoneID, dynamicMapID);
  } else if (currentZone) {
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    // Remove any opponents
    characterManager->AddRemoveOpponent(false, cState, nullptr);
    characterManager->AddRemoveOpponent(false, dState, nullptr);

    // Deactivate and save the updated status effects
    cState->SetStatusEffectsActive(false, definitionManager);
    dState->SetStatusEffectsActive(false, definitionManager);

    characterManager->UpdateStatusEffects(cState, true);
    characterManager->UpdateStatusEffects(dState, true);

    // Unlike PreviousZone on the character, always set last zone
    // on the state so populate zone actions can act accordingly
    state->SetLastZoneID(currentZone->GetID());
    state->SetLastInstanceID(currentZone->GetInstanceID());
  }

  if (!currentZone || currentZone != nextZone) {
    if (currentZone) {
      if (dState->GetEntity()) {
        LogZoneManagerDebug([cState, dState, zoneID, currentZone]() {
          return libcomp::String("%1 and %2 entering zone %3 from %4: %5\n")
              .Arg(cState->GetEntityLabel())
              .Arg(dState->GetEntityLabel())
              .Arg(zoneID)
              .Arg(currentZone->GetDefinitionID())
              .Arg(cState->GetEntityUUID().ToString());
        });
      } else {
        LogZoneManagerDebug([cState, zoneID, currentZone]() {
          return libcomp::String("%1 entering zone %2 from %3: %4\n")
              .Arg(cState->GetEntityLabel())
              .Arg(zoneID)
              .Arg(currentZone->GetDefinitionID())
              .Arg(cState->GetEntityUUID().ToString());
        });
      }
    } else {
      if (dState->GetEntity()) {
        LogZoneManagerDebug([cState, dState, zoneID]() {
          return libcomp::String("%1 and %2 starting in zone %3: %4\n")
              .Arg(cState->GetEntityLabel())
              .Arg(dState->GetEntityLabel())
              .Arg(zoneID)
              .Arg(cState->GetEntityUUID().ToString());
        });
      } else {
        LogZoneManagerDebug([cState, zoneID]() {
          return libcomp::String("%1 starting in zone %2: %3\n")
              .Arg(cState->GetEntityLabel())
              .Arg(zoneID)
              .Arg(cState->GetEntityUUID().ToString());
        });
      }
    }
  }

  auto uniqueID = nextZone->GetID();

  bool firstConnection = false;
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    mEntityMap[worldCID] = uniqueID;

    // When the player enters the instance they have access to
    // revoke access so they cannot re-enter
    if (nextInstance) {
      auto syncManager = server->GetChannelSyncManager();

      auto accessIter = mZoneInstanceAccess.find(worldCID);
      if (accessIter != mZoneInstanceAccess.end() &&
          accessIter->second->GetIsLocal() &&
          accessIter->second->GetInstanceID() == nextInstance->GetID()) {
        auto access = accessIter->second;
        access->RemoveAccessCIDs(worldCID);
        mZoneInstanceAccess.erase(accessIter);

        syncManager->UpdateRecord(access, "InstanceAccess");
      }

      nextInstance->GetAccess()->RemoveAccessCIDs(worldCID);
      syncManager->UpdateRecord(nextInstance->GetAccess(), "InstanceAccess");

      syncManager->SyncOutgoing();
    }

    // Reactive the zone if its not active already
    bool activateTracking = false;
    if (mActiveZones.find(uniqueID) == mActiveZones.end()) {
      mActiveZones.insert(uniqueID);
      firstConnection = true;

      if (nextZone->GetInstanceType() == InstanceType_t::DIASPORA ||
          zoneDef->GetTrackTeam() || zoneDef->GetGlobalBossGroup()) {
        mActiveTrackedZones.insert(uniqueID);
        activateTracking = true;
      }
    }

    // If global boss refreshes are inactive and the player is entering
    // a zone with a global boss group, set the refresh time to the
    // next active zone update
    if (activateTracking && !mTrackingRefresh) {
      mTrackingRefresh = 1;
    }
  }

  if (!nextZone->AddConnection(client)) {
    LogZoneManagerError([&]() {
      return libcomp::String("Failed to add client to zone: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    client->Close();
    return false;
  }

  // Both player characters and demons start with an AI ignore delay
  // upon entering the first zone on the channel
  if (!currentZone) {
    uint64_t delay =
        ChannelServer::GetServerTime() + (uint64_t)AI_ZONE_IN_IGNORE;
    cState->SetStatusTimes(STATUS_IGNORE, delay);
    dState->SetStatusTimes(STATUS_IGNORE, delay);
  }

  cState->SetZone(nextZone);
  dState->SetZone(nextZone);

  // Reset state values that do not persist between zones
  state->SetAcceptRevival(false);
  cState->SetDeathTimeOut(0);
  dState->SetDeathTimeOut(0);
  state->SetZoneInSpotID(0);
  cState->ClearAggroIDs();
  dState->ClearAggroIDs();

  auto ticks = server->GetServerTime();

  // Clear any additional zone dependent tokusei if changing instances
  // or public zones
  std::unordered_map<std::shared_ptr<ActiveEntityState>, std::set<int32_t>>
      removeTokusei;
  if (currentInstance != nextInstance || !currentInstance) {
    for (auto& pair : cState->GetAdditionalTokusei()) {
      removeTokusei[cState].insert(pair.first);
    }

    for (auto& pair : dState->GetAdditionalTokusei()) {
      removeTokusei[dState].insert(pair.first);
    }
  }

  // Bike boosting does not persist between zones
  if (state->GetBikeBoosting()) {
    state->SetBikeBoosting(false);
    removeTokusei[cState].insert(SVR_CONST.TOKUSEI_BIKE_BOOST);
  }

  if (removeTokusei.size() > 0) {
    for (auto& pair : removeTokusei) {
      for (int32_t tokuseiID : pair.second) {
        pair.first->RemoveAdditionalTokusei(tokuseiID);
      }
    }

    server->GetTokuseiManager()->Recalculate(cState, true);
  }

  // Set the zone-in spot for instances
  if (nextInstance) {
    auto dynamicMap = nextZone->GetDynamicMap();

    if (dynamicMap) {
      auto spots = server->GetDefinitionManager()->GetSpotData(
          zoneDef->GetDynamicMapID());
      for (auto spotPair : spots) {
        auto spot = spotPair.second;
        auto spotIter = dynamicMap->Spots.find(spotPair.first);

        // Filter valid zone-in spots only
        if ((spot->GetType() == objects::MiSpotData::Type_t::ZONE_IN_POINT ||
             spot->GetType() ==
                 objects::MiSpotData::Type_t::DIASPORA_ZONE_IN) &&
            spotIter != dynamicMap->Spots.end()) {
          if (PointInPolygon(Point(xCoord, yCoord),
                             spotIter->second->Vertices)) {
            state->SetZoneInSpotID(spotPair.first);
            break;
          }
        }
      }
    }

    nextInstance->SetAccessTimeOut(0);
    nextInstance->RefreshPlayerState();
    nextInstance->SetAccessed(true);
  }

  // Lock movement and move the entities to the new location
  state->SetLockMovement(true);

  for (auto eState : {std::dynamic_pointer_cast<ActiveEntityState>(cState),
                      std::dynamic_pointer_cast<ActiveEntityState>(dState)}) {
    eState->SetOriginX(xCoord);
    eState->SetOriginY(yCoord);
    eState->SetOriginRotation(rotation);
    eState->SetOriginTicks(ticks);
    eState->SetDestinationX(xCoord);
    eState->SetDestinationY(yCoord);
    eState->SetDestinationRotation(rotation);
    eState->SetDestinationTicks(ticks);
    eState->SetCurrentX(xCoord);
    eState->SetCurrentY(yCoord);
    eState->SetCurrentRotation(rotation);
  }

  server->GetTokuseiManager()->RecalculateParty(state->GetParty());

  auto matchManager = server->GetMatchManager();
  bool matchEntryExists =
      matchManager->GetMatchEntry(state->GetWorldCID()) != nullptr;
  if (currentInstance != nextInstance) {
    // End any previous instance specific data if leaving
    if (currentInstance) {
      EndInstanceTimer(currentInstance, client, false, true);
    }

    // Match entries are not valid across instances
    if (matchEntryExists) {
      matchEntryExists = !matchManager->CancelQueue(client);
    }

    // Reset values that don't persist between instances
    cState->SetFactionGroup(0);
    dState->SetFactionGroup(0);
    cState->SetKillValue(0);
    dState->SetKillValue(0);
    state->SetInstanceBethel(0);

    // If entering or exiting a digitalize instance, end any
    // current digitalize session
    for (auto inst : {currentInstance, nextInstance}) {
      auto variant = inst ? inst->GetVariant() : nullptr;
      if (variant && variant->GetInstanceType() == InstanceType_t::DIGITALIZE) {
        server->GetCharacterManager()->DigitalizeEnd(client);
        break;
      }
    }

    // If we're entering an instance but its not the first zone for the
    // login, send the "moved to" message
    if (nextInstance && currentZone) {
      SendAccessMessage(nextInstance->GetAccess(), true, client);
    }
  } else if (!nextInstance) {
    // Kill values do not persist between public zones
    cState->SetKillValue(0);
    dState->SetKillValue(0);
  }

  auto team = state->GetTeam();
  if (team) {
    // Teams are not valid when changing zones unless they are queued
    // for a match, the player is in an instance or the new zone allows
    // that team type
    auto globalPartial = server->GetServerDataManager()->GetZonePartialData(0);
    if (!matchEntryExists && !nextInstance &&
        !zoneDef->ValidTeamTypesContains(team->GetType()) &&
        (!globalPartial ||
         !globalPartial->ValidTeamTypesContains(team->GetType()))) {
      matchManager->LeaveTeam(client, team->GetID());
      team = nullptr;
    } else {
      // Update team tracking (non-team tracking handled elsewhere)
      if (!UpdateTrackedTeam(team, currentZone)) {
        UpdateTrackedTeam(team, nextZone);
      }
    }
  }

  if (!nextInstance && currentZone) {
    // If moving to a non-instanced zone, set log out zone for the new
    // location in case of a disconnect without saving
    auto character = cState->GetEntity();
    character->SetLogoutZone(zoneID);
    character->SetLogoutInstance(0);
    character->SetLogoutX(cState->GetCurrentX());
    character->SetLogoutY(cState->GetCurrentY());
    character->SetLogoutRotation(cState->GetCurrentRotation());

    server->GetWorldDatabase()->QueueUpdate(character);
  }

  // Fire pre-zone in for both character and demon
  auto triggers = GetZoneTriggers(nextZone, ZoneTrigger_t::PRE_ZONE_IN);
  if (triggers.size() > 0) {
    // If max HP or MP raises from pre-zone in, retain percent for
    // new value
    std::array<int32_t, 2> maxHPs = {{cState->GetMaxHP(), dState->GetMaxHP()}};
    std::array<int32_t, 2> maxMPs = {{cState->GetMaxMP(), dState->GetMaxMP()}};
    std::array<std::shared_ptr<ActiveEntityState>, 2> entitites = {
        {std::dynamic_pointer_cast<ActiveEntityState>(cState),
         std::dynamic_pointer_cast<ActiveEntityState>(dState)}};

    if (TriggerZoneActions(nextZone, {cState, dState},
                           ZoneTrigger_t::PRE_ZONE_IN, client)) {
      for (size_t i = 0; i < 2; i++) {
        auto eState = entitites[i];
        auto cs = eState->GetCoreStats();
        if (!cs) {
          continue;
        }

        int32_t hp = cs->GetHP();
        int32_t mp = cs->GetMP();

        bool updated = false;
        if (eState->GetMaxHP() > maxHPs[i] && hp < eState->GetMaxHP()) {
          cs->SetHP((int32_t)ceil((double)eState->GetMaxHP() * (double)hp /
                                  (double)maxHPs[i]));
          updated = true;
        }

        if (eState->GetMaxMP() > maxMPs[i] && mp < eState->GetMaxMP()) {
          cs->SetMP((int32_t)ceil((double)eState->GetMaxMP() * (double)mp /
                                  (double)maxMPs[i]));
          updated = true;
        }

        if (updated) {
          libcomp::Packet p;
          CharacterManager::GetTDamagePacket(
              p, eState->GetEntityID(), cs->GetHP() - hp, cs->GetMP() - mp);
          client->QueuePacket(p);
        }
      }
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE);
  reply.WriteS32Little((int32_t)zoneDef->GetID());
  reply.WriteS32Little((int32_t)nextZone->GetID());
  reply.WriteFloat(cState->GetCurrentX());
  reply.WriteFloat(cState->GetCurrentY());
  reply.WriteFloat(cState->GetCurrentRotation());
  reply.WriteS32Little((int32_t)zoneDef->GetDynamicMapID());

  client->QueuePacket(reply);

  if (firstConnection && nextInstance &&
      nextZone->GetInstanceType() != InstanceType_t::PVP &&
      nextZone->GetInstanceType() != InstanceType_t::MISSION) {
    // Timer start handled elsewhere
    StartInstanceTimer(nextInstance);
  }

  client->FlushOutgoing();

  // Tell the world that the character has changed zones
  auto cLogin = state->GetAccountLogin()->GetCharacterLogin();

  libcomp::Packet request;
  request.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
  request.WriteS32Little(cLogin->GetWorldCID());
  if (cLogin->GetZoneID() == 0) {
    // Send first zone in info
    request.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_STATUS |
                    (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE);
    request.WriteS8((int8_t)cLogin->GetStatus());
  } else {
    // Send normal zone change info
    request.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE);
  }
  request.WriteU32Little(zoneID);
  cLogin->SetZoneID(zoneID);

  server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

  return true;
}

void ZoneManager::LeaveZone(
    const std::shared_ptr<ChannelClientConnection>& client, bool logOut,
    uint32_t newZoneID, uint32_t newDynamicMapID) {
  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto worldCID = state->GetWorldCID();

  // Cancel any active skills
  for (auto eState : {std::dynamic_pointer_cast<ActiveEntityState>(cState),
                      std::dynamic_pointer_cast<ActiveEntityState>(dState)}) {
    auto activated = eState->GetActivatedAbility();
    if (activated) {
      server->GetSkillManager()->CancelSkill(eState,
                                             activated->GetActivationID());
    }
  }

  // Lock entity interactions in the zone
  state->SetZoneInTime(0);

  // Detach from zone specific state info
  auto exchangeSession = state->GetExchangeSession();
  if (exchangeSession) {
    auto otherCState = std::dynamic_pointer_cast<CharacterState>(
        exchangeSession->GetOtherCharacterState());
    if (otherCState &&
        (otherCState != cState ||
         exchangeSession->GetSourceEntityID() != cState->GetEntityID())) {
      auto connectionManager = server->GetManagerConnection();
      auto otherClient = connectionManager->GetEntityClient(
          otherCState != cState ? otherCState->GetEntityID()
                                : exchangeSession->GetSourceEntityID(),
          false);

      if (otherClient) {
        characterManager->EndExchange(otherClient);
      }
    }

    characterManager->EndExchange(client);
  }

  // Remove any opponents
  characterManager->AddRemoveOpponent(false, cState, nullptr);
  characterManager->AddRemoveOpponent(false, dState, nullptr);

  // If there is a pending bazaar, mark as active again
  auto bState = state->GetBazaarState();
  if (bState) {
    auto worldData = state->GetAccountWorldData().Get();
    auto bData = worldData->GetBazaarData().Get();
    if (bData && bState->GetCurrentMarket(bData->GetMarketID()) == bData &&
        bData->GetState() == objects::BazaarData::State_t::BAZAAR_PREPARING) {
      bData->SetState(objects::BazaarData::State_t::BAZAAR_ACTIVE);
      SendBazaarMarketData(state->GetZone(), bState, bData->GetMarketID());
    }
  }

  if (newZoneID == 0) {
    // Trigger the zone-out actions before leaving if not going
    // somewhere else
    std::shared_ptr<Zone> zone;
    {
      std::lock_guard<libcomp::Mutex> lock(mLock);
      auto iter = mEntityMap.find(worldCID);
      if (iter != mEntityMap.end()) {
        zone = mZones[iter->second];
      }
    }

    if (zone) {
      TriggerZoneActions(zone, {cState, dState}, ZoneTrigger_t::ON_ZONE_OUT,
                         client);
    }
  }

  std::shared_ptr<Zone> zone = nullptr;
  bool instanceLeft = false;
  bool instanceRemoved = false;
  bool instanceDisconnect = false;
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    auto iter = mEntityMap.find(worldCID);
    if (iter != mEntityMap.end()) {
      uint32_t uniqueID = iter->second;
      zone = mZones[uniqueID];

      auto def = zone->GetDefinition();
      auto instance = zone->GetInstance();

      mEntityMap.erase(worldCID);
      zone->RemoveConnection(client);

      instanceLeft =
          instance &&
          !server->GetServerDataManager()->ExistsInInstance(
              instance->GetDefinitionID(), newZoneID, newDynamicMapID);

      // PvP ignores normal disconnect logic
      instanceDisconnect = instanceLeft && logOut && !state->GetLogoutTimer() &&
                           !state->GetChannelLogin() &&
                           zone->GetInstanceType() != InstanceType_t::PVP;
      if (instanceDisconnect) {
        // Disconnecting from an instance puts the player's last
        // zone location at their most recent zone-in point
        auto character = cState->GetEntity();

        float x = cState->GetDestinationX();
        float y = cState->GetDestinationY();
        float rot = cState->GetDestinationRotation();

        if (state->GetZoneInSpotID()) {
          GetSpotPosition(zone->GetDynamicMapID(), state->GetZoneInSpotID(), x,
                          y, rot);
        }

        // Make sure nothing incorrect updates it later
        character->SetLogoutX(x);
        character->SetLogoutY(y);
        character->SetLogoutRotation(rot);

        cState->SetOriginX(x);
        cState->SetOriginY(y);
        cState->SetOriginRotation(rot);
        cState->SetDestinationX(x);
        cState->SetDestinationY(y);
        cState->SetDestinationRotation(rot);
        cState->SetCurrentX(x);
        cState->SetCurrentY(y);
        cState->SetCurrentRotation(rot);

        // Notify the world that the character can relog after
        // disconnecting until the instance is removed
        auto relogin = std::make_shared<objects::ChannelLogin>();
        relogin->SetWorldCID(state->GetWorldCID());
        relogin->SetToChannel((int8_t)server->GetChannelID());
        relogin->SetToZoneID(zone->GetDefinitionID());
        relogin->SetToDynamicMapID(zone->GetDynamicMapID());

        libcomp::Packet p;
        p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
        p.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);
        p.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8,
            state->GetAccountLogin()->GetAccount()->GetUsername());
        p.WriteS8(-1);  // Can relog
        relogin->SavePacket(p);
        p.WriteU32Little(instance->GetID());

        server->GetManagerConnection()->GetWorldConnection()->SendPacket(p);

        // Put the original instance access back for the character
        auto access = instance->GetAccess();
        access->InsertAccessCIDs(state->GetWorldCID());
        mZoneInstanceAccess[state->GetWorldCID()] = access;
      }

      // Determine actions needed if the last connection has left
      if (zone->GetConnections().size() == 0) {
        // Always "freeze" the zone
        RemoveZone(zone, true);

        // If the current zone is global, the next zone is the same
        // or the next zone is will be on the same instance, keep it
        bool keepZone = !instanceLeft;
        if (def->GetGlobal() || (def->GetID() == newZoneID &&
                                 def->GetDynamicMapID() == newDynamicMapID)) {
          keepZone = true;
        }

        // If an instance zone is being left see if it
        // is empty and can be removed
        if (!keepZone && instance) {
          if (instanceDisconnect) {
            // Sudden disconnects will delay instance cleanup for
            // 5 minutes
            LogZoneManagerDebug([&]() {
              return libcomp::String(
                         "Last disconnect occurred in zone instance %1. Access "
                         "time-out started.\n")
                  .Arg(instance->GetID());
            });

            ScheduleInstanceAccessTimeOut(instance);
          } else {
            instanceRemoved = RemoveInstance(instance->GetID());
          }
        }

        if (keepZone) {
          // Stop all AI in place
          uint64_t now = ChannelServer::GetServerTime();
          for (auto eState : zone->GetEnemies()) {
            eState->Stop(now);
          }
        }

        // Reset tracking refresh if no other zones are active
        if (mTrackingRefresh && mActiveTrackedZones.size() == 0) {
          // No need to refresh right now
          mTrackingRefresh = 0;
        }
      }

      // Refresh the state of the instance being left
      if (instance && !instanceRemoved) {
        instance->RefreshPlayerState();
      }
    } else {
      // Not in a zone, nothing to do
      return;
    }
  }

  if (!instanceRemoved) {
    auto characterID = cState->GetEntityID();
    auto demonID = dState->GetEntityID();
    std::list<int32_t> entityIDs = {characterID, demonID};
    RemoveEntitiesFromZone(zone, entityIDs);

    if (instanceLeft && zone->GetDefinitionID() != newZoneID) {
      switch (zone->GetInstanceType()) {
        case InstanceType_t::PVP:
          // Inform other players still in the PvP match
          server->GetMatchManager()->SendPvPLocation(
              client, zone->GetInstance()->GetID(), false);
          break;
        case InstanceType_t::DIASPORA:
          // Inform other players still in the Diaspora match
          server->GetMatchManager()->SendDiasporaLocation(
              client, zone->GetInstance()->GetID(), false);
          break;
        default:
          break;
      }
    }
  }

  if (zone->GetUBMatch() &&
      (instanceLeft ||
       (!zone->GetInstance() && zone->GetDefinitionID() != newZoneID))) {
    server->GetMatchManager()->LeaveUltimateBattle(client, zone);
  }

  if (newZoneID == 0) {
    // Not entering another zone, recalculate tokusei for
    // remaining party member effects
    server->GetTokuseiManager()->RecalculateParty(state->GetParty());

    // Update tracking
    UpdateTrackedZone(zone, state->GetTeam());

    cState->SetZone(nullptr);
    dState->SetZone(nullptr);

    if (zone) {
      LogZoneManagerDebug([&]() {
        return libcomp::String("Character left zone %1: %2\n")
            .Arg(zone->GetDefinitionID())
            .Arg(cState->GetEntityUUID().ToString());
      });
    }
  } else {
    // Set the previous zone
    if (newZoneID != zone->GetDefinitionID()) {
      // Only change previous zone if actually changing zones
      cState->GetEntity()->SetPreviousZone(zone->GetDefinitionID());
    }

    state->SetLastZoneID(zone->GetID());
    state->SetLastInstanceID(zone->GetInstanceID());
  }

  // If logging out, cancel zone out and log out effects (zone out effects
  // are cancelled on zone enter instead if not logging out)
  if (logOut) {
    // Instance disconnects don't trigger zone out effect cancellations
    uint8_t cancelFlags = instanceDisconnect ? 0 : EFFECT_CANCEL_ZONEOUT;

    auto channelLogin = state->GetChannelLogin();
    bool channelChange =
        channelLogin && channelLogin->GetToChannel() != server->GetChannelID();
    if (!channelChange) {
      // Only cancel logout status effects if we're not changing
      // channels
      cancelFlags |= EFFECT_CANCEL_LOGOUT;
    }

    characterManager->CancelStatusEffects(client, cancelFlags);
  } else {
    // If a pending match bound to the zone being left exists, end it
    auto match = state->GetPendingMatch();
    if (match && match->GetZoneDefinitionID() == zone->GetDefinitionID()) {
      server->GetMatchManager()->CleanupPendingMatch(client);
    }
  }

  // Deactivate and save the updated status effects
  cState->SetStatusEffectsActive(false, definitionManager);
  dState->SetStatusEffectsActive(false, definitionManager);
  characterManager->UpdateStatusEffects(cState, !logOut);
  characterManager->UpdateStatusEffects(dState, !logOut);
}

uint8_t ZoneManager::CreateInstance(
    const std::shared_ptr<objects::InstanceAccess>& access) {
  auto server = mServer.lock();
  auto serverDataManager = server->GetServerDataManager();

  auto def = serverDataManager->GetZoneInstanceData(access->GetDefinitionID());
  if (!def) {
    LogZoneManagerError([&]() {
      return libcomp::String("Attempted to create invalid zone instance: %1\n")
          .Arg(access->GetDefinitionID());
    });

    return 0;
  }

  uint32_t variantID = access->GetVariantID();
  auto variant = variantID
                     ? serverDataManager->GetZoneInstanceVariantData(variantID)
                     : nullptr;
  if (variantID) {
    if (!variant) {
      LogZoneManagerError([&]() {
        return libcomp::String(
                   "Invalid variant encountered during instance creation: %1\n")
            .Arg(variantID);
      });

      return 0;
    }

    if (variant->GetInstanceType() == InstanceType_t::PVP &&
        !serverDataManager->VerifyPvPInstance(access->GetDefinitionID(),
                                              server->GetDefinitionManager())) {
      return 0;
    }
  }

  std::shared_ptr<objects::MiTimeLimitData> timeData;
  if (access->GetCreateTimerID()) {
    if (!variant || variant->GetInstanceType() == InstanceType_t::NORMAL) {
      timeData = server->GetDefinitionManager()->GetTimeLimitData(
          access->GetCreateTimerID());
      if (!timeData) {
        LogZoneManagerError([&]() {
          return libcomp::String(
                     "Invalid timer ID specified for instance creation: %1\n")
              .Arg(access->GetCreateTimerID());
        });

        return 0;
      }
    } else if (variant->GetInstanceType() != InstanceType_t::DEMON_ONLY) {
      // Demon only instances use the timer ID to specify timer color
      // 0 = bronze, 1 = silver, 2 = gold

      LogZoneManagerError([&]() {
        return libcomp::String(
                   "Attempted to specify a timer during special instance "
                   "creation: %1\n")
            .Arg(variantID);
      });

      return 0;
    }
  }

  auto syncManager = server->GetChannelSyncManager();
  auto sharedConfig = server->GetWorldSharedConfig();

  uint8_t channelID = server->GetChannelID();
  uint8_t ownerChannelID =
      sharedConfig->ChannelDistributionCount() > 0
          ? sharedConfig->GetChannelDistribution(def->GetGroupID())
          : channelID;

  access->SetChannelID(ownerChannelID);

  // Notify players on this channel that it has been created (other channels
  // will receive messages when the access is added)
  SendAccessMessage(access, false);

  std::lock_guard<libcomp::Mutex> lock(mLock);

  std::set<std::shared_ptr<objects::InstanceAccess>> existing;
  for (int32_t cid : access->GetAccessCIDs()) {
    auto other = mZoneInstanceAccess[cid];
    mZoneInstanceAccess[cid] = access;

    if (other && other != access) {
      other->RemoveAccessCIDs(cid);
      existing.insert(other);
    }
  }

  for (auto other : existing) {
    if (other->AccessCIDsCount() == 0) {
      syncManager->RemoveRecord(other, "InstanceAccess");
    } else {
      syncManager->UpdateRecord(other, "InstanceAccess");
    }
  }

  if (channelID != ownerChannelID) {
    // Set any local client's access values now (will sync back later)
    access->SetIsLocal(false);
    access->SetRequestID(libobjgen::UUID::Random());

    // Sync the record
    syncManager->SyncRecordUpdate(access, "InstanceAccess");

    return 2;  // Requested on another channel
  } else {
    // Instance valid, determine if the instance should be created here or
    // on a different channel
    std::shared_ptr<ZoneInstance> instance;
    {
      uint32_t id = mNextZoneInstanceID++;

      instance = std::make_shared<ZoneInstance>(id, def, access);
      if (variant) {
        instance->SetVariant(variant);
        instance->SetTimerExpirationEventID(
            variant->GetTimerExpirationEventID());

        // If the variant uses a shared destiny box, make it now
        instance->GetDestinyBox(0);
      }

      if (timeData) {
        instance->SetTimeLimitData(timeData);
      }

      if (instance->GetTimerExpirationEventID().IsEmpty()) {
        instance->SetTimerExpirationEventID(
            access->GetCreateTimerExpirationEventID());
      }

      instance->SetTimerID(access->GetCreateTimerID());

      access->SetInstanceID(id);
      access->SetIsLocal(true);
      if (access->GetRequestID().IsNull()) {
        access->SetRequestID(libobjgen::UUID::Random());
      }

      mZoneInstances[id] = instance;

      LogZoneManagerDebug([&]() {
        return libcomp::String("Creating zone instance: %1 (%2%3)\n")
            .Arg(id)
            .Arg(def->GetID())
            .Arg(variantID ? libcomp::String(": %1").Arg(variantID) : "");
      });
    }

    if (variant && variant->GetInstanceType() == InstanceType_t::DIASPORA) {
      // Create a default match and send to the world to disband the
      // team(s) and relay back the instance enter request
      auto match = std::make_shared<objects::Match>();
      match->SetType(objects::Match::Type_t::DIASPORA);
      match->SetInstanceDefinitionID(access->GetDefinitionID());
      match->SetInstanceID(instance->GetID());
      match->SetVariantID(variantID);
      match->SetMemberIDs(access->GetAccessCIDs());

      instance->SetMatch(match);

      syncManager->UpdateRecord(match, "Match");
    }

    // Expire the instance access if no one ever enters
    ScheduleInstanceAccessTimeOut(instance);

    // Sync the record to notify the rest
    syncManager->UpdateRecord(access, "InstanceAccess");
    syncManager->SyncOutgoing();

    return 1;  // Created local
  }
}

void ZoneManager::ExpireInstance(uint32_t instanceID, uint64_t timeOut) {
  auto instance = GetInstance(instanceID);

  if (instance && instance->GetAccessTimeOut() == timeOut) {
    RemoveInstance(instanceID);
  }
}

std::shared_ptr<ZoneInstance> ZoneManager::GetInstance(uint32_t instanceID) {
  std::lock_guard<libcomp::Mutex> lock(mLock);
  auto it = mZoneInstances.find(instanceID);
  return it != mZoneInstances.end() ? it->second : nullptr;
}

std::shared_ptr<objects::InstanceAccess> ZoneManager::GetInstanceAccess(
    int32_t worldCID) {
  std::lock_guard<libcomp::Mutex> lock(mLock);
  auto it = mZoneInstanceAccess.find(worldCID);
  return it != mZoneInstanceAccess.end() ? it->second : nullptr;
}

std::shared_ptr<Zone> ZoneManager::GetInstanceStartingZone(
    const std::shared_ptr<ZoneInstance>& instance) {
  auto def = instance ? instance->GetDefinition() : nullptr;
  if (def) {
    uint32_t firstZoneID = *def->ZoneIDsBegin();
    uint32_t firstDynamicMapID = *def->DynamicMapIDsBegin();
    return GetInstanceZone(instance, firstZoneID, firstDynamicMapID);
  }

  return nullptr;
}

bool ZoneManager::GetMatchStartPosition(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<Zone>& zone, float& x, float& y, float& rot) {
  auto instance = zone ? zone->GetInstance() : nullptr;
  if (!zone || !client) {
    return false;
  }

  x = y = rot = 0.f;

  auto state = client->GetClientState();
  int32_t worldCID = state->GetWorldCID();

  bool isPvp = false;
  int8_t groupIdx = -1;
  switch (zone->GetInstanceType()) {
    case InstanceType_t::PVP: {
      auto pvpStats = instance ? instance->GetPvPStats() : nullptr;
      if (!pvpStats) {
        return false;
      }

      auto match = pvpStats->GetMatch();
      if (match) {
        for (int32_t memberID : match->GetBlueMemberIDs()) {
          if (memberID == worldCID) {
            groupIdx = 0;
            break;
          }
        }

        for (int32_t memberID : match->GetRedMemberIDs()) {
          if (memberID == worldCID) {
            groupIdx = 1;
            break;
          }
        }
      }

      isPvp = true;
    } break;
    case InstanceType_t::DIASPORA:
    default:
      groupIdx = 0;
      break;
  }

  auto def = zone->GetDefinition();

  // Move to blue spot, red spot or starting/zone-in spot
  uint32_t spotID = 0;
  if (groupIdx >= 0) {
    // Gather team spots
    std::unordered_map<uint8_t, std::set<uint32_t>> teamSpotIDs;
    for (auto& spotPair : def->GetSpots()) {
      auto spot = spotPair.second;
      if (spot->GetMatchZoneInLimit()) {
        // Skip if the limit has been reached
        size_t count = 0;
        for (auto c : zone->GetConnectionList()) {
          if (c != client &&
              c->GetClientState()->GetZoneInSpotID() == spotPair.first) {
            count++;
          }
        }

        if (count >= (size_t)spot->GetMatchZoneInLimit()) {
          continue;
        }
      }

      switch (spot->GetMatchSpawn()) {
        case objects::ServerZoneSpot::MatchSpawn_t::PVP_BLUE:
          if (isPvp) {
            teamSpotIDs[0].insert(spotPair.first);
          }
          break;
        case objects::ServerZoneSpot::MatchSpawn_t::PVP_RED:
          if (isPvp) {
            teamSpotIDs[1].insert(spotPair.first);
          }
          break;
        case objects::ServerZoneSpot::MatchSpawn_t::ALL:
          teamSpotIDs[0].insert(spotPair.first);
          teamSpotIDs[1].insert(spotPair.first);
          break;
        default:
          break;
      }
    }

    spotID = libcomp::Randomizer::GetEntry(teamSpotIDs[(uint8_t)groupIdx]);
  } else if (state->GetZone() == zone) {
    spotID = state->GetZoneInSpotID();
  }

  if (spotID) {
    auto definitionManager = mServer.lock()->GetDefinitionManager();
    auto spots = definitionManager->GetSpotData(def->GetDynamicMapID());
    auto spotIter = spots.find(spotID);
    if (spotIter != spots.end()) {
      auto zoneData = definitionManager->GetZoneData(def->GetID());

      Point p = GetRandomSpotPoint(spotIter->second, zoneData);
      x = p.x;
      y = p.y;
      rot = spotIter->second->GetRotation();
      return true;
    }
  }

  // If not defined, use the starting spot
  x = def->GetStartingX();
  y = def->GetStartingY();
  rot = def->GetStartingRotation();

  return true;
}

bool ZoneManager::MoveToInstance(
    const std::shared_ptr<ChannelClientConnection>& client,
    std::shared_ptr<objects::InstanceAccess> access, bool diasporaEnter) {
  auto state = client->GetClientState();
  if (!access) {
    access = GetInstanceAccess(state->GetWorldCID());
    if (!access) {
      return false;
    }
  }

  auto server = mServer.lock();
  if (access->GetIsLocal()) {
    // Enter the instance on this channel
    auto instance = GetInstance(access->GetInstanceID());
    auto zone = GetInstanceStartingZone(instance);
    if (zone) {
      auto zoneDef = zone->GetDefinition();

      float x = zoneDef->GetStartingX();
      float y = zoneDef->GetStartingY();
      float rot = zoneDef->GetStartingRotation();
      if (zone->GetInstanceType() == InstanceType_t::DIASPORA) {
        if (!diasporaEnter) {
          return false;
        } else if (!GetMatchStartPosition(client, zone, x, y, rot)) {
          LogZoneManagerWarningMsg(
              "Failed to find the Diaspora instance starting spot. Using the "
              "starting coordinates instead.\n");
        }
      } else if (zone->GetInstanceType() == InstanceType_t::PVP &&
                 !GetMatchStartPosition(client, zone, x, y, rot)) {
        LogZoneManagerWarningMsg(
            "Failed to find the PvP instance starting spot. Using the starting "
            "coordinates instead.\n");
      } else if (state->GetChannelLogin()) {
        // Check to see if this is a reconnect
        auto character = state->GetCharacterState()->GetEntity();
        auto channelLogin = state->GetChannelLogin();

        auto instZone = instance->GetZone(channelLogin->GetToZoneID(),
                                          channelLogin->GetToDynamicMapID());
        if (instZone &&
            instZone->GetDefinitionID() == character->GetLogoutZone()) {
          zone = instZone;
          zoneDef = zone->GetDefinition();
          x = character->GetLogoutX();
          y = character->GetLogoutY();
          rot = character->GetLogoutRotation();
        }
      }

      // No need to print the recovery message if no one has entered yet
      if (instance->GetAccessTimeOut() && instance->GetAccessed()) {
        LogZoneManagerDebug([&]() {
          return libcomp::String(
                     "Zone instance %1 recovered before access expired.\n")
              .Arg(instance->GetID());
        });
      }

      return EnterZone(client, zone->GetDefinitionID(), zone->GetDynamicMapID(),
                       x, y, rot);
    }
  } else {
    // Send to the correct channel
    auto instDef = server->GetServerDataManager()->GetZoneInstanceData(
        access->GetDefinitionID());

    return MoveToZoneChannel(client, instDef->GetZoneIDs(0),
                             instDef->GetDynamicMapIDs(0), access);
  }

  return false;
}

bool ZoneManager::MoveToLobby(
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto state = client->GetClientState();
  auto zone = state->GetZone();
  if (!zone || !zone->GetDefinition()->GetGroupID()) {
    return false;
  }

  auto lobby = mServer.lock()->GetServerDataManager()->GetZoneData(
      zone->GetDefinition()->GetGroupID(), 0);
  if (lobby) {
    return EnterZone(client, lobby->GetID(), lobby->GetDynamicMapID());
  }

  return false;
}

bool ZoneManager::SendPopulateZoneData(
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto server = mServer.lock();
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();

  auto zone = GetCurrentZone(state->GetWorldCID());
  if (!zone) {
    // Not in a zone, quit now
    return false;
  }

  // Unlock movement now that the client is acknowledging being in the zone
  state->SetLockMovement(false);
  state->SetZoneInTime(ChannelServer::GetServerTime());

  auto zoneDef = zone->GetDefinition();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  bool spectatingMatch = MatchManager::SpectatingMatch(client, zone);

  // Send the new connection entity data to the other clients
  auto otherClients = GetZoneConnections(client, false);
  if (otherClients.size() > 0 && !spectatingMatch) {
    characterManager->SendOtherCharacterData(otherClients, state);

    if (dState->GetEntity()) {
      characterManager->SendOtherPartnerData(otherClients, state);
    }
  }

  // Expire zone change status effects. Do not expire if changing zones in
  // the same instance
  if (!state->GetLastInstanceID() ||
      (zone->GetInstanceID() != state->GetLastInstanceID())) {
    characterManager->CancelStatusEffects(client, EFFECT_CANCEL_ZONEOUT);
  }

  HandleSpecialInstancePopulate(client, zone);

  if (!spectatingMatch) {
    ShowEntityToZone(zone, cState->GetEntityID());
    characterManager->SendMovementSpeed(client, cState, true);

    if (dState->GetEntity()) {
      ShowDemonToZone(zone, client);
    }
  } else {
    ShowEntity(client, cState->GetEntityID());
  }

  // Activate status effects
  cState->SetStatusEffectsActive(true, definitionManager);
  dState->SetStatusEffectsActive(true, definitionManager);

  // Trigger zone-in actions (and login if starting zone)
  if (state->GetLastZoneID() == 0) {
    TriggerZoneActions(zone, {cState}, ZoneTrigger_t::ON_LOGIN, client);
  }

  TriggerZoneActions(zone, {cState, dState}, ZoneTrigger_t::ON_ZONE_IN, client);

  // All zone information is queued and sent together to minimize excess
  // communication
  for (auto enemyState : zone->GetEnemies()) {
    SendEnemyData(enemyState, client, zone, true);
  }

  for (auto npcState : zone->GetNPCs()) {
    if (npcState->GetEntity()->GetState() == HNPC_STATE_SHOW) {
      ShowNPC(zone, {client}, npcState, true);
    }
  }

  for (auto objState : zone->GetServerObjects()) {
    if (objState->GetEntity()->GetState() != ONPC_STATE_HIDE) {
      ShowObject(zone, {client}, objState, true);
    }
  }

  for (auto plasmaPair : zone->GetPlasma()) {
    auto pState = plasmaPair.second;
    auto pSpawn = pState->GetEntity();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_DATA);
    p.WriteS32Little(pState->GetEntityID());
    p.WriteS32Little((int32_t)zone->GetID());
    p.WriteS32Little((int32_t)zoneDef->GetID());
    p.WriteFloat(pState->GetCurrentX());
    p.WriteFloat(pState->GetCurrentY());
    p.WriteFloat(pState->GetCurrentRotation());
    p.WriteS8((int8_t)pSpawn->GetColor());
    p.WriteS8((int8_t)pSpawn->GetPickTime());
    p.WriteS8((int8_t)pSpawn->GetPickSpeed());
    p.WriteU16Little(pSpawn->GetPickSize());

    auto activePoints = pState->GetActivePoints();

    uint8_t pointCount = (uint8_t)activePoints.size();
    p.WriteS8((int8_t)pointCount);
    for (auto point : activePoints) {
      p.WriteS8((int8_t)point->GetID());
      p.WriteS32Little(point->GetState(state->GetWorldCID()));

      p.WriteFloat(point->GetX());
      p.WriteFloat(point->GetY());
      p.WriteFloat(point->GetRotation());
    }

    client->QueuePacket(p);
    ShowEntity(client, pState->GetEntityID(), true);
  }

  for (auto bState : zone->GetBazaars()) {
    auto bazaar = bState->GetEntity();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_DATA);
    p.WriteS32Little(bState->GetEntityID());
    p.WriteS32Little((int32_t)zone->GetID());
    p.WriteS32Little((int32_t)zoneDef->GetID());
    p.WriteFloat(bState->GetCurrentX());
    p.WriteFloat(bState->GetCurrentY());
    p.WriteFloat(bState->GetCurrentRotation());
    p.WriteS32Little((int32_t)bazaar->MarketIDsCount());

    for (uint32_t marketID : bazaar->GetMarketIDs()) {
      auto market = bState->GetCurrentMarket(marketID);
      if (market &&
          market->GetState() == objects::BazaarData::State_t::BAZAAR_INACTIVE) {
        market = nullptr;
      }

      p.WriteU32Little(marketID);
      p.WriteS32Little(market ? (int32_t)market->GetState() : 0);
      p.WriteS32Little(market ? market->GetNPCType() : -1);
      p.WriteString16Little(state->GetClientStringEncoding(),
                            market ? market->GetComment() : "", true);
    }

    client->QueuePacket(p);
    ShowEntity(client, bState->GetEntityID(), true);
  }

  for (auto& cmPair : zone->GetCultureMachines()) {
    auto cmState = cmPair.second;
    auto rental = cmState->GetRentalData();
    bool active = rental && rental->GetActive();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CULTURE_MACHINE_DATA);
    p.WriteS32Little(cmState->GetEntityID());
    p.WriteU32Little(cmPair.first);
    p.WriteU8(active ? 1 : 0);
    p.WriteS32Little((int32_t)zone->GetID());
    p.WriteS32Little((int32_t)zoneDef->GetID());
    p.WriteFloat(cmState->GetCurrentX());
    p.WriteFloat(cmState->GetCurrentY());
    p.WriteFloat(cmState->GetCurrentRotation());
    p.WriteU8(active && rental->GetCharacter() == cState->GetEntityUUID() ? 1
                                                                          : 0);

    client->QueuePacket(p);
    ShowEntity(client, cmState->GetEntityID(), true);
  }

  for (auto lState : zone->GetLootBoxes()) {
    SendLootBoxData(client, lState, nullptr, false, true);
  }

  for (auto allyState : zone->GetAllies()) {
    SendAllyData(allyState, client, zone, true);
  }

  // Send all the queued NPC packets
  client->FlushOutgoing();

  std::list<std::shared_ptr<ChannelClientConnection>> self = {client};
  for (auto oConnection : otherClients) {
    auto oState = oConnection->GetClientState();
    auto oCState = oState->GetCharacterState();
    auto oDState = oState->GetDemonState();

    if (oCState->IsClientVisible()) {
      characterManager->SendOtherCharacterData(self, oState);

      PopEntityForProduction(client, oCState->GetEntityID(), 0);
      ShowEntity(client, oCState->GetEntityID());
    }

    if (oDState->IsClientVisible()) {
      characterManager->SendOtherPartnerData(self, oState);
      PopEntityForProduction(client, oDState->GetEntityID(), 0);
      ShowEntity(client, oDState->GetEntityID());

      if (oDState->GetDeathTimeOut()) {
        UpdateDeathTimeOut(oState, 0, client);
      }
    }
  }

  return true;
}

void ZoneManager::ShowEntity(
    const std::shared_ptr<ChannelClientConnection>& client, int32_t entityID,
    bool queue) {
  std::list<std::shared_ptr<ChannelClientConnection>> clients;
  clients.push_back(client);
  ShowEntity(clients, entityID, queue);
}

void ZoneManager::ShowEntityToZone(const std::shared_ptr<Zone>& zone,
                                   int32_t entityID) {
  auto clients = zone->GetConnectionList();
  ShowEntity(clients, entityID, false);

  // If its an active entity, set it as displayed
  auto activeState = zone->GetActiveEntity(entityID);
  if (activeState &&
      activeState->GetDisplayState() < ActiveDisplayState_t::ACTIVE) {
    activeState->SetDisplayState(ActiveDisplayState_t::ACTIVE);
  }
}

void ZoneManager::ShowDemonToZone(
    const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto dState = client ? client->GetClientState()->GetDemonState() : nullptr;
  if (!zone || !dState) {
    return;
  }

  auto server = mServer.lock();

  bool summonWait =
      dState->GetDisplayState() == ActiveDisplayState_t::AWAITING_SUMMON;
  PopEntityForZoneProduction(zone, dState->GetEntityID(), summonWait ? 2 : 0);
  ShowEntityToZone(zone, dState->GetEntityID());

  // Relay starting state to source client
  server->GetTokuseiManager()->SendCostAdjustments(dState->GetEntityID(),
                                                   client);
  server->GetCharacterManager()->SendMovementSpeed(client, dState, true);
}

void ZoneManager::ShowEntity(
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    int32_t entityID, bool queue) {
  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_ENTITY);
  p.WriteS32Little(entityID);

  ChannelClientConnection::BroadcastPacket(clients, p, queue);
}

void ZoneManager::PopEntityForProduction(
    const std::shared_ptr<ChannelClientConnection>& client, int32_t entityID,
    int32_t type, bool queue) {
  std::list<std::shared_ptr<ChannelClientConnection>> clients;
  clients.push_back(client);
  PopEntityForProduction(clients, entityID, type, queue);
}

void ZoneManager::PopEntityForZoneProduction(const std::shared_ptr<Zone>& zone,
                                             int32_t entityID, int32_t type) {
  auto clients = zone->GetConnectionList();
  PopEntityForProduction(clients, entityID, type, false);
}

void ZoneManager::PopEntityForProduction(
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    int32_t entityID, int32_t type, bool queue) {
  libcomp::Packet p;
  p.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_POP_ENTITY_FOR_PRODUCTION);
  p.WriteS32Little(entityID);
  p.WriteS32Little(type);

  ChannelClientConnection::BroadcastPacket(clients, p, queue);
}

void ZoneManager::RemoveEntitiesFromZone(const std::shared_ptr<Zone>& zone,
                                         const std::list<int32_t>& entityIDs,
                                         int32_t removalMode, bool queue) {
  auto clients = zone->GetConnectionList();
  RemoveEntities(clients, entityIDs, removalMode, queue);
}

void ZoneManager::RemoveEntities(
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    const std::list<int32_t>& entityIDs, int32_t removalMode, bool queue) {
  for (int32_t entityID : entityIDs) {
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_ENTITY);
    p.WriteS32Little(entityID);
    p.WriteS32Little(removalMode);

    ChannelClientConnection::BroadcastPacket(clients, p, true);

    p.Clear();
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_OBJECT);
    p.WriteS32Little(entityID);

    ChannelClientConnection::BroadcastPacket(clients, p, true);
  }

  if (!queue) {
    ChannelClientConnection::FlushAllOutgoing(clients);
  }
}

void ZoneManager::ShowNPC(
    const std::shared_ptr<Zone>& zone,
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    const std::shared_ptr<NPCState>& npcState, bool queue) {
  auto npc = npcState->GetEntity();

  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_NPC_DATA);
  p.WriteS32Little(npcState->GetEntityID());
  p.WriteU32Little(npc->GetID());
  p.WriteS32Little((int32_t)zone->GetID());
  p.WriteS32Little((int32_t)zone->GetDefinitionID());
  p.WriteFloat(npcState->GetCurrentX());
  p.WriteFloat(npcState->GetCurrentY());
  p.WriteFloat(npcState->GetCurrentRotation());

  // Client side display value, mostly replaced with event conditions
  // but still useful for "modal" NPCs that change with game state.
  // See NPCInvisibleData for the matching IDs and criteria.
  p.WriteS16Little(npc->GetDisplayFlag());

  ChannelClientConnection::BroadcastPacket(clients, p, true);

  ShowEntity(clients, npcState->GetEntityID(), true);

  if (!queue) {
    ChannelClientConnection::FlushAllOutgoing(clients);
  }
}

void ZoneManager::ShowObject(
    const std::shared_ptr<Zone>& zone,
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    const std::shared_ptr<ServerObjectState>& objState, bool queue) {
  auto obj = objState->GetEntity();

  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_OBJECT_NPC_DATA);
  p.WriteS32Little(objState->GetEntityID());
  p.WriteU32Little(obj->GetID());
  p.WriteU8(obj->GetState());
  p.WriteS32Little((int32_t)zone->GetID());
  p.WriteS32Little((int32_t)zone->GetDefinitionID());
  p.WriteFloat(objState->GetCurrentX());
  p.WriteFloat(objState->GetCurrentY());
  p.WriteFloat(objState->GetCurrentRotation());

  ChannelClientConnection::BroadcastPacket(clients, p, true);

  ShowEntity(clients, objState->GetEntityID(), true);

  if (!queue) {
    ChannelClientConnection::FlushAllOutgoing(clients);
  }
}

void ZoneManager::FixCurrentPosition(
    const std::shared_ptr<ActiveEntityState>& eState, uint64_t fixUntil,
    uint64_t now) {
  auto zone = eState->GetZone();
  if (zone) {
    if (now == 0) {
      now = ChannelServer::GetServerTime();
    }

    eState->RefreshCurrentPosition(now);

    // In between rotation values do not matter
    float rot = eState->GetDestinationRotation();
    eState->SetCurrentRotation(rot);

    eState->Stop(now);

    float x = eState->GetCurrentX();
    float y = eState->GetCurrentY();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FIX_POSITION);
    p.WriteS32Little(eState->GetEntityID());
    p.WriteFloat(x);
    p.WriteFloat(y);
    p.WriteFloat(rot);

    RelativeTimeMap timeMap;
    timeMap[p.Size()] = now;
    timeMap[p.Size() + 4] = fixUntil;

    auto zConnections = zone->GetConnectionList();
    ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
  }
}

void ZoneManager::ScheduleEntityRemoval(uint64_t time,
                                        const std::shared_ptr<Zone>& zone,
                                        const std::list<int32_t>& entityIDs,
                                        int32_t removeMode) {
  mServer.lock()->ScheduleWork(
      time,
      [](ZoneManager* zoneManager, uint64_t pTime,
         const std::shared_ptr<Zone> pZone, std::list<int32_t> pEntityIDs,
         int32_t pRemoveMode) {
        std::list<int32_t> finalList;
        for (int32_t lootEntityID : pEntityIDs) {
          auto state = pZone->GetEntity(lootEntityID);
          auto aState = std::dynamic_pointer_cast<ActiveEntityState>(state);
          if (state && (!aState || aState->GetDeathTimeOut() <= pTime)) {
            pZone->RemoveEntity(lootEntityID);
            finalList.push_back(lootEntityID);
          }
        }

        if (finalList.size() > 0) {
          zoneManager->RemoveEntitiesFromZone(pZone, finalList, pRemoveMode);
        }
      },
      this, time, zone, entityIDs, removeMode);
}

void ZoneManager::SendLootBoxData(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<LootBoxState>& lState,
    const std::shared_ptr<ActiveEntityState>& eState, bool sendToAll,
    bool queue) {
  auto box = lState->GetEntity();
  auto zone = GetCurrentZone(client);
  if (!zone) {
    return;
  }

  libcomp::Packet p;

  auto lootType = box->GetType();
  switch (lootType) {
    case objects::LootBox::Type_t::BODY: {
      auto enemy = box->GetEnemy();

      p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOOT_BODY_DATA);
      p.WriteS32Little(lState->GetEntityID());
      p.WriteS32Little(eState ? eState->GetEntityID() : -1);
      p.WriteS32Little((int32_t)enemy->GetType());
      p.WriteS32Little((int32_t)zone->GetID());
      p.WriteS32Little((int32_t)zone->GetDefinition()->GetID());
      p.WriteFloat(lState->GetCurrentX());
      p.WriteFloat(lState->GetCurrentY());
      p.WriteFloat(lState->GetCurrentRotation());
      p.WriteU32Little(enemy->GetVariantType());
    } break;
    case objects::LootBox::Type_t::GIFT_BOX:
    case objects::LootBox::Type_t::EGG:
    case objects::LootBox::Type_t::BOSS_BOX:
    case objects::LootBox::Type_t::TREASURE_BOX: {
      p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOOT_BOX_DATA);
      p.WriteS32Little(lState->GetEntityID());
      p.WriteS32Little(eState ? eState->GetEntityID() : -1);
      p.WriteS8((int8_t)lootType);
      p.WriteS32Little((int32_t)zone->GetID());
      p.WriteS32Little((int32_t)zone->GetDefinition()->GetID());
      p.WriteFloat(lState->GetCurrentX());
      p.WriteFloat(lState->GetCurrentY());
      p.WriteFloat(lState->GetCurrentRotation());
      p.WriteFloat(0.f);  // Unknown
    } break;
    default:
      return;
  }

  std::list<std::shared_ptr<ChannelClientConnection>> clients;
  if (sendToAll) {
    clients = zone->GetConnectionList();
  } else {
    clients.push_back(client);
  }

  // Send the data and prepare it to show
  for (auto zClient : clients) {
    zClient->QueuePacketCopy(p);
    PopEntityForProduction(zClient, lState->GetEntityID(), 0, true);
  }

  // Send the loot data if it exists (except for treasure chests)
  if (lootType != objects::LootBox::Type_t::BOSS_BOX &&
      lootType != objects::LootBox::Type_t::TREASURE_BOX) {
    for (auto loot : box->GetLoot()) {
      if (loot) {
        auto characterManager = mServer.lock()->GetCharacterManager();
        characterManager->SendLootItemData(clients, lState, true);
        break;
      }
    }
  }

  // Show the box
  for (auto zClient : clients) {
    ShowEntity(zClient, lState->GetEntityID(), true);
  }

  if (!queue) {
    ChannelClientConnection::FlushAllOutgoing(clients);
  }
}

void ZoneManager::SendBazaarMarketData(
    const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<BazaarState>& bState, uint32_t marketID) {
  auto market = bState->GetCurrentMarket(marketID);

  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_NPC_CHANGED);
  p.WriteS32Little(bState->GetEntityID());
  p.WriteS32Little((int32_t)marketID);
  p.WriteS32Little(market ? (int32_t)market->GetNPCType() : -1);
  p.WriteS32Little(market ? (int32_t)market->GetState() : 0);
  p.WriteString16Little(libcomp::Convert::ENCODING_DEFAULT,
                        market ? market->GetComment() : "", true);

  BroadcastPacket(zone, p);
}

void ZoneManager::SendCultureMachineData(
    const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<CultureMachineState>& cmState) {
  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CULTURE_MACHINE_UPDATE);
  p.WriteS32Little(cmState->GetEntityID());
  p.WriteU8(cmState->GetRentalData() ? 1 : 0);

  BroadcastPacket(zone, p);
}

void ZoneManager::ExpireRentals(const std::shared_ptr<Zone>& zone,
                                uint32_t expireBefore) {
  auto server = mServer.lock();
  auto managerConnection = server->GetManagerConnection();

  bool expireNow = expireBefore == 0;
  if (expireNow) {
    expireBefore = (uint32_t)std::time(0);
  }

  uint32_t currentExpiration = zone->GetNextRentalExpiration();

  auto machines = zone->GetCultureMachines();
  auto bazaars = zone->GetBazaars();

  std::list<std::shared_ptr<objects::BazaarData>> rMarkets;
  for (auto bState : bazaars) {
    for (uint32_t marketID : bState->GetEntity()->GetMarketIDs()) {
      auto market = bState->GetCurrentMarket(marketID);
      if (market && market->GetExpiration() <= expireBefore) {
        bState->SetCurrentMarket(marketID, nullptr);

        // Send the close notification
        auto sellerAccount = market->GetAccount().Get();

        // Relay the packet through the world
        libcomp::Packet p;
        p.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
        p.WriteS32Little(0);
        p.WriteU8((uint8_t)PacketRelayMode_t::RELAY_ACCOUNT);
        p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                              market->GetAccount().GetUUID().ToString(), true);
        p.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_CLOSE);
        p.WriteS32Little(0);

        managerConnection->GetWorldConnection()->SendPacket(p);

        SendBazaarMarketData(zone, bState, marketID);

        rMarkets.push_back(market);
      }
    }
  }

  std::list<std::shared_ptr<objects::CultureData>> rMachines;
  for (auto cmPair : machines) {
    auto cmState = cmPair.second;
    auto rental = cmState->GetRentalData();
    if (rental && rental->GetExpiration() <= expireBefore) {
      cmState->SetRentalData(nullptr);

      // Send the complete notification
      auto renter =
          libcomp::PersistentObject::LoadObjectByUUID<objects::Character>(
              server->GetWorldDatabase(), rental->GetCharacter(), false);
      if (renter) {
        auto cItem = rental->GetItem().Get(server->GetWorldDatabase());

        // Relay the packet through the world
        libcomp::Packet p;
        p.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
        p.WriteS32Little(0);
        p.WriteU8((uint8_t)PacketRelayMode_t::RELAY_CHARACTER);
        p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                              renter->GetName(), true);
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CULTURE_COMPLETE);
        p.WriteS32Little((int32_t)(cItem ? cItem->GetType() : 0));

        managerConnection->GetWorldConnection()->SendPacket(p);
      }

      SendCultureMachineData(zone, cmState);

      rMachines.push_back(rental);
    }
  }

  if (rMachines.size() > 0 || rMarkets.size() > 0) {
    if (rMachines.size() > 0) {
      LogZoneManagerDebug([rMachines, zone]() {
        return libcomp::String("%1 culture machine rental(s) expired in %2s\n")
            .Arg(rMachines.size())
            .Arg(zone->GetDefinitionID());
      });
    }

    if (rMarkets.size() > 0) {
      LogZoneManagerDebug([rMarkets, zone]() {
        return libcomp::String("%1 bazaar market rental(s) expired in %2s\n")
            .Arg(rMarkets.size())
            .Arg(zone->GetDefinitionID());
      });
    }

    auto dbChanges = libcomp::DatabaseChangeSet::Create();
    for (auto machine : rMachines) {
      machine->SetActive(false);
      dbChanges->Update(machine);
    }

    for (auto market : rMarkets) {
      market->SetState(objects::BazaarData::State_t::BAZAAR_INACTIVE);
      dbChanges->Update(market);
    }

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
  }

  uint32_t nextExpiration = zone->SetNextRentalExpiration();
  if (nextExpiration != 0 && nextExpiration != currentExpiration) {
    // If the next run is sooner than what is scheduled, schedule again
    uint32_t now = expireNow ? expireBefore : (uint32_t)std::time(0);
    ServerTime nextTime = ChannelServer::GetServerTime() +
                          ((uint64_t)(nextExpiration - now) * 1000000ULL);

    server->ScheduleWork(
        nextTime,
        [](ZoneManager* zoneManager, const std::shared_ptr<Zone> pZone,
           uint32_t pSynch) { zoneManager->ExpireRentals(pZone, pSynch); },
        this, zone, nextExpiration);

    LogZoneManagerDebug([zone, nextExpiration, now]() {
      return libcomp::String(
                 "Scheduling zone rental expirations for %1 in %2s\n")
          .Arg(zone->GetDefinitionID())
          .Arg(nextExpiration - now);
    });
  }
}

void ZoneManager::SendEnemyData(
    const std::shared_ptr<EnemyState>& enemyState,
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<Zone>& zone, bool queue) {
  std::list<std::shared_ptr<ChannelClientConnection>> clients;
  if (client) {
    clients.push_back(client);
  } else {
    clients = zone->GetConnectionList();
  }

  if (clients.size() == 0) {
    // No one to send the data to
    return;
  }

  auto eBase = enemyState->GetEnemyBase();
  auto stats = enemyState->GetCoreStats();
  auto zoneData = zone->GetDefinition();

  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENEMY_DATA);
  p.WriteS32Little(enemyState->GetEntityID());
  p.WriteS32Little((int32_t)eBase->GetType());
  p.WriteS32Little(enemyState->GetMaxHP());
  p.WriteS32Little(stats->GetHP());
  p.WriteS8(stats->GetLevel());
  p.WriteS32Little((int32_t)zone->GetID());
  p.WriteS32Little((int32_t)zoneData->GetID());

  // Send destination instead of origin so the next move doesn't look
  // off and they are more likely to be valid for attacking
  p.WriteFloat(enemyState->GetDestinationX());
  p.WriteFloat(enemyState->GetDestinationY());
  p.WriteFloat(enemyState->GetDestinationRotation());

  auto statusEffects = enemyState->GetCurrentStatusEffectStates();

  p.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
  for (auto ePair : statusEffects) {
    p.WriteU32Little(ePair.first->GetEffect());
    p.WriteS32Little((int32_t)ePair.second);
    p.WriteU8(ePair.first->GetStack());
  }

  p.WriteU32Little(eBase->GetVariantType());

  for (auto zClient : clients) {
    zClient->QueuePacketCopy(p);
    PopEntityForProduction(zClient, enemyState->GetEntityID(), !client ? 3 : 0,
                           true);
    ShowEntity(zClient, enemyState->GetEntityID(), true);
  }

  if (!queue) {
    ChannelClientConnection::FlushAllOutgoing(clients);
  }

  // If we're sending to the whole zone and its a grouped boss, send that
  // info now too
  if (!client) {
    auto spawn = eBase->GetSpawnSource();
    if (spawn && spawn->GetBossGroup()) {
      SendMultiZoneBossStatus(spawn->GetBossGroup());
    }
  }
}

void ZoneManager::SendAllyData(
    const std::shared_ptr<AllyState>& allyState,
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<Zone>& zone, bool queue) {
  std::list<std::shared_ptr<ChannelClientConnection>> clients;
  if (client) {
    clients.push_back(client);
  } else {
    clients = zone->GetConnectionList();
  }

  if (clients.size() == 0) {
    // No one to send the data to
    return;
  }

  auto stats = allyState->GetCoreStats();
  auto zoneData = zone->GetDefinition();

  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ALLY_DATA);
  p.WriteS32Little(allyState->GetEntityID());
  p.WriteS32Little((int32_t)allyState->GetEntity()->GetType());
  p.WriteS32Little(allyState->GetMaxHP());
  p.WriteS32Little(stats->GetHP());
  p.WriteS8(stats->GetLevel());
  p.WriteS32Little((int32_t)zone->GetID());
  p.WriteS32Little((int32_t)zoneData->GetID());

  // Send destination instead of origin so the next move doesn't look
  // off and they are more likely to be valid for using skills on
  p.WriteFloat(allyState->GetDestinationX());
  p.WriteFloat(allyState->GetDestinationY());
  p.WriteFloat(allyState->GetDestinationRotation());

  auto statusEffects = allyState->GetCurrentStatusEffectStates();

  p.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
  for (auto ePair : statusEffects) {
    p.WriteU32Little(ePair.first->GetEffect());
    p.WriteS32Little((int32_t)ePair.second);
    p.WriteU8(ePair.first->GetStack());
  }

  p.WriteU32Little(allyState->GetEntity()->GetVariantType());

  // Ally NPCs have a unique distinction from enemies that allows them to
  // contextually be treated as enemies to player entities with non-default
  // faction groups (ex: in PvP)
  auto enemyClients = clients;
  enemyClients.remove_if(
      [allyState](const std::shared_ptr<ChannelClientConnection>& c) {
        auto cState = c->GetClientState()->GetCharacterState();
        return cState->SameFaction(allyState);
      });

  std::array<std::list<std::shared_ptr<ChannelClientConnection>>, 2>
      factionClients;
  if (enemyClients.size() > 0) {
    clients.remove_if(
        [allyState](const std::shared_ptr<ChannelClientConnection>& c) {
          auto cState = c->GetClientState()->GetCharacterState();
          return !cState->SameFaction(allyState);
        });

    factionClients[1] = enemyClients;
  }

  factionClients[0] = clients;

  for (size_t i = 0; i < 2; i++) {
    auto faction = factionClients[i];
    if (faction.size() > 0) {
      if (i == 1) {
        p.Seek(0);
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENEMY_DATA);
      }

      for (auto fClient : faction) {
        fClient->QueuePacketCopy(p);
        PopEntityForProduction(fClient, allyState->GetEntityID(),
                               !client ? 3 : 0, true);
        ShowEntity(fClient, allyState->GetEntityID(), true);
      }

      if (!queue) {
        ChannelClientConnection::FlushAllOutgoing(faction);
      }
    }
  }
}

void ZoneManager::HandleDespawns(const std::shared_ptr<Zone>& zone) {
  std::list<int32_t> enemyIDs;

  std::set<int32_t> despawnEntities = zone->GetDespawnEntities();
  if (despawnEntities.size() > 0) {
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    for (int32_t entityID : despawnEntities) {
      auto eState = zone->GetActiveEntity(entityID);
      auto eBase = eState ? eState->GetEnemyBase() : nullptr;
      if (eBase) {
        enemyIDs.push_back(entityID);

        // Remove from combat first
        characterManager->AddRemoveOpponent(false, eState, nullptr);
        zone->RemoveEntity(entityID);
      }
    }

    if (enemyIDs.size() > 0) {
      RemoveEntitiesFromZone(zone, enemyIDs, 7, false);
    }

    if (zone->DiasporaMiniBossUpdated()) {
      server->GetTokuseiManager()->UpdateDiasporaMinibossCount(zone);
    }
  }
}

void ZoneManager::UpdateStatusEffectStates(const std::shared_ptr<Zone>& zone,
                                           uint32_t now) {
  auto entities = zone->GetUpdatedStatusEffectEntities(now);
  if (entities.size() > 0) {
    UpdateStatusEffectStates(zone, now, entities);
  }
}

void ZoneManager::UpdateStatusEffectStates(
    const std::shared_ptr<Zone>& zone, uint32_t now,
    const std::list<std::shared_ptr<ActiveEntityState>>& entities) {
  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto tokuseiManager = server->GetTokuseiManager();

  const static std::set<uint32_t> dgStatusEffectIDs = {
      SVR_CONST.STATUS_DIGITALIZE[0], SVR_CONST.STATUS_DIGITALIZE[1]};

  std::list<libcomp::Packet> zonePackets;
  std::set<uint32_t> added, updated, removed;
  std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
  std::set<std::shared_ptr<ActiveEntityState>> recalc;

  int32_t hpTDamage, mpTDamage, upkeepCost;
  for (auto entity : entities) {
    uint8_t result = entity->PopEffectTicks(
        now, hpTDamage, mpTDamage, upkeepCost, added, updated, removed);
    if (!result) continue;

    // Send removes first in case an effect is removed then added back
    if (removed.size() > 0) {
      libcomp::Packet p;
      if (characterManager->GetRemovedStatusesPacket(p, entity->GetEntityID(),
                                                     removed)) {
        zonePackets.push_back(p);
      }

      recalc.insert(entity);

      // If a digitalize status was removed, update the client state
      for (uint32_t effectID : dgStatusEffectIDs) {
        if (removed.find(effectID) != removed.end()) {
          auto client = server->GetManagerConnection()->GetEntityClient(
              entity->GetEntityID());
          if (client) {
            characterManager->DigitalizeEnd(client);
          }
          break;
        }
      }
    }

    if (added.size() > 0 || updated.size() > 0) {
      auto effectMap = entity->GetStatusEffects();

      std::list<std::shared_ptr<objects::StatusEffect>> active;
      for (uint32_t effectType : added) {
        auto it = effectMap.find(effectType);
        if (it != effectMap.end()) {
          active.push_back(it->second);
        }
      }

      for (uint32_t effectType : updated) {
        auto it = effectMap.find(effectType);
        if (it != effectMap.end()) {
          active.push_back(it->second);
        }
      }

      libcomp::Packet p;
      if (characterManager->GetActiveStatusesPacket(p, entity->GetEntityID(),
                                                    active)) {
        zonePackets.push_back(p);
      }

      recalc.insert(entity);
    }

    bool hpMpRecalc = false;

    // Regen/deal T-damage first
    if (hpTDamage != 0 || mpTDamage != 0) {
      int32_t hpAdjusted, mpAdjusted;
      if (entity->SetHPMP(-hpTDamage, -(mpTDamage + upkeepCost), true, false, 0,
                          hpAdjusted, mpAdjusted)) {
        if (hpAdjusted < 0) {
          entity->CancelStatusEffects(EFFECT_CANCEL_DAMAGE);
        }

        displayStateModified.insert(entity);

        libcomp::Packet p;
        CharacterManager::GetTDamagePacket(p, entity->GetEntityID(), hpAdjusted,
                                           mpAdjusted);
        zonePackets.push_back(p);

        hpMpRecalc = true;
      }
    }

    // Pay upkeep costs next
    if (upkeepCost != 0) {
      int32_t hpAdjusted, mpAdjusted;
      if (entity->SetHPMP(0, -upkeepCost, true, false, 0, hpAdjusted,
                          mpAdjusted)) {
        displayStateModified.insert(entity);

        libcomp::Packet p;
        p.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_SKILL_UPKEEP_COST);
        p.WriteS32Little(entity->GetEntityID());
        p.WriteU32Little((uint32_t)(-mpAdjusted));
        zonePackets.push_back(p);

        hpMpRecalc = true;
      }
    }

    if (hpMpRecalc) {
      tokuseiManager->Recalculate(
          entity,
          std::set<TokuseiConditionType>{TokuseiConditionType::CURRENT_HP,
                                         TokuseiConditionType::CURRENT_MP});
    }

    if (result & 0x02) {
      // Special T-damage effect should occur
      characterManager->ApplyTDamageSpecial(entity);
    }
  }

  if (zonePackets.size() > 0) {
    auto zConnections = zone->GetConnectionList();
    ChannelClientConnection::BroadcastPackets(zConnections, zonePackets);
  }

  for (auto eState : recalc) {
    // Make sure T-damage is sent first
    tokuseiManager->Recalculate(eState, true,
                                std::set<int32_t>{eState->GetEntityID()});
    if (characterManager->RecalculateStats(eState) & ENTITY_CALC_STAT_WORLD) {
      // Do not send twice
      displayStateModified.erase(eState);
    }
  }

  if (displayStateModified.size() > 0) {
    characterManager->UpdateWorldDisplayState(displayStateModified);
  }
}

void ZoneManager::HandleSpecialInstancePopulate(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<Zone>& zone) {
  auto instance = zone->GetInstance();
  if (instance) {
    SendDestinyBox(client, false, true);
  }

  auto instVariant = instance ? instance->GetVariant() : nullptr;
  if (instVariant) {
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    switch (instVariant->GetInstanceType()) {
      case InstanceType_t::TIME_TRIAL:
      case InstanceType_t::MISSION:
      case InstanceType_t::DIGITALIZE: {
        SendInstanceTimer(instance, client, true);
      } break;
      case InstanceType_t::PVP: {
        if (state->GetLastZoneID() != zone->GetID()) {
          // Ready character now and notify the match manager
          /// @todo: determine why initial zone in VS chat is busted
          /// (along with other things if this is sent earlier)
          mServer.lock()->GetMatchManager()->EnterPvP(client,
                                                      instance->GetID());
        }

        for (auto bState : zone->GetPvPBases()) {
          auto base = bState->GetEntity();

          libcomp::Packet p;
          p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PVP_BASE_DATA);
          p.WriteS32Little(bState->GetEntityID());
          p.WriteS32Little((int32_t)zone->GetID());
          p.WriteS32Little((int32_t)zone->GetDefinitionID());
          p.WriteFloat(bState->GetCurrentX());
          p.WriteFloat(bState->GetCurrentY());
          p.WriteFloat(bState->GetCurrentRotation());
          p.WriteS8(base->GetTeam());
          p.WriteU8(base->GetRank());
          p.WriteU8(base->GetSpeed());
          p.WriteS8(1);  // Unknown

          client->QueuePacket(p);
          ShowEntity(client, bState->GetEntityID(), true);
        }
      } break;
      case InstanceType_t::DIASPORA: {
        mServer.lock()->GetMatchManager()->EnterDiaspora(client, zone);

        uint64_t now = ChannelServer::GetServerTime();
        for (auto bState : zone->GetDiasporaBases()) {
          auto base = bState->GetEntity();
          auto obj = base ? base->GetBoundObject() : nullptr;

          uint64_t reset = base->GetResetTime();
          float timeLeft =
              reset < now ? 0.f : (float)((double)(reset - now) / 1000000.0);

          libcomp::Packet p;
          p.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_DIASPORA_BASE_DATA);
          p.WriteS32Little(bState->GetEntityID());
          p.WriteS32Little((int32_t)zone->GetID());
          p.WriteS32Little((int32_t)zone->GetDefinitionID());
          p.WriteFloat(bState->GetCurrentX());
          p.WriteFloat(bState->GetCurrentY());
          p.WriteFloat(bState->GetCurrentRotation());
          p.WriteU32Little(obj ? obj->GetID() : 0);
          p.WriteU32Little(base->GetDefinition()->GetLetter());
          p.WriteFloat(timeLeft);

          client->QueuePacket(p);
          ShowEntity(client, bState->GetEntityID(), true);
        }
      } break;
      case InstanceType_t::DEMON_ONLY: {
        auto characterManager = mServer.lock()->GetCharacterManager();

        // Cancel mount state if the player has it
        characterManager->CancelMount(state);

        // Refresh the demon-only status effect
        StatusEffectChanges effects;
        effects[SVR_CONST.STATUS_DEMON_ONLY] =
            StatusEffectChange(SVR_CONST.STATUS_DEMON_ONLY, 1, true);

        characterManager->AddStatusEffectImmediate(client, cState, effects);

        SendInstanceTimer(instance, client, true);
      } break;
      default:
        break;
    }
  } else if (instance && instance->GetTimeLimitData()) {
    auto state = client->GetClientState();
    if (state->GetLastInstanceID() != instance->GetID() ||
        !state->GetInstanceTimerActive()) {
      // Normal timer running already, send to the client (once)
      SendInstanceTimer(instance, client, true);
    }
  } else if (zone->GetUBMatch()) {
    mServer.lock()->GetMatchManager()->EnterUltimateBattle(client, zone);
  }
}

void ZoneManager::BroadcastPacket(
    const std::shared_ptr<ChannelClientConnection>& client, libcomp::Packet& p,
    bool includeSelf) {
  std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
  for (auto connection : GetZoneConnections(client, includeSelf)) {
    connections.push_back(connection);
  }

  libcomp::TcpConnection::BroadcastPacket(connections, p);
}

void ZoneManager::BroadcastPacket(const std::shared_ptr<Zone>& zone,
                                  libcomp::Packet& p) {
  if (nullptr != zone) {
    std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
    for (auto connectionPair : zone->GetConnections()) {
      connections.push_back(connectionPair.second);
    }

    libcomp::TcpConnection::BroadcastPacket(connections, p);
  }
}

void ZoneManager::SendToRange(
    const std::shared_ptr<ChannelClientConnection>& client, libcomp::Packet& p,
    bool includeSelf) {
  uint64_t now = mServer.lock()->GetServerTime();

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();

  cState->RefreshCurrentPosition(now);

  std::list<std::shared_ptr<libcomp::TcpConnection>> zConnections;
  if (includeSelf) {
    zConnections.push_back(client);
  }

  float rSquared = (float)std::pow(MAX_ENTITY_DRAW_DISTANCE, 2);
  for (auto zConnection : GetZoneConnections(client, false)) {
    auto otherCState = zConnection->GetClientState()->GetCharacterState();
    otherCState->RefreshCurrentPosition(now);

    if (rSquared >= cState->GetDistance(otherCState, true)) {
      zConnections.push_back(zConnection);
    }
  }
  libcomp::TcpConnection::BroadcastPacket(zConnections, p);
}

std::list<std::shared_ptr<ChannelClientConnection>>
ZoneManager::GetZoneConnections(
    const std::shared_ptr<ChannelClientConnection>& client, bool includeSelf) {
  std::list<std::shared_ptr<ChannelClientConnection>> connections;

  auto worldCID = client->GetClientState()->GetWorldCID();
  std::shared_ptr<Zone> zone;
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    auto iter = mEntityMap.find(worldCID);
    if (iter != mEntityMap.end()) {
      zone = mZones[iter->second];
    }
  }

  if (nullptr != zone) {
    for (auto connectionPair : zone->GetConnections()) {
      if (includeSelf || connectionPair.first != worldCID) {
        connections.push_back(connectionPair.second);
      }
    }
  }

  return connections;
}

bool ZoneManager::SpawnEnemy(const std::shared_ptr<Zone>& zone,
                             uint32_t demonID, float x, float y, float rot,
                             const libcomp::String& aiType) {
  auto eState = std::dynamic_pointer_cast<EnemyState>(
      CreateEnemy(zone, demonID, 0, 0, x, y, rot));
  if (eState) {
    auto server = mServer.lock();
    server->GetAIManager()->Prepare(eState, aiType);
    zone->AddEnemy(eState);

    if (TriggerZoneActions(zone, {eState}, ZoneTrigger_t::ON_SPAWN)) {
      // Make sure they still have max HP/MP to start
      auto cs = eState->GetCoreStats();
      cs->SetHP(eState->GetMaxHP());
      cs->SetMP(eState->GetMaxMP());
    }

    SendEnemyData(eState, nullptr, zone, false);

    return true;
  } else {
    return false;
  }
}

std::shared_ptr<ActiveEntityState> ZoneManager::CreateAlly(
    const std::shared_ptr<Zone>& zone, uint32_t demonID, uint32_t spotID,
    float x, float y, float rot) {
  if (!zone || !demonID) {
    return nullptr;
  }

  if (spotID) {
    auto definitionManager = mServer.lock()->GetDefinitionManager();
    auto spots = definitionManager->GetSpotData(zone->GetDynamicMapID());
    auto spotIter = spots.find(spotID);
    if (spotIter != spots.end()) {
      auto zoneData = definitionManager->GetZoneData(zone->GetDefinitionID());

      Point p = GetRandomSpotPoint(spotIter->second, zoneData);
      x = p.x;
      y = p.y;
      rot = spotIter->second->GetRotation();
    } else {
      LogZoneManagerError([zone, spotID]() {
        return libcomp::String(
                   "Attempted to create an ally in zone %1 in an invalid spot: "
                   "%2\n")
            .Arg(zone->GetDefinitionID())
            .Arg(spotID);
      });

      return nullptr;
    }
  }

  auto eState = CreateEnemy(zone, demonID, nullptr, x, y, rot, true);
  if (eState) {
    eState->GetEnemyBase()->SetSpawnSpotID(spotID);
  }

  return eState;
}

std::shared_ptr<ActiveEntityState> ZoneManager::CreateEnemy(
    const std::shared_ptr<Zone>& zone, uint32_t demonID, uint32_t spawnID,
    uint32_t spotID, float x, float y, float rot,
    const libobjgen::UUID& responsibleEntity) {
  auto spawn = zone->GetDefinition()->GetSpawns(spawnID);
  if (!spawn && spawnID) {
    LogZoneManagerError([&]() {
      return libcomp::String("Failed to load spawn ID %1 in zone %2\n")
          .Arg(spawnID)
          .Arg(zone->GetDefinitionID());
    });
  } else if (spawn) {
    // Ignore the demon ID if the spawn is found as these shouldn't differ
    demonID = spawn->GetEnemyType();
  }

  if (!zone || !demonID) {
    return nullptr;
  }

  if (spotID) {
    auto definitionManager = mServer.lock()->GetDefinitionManager();
    auto spots = definitionManager->GetSpotData(zone->GetDynamicMapID());
    auto spotIter = spots.find(spotID);
    if (spotIter != spots.end()) {
      auto zoneData = definitionManager->GetZoneData(zone->GetDefinitionID());

      Point p = GetRandomSpotPoint(spotIter->second, zoneData);
      x = p.x;
      y = p.y;
      rot = spotIter->second->GetRotation();
    } else {
      LogZoneManagerError([zone, spotID]() {
        return libcomp::String(
                   "Attempted to create an enemy in zone %1 in an invalid "
                   "spot: %2\n")
            .Arg(zone->GetDefinitionID())
            .Arg(spotID);
      });

      return nullptr;
    }
  }

  auto eState =
      CreateEnemy(zone, demonID, spawn, x, y, rot, false, responsibleEntity);
  if (eState) {
    eState->GetEnemyBase()->SetSpawnSpotID(spotID);
  }

  return eState;
}

bool ZoneManager::CopyToEnemy(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<ActiveEntityState>& copyState) {
  auto eBase = eState ? eState->GetEnemyBase() : nullptr;
  if (!eBase || !copyState ||
      eState->GetDisplayState() >= ActiveDisplayState_t::ACTIVE) {
    return false;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  if (!copyState->CopyToEnemy(eState, definitionManager)) {
    return false;
  }

  // Copy successful, set core stats recalculate the new state
  auto cs = eBase->GetCoreStats();
  auto extension = eBase->GetExtension();

  cs->SetMaxHP(extension->GetCorrectTbl((size_t)CorrectTbl::HP_MAX));
  cs->SetMaxMP(extension->GetCorrectTbl((size_t)CorrectTbl::MP_MAX));
  cs->SetHP(extension->GetCorrectTbl((size_t)CorrectTbl::HP_MAX));
  cs->SetMP(extension->GetCorrectTbl((size_t)CorrectTbl::MP_MAX));
  cs->SetSTR(extension->GetCorrectTbl((size_t)CorrectTbl::STR));
  cs->SetMAGIC(extension->GetCorrectTbl((size_t)CorrectTbl::MAGIC));
  cs->SetVIT(extension->GetCorrectTbl((size_t)CorrectTbl::VIT));
  cs->SetINTEL(extension->GetCorrectTbl((size_t)CorrectTbl::INT));
  cs->SetSPEED(extension->GetCorrectTbl((size_t)CorrectTbl::SPEED));
  cs->SetLUCK(extension->GetCorrectTbl((size_t)CorrectTbl::LUCK));
  cs->SetLNGR(extension->GetCorrectTbl((size_t)CorrectTbl::LNGR));
  cs->SetSPELL(extension->GetCorrectTbl((size_t)CorrectTbl::SPELL));
  cs->SetSUPPORT(extension->GetCorrectTbl((size_t)CorrectTbl::SUPPORT));
  cs->SetPDEF(extension->GetCorrectTbl((size_t)CorrectTbl::PDEF));
  cs->SetMDEF(extension->GetCorrectTbl((size_t)CorrectTbl::MDEF));

  mServer.lock()->GetTokuseiManager()->Recalculate(eState, false);

  eState->RecalculateStats(definitionManager);

  return true;
}

bool ZoneManager::CopyDemon(const std::shared_ptr<ActiveEntityState>& eState,
                            const std::shared_ptr<objects::Demon>& copyDemon,
                            int32_t copyEntityID) {
  auto eBase = eState ? eState->GetEnemyBase() : nullptr;
  if (!eBase || eState->GetDisplayState() >= ActiveDisplayState_t::ACTIVE) {
    return false;
  }

  // Create a new demon state and calculate as if they belonged to the
  // character still. Ignore shared state bonuses though as those do not
  // actually activate on the enemy.
  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  auto copyState = std::make_shared<DemonState>();
  copyState->SetEntity(copyDemon, definitionManager);
  copyState->SetEntityID(copyEntityID);

  copyState->SetCurrentSkills(
      copyState->GetAllSkills(definitionManager, false));

  copyState->UpdateDemonState(definitionManager);

  // Recalculate effects to get the correct skills and tokusei boosts but
  // remove the entity ID temporarily so they do not recalculate as the
  // character's demon.
  copyState->SetEntityID(0);

  server->GetTokuseiManager()->Recalculate(copyState, false);

  copyState->SetEntityID(copyEntityID);

  return CopyToEnemy(eState, copyState);
}

bool ZoneManager::AddEnemiesToZone(
    const std::list<std::shared_ptr<ActiveEntityState>>& eStates,
    const std::shared_ptr<Zone>& zone, bool staggerSpawn, bool asEncounter,
    const std::list<std::shared_ptr<objects::Action>>& defeatActions) {
  if (!zone) {
    return false;
  }

  for (auto eState : eStates) {
    if (eState->GetEntityType() != EntityType_t::ENEMY &&
        eState->GetEntityType() != EntityType_t::ALLY) {
      LogZoneManagerError([&]() {
        return libcomp::String(
                   "Attempted to add an entity other than an enemy or ally via "
                   "AddEnemiesToZone: %1\n")
            .Arg(zone->GetDefinitionID());
      });

      return false;
    }

    if (eState->GetZone() != zone) {
      LogZoneManagerError([&]() {
        return libcomp::String(
                   "At least one enemy being added to zone %1 was not created "
                   "there\n")
            .Arg(zone->GetDefinitionID());
      });

      return false;
    }
  }

  // Spawn staggering only happens when zones are active
  staggerSpawn &= mActiveZones.find(zone->GetID()) != mActiveZones.end();

  auto server = mServer.lock();
  auto aiManager = server->GetAIManager();

  bool first = true;
  uint64_t staggerTime = 0;
  for (auto eState : eStates) {
    auto spawn = eState->GetEnemyBase()->GetSpawnSource();
    if (spawn) {
      // Prepare spawn based AI
      if (!aiManager->Prepare(eState, spawn->GetAIScriptID())) {
        LogZoneManagerWarning([&]() {
          return libcomp::String("Failed to prepare AI for enemy: %1\n")
              .Arg(spawn->GetAIScriptID());
        });
      }
    } else {
      // Prepare default AI
      aiManager->Prepare(eState, "");
    }

    if (!asEncounter) {
      if (!first && staggerSpawn) {
        if (!staggerTime) {
          staggerTime = ChannelServer::GetServerTime();
        }

        // Spawn every half second
        staggerTime = (uint64_t)(staggerTime + 500000);
      }

      if (eState->GetEntityType() == EntityType_t::ENEMY) {
        auto e = std::dynamic_pointer_cast<EnemyState>(eState);
        zone->AddEnemy(e, staggerTime);
      } else {
        auto a = std::dynamic_pointer_cast<AllyState>(eState);
        zone->AddAlly(a, staggerTime);
      }

      first = false;
    }
  }

  if (asEncounter) {
    zone->CreateEncounter(eStates, staggerSpawn, defeatActions);
  }

  if (zone->DiasporaMiniBossUpdated()) {
    server->GetTokuseiManager()->UpdateDiasporaMinibossCount(zone);
  }

  bool actionsExecuted =
      TriggerZoneActions(zone, eStates, ZoneTrigger_t::ON_SPAWN);

  for (auto& eState : eStates) {
    if (actionsExecuted) {
      // Make sure they still have max HP/MP to start
      auto cs = eState->GetCoreStats();
      cs->SetHP(eState->GetMaxHP());
      cs->SetMP(eState->GetMaxMP());
    }

    if (eState->Ready()) {
      if (eState->GetEntityType() == EntityType_t::ENEMY) {
        auto e = std::dynamic_pointer_cast<EnemyState>(eState);
        SendEnemyData(e, nullptr, zone, false);
      } else {
        auto a = std::dynamic_pointer_cast<AllyState>(eState);
        SendAllyData(a, nullptr, zone, false);
      }
    }
  }

  return true;
}

bool ZoneManager::AddEnemiesToZone(
    std::list<std::shared_ptr<ActiveEntityState>> eStates,
    const std::shared_ptr<Zone>& zone, bool staggerSpawn, bool asEncounter,
    const libcomp::String& defeatEventID) {
  if (!zone) {
    return false;
  }

  std::list<std::shared_ptr<objects::Action>> defeatActions;
  if (asEncounter && !defeatEventID.IsEmpty()) {
    auto startEvent = std::make_shared<objects::ActionStartEvent>();
    startEvent->SetEventID(defeatEventID);
    defeatActions.push_back(startEvent);
  }

  for (auto& eState : eStates) {
    if (!eState) {
      LogZoneManagerError([&]() {
        return libcomp::String("Attempted to add null enemy to zone: %1\n")
            .Arg(zone->GetDefinitionID());
      });

      return false;
    }

    if (zone->GetActiveEntity(eState->GetEntityID())) {
      LogZoneManagerError([&]() {
        return libcomp::String("Attempted to add enemy to zone twice: %1\n")
            .Arg(zone->GetDefinitionID());
      });

      return false;
    }
  }

  return AddEnemiesToZone(eStates, zone, staggerSpawn, asEncounter,
                          defeatActions);
}

bool ZoneManager::LinearReposition(
    const std::shared_ptr<ActiveEntityState>& eState, float x, float y) {
  auto zone = eState ? eState->GetZone() : nullptr;
  if (!zone) {
    return false;
  }

  Point src(eState->GetCurrentX(), eState->GetCurrentY());
  Point dest(x, y);

  Point newPoint = GetLinearPoint(src.x, src.y, dest.x, dest.y,
                                  src.GetDistance(dest), false, zone);

  eState->SetCurrentX(newPoint.x);
  eState->SetCurrentY(newPoint.y);
  eState->SetOriginX(newPoint.x);
  eState->SetOriginY(newPoint.y);
  eState->SetDestinationX(newPoint.x);
  eState->SetDestinationY(newPoint.y);

  return newPoint == dest;
}

bool ZoneManager::UpdateSpawnGroups(
    const std::shared_ptr<Zone>& zone, bool refreshAll, uint64_t now,
    std::shared_ptr<objects::ActionSpawn> actionSource) {
  auto dynamicMap = zone->GetDynamicMap();
  auto zoneDef = zone->GetDefinition();

  // Location ID then group ID (true) or group ID then spot ID (false)
  std::list<std::pair<bool, std::pair<uint32_t, uint32_t>>> groups;
  if (actionSource) {
    auto mode = actionSource->GetMode();

    std::list<std::pair<bool, std::pair<uint32_t, uint32_t>>> checking;
    for (uint32_t slgID : actionSource->GetSpawnLocationGroupIDs()) {
      std::pair<bool, std::pair<uint32_t, uint32_t>> p(
          true, std::pair<uint32_t, uint32_t>(slgID, 0));
      checking.push_back(p);
    }

    std::set<uint32_t> disabledGroupIDs = zone->GetDisabledSpawnGroups();
    for (auto gPair : actionSource->GetSpawnGroupIDs()) {
      if (disabledGroupIDs.find(gPair.first) == disabledGroupIDs.end()) {
        std::pair<bool, std::pair<uint32_t, uint32_t>> p(
            false, std::pair<uint32_t, uint32_t>(gPair.first, gPair.second));
        checking.push_back(p);
      }
    }

    // Despawn works a bit different than normal spawns
    if (mode == objects::ActionSpawn::Mode_t::DESPAWN) {
      // Match enemies in zone on specified locations and
      // group/location pairs
      for (auto eState : zone->GetEnemiesAndAllies(true)) {
        auto eBase = eState->GetEnemyBase();
        if (eBase->GetSpawnGroupID() > 0 ||
            eBase->GetSpawnLocationGroupID() > 0) {
          bool despawn = false;
          for (auto& pair : checking) {
            if (pair.first) {
              // Location
              uint32_t slgID = pair.second.first;
              if (eBase->GetSpawnLocationGroupID() == slgID) {
                despawn = true;
                break;
              }
            } else {
              // Group
              uint32_t sgID = pair.second.first;
              uint32_t slgID = pair.second.second;

              // Use specified location or any if zero
              if (eBase->GetSpawnGroupID() == sgID &&
                  (!slgID || eBase->GetSpawnLocationGroupID() == slgID)) {
                despawn = true;
                break;
              }
            }
          }

          if (despawn) {
            zone->MarkDespawn(eState->GetEntityID());
          }
        }
      }

      return false;
    }

    // Spawn is not a special type, continue processing
    bool spawnValidated = false;
    if (actionSource->GetSpotID() != 0 &&
        (mode == objects::ActionSpawn::Mode_t::ONE_TIME ||
         mode == objects::ActionSpawn::Mode_t::ONE_TIME_RANDOM)) {
      if (zone->SpawnedAtSpot(actionSource->GetSpotID())) {
        // Nothing to do, spawns have already happened at the
        // explicit spot
        return false;
      }

      spawnValidated = true;
    }

    for (auto cPair : checking) {
      auto& gPair = cPair.second;

      bool add = false;
      if (spawnValidated) {
        add = true;
      } else {
        switch (mode) {
          case objects::ActionSpawn::Mode_t::ONE_TIME:
            add = !zone->GroupHasSpawned(gPair.first, cPair.first, false);
            break;
          case objects::ActionSpawn::Mode_t::ONE_TIME_RANDOM:
            if (!zone->GroupHasSpawned(gPair.first, cPair.first, false)) {
              add = true;
            } else {
              // Stop here if any have spawned
              return false;
            }
            break;
          case objects::ActionSpawn::Mode_t::NONE_EXIST:
            add = !zone->GroupHasSpawned(gPair.first, cPair.first, true);
            break;
          case objects::ActionSpawn::Mode_t::NORMAL:
          default:
            add = true;
            break;
        }
      }

      if (add) {
        groups.push_back(cPair);
      }
    }

    if (mode == objects::ActionSpawn::Mode_t::ONE_TIME_RANDOM &&
        groups.size() > 1) {
      auto g = libcomp::Randomizer::GetEntry(groups);
      groups.clear();
      groups.push_back(g);
    }
  } else if (refreshAll) {
    // All spawn location groups will be refreshed
    for (auto slgPair : zoneDef->GetSpawnLocationGroups()) {
      if (slgPair.second->GetRespawnTime() > 0) {
        std::pair<bool, std::pair<uint32_t, uint32_t>> p(
            true, std::pair<uint32_t, uint32_t>(slgPair.first, 0));
        groups.push_back(p);
      }
    }
  } else {
    // Determine normal spawns needed
    if (now == 0) {
      now = ChannelServer::GetServerTime();
    }

    auto slgIDs = zone->GetRespawnLocations(now);
    if (slgIDs.size() == 0) {
      return false;
    }

    for (uint32_t slgID : slgIDs) {
      std::pair<bool, std::pair<uint32_t, uint32_t>> p(
          true, std::pair<uint32_t, uint32_t>(slgID, 0));
      groups.push_back(p);
    }
  }

  if (groups.size() == 0) {
    return false;
  }

  bool containsSimpleSpawns = false;
  bool mergeEncounter = actionSource && actionSource->DefeatActionsCount() > 0;
  std::set<uint32_t> disabledGroupIDs = zone->GetDisabledSpawnGroups();

  std::list<std::list<std::shared_ptr<ActiveEntityState>>> eStateGroups;
  std::list<std::shared_ptr<objects::SpawnGroup>> spawnActionGroups;
  for (auto groupPair : groups) {
    uint32_t sgID =
        groupPair.first ? groupPair.second.second : groupPair.second.first;
    uint32_t slgID = groupPair.first ? groupPair.second.first : 0;
    uint32_t spotID = !groupPair.first ? groupPair.second.second : 0;

    std::set<uint32_t> spotIDs;
    std::list<std::shared_ptr<objects::SpawnLocation>> locations;
    if (actionSource && actionSource->GetSpotID() != 0) {
      // Explicit spot set on the action
      spotID = actionSource->GetSpotID();
    }

    std::shared_ptr<objects::SpawnLocationGroup> slg;

    if (slgID) {
      slg = zoneDef->GetSpawnLocationGroups(slgID);

      if (!slg) {
        LogZoneManagerWarning([&]() {
          return libcomp::String("Skipping invalid spawn location group %1\n")
              .Arg(slgID);
        });

        continue;
      }

      if (!spotID) {
        spotIDs = slg->GetSpotIDs();
      }

      locations = slg->GetLocations();

      // Get the random group now
      std::list<uint32_t> groupIDs;
      for (uint32_t groupID : slg->GetGroupIDs()) {
        if (disabledGroupIDs.find(groupID) == disabledGroupIDs.end()) {
          groupIDs.push_back(groupID);
        }
      }

      if (groupIDs.size() > 0) {
        sgID = libcomp::Randomizer::GetEntry(groupIDs);
      }
    }

    if (sgID == 0) continue;

    if (spotID) {
      // Add the spot to the set and clear it for the selection logic
      // below
      spotIDs.insert(spotID);

      spotID = 0;
    }

    bool useSpotID = dynamicMap && spotIDs.size() > 0;

    if (!useSpotID && locations.size() == 0) continue;

    auto sg = sgID > 0 ? zoneDef->GetSpawnGroups(sgID) : nullptr;
    if (!sg) {
      LogZoneManagerWarning([&]() {
        return libcomp::String("Skipping invalid spawn group %1\n").Arg(sgID);
      });

      continue;
    }

    std::list<std::shared_ptr<ActiveEntityState>>* eStateGroup = 0;
    if (mergeEncounter) {
      // If the entities should all be considered a single encounter,
      // add them all to the same grouping
      if (eStateGroups.size() == 0) {
        eStateGroups.push_front(
            std::list<std::shared_ptr<ActiveEntityState>>());
      }
      eStateGroup = &eStateGroups.front();
    } else if (sg->DefeatActionsCount() == 0) {
      if (!containsSimpleSpawns) {
        eStateGroups.push_front(
            std::list<std::shared_ptr<ActiveEntityState>>());
        containsSimpleSpawns = true;
      }
      eStateGroup = &eStateGroups.front();
    } else {
      eStateGroups.push_back(std::list<std::shared_ptr<ActiveEntityState>>());
      eStateGroup = &eStateGroups.back();
    }

    bool isSpread =
        slg && slg->GetSpotSelection() ==
                   objects::SpawnLocationGroup::SpotSelection_t::SPREAD;

    // Create each entity at a random position in the same location
    std::shared_ptr<channel::ZoneSpotShape> spot;
    std::shared_ptr<objects::SpawnLocation> location;

    if (!isSpread &&
        !SelectSpotAndLocation(useSpotID, spotID, spotIDs, spot, location,
                               dynamicMap, zoneDef, locations)) {
      LogZoneManagerError([sgID, spotID, zone]() {
        return libcomp::String(
                   "Failed to spawn group %1 at unknown spot %2 in zone %3\n")
            .Arg(sgID)
            .Arg(spotID)
            .Arg(zone->GetDefinitionID());
      });

      continue;
    }

    bool locationFailed = false;

    for (auto sPair : sg->GetSpawns()) {
      auto spawn = zoneDef->GetSpawns(sPair.first);
      for (uint16_t i = 0; i < sPair.second; i++) {
        if (isSpread &&
            !SelectSpotAndLocation(useSpotID, spotID, spotIDs, spot, location,
                                   dynamicMap, zoneDef, locations)) {
          LogZoneManagerError([sgID, spotID, zone]() {
            return libcomp::String(
                       "Failed to spawn group %1 at unknown spot %2 in zone "
                       "%3\n")
                .Arg(sgID)
                .Arg(spotID)
                .Arg(zone->GetDefinitionID());
          });

          spotID = 0;

          locationFailed = true;
          break;
        }

        // Use the spot only if a location was not selected as a
        // location being set at this point means either no spot
        // exists or the spot has a SpawnArea redefined
        float x = 0.f, y = 0.f, rot = 0.f;
        if (useSpotID && !location) {
          // Get a random point in the polygon
          Point p = GetRandomSpotPoint(spot->Definition);
          Point center(spot->Definition->GetCenterX(),
                       spot->Definition->GetCenterY());

          // Make sure a straight line can be drawn from the center
          // point so the enemy is not spawned outside of the zone
          p = GetLinearPoint(center.x, center.y, p.x, p.y,
                             center.GetDistance(p), false, zone);

          x = p.x;
          y = p.y;
          rot = spot->Definition->GetRotation();
        } else {
          // Spawn location bounding box points start in the top left corner of
          // the rectangle and extend towards +X/-Y
          auto rPoint =
              GetRandomPoint(location->GetWidth(), location->GetHeight());
          x = location->GetX() + rPoint.x;
          y = location->GetY() - rPoint.y;
          rot = ZoneManager::GetRandomRotation();
        }

        // Create the entity state
        auto state = CreateEnemy(zone, spawn->GetEnemyType(), spawn, x, y, rot);
        if (state) {
          // Set the spawn information
          auto eBase = state->GetEnemyBase();
          eBase->SetSpawnLocation(location);
          eBase->SetSpawnSpotID(spotID);
          eBase->SetSpawnGroupID(sgID);
          eBase->SetSpawnLocationGroupID(slgID);

          if (spawn->GetBossGroup() &&
              !ValidateBossGroup(
                  std::dynamic_pointer_cast<EnemyState>(state))) {
            // Do not fail the whole location
            break;
          }

          // If this is a spread clear the spot ID so we start again.
          if (isSpread) {
            spotID = 0;
          }

          eStateGroup->push_back(state);
        } else {
          locationFailed = true;
          break;
        }
      }

      if (locationFailed) {
        break;
      }
    }

    if (locationFailed) {
      continue;
    }

    if (sg->SpawnActionsCount() > 0) {
      spawnActionGroups.push_back(sg);
    }
  }

  if (eStateGroups.size() > 0) {
    auto server = mServer.lock();

    // Spawn encounters or simple groups
    std::list<std::shared_ptr<objects::Action>> defeatActions;
    for (auto& eStateGroup : eStateGroups) {
      bool stagger =
          !refreshAll || (actionSource && !actionSource->GetNoStagger());
      bool encounterSpawn =
          !containsSimpleSpawns || eStateGroup != eStateGroups.front();

      if (actionSource) {
        defeatActions = actionSource->GetDefeatActions();
      } else {
        defeatActions.clear();
      }

      AddEnemiesToZone(eStateGroup, zone, stagger, encounterSpawn,
                       defeatActions);
    }

    // Fire spawn group actions
    for (auto sg : spawnActionGroups) {
      ActionOptions options;
      options.GroupID = sg->GetID();

      server->GetActionManager()->PerformActions(nullptr, sg->GetSpawnActions(),
                                                 0, zone, options);
    }

    return true;
  }

  return false;
}

bool ZoneManager::UpdateStaggeredSpawns(const std::shared_ptr<Zone>& zone,
                                        uint64_t now) {
  auto stagger = zone->UpdateStaggeredSpawns(now);
  for (auto eState : stagger) {
    if (eState->GetEntityType() == EntityType_t::ENEMY) {
      auto e = std::dynamic_pointer_cast<EnemyState>(eState);
      SendEnemyData(e, nullptr, zone, false);
    } else {
      auto a = std::dynamic_pointer_cast<AllyState>(eState);
      SendAllyData(a, nullptr, zone, false);
    }
  }

  return stagger.size() > 0;
}

bool ZoneManager::SelectSpotAndLocation(
    bool useSpotID, uint32_t& spotID, const std::set<uint32_t>& spotIDs,
    std::shared_ptr<channel::ZoneSpotShape>& spot,
    std::shared_ptr<objects::SpawnLocation>& location,
    std::shared_ptr<DynamicMap>& dynamicMap,
    std::shared_ptr<objects::ServerZone>& zoneDef,
    std::list<std::shared_ptr<objects::SpawnLocation>>& locations) {
  spot = nullptr;
  location = nullptr;

  if (useSpotID) {
    if (!spotID) {
      spotID = libcomp::Randomizer::GetEntry(spotIDs);
    }

    auto spotPair = dynamicMap->Spots.find(spotID);
    if (spotPair != dynamicMap->Spots.end()) {
      spot = spotPair->second;

      // If the spot is defined with a spawn area, use that as
      // the AI wandering region
      auto serverSpot = zoneDef->GetSpots(spotID);
      if (serverSpot) {
        location = serverSpot->GetSpawnArea();
      }
    } else {
      return false;
    }
  } else {
    location = libcomp::Randomizer::GetEntry(locations);
  }

  return true;
}

bool ZoneManager::MoveToZoneChannel(
    const std::shared_ptr<ChannelClientConnection>& client, uint32_t zoneID,
    uint32_t dynamicMapID,
    const std::shared_ptr<objects::InstanceAccess>& toInstance, float x,
    float y, float rot) {
  auto state = client->GetClientState();
  auto server = mServer.lock();

  auto zoneData =
      server->GetServerDataManager()->GetZoneData(zoneID, dynamicMapID);
  if (!zoneData) {
    LogZoneManagerError([&]() {
      return libcomp::String(
                 "Attempted to move player to another channel for invalid zone "
                 "%1 (%2): %3\n")
          .Arg(zoneID)
          .Arg(dynamicMapID)
          .Arg(state->GetAccountUID().ToString());
    });

    return false;
  }

  dynamicMapID = zoneData->GetDynamicMapID();

  uint8_t channelID = 0;
  if (toInstance) {
    channelID = toInstance->GetChannelID();
  } else if (!zoneData->GetGlobal()) {
    // Check if current instance access has the zone
    auto instAccess = GetInstanceAccess(state->GetWorldCID());
    if (!instAccess) {
      LogZoneManagerError([&]() {
        return libcomp::String(
                   "Attempted to move player to instance zone %1 (%2) with no "
                   "access: %3\n")
            .Arg(zoneID)
            .Arg(dynamicMapID)
            .Arg(state->GetAccountUID().ToString());
      });

      return false;
    }

    if (!server->GetServerDataManager()->ExistsInInstance(
            instAccess->GetDefinitionID(), zoneID, dynamicMapID)) {
      LogZoneManagerError([&]() {
        return libcomp::String(
                   "Attempted to move player to instance zone %1 (%2) that did "
                   "not match their current access: %3\n")
            .Arg(zoneID)
            .Arg(dynamicMapID)
            .Arg(state->GetAccountUID().ToString());
      });

      return false;
    }

    channelID = instAccess->GetChannelID();
  } else {
    auto sharedConfig = server->GetWorldSharedConfig();
    if (sharedConfig->ChannelDistributionCount()) {
      channelID = sharedConfig->GetChannelDistribution(zoneData->GetGroupID());
    } else {
      channelID = server->GetChannelID();
    }
  }

  auto channelLogin = state->GetChannelLogin();
  if (channelLogin && channelLogin->GetFromChannel() != (int8_t)channelID) {
    if (channelLogin->GetToChannel() != (int8_t)channelID) {
      LogZoneManagerError([&]() {
        return libcomp::String(
                   "Attempted to move player to two different channels: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      return false;
    } else {
      // Nothing to do
      return true;
    }
  }

  if (!zoneData->GetGlobal()) {
    // Send move message now
    SendAccessMessage(toInstance, true, client);
  }

  if (toInstance) {
    // Get default x, y and rotation for the zone
    x = zoneData->GetStartingX();
    y = zoneData->GetStartingY();
    rot = zoneData->GetStartingRotation();
  }

  auto character = state->GetCharacterState()->GetEntity();
  if (character) {
    character->SetLogoutZone(zoneID);
    character->SetLogoutInstance(toInstance ? toInstance->GetInstanceID() : 0);
    character->SetLogoutX(x);
    character->SetLogoutY(y);
    character->SetLogoutRotation(rot);
  }

  server->GetAccountManager()->PrepareChannelChange(client, zoneID,
                                                    dynamicMapID, channelID);

  if (state->GetZone()) {
    // Handle like a logout
    LeaveZone(client, true, 0, 0);
  }

  // Request channel change from client (will relay back to world)
  libcomp::Packet request;
  request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHANNEL_CHANGED);
  request.WriteS8((int8_t)channelID);

  client->SendPacket(request);

  return true;
}

bool ZoneManager::UpdatePlasma(const std::shared_ptr<Zone>& zone,
                               uint64_t now) {
  if (zone->GetDefinition()->PlasmaSpawnsCount() == 0) {
    return false;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto zoneData = definitionManager->GetZoneData(zone->GetDefinitionID());

  auto spots =
      definitionManager->GetSpotData(zone->GetDefinition()->GetDynamicMapID());
  for (auto plasmaPair : zone->GetPlasma()) {
    auto pState = plasmaPair.second;
    auto pSpawn = pState->GetEntity();

    if (pState->HasStateChangePoints(true, now)) {
      auto spotIter = spots.find(pSpawn->GetSpotID());

      auto hiddenPoints = pState->PopRespawnPoints(now);

      libcomp::Packet notify;
      notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_REPOP);
      notify.WriteS32Little(pState->GetEntityID());
      notify.WriteS8((int8_t)hiddenPoints.size());

      for (auto point : hiddenPoints) {
        if (spotIter != spots.end()) {
          Point rPoint = GetRandomSpotPoint(spotIter->second, zoneData);

          point->SetX(rPoint.x);
          point->SetY(rPoint.y);
        } else {
          // Default to the explicit location
          point->SetX(pState->GetCurrentX());
          point->SetY(pState->GetCurrentY());
        }

        point->Refresh();

        notify.WriteS8((int8_t)point->GetID());
        notify.WriteS32Little(point->GetState());

        notify.WriteFloat(point->GetX());
        notify.WriteFloat(point->GetY());
        notify.WriteFloat(point->GetRotation());
      }

      BroadcastPacket(zone, notify);
    }

    if (pState->HasStateChangePoints(false, now)) {
      std::list<uint32_t> pointIDs;
      for (auto hidePoint : pState->PopHidePoints(now)) {
        pointIDs.push_back(hidePoint->GetID());
      }

      if (pointIDs.size() > 0) {
        libcomp::Packet notify;
        pState->GetPointStatusData(notify, pointIDs);

        BroadcastPacket(zone, notify);
      }
    }
  }

  return true;
}

void ZoneManager::FailPlasma(
    const std::shared_ptr<ChannelClientConnection>& client, int32_t plasmaID,
    int8_t pointID) {
  // Set the result first
  auto state = client->GetClientState();
  auto zone = state->GetZone();
  auto pState =
      zone ? std::dynamic_pointer_cast<PlasmaState>(zone->GetEntity(plasmaID))
           : nullptr;

  auto point = pState ? pState->SetPickResult((uint32_t)pointID,
                                              state->GetWorldCID(), -1)
                      : nullptr;
  if (point && !pointID) {
    pointID = (int8_t)point->GetID();
  }

  if (point) {
    // Send the faillure notification to the player next
    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_END);
    notify.WriteS32Little(plasmaID);
    notify.WriteS8(pointID);
    notify.WriteS32Little(1);  // Failed

    client->QueuePacket(notify);

    // Now end the system event
    mServer.lock()->GetEventManager()->HandleEvent(client, nullptr);

    // Lastly send the failure to the zone
    notify.Clear();
    pState->GetPointStatusData(notify, point->GetID());
    BroadcastPacket(zone, notify);

    client->FlushOutgoing();
  }
}

Point ZoneManager::RotatePoint(const Point& p, const Point& origin,
                               float radians) {
  float xDelta = p.x - origin.x;
  float yDelta = p.y - origin.y;

  return Point(
      (float)((xDelta * cos(radians)) - (yDelta * sin(radians))) + origin.x,
      (float)((xDelta * sin(radians)) + (yDelta * cos(radians))) + origin.y);
}

void ZoneManager::SyncInstanceAccess(
    std::list<std::shared_ptr<objects::InstanceAccess>> updates,
    std::list<std::shared_ptr<objects::InstanceAccess>> removes) {
  std::list<std::shared_ptr<objects::InstanceAccess>> notify;

  std::lock_guard<libcomp::Mutex> lock(mLock);
  for (auto update : updates) {
    // If we don't have an instance ID here, don't update yet
    if (!update->GetInstanceID()) continue;

    std::shared_ptr<objects::InstanceAccess> existing;
    if (update->GetIsLocal()) {
      auto it = mZoneInstances.find(update->GetInstanceID());
      if (it != mZoneInstances.end()) {
        existing = it->second->GetAccess();
      }
    }

    if (!existing) {
      // Try to retrieve from any current accesses
      for (int32_t worldCID : update->GetAccessCIDs()) {
        auto it = mZoneInstanceAccess.find(worldCID);
        if (it != mZoneInstanceAccess.end() &&
            it->second->GetRequestID() == update->GetRequestID()) {
          existing = it->second;
          break;
        }
      }
    }

    bool sendCreateMessage = !existing;
    if (existing) {
      // Update access (only removals can happen)
      auto currentCIDs = existing->GetAccessCIDs();
      for (int32_t cid : currentCIDs) {
        if (!update->AccessCIDsContains(cid)) {
          existing->RemoveAccessCIDs(cid);
        }
      }

      // If the instance ID was set, add it here
      if (!existing->GetInstanceID() &&
          existing->GetInstanceID() != update->GetInstanceID()) {
        existing->SetInstanceID(update->GetInstanceID());
      }
    } else {
      // Insert new
      for (int32_t worldCID : update->GetAccessCIDs()) {
        auto other = mZoneInstanceAccess[worldCID];
        mZoneInstanceAccess[worldCID] = update;

        if (other && other != update) {
          other->RemoveAccessCIDs(worldCID);
          if (other->GetIsLocal() && !other->AccessCIDsCount()) {
            RemoveInstance(other->GetInstanceID());
          }
        }
      }
    }

    if (sendCreateMessage) {
      notify.push_back(update);
    }
  }

  for (auto remove : removes) {
    // Remove all access then remove the instance if its local
    std::set<int32_t> removeCIDs;
    for (auto& pair : mZoneInstanceAccess) {
      if (pair.second->GetRequestID() == remove->GetRequestID()) {
        removeCIDs.insert(pair.first);
      }
    }

    for (int32_t worldCID : removeCIDs) {
      mZoneInstanceAccess.erase(worldCID);
    }

    if (remove->GetIsLocal()) {
      RemoveInstance(remove->GetInstanceID());
    }
  }

  // Send any create messages needed
  if (notify.size() > 0) {
    for (auto n : notify) {
      SendAccessMessage(n, false);
    }
  }
}

std::shared_ptr<ActiveEntityState> ZoneManager::CreateEnemy(
    const std::shared_ptr<Zone>& zone, uint32_t demonID,
    const std::shared_ptr<objects::Spawn>& spawn, float x, float y, float rot,
    bool asAlly, const libobjgen::UUID& responsibleEntity) {
  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto def = definitionManager->GetDevilData(demonID);

  if (nullptr == def) {
    LogZoneManagerError([&]() {
      return libcomp::String("Attempted to spawn invalid demon: %1\n")
          .Arg(demonID);
    });

    return nullptr;
  } else if (spawn && spawn->GetBossGroup() &&
             !zone->GetDefinition()->GetGlobalBossGroup()) {
    LogZoneManagerError([&]() {
      return libcomp::String(
                 "Attempted to spawn a multi-zone boss in an invalid zone %1: "
                 "%2\n")
          .Arg(zone->GetDefinitionID())
          .Arg(spawn->GetID());
    });

    return nullptr;
  }

  auto stats = libcomp::PersistentObject::New<objects::EntityStats>();
  stats->SetLevel(spawn && spawn->GetLevel() > 0
                      ? spawn->GetLevel()
                      : (int8_t)def->GetGrowth()->GetBaseLevel());
  server->GetCharacterManager()->CalculateDemonBaseStats(nullptr, stats, def);

  std::shared_ptr<ActiveEntityState> state;
  std::shared_ptr<objects::EnemyBase> eBase;
  if (!asAlly &&
      (!spawn || spawn->GetCategory() != objects::Spawn::Category_t::ALLY)) {
    // Building an enemy
    auto enemy = std::make_shared<objects::Enemy>();
    enemy->SetCoreStats(stats);
    enemy->SetType(demonID);
    enemy->SetVariantType(spawn ? spawn->GetVariantType() : 0);
    enemy->SetSpawnSource(spawn);
    eBase = enemy;

    auto eState = std::make_shared<EnemyState>();
    eState->SetResponsibleEntity(responsibleEntity);
    eState->SetEntity(enemy, definitionManager);
    state = eState;
  } else {
    // Building an ally
    auto ally = std::make_shared<objects::Ally>();
    ally->SetCoreStats(stats);
    ally->SetType(demonID);
    ally->SetVariantType(spawn ? spawn->GetVariantType() : 0);
    ally->SetSpawnSource(spawn);
    eBase = ally;

    auto aState = std::make_shared<AllyState>();
    aState->SetEntity(ally, definitionManager);
    state = aState;
  }

  eBase->SetCanRevive(spawn && spawn->GetCanRevive());

  state->SetEntityID(server->GetNextEntityID());
  state->SetOriginX(x);
  state->SetOriginY(y);
  state->SetOriginRotation(rot);
  state->SetDestinationX(x);
  state->SetDestinationY(y);
  state->SetDestinationRotation(rot);
  state->SetCurrentX(x);
  state->SetCurrentY(y);
  state->SetCurrentRotation(rot);
  state->SetStatusEffectsActive(true, definitionManager);
  state->SetKillValue(spawn ? spawn->GetKillValue() : 0);
  state->SetZone(zone);

  server->GetTokuseiManager()->Recalculate(state);
  state->RecalculateStats(definitionManager);

  // Reset HP to max to account for extra HP boosts
  stats->SetHP(state->GetMaxHP());

  return state;
}

void ZoneManager::UpdateActiveZoneStates() {
  auto serverTime = ChannelServer::GetServerTime();

  bool refreshTracking = false;
  std::list<std::shared_ptr<Zone>> zones;
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    if (mTrackingRefresh && serverTime >= mTrackingRefresh) {
      // Refresh again 10 seconds from now
      mTrackingRefresh = serverTime + (ServerTime)10000000ULL;
      refreshTracking = true;
    }

    for (auto uniqueID : mActiveZones) {
      zones.push_back(mZones[uniqueID]);
    }
  }

  auto server = mServer.lock();

  // Performance timer to measure tasks.
  PerformanceTimer perf(server.get());
  PerformanceTimer perf2(server.get());

  // Spin through entities with updated status effects
  perf.Start();
  auto worldClock = server->GetWorldClockTime();
  for (auto zone : zones) {
    UpdateStatusEffectStates(zone, worldClock.SystemTime);
  }
  perf.Stop("UpdateStatusEffectStates");

  auto aiManager = server->GetAIManager();

  bool isNight = worldClock.IsNight();

  for (auto zone : zones) {
    perf.Start();

    // Despawn first
    HandleDespawns(zone);

    // Stop combat next
    for (int32_t combatantID : zone->GetCombatantIDs()) {
      auto entity = zone->StartStopCombat(combatantID, serverTime, true);
      if (entity) {
        server->GetCharacterManager()->AddRemoveOpponent(false, entity,
                                                         nullptr);
      }
    }

    // Update active AI controlled entities
    perf2.Start();
    aiManager->UpdateActiveStates(zone, serverTime, isNight);
    perf2.Stop("Zone AI");

    // Update staggered spawns before doing any normal spawns
    if (zone->HasStaggeredSpawns(serverTime)) {
      UpdateStaggeredSpawns(zone, serverTime);
    }

    if (zone->HasRespawns()) {
      // Spawn new enemies next (since they should not immediately act)
      UpdateSpawnGroups(zone, false, serverTime);

      // Now update plasma spawns
      UpdatePlasma(zone, serverTime);
    }

    mTimeRestrictUpdatedZones.erase(zone->GetID());

    perf.Stop(libcomp::String("Zone %1").Arg(zone->GetDefinitionID()));
  }

  // Get any updated time restricted zones and clear the list
  // after retrieval (essentially they "unfreeze" momentarily)
  {
    zones.clear();

    std::lock_guard<libcomp::Mutex> lock(mLock);
    if (mTimeRestrictUpdatedZones.size() > 0) {
      for (auto uniqueID : mTimeRestrictUpdatedZones) {
        zones.push_back(mZones[uniqueID]);
      }

      mTimeRestrictUpdatedZones.clear();
    }
  }

  // Handle all time restrict updated zones
  perf.Start();
  for (auto zone : zones) {
    // Despawn first
    HandleDespawns(zone);

    if (zone->HasRespawns()) {
      // Spawn next
      UpdateSpawnGroups(zone, false, serverTime);
    }
  }
  perf.Stop("TimeRestrictedSpawns");

  if (refreshTracking) {
    perf.Start();

    // Refresh all tracking zones and boss groups as needed
    zones.clear();
    std::set<uint32_t> activeGroups;
    {
      std::lock_guard<libcomp::Mutex> lock(mLock);
      for (uint32_t uniqueID : mActiveTrackedZones) {
        zones.push_back(mZones[uniqueID]);
      }

      for (auto& pair : mGlobalBossZones) {
        for (auto uniqueID : pair.second) {
          if (mActiveZones.find(uniqueID) != mActiveZones.end()) {
            activeGroups.insert(pair.first);
            break;
          }
        }
      }
    }

    for (auto zone : zones) {
      if (zone->GetInstanceType() == InstanceType_t::DIASPORA) {
        // Track the entire zone
        UpdateTrackedZone(zone);
      } else {
        // Track teams in the zone
        std::set<std::shared_ptr<objects::Team>> teams;
        for (auto client : zone->GetConnectionList()) {
          auto team = client->GetClientState()->GetTeam();
          if (team) {
            teams.insert(team);
          }
        }

        for (auto team : teams) {
          UpdateTrackedTeam(team, zone);
        }
      }
    }

    for (uint32_t groupID : activeGroups) {
      SendMultiZoneBossStatus(groupID);
    }

    perf.Stop("refreshTracking");
  }
}

void ZoneManager::Warp(const std::shared_ptr<ChannelClientConnection>& client,
                       const std::shared_ptr<ActiveEntityState>& eState,
                       float xPos, float yPos, float rot) {
  ServerTime timestamp = ChannelServer::GetServerTime();

  eState->SetOriginX(xPos);
  eState->SetOriginY(yPos);
  eState->SetOriginTicks(timestamp);
  eState->SetDestinationX(xPos);
  eState->SetDestinationY(yPos);
  eState->SetDestinationTicks(timestamp);
  eState->SetCurrentX(xPos);
  eState->SetCurrentY(yPos);

  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_WARP);
  p.WriteS32Little(eState->GetEntityID());
  p.WriteFloat(xPos);
  p.WriteFloat(yPos);
  p.WriteFloat(0.0f);  // Unknown
  p.WriteFloat(rot);

  RelativeTimeMap timeMap;
  timeMap[p.Size()] = timestamp;

  std::list<std::shared_ptr<ChannelClientConnection>> connections;
  if (client) {
    connections = GetZoneConnections(client, true);
  } else if (eState->GetZone()) {
    connections = eState->GetZone()->GetConnectionList();
  }

  ChannelClientConnection::SendRelativeTimePacket(connections, p, timeMap);
}

void ZoneManager::HandleTimedActions(const WorldClock& clock,
                                     const WorldClockTime& lastTrigger) {
  std::list<std::shared_ptr<Zone>> timeRestrictZones;
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    for (uint32_t zoneID : mAllTimeRestrictZones) {
      auto zone = mZones[zoneID];
      if (zone) {
        timeRestrictZones.push_back(zone);
      }
    }
  }

  std::set<uint32_t> updated;
  for (auto zone : timeRestrictZones) {
    if (zone->UpdateTimedSpawns(clock)) {
      updated.insert(zone->GetID());
    }
  }

  // Fire zone time event triggers
  if (lastTrigger.IsSet()) {
    int32_t timeFrom = (int32_t)(lastTrigger.Hour * 100 + lastTrigger.Min);
    int32_t timeTo = (int32_t)(clock.Hour * 100 + clock.Min);
    bool timeChange = timeTo != timeFrom;
    bool timeRoll = timeTo < timeFrom;

    int32_t sTimeFrom =
        (int32_t)(lastTrigger.SystemHour * 100 + lastTrigger.SystemMin);
    int32_t sTimeTo = (int32_t)(clock.SystemHour * 100 + clock.SystemMin);
    bool sTimeChange = sTimeTo != sTimeFrom;
    bool sTimeRoll = sTimeTo < sTimeFrom;

    bool moonChange = clock.MoonPhase != lastTrigger.MoonPhase;
    bool moonRoll = clock.MoonPhase < lastTrigger.MoonPhase;

    // Gather and evaluate all time triggers
    auto triggers = mGlobalTimeTriggers;
    for (auto zone : timeRestrictZones) {
      for (auto trigger : zone->GetTimeTriggers()) {
        triggers.push_back(trigger);
      }
    }

    std::set<std::shared_ptr<objects::ServerZoneTrigger>> fired;
    for (auto trigger : triggers) {
      int32_t from = 0;
      int32_t to = 0;
      bool rollOver = false;

      switch (trigger->GetTrigger()) {
        case ZoneTrigger_t::ON_TIME:
          if (!timeChange) continue;
          from = timeFrom;
          to = timeTo;
          rollOver = timeRoll;
          break;
        case ZoneTrigger_t::ON_SYSTEMTIME:
          if (!sTimeChange) continue;
          from = sTimeFrom;
          to = sTimeTo;
          rollOver = sTimeRoll;
          break;
        case ZoneTrigger_t::ON_MOONPHASE:
          if (!moonChange) continue;
          from = lastTrigger.MoonPhase;
          to = clock.MoonPhase;
          rollOver = moonRoll;
          break;
        default:
          break;
      }

      int32_t val = trigger->GetValue();
      if ((!rollOver && from < val && val <= to) ||
          (rollOver && (from < val || val <= to))) {
        fired.insert(trigger);
      }
    }

    for (auto zone : timeRestrictZones) {
      for (auto trigger : zone->GetTimeTriggers()) {
        if (fired.find(trigger) != fired.end()) {
          LogZoneManagerDebug([&]() {
            return libcomp::String("Triggering timed actions in zone %1\n")
                .Arg(zone->GetDefinitionID());
          });

          mServer.lock()->GetActionManager()->PerformActions(
              nullptr, trigger->GetActions(), 0, zone);
          updated.insert(zone->GetID());
        }
      }
    }

    // Global triggers always fire after zone specific ones
    for (auto trigger : mGlobalTimeTriggers) {
      if (fired.find(trigger) != fired.end()) {
        LogZoneManagerDebugMsg("Triggering global timed actions\n");

        mServer.lock()->GetActionManager()->PerformActions(
            nullptr, trigger->GetActions(), 0);
      }
    }
  }

  if (updated.size() > 0) {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    for (uint32_t zoneID : updated) {
      mTimeRestrictUpdatedZones.insert(zoneID);
    }
  }
}

bool ZoneManager::StartInstanceTimer(
    const std::shared_ptr<ZoneInstance>& instance) {
  if (!instance) {
    return false;
  }

  auto instVariant = instance->GetVariant();
  auto instType =
      instVariant ? instVariant->GetInstanceType() : InstanceType_t::NORMAL;

  bool sendTimer = false;
  bool scheduleExpiration = false;
  switch (instType) {
    case InstanceType_t::TIME_TRIAL: {
      // Timer counts up, set start time only
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (instance->GetTimerStart() == 0) {
        uint64_t now = ChannelServer::GetServerTime();
        uint16_t duration = instVariant->GetTimePoints(0);

        uint64_t expireTime = now + (uint64_t)duration * 1000000ULL;
        instance->SetTimerStart(now);
        instance->SetTimerExpire(expireTime);

        scheduleExpiration = true;
      }
    } break;
    case InstanceType_t::PVP: {
      // Expiration already set, setup expiration event and set
      // the start time
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (instance->GetTimerExpire() && !instance->GetTimerStart()) {
        uint64_t now = ChannelServer::GetServerTime();
        instance->SetTimerStart(now);

        scheduleExpiration = true;
      }
    } break;
    case InstanceType_t::DEMON_ONLY: {
      // Timer counts down, set start and expire time
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (instance->GetTimerStart() == 0) {
        uint64_t now = ChannelServer::GetServerTime();
        uint16_t duration = instVariant->GetTimePoints(0);

        size_t partyMemberCount = instance->OriginalAccessCIDsCount();
        if (partyMemberCount > 1) {
          // Reduce the time by a calculated or fixed amount based
          // on how many party members existed on instance creation
          uint16_t reduction = 0;
          if (instVariant->TimePointsCount() == 4) {
            reduction = instVariant->GetTimePoints(3);
          } else {
            reduction = (uint16_t)(floor((float)duration / 600.f) * 30);
          }

          duration =
              (uint16_t)(duration - (reduction * (partyMemberCount - 1)));
        }

        uint64_t expireTime = now + (uint64_t)duration * 1000000ULL;
        instance->SetTimerStart(now);
        instance->SetTimerExpire(expireTime);

        scheduleExpiration = true;
      }
    } break;
    case InstanceType_t::DIASPORA: {
      // Timer counts down, set start and expire time
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (instance->GetTimerStart() == 0) {
        uint64_t now = ChannelServer::GetServerTime();
        uint16_t duration = instVariant->GetTimePoints(
            instance->GetMatch()->GetPhase() ? 1 : 0);

        uint64_t expireTime = now + (uint64_t)duration * 1000000ULL;
        instance->SetTimerStart(now);
        instance->SetTimerExpire(expireTime);

        scheduleExpiration = sendTimer = true;
      }
    } break;
    case InstanceType_t::MISSION: {
      // Timer counts down, set start and expire time
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (instance->GetTimerStart() == 0) {
        uint64_t now = ChannelServer::GetServerTime();
        uint16_t duration = instVariant->GetTimePoints(0);

        uint64_t expireTime = now + (uint64_t)duration * 1000000ULL;
        instance->SetTimerStart(now);
        instance->SetTimerExpire(expireTime);

        scheduleExpiration = sendTimer = true;
      }
    } break;
    case InstanceType_t::DIGITALIZE: {
      // Timer counts up, set start time only
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (instance->GetTimerStart() == 0) {
        uint64_t now = ChannelServer::GetServerTime();
        instance->SetTimerStart(now);
      }
    } break;
    case InstanceType_t::NORMAL:
    default: {
      auto timeLimitData = instance->GetTimeLimitData();
      if (!timeLimitData) {
        return false;
      }

      // Timer counts down, set start and expire time from time limit
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (instance->GetTimerStart() == 0) {
        uint64_t now = ChannelServer::GetServerTime();
        uint16_t duration = timeLimitData->GetDuration();

        uint64_t expireTime = now + (uint64_t)duration * 1000000ULL;
        instance->SetTimerStart(now);
        instance->SetTimerExpire(expireTime);

        scheduleExpiration = sendTimer = true;
      }
    } break;
  }

  if (scheduleExpiration) {
    ScheduleTimerExpiration(instance);
  }

  if (sendTimer) {
    SendInstanceTimer(instance, nullptr, false);
  }

  return true;
}

bool ZoneManager::ExtendInstanceTimer(
    const std::shared_ptr<ZoneInstance>& instance, uint32_t seconds) {
  if (!instance->GetTimerStart() || !instance->GetTimerExpire() ||
      instance->GetTimerStop()) {
    // The timer must be running
    return false;
  }

  auto instVariant = instance->GetVariant();
  auto instType =
      instVariant ? instVariant->GetInstanceType() : InstanceType_t::NORMAL;

  switch (instType) {
    case InstanceType_t::MISSION: {
      uint64_t expireTime = instance->GetTimerExpire();
      expireTime = expireTime + (uint64_t)seconds * 1000000ULL;

      instance->SetTimerExpire(expireTime);
    } break;
    case InstanceType_t::NORMAL:
    case InstanceType_t::TIME_TRIAL:
    case InstanceType_t::PVP:
    case InstanceType_t::DEMON_ONLY:
    case InstanceType_t::DIGITALIZE:
    default:
      LogZoneManagerErrorMsg(
          "Attempted to extend instance timer of invalid type\n");

      return false;
  }

  // Schedule the new expiration
  ScheduleTimerExpiration(instance);

  // Notify the current players
  SendInstanceTimer(instance, nullptr, false, seconds);

  return true;
}

bool ZoneManager::StopInstanceTimer(
    const std::shared_ptr<ZoneInstance>& instance, uint64_t stopTime) {
  if (!instance) {
    return false;
  }

  auto instVariant = instance->GetVariant();
  auto instType =
      instVariant ? instVariant->GetInstanceType() : InstanceType_t::NORMAL;

  if (!stopTime) {
    stopTime = ChannelServer::GetServerTime();
  }

  bool expired = false;
  switch (instType) {
    case InstanceType_t::TIME_TRIAL: {
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (!instance->GetTimerStop()) {
        instance->SetTimerStop(stopTime);

        if (instance->GetTimerExpire() <= stopTime) {
          // Instance timer expired (client actually handles the
          // time-out display)
          instance->SetTimerStop(instance->GetTimerExpire());
          expired = true;
        } else {
          // Complete time trial
          for (auto client : instance->GetConnections()) {
            EndInstanceTimer(instance, client, true);
          }
        }
      }
    } break;
    case InstanceType_t::PVP: {
      bool end = false;
      {
        std::lock_guard<libcomp::Mutex> lock(mLock);
        if (!instance->GetTimerStop()) {
          instance->SetTimerStop(stopTime);

          end = true;
        }
      }

      if (end) {
        // Handle end actions in match manager
        mServer.lock()->GetMatchManager()->EndPvPMatch(instance->GetID());

        // Handle all stops as expirations
        expired = true;
      }
    } break;
    case InstanceType_t::DEMON_ONLY: {
      std::lock_guard<libcomp::Mutex> lock(mLock);
      if (instance->GetTimerExpire() && !instance->GetTimerStop()) {
        instance->SetTimerStop(stopTime);

        if (instance->GetTimerExpire() <= stopTime) {
          // Instance timer expired
          instance->SetTimerStop(instance->GetTimerExpire());
          expired = true;

          for (auto client : instance->GetConnections()) {
            EndInstanceTimer(instance, client, false);
          }

          // Kick everyone in 5 seconds
          mServer.lock()->GetTimerManager()->ScheduleEventIn(
              5,
              [](ZoneManager* pZoneManager, uint32_t pInstanceID) {
                auto pInstance = pZoneManager->GetInstance(pInstanceID);
                if (pInstance) {
                  for (auto connection : pInstance->GetConnections()) {
                    pZoneManager->EnterZone(
                        connection, pInstance->GetDefinition()->GetLobbyID(),
                        0);
                  }
                }
              },
              this, instance->GetID());
        } else {
          // Instance timer success
          for (auto client : instance->GetConnections()) {
            EndInstanceTimer(instance, client, true);
          }
        }
      }
    } break;
    case InstanceType_t::DIASPORA: {
      bool nextPhase = false;
      {
        std::lock_guard<libcomp::Mutex> lock(mLock);
        if (!instance->GetTimerStop()) {
          if (instance->GetTimerExpire() <= stopTime) {
            // Timer expired
            instance->SetTimerStop(instance->GetTimerExpire());
            expired = true;
          } else {
            instance->SetTimerStop(stopTime);
          }

          if (!instance->GetMatch()->GetPhase()) {
            nextPhase = true;
          }
        }
      }

      if (nextPhase) {
        // Move on to the next phase
        mServer.lock()->GetMatchManager()->AdvancePhase(
            GetInstanceStartingZone(instance), 1, 0);
      } else {
        // Complete timer
        for (auto client : instance->GetConnections()) {
          EndInstanceTimer(instance, client, true);
        }
      }
    } break;
    case InstanceType_t::MISSION:
    case InstanceType_t::DIGITALIZE: {
      bool endInstanceTimer = false;

      {
        std::lock_guard<libcomp::Mutex> lock(mLock);

        if (!instance->GetTimerStop()) {
          if (instance->GetTimerExpire() &&
              instance->GetTimerExpire() <= stopTime) {
            // Timer expired
            instance->SetTimerStop(instance->GetTimerExpire());
            expired = true;
          } else {
            instance->SetTimerStop(stopTime);
          }

          endInstanceTimer = true;
        }
      }

      // Complete timer
      if (endInstanceTimer) {
        for (auto client : instance->GetConnections()) {
          EndInstanceTimer(instance, client, true);
        }
      }
    } break;
    case InstanceType_t::NORMAL:
    default: {
      auto timeLimitData = instance->GetTimeLimitData();
      if (!timeLimitData) {
        return false;
      }

      bool stopped = false;
      {
        std::lock_guard<libcomp::Mutex> lock(mLock);
        if (!instance->GetTimerStop()) {
          instance->SetTimerStop(stopTime);

          stopped = true;
          if (instance->GetTimerExpire() <= stopTime) {
            instance->SetTimerStop(instance->GetTimerExpire());
            expired = true;
          }
        }
      }

      if (stopped && !expired) {
        for (auto client : instance->GetConnections()) {
          EndInstanceTimer(instance, client, false);
        }
      }
    } break;
  }

  if (expired && !instance->GetTimerExpirationEventID().IsEmpty()) {
    // Fire the expiration event in the starting zone with no default
    // player context. If a player context or a different zone context
    // needs to be changed to for the event itself, an ActionStartEvent
    // with a non-default context should be used.
    EventOptions options;
    options.AutoOnly = true;
    options.NoInterrupt = true;

    mServer.lock()->GetEventManager()->HandleEvent(
        nullptr, instance->GetTimerExpirationEventID(), 0,
        GetInstanceStartingZone(instance), options);
  }

  return true;
}

void ZoneManager::SendInstanceTimer(
    const std::shared_ptr<ZoneInstance>& instance,
    const std::shared_ptr<ChannelClientConnection>& client, bool queue,
    uint32_t extension) {
  if (!instance) {
    return;
  }

  auto instVariant = instance->GetVariant();
  auto instType =
      instVariant ? instVariant->GetInstanceType() : InstanceType_t::NORMAL;
  bool timerActive = instance->GetTimerStart() && !instance->GetTimerStop();

  libcomp::Packet p;
  RelativeTimeMap timeMap;

  switch (instType) {
    case InstanceType_t::TIME_TRIAL:
      if (!timerActive) {
        return;
      } else {
        p.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_TIME_TRIAL_UPDATE);
        p.WriteS8((int8_t)instVariant->GetSubID());

        timeMap[p.Size()] = instance->GetTimerStart();
        p.WriteFloat(0.f);

        p.WriteS32Little(0);  // Unknown
        p.WriteBlank(32);     // Unknown array (phases?)
        p.WriteU8(1);         // Unknown bool
      }
      break;
    case InstanceType_t::DEMON_ONLY:
      if (!timerActive) {
        return;
      } else {
        uint64_t now = ChannelServer::GetServerTime();

        uint64_t expireTime = instance->GetTimerExpire();
        float timeLeft = expireTime < now
                             ? 0.f
                             : (float)((double)(expireTime - now) / 1000000.0);

        p.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_DEMON_SOLO_UPDATE);
        p.WriteU32Little(instVariant->GetSubID());
        p.WriteFloat(timeLeft);
        p.WriteS32Little(1);
        p.WriteS32Little((int32_t)instance->GetTimerID());
      }
      break;
    case InstanceType_t::DIASPORA: {
      uint64_t now = ChannelServer::GetServerTime();

      uint64_t expireTime = instance->GetTimerExpire();
      float timeLeft = expireTime < now
                           ? 0.f
                           : (float)((double)(expireTime - now) / 1000000.0);

      int32_t timerState = 0;
      if (instance->GetMatch()->GetPhase()) {
        if (instance->GetTimerStop() &&
            instance->GetTimerStop() < instance->GetTimerExpire()) {
          // Success
          timerState = 2;
        } else {
          // Running
          timerState = 1;
        }
      }

      p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DIASPORA_TIME);
      p.WriteS32Little(timerState);
      p.WriteS32Little(0);  // Timer just started (not used)
      p.WriteFloat(timeLeft);
      p.WriteFloat((float)extension);
    } break;
    case InstanceType_t::MISSION: {
      uint8_t timerState = 0;
      float timeLeft = (float)instVariant->GetTimePoints(0);

      uint64_t expireTime = instance->GetTimerExpire();
      if (expireTime) {
        uint64_t now = ChannelServer::GetServerTime();

        timerState =
            timerActive
                ? 1
                : (instance->GetTimerStop() < instance->GetTimerExpire() ? 3
                                                                         : 4);
        timeLeft = (expireTime && expireTime < now)
                       ? 0.f
                       : (float)((double)(expireTime - now) / 1000000.0);
      }

      p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MISSION_STATE);
      p.WriteU32Little(instVariant->GetSubID());
      p.WriteU8(timerState);
      p.WriteS32Little((int32_t)timeLeft);
    } break;
    case InstanceType_t::DIGITALIZE:
      if (!timerActive) {
        return;
      } else {
        uint64_t now = ChannelServer::GetServerTime();
        float elapsed =
            (float)(((double)now - (double)instance->GetTimerStart()) /
                    1000000.0);

        if (client && !client->GetClientState()->GetInstanceTimerActive()) {
          p.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_DIGITALIZE_DUNGEON_START);
        } else {
          p.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_DIGITALIZE_DUNGEON_UPDATE);
        }

        p.WriteU32Little(instVariant->GetSubID());
        p.WriteFloat(elapsed);
        p.WriteS8(0);
      }
      break;
    case InstanceType_t::NORMAL:
    default: {
      auto timeLimitData = instance->GetTimeLimitData();
      if (!timeLimitData || instance->GetTimerStop()) {
        return;
      }

      p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TIME_LIMIT_UPDATE);
      p.WriteS8((int8_t)timeLimitData->GetID());

      timeMap[p.Size()] = instance->GetTimerStart();
      p.WriteFloat(0.f);

      p.WriteU8(1);  // Unknown bool
      p.WriteU8(0);  // Unknown bool
    }
  }

  std::list<std::shared_ptr<ChannelClientConnection>> connections;
  if (timeMap.size() > 0) {
    if (client) {
      connections.push_back(client);
    } else {
      connections = instance->GetConnections();
    }

    ChannelClientConnection::SendRelativeTimePacket(connections, p, timeMap,
                                                    queue);
  } else if (client) {
    if (queue) {
      client->QueuePacket(p);
    } else {
      client->SendPacket(p);
    }

    connections.push_back(client);
  } else {
    connections = instance->GetConnections();
    ChannelClientConnection::BroadcastPacket(connections, p);
  }

  // Set the instance timers as active
  for (auto c : connections) {
    auto state = c->GetClientState();
    state->SetInstanceTimerActive(true);
  }
}

void ZoneManager::UpdateDeathTimeOut(
    ClientState* state, int32_t time,
    const std::shared_ptr<ChannelClientConnection>& client) {
  if (!state) {
    return;
  } else if (time == -1) {
    // Remove all time-outs no matter what
    state->GetCharacterState()->SetDeathTimeOut(0);
    state->GetDemonState()->SetDeathTimeOut(0);
  }

  auto zone = state->GetZone();
  if (!zone) {
    return;
  }

  std::shared_ptr<ActiveEntityState> eState;
  switch (zone->GetInstanceType()) {
    case InstanceType_t::PVP:
      eState = state->GetCharacterState();
      break;
    case InstanceType_t::DEMON_ONLY:
      eState = state->GetDemonState();
      break;
    default:
      // No other instance types supported
      return;
  }

  auto instance = zone->GetInstance();
  auto instVariant = instance->GetVariant();

  float timeLeft = 0.f;
  if (time > 0) {
    // Setting timeout
    uint64_t now = ChannelServer::GetServerTime();
    uint64_t deathTime = now + (uint64_t)time * (uint64_t)1000000;

    eState->SetDeathTimeOut(deathTime);
    timeLeft = (float)time;

    auto killedClient = mServer.lock()->GetManagerConnection()->GetEntityClient(
        state->GetCharacterState()->GetEntityID());
    if (killedClient) {
      HandleDeathTimeOut(instance, killedClient, 0);
    }
  }

  if (zone->GetInstanceType() == InstanceType_t::DEMON_ONLY) {
    if (time == 0) {
      // Calculate remaining time
      uint64_t now = ChannelServer::GetServerTime();
      uint64_t deathTime = eState->GetDeathTimeOut();
      if (deathTime > now) {
        timeLeft = (float)((double)(deathTime - now) / 1000000.0);
      }
    }

    libcomp::Packet p;
    p.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_DEMON_SOLO_DEATH_TIME);
    p.WriteU32Little(instVariant ? instVariant->GetSubID() : 0);
    p.WriteS32Little(eState->GetEntityID());
    p.WriteFloat(timeLeft);

    if (client) {
      client->SendPacket(p);
    } else {
      BroadcastPacket(zone, p);
    }
  }
}

void ZoneManager::HandleDeathTimeOut(
    const std::shared_ptr<ZoneInstance>& instance,
    const std::shared_ptr<ChannelClientConnection>& client,
    uint64_t deathTime) {
  auto state = client->GetClientState();
  auto zone = state->GetZone();
  if (!zone || zone->GetInstance() != instance) {
    // Zone no longer valid
    return;
  }

  std::shared_ptr<ActiveEntityState> eState;
  switch (zone->GetInstanceType()) {
    case InstanceType_t::PVP:
      eState = state->GetCharacterState();
      break;
    case InstanceType_t::DEMON_ONLY:
      eState = state->GetDemonState();
      break;
    default:
      // No other instance types supported
      return;
  }

  if (deathTime && eState->GetDeathTimeOut() != deathTime) {
    // Entity no longer valid
    return;
  }

  if (deathTime) {
    switch (zone->GetInstanceType()) {
      case InstanceType_t::PVP: {
        // Auto-revive at starting point
        mServer.lock()->GetCharacterManager()->ReviveCharacter(
            client, REVIVE_PVP_RESPAWN);
      } break;
      case InstanceType_t::DEMON_ONLY: {
        // Disable revival and schedule removal in 5 seconds
        state->SetAcceptRevival(false);

        EndInstanceTimer(instance, client, false, false);

        mServer.lock()->GetTimerManager()->ScheduleEventIn(
            5,
            [](ZoneManager* pZoneManager,
               std::shared_ptr<ChannelClientConnection> pClient,
               uint32_t pInstanceID) {
              auto pState = pClient->GetClientState();
              auto pZone = pState->GetZone();
              auto pInstance = pZoneManager->GetInstance(pInstanceID);
              if (pInstance && pZone && pZone->GetInstance() == pInstance) {
                pZoneManager->EnterZone(
                    pClient, pInstance->GetDefinition()->GetLobbyID(), 0);
              }
            },
            this, client, instance->GetID());
      } break;
      default:
        break;
    }
  } else {
    // Schedule the death time-out
    uint64_t now = ChannelServer::GetServerTime();
    deathTime = eState->GetDeathTimeOut();

    int timeLeft = (int)((deathTime > now ? (deathTime - now) : 0) / 1000000);
    mServer.lock()->GetTimerManager()->ScheduleEventIn(
        timeLeft,
        [](ZoneManager* pZoneManager,
           std::shared_ptr<ChannelClientConnection> pClient,
           uint32_t pInstanceID, uint64_t pDeathTime) {
          auto pInstance = pZoneManager->GetInstance(pInstanceID);
          if (pInstance) {
            pZoneManager->HandleDeathTimeOut(pInstance, pClient, pDeathTime);
          }
        },
        this, client, instance->GetID(), deathTime);
  }
}

void ZoneManager::EndInstanceTimer(
    const std::shared_ptr<ZoneInstance>& instance,
    const std::shared_ptr<ChannelClientConnection>& client, bool isSuccess,
    bool queue) {
  auto state = client->GetClientState();
  auto zone = state->GetZone();
  if (!zone || !instance) {
    return;
  }

  auto instVariant = instance->GetVariant();
  auto instType =
      instVariant ? instVariant->GetInstanceType() : InstanceType_t::NORMAL;

  libcomp::Packet notify;

  switch (instType) {
    case InstanceType_t::TIME_TRIAL: {
      // Quit the trial if the timer has not stopped yet
      bool quit = !instance->GetTimerStop();
      if (quit && isSuccess) {
        return;
      }

      auto cState = state->GetCharacterState();
      auto character = cState->GetEntity();
      auto progress = character ? character->GetProgress().Get() : nullptr;
      if (isSuccess && progress) {
        uint16_t timeResult = (uint16_t)(
            (instance->GetTimerStop() - instance->GetTimerStart()) / 1000000);

        // Get the best result applicable
        int8_t result =
            (int8_t)objects::CharacterProgress::TimeTrialResult_t::FAILED;
        for (size_t rank = 0; rank < 4; rank++) {
          uint16_t timePoint = instVariant->GetTimePoints(rank);
          if (timeResult > timePoint ||
              (rank == 0 && timeResult == timePoint)) {
            // Times that match the expiration exactly still
            // count as failures
            break;
          }

          result--;
        }

        if (result ==
            (int8_t)objects::CharacterProgress::TimeTrialResult_t::FAILED) {
          // Do nothing
          return;
        } else {
          // Set the state and register the time
          int8_t trialID = (int8_t)instVariant->GetSubID();
          progress->SetTimeTrialID(trialID);
          progress->SetTimeTrialTime(timeResult);

          uint16_t previousTime =
              progress->GetTimeTrialRecords((size_t)(trialID - 1));

          bool newBest = previousTime == 0;
          bool firstA = previousTime == 0 && result == 1;
          if (previousTime && previousTime > timeResult) {
            newBest = true;

            // If the previous best time was not an A-rank and the
            // new one is, set first A-rank
            firstA =
                previousTime > instVariant->GetTimePoints(3) && result == 1;
          }

          if (firstA) {
            result = (int8_t)
                objects::CharacterProgress::TimeTrialResult_t::A_RANK_FIRST;
          }

          progress->SetTimeTrialResult(
              (objects::CharacterProgress::TimeTrialResult_t)result);

          if (newBest) {
            // Report the new best time to others in the instance
            std::list<std::shared_ptr<ChannelClientConnection>> connections;
            for (auto connection : instance->GetConnections()) {
              if (connection != client) {
                connections.push_back(connection);
              }
            }

            if (connections.size() > 0) {
              libcomp::Packet p;
              p.WritePacketCode(
                  ChannelToClientPacketCode_t::PACKET_DUNGEON_RECORDS_UPDATE);
              p.WriteString16Little(libcomp::Convert::ENCODING_DEFAULT,
                                    character->GetName(), true);

              ChannelClientConnection::BroadcastPacket(connections, p);
            }
          }

          mServer.lock()->GetWorldDatabase()->QueueUpdate(
              progress, state->GetAccountUID());
        }
      } else {
        quit = true;
      }

      notify.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_TIME_TRIAL_END);

      if (!quit) {
        notify.WriteS8((int8_t)instVariant->GetSubID());
        notify.WriteFloat(state->ToClientTime(instance->GetTimerStart()));
        notify.WriteFloat(state->ToClientTime(instance->GetTimerStop()));
        notify.WriteS8(0);
      } else {
        notify.WriteS8(-1);
        notify.WriteFloat(0);
        notify.WriteFloat(0);
        notify.WriteS8(1);
      }
    } break;
    case InstanceType_t::DEMON_ONLY: {
      // A player successfully completed the instance if they're in it
      // when the timer stops before the expiration, whether they're
      // alive or not
      bool success = zone->GetInstance() == instance &&
                     instance->GetTimerStop() &&
                     instance->GetTimerStop() < instance->GetTimerExpire();
      if (isSuccess != success) {
        return;
      }

      if (success && state->GetDemonState()->GetDeathTimeOut()) {
        // Remove the death time-out
        UpdateDeathTimeOut(state, -1);
      }

      float timeLeft = 0.f;
      int32_t rank = -1;
      int32_t spGain = 0;
      if (success) {
        uint16_t rankA = instVariant->GetTimePoints(1);
        uint16_t rankB = instVariant->GetTimePoints(2);

        uint16_t maxAdjusted = (uint16_t)(
            (instance->GetTimerExpire() - instance->GetTimerStart()) /
            1000000ULL);
        if (maxAdjusted != instVariant->GetTimePoints(0)) {
          double modifier =
              (double)maxAdjusted / (double)instVariant->GetTimePoints(0);
          rankA = (uint16_t)((double)rankA * modifier);
          rankB = (uint16_t)((double)rankB * modifier);
        }

        timeLeft = (float)((double)(instance->GetTimerExpire() -
                                    instance->GetTimerStop()) /
                           1000000.0);

        if (timeLeft > (float)rankA) {
          // Rank A
          rank = 0;
        } else if (timeLeft > (float)rankB) {
          // Rank B
          rank = 1;
        } else {
          // Rank C
          rank = 2;
        }

        spGain = (int32_t)instVariant->GetFixedReward();

        int32_t rewardModifier = instVariant->GetRewardModifier();
        if (rewardModifier) {
          // Increase by 10% per additional party member, decrease by
          // percentage of time used
          size_t partyMemberCount = instance->OriginalAccessCIDsCount();
          double boost = (double)(partyMemberCount - 1) * 0.1;
          float timePercentLeft = timeLeft / (float)maxAdjusted;

          spGain =
              spGain + (int32_t)ceil((double)rewardModifier * (1.0 + boost) *
                                     (double)timePercentLeft);
        }

        spGain = mServer.lock()->GetCharacterManager()->UpdateSoulPoints(
            client, spGain, true, true);
      }

      notify.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_DEMON_SOLO_END);
      notify.WriteU32Little(instVariant->GetSubID());
      notify.WriteS32Little(success ? 0 : 1);
      notify.WriteFloat(timeLeft);
      notify.WriteS32Little(rank);
      notify.WriteS32Little(spGain);
    } break;
    case InstanceType_t::DIASPORA: {
      // A player successfully completed the instance if they're in it
      // when the timer stops, has not expired and is not in phase 0
      bool success = zone->GetInstance() == instance &&
                     instance->GetTimerStop() &&
                     instance->GetTimerExpire() != instance->GetTimerStop() &&
                     zone->GetMatch()->GetPhase();
      if (isSuccess != success) {
        return;
      }

      if (success) {
        SendInstanceTimer(instance, client, false);
      }

      notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DIASPORA_END);
      notify.WriteS32Little(success ? 1 : 0);
    } break;
    case InstanceType_t::MISSION:
      // Just send the timer as it displays in all states
      SendInstanceTimer(instance, client, false);
      return;
    case InstanceType_t::DIGITALIZE: {
      // A player successfully completed the instance if they're in it
      // when the timer stops whether they're alive or not
      bool success =
          zone->GetInstance() == instance && instance->GetTimerStop();
      if (!isSuccess || !success) {
        // Do not send failure state for this instance type
        return;
      }

      int8_t result = 0;

      float elapsed = (float)(((double)instance->GetTimerStop() -
                               (double)instance->GetTimerStart()) /
                              1000000.0);

      uint16_t rankB = instVariant->GetTimePoints(0);
      uint16_t rankA = instVariant->GetTimePoints(1);

      if (elapsed <= (float)rankA) {
        // Rank A
        result = 0;

        auto dgState = state->GetCharacterState()->GetDigitalizeState();
        uint8_t raceID = dgState ? dgState->GetRaceID() : 0;
        if (raceID) {
          // Only rank A grants points
          int32_t gain = (int32_t)instVariant->GetFixedReward();
          int32_t rewardModifier = instVariant->GetRewardModifier();
          if (rewardModifier) {
            float globalDXPBonus = mServer.lock()
                                       ->GetWorldSharedConfig()
                                       ->GetDigitalizePointBonus();
            float timePercent = rankA ? (elapsed / (float)rankA) : 1.f;

            gain = gain +
                   (int32_t)ceil((double)(rankA * rewardModifier) *
                                 (double)timePercent * (1.0 + globalDXPBonus));
          }

          std::unordered_map<uint8_t, int32_t> points;
          points[raceID] = gain;

          mServer.lock()->GetCharacterManager()->UpdateDigitalizePoints(
              client, points, true);
        }
      } else if (elapsed <= (float)rankB) {
        // Rank B
        result = 1;
      } else {
        // Rank C
        result = 2;
      }

      notify.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_DIGITALIZE_DUNGEON_END);
      notify.WriteU32Little(instVariant->GetSubID());
      notify.WriteFloat(elapsed);
      notify.WriteS8(result);
    } break;
    case InstanceType_t::NORMAL:
    default: {
      // Time limits do not have a success or fail state
      auto timeLimitData = instance->GetTimeLimitData();
      if (!timeLimitData) {
        return;
      }

      notify.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_TIME_LIMIT_END);
      notify.WriteU32Little(timeLimitData->GetID());
      notify.WriteS8(0);  // Unknown
      notify.WriteU8(1);  // Unknown bool
    } break;
  }

  if (queue) {
    client->QueuePacket(notify);
  } else {
    client->SendPacket(notify);
  }

  client->GetClientState()->SetInstanceTimerActive(false);
}

bool ZoneManager::UpdateTrackedZone(
    const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<objects::Team>& team) {
  if (!zone) {
    return false;
  } else if (zone->GetInstanceType() == InstanceType_t::DIASPORA) {
    // Uses a special packet for tracking players which act as team members
    auto clients = zone->GetConnectionList();

    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DIASPORA_STATUS);
    notify.WriteU32Little((uint32_t)clients.size());

    for (auto client : clients) {
      auto state = client->GetClientState();
      auto cState = state->GetCharacterState();

      notify.WriteU32Little((uint32_t)state->GetWorldCID());
      notify.WriteFloat(cState->GetDestinationX());
      notify.WriteFloat(cState->GetDestinationY());
      notify.WriteS32Little(cState->GetCoreStats()->GetHP());
    }

    auto bosses = zone->GetBosses();
    auto boss = bosses.size() > 0 ? bosses.front() : nullptr;

    auto match = zone->GetMatch();
    if (!match || match->GetPhase() != DIASPORA_PHASE_BOSS) {
      // Only track bosses during the boss phase
      boss = nullptr;
    }

    notify.WriteU32Little(boss ? 1 : 0);
    if (boss) {
      auto enemy = boss->GetEntity();
      auto mbCounts = zone->GetDiasporaMiniBossCount();

      notify.WriteU32Little(enemy->GetType());
      notify.WriteS32Little(boss->GetCoreStats()->GetHP());
      notify.WriteS32Little(boss->GetMaxHP());
      notify.WriteS32Little((int32_t)(mbCounts.second - mbCounts.first));
      notify.WriteS32Little((int32_t)mbCounts.second);
    }

    ChannelClientConnection::BroadcastPacket(clients, notify);

    return true;
  } else if (zone->GetDefinition()->GetTrackTeam() && team) {
    // Team specified, send just that
    return UpdateTrackedTeam(team, zone);
  }

  return false;
}

bool ZoneManager::UpdateTrackedTeam(const std::shared_ptr<objects::Team>& team,
                                    const std::shared_ptr<Zone>& zone) {
  if (!team || (zone && !zone->GetDefinition()->GetTrackTeam())) {
    // No team or source is not in a trackable zone
    return false;
  }

  std::list<std::shared_ptr<ChannelClientConnection>> clients;

  libcomp::Packet notify;
  notify.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_TEAM_MEMBER_UPDATE);
  notify.WriteS8((int8_t)team->MemberIDsCount());

  auto managerConnection = mServer.lock()->GetManagerConnection();
  for (int32_t worldCID : team->GetMemberIDs()) {
    notify.WriteS32Little(worldCID);

    auto client = managerConnection->GetEntityClient(worldCID, true);
    auto state = client ? client->GetClientState() : nullptr;
    auto cState = state ? state->GetCharacterState() : nullptr;
    auto oZone = state ? state->GetZone() : nullptr;
    if (cState && oZone && oZone->GetDefinition()->GetTrackTeam()) {
      notify.WriteU32Little(oZone->GetDefinitionID());
      notify.WriteFloat(cState->GetDestinationX());
      notify.WriteFloat(cState->GetDestinationY());
      notify.WriteS32Little(cState->GetCoreStats()->GetHP());

      clients.push_back(client);
    } else {
      // Not tracked
      notify.WriteBlank(16);
    }
  }

  ChannelClientConnection::BroadcastPacket(clients, notify);

  return true;
}

bool ZoneManager::UpdateDestinyBox(
    const std::shared_ptr<ZoneInstance>& instance, int32_t worldCID,
    const std::list<std::shared_ptr<objects::Loot>>& add,
    std::set<uint8_t> remove) {
  if (!instance || !worldCID) {
    // No instance or player source entity
    return false;
  }

  auto dBox = instance->GetDestinyBox(worldCID);
  if (!dBox) {
    // No box
    return false;
  }

  uint8_t newNext = 0;
  auto results = instance->UpdateDestinyBox(worldCID, newNext, add, remove);
  if (results.size() == 0) {
    // Nothing happened
    return false;
  }

  // Report results to client(s)
  std::list<std::shared_ptr<ChannelClientConnection>> clients;
  if (dBox->GetOwnerCID()) {
    auto client = mServer.lock()->GetManagerConnection()->GetEntityClient(
        dBox->GetOwnerCID(), true);
    if (client) {
      clients.push_back(client);
    }
  } else {
    clients = instance->GetConnections();
  }

  if (clients.size() > 0) {
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DESTINY_BOX_UPDATE);

    p.WriteS32Little(0);  // Write later

    int32_t updates = 0;
    for (auto& pair : results) {
      auto loot = pair.second;
      if (loot) {
        // Added/updated
        p.WriteU8(pair.first);
        p.WriteU32Little(loot->GetType());
        p.WriteU16Little(loot->GetCount());

        updates++;
      }
    }

    uint32_t removePos = p.Size();
    p.WriteS32Little(0);  // Write later

    int32_t removes = 0;
    for (auto& pair : results) {
      auto loot = pair.second;
      if (!loot) {
        // Removed
        p.WriteU8(pair.first);

        removes++;
      }
    }

    p.WriteS32Little((int32_t)newNext);

    // Now rewind and write counts
    p.Seek(2);
    p.WriteS32Little(updates);

    p.Seek(removePos);
    p.WriteS32Little(removes);

    ChannelClientConnection::BroadcastPacket(clients, p);
  }

  return true;
}

void ZoneManager::SendDestinyBox(
    const std::shared_ptr<ChannelClientConnection>& client, bool eventMenu,
    bool queue) {
  auto state = client->GetClientState();
  auto zone = state->GetZone();
  auto instance = zone ? zone->GetInstance() : nullptr;
  if (!instance) {
    return;
  }

  auto dBox = instance->GetDestinyBox(state->GetWorldCID());
  if (!eventMenu && !dBox) {
    return;
  }

  libcomp::Packet p;
  if (eventMenu) {
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DESTINY_BOX_DATA);
    p.WriteS32Little(dBox != nullptr ? 0 : -1);  // Success/failure
  } else {
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DESTINY_BOX);
  }

  if (dBox) {
    int8_t size = (int8_t)dBox->LootCount();

    p.WriteS32Little((int32_t)size);

    int32_t lootSet = 0;

    // Write the lootSet count at the end
    p.WriteS32Little(0);
    for (int8_t i = 0; i < size; i++) {
      auto loot = dBox->GetLoot((size_t)i);
      if (loot) {
        p.WriteS8(i);
        p.WriteU32Little(loot->GetType());
        p.WriteU16Little(loot->GetCount());

        lootSet++;
      }
    }

    if (!eventMenu) {
      // The next position is only communicated outside of the event
      p.WriteS32Little((int32_t)dBox->GetNextPosition());
    }

    p.Seek(eventMenu ? 10 : 6);
    p.WriteS32Little(lootSet);
  }

  if (queue) {
    client->QueuePacket(p);
  } else {
    client->SendPacket(p);
  }
}

void ZoneManager::MultiZoneBossKilled(const std::shared_ptr<Zone>& zone,
                                      ClientState* sourceState,
                                      const std::list<uint32_t>& types) {
  uint32_t zoneGroupID = zone->GetDefinition()->GetGlobalBossGroup();
  if (sourceState) {
    auto cState = sourceState->GetCharacterState();

    std::list<std::shared_ptr<ChannelClientConnection>> clients;
    {
      std::lock_guard<libcomp::Mutex> lock(mLock);
      for (auto uniqueID : mGlobalBossZones[zoneGroupID]) {
        auto z = mZones[uniqueID];
        for (auto c : z->GetConnectionList()) {
          clients.push_back(c);
        }
      }
    }

    for (uint32_t type : types) {
      libcomp::Packet p;
      p.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_MULTIZONE_BOSS_KILLED);
      p.WriteU32Little(type);
      p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_DEFAULT,
                            cState->GetEntity()->GetName(), true);

      ChannelClientConnection::BroadcastPacket(clients, p);
    }
  }

  SendMultiZoneBossStatus(zoneGroupID);
}

bool ZoneManager::TriggerZoneActions(
    const std::shared_ptr<Zone>& zone,
    std::list<std::shared_ptr<ActiveEntityState>> entities,
    ZoneTrigger_t trigger,
    const std::shared_ptr<ChannelClientConnection>& client) {
  bool executed = false;

  auto triggers = GetZoneTriggers(zone, trigger);
  if (triggers.size() > 0) {
    if (trigger == ZoneTrigger_t::ON_SPAWN) {
      // Check to see if any trigger values are specified, never fire
      // without an entity
      auto validTriggers = triggers;
      validTriggers.remove_if(
          [](const std::shared_ptr<objects::ServerZoneTrigger>& t) {
            return t->GetValue() != 0;
          });

      if (validTriggers.size() == triggers.size()) {
        // No trigger values, fire all for each entity
        for (auto entity : entities) {
          executed |= HandleZoneTriggers(zone, triggers, entity, client);
        }
      } else {
        // Fire triggers for value 0 or value matching the spawn ID
        for (auto entity : entities) {
          auto eBase = entity->GetEnemyBase();
          auto spawn = eBase ? eBase->GetSpawnSource() : nullptr;
          validTriggers = triggers;

          validTriggers.remove_if(
              [spawn](const std::shared_ptr<objects::ServerZoneTrigger>& t) {
                return t->GetValue() != 0 &&
                       (!spawn || (uint32_t)t->GetValue() != spawn->GetID());
              });

          executed |= HandleZoneTriggers(zone, validTriggers, entity, client);
        }
      }
    } else if (entities.size() > 0) {
      // Execute once per entity
      for (auto entity : entities) {
        executed |= HandleZoneTriggers(zone, triggers, entity, client);
      }
    } else {
      // Execute once with no entity
      executed = HandleZoneTriggers(zone, triggers);
    }
  }

  return executed;
}

std::list<std::shared_ptr<objects::ServerZoneTrigger>>
ZoneManager::GetZoneTriggers(const std::shared_ptr<Zone>& zone,
                             ZoneTrigger_t trigger) {
  std::list<std::shared_ptr<objects::ServerZoneTrigger>> triggers;
  if (!zone) {
    return triggers;
  }

  switch (trigger) {
    case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_IN:
    case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_OUT:
    case objects::ServerZoneTrigger::Trigger_t::PRE_ZONE_IN:
    case objects::ServerZoneTrigger::Trigger_t::ON_LOGIN:
      triggers = zone->GetZoneChangeTriggers();
      break;
    case objects::ServerZoneTrigger::Trigger_t::ON_SPAWN:
    case objects::ServerZoneTrigger::Trigger_t::ON_RESPAWN:
      triggers = zone->GetSpawnTriggers();
      break;
    case objects::ServerZoneTrigger::Trigger_t::ON_DEATH:
    case objects::ServerZoneTrigger::Trigger_t::ON_REVIVAL:
      triggers = zone->GetDeathTriggers();
      break;
    default:
      triggers = zone->GetMiscTriggers();
      break;
  }

  triggers.remove_if(
      [trigger](const std::shared_ptr<objects::ServerZoneTrigger>& t) {
        return t->GetTrigger() != trigger;
      });

  // Add global triggers to the end of the list if they exist
  auto globalDef =
      mServer.lock()->GetServerDataManager()->GetZonePartialData(0);
  if (globalDef) {
    for (auto t : globalDef->GetTriggers()) {
      if (t->GetTrigger() == trigger) {
        triggers.push_back(t);
      }
    }
  }

  return triggers;
}

bool ZoneManager::HandleZoneTriggers(
    const std::shared_ptr<Zone>& zone,
    const std::list<std::shared_ptr<objects::ServerZoneTrigger>>& triggers,
    const std::shared_ptr<ActiveEntityState>& entity,
    const std::shared_ptr<ChannelClientConnection>& client) {
  bool executed = false;
  if (!entity || entity->Ready(true)) {
    auto actionManager = mServer.lock()->GetActionManager();

    ActionOptions options;
    options.AutoEventsOnly = true;

    int32_t entityID = entity ? entity->GetEntityID() : 0;
    for (auto tr : triggers) {
      actionManager->PerformActions(client, tr->GetActions(), entityID, zone,
                                    options);
      executed = true;
    }
  }

  return executed;
}

bool ZoneManager::StartZoneEvent(const std::shared_ptr<Zone>& zone,
                                 const libcomp::String& eventID) {
  auto server = mServer.lock();
  auto eventManager = server->GetEventManager();

  return eventManager->HandleEvent(nullptr, eventID, 0, zone);
}

bool ZoneManager::UpdateGeometryElement(
    const std::shared_ptr<Zone>& zone,
    std::shared_ptr<objects::ServerObject> elemObject) {
  auto geometry = zone->GetGeometry();
  if (geometry) {
    bool updated = false;

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto objDef = definitionManager->GetONPCData(elemObject->GetID());
    if (objDef && !objDef->GetBarrierName().IsEmpty()) {
      bool disabled = IsGeometryDisabled(elemObject);

      libcomp::String name = objDef->GetBarrierName();
      for (auto elem : geometry->Elements) {
        if (elem->GetName() == name) {
          if (disabled) {
            zone->InsertDisabledBarriers(elem->GetID());
          } else {
            zone->RemoveDisabledBarriers(elem->GetID());
          }

          updated = true;

          // There should only ever be one of these but do not
          // break just in case as there is no hard restriction
        }
      }
    }

    return updated;
  }

  return false;
}

bool ZoneManager::GetSpotPosition(uint32_t dynamicMapID, uint32_t spotID,
                                  float& x, float& y, float& rot) const {
  if (spotID == 0 || dynamicMapID == 0) {
    return false;
  }

  auto spots =
      mServer.lock()->GetDefinitionManager()->GetSpotData(dynamicMapID);
  auto spotIter = spots.find(spotID);
  if (spotIter != spots.end()) {
    x = spotIter->second->GetCenterX();
    y = spotIter->second->GetCenterY();
    rot = spotIter->second->GetRotation();

    return true;
  }

  return false;
}

Point ZoneManager::GetRandomPoint(float width, float height) const {
  return Point(RNG_DEC(float, 0.f, (float)fabs(width), 2),
               RNG_DEC(float, 0.f, (float)fabs(height), 2));
}

Point ZoneManager::GetRandomSpotPoint(
    const std::shared_ptr<objects::MiSpotData>& spot,
    const std::shared_ptr<objects::MiZoneData>& zoneData) {
  Point center(spot->GetCenterX(), spot->GetCenterY());

  Point untransformed =
      GetRandomPoint(spot->GetSpanX() * 2.f, spot->GetSpanY() * 2.f);
  untransformed.x += center.x - spot->GetSpanX();
  untransformed.y += center.y - spot->GetSpanY();

  Point transformed =
      spot->GetRotation() != 0.f
          ? RotatePoint(untransformed, center, spot->GetRotation())
          : untransformed;

  if (zoneData) {
    // Ensure that the random spot is in the zone boundaries
    std::shared_ptr<ZoneGeometry> geometry;

    auto qmpFile = zoneData->GetFile()->GetQmpFile();
    if (!qmpFile.IsEmpty()) {
      std::lock_guard<libcomp::Mutex> lock(mLock);
      auto geoIter = mZoneGeometry.find(qmpFile.C());
      if (geoIter != mZoneGeometry.end()) {
        geometry = geoIter->second;
      }
    }

    Line centerLine(center, transformed);

    Point collision;
    Line collideLine;
    std::shared_ptr<ZoneShape> outShape;
    if (geometry &&
        geometry->Collides(centerLine, collision, collideLine, outShape)) {
      transformed = CollisionAdjust(center, collision, collideLine);
    }
  }

  return transformed;
}

float ZoneManager::GetRandomRotation() {
  return (float)RNG_DEC(double, -libcomp::PI, libcomp::PI, 2);
}

Point ZoneManager::GetLinearPoint(float sourceX, float sourceY, float targetX,
                                  float targetY, float distance, bool away,
                                  const std::shared_ptr<Zone>& zone) {
  bool adjusted = true;

  Point dest(sourceX, sourceY);
  if (targetX != sourceX) {
    float slope = (targetY - sourceY) / (targetX - sourceX);
    float denom = (float)std::sqrt(1.0f + std::pow(slope, 2));

    float xOffset = (float)(distance / denom);
    float yOffset = (float)fabs((slope * distance) / denom);

    dest.x = (away == (targetX > sourceX)) ? (sourceX - xOffset)
                                           : (sourceX + xOffset);
    dest.y = (away == (targetY > sourceY)) ? (sourceY - yOffset)
                                           : (sourceY + yOffset);
  } else if (targetY != sourceY) {
    dest.y = (away == (targetY > sourceY)) ? (sourceY - distance)
                                           : (sourceY + distance);
  } else {
    adjusted = false;
  }

  if (zone && adjusted) {
    // Check collision and back off if one happens
    Point src(sourceX, sourceY);

    Point collidePoint;
    Line collideLine;
    if (zone->Collides(Line(src, dest), collidePoint, collideLine)) {
      dest = CollisionAdjust(src, collidePoint, collideLine);
    }
  }

  return dest;
}

Point ZoneManager::MoveRelative(
    const std::shared_ptr<ActiveEntityState>& eState, float targetX,
    float targetY, float distance, bool away, uint64_t now, uint64_t endTime) {
  float x = eState->GetCurrentX();
  float y = eState->GetCurrentY();

  auto point =
      GetLinearPoint(x, y, targetX, targetY, distance, away, eState->GetZone());

  if (point.x != x || point.y != y) {
    eState->SetOriginX(x);
    eState->SetOriginY(y);
    eState->SetOriginTicks(now);

    eState->SetDestinationX(point.x);
    eState->SetDestinationY(point.y);
    eState->SetDestinationTicks(endTime);
  }

  return point;
}

bool ZoneManager::CorrectClientPosition(
    const std::shared_ptr<ActiveEntityState>& eState, Point& dest) {
  Point src(eState->GetOriginX(), eState->GetOriginY());
  ServerTime unused = 0;

  return CorrectClientPosition(eState, src, dest, unused, unused) != 0;
}

uint8_t ZoneManager::CorrectClientPosition(
    const std::shared_ptr<ActiveEntityState>& eState, Point& src, Point& dest,
    ServerTime& startTime, ServerTime& stopTime, bool isMove) {
  auto zone = eState->GetZone();
  if (!zone) {
    return false;
  }

  uint8_t result = 0;

  float serverX1 = eState->GetOriginX();
  float serverY1 = eState->GetOriginY();
  float serverX2 = eState->GetDestinationX();
  float serverY2 = eState->GetDestinationY();

  bool checkGeometry = true;
  if (isMove) {
    float maxRatePerSec = eState->GetMovementSpeed();

    // Check if the source position is valid first
    if (src.x != serverX2 || src.y != serverY2) {
      // Movement origin is not the previous destination
      bool correctSrc = false;
      if (serverX1 == serverX2 && serverY1 == serverY2) {
        // Last movement was actually stationary
        correctSrc = true;
      } else if (((src.x > serverX1) == (src.x > serverX2)) ||
                 ((src.x < serverX1) == (src.x < serverX2)) ||
                 ((src.y > serverY1) == (src.y > serverY2)) ||
                 ((src.y < serverY1) == (src.y < serverY2))) {
        // Movement origin not between movement points
        correctSrc = true;
      } else if ((serverX1 * (serverY2 - src.y) +
                  serverX2 * (src.y - serverY1) +
                  src.x * (serverY1 - serverY2)) != 0.f) {
        // Movement origin is not collinear with last movement
        correctSrc = true;
      }

      if (correctSrc) {
        // Check if it lies within the allowed movement threshold based
        // on max movement speed per before checking collision. Use
        // distance to previous movement points first as it is quicker
        // than the point to line distance formula
        if (src.GetDistance(Point(serverX2, serverY2)) > maxRatePerSec &&
            src.GetDistance(Point(serverX1, serverY1)) > maxRatePerSec) {
          float distance = 0.f;
          if (serverX2 == serverX1) {
            // Check perpendicular horizontal line distance
            distance = src.GetDistance(Point(serverX2, serverY2));
          } else {
            // Check point to line distance
            Line lastMove(Point(serverX1, serverY1), Point(serverX2, serverY2));
            distance = GetPointToLineDistance(lastMove, src);
          }

          if (distance > maxRatePerSec) {
            // Roll back movement
            result = 0x01;
            src.x = dest.x = serverX1;
            src.y = dest.y = serverY1;
            startTime = stopTime = eState->GetDestinationTicks();
            checkGeometry = false;
          }
        }

        if (!result) {
          // Check collision based on the last destination, not new
          // origin because the client has skipped ahead an
          // acceptable distance. If we don't do this the movement
          // will ocassionally "start" from outside of the wall etc.
          // Do not bother to send the corrected origin to the source
          // as the server values have not actually changed yet.
          src.x = serverX2;
          src.y = serverY2;
        }
      }
    }

    // If no error yet, check if the entity is requesting a move that is
    // too far for their max speed
    if (!result && src.GetDistance(dest) > maxRatePerSec) {
      // Do not stop the entity but limit to max speed
      dest = GetLinearPoint(src.x, src.y, dest.x, dest.y, maxRatePerSec, false);
      result = 0x01;
    }
  }

  auto geometry = zone->GetGeometry();
  if (checkGeometry && geometry) {
    // Movement origin valid and geometry exists, check collision
    Line path(Point(src.x, src.y), Point(dest.x, dest.y));

    Point collidePoint;
    Line collideLine;
    if (zone->Collides(path, collidePoint, collideLine)) {
      dest = CollisionAdjust(src, collidePoint, collideLine);
      result = 0x02;
    }
  }

  return result;
}

Point ZoneManager::CollisionAdjust(const Point& src, const Point& collidePoint,
                                   const Line& collideLine) {
  // Back off by 10 units. Typically the client stops you when you approach
  // 10 units from any geometry. Functionally you will not get "stuck" until
  // you are less than 1 unit away (but only sometimes, oddly enough).
  Point adjusted =
      GetLinearPoint(collidePoint.x, collidePoint.y, src.x, src.y, 10.f, false);

  // If the points approach the colliding line at an acute angle, its
  // possible the adjusted point is still too close to the geometry. If this
  // occurs, move it the minimum distance off the line's nearest point to
  // avoid client rounding causing the player to get stuck.
  Point nearest = GetNearestPoint(collideLine, adjusted);
  if (nearest.GetDistance(adjusted) < 1.f) {
    adjusted =
        GetLinearPoint(adjusted.x, adjusted.y, nearest.x, nearest.y, 1.f, true);
  }

  return adjusted;
}

std::list<Point> ZoneManager::GetShortestPath(const std::shared_ptr<Zone>& zone,
                                              const Point& source,
                                              const Point& dest,
                                              float maxDistance) {
  std::list<Point> result;

  bool collision = false;

  auto geometry = zone->GetGeometry();
  if (geometry) {
    Line path(source, dest);

    Point collidePoint;
    if (zone->Collides(path, collidePoint)) {
      // Grab the closest points to the source and the target, determine
      // shortest path(s) between them and simplify
      std::array<std::shared_ptr<objects::QmpNavPoint>, 2> startPoints;

      size_t idx = 0;
      for (const Point& p : {source, dest}) {
        std::list<std::pair<float, std::shared_ptr<objects::QmpNavPoint>>>
            points;
        for (auto& pair : geometry->NavPoints) {
          float dist = (float)(std::pow(((float)pair.second->GetX() - p.x), 2) +
                               std::pow(((float)pair.second->GetY() - p.y), 2));
          points.push_back(std::make_pair(dist, pair.second));
        }

        points.sort(
            [](const std::pair<float, std::shared_ptr<objects::QmpNavPoint>>& a,
               const std::pair<float, std::shared_ptr<objects::QmpNavPoint>>&
                   b) { return a.first < b.first; });

        for (auto& pair : points) {
          Line l(p,
                 Point((float)pair.second->GetX(), (float)pair.second->GetY()));
          if (!zone->Collides(l, collidePoint)) {
            startPoints[idx] = pair.second;
            break;
          }
        }

        idx++;
      }

      if (!startPoints[0] || !startPoints[1]) {
        // Impossible to calculate
        return result;
      } else if (startPoints[0] == startPoints[1]) {
        // Rounding one corner
        result.push_back(Point((float)startPoints[0]->GetX(),
                               (float)startPoints[0]->GetY()));
      } else {
        auto pointIDs = GetShortestPath(geometry, startPoints[0]->GetPointID(),
                                        startPoints[1]->GetPointID());
        if (pointIDs.size() == 0) {
          // Could not calculate
          return result;
        }

        for (uint32_t pointID : pointIDs) {
          auto n = geometry->NavPoints[pointID];
          result.push_back(Point((float)n->GetX(), (float)n->GetY()));
        }
      }

      // Skip forward from the starting point (always leave 1)
      size_t remove = 0;
      for (auto iter = result.begin(); iter != result.end(); iter++) {
        if (iter == result.begin()) continue;

        Line l(source, *iter);
        if (zone->Collides(l, collidePoint)) {
          break;
        }

        remove++;
      }

      while (remove) {
        result.pop_front();
        remove--;
      }

      // Skip forward to the end point (always leave 1)
      remove = 0;
      for (auto rIter = result.rbegin(); rIter != result.rend(); rIter++) {
        if (rIter == result.rbegin()) continue;

        Line l(dest, *rIter);
        if (zone->Collides(l, collidePoint)) {
          break;
        }

        remove++;
      }

      while (remove) {
        result.pop_back();
        remove--;
      }

      result.push_back(dest);

      // Make sure the max distance is not exceeded
      if (maxDistance > 0.f) {
        // Add source for calculating distance
        result.push_front(source);

        auto iter1 = result.begin();
        auto iter2 = result.begin();
        iter2++;

        float distance = 0.f;
        while (iter2 != result.end()) {
          distance += iter1->GetDistance(*iter2);

          iter1++;
          iter2++;
        }

        if (distance > maxDistance) {
          // Too far, return failure
          result.clear();
          return result;
        }

        result.pop_front();
      }

      collision = true;
    }
  }

  if (!collision) {
    result.push_back(dest);
  }

  return result;
}

std::list<uint32_t> ZoneManager::GetShortestPath(
    const std::shared_ptr<ZoneGeometry>& geometry, uint32_t sourceID,
    uint32_t destID) {
  std::list<uint32_t> result;

  std::set<uint32_t> check;
  std::unordered_map<uint32_t, std::shared_ptr<objects::QmpNavPoint>> points;
  std::unordered_map<uint32_t, uint32_t> paths;
  std::unordered_map<uint32_t, float> distances;

  auto it = geometry->NavPoints.find(sourceID);
  if (it == geometry->NavPoints.end()) {
    // Error
    return result;
  }

  check.insert(sourceID);
  points[sourceID] = it->second;
  distances[sourceID] = 0.f;

  // Not found yet
  distances[destID] = -1.f;

  while (check.size() > 0) {
    uint32_t pointID = *check.begin();
    float dist = distances[pointID];
    check.erase(pointID);

    auto point = geometry->NavPoints[pointID];
    for (auto& pair : point->GetDistances()) {
      float dist2 = dist + pair.second;

      // Once we find a full path, ignore nodes that go further out
      // than what we've already found
      if (distances[destID] != -1.f && dist2 > distances[destID]) continue;

      if (points.find(pair.first) == points.end()) {
        // New point reached
        it = geometry->NavPoints.find(pair.first);
        if (it != geometry->NavPoints.end()) {
          points[pair.first] = it->second;
          check.insert(pair.first);
          paths[pair.first] = point->GetPointID();
          distances[pair.first] = dist2;
        }
      } else if (distances[pair.first] > dist2) {
        // Closer path found, update the chain
        float delta = dist2 - distances[pair.first];
        distances[pair.first] = dist2;

        paths[pair.first] = point->GetPointID();

        std::set<uint32_t> affected;
        affected.insert(pair.first);

        bool repeat = true;
        while (repeat) {
          repeat = false;
          for (auto& pair2 : paths) {
            if (affected.find(pair2.first) == affected.end() &&
                affected.find(pair2.second) != affected.end()) {
              affected.insert(pair2.first);
              distances[pair2.first] += delta;
              repeat = true;
            }
          }
        }
      }
    }
  }

  if (distances[destID] != -1.f) {
    // End point was found, backtrack to get the path
    result.push_back(destID);

    uint32_t current = paths[destID];
    while (current) {
      result.push_front(current);

      auto pIter = paths.find(current);
      if (pIter != paths.end()) {
        current = pIter->second;
      } else {
        current = 0;
      }
    }
  }

  return result;
}

float ZoneManager::GetPointToLineDistance(const Line& line,
                                          const Point& point) {
  auto nearest = GetNearestPoint(line, point);
  return point.GetDistance(nearest);
}

Point ZoneManager::GetNearestPoint(const Line& line, const Point& point) {
  float xDiff = line.second.x - line.first.x;
  float yDiff = line.second.y - line.first.y;

  float calc =
      ((point.x - line.first.x) * xDiff + (point.y - line.first.y) * yDiff) /
      (xDiff * xDiff + yDiff * yDiff);

  if (calc < 0.f) {
    return line.first;
  } else if (calc > 1.f) {
    return line.second;
  } else {
    Point nearest;
    nearest.x = line.first.x + calc * xDiff;
    nearest.y = line.first.y + calc * yDiff;

    return nearest;
  }
}

bool ZoneManager::PointInPolygon(const Point& p,
                                 const std::list<Point> vertices,
                                 float overlapRadius) {
  auto p1 = vertices.begin();
  auto p2 = vertices.begin();
  p2++;

  uint32_t crosses = 0;
  size_t count = vertices.size();
  for (size_t i = 0; i < count; i++) {
    // Check if the point is on the vertex
    if (p.x == p1->x && p.y == p2->y) {
      return true;
    }

    if (((p1->y >= p.y) != (p2->y >= p.y)) &&
        (p.x <= (p2->x - p1->x) * (p.y - p1->y) / (p2->y - p1->y) + p1->x)) {
      crosses++;
    }

    if (overlapRadius && (p1->x != p2->x || p1->y != p2->y)) {
      // Check if a circle with a center at the point and radius matching
      // the supplied value enters the polygon by checking line distances
      if (p.GetDistance(*p1) <= overlapRadius) {
        // Distance to current point is smaller
        return true;
      } else {
        // Check point to line distance
        Line l(*p1, *p2);
        if (GetPointToLineDistance(l, p) <= overlapRadius) {
          return true;
        }
      }
    }

    p1++;
    p2++;

    if (p2 == vertices.end()) {
      // One left, loop back to the start
      p2 = vertices.begin();
    }
  }

  return (crosses % 2) == 1;
}

std::list<std::shared_ptr<ActiveEntityState>> ZoneManager::GetEntitiesInFoV(
    const std::list<std::shared_ptr<ActiveEntityState>>& entities, float x,
    float y, float rot, float maxAngle, bool useHitbox) {
  std::list<std::shared_ptr<ActiveEntityState>> results;

  // Max and min radians of the arc's circle
  float maxRotL = rot + maxAngle;
  float maxRotR = rot - maxAngle;

  for (auto e : entities) {
    Point ePoint(e->GetCurrentX(), e->GetCurrentY());
    float eRot = (float)atan2((float)(y - ePoint.y), (float)(x - ePoint.x));

    if (maxRotL >= eRot && maxRotR <= eRot) {
      results.push_back(e);
    } else if (useHitbox) {
      // "Shift" the center of the entity based on the rotation and
      // recalculate to see if the hitbox is included for each side
      float extend = (float)e->GetHitboxSize() * 10.f;
      for (float max : {maxRotL, maxRotR}) {
        Point exPoint(ePoint.x, ePoint.y + extend);
        exPoint = RotatePoint(exPoint, ePoint,
                              ActiveEntityState::CorrectRotation(-max));
        eRot = (float)atan2((float)(y - exPoint.y), (float)(x - exPoint.x));
        if (maxRotL >= eRot && maxRotR <= eRot) {
          results.push_back(e);
          break;
        }
      }
    }
  }

  return results;
}

void ZoneManager::ScheduleInstanceAccessTimeOut(
    const std::shared_ptr<ZoneInstance>& instance) {
  uint64_t timeOut = (uint64_t)(300000000ULL + ChannelServer::GetServerTime());
  instance->SetAccessTimeOut(timeOut);

  mServer.lock()->ScheduleWork(
      timeOut,
      [](ZoneManager* pZoneManager, uint32_t pInstanceID,
         uint64_t pExpireTime) {
        pZoneManager->ExpireInstance(pInstanceID, pExpireTime);
      },
      this, instance->GetID(), timeOut);
}

void ZoneManager::ScheduleTimerExpiration(
    const std::shared_ptr<ZoneInstance>& instance) {
  auto instVariant = instance->GetVariant();
  auto instType =
      instVariant ? instVariant->GetInstanceType() : InstanceType_t::NORMAL;

  uint64_t expireTime = instance->GetTimerExpire();
  if (!expireTime) {
    return;
  }

  bool padTimer = false;
  switch (instType) {
    case InstanceType_t::TIME_TRIAL:
    case InstanceType_t::DIASPORA:
    case InstanceType_t::MISSION:
    case InstanceType_t::DIGITALIZE:
      // These handle client timing via more accurate packets so no
      // need to pad them
      break;
    case InstanceType_t::PVP:
    case InstanceType_t::DEMON_ONLY:
    case InstanceType_t::NORMAL:
    default:
      padTimer = true;
      break;
  }

  uint64_t now = ChannelServer::GetServerTime();
  int32_t scheduleIn =
      (expireTime >= now) ? (int32_t)((expireTime - now) / 1000000ULL) : 0;
  if (padTimer) {
    // Add a 1 second buffer so we don't undershoot client times
    scheduleIn = (int32_t)(scheduleIn + 1);
  }

  // When the instance timer expires, stop the normal timer and let it
  // handle all time out events
  mServer.lock()->GetTimerManager()->ScheduleEventIn(
      (int)scheduleIn,
      [](ZoneManager* pZoneManager, uint32_t pInstanceID,
         uint64_t pExpireTime) {
        auto pInstance = pZoneManager->GetInstance(pInstanceID);
        if (pInstance && !pInstance->GetTimerStop() &&
            pInstance->GetTimerExpire() == pExpireTime) {
          LogZoneManagerDebug([&]() {
            return libcomp::String("Expiring instance timer %1: %2\n")
                .Arg(pInstance->GetTimerID())
                .Arg(pInstanceID);
          });

          pZoneManager->StopInstanceTimer(pInstance, pExpireTime);
        }
      },
      this, instance->GetID(), expireTime);
}

bool ZoneManager::ValidateBossGroup(
    const std::shared_ptr<EnemyState>& enemyState) {
  bool failed = true;

  auto zone = enemyState ? enemyState->GetZone() : nullptr;
  if (zone && zone->GetDefinition()->GetGlobalBossGroup()) {
    uint8_t groupID = enemyState->GetEntity()->GetSpawnSource()->GetBossGroup();
    uint32_t zoneGroupID = zone->GetDefinition()->GetGlobalBossGroup();

    failed = false;

    std::lock_guard<libcomp::Mutex> lock(mLock);
    for (auto uniqueID : mGlobalBossZones[zoneGroupID]) {
      for (auto boss : mZones[uniqueID]->GetBosses()) {
        auto spawn = boss->GetEntity()->GetSpawnSource();
        if (spawn->GetBossGroup() == groupID) {
          LogZoneManagerError([&]() {
            return libcomp::String(
                       "Failed to spawn duplicate global group boss %1 in zone "
                       "group %2\n")
                .Arg(groupID)
                .Arg(zoneGroupID);
          });

          failed = true;
          break;
        } else if (enemyState->GetDevilData() == boss->GetDevilData()) {
          LogZoneManagerError([&]() {
            return libcomp::String(
                       "Failed to spawn duplicate global group boss type %1 in "
                       "zone group %2\n")
                .Arg(boss->GetEnemyBase()->GetType())
                .Arg(zoneGroupID);
          });

          failed = true;
          break;
        }
      }
    }
  }

  return !failed;
}

void ZoneManager::SendMultiZoneBossStatus(uint32_t groupID) {
  uint8_t count = 0;
  std::array<std::shared_ptr<EnemyState>, 3> bosses;
  std::list<std::shared_ptr<Zone>> zones;
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    for (auto uniqueID : mGlobalBossZones[groupID]) {
      zones.push_back(mZones[uniqueID]);
      for (auto boss : mZones[uniqueID]->GetBosses()) {
        auto spawn = boss->GetEntity()->GetSpawnSource();
        bosses[(size_t)(spawn->GetBossGroup() - 1)] = boss;
        count++;
      }
    }
  }

  if (count) {
    // Send boss statuses to all players in the zones
    libcomp::Packet p;
    p.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_MULTIZONE_BOSS_STATUS);
    p.WriteS8(3);
    for (size_t i = 0; i < 3; i++) {
      auto boss = bosses[i];
      auto eBase = boss ? boss->GetEnemyBase() : nullptr;
      auto zone = boss ? boss->GetZone() : nullptr;

      p.WriteS8((int8_t)(i + 1));
      p.WriteU32Little(eBase ? eBase->GetType() : 0xFFFFFF);
      p.WriteU32Little(zone ? zone->GetDefinitionID() : 0xFFFFFF);
      p.WriteFloat(boss ? boss->GetDestinationX() : 0.f);
      p.WriteFloat(boss ? boss->GetDestinationY() : 0.f);
      p.WriteS32Little(boss ? boss->GetCoreStats()->GetHP() : 0);
      p.WriteS32Little(boss ? boss->GetMaxHP() : -1);
    }

    for (auto zone : zones) {
      libcomp::Packet copy(p);
      BroadcastPacket(zone, copy);
    }
  }
}

std::shared_ptr<Zone> ZoneManager::GetZone(
    uint32_t zoneID, uint32_t dynamicMapID,
    const std::shared_ptr<ChannelClientConnection>& client,
    uint32_t currentInstanceID) {
  auto server = mServer.lock();
  auto serverDataManager = server->GetServerDataManager();

  auto zoneDefinition = serverDataManager->GetZoneData(zoneID, dynamicMapID);
  if (!zoneDefinition) {
    return nullptr;
  }

  std::shared_ptr<Zone> zone;
  {
    if (zoneDefinition->GetGlobal()) {
      std::lock_guard<libcomp::Mutex> lock(mLock);
      auto iter = mGlobalZoneMap.find(zoneID);
      if (iter != mGlobalZoneMap.end()) {
        for (auto dPair : iter->second) {
          // If dynamicMapID is 0, check all valid instances and take
          // the first one that applies
          if (dynamicMapID == 0 || dPair.first == dynamicMapID) {
            zone = mZones[dPair.second];
            break;
          }
        }
      }

      if (nullptr == zone) {
        LogZoneManagerError([&]() {
          return libcomp::String(
                     "Global zone encountered that has not been instanced: "
                     "%1\n")
              .Arg(zoneID);
        });

        return nullptr;
      }
    } else {
      // Get or create the zone in the player instance
      auto state = client->GetClientState();

      std::shared_ptr<ZoneInstance> instance;
      {
        std::lock_guard<libcomp::Mutex> lock(mLock);
        uint32_t instanceID = currentInstanceID;
        if (!instanceID) {
          auto accessIter = mZoneInstanceAccess.find(state->GetWorldCID());
          if (accessIter != mZoneInstanceAccess.end() &&
              accessIter->second->GetIsLocal()) {
            instanceID = accessIter->second->GetInstanceID();
          }
        }

        if (instanceID == 0) {
          LogZoneManagerError([&]() {
            return libcomp::String(
                       "Character attempted to enter a zone instance that does "
                       "not exist: %1\n")
                .Arg(state->GetAccountUID().ToString());
          });

          return nullptr;
        }

        auto instIter = mZoneInstances.find(instanceID);
        if (instIter == mZoneInstances.end()) {
          LogZoneManagerError([&]() {
            return libcomp::String(
                       "Character could not be added to the requested "
                       "instance: %1\n")
                .Arg(state->GetAccountUID().ToString());
          });

          return nullptr;
        }

        instance = instIter->second;
      }

      zone = GetInstanceZone(instance, zoneID, dynamicMapID);
    }
  }

  return zone;
}

std::shared_ptr<Zone> ZoneManager::GetInstanceZone(
    const std::shared_ptr<ZoneInstance>& instance, uint32_t zoneID,
    uint32_t dynamicMapID) {
  std::lock_guard<libcomp::Mutex> iLock(mInstanceZoneLock);

  auto zone = instance->GetZone(zoneID, dynamicMapID);
  if (zone) {
    return zone;
  }

  // Ensure the zone is part of the instance definition and create it
  auto instanceDef = instance->GetDefinition();
  auto instVariant = instance->GetVariant();

  auto server = mServer.lock();
  auto serverDataManager = server->GetServerDataManager();

  std::shared_ptr<objects::ServerZone> zoneDefinition;

  for (size_t i = 0; i < instanceDef->ZoneIDsCount(); i++) {
    uint32_t zID = instanceDef->GetZoneIDs(i);
    uint32_t dID = instanceDef->GetDynamicMapIDs(i);
    if (zID == zoneID && (dynamicMapID == 0 || dID == dynamicMapID)) {
      std::set<uint32_t> partialIDs;
      if (instVariant) {
        partialIDs = instVariant->GetZonePartialIDs();
      }

      zoneDefinition =
          serverDataManager->GetZoneData(zID, dID, true, partialIDs);
      break;
    }
  }

  if (zoneDefinition) {
    zone = CreateZone(zoneDefinition, instance);
    if (!instance->AddZone(zone)) {
      LogZoneManagerError([&]() {
        return libcomp::String("Failed to add zone to instance: %1 (%2)\n")
            .Arg(zoneID)
            .Arg(dynamicMapID);
      });

      std::lock_guard<libcomp::Mutex> lock(mLock);
      RemoveZone(zone, false);
      return nullptr;
    }
  } else {
    LogZoneManagerError([&]() {
      return libcomp::String(
                 "Attempted to add invalid zone to instance: %1 (%2)\n")
          .Arg(zoneID)
          .Arg(dynamicMapID);
    });

    return nullptr;
  }

  return zone;
}

bool ZoneManager::GetLoginZone(
    const std::shared_ptr<objects::Character>& character, uint32_t& zoneID,
    uint32_t& dynamicMapID, int8_t& channelID, float& x, float& y, float& rot) {
  // Default to last logout information first
  zoneID = character->GetLogoutZone();
  dynamicMapID = 0;
  channelID = -1;
  x = character->GetLogoutX();
  y = character->GetLogoutY();
  rot = character->GetLogoutRotation();

  auto server = mServer.lock();
  auto serverDataManager = server->GetServerDataManager();

  // Make sure the player can start in the zone
  if (zoneID) {
    auto zoneData = serverDataManager->GetZoneData(zoneID, 0);
    if (!zoneData) {
      // Can't discern any information about the logout zone
      zoneID = 0;
    } else if (!zoneData->GetGlobal() || zoneData->GetRestricted()) {
      // Determine which public zone to go to instead, defaulting to the
      // lobby on the last instance, then the one matching the group ID
      uint32_t publicID = 0;
      if (character->GetLogoutInstance()) {
        auto instDef = serverDataManager->GetZoneInstanceData(
            character->GetLogoutInstance());
        if (instDef &&
            serverDataManager->GetZoneData(instDef->GetLobbyID(), 0)) {
          publicID = instDef->GetLobbyID();
        }
      }

      if (!publicID) {
        publicID = zoneData->GetGroupID();
      }

      if (!publicID && character->GetPreviousZone()) {
        // If there is no group for the zone, return to
        // the previous public zone
        publicID = character->GetPreviousZone();
      }

      auto publicData = serverDataManager->GetZoneData(publicID, 0);
      if (publicData && publicData->GetGlobal()) {
        // See if the zone definition has a starting spot for a parent
        // connection, otherwise go to the server starting position
        auto def = server->GetDefinitionManager()->GetZoneData(zoneID);
        auto basic = def ? def->GetBasic() : nullptr;

        zoneID = publicData->GetID();
        dynamicMapID = publicData->GetDynamicMapID();

        if (!basic || basic->GetParentID() != zoneID ||
            !GetSpotPosition(dynamicMapID, basic->GetStartingSpot(), x, y,
                             rot)) {
          x = publicData->GetStartingX();
          y = publicData->GetStartingY();
          rot = publicData->GetStartingRotation();
        }
      } else {
        // Correct it further down
        zoneID = 0;
      }
    }
  }

  // Default to homepoint second
  if (zoneID == 0) {
    zoneID = character->GetHomepointZone();

    auto zoneData = serverDataManager->GetZoneData(zoneID, 0);
    if (zoneData) {
      dynamicMapID = zoneData->GetDynamicMapID();
      GetSpotPosition(dynamicMapID, character->GetHomepointSpotID(), x, y, rot);
    }
  }

  // If all else fails start in the default zone
  if (zoneID == 0) {
    auto zoneData = serverDataManager->GetZoneData(SVR_CONST.ZONE_DEFAULT, 0);
    if (zoneData) {
      zoneID = zoneData->GetID();
      dynamicMapID = zoneData->GetDynamicMapID();
      x = zoneData->GetStartingX();
      y = zoneData->GetStartingY();
      rot = zoneData->GetStartingRotation();
    }
  }

  auto zoneData = serverDataManager->GetZoneData(zoneID, dynamicMapID);
  if (zoneData) {
    // Set dynamic map ID again in case its not set
    dynamicMapID = zoneData->GetDynamicMapID();

    auto sharedConfig = server->GetWorldSharedConfig();
    if (sharedConfig->ChannelDistributionCount()) {
      // Channel distribution configured, determine which channel
      // has the zone
      if (!zoneData->GetGlobal()) {
        // Shouldn't get here with an instance zone
        return false;
      }

      // Entries that do not exist are mapped to channel 0
      channelID =
          (int8_t)sharedConfig->GetChannelDistribution(zoneData->GetGroupID());
    }

    return true;
  } else {
    return false;
  }
}

std::shared_ptr<Zone> ZoneManager::CreateZone(
    const std::shared_ptr<objects::ServerZone>& definition,
    const std::shared_ptr<ZoneInstance>& instance) {
  if (nullptr == definition) {
    return nullptr;
  }

  uint32_t zoneID = definition->GetID();
  uint32_t dynamicMapID = definition->GetDynamicMapID();

  libcomp::String zoneStr = libcomp::String("%1%2").Arg(zoneID).Arg(
      zoneID != dynamicMapID ? libcomp::String(" (%1)").Arg(dynamicMapID) : "");

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto zoneData = definitionManager->GetZoneData(zoneID);

  std::shared_ptr<Zone> zone;
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    uint32_t id = mNextZoneID++;

    zone = std::shared_ptr<Zone>(new Zone(id, definition));

    if (instance) {
      zone->SetInstance(instance);
      zone->SetMatch(instance->GetMatch());
    }

    auto qmpFile = zoneData->GetFile()->GetQmpFile();
    auto geoIter = !qmpFile.IsEmpty() ? mZoneGeometry.find(qmpFile.C())
                                      : mZoneGeometry.end();
    if (geoIter != mZoneGeometry.end()) {
      zone->SetGeometry(geoIter->second);
    }

    auto it = mDynamicMaps.find(dynamicMapID);
    if (it != mDynamicMaps.end()) {
      zone->SetDynamicMap(it->second);
    } else {
      LogZoneManagerWarning([zoneID, dynamicMapID]() {
        return libcomp::String(
                   "Creating zone %1 with invalid dynamic map ID %2. Zone will "
                   "still be usable but most spot functionality will be "
                   "disabled.\n")
            .Arg(zoneID)
            .Arg(dynamicMapID);
      });
    }
  }

  for (auto npc : definition->GetNPCs()) {
    auto copy = std::make_shared<objects::ServerNPC>(*npc);

    auto state = std::shared_ptr<NPCState>(new NPCState(copy));

    float x = npc->GetX();
    float y = npc->GetY();
    float rot = npc->GetRotation();
    if (npc->GetSpotID() &&
        !GetSpotPosition(dynamicMapID, npc->GetSpotID(), x, y, rot)) {
      LogZoneManagerWarning([&]() {
        return libcomp::String(
                   "NPC %1 in zone %2 is placed in an invalid spot and will be "
                   "ignored: %3\n")
            .Arg(npc->GetID())
            .Arg(zoneStr)
            .Arg(npc->GetSpotID());
      });

      continue;
    }

    state->SetCurrentX(x);
    state->SetCurrentY(y);
    state->SetCurrentRotation(rot);

    state->SetEntityID(server->GetNextEntityID());
    zone->AddNPC(state);
  }

  std::set<uint32_t> diasporaSpots;
  if (instance && instance->GetVariant()) {
    auto instDef = instance->GetDefinition();
    auto instVariant = instance->GetVariant();

    // Apply any special instance changes
    switch (instVariant->GetInstanceType()) {
      case InstanceType_t::PVP:
        // PvP instances should have bases added to the starting zone
        if (instDef->GetZoneIDs(0) == definition->GetID() &&
            instDef->GetDynamicMapIDs(0) == definition->GetDynamicMapID()) {
          auto pvpVariant =
              std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
                  instVariant);
          if (pvpVariant) {
            AddPvPBases(zone, pvpVariant);
          }
        }
        break;
      case InstanceType_t::DIASPORA:
        // If a server object is placed on the same spot ID as a diaspora
        // base, do not place it as the spot will be bound to it later
        for (auto& spotPair : definition->GetSpots()) {
          if (spotPair.second->GetMatchBase()) {
            diasporaSpots.insert(spotPair.first);
          }
        }

        AddDiasporaBases(zone);
        break;
      default:
        break;
    }
  }

  for (auto obj : definition->GetObjects()) {
    if (obj->GetSpotID() &&
        diasporaSpots.find(obj->GetSpotID()) != diasporaSpots.end()) {
      continue;
    }

    auto copy = std::make_shared<objects::ServerObject>(*obj);

    auto state =
        std::shared_ptr<ServerObjectState>(new ServerObjectState(copy));

    float x = obj->GetX();
    float y = obj->GetY();
    float rot = obj->GetRotation();
    if (obj->GetSpotID() &&
        !GetSpotPosition(dynamicMapID, obj->GetSpotID(), x, y, rot)) {
      LogZoneManagerWarning([&]() {
        return libcomp::String(
                   "Object %1 in zone %2 is placed in an invalid spot and will "
                   "be ignored: %3\n")
            .Arg(obj->GetID())
            .Arg(zoneStr)
            .Arg(obj->GetSpotID());
      });

      continue;
    }

    state->SetCurrentX(x);
    state->SetCurrentY(y);
    state->SetCurrentRotation(rot);

    state->SetEntityID(server->GetNextEntityID());
    zone->AddObject(state);

    // Objects are assumed to be enabled by default so check geometry
    if (IsGeometryDisabled(copy)) {
      UpdateGeometryElement(zone, copy);
    }
  }

  if (definition->PlasmaSpawnsCount() > 0) {
    for (auto plasmaPair : definition->GetPlasmaSpawns()) {
      auto pSpawn = plasmaPair.second;
      auto state = std::make_shared<PlasmaState>(pSpawn);

      float x = pSpawn->GetX();
      float y = pSpawn->GetY();
      float rot = pSpawn->GetRotation();
      if (pSpawn->GetSpotID() &&
          !GetSpotPosition(dynamicMapID, pSpawn->GetSpotID(), x, y, rot)) {
        LogZoneManagerWarning([&]() {
          return libcomp::String(
                     "Plasma %1 in zone %2 is placed in an invalid spot and "
                     "will be ignored.\n")
              .Arg(pSpawn->GetID())
              .Arg(zoneStr);
        });

        continue;
      }

      state->SetCurrentX(x);
      state->SetCurrentY(y);
      state->SetCurrentRotation(rot);

      state->CreatePoints();

      state->SetEntityID(server->GetNextEntityID());

      auto restriction = pSpawn->GetRestrictions();
      if (restriction && restriction->GetDisabled()) {
        // Explicitly deactivate it to start
        state->Toggle(false, true);
      }

      zone->AddPlasma(state);
    }

    UpdatePlasma(zone);
  }

  if (definition->BazaarsCount() > 0) {
    uint8_t channelID = server->GetChannelID();
    bool distributedZones =
        server->GetWorldSharedConfig()->ChannelDistributionCount() > 0;

    std::list<std::shared_ptr<objects::BazaarData>> activeMarkets;
    for (auto m : objects::BazaarData::LoadBazaarDataListByZone(
             server->GetWorldDatabase(), zoneID)) {
      if (m->GetState() == objects::BazaarData::State_t::BAZAAR_ACTIVE &&
          (distributedZones || m->GetChannelID() == channelID)) {
        activeMarkets.push_back(m);
      }
    }

    for (auto bazaar : definition->GetBazaars()) {
      auto state = std::make_shared<BazaarState>(bazaar);

      float x = bazaar->GetX();
      float y = bazaar->GetY();
      float rot = bazaar->GetRotation();
      if (bazaar->GetSpotID() &&
          !GetSpotPosition(dynamicMapID, bazaar->GetSpotID(), x, y, rot)) {
        LogZoneManagerWarning([&]() {
          return libcomp::String(
                     "Bazaar %1 in zone %2 is placed in an invalid spot and "
                     "will be ignored.\n")
              .Arg(bazaar->GetID())
              .Arg(zoneStr);
        });

        continue;
      }

      state->SetCurrentX(x);
      state->SetCurrentY(y);
      state->SetCurrentRotation(rot);

      state->SetEntityID(server->GetNextEntityID());

      for (auto m : activeMarkets) {
        if (bazaar->MarketIDsContains(m->GetMarketID())) {
          state->SetCurrentMarket(m->GetMarketID(), m);
        }
      }

      zone->AddBazaar(state);
    }
  }

  if (definition->CultureMachinesCount() > 0) {
    std::list<std::shared_ptr<objects::CultureData>> activeMachines;
    for (auto m : objects::CultureData::LoadCultureDataListByZone(
             server->GetWorldDatabase(), zoneID)) {
      if (m->GetActive()) {
        activeMachines.push_back(m);
      }
    }

    for (auto machineSet : definition->GetCultureMachines()) {
      for (auto machine : machineSet->GetMachines()) {
        auto state = std::shared_ptr<CultureMachineState>(
            new CultureMachineState(machine->GetID(), machineSet));

        float x = machine->GetX();
        float y = machine->GetY();
        float rot = machine->GetRotation();
        if (machine->GetSpotID() &&
            !GetSpotPosition(dynamicMapID, machine->GetSpotID(), x, y, rot)) {
          LogZoneManagerWarning([&]() {
            return libcomp::String(
                       "Culture machine %1 in zone %2 is placed in an invalid "
                       "spot and will be ignored.\n")
                .Arg(machine->GetID())
                .Arg(zoneStr);
          });

          continue;
        }

        state->SetCurrentX(x);
        state->SetCurrentY(y);
        state->SetCurrentRotation(rot);

        state->SetEntityID(server->GetNextEntityID());

        for (auto m : activeMachines) {
          if (machine->GetID() == m->GetMachineID()) {
            state->SetRentalData(m);
          }
        }

        zone->AddCultureMachine(state);
      }
    }
  }

  if (definition->BazaarsCount() > 0 ||
      definition->CultureMachinesCount() > 0) {
    // Set/expire any existing rental expirations
    ExpireRentals(zone);
  }

  // Gather setup triggers and sort all other types from the definition
  std::list<std::shared_ptr<objects::ServerZoneTrigger>> setupTriggers;
  for (auto trigger : definition->GetTriggers()) {
    switch (trigger->GetTrigger()) {
      case objects::ServerZoneTrigger::Trigger_t::ON_SETUP:
        setupTriggers.push_back(trigger);
        break;
      case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_IN:
      case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_OUT:
      case objects::ServerZoneTrigger::Trigger_t::PRE_ZONE_IN:
      case objects::ServerZoneTrigger::Trigger_t::ON_LOGIN:
        zone->AppendZoneChangeTriggers(trigger);
        break;
      case objects::ServerZoneTrigger::Trigger_t::ON_SPAWN:
      case objects::ServerZoneTrigger::Trigger_t::ON_RESPAWN:
        zone->AppendSpawnTriggers(trigger);
        break;
      case objects::ServerZoneTrigger::Trigger_t::ON_DEATH:
      case objects::ServerZoneTrigger::Trigger_t::ON_REVIVAL:
        zone->AppendDeathTriggers(trigger);
        break;
      case objects::ServerZoneTrigger::Trigger_t::ON_FLAG_SET:
        zone->AppendFlagSetTriggers(trigger);
        zone->InsertFlagSetKeys(trigger->GetValue());
        break;
      case objects::ServerZoneTrigger::Trigger_t::ON_PHASE:
      case objects::ServerZoneTrigger::Trigger_t::ON_PVP_START:
      case objects::ServerZoneTrigger::Trigger_t::ON_PVP_BASE_CAPTURE:
      case objects::ServerZoneTrigger::Trigger_t::ON_PVP_COMPLETE:
      case objects::ServerZoneTrigger::Trigger_t::ON_DIASPORA_BASE_CAPTURE:
      case objects::ServerZoneTrigger::Trigger_t::ON_DIASPORA_BASE_RESET:
      case objects::ServerZoneTrigger::Trigger_t::ON_TOKUSEI_EXPIRED:
      case objects::ServerZoneTrigger::Trigger_t::ON_UB_TICK:
      case objects::ServerZoneTrigger::Trigger_t::ON_UB_GAUGE_OVER:
      case objects::ServerZoneTrigger::Trigger_t::ON_UB_GAUGE_UNDER:
        zone->AppendMiscTriggers(trigger);
        break;
      case objects::ServerZoneTrigger::Trigger_t::ON_TIME:
      case objects::ServerZoneTrigger::Trigger_t::ON_SYSTEMTIME:
      case objects::ServerZoneTrigger::Trigger_t::ON_MOONPHASE:
        zone->AppendTimeTriggers(trigger);
        break;
      default:
        break;
    }
  }

  // Zone successfully created, register with the manager
  {
    std::lock_guard<libcomp::Mutex> lock(mLock);
    mZones[zone->GetID()] = zone;
  }

  // Register time restrictions and calculate current state if any exist
  if (RegisterTimeRestrictions(zone, definition)) {
    auto clock = server->GetWorldClockTime();
    zone->UpdateTimedSpawns(clock, true);
  }

  // Run all setup actions
  if (setupTriggers.size() > 0) {
    auto actionManager = server->GetActionManager();
    for (auto trigger : setupTriggers) {
      actionManager->PerformActions(nullptr, trigger->GetActions(), 0, zone);
    }
  }

  // Populate all spawnpoints
  UpdateSpawnGroups(zone, true);

  return zone;
}

void ZoneManager::AddPvPBases(
    const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<objects::PvPInstanceVariant>& variant) {
  auto baseRanks = variant->GetBaseRanks();
  if (baseRanks.size() == 0) {
    // Nothing to do
    return;
  }

  auto def = zone->GetDefinition();
  auto server = mServer.lock();

  std::set<uint32_t> validSpotIDs;
  for (auto& pair :
       server->GetDefinitionManager()->GetSpotData(def->GetDynamicMapID())) {
    if ((uint8_t)pair.second->GetType() == variant->GetBaseSpotType()) {
      validSpotIDs.insert(pair.first);
    }
  }

  // Bind to explicit spots before continuing
  std::unordered_map<uint8_t, std::set<uint32_t>> rankSpots;
  for (auto& pair : def->GetSpots()) {
    if (pair.second->GetMatchBase()) {
      rankSpots[pair.second->GetMatchBase()].insert(pair.first);
    }
  }

  std::list<std::pair<uint8_t, uint32_t>> boundSpots;
  if (rankSpots.size() > 0) {
    std::list<uint8_t> ranks;
    for (uint8_t rank : baseRanks) {
      if (rankSpots[rank].size() > 0) {
        uint32_t spotID = libcomp::Randomizer::GetEntry(rankSpots[rank]);
        rankSpots[rank].erase(spotID);
        validSpotIDs.erase(spotID);

        boundSpots.push_back(std::pair<uint8_t, uint32_t>(rank, spotID));
      } else {
        ranks.push_back(rank);
      }
    }

    baseRanks = ranks;
  }

  // Remove any spots that have NPCs or objects in them
  for (auto npc : def->GetNPCs()) {
    validSpotIDs.erase(npc->GetSpotID());
  }

  for (auto obj : def->GetObjects()) {
    validSpotIDs.erase(obj->GetSpotID());
  }

  // Bind the rest of the spots
  for (uint8_t rank : baseRanks) {
    uint32_t spotID = libcomp::Randomizer::GetEntry(validSpotIDs);
    if (spotID) {
      boundSpots.push_back(std::pair<uint8_t, uint32_t>(rank, spotID));
    }

    validSpotIDs.erase(spotID);
  }

  // Now place the bound spots
  uint8_t baseCount = 0;
  for (auto& bPair : boundSpots) {
    uint8_t bRank = bPair.first;
    uint32_t spotID = bPair.second;

    float x, y, rot;
    if (spotID && GetSpotPosition(def->GetDynamicMapID(), spotID, x, y, rot)) {
      auto pvpBase = std::make_shared<objects::PvPBase>();
      pvpBase->SetRank(bRank);

      switch (bRank) {
        case 1:
          // Always 3
          pvpBase->SetSpeed(3);
          break;
        case 10:
          // Always 1
          pvpBase->SetSpeed(1);
          break;
        default:
          // Random value between 1 and 3
          pvpBase->SetSpeed((uint8_t)RNG(int32_t, 1, 3));
          break;
      }

      auto bState = std::make_shared<PvPBaseState>(pvpBase);
      bState->SetCurrentX(x);
      bState->SetCurrentY(y);
      bState->SetCurrentRotation(rot);

      bState->SetEntityID(server->GetNextEntityID());

      zone->AddBase(bState);

      validSpotIDs.erase(spotID);
    } else {
      LogZoneManagerWarning([&]() {
        return libcomp::String(
                   "One or more PvP bases could not be placed in zone %1 from "
                   "variant %2\n")
            .Arg(def->GetID())
            .Arg(variant->GetID());
      });

      break;
    }

    // UI only supports 5 bases per zone
    if (++baseCount == 5) {
      break;
    }
  }
}

void ZoneManager::AddDiasporaBases(const std::shared_ptr<Zone>& zone) {
  auto instance = zone->GetInstance();
  auto variant = instance ? instance->GetVariant() : nullptr;
  if (!variant || variant->GetInstanceType() != InstanceType_t::DIASPORA) {
    return;
  }

  auto def = zone->GetDefinition();

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  // Gather objects to bind to the bases (see CreateZone for the other half
  // of this)
  std::unordered_map<uint32_t, std::shared_ptr<objects::ServerObject>>
      spotObjects;
  for (auto obj : def->GetObjects()) {
    if (obj->GetSpotID()) {
      spotObjects[obj->GetSpotID()] = obj;
    }
  }

  // Pull all spots local and sort by ID as the order they are added to the
  // zone matters for displaying correctly in the zone
  std::list<std::shared_ptr<objects::ServerZoneSpot>> baseSpots;
  for (auto& spotPair : def->GetSpots()) {
    uint8_t matchBase = spotPair.second->GetMatchBase();
    if (matchBase) {
      baseSpots.push_back(spotPair.second);
    }
  }

  baseSpots.sort([](const std::shared_ptr<objects::ServerZoneSpot>& a,
                    const std::shared_ptr<objects::ServerZoneSpot>& b) {
    return a->GetID() < b->GetID();
  });

  std::set<uint32_t> invalidSpotIDs;
  for (auto spot : baseSpots) {
    uint32_t spotID = spot->GetID();

    auto towerData = definitionManager->GetUraFieldTowerData(
        variant->GetSubID(), spot->GetMatchBase());

    float x, y, rot;
    if (!towerData) {
      LogZoneManagerWarning([&]() {
        return libcomp::String("Invalid Diaspora base encountered in zone %1\n")
            .Arg(def->GetID());
      });
    } else if (invalidSpotIDs.find(spotID) != invalidSpotIDs.end()) {
      LogZoneManagerWarning([&]() {
        return libcomp::String(
                   "Diaspora base %1 specified multiple times in zone %2\n")
            .Arg(spotID)
            .Arg(def->GetID());
      });
    } else if (GetSpotPosition(def->GetDynamicMapID(), spotID, x, y, rot)) {
      auto dBase = std::make_shared<objects::DiasporaBase>();
      dBase->SetDefinition(towerData);

      dBase->SetBoundObject(spotObjects[spotID]);
      if (!dBase->GetBoundObject()) {
        LogZoneManagerWarning([&]() {
          return libcomp::String(
                     "Diaspora base with no bound server object encountered in "
                     "zone %1 at spot: %2\n")
              .Arg(def->GetID())
              .Arg(spotID);
        });
      }

      auto bState = std::make_shared<DiasporaBaseState>(dBase);
      bState->SetCurrentX(x);
      bState->SetCurrentY(y);
      bState->SetCurrentRotation(rot);

      bState->SetEntityID(server->GetNextEntityID());

      zone->AddBase(bState);

      invalidSpotIDs.erase(spotID);
    } else {
      LogZoneManagerWarning([&]() {
        return libcomp::String(
                   "Invalid Diaspora base spot %1 encountered in zone %2\n")
            .Arg(spotID)
            .Arg(def->GetID());
      });
    }
  }
}

bool ZoneManager::CanEnterRestrictedZone(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<Zone>& zone) {
  if (!zone || !client) {
    return false;
  }

  auto state = client->GetClientState();
  auto def = zone->GetDefinition();
  if (!def->GetRestricted()) {
    // Not actually restricted
    return true;
  }

  auto match = zone->GetMatch();
  if (match) {
    // Can enter zone if part of the match
    if (match->MemberIDsContains(state->GetWorldCID())) {
      return true;
    }

    auto ubMatch = std::dynamic_pointer_cast<objects::UBMatch>(match);
    if (ubMatch && ubMatch->SpectatorIDsContains(state->GetWorldCID())) {
      return true;
    }

    return false;
  }

  // No explicit restrictions
  return true;
}

void ZoneManager::SendAccessMessage(
    const std::shared_ptr<objects::InstanceAccess>& access, bool joined,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto server = mServer.lock();
  auto serverDataManager = server->GetServerDataManager();

  auto instDef =
      access ? serverDataManager->GetZoneInstanceData(access->GetDefinitionID())
             : nullptr;
  if (instDef) {
    int32_t messageID =
        joined ? instDef->GetJoinMessageID() : instDef->GetCreateMessageID();
    auto createEventID = !joined ? instDef->GetCreateEventID() : "";
    if (messageID || !createEventID.IsEmpty()) {
      std::list<std::shared_ptr<ChannelClientConnection>> clients;
      if (client) {
        clients.push_back(client);
      } else {
        clients = server->GetManagerConnection()->GetEntityClients(
            access->GetAccessCIDs(), true);
      }

      if (messageID) {
        auto actionManager = server->GetActionManager();
        for (auto c : clients) {
          actionManager->SendStageEffect(c, messageID, 0, true);
        }
      }

      if (!createEventID.IsEmpty()) {
        auto eventManager = server->GetEventManager();
        for (auto c : clients) {
          EventOptions options;
          options.AutoOnly = true;
          options.NoInterrupt = true;

          auto state = c->GetClientState();
          auto entityID = state->GetCharacterState()->GetEntityID();
          eventManager->HandleEvent(c, createEventID, entityID, nullptr,
                                    options);
        }
      }
    }
  }
}

void ZoneManager::RemoveZone(const std::shared_ptr<Zone>& zone,
                             bool freezeOnly) {
  if (!freezeOnly) {
    mZones.erase(zone->GetID());
    zone->Cleanup();
    mTimeRestrictUpdatedZones.erase(zone->GetID());
  } else {
    // Remove any AI aggro in the zone
    auto eBases = zone->GetEnemiesAndAllies();
    if (eBases.size() > 0) {
      auto aiManager = mServer.lock()->GetAIManager();
      for (auto eBase : eBases) {
        aiManager->UpdateAggro(eBase, -1);
      }
    }
  }

  mActiveZones.erase(zone->GetID());
  mActiveTrackedZones.erase(zone->GetID());

  if (!freezeOnly) {
    std::list<WorldClockTime> removeSpawnTimes;

    if (mAllTimeRestrictZones.find(zone->GetID()) !=
        mAllTimeRestrictZones.end()) {
      for (auto tPair : mSpawnTimeRestrictZones) {
        tPair.second.erase(zone->GetID());
        if (tPair.second.size() == 0) {
          removeSpawnTimes.push_back(tPair.first);
        }
      }

      mAllTimeRestrictZones.erase(zone->GetID());
    }

    // Clean up any time restrictions
    if (removeSpawnTimes.size() > 0) {
      auto server = mServer.lock();
      for (auto& t : removeSpawnTimes) {
        server->RegisterClockEvent(t, 1, true);
      }
    }
  }
}

bool ZoneManager::RemoveInstance(uint32_t instanceID) {
  auto instIter = mZoneInstances.find(instanceID);
  if (instIter == mZoneInstances.end()) {
    return false;
  }

  auto instance = instIter->second;

  std::list<std::shared_ptr<Zone>> cleanupZones;
  for (auto z : instance->GetZones()) {
    if (z->GetConnections().size() == 0) {
      cleanupZones.push_back(z);
    } else {
      return false;
    }
  }

  // Since the zones will all be cleaned up, drop
  // the instance and remove all access
  auto access = instance->GetAccess();
  for (int32_t accessCID : access->GetAccessCIDs()) {
    auto it = mZoneInstanceAccess.find(accessCID);
    if (it != mZoneInstanceAccess.end() &&
        it->second->GetRequestID() == access->GetRequestID()) {
      mZoneInstanceAccess.erase(it);
    }
  }

  access->ClearAccessCIDs();

  LogZoneManagerDebug([&]() {
    return libcomp::String("%1 zone instance: %2 (%3)\n")
        .Arg(instance->GetAccessTimeOut() ? "Expiring" : "Cleaning up")
        .Arg(instance->GetID())
        .Arg(instance->GetDefinitionID());
  });

  mZoneInstances.erase(instance->GetID());

  for (auto z : cleanupZones) {
    RemoveZone(z, false);
  }

  mServer.lock()->GetChannelSyncManager()->SyncRecordRemoval(access,
                                                             "InstanceAccess");

  return true;
}

bool ZoneManager::IsGeometryDisabled(
    const std::shared_ptr<objects::ServerObject>& obj) {
  // Two open states and one hidden state
  return obj->GetState() == 2 || obj->GetState() == 3 ||
         obj->GetState() == ONPC_STATE_HIDE;
}

bool ZoneManager::RegisterTimeRestrictions(
    const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<objects::ServerZone>& definition) {
  std::list<WorldClockTime> spawnTimes;

  // Gather spawn restrictions from spawn groups and plasma
  std::list<std::shared_ptr<objects::SpawnRestriction>> restrictions;
  for (auto sgPair : definition->GetSpawnGroups()) {
    auto sg = sgPair.second;
    auto restriction = sg->GetRestrictions();
    if (restriction) {
      restrictions.push_back(restriction);
    }
  }

  for (auto pPair : definition->GetPlasmaSpawns()) {
    auto plasma = pPair.second;
    auto restriction = plasma->GetRestrictions();
    if (restriction) {
      restrictions.push_back(restriction);
    }
  }

  // Build times from spawn restrictions
  for (auto restriction : restrictions) {
    std::set<int8_t> phases;
    if (restriction->GetMoonRestriction() != 0xFFFF) {
      for (int8_t p = 0; p < 16; p++) {
        if (((restriction->GetMoonRestriction() >> p) & 0x01) != 0) {
          // Add the phase and next phase
          phases.insert(p);
          phases.insert((int8_t)((p + 1) % 16));
        }
      }
    }

    if (restriction->TimeRestrictionCount() > 0) {
      std::list<WorldClockTime> gameTimes;
      for (auto pair : restriction->GetTimeRestriction()) {
        WorldClockTime after;
        after.Hour = (int8_t)(pair.first / 100 % 24);
        after.Min = (int8_t)((pair.first % 100) % 60);
        gameTimes.push_back(after);

        // Actual end time is one minute later
        WorldClockTime before;
        before.Hour = (int8_t)(pair.second / 100 % 24);
        before.Min = (int8_t)((pair.second % 100) % 60);
        if (before.Min == 59) {
          before.Min = 0;
          before.Hour = (int8_t)((before.Hour + 1) % 24);
        } else {
          before.Min++;
        }

        gameTimes.push_back(before);
      }

      if (phases.size() > 0) {
        // Phase and game time
        for (int8_t phase : phases) {
          for (WorldClockTime t : gameTimes) {
            t.MoonPhase = phase;
            spawnTimes.push_back(t);
          }
        }
      } else {
        // Game time only
        for (WorldClockTime t : gameTimes) {
          spawnTimes.push_back(t);
        }
      }
    } else if (restriction->SystemTimeRestrictionCount() > 0) {
      std::list<WorldClockTime> sysTimes;
      for (auto pair : restriction->GetSystemTimeRestriction()) {
        WorldClockTime after;
        after.SystemHour = (int8_t)(pair.first / 100 % 24);
        after.SystemMin = (int8_t)((pair.first % 100) % 60);
        sysTimes.push_back(after);

        // Actual end time is one minute later
        WorldClockTime before;
        before.SystemHour = (int8_t)(pair.second / 100 % 24);
        before.SystemMin = (int8_t)((pair.second % 100) % 60);
        if (before.SystemMin == 59) {
          before.SystemMin = 0;
          before.SystemHour = (int8_t)((before.SystemHour + 1) % 24);
        } else {
          before.SystemMin++;
        }

        sysTimes.push_back(before);
      }

      if (phases.size() > 0) {
        // Phase and system time
        for (int8_t phase : phases) {
          for (WorldClockTime t : sysTimes) {
            t.MoonPhase = phase;
            spawnTimes.push_back(t);
          }
        }
      } else {
        // System time only
        for (WorldClockTime t : sysTimes) {
          spawnTimes.push_back(t);
        }
      }
    } else if (phases.size() > 0) {
      // Phase only
      for (int8_t phase : phases) {
        WorldClockTime t;
        t.MoonPhase = phase;
        spawnTimes.push_back(t);
      }
    }

    // If day or date restrictions are set, add midnight too
    if (restriction->GetDayRestriction() != 0x8F ||
        restriction->DateRestrictionCount() > 0) {
      WorldClockTime t;
      t.SystemHour = 0;
      t.SystemMin = 0;
      spawnTimes.push_back(t);
    }
  }

  // Build event times
  auto eventTimes = GetTriggerTimes(definition->GetTriggers());

  // Register all times
  if (spawnTimes.size() > 0 || eventTimes.size() > 0) {
    auto server = mServer.lock();

    for (auto& t : spawnTimes) {
      mSpawnTimeRestrictZones[t].insert(zone->GetID());
      server->RegisterClockEvent(t, 1, false);
    }

    for (auto& t : eventTimes) {
      mSpawnTimeRestrictZones[t].insert(zone->GetID());
      server->RegisterClockEvent(t, 3, false);
    }

    mAllTimeRestrictZones.insert(zone->GetID());

    return true;
  }

  return false;
}

std::list<WorldClockTime> ZoneManager::GetTriggerTimes(
    const std::list<std::shared_ptr<objects::ServerZoneTrigger>>& triggers) {
  std::list<WorldClockTime> times;
  for (auto trigger : triggers) {
    switch (trigger->GetTrigger()) {
      case objects::ServerZoneTrigger::Trigger_t::ON_TIME: {
        WorldClockTime t;
        t.Hour = (int8_t)(trigger->GetValue() / 100);
        t.Min = (int8_t)(trigger->GetValue() % 100);
        times.push_back(t);
      } break;
      case objects::ServerZoneTrigger::Trigger_t::ON_SYSTEMTIME: {
        WorldClockTime t;
        t.SystemHour = (int8_t)(trigger->GetValue() / 100);
        t.SystemMin = (int8_t)(trigger->GetValue() % 100);
        times.push_back(t);
      } break;
      case objects::ServerZoneTrigger::Trigger_t::ON_MOONPHASE: {
        WorldClockTime t;
        t.MoonPhase = (int8_t)trigger->GetValue();
        times.push_back(t);
      } break;
      default:
        break;
    }
  }

  return times;
}
