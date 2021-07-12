/**
 * @file server/world/src/WorldSyncManager.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief World specific implementation of the DataSyncManager in
 *  charge of performing server side update operations.
 *
 * This file is part of the World Server (world).
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

#include "WorldSyncManager.h"

// libhack Includes
#include <Constants.h>
#include <PacketCodes.h>

// libcomp Includes
#include <Log.h>
#include <Randomizer.h>

// Standard C++11 Includes
#include <algorithm>

// object Includes
#include <Account.h>
#include <ChannelLogin.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <EventCounter.h>
#include <InstanceAccess.h>
#include <Match.h>
#include <MatchEntry.h>
#include <PentalphaEntry.h>
#include <PentalphaMatch.h>
#include <PvPMatch.h>
#include <StatusEffect.h>
#include <UBResult.h>
#include <UBTournament.h>
#include <WorldConfig.h>
#include <WorldSharedConfig.h>

// world Includes
#include "AccountManager.h"
#include "CharacterManager.h"
#include "WorldServer.h"

using namespace world;

WorldSyncManager::WorldSyncManager(const std::weak_ptr<WorldServer>& server)
    : libcomp::DataSyncManager(
          to_underlying(InternalPacketCode_t::PACKET_DATA_SYNC)),
      mNextMatchID(0),
      mServer(server) {
  mPvPReadyTimes[0] = {{0, 0}};
  mPvPReadyTimes[1] = {{0, 0}};
  mUBRecalcMin = {{0, 0, 0}};
}

WorldSyncManager::~WorldSyncManager() {}

bool WorldSyncManager::Initialize() {
  auto server = mServer.lock();
  auto lobbyDB = server->GetLobbyDatabase();
  auto worldDB = server->GetWorldDatabase();

  // Build the configs
  auto cfg = std::make_shared<ObjectConfig>("SearchEntry", true);
  cfg->BuildHandler = &DataSyncManager::New<objects::SearchEntry>;
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::SearchEntry>;

  mRegisteredTypes["SearchEntry"] = cfg;

  cfg = std::make_shared<ObjectConfig>("Account", false, lobbyDB);
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::Account>;

  mRegisteredTypes["Account"] = cfg;

  cfg = std::make_shared<ObjectConfig>("Character", true, worldDB);
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::Character>;

  mRegisteredTypes["Character"] = cfg;

  cfg = std::make_shared<ObjectConfig>("CharacterLogin", true);
  cfg->BuildHandler = &DataSyncManager::New<objects::CharacterLogin>;
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::CharacterLogin>;

  mRegisteredTypes["CharacterLogin"] = cfg;

  cfg = std::make_shared<ObjectConfig>("CharacterProgress", true, worldDB);
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::CharacterProgress>;

  mRegisteredTypes["CharacterProgress"] = cfg;

  cfg = std::make_shared<ObjectConfig>("EventCounter", true, worldDB);
  cfg->SyncCompleteHandler =
      &DataSyncManager::SyncComplete<WorldSyncManager, objects::EventCounter>;

  mRegisteredTypes["EventCounter"] = cfg;

  cfg = std::make_shared<ObjectConfig>("InstanceAccess", true);
  cfg->BuildHandler = &DataSyncManager::New<objects::InstanceAccess>;
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::InstanceAccess>;

  mRegisteredTypes["InstanceAccess"] = cfg;

  cfg = std::make_shared<ObjectConfig>("Match", true);
  cfg->BuildHandler = &DataSyncManager::New<objects::Match>;
  cfg->SyncCompleteHandler =
      &DataSyncManager::SyncComplete<WorldSyncManager, objects::Match>;

  mRegisteredTypes["Match"] = cfg;

  cfg = std::make_shared<ObjectConfig>("MatchEntry", true);
  cfg->BuildHandler = &DataSyncManager::New<objects::MatchEntry>;
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::MatchEntry>;
  cfg->SyncCompleteHandler =
      &DataSyncManager::SyncComplete<WorldSyncManager, objects::MatchEntry>;

  mRegisteredTypes["MatchEntry"] = cfg;

  cfg = std::make_shared<ObjectConfig>("PentalphaEntry", true, worldDB);
  cfg->SyncCompleteHandler =
      &DataSyncManager::SyncComplete<WorldSyncManager, objects::PentalphaEntry>;

  mRegisteredTypes["PentalphaEntry"] = cfg;

  cfg = std::make_shared<ObjectConfig>("PentalphaMatch", true, worldDB);
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::PentalphaMatch>;
  cfg->SyncCompleteHandler =
      &DataSyncManager::SyncComplete<WorldSyncManager, objects::PentalphaMatch>;

  mRegisteredTypes["PentalphaMatch"] = cfg;

  cfg = std::make_shared<ObjectConfig>("PvPMatch", true);
  cfg->BuildHandler = &DataSyncManager::New<objects::PvPMatch>;
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::PvPMatch>;
  cfg->SyncCompleteHandler =
      &DataSyncManager::SyncComplete<WorldSyncManager, objects::PvPMatch>;

  mRegisteredTypes["PvPMatch"] = cfg;

  cfg = std::make_shared<ObjectConfig>("StatusEffect", false, worldDB);
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::StatusEffect>;

  mRegisteredTypes["StatusEffect"] = cfg;

  cfg = std::make_shared<ObjectConfig>("UBResult", true, worldDB);
  cfg->SyncCompleteHandler =
      &DataSyncManager::SyncComplete<WorldSyncManager, objects::UBResult>;

  mRegisteredTypes["UBResult"] = cfg;

  cfg = std::make_shared<ObjectConfig>("UBTournament", true, worldDB);
  cfg->UpdateHandler =
      &DataSyncManager::Update<WorldSyncManager, objects::UBTournament>;
  cfg->SyncCompleteHandler =
      &DataSyncManager::SyncComplete<WorldSyncManager, objects::UBTournament>;

  mRegisteredTypes["UBTournament"] = cfg;

  bool sync = false;

  // Get the current Pentalpha match if one exists
  std::list<std::shared_ptr<objects::PentalphaMatch>> activeMatches;
  for (auto match :
       libcomp::PersistentObject::LoadAll<objects::PentalphaMatch>(worldDB)) {
    if (!match->GetEndTime()) {
      activeMatches.push_back(match);
    }
  }

  if (activeMatches.size() > 1) {
    LogDataSyncManagerWarningMsg(
        "Multiple active Pentalpha matches found. Ending all but the newest "
        "entry.\n");

    activeMatches.sort([](const std::shared_ptr<objects::PentalphaMatch>& a,
                          const std::shared_ptr<objects::PentalphaMatch>& b) {
      return a->GetStartTime() < b->GetStartTime();
    });

    mPentalphaMatch = activeMatches.back();
    activeMatches.pop_back();

    for (auto match : activeMatches) {
      if (!EndMatch(match)) {
        return false;
      }
    }

    sync = true;
  } else if (activeMatches.size() > 0) {
    mPentalphaMatch = activeMatches.front();
  }

  // Get the current UB tournament if one exists
  std::list<std::shared_ptr<objects::UBTournament>> activeTournaments;
  for (auto match :
       libcomp::PersistentObject::LoadAll<objects::UBTournament>(worldDB)) {
    if (!match->GetEndTime()) {
      activeTournaments.push_back(match);
    }
  }

  if (activeTournaments.size() > 1) {
    LogDataSyncManagerWarningMsg(
        "Multiple active Ultimate Battle tournaments found. Ending all but the "
        "newest entry.\n");

    activeTournaments.sort([](const std::shared_ptr<objects::UBTournament>& a,
                              const std::shared_ptr<objects::UBTournament>& b) {
      return a->GetStartTime() < b->GetStartTime();
    });

    mUBTournament = activeTournaments.back();
    activeTournaments.pop_back();

    for (auto tournament : activeTournaments) {
      if (!EndTournament(tournament)) {
        return false;
      }
    }

    sync = true;
  } else if (activeTournaments.size() > 0) {
    mUBTournament = activeTournaments.front();
  }

  sync |= RecalculateUBRankings();

  if (sync) {
    SyncOutgoing();
  }

  return true;
}

namespace world {
template <>
int8_t WorldSyncManager::Update<objects::Account>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)isRemove;

  auto entry = std::dynamic_pointer_cast<objects::Account>(obj);

  libcomp::Packet p;

  if (source == "lobby") {
    // Send to the channel where the account is logged in
    auto server = mServer.lock();
    auto accountManager = server->GetAccountManager();

    auto accountLogin = accountManager->GetUserLogin(entry->GetUsername());
    auto cLogin = accountLogin ? accountLogin->GetCharacterLogin() : nullptr;
    int8_t channelID = cLogin ? cLogin->GetChannelID() : -1;

    if (channelID >= 0) {
      auto channel = server->GetChannelConnectionByID(channelID);
      if (channel) {
        LogDataSyncManagerDebug([channelID, entry]() {
          return libcomp::String(
                     "Relaying lobby Account update to channel %1: %2\n")
              .Arg(channelID)
              .Arg(entry->GetUUID().ToString());
        });

        WriteOutgoingRecord(p, true, "Account", entry);
        channel->SendPacket(p);
      }
    }
  } else {
    // Send to the lobby
    LogDataSyncManagerDebug([entry]() {
      return libcomp::String("Relaying channel Account update to lobby: %1\n")
          .Arg(entry->GetUUID().ToString());
    });

    WriteOutgoingRecord(p, true, "Account", entry);
    mServer.lock()->GetLobbyConnection()->SendPacket(p);
  }

  return SYNC_HANDLED;
}

template <>
int8_t WorldSyncManager::Update<objects::Character>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;

  auto entry = std::dynamic_pointer_cast<objects::Character>(obj);

  if (isRemove) {
    // Nothing special to do if not removing
    if (source == "lobby") {
      // Delete is valid, queue the delete
      auto server = mServer.lock();
      server->QueueWork(
          [](std::shared_ptr<WorldServer> pServer,
             std::shared_ptr<objects::Character> character) {
            pServer->GetAccountManager()->DeleteCharacter(character);
          },
          server, entry);
    } else {
      LogDataSyncManagerError([&]() {
        return libcomp::String(
                   "Channel requested a character delete which will be "
                   "ignored: %1\n")
            .Arg(source);
      });
    }
  }

  return SYNC_HANDLED;
}

template <>
int8_t WorldSyncManager::Update<objects::CharacterLogin>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)obj;
  (void)isRemove;
  (void)source;

  // Let all changes through
  return SYNC_UPDATED;
}

template <>
int8_t WorldSyncManager::Update<objects::CharacterProgress>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)isRemove;

  auto entry = std::dynamic_pointer_cast<objects::CharacterProgress>(obj);

  if (source == "lobby") {
    // Send to the channel where the character is logged in, no need to
    // sync if they are not
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto character = std::dynamic_pointer_cast<objects::Character>(
        libcomp::PersistentObject::GetObjectByUUID(entry->GetCharacter()));

    auto cLogin =
        character ? characterManager->GetCharacterLogin(character->GetName())
                  : nullptr;
    int8_t channelID = cLogin ? cLogin->GetChannelID() : -1;

    if (channelID >= 0) {
      auto channel = server->GetChannelConnectionByID(channelID);
      if (channel) {
        libcomp::Packet p;
        WriteOutgoingRecord(p, true, "CharacterProgress", entry);
        channel->SendPacket(p);
      }
    }
  }

  return SYNC_HANDLED;
}

template <>
int8_t WorldSyncManager::Update<objects::InstanceAccess>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)source;

  auto access = std::dynamic_pointer_cast<objects::InstanceAccess>(obj);

  if (!isRemove) {
    if (access->GetInstanceID()) {
      // Could be an access update or getting the ID, forward to all
      mInstanceAccess[access->GetChannelID()][access->GetInstanceID()] = access;

      return SYNC_UPDATED;
    } else {
      // Send to the correct channel to set the instance ID but do not
      // store it
      auto server = mServer.lock();
      auto channel =
          server->GetChannelConnectionByID((int8_t)access->GetChannelID());
      if (channel) {
        libcomp::Packet p;
        WriteOutgoingRecord(p, false, "InstanceAccess", access);
        channel->SendPacket(p);
      }

      return SYNC_HANDLED;
    }
  } else if (access->GetInstanceID()) {
    // Drop it from the cache
    mInstanceAccess[access->GetChannelID()].erase(access->GetInstanceID());
    if (mInstanceAccess[access->GetChannelID()].size() == 0) {
      mInstanceAccess.erase(access->GetChannelID());
    }

    // Remove any relogins associated to the instance
    std::set<int32_t> worldCIDs;
    for (auto& pair : mRelogins) {
      if (pair.second.first == access->GetInstanceID()) {
        worldCIDs.insert(pair.first);
      }
    }

    for (int32_t worldCID : worldCIDs) {
      worldCIDs.erase(worldCID);
    }

    // Forward to all
    return SYNC_UPDATED;
  }

  // Otherwise just ignore
  return SYNC_HANDLED;
}

template <>
void WorldSyncManager::SyncComplete<objects::Match>(
    const libcomp::String& type,
    const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
    const libcomp::String& source) {
  (void)type;
  (void)source;

  for (auto& objPair : objs) {
    auto match = std::dynamic_pointer_cast<objects::Match>(objPair.first);

    if (!objPair.second) {
      if (match->GetType() == objects::Match::Type_t::DIASPORA) {
        // Disband all related teams and notify the players that they
        // are ready to be moved
        auto characterManager = mServer.lock()->GetCharacterManager();

        std::set<int32_t> teamIDs;
        for (int32_t cid : match->GetMemberIDs()) {
          auto cLogin = characterManager->GetCharacterLogin(cid);
          if (cLogin) {
            teamIDs.insert(cLogin->GetTeamID());
          }
        }

        teamIDs.erase(0);

        for (int32_t teamID : teamIDs) {
          characterManager->TeamDisband(teamID, 0, true);
        }
      }
    }
  }
}

template <>
int8_t WorldSyncManager::Update<objects::MatchEntry>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)source;

  auto entry = std::dynamic_pointer_cast<objects::MatchEntry>(obj);

  if (isRemove) {
    mMatchEntries.erase(entry->GetWorldCID());
  } else {
    auto cLogin = mServer.lock()->GetCharacterManager()->GetCharacterLogin(
        entry->GetWorldCID());
    if (!cLogin || cLogin->GetChannelID() < 0) {
      // Not logged in, do not register
      return SYNC_FAILED;
    }

    if (!source.IsEmpty()) {
      // Update came from other server
      entry->SetEntryTime((uint32_t)std::time(0));
    }

    mMatchEntries[entry->GetWorldCID()] = entry;
  }

  return SYNC_UPDATED;
}

template <>
void WorldSyncManager::SyncComplete<objects::MatchEntry>(
    const libcomp::String& type,
    const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
    const libcomp::String& source) {
  (void)type;
  (void)source;

  bool pvpAdded[2][2] = {{false, false}, {false, false}};
  bool pvpRemoved[2][2] = {{false, false}, {false, false}};
  std::array<std::array<std::set<int32_t>, 2>, 2> cids;

  for (auto& pair : objs) {
    auto entry = std::dynamic_pointer_cast<objects::MatchEntry>(pair.first);
    uint8_t isTeamIdx = entry->GetTeamID() != 0 ? 1 : 0;
    uint8_t matchType = (uint8_t)entry->GetMatchType();
    if (matchType < 2) {
      if (pair.second) {
        pvpRemoved[isTeamIdx][matchType] = true;
      } else {
        pvpAdded[isTeamIdx][matchType] = true;
      }

      cids[isTeamIdx][matchType].insert(entry->GetWorldCID());
    }
  }

  for (uint8_t i = 0; i < 2; i++) {
    for (uint8_t k = 0; k < 2; k++) {
      // If match is ready and entrants were removed or match is not
      // ready and entrants were added, check if an update needs to
      // happen
      bool checkStop = (mPvPReadyTimes[i][k] != 0) == pvpRemoved[i][k];
      bool checkStart = (mPvPReadyTimes[i][k] == 0) == pvpAdded[i][k];
      if (checkStop || checkStart) {
        if (i == 0) {
          DeterminePvPMatch(k, cids[i][k]);
        } else {
          DetermineTeamPvPMatch(k, cids[i][k]);
        }
      }
    }
  }
}

template <>
void WorldSyncManager::SyncComplete<objects::EventCounter>(
    const libcomp::String& type,
    const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
    const libcomp::String& source) {
  (void)type;
  (void)source;

  std::set<int32_t> eventTypes;
  for (auto& objPair : objs) {
    auto eCounter =
        std::dynamic_pointer_cast<objects::EventCounter>(objPair.first);

    // Pull both current and expired types to correct the total
    eventTypes.insert(eCounter->GetType());
    eventTypes.insert(eCounter->GetPreExpireType());
  }

  eventTypes.erase(0);

  if (eventTypes.size() > 0) {
    std::set<std::shared_ptr<objects::EventCounter>> updates;
    std::set<std::shared_ptr<objects::EventCounter>> deletes;
    {
      std::lock_guard<std::mutex> lock(mLock);

      auto server = mServer.lock();
      auto worldDB = server->GetWorldDatabase();

      auto dbChanges = libcomp::DatabaseChangeSet::Create();

      for (int32_t t : eventTypes) {
        int32_t sum = 0;
        std::shared_ptr<objects::EventCounter> gCounter;
        std::shared_ptr<objects::EventCounter> sWCounter;
        std::unordered_map<std::string, std::shared_ptr<objects::EventCounter>>
            cCounters;

        for (auto e :
             objects::EventCounter::LoadEventCounterListByType(worldDB, t)) {
          if (e->GetGroupCounter()) {
            if (!gCounter) {
              // Group master found
              gCounter = e;
            } else {
              // Delete duplicate group counter
              dbChanges->Delete(e);
              deletes.insert(e);
            }
          } else if (e->GetStandaloneWorldCounter()) {
            if (!sWCounter) {
              // First copy of standalone world counter found, set it up for update
              sWCounter = e;
              dbChanges->Update(e);
              updates.insert(e);
            } else {
              // Delete duplicate standalone world counter
              dbChanges->Delete(e);
              deletes.insert(e);
            }
          } else if (!e->GetCharacter().IsNull()) {
            // Should be a character counter at this point
            sum += e->GetCounter();

            if (e->GetGroupCounter() || e->GetStandaloneWorldCounter()) {
              // Character counters cannot be group or standalone world counters
              e->SetGroupCounter(false);
              e->SetStandaloneWorldCounter(false);

              dbChanges->Update(e);

              updates.insert(e);
            }

            // Make sure there is only one per character
            std::string uid = e->GetCharacter().ToString();
            auto existing = cCounters[uid];
            if (existing) {
              // Keep whichever one was created first
              if (existing->GetTimestamp() > e->GetTimestamp()) {
                e->SetCounter(e->GetCounter() + existing->GetCounter());
                cCounters[uid] = e;

                dbChanges->Update(e);
                dbChanges->Delete(existing);

                updates.insert(e);
                deletes.insert(existing);
              } else {
                existing->SetCounter(e->GetCounter() + existing->GetCounter());

                dbChanges->Update(existing);
                dbChanges->Delete(e);

                updates.insert(existing);
                deletes.insert(e);
              }
            } else {
              cCounters[uid] = e;
            }
          } else {
            // Broken counter, remove it
            dbChanges->Delete(e);
            deletes.insert(e);
          }
        }

        if (!gCounter && !sWCounter) {
          // A character counter lacking a world group master counter was found,
          // create group counter
          gCounter =
              libcomp::PersistentObject::New<objects::EventCounter>(true);
          gCounter->SetType(t);
          gCounter->SetCounter(sum);
          gCounter->SetGroupCounter(true);
          gCounter->SetTimestamp((uint32_t)std::time(0));

          dbChanges->Insert(gCounter);

          updates.insert(gCounter);
        } else if (gCounter && gCounter->GetCounter() != sum) {
          // Update group master counter with the sum of all character counters
          gCounter->SetCounter(sum);

          dbChanges->Update(gCounter);

          updates.insert(gCounter);
        } 
      }

      worldDB->ProcessChangeSet(dbChanges);
    }

    for (auto r : updates) {
      UpdateRecord(r, "EventCounter");
    }

    for (auto r : deletes) {
      RemoveRecord(r, "EventCounter");
    }
  }
}

template <>
void WorldSyncManager::SyncComplete<objects::PentalphaEntry>(
    const libcomp::String& type,
    const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
    const libcomp::String& source) {
  (void)type;
  (void)source;

  auto match = mPentalphaMatch;
  if (match) {
    bool recalc = false;
    for (auto& objPair : objs) {
      auto entry =
          std::dynamic_pointer_cast<objects::PentalphaEntry>(objPair.first);
      if (entry->GetMatch() == match->GetUUID()) {
        recalc = true;
        break;
      }
    }

    if (recalc) {
      // Recalculate points for the match
      auto db = mServer.lock()->GetWorldDatabase();
      std::array<int32_t, 5> points = {{0, 0, 0, 0, 0}};
      {
        std::lock_guard<std::mutex> lock(mLock);
        for (auto entry :
             objects::PentalphaEntry::LoadPentalphaEntryListByMatch(
                 db, match->GetUUID())) {
          for (size_t i = 0; i < 5; i++) {
            points[i] = entry->GetPoints(i) + points[i];
          }
        }

        match->SetPoints(points);
      }

      if (match->Update(db)) {
        UpdateRecord(match, "PentalphaMatch");
      }
    }
  }
}

template <>
int8_t WorldSyncManager::Update<objects::PentalphaMatch>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)source;

  auto match = std::dynamic_pointer_cast<objects::PentalphaMatch>(obj);
  if (!mPentalphaMatch ||
      (match->GetPrevious() == mPentalphaMatch->GetUUID() && !isRemove)) {
    mPentalphaMatch = match;
  } else if (isRemove) {
    mPentalphaMatch = nullptr;
  }

  // Let all changes through
  return SYNC_UPDATED;
}

template <>
int8_t WorldSyncManager::Update<objects::StatusEffect>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)isRemove;
  (void)source;

  auto entry = std::dynamic_pointer_cast<objects::StatusEffect>(obj);

  // Send to the channel where the character is logged in, channel init
  // is responsible for connecting if they are not logged in
  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();

  auto cLogin = characterManager->GetCharacterLogin(entry->GetEntity());
  int8_t channelID = cLogin ? cLogin->GetChannelID() : -1;

  if (channelID >= 0) {
    auto channel = server->GetChannelConnectionByID(channelID);
    if (channel) {
      libcomp::Packet p;
      WriteOutgoingRecord(p, true, "StatusEffect", entry);
      channel->SendPacket(p);
    }
  }

  return SYNC_HANDLED;
}

template <>
void WorldSyncManager::SyncComplete<objects::PentalphaMatch>(
    const libcomp::String& type,
    const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
    const libcomp::String& source) {
  (void)type;
  (void)source;

  for (auto& objPair : objs) {
    auto match =
        std::dynamic_pointer_cast<objects::PentalphaMatch>(objPair.first);

    if (!objPair.second && match->GetEndTime() && !source.IsEmpty()) {
      EndMatch(match);
    }
  }
}

template <>
void WorldSyncManager::SyncComplete<objects::UBResult>(
    const libcomp::String& type,
    const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
    const libcomp::String& source) {
  (void)type;
  (void)source;

  bool recalcRank = false;
  bool recalcTournament = false;
  {
    std::lock_guard<std::mutex> lock(mLock);
    for (auto& objPair : objs) {
      auto result = std::dynamic_pointer_cast<objects::UBResult>(objPair.first);
      if (result->GetTournament().IsNull()) {
        if (result->GetPoints() >= mUBRecalcMin[1] ||
            result->GetTopPoints() >= mUBRecalcMin[2] || result->GetRanked()) {
          recalcRank = true;
        }
      } else if (result->GetPoints() >= mUBRecalcMin[0] ||
                 result->GetTournamentRank()) {
        recalcTournament = true;
      }
    }
  }

  if (recalcRank) {
    RecalculateUBRankings();
  }

  if (recalcTournament) {
    auto tournament = mUBTournament;
    if (tournament) {
      RecalculateTournamentRankings(tournament->GetUUID());
    }
  }
}

template <>
int8_t WorldSyncManager::Update<objects::UBTournament>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)obj;
  (void)isRemove;
  (void)source;

  // Let all changes through
  return SYNC_UPDATED;
}

template <>
void WorldSyncManager::SyncComplete<objects::UBTournament>(
    const libcomp::String& type,
    const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
    const libcomp::String& source) {
  (void)type;
  (void)source;

  {
    std::lock_guard<std::mutex> lock(mLock);
    if (mUBTournament && mUBTournament->GetEndTime()) {
      mUBTournament = nullptr;
      mUBRecalcMin = {{0, 0, 0}};
    }
  }

  for (auto& objPair : objs) {
    auto tournament =
        std::dynamic_pointer_cast<objects::UBTournament>(objPair.first);

    bool recalc = false;
    if (!objPair.second && tournament->GetEndTime() && !source.IsEmpty()) {
      EndTournament(tournament);
    } else if (!tournament->GetEndTime()) {
      std::lock_guard<std::mutex> lock(mLock);
      mUBTournament = tournament;
      recalc = true;
    }

    if (recalc) {
      RecalculateTournamentRankings(tournament->GetUUID());
    }
  }
}

template <>
int8_t WorldSyncManager::Update<objects::PvPMatch>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;

  auto match = std::dynamic_pointer_cast<objects::PvPMatch>(obj);
  if (!isRemove && !source.IsEmpty()) {
    if (match->GetID()) {
      // Matches with IDs should not be passing through here from
      // other servers
      return SYNC_FAILED;
    } else if (match->MemberIDsCount() == 0 && match->GetReadyTime() &&
               (match->GetType() == objects::PvPMatch::Type_t::PVP_FATE ||
                match->GetType() == objects::PvPMatch::Type_t::PVP_VALHALLA)) {
      // Primary channel supplied pending match, do not sync back
      size_t idx = (size_t)match->GetType();
      for (size_t i = 0; i < 2; i++) {
        if (mPvPReadyTimes[i][idx] == match->GetReadyTime()) {
          // Copy the match in case team and solo matches somhow
          // managed to get the exact same ready time
          mPvPPendingMatches[i][idx] =
              std::make_shared<objects::PvPMatch>(*match);
        }
      }

      return SYNC_HANDLED;
    }
  }

  return SYNC_UPDATED;
}

template <>
void WorldSyncManager::SyncComplete<objects::PvPMatch>(
    const libcomp::String& type,
    const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
    const libcomp::String& source) {
  (void)type;

  for (auto& objPair : objs) {
    auto match = std::dynamic_pointer_cast<objects::PvPMatch>(objPair.first);

    if (!objPair.second && !source.IsEmpty()) {
      // Channel is requesting a custom match, set it up and let it
      // sync back
      PreparePvPMatch(match);
    }
  }
}

template <>
void WorldSyncManager::Expire<objects::SearchEntry>(int32_t entryID,
                                                    uint32_t expirationTime) {
  std::shared_ptr<objects::SearchEntry> entry;
  {
    std::lock_guard<std::mutex> lock(mLock);
    for (auto e : mSearchEntries) {
      if (e->GetEntryID() == entryID &&
          e->GetExpirationTime() == expirationTime) {
        entry = e;
        break;
      }
    }
  }

  if (entry) {
    SyncRecordRemoval(entry, "SearchEntry");
  }
}

template <>
int8_t WorldSyncManager::Update<objects::SearchEntry>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source) {
  (void)type;
  (void)source;

  auto entry = std::dynamic_pointer_cast<objects::SearchEntry>(obj);

  auto it = mSearchEntries.begin();
  while (it != mSearchEntries.end()) {
    if ((*it)->GetEntryID() == entry->GetEntryID()) {
      auto existing = *it;

      // If the entry is being removed or having its type or source modified
      // update the map count
      if (isRemove || existing->GetSourceCID() != entry->GetSourceCID() ||
          existing->GetType() != entry->GetType()) {
        AdjustSearchEntryCount(existing->GetSourceCID(), existing->GetType(),
                               false);
      }

      it = mSearchEntries.erase(it);
      if (!isRemove) {
        // Replace the existing element and update the count again
        mSearchEntries.insert(it, entry);
        AdjustSearchEntryCount(entry->GetSourceCID(), entry->GetType(), true);
      }

      return SYNC_UPDATED;
    }

    it++;
  }

  if (isRemove) {
    LogDataSyncManagerWarning([&]() {
      return libcomp::String(
                 "No SearchEntry with ID '%1' found for sync removal\n")
          .Arg(entry->GetEntryID());
    });

    return SYNC_FAILED;
  } else {
    int32_t nextEntryID = 1;
    if (mSearchEntries.size() > 0) {
      nextEntryID = mSearchEntries.front()->GetEntryID() + 1;
    }

    entry->SetEntryID(nextEntryID);

    AdjustSearchEntryCount(entry->GetSourceCID(), entry->GetType(), true);

    mSearchEntries.push_front(entry);

    if (entry->GetExpirationTime() != 0) {
      // Set to expire
      uint32_t now = (uint32_t)std::time(0);

      // Default to 10 minutes if the expiration is invalid or passed
      uint32_t exp = now < entry->GetExpirationTime()
                         ? (uint32_t)(entry->GetExpirationTime() - now)
                         : 600;

      mServer.lock()->GetTimerManager()->ScheduleEventIn(
          (int)exp,
          [](WorldSyncManager* pSyncManager, int32_t pEntryID,
             uint32_t pExpireTime) {
            pSyncManager->Expire<objects::SearchEntry>(pEntryID, pExpireTime);
          },
          this, entry->GetEntryID(), entry->GetExpirationTime());
    }

    return SYNC_UPDATED;
  }
}
}  // namespace world

bool WorldSyncManager::RemoveRecord(
    const std::shared_ptr<libcomp::Object>& record,
    const libcomp::String& type) {
  if (DataSyncManager::RemoveRecord(record, type)) {
    bool recalcTeamPvP = false;
    std::list<std::shared_ptr<libcomp::Object>> additionalRemoves;
    if (type == "MatchEntry") {
      // If a team match entry is being removed, remove the rest of
      // the team
      auto entry = std::dynamic_pointer_cast<objects::MatchEntry>(record);
      if (entry->GetTeamID()) {
        std::lock_guard<std::mutex> lock(mLock);
        for (auto& ePair : mMatchEntries) {
          if (ePair.second->GetTeamID() == entry->GetTeamID()) {
            additionalRemoves.push_back(ePair.second);
          }
        }

        recalcTeamPvP = true;
      }
    } else if (type == "SearchEntry") {
      // If the record is a search entry, be sure to also remove all
      // child entries
      std::lock_guard<std::mutex> lock(mLock);

      auto entry = std::dynamic_pointer_cast<objects::SearchEntry>(record);
      for (auto e : mSearchEntries) {
        if (e->GetParentEntryID() == entry->GetEntryID()) {
          additionalRemoves.push_back(e);
        }
      }
    }

    for (auto entry : additionalRemoves) {
      DataSyncManager::RemoveRecord(entry, type);
    }

    if (recalcTeamPvP) {
      auto entry = std::dynamic_pointer_cast<objects::MatchEntry>(record);
      DetermineTeamPvPMatch((uint8_t)entry->GetMatchType());
    }

    return true;
  }

  return false;
}

std::shared_ptr<objects::ChannelLogin> WorldSyncManager::PopRelogin(
    int32_t worldCID) {
  std::lock_guard<std::mutex> lock(mLock);

  std::shared_ptr<objects::ChannelLogin> result;

  auto it = mRelogins.find(worldCID);
  if (it != mRelogins.end()) {
    auto rPair = it->second;
    mRelogins.erase(it);

    // If the instance doesn't exist anymore or access is no longer
    // granted, don't return the relogin
    uint32_t instanceID = rPair.first;
    for (auto& pair : mInstanceAccess) {
      auto it2 = pair.second.find(instanceID);
      if (it2 != pair.second.end() &&
          it2->second->AccessCIDsContains(worldCID)) {
        result = rPair.second;
        break;
      }
    }
  }

  return result;
}

bool WorldSyncManager::PushRelogin(
    const std::shared_ptr<objects::ChannelLogin>& login, uint32_t instanceID) {
  std::shared_ptr<objects::InstanceAccess> access;
  {
    std::lock_guard<std::mutex> lock(mLock);

    int32_t worldCID = login->GetWorldCID();
    for (auto& pair : mInstanceAccess) {
      auto it = pair.second.find(instanceID);
      if (it != pair.second.end()) {
        access = it->second;
        access->InsertAccessCIDs(worldCID);
        mRelogins[worldCID] = std::make_pair(instanceID, login);
      }
    }
  }

  if (access) {
    SyncRecordUpdate(access, "InstanceAccess");
    return true;
  }

  return false;
}

std::shared_ptr<objects::MatchEntry> WorldSyncManager::GetMatchEntry(
    int32_t worldCID) {
  std::lock_guard<std::mutex> lock(mLock);
  auto it = mMatchEntries.find(worldCID);
  return it != mMatchEntries.end() ? it->second : nullptr;
}

bool WorldSyncManager::CleanUpCharacterLogin(int32_t worldCID,
                                             bool flushOutgoing) {
  bool result = false;

  auto cLogin =
      mServer.lock()->GetCharacterManager()->GetCharacterLogin(worldCID);
  if (cLogin) {
    result |= RemoveRecord(cLogin, "CharacterLogin");
  }

  std::list<std::shared_ptr<libcomp::Object>> searchEntries;
  std::shared_ptr<objects::MatchEntry> matchEntry;
  std::list<std::shared_ptr<objects::InstanceAccess>> instAccess;
  {
    std::lock_guard<std::mutex> lock(mLock);
    if (mSearchEntryCounts.find(worldCID) != mSearchEntryCounts.end()) {
      // Drop all non-clan search entries
      for (auto entry : mSearchEntries) {
        if (entry->GetSourceCID() == worldCID &&
            entry->GetType() != objects::SearchEntry::Type_t::CLAN_JOIN &&
            entry->GetType() != objects::SearchEntry::Type_t::CLAN_RECRUIT) {
          entry->SetLastAction(
              objects::SearchEntry::LastAction_t::REMOVE_LOGOFF);
          searchEntries.push_back(entry);
        }
      }
    }

    auto it = mMatchEntries.find(worldCID);
    if (it != mMatchEntries.end()) {
      matchEntry = it->second;
    }

    // Get the relogin instance ID so access is not removed from it
    uint32_t reloginInstanceID = 0;
    auto rIter = mRelogins.find(worldCID);
    if (rIter != mRelogins.end()) {
      reloginInstanceID = rIter->second.first;
    }

    for (auto& pair : mInstanceAccess) {
      for (auto& subPair : pair.second) {
        if (subPair.second->AccessCIDsContains(worldCID) &&
            subPair.first != reloginInstanceID) {
          instAccess.push_back(subPair.second);
        }
      }
    }
  }

  // Remove search entries
  if (searchEntries.size() > 0) {
    for (auto entry : searchEntries) {
      result |= RemoveRecord(entry, "SearchEntry");
    }
  }

  // Remove match entry (and recalc)
  if (matchEntry) {
    result |= RemoveRecord(matchEntry, "MatchEntry");
    if (!matchEntry->GetTeamID()) {
      result |= DeterminePvPMatch((uint8_t)matchEntry->GetMatchType());
    } else {
      result |= DetermineTeamPvPMatch((uint8_t)matchEntry->GetMatchType());
    }
  }

  // Remove from instance access and sync
  for (auto access : instAccess) {
    access->RemoveAccessCIDs(worldCID);

    // Only remove access CIDs here, let the channel handle removing
    // the instance access itself in case of access time-outs
    result |= UpdateRecord(access, "InstanceAccess");
  }

  if (result && flushOutgoing) {
    SyncOutgoing();
  }

  return result;
}

void WorldSyncManager::SyncExistingChannelRecords(
    const std::shared_ptr<libcomp::InternalConnection>& connection) {
  LogDataSyncManagerDebug([connection]() {
    return libcomp::String(
               "Synchronizing existing records with new channel connection: "
               "%1\n")
        .Arg(connection->GetName());
  });

  std::lock_guard<std::mutex> lock(mLock);

  std::set<std::shared_ptr<libcomp::Object>> records;
  std::set<std::shared_ptr<libcomp::Object>> blank;
  for (auto entry : mSearchEntries) {
    records.insert(entry);
  }

  QueueOutgoing("SearchEntry", connection, records, blank);

  records.clear();
  for (auto cLogin :
       mServer.lock()->GetCharacterManager()->GetActiveCharacters()) {
    records.insert(cLogin);
  }

  QueueOutgoing("CharacterLogin", connection, records, blank);

  records.clear();
  for (auto& pair : mInstanceAccess) {
    for (auto& subPair : pair.second) {
      records.insert(subPair.second);
    }
  }

  QueueOutgoing("InstanceAccess", connection, records, blank);

  records.clear();
  for (auto& pair : mMatchEntries) {
    records.insert(pair.second);
  }

  QueueOutgoing("MatchEntry", connection, records, blank);

  if (mPentalphaMatch) {
    records.clear();
    records.insert(mPentalphaMatch);

    QueueOutgoing("PentalphaMatch", connection, records, blank);
  }

  if (mUBTournament) {
    records.clear();
    records.insert(mUBTournament);

    QueueOutgoing("UBTournament", connection, records, blank);
  }

  connection->FlushOutgoing();
}

void WorldSyncManager::StartPvPMatch(uint32_t time, uint8_t type) {
  bool start = false;
  uint32_t queuedTime = 0;
  std::shared_ptr<objects::PvPMatch> match;
  std::list<std::shared_ptr<objects::MatchEntry>> entries;
  {
    std::lock_guard<std::mutex> lock(mLock);
    if (type < 2) {
      queuedTime = mPvPReadyTimes[0][type];
      match = mPvPPendingMatches[0][type];
      if (queuedTime == time) {
        mPvPReadyTimes[0][type] = 0;
        mPvPPendingMatches[0][type] = nullptr;
        if (match != nullptr) {
          entries = GetMatchEntryTeams(type)[0];
          start = true;
        }
      }
    }
  }

  bool recheck = !queuedTime;
  if (start) {
    LogDataSyncManagerDebug([type]() {
      return libcomp::String("Starting PvP match type %1.\n").Arg(type);
    });

    // Check if the required number of entries exist. If they do, form
    // teams and create match
    size_t minCount = type == 0 ? PVP_FATE_PLAYER_MIN : PVP_VALHALLA_PLAYER_MIN;
    size_t maxCount = type == 0 ? PVP_FATE_PLAYER_MAX : PVP_VALHALLA_PLAYER_MAX;

    uint8_t ghost = std::dynamic_pointer_cast<objects::WorldConfig>(
                        mServer.lock()->GetConfig())
                        ->GetWorldSharedConfig()
                        ->GetPvPGhosts((size_t)type);
    if (entries.size() >= 2 && (size_t)(entries.size() + ghost) >= minCount) {
      // First in, first out
      entries.sort([](const std::shared_ptr<objects::MatchEntry>& a,
                      const std::shared_ptr<objects::MatchEntry>& b) {
        return a->GetEntryTime() < b->GetEntryTime();
      });

      size_t teamCount = entries.size();
      if (teamCount > maxCount) {
        teamCount = maxCount;
      }

      // If team sizes does not match, drop the last one
      if (teamCount % 2 == 1) {
        teamCount = (size_t)(teamCount - 1);
      }

      PreparePvPMatch(match);

      // Split the teams and set the match ID
      size_t left = teamCount;

      std::set<int32_t> cids;
      while (left) {
        auto entry = entries.front();
        entries.pop_front();

        entry->SetMatchID(match->GetID());
        entry->SetReadyTime(match->GetReadyTime());

        // World is now done with the record
        RemoveRecord(entry, "MatchEntry");

        cids.insert(entry->GetWorldCID());

        left = (size_t)(left - 1);
      }

      for (size_t i = 0; i < teamCount; i++) {
        int32_t cid = libcomp::Randomizer::GetEntry(cids);
        cids.erase(cid);

        match->InsertMemberIDs(cid);
        if (i % 2 == 0) {
          match->AppendBlueMemberIDs(cid);
        } else {
          match->AppendRedMemberIDs(cid);
        }
      }

      SyncRecordUpdate(match, "PvPMatch");
    }

    // Always recheck
    recheck = true;
  }

  if (recheck) {
    if (DeterminePvPMatch(type)) {
      SyncOutgoing();
    }
  }
}

void WorldSyncManager::StartTeamPvPMatch(uint32_t time, uint8_t type) {
  bool start = false;
  uint32_t queuedTime = 0;
  std::shared_ptr<objects::PvPMatch> match;
  std::unordered_map<int32_t, std::list<std::shared_ptr<objects::MatchEntry>>>
      teamEntries;
  {
    std::lock_guard<std::mutex> lock(mLock);
    if (type < 2) {
      queuedTime = mPvPReadyTimes[1][type];
      match = mPvPPendingMatches[1][type];
      if (queuedTime == time) {
        mPvPReadyTimes[1][type] = 0;
        mPvPPendingMatches[1][type] = nullptr;
        if (match != nullptr) {
          teamEntries = GetMatchEntryTeams(type);
          start = true;
        }
      }
    }
  }

  bool recheck = !queuedTime;
  if (start) {
    LogDataSyncManagerDebug([type]() {
      return libcomp::String("Starting team PvP match type %1.\n").Arg(type);
    });

    // Check if two teams with the required number of entries exist.
    // If they do, form teams and create match
    size_t minCount = (size_t)(
        (type == 0 ? PVP_FATE_PLAYER_MIN : PVP_VALHALLA_PLAYER_MIN) / 2);

    uint8_t ghost = std::dynamic_pointer_cast<objects::WorldConfig>(
                        mServer.lock()->GetConfig())
                        ->GetWorldSharedConfig()
                        ->GetPvPGhosts((size_t)type);
    uint8_t gAdjust = (uint8_t)((ghost + 1) / 2);

    teamEntries.erase(0);  // Drop solo entries

    std::list<std::list<std::shared_ptr<objects::MatchEntry>>> readyTeams;
    for (auto& pair : teamEntries) {
      auto team = pair.second;
      if ((size_t)(team.size() + gAdjust) >= minCount) {
        // Sort by first registered
        team.sort([](const std::shared_ptr<objects::MatchEntry>& a,
                     const std::shared_ptr<objects::MatchEntry>& b) {
          return a->GetEntryTime() < b->GetEntryTime();
        });

        readyTeams.push_back(team);
      }
    }

    if (readyTeams.size() >= 2) {
      // Determine first in, first out based on team members and
      // split into blue and read teams
      readyTeams.sort(
          [](const std::list<std::shared_ptr<objects::MatchEntry>>& a,
             const std::list<std::shared_ptr<objects::MatchEntry>>& b) {
            return a.front()->GetEntryTime() < b.front()->GetEntryTime();
          });

      std::array<std::list<std::shared_ptr<objects::MatchEntry>>, 2> teams;
      teams[0] = readyTeams.front();
      readyTeams.pop_front();

      teams[1] = readyTeams.front();

      PreparePvPMatch(match);

      std::set<int32_t> cids;
      for (size_t i = 0; i < 2; i++) {
        for (auto entry : teams[i]) {
          entry->SetMatchID(match->GetID());
          entry->SetReadyTime(match->GetReadyTime());

          // World is now done with the record
          RemoveRecord(entry, "MatchEntry");

          cids.insert(entry->GetWorldCID());

          match->InsertMemberIDs(entry->GetWorldCID());
          if (i % 2 == 0) {
            match->AppendBlueMemberIDs(entry->GetWorldCID());
          } else {
            match->AppendRedMemberIDs(entry->GetWorldCID());
          }
        }
      }

      SyncRecordUpdate(match, "PvPMatch");
    }

    // Always recheck
    recheck = true;
  }

  if (recheck) {
    if (DetermineTeamPvPMatch(type)) {
      SyncOutgoing();
    }
  }
}

void WorldSyncManager::AdjustSearchEntryCount(int32_t sourceCID,
                                              objects::SearchEntry::Type_t type,
                                              bool increment) {
  if (increment) {
    if (mSearchEntryCounts[sourceCID].find(type) ==
        mSearchEntryCounts[sourceCID].end()) {
      mSearchEntryCounts[sourceCID][type] = 1;
    } else {
      mSearchEntryCounts[sourceCID][type]++;
    }
  } else {
    auto it = mSearchEntryCounts[sourceCID].find(type);
    if (it != mSearchEntryCounts[sourceCID].end() && it->second != 0) {
      if (it->second == 1) {
        mSearchEntryCounts[sourceCID].erase(type);
        if (mSearchEntryCounts[sourceCID].size() == 0) {
          mSearchEntryCounts.erase(sourceCID);
        }
      } else {
        it->second--;
      }
    }
  }
}

bool WorldSyncManager::DeterminePvPMatch(uint8_t type,
                                         std::set<int32_t> updated) {
  if (type >= 2) {
    // Not a standard PvP type
    return false;
  }

  LogDataSyncManagerDebug([type]() {
    return libcomp::String("Recalculating queue for PvP match type %1.\n")
        .Arg(type);
  });

  uint32_t time = 0;
  std::list<std::shared_ptr<objects::MatchEntry>> soloEntries;
  {
    std::lock_guard<std::mutex> lock(mLock);

    time = mPvPReadyTimes[0][type];

    bool checkStop = time != 0;
    bool checkStart = time == 0;

    size_t minCount = type == 0 ? PVP_FATE_PLAYER_MIN : PVP_VALHALLA_PLAYER_MIN;

    soloEntries = GetMatchEntryTeams(type)[0];

    std::list<std::shared_ptr<objects::MatchEntry>> readyPrevious;
    for (auto entry : soloEntries) {
      if (entry->GetReadyTime() &&
          updated.find(entry->GetWorldCID()) == updated.end()) {
        readyPrevious.push_back(entry);
      }
    }

    auto server = mServer.lock();
    auto config =
        std::dynamic_pointer_cast<objects::WorldConfig>(server->GetConfig())
            ->GetWorldSharedConfig();

    int32_t qWait = (int32_t)config->GetPvPQueueWait();

    uint8_t ghost = config->GetPvPGhosts((size_t)type);
    if (checkStop && (readyPrevious.size() < 2 ||
                      (size_t)(readyPrevious.size() + ghost) < minCount)) {
      // Ready count dropped below min amount, reset the ready time
      mPvPReadyTimes[0][type] = time = 0;
      mPvPPendingMatches[0][type] = nullptr;

      // Allow restart if new entry count meets minimum
      checkStart = true;
    }

    if (checkStart && (size_t)(soloEntries.size() + ghost) >= minCount) {
      // Entry count raised above min amount, queue match start
      mPvPReadyTimes[0][type] = time =
          (uint32_t)((int32_t)std::time(0) + qWait);

      server->GetTimerManager()->ScheduleEventIn(
          (int)qWait,
          [](WorldSyncManager* pSyncManager, uint32_t pTime, uint8_t pType) {
            pSyncManager->StartPvPMatch(pTime, pType);
          },
          this, time, type);
    }
  }

  // Sync all current ready times
  bool queued = false;
  for (auto entry : soloEntries) {
    if (entry->GetReadyTime() != time) {
      entry->SetReadyTime(time);
      queued |= UpdateRecord(entry, "MatchEntry");
    }
  }

  return queued;
}

bool WorldSyncManager::DetermineTeamPvPMatch(uint8_t type,
                                             std::set<int32_t> updated) {
  if (type >= 2) {
    // Not a standard PvP type
    return false;
  }

  LogDataSyncManagerDebug([type]() {
    return libcomp::String("Recalculating queue for team PvP match type %1.\n")
        .Arg(type);
  });

  uint32_t time = 0;
  std::unordered_map<int32_t, std::list<std::shared_ptr<objects::MatchEntry>>>
      teamEntries;
  std::list<std::shared_ptr<objects::MatchEntry>> drop;
  {
    std::lock_guard<std::mutex> lock(mLock);

    time = mPvPReadyTimes[1][type];

    bool checkStop = time != 0;
    bool checkStart = time == 0;

    size_t minCount = (size_t)(
        (type == 0 ? PVP_FATE_PLAYER_MIN : PVP_VALHALLA_PLAYER_MIN) / 2);
    size_t maxCount = (size_t)(
        (type == 0 ? PVP_FATE_PLAYER_MAX : PVP_VALHALLA_PLAYER_MAX) / 2);

    auto server = mServer.lock();
    auto config =
        std::dynamic_pointer_cast<objects::WorldConfig>(server->GetConfig())
            ->GetWorldSharedConfig();

    int32_t qWait = (int32_t)config->GetPvPQueueWait();

    uint8_t ghost = config->GetPvPGhosts((size_t)type);
    uint8_t gAdjust = (uint8_t)((ghost + 1) / 2);

    teamEntries = GetMatchEntryTeams(type);
    teamEntries.erase(0);  // Drop solo entries

    std::set<int32_t> readyTeams;
    std::set<int32_t> previousReadyTeams;
    for (auto& pair : teamEntries) {
      if ((size_t)(pair.second.size() + gAdjust) >= minCount &&
          pair.second.size() <= maxCount) {
        readyTeams.insert(pair.first);
        previousReadyTeams.insert(pair.first);

        // If any entry was updated, remove the team from the ready set
        for (auto entry : pair.second) {
          if (updated.find(entry->GetWorldCID()) == updated.end()) {
            previousReadyTeams.erase(pair.first);
            break;
          }
        }
      } else {
        // Drop all entries as the team is not a valid size
        for (auto entry : pair.second) {
          drop.push_back(entry);
        }

        pair.second.clear();
      }
    }

    if (checkStop && previousReadyTeams.size() < 2) {
      // Ready team count dropped below min amount, reset the ready time
      mPvPReadyTimes[1][type] = time = 0;
      mPvPPendingMatches[1][type] = nullptr;

      // Allow restart if new entry count meets minimum
      checkStart = true;
    }

    if (checkStart && readyTeams.size() >= 2) {
      // Ready team count raised above min amount, queue match start
      mPvPReadyTimes[1][type] = time =
          (uint32_t)((int32_t)std::time(0) + qWait);

      server->GetTimerManager()->ScheduleEventIn(
          (int)qWait,
          [](WorldSyncManager* pSyncManager, uint32_t pTime, uint8_t pType) {
            pSyncManager->StartTeamPvPMatch(pTime, pType);
          },
          this, time, type);
    }
  }

  bool queued = false;
  for (auto entry : drop) {
    queued |= RemoveRecord(entry, "MatchEntry");
  }

  // Sync all current ready times
  for (auto& pair : teamEntries) {
    for (auto entry : pair.second) {
      if (entry->GetReadyTime() != time) {
        entry->SetReadyTime(time);
        queued |= UpdateRecord(entry, "MatchEntry");
      }
    }
  }

  return queued;
}

bool WorldSyncManager::PreparePvPMatch(
    std::shared_ptr<objects::PvPMatch> match) {
  if (match->GetID()) {
    // Already prepared
    return false;
  }

  uint32_t matchID = 0;
  {
    std::lock_guard<std::mutex> lock(mLock);
    matchID = ++mNextMatchID;
  }

  uint32_t matchReady = (uint32_t)((uint32_t)std::time(0) + 30);
  match->SetID(matchID);
  match->SetReadyTime(matchReady);

  return true;
}

std::unordered_map<int32_t, std::list<std::shared_ptr<objects::MatchEntry>>>
WorldSyncManager::GetMatchEntryTeams(uint8_t type) {
  std::unordered_map<int32_t, std::list<std::shared_ptr<objects::MatchEntry>>>
      entryTeams;
  for (auto& pair : mMatchEntries) {
    auto entry = pair.second;
    if ((uint8_t)entry->GetMatchType() == type && !entry->GetMatchID()) {
      entryTeams[entry->GetTeamID()].push_back(entry);
    }
  }

  return entryTeams;
}

bool WorldSyncManager::EndMatch(
    const std::shared_ptr<objects::PentalphaMatch>& match) {
  LogDataSyncManagerDebug([match]() {
    return libcomp::String("Ending Pentalpha match: %1\n")
        .Arg(match->GetUUID().ToString());
  });

  auto server = mServer.lock();
  auto db = server->GetWorldDatabase();

  if (!match->GetEndTime()) {
    match->SetEndTime((uint32_t)std::time(0));
  }

  // Rank teams, from highest points earned to lowest
  std::vector<std::pair<size_t, int32_t>> ranks;
  for (size_t i = 0; i < 5; i++) {
    ranks.push_back(std::make_pair(i, match->GetPoints(i)));
  }

  std::sort(
      ranks.begin(), ranks.end(),
      [](const std::pair<size_t, int32_t>& a,
         const std::pair<size_t, int32_t>& b) { return a.second > b.second; });

  int32_t pointsFirst = ranks[0].second;
  // While this variable can legitimately be negative, it should be floored to 0
  // for use in calculations
  int32_t pointsSecond = ranks[1].second > 0 ? ranks[1].second : 0;

  uint8_t bestRank = 1;
  for (size_t i = 0; i < 5; i++) {
    match->SetRankings(ranks[i].first, bestRank);
    if (i != 4 && ranks[i + 1].second != ranks[i].second) {
      bestRank++;
    }
  }

  // Calculate base gain from how much 1st place won by
  double cowrieGain =
      pointsFirst > 0
          ? ((double)(pointsFirst - pointsSecond) / (double)pointsFirst)
          : 0.0;

  auto sharedConfig = std::dynamic_pointer_cast<objects::WorldConfig>(
                          mServer.lock()->GetConfig())
                          ->GetWorldSharedConfig();

  bool payoutAll = sharedConfig->GetPentalphaPayoutAll();
  auto payoutRange = sharedConfig->GetPentalphaMinMaxPayout();

  if (payoutRange[1] >= 0 && payoutRange[0] > payoutRange[1]) {
    // Invalid, reset both min and max
    payoutRange[0] = payoutRange[1] = -1;
  }

  // Set minimum and maximum gain
  int32_t min = payoutRange[0] >= 0 ? payoutRange[0] : 100;
  int32_t max = payoutRange[1] >= 0 ? payoutRange[1] : 300;

  auto entries = objects::PentalphaEntry::LoadPentalphaEntryListByMatch(
      db, match->GetUUID());

  int32_t winningBethel = 0;
  for (auto entry : entries) {
    if (match->GetRankings((uint8_t)entry->GetTeam()) == 1) {
      winningBethel += entry->GetBethel();
    }
  }

  auto opChangeset = std::make_shared<libcomp::DBOperationalChangeSet>();

  std::set<std::shared_ptr<libcomp::Object>> progresses;
  for (auto entry : entries) {
    if (!entry->GetActive()) continue;

    uint8_t rank = match->GetRankings((uint8_t)entry->GetTeam());
    if (rank == 1 || payoutAll) {
      // Player was on winning team or all teams being paid
      auto progress =
          objects::CharacterProgress::LoadCharacterProgressByCharacter(
              db, entry->GetCharacter());
      if (!progress) {
        return false;
      }

      auto cowrieGained = cowrieGain;
      if (winningBethel > 0) {
        // Give cowries based on proportion of points the winning team earned
        cowrieGained *= ((double)entry->GetBethel() / (double)winningBethel);

        if (rank != 1) {
          // Reduce by 10% per rank
          cowrieGained *= (1.0 - ((double)(rank - 1) * 0.1));
        }
      }
      // Bound by max or min
      cowrieGained = (double)min + (double)(max - min) * cowrieGained;

      entry->SetCowrie((int32_t)cowrieGained);

      auto expl = std::make_shared<libcomp::DBExplicitUpdate>(progress);
      expl->Add<int32_t>("Cowrie", (int32_t)cowrieGained);
      opChangeset->AddOperation(expl);

      progresses.insert(progress);
    }

    entry->SetActive(false);
    opChangeset->Update(entry);
  }

  if (entries.size() > 0) {
    opChangeset->Update(match);
  } else {
    // No one participated so it can be deleted
    opChangeset->Delete(match);
  }

  if (!db->ProcessChangeSet(opChangeset)) {
    return false;
  }

  if (entries.size() > 0) {
    UpdateRecord(match, "PentalphaMatch");

    for (auto entry : entries) {
      UpdateRecord(entry, "PentalphaEntry");
    }

    for (auto progress : progresses) {
      UpdateRecord(progress, "CharacterProgress");
    }
  } else {
    RemoveRecord(match, "PentalphaMatch");
  }

  return true;
}

bool WorldSyncManager::RecalculateTournamentRankings(
    const libobjgen::UUID& tournamentUID) {
  LogDataSyncManagerDebug([tournamentUID]() {
    return libcomp::String("Recalculating UB Tournament rankings: %1\n")
        .Arg(tournamentUID.ToString());
  });

  auto server = mServer.lock();

  auto results = objects::UBResult::LoadUBResultListByTournament(
      server->GetWorldDatabase(), tournamentUID);

  results.sort([](const std::shared_ptr<objects::UBResult>& a,
                  const std::shared_ptr<objects::UBResult>& b) {
    return a->GetPoints() > b->GetPoints();
  });

  std::list<std::shared_ptr<objects::UBResult>> updated;
  {
    std::lock_guard<std::mutex> lock(mLock);

    mUBRecalcMin[0] = 0;

    size_t idx = 0;
    uint8_t rank = 0;
    int32_t points = -1;
    for (auto result : results) {
      if (rank <= 10 && points != (int32_t)result->GetPoints()) {
        rank = (uint8_t)(rank + 1);
        points = (int32_t)result->GetPoints();
      }

      if (rank <= 10) {
        if (result->GetTournamentRank() != rank) {
          result->SetTournamentRank(rank);
          updated.push_back(result);
        }
      } else if (result->GetTournamentRank()) {
        result->SetTournamentRank(0);
        updated.push_back(result);
      }

      if (idx++ == 10) {
        mUBRecalcMin[0] = (uint32_t)points;
      }
    }
  }

  if (updated.size() > 0) {
    auto dbChanges = libcomp::DatabaseChangeSet::Create();

    for (auto update : updated) {
      dbChanges->Update(update);

      UpdateRecord(update, "UBResult");
    }

    server->GetWorldDatabase()->ProcessChangeSet(dbChanges);
  }

  return results.size() > 0;
}

bool WorldSyncManager::RecalculateUBRankings() {
  auto server = mServer.lock();

  auto results = objects::UBResult::LoadUBResultListByTournament(
      server->GetWorldDatabase(), NULLUUID);

  std::set<std::shared_ptr<objects::UBResult>> updated;
  {
    std::lock_guard<std::mutex> lock(mLock);

    mUBRecalcMin[1] = 0;
    mUBRecalcMin[2] = 0;

    // Calculate all time ranks
    results.sort([](const std::shared_ptr<objects::UBResult>& a,
                    const std::shared_ptr<objects::UBResult>& b) {
      return a->GetPoints() > b->GetPoints();
    });

    size_t idx = 0;
    uint8_t rank = 0;
    int32_t points = -1;
    for (auto result : results) {
      if (rank <= 10 && points != (int32_t)result->GetPoints()) {
        rank = (uint8_t)(rank + 1);
        points = (int32_t)result->GetPoints();
      }

      if (rank <= 10) {
        if (result->GetAllTimeRank() != rank) {
          result->SetAllTimeRank(rank);
          updated.insert(result);
        }
      } else if (result->GetAllTimeRank()) {
        result->SetAllTimeRank(0);
        updated.insert(result);
      }

      if (idx++ == 10) {
        mUBRecalcMin[1] = (uint32_t)points;
      }
    }

    // Calculate top point ranks
    results.sort([](const std::shared_ptr<objects::UBResult>& a,
                    const std::shared_ptr<objects::UBResult>& b) {
      return a->GetTopPoints() > b->GetTopPoints();
    });

    idx = 0;
    rank = 0;
    points = -1;
    for (auto result : results) {
      if (rank <= 10 && points != (int32_t)result->GetTopPoints()) {
        rank = (uint8_t)(rank + 1);
        points = (int32_t)result->GetPoints();
      }

      if (rank <= 10) {
        if (result->GetTopPointRank() != rank) {
          result->SetTopPointRank(rank);
          updated.insert(result);
        }
      } else if (result->GetTopPointRank()) {
        result->SetTopPointRank(0);
        updated.insert(result);
      }

      if (idx++ == 10) {
        mUBRecalcMin[2] = (uint32_t)points;
      }
    }
  }

  if (updated.size() > 0) {
    auto dbChanges = libcomp::DatabaseChangeSet::Create();

    for (auto update : updated) {
      update->SetRanked(update->GetAllTimeRank() || update->GetTopPointRank());

      dbChanges->Update(update);

      UpdateRecord(update, "UBResult");
    }

    server->GetWorldDatabase()->ProcessChangeSet(dbChanges);

    return true;
  }

  return false;
}

bool WorldSyncManager::EndTournament(
    const std::shared_ptr<objects::UBTournament>& tournament) {
  if (!tournament->GetEndTime()) {
    tournament->SetEndTime((uint32_t)std::time(0));

    if (!tournament->Update(mServer.lock()->GetWorldDatabase())) {
      return false;
    }

    LogDataSyncManagerDebug([tournament]() {
      return libcomp::String("UB Tournament has ended: %1\n")
          .Arg(tournament->GetUUID().ToString());
    });
  }

  if (RecalculateTournamentRankings(tournament->GetUUID())) {
    UpdateRecord(tournament, "UBTournament");
  } else {
    RemoveRecord(tournament, "UBTournament");
  }

  return true;
}
