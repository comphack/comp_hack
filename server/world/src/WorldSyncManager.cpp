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

#include "WorldSyncManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>

// Standard C++11 Includes
#include <algorithm>

// object Includes
#include <Account.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <MatchEntry.h>
#include <PvPMatch.h>
#include <WorldConfig.h>
#include <WorldSharedConfig.h>

// world Includes
#include "AccountManager.h"
#include "CharacterManager.h"
#include "WorldServer.h"

using namespace world;

WorldSyncManager::WorldSyncManager(const std::weak_ptr<
    WorldServer>& server) : mNextMatchID(0), mServer(server)
{
    mPvPReadyTimes[0] = { { 0, 0 } };
    mPvPReadyTimes[1] = { { 0, 0 } };
}

WorldSyncManager::~WorldSyncManager()
{
}

bool WorldSyncManager::Initialize()
{
    auto server = mServer.lock();
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();

    // Build the configs
    auto cfg = std::make_shared<ObjectConfig>("SearchEntry", true);
    cfg->BuildHandler = &DataSyncManager::New<objects::SearchEntry>;
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::SearchEntry>;

    mRegisteredTypes["SearchEntry"] = cfg;

    cfg = std::make_shared<ObjectConfig>("Account", false, lobbyDB);
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::Account>;

    mRegisteredTypes["Account"] = cfg;

    cfg = std::make_shared<ObjectConfig>("Character", true, worldDB);
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::Character>;

    mRegisteredTypes["Character"] = cfg;

    cfg = std::make_shared<ObjectConfig>("CharacterProgress", true, worldDB);
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::CharacterProgress>;

    mRegisteredTypes["CharacterProgress"] = cfg;

    cfg = std::make_shared<ObjectConfig>("MatchEntry", true);
    cfg->BuildHandler = &DataSyncManager::New<objects::MatchEntry>;
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::MatchEntry>;
    cfg->SyncCompleteHandler = &DataSyncManager::SyncComplete<
        WorldSyncManager, objects::MatchEntry>;

    mRegisteredTypes["MatchEntry"] = cfg;

    cfg = std::make_shared<ObjectConfig>("PvPMatch", true);
    cfg->BuildHandler = &DataSyncManager::New<objects::PvPMatch>;
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::PvPMatch>;
    cfg->SyncCompleteHandler = &DataSyncManager::SyncComplete<
        WorldSyncManager, objects::PvPMatch>;

    mRegisteredTypes["PvPMatch"] = cfg;

    return true;
}

namespace world
{
template<>
int8_t WorldSyncManager::Update<objects::Account>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
    const libcomp::String& source)
{
    (void)type;
    (void)isRemove;

    auto entry = std::dynamic_pointer_cast<objects::Account>(obj);

    libcomp::Packet p;

    if(source == "lobby")
    {
        // Send to the channel where the account is logged in
        auto server = mServer.lock();
        auto accountManager = server->GetAccountManager();

        auto accountLogin = accountManager->GetUserLogin(entry->GetUsername());
        auto cLogin = accountLogin ? accountLogin->GetCharacterLogin() : nullptr;
        int8_t channelID = cLogin ? cLogin->GetChannelID() : -1;

        if(channelID >= 0)
        {
            auto channel = server->GetChannelConnectionByID(channelID);
            if (channel)
            {
                WriteOutgoingRecord(p, true, "Account", entry);
                channel->SendPacket(p);
            }
        }
    }
    else
    {
        // Send to the lobby
        WriteOutgoingRecord(p, true, "Account", entry);
        mServer.lock()->GetLobbyConnection()->SendPacket(p);
    }

    return SYNC_HANDLED;
}

template<>
int8_t WorldSyncManager::Update<objects::Character>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
    const libcomp::String& source)
{
    (void)type;

    auto entry = std::dynamic_pointer_cast<objects::Character>(obj);

    if(isRemove)
    {
        // Nothing special to do if not removing
        if(source == "lobby")
        {
            // Delete is valid, queue the delete
            auto server = mServer.lock();
            server->QueueWork([](std::shared_ptr<WorldServer> pServer,
                std::shared_ptr<objects::Character> character)
            {
                pServer->GetAccountManager()->DeleteCharacter(character);
            }, server, entry);
        }
        else
        {
            LOG_ERROR(libcomp::String("Channel requested a character delete."
                " which will be ignored: %1\n").Arg(source));
        }
    }

    return SYNC_HANDLED;
}

template<>
int8_t WorldSyncManager::Update<objects::CharacterProgress>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source)
{
    (void)type;
    (void)isRemove;

    auto entry = std::dynamic_pointer_cast<objects::CharacterProgress>(obj);
  
    if(source == "lobby")
    {
        // Send to the channel where the character is logged in, no need to
        // sync if they are not
        auto server = mServer.lock();
        auto characterManager = server->GetCharacterManager();
        auto character = std::dynamic_pointer_cast<objects::Character>(
            libcomp::PersistentObject::GetObjectByUUID(entry->GetCharacter()));

        auto cLogin = character
            ? characterManager->GetCharacterLogin(character->GetName()) : nullptr;
        int8_t channelID = cLogin ? cLogin->GetChannelID() : -1;

        if(channelID >= 0)
        {
            auto channel = server->GetChannelConnectionByID(channelID);
            if(channel)
            {
                libcomp::Packet p;
                WriteOutgoingRecord(p, true, "CharacterProgress", entry);
                channel->SendPacket(p);
            }
        }
    }

    return SYNC_HANDLED;
}

template<>
int8_t WorldSyncManager::Update<objects::MatchEntry>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
    const libcomp::String& source)
{
    (void)type;
    (void)source;

    auto entry = std::dynamic_pointer_cast<objects::MatchEntry>(obj);

    if(isRemove)
    {
        mMatchEntries.erase(entry->GetWorldCID());
    }
    else
    {
        auto cLogin = mServer.lock()->GetCharacterManager()->GetCharacterLogin(
            entry->GetWorldCID());
        if(!cLogin || cLogin->GetChannelID() < 0)
        {
            // Not logged in, do not register
            return SYNC_FAILED;
        }

        if(!source.IsEmpty())
        {
            // Update came from other server
            entry->SetEntryTime((uint32_t)std::time(0));
        }

        mMatchEntries[entry->GetWorldCID()] = entry;
    }

    return SYNC_UPDATED;
}

template<>
void WorldSyncManager::SyncComplete<objects::MatchEntry>(
    const libcomp::String& type, const std::list<std::pair<std::shared_ptr<
    libcomp::Object>, bool>>& objs, const libcomp::String& source)
{
    (void)type;
    (void)source;

    bool pvpAdded[2][2] = { { false, false }, { false, false } };
    bool pvpRemoved[2][2] = { { false, false }, { false, false } };
    std::array<std::array<std::set<int32_t>, 2>, 2> cids;

    for(auto& pair : objs)
    {
        auto entry = std::dynamic_pointer_cast<objects::MatchEntry>(
            pair.first);
        uint8_t isTeamIdx = entry->GetTeamID() != 0 ? 1 : 0;
        uint8_t matchType = (uint8_t)entry->GetMatchType();
        if(matchType < 2)
        {
            if(pair.second)
            {
                pvpRemoved[isTeamIdx][matchType] = true;
            }
            else
            {
                pvpAdded[isTeamIdx][matchType] = true;
            }

            cids[isTeamIdx][matchType].insert(entry->GetWorldCID());
        }
    }

    for(uint8_t i = 0; i < 2; i++)
    {
        for(uint8_t k = 0; k < 2; k++)
        {
            // If match is ready and entrants were removed or match is not
            // ready and entrants were added, check if an update needs to
            // happen
            bool checkStop = (mPvPReadyTimes[i][k] != 0) == pvpRemoved[i][k];
            bool checkStart = (mPvPReadyTimes[i][k] == 0) == pvpAdded[i][k];
            if(checkStop || checkStart)
            {
                if(i == 0)
                {
                    DeterminePvPMatch(k, cids[i][k]);
                }
                else
                {
                    DetermineTeamPvPMatch(k, cids[i][k]);
                }
            }
        }
    }
}

template<>
int8_t WorldSyncManager::Update<objects::PvPMatch>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
    const libcomp::String& source)
{
    (void)type;

    auto match = std::dynamic_pointer_cast<objects::PvPMatch>(obj);
    if(!isRemove && !source.IsEmpty() && match->GetID())
    {
        // Matches with IDs should not be passing through here from
        // other servers
        return SYNC_FAILED;
    }

    return SYNC_UPDATED;
}

template<>
void WorldSyncManager::SyncComplete<objects::PvPMatch>(
    const libcomp::String& type, const std::list<std::pair<std::shared_ptr<
    libcomp::Object>, bool>>& objs, const libcomp::String& source)
{
    (void)type;

    for(auto& objPair : objs)
    {
        auto match = std::dynamic_pointer_cast<objects::PvPMatch>(
            objPair.first);

        if(!objPair.second && !source.IsEmpty())
        {
            // Channel is requesting a custom match, set it up and let it
            // sync back
            CreatePvPMatch(match->GetType(), match);
        }
    }
}

template<>
void WorldSyncManager::Expire<objects::SearchEntry>(int32_t entryID,
    uint32_t expirationTime)
{
    std::shared_ptr<objects::SearchEntry> entry;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto e : mSearchEntries)
        {
            if(e->GetEntryID() == entryID &&
                e->GetExpirationTime() == expirationTime)
            {
                entry = e;
                break;
            }
        }
    }

    if(entry && RemoveRecord(entry, "SearchEntry"))
    {
        SyncOutgoing();
    }
}

template<>
int8_t WorldSyncManager::Update<objects::SearchEntry>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
    const libcomp::String& source)
{
    (void)type;
    (void)source;

    auto entry = std::dynamic_pointer_cast<objects::SearchEntry>(obj);
    
    auto it = mSearchEntries.begin();
    while(it != mSearchEntries.end())
    {
        if((*it)->GetEntryID() == entry->GetEntryID())
        {
            auto existing = *it;

            // If the entry is being removed or having its type or source modified
            // update the map count
            if(isRemove || existing->GetSourceCID() != entry->GetSourceCID() ||
                existing->GetType() != entry->GetType())
            {
                AdjustSearchEntryCount(existing->GetSourceCID(), existing->GetType(), false);
            }

            it = mSearchEntries.erase(it);
            if(!isRemove)
            {
                // Replace the existing element and update the count again
                mSearchEntries.insert(it, entry);
                AdjustSearchEntryCount(entry->GetSourceCID(), entry->GetType(), true);
            }

            return SYNC_UPDATED;
        }

        it++;
    }

    if(isRemove)
    {
        LOG_WARNING(libcomp::String("No SearchEntry with ID '%1' found"
            " for sync removal\n").Arg(entry->GetEntryID()));
        return SYNC_FAILED;
    }
    else
    {
        int32_t nextEntryID = 1;
        if(mSearchEntries.size() > 0)
        {
            nextEntryID = mSearchEntries.front()->GetEntryID() + 1;
        }

        entry->SetEntryID(nextEntryID);

        AdjustSearchEntryCount(entry->GetSourceCID(), entry->GetType(), true);

        mSearchEntries.push_front(entry);

        if(entry->GetExpirationTime() != 0)
        {
            // Set to expire
            uint32_t now = (uint32_t)std::time(0);

            // Default to 10 minutes if the expiration is invalid or passed
            uint32_t exp = now < entry->GetExpirationTime()
                ? (uint32_t)(entry->GetExpirationTime() - now) : 600;

            mServer.lock()->GetTimerManager()->ScheduleEventIn((int)exp, []
                (WorldSyncManager* pSyncManager, int32_t pEntryID,
                uint32_t pExpireTime)
            {
                pSyncManager->Expire<objects::SearchEntry>(
                    pEntryID, pExpireTime);
            }, this, entry->GetEntryID(), entry->GetExpirationTime());
        }

        return SYNC_UPDATED;
    }
}
}

bool WorldSyncManager::RemoveRecord(const std::shared_ptr<libcomp::Object>& record,
    const libcomp::String& type)
{
    if(DataSyncManager::RemoveRecord(record, type))
    {
        bool recalcTeamPvP = false;
        std::list<std::shared_ptr<libcomp::Object>> additionalRemoves;
        if(type == "MatchEntry")
        {
            // If a team match entry is being removed, remove the rest of
            // the team
            auto entry = std::dynamic_pointer_cast<objects::MatchEntry>(
                record);
            if(entry->GetTeamID())
            {
                std::lock_guard<std::mutex> lock(mLock);
                for(auto& ePair : mMatchEntries)
                {
                    if(ePair.second->GetTeamID() == entry->GetTeamID())
                    {
                        additionalRemoves.push_back(ePair.second);
                    }
                }

                recalcTeamPvP = true;
            }
        }
        else if(type == "SearchEntry")
        {
            // If the record is a search entry, be sure to also remove all
            // child entries
            std::lock_guard<std::mutex> lock(mLock);

            auto entry = std::dynamic_pointer_cast<objects::SearchEntry>(
                record);
            for(auto e : mSearchEntries)
            {
                if(e->GetParentEntryID() == entry->GetEntryID())
                {
                    additionalRemoves.push_back(e);
                }
            }
        }

        for(auto entry : additionalRemoves)
        {
            DataSyncManager::RemoveRecord(entry, type);
        }

        if(recalcTeamPvP)
        {
            auto entry = std::dynamic_pointer_cast<objects::MatchEntry>(
                record);

            std::lock_guard<std::mutex> lock(mLock);
            DetermineTeamPvPMatch((uint8_t)entry->GetMatchType());
        }

        return true;
    }

    return false;
}

std::shared_ptr<objects::MatchEntry> WorldSyncManager::GetMatchEntry(
    int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mMatchEntries.find(worldCID);
    return it != mMatchEntries.end() ? it->second : nullptr;
}

bool WorldSyncManager::CleanUpCharacterLogin(int32_t worldCID, bool flushOutgoing)
{
    bool result = false;

    std::list<std::shared_ptr<libcomp::Object>> searchEntries;
    std::shared_ptr<objects::MatchEntry> matchEntry;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(mSearchEntryCounts.find(worldCID) != mSearchEntryCounts.end())
        {
            // Drop all non-clan search entries
            for(auto entry : mSearchEntries)
            {
                if(entry->GetSourceCID() == worldCID &&
                    entry->GetType() != objects::SearchEntry::Type_t::CLAN_JOIN &&
                    entry->GetType() != objects::SearchEntry::Type_t::CLAN_RECRUIT)
                {
                    entry->SetLastAction(
                        objects::SearchEntry::LastAction_t::REMOVE_LOGOFF);
                    searchEntries.push_back(entry);
                }
            }
        }

        auto it = mMatchEntries.find(worldCID);
        if(it != mMatchEntries.end())
        {
            matchEntry = it->second;
        }
    }

    // Remove search entries
    if(searchEntries.size() > 0)
    {
        for(auto entry : searchEntries)
        {
            result |= RemoveRecord(entry, "SearchEntry");
        }
    }

    // Remove match entry (and recalc)
    if(matchEntry)
    {
        result |= RemoveRecord(matchEntry, "MatchEntry");
        if(!matchEntry->GetTeamID())
        {
            result |= DeterminePvPMatch((uint8_t)matchEntry->GetMatchType());
        }
        else
        {
            result |= DetermineTeamPvPMatch((uint8_t)matchEntry
                ->GetMatchType());
        }
    }

    if(result && flushOutgoing)
    {
        SyncOutgoing();
    }

    return result;
}

void WorldSyncManager::SyncExistingChannelRecords(const std::shared_ptr<
    libcomp::InternalConnection>& connection)
{
    std::lock_guard<std::mutex> lock(mLock);

    std::set<std::shared_ptr<libcomp::Object>> records;
    std::set<std::shared_ptr<libcomp::Object>> blank;
    for(auto entry : mSearchEntries)
    {
        records.insert(entry);
    }

    QueueOutgoing("SearchEntry", connection, records, blank);

    records.clear();
    for(auto& pair : mMatchEntries)
    {
        records.insert(pair.second);
    }

    QueueOutgoing("MatchEntry", connection, records, blank);

    connection->FlushOutgoing();
}

void WorldSyncManager::StartPvPMatch(uint32_t time, uint8_t type)
{
    bool start = false;
    uint32_t queuedTime = 0;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(type < 2)
        {
            queuedTime = mPvPReadyTimes[0][type];
            if(queuedTime == time)
            {
                mPvPReadyTimes[0][type] = 0;
                start = true;
            }
        }
    }

    bool recheck = !queuedTime;
    if(start)
    {
        // Check if the required number of entries exist. If they do, form
        // teams and create match
        size_t minCount = type == 0 ? PVP_FATE_PLAYER_MIN
            : PVP_VALHALLA_PLAYER_MIN;
        size_t maxCount = type == 0 ? PVP_FATE_PLAYER_MAX
            : PVP_VALHALLA_PLAYER_MAX;

        auto entries = GetMatchEntryTeams(type)[0];

        uint8_t ghost = std::dynamic_pointer_cast<objects::WorldConfig>(
            mServer.lock()->GetConfig())->GetWorldSharedConfig()
            ->GetPvPGhosts((size_t)type);
        if(entries.size() >= 2 && (size_t)(entries.size() + ghost) >= minCount)
        {
            // First in, first out
            entries.sort([](const std::shared_ptr<objects::MatchEntry>& a,
                const std::shared_ptr<objects::MatchEntry>& b)
                {
                    return a->GetEntryTime() < b->GetEntryTime();
                });

            size_t teamCount = entries.size();
            if(teamCount > maxCount)
            {
                teamCount = maxCount;
            }

            // If team sizes does not match, drop the last one
            if(teamCount % 2 == 1)
            {
                teamCount = (size_t)(teamCount - 1);
            }

            auto match = CreatePvPMatch((int8_t)type);

            // Split the teams and set the match ID
            size_t left = teamCount;

            std::set<int32_t> cids;
            while(left)
            {
                auto entry = entries.front();
                entries.pop_front();

                entry->SetMatchID(match->GetID());
                entry->SetReadyTime(match->GetReadyTime());

                // World is now done with the record
                RemoveRecord(entry, "MatchEntry");

                cids.insert(entry->GetWorldCID());

                left = (size_t)(left - 1);
            }

            for(size_t i = 0; i < teamCount; i++)
            {
                int32_t cid = libcomp::Randomizer::GetEntry(cids);
                cids.erase(cid);

                if(i % 2 == 0)
                {
                    match->AppendBlueMemberIDs(cid);
                }
                else
                {
                    match->AppendRedMemberIDs(cid);
                }
            }

            UpdateRecord(match, "PvPMatch");
            SyncOutgoing();
        }

        // Always recheck
        recheck = true;
    }

    if(recheck)
    {
        if(DeterminePvPMatch(type))
        {
            SyncOutgoing();
        }
    }
}

void WorldSyncManager::StartTeamPvPMatch(uint32_t time, uint8_t type)
{
    bool start = false;
    uint32_t queuedTime = 0;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(type < 2)
        {
            queuedTime = mPvPReadyTimes[1][type];
            if(queuedTime == time)
            {
                mPvPReadyTimes[1][type] = 0;
                start = true;
            }
        }
    }

    bool recheck = !queuedTime;
    if(start)
    {
        // Check if two teams with the required number of entries exist.
        // If they do, form teams and create match
        size_t minCount = (size_t)((type == 0 ? PVP_FATE_PLAYER_MIN
            : PVP_VALHALLA_PLAYER_MIN) / 2);

        uint8_t ghost = std::dynamic_pointer_cast<objects::WorldConfig>(
            mServer.lock()->GetConfig())->GetWorldSharedConfig()
            ->GetPvPGhosts((size_t)type);
        uint8_t gAdjust = (uint8_t)((ghost + 1) / 2);

        auto teamEntries = GetMatchEntryTeams(type);
        teamEntries.erase(0);   // Drop solo entries

        std::list<std::list<std::shared_ptr<objects::MatchEntry>>> readyTeams;
        for(auto& pair : teamEntries)
        {
            auto team = pair.second;
            if((size_t)(team.size() + gAdjust) >= minCount)
            {
                // Sort by first registered
                team.sort([](const std::shared_ptr<objects::MatchEntry>& a,
                    const std::shared_ptr<objects::MatchEntry>& b)
                    {
                        return a->GetEntryTime() < b->GetEntryTime();
                    });

                readyTeams.push_back(team);
            }
        }

        if(readyTeams.size() >= 2)
        {
            // Determine first in, first out based on team members and
            // split into blue and read teams
            readyTeams.sort([]
                (const std::list<std::shared_ptr<objects::MatchEntry>>& a,
                const std::list<std::shared_ptr<objects::MatchEntry>>& b)
                {
                    return a.front()->GetEntryTime() <
                        b.front()->GetEntryTime();
                });

            std::array<std::list<
                std::shared_ptr<objects::MatchEntry>>, 2> teams;
            teams[0] = readyTeams.front();
            readyTeams.pop_front();

            teams[1] = readyTeams.front();

            auto match = CreatePvPMatch((int8_t)type);

            std::set<int32_t> cids;
            for(size_t i = 0; i < 2; i++)
            {
                for(auto entry : teams[i])
                {
                    entry->SetMatchID(match->GetID());
                    entry->SetReadyTime(match->GetReadyTime());

                    // World is now done with the record
                    RemoveRecord(entry, "MatchEntry");

                    cids.insert(entry->GetWorldCID());

                    if(i % 2 == 0)
                    {
                        match->AppendBlueMemberIDs(entry->GetWorldCID());
                    }
                    else
                    {
                        match->AppendRedMemberIDs(entry->GetWorldCID());
                    }
                }
            }

            UpdateRecord(match, "PvPMatch");
            SyncOutgoing();
        }

        // Always recheck
        recheck = true;
    }

    if(recheck)
    {
        if(DetermineTeamPvPMatch(type))
        {
            SyncOutgoing();
        }
    }
}

void WorldSyncManager::AdjustSearchEntryCount(int32_t sourceCID,
    objects::SearchEntry::Type_t type, bool increment)
{
    if(increment)
    {
        if(mSearchEntryCounts[sourceCID].find(type) ==
            mSearchEntryCounts[sourceCID].end())
        {
            mSearchEntryCounts[sourceCID][type] = 1;
        }
        else
        {
            mSearchEntryCounts[sourceCID][type]++;
        }
    }
    else
    {
        auto it = mSearchEntryCounts[sourceCID].find(type);
        if(it != mSearchEntryCounts[sourceCID].end() && it->second != 0)
        {
            if(it->second == 1)
            {
                mSearchEntryCounts[sourceCID].erase(type);
                if(mSearchEntryCounts[sourceCID].size() == 0)
                {
                    mSearchEntryCounts.erase(sourceCID);
                }
            }
            else
            {
                it->second--;
            }
        }
    }
}

bool WorldSyncManager::DeterminePvPMatch(uint8_t type,
    std::set<int32_t> updated)
{
    if(type >= 2)
    {
        // Not a standard PvP type
        return false;
    }

    uint32_t time = mPvPReadyTimes[0][type];

    bool checkStop = time != 0;
    bool checkStart = time == 0;

    size_t minCount = type == 0 ? PVP_FATE_PLAYER_MIN
        : PVP_VALHALLA_PLAYER_MIN;

    auto soloEntries = GetMatchEntryTeams(type)[0];

    std::list<std::shared_ptr<objects::MatchEntry>> readyPrevious;
    for(auto entry : soloEntries)
    {
        if(entry->GetReadyTime() &&
            updated.find(entry->GetWorldCID()) == updated.end())
        {
            readyPrevious.push_back(entry);
        }
    }

    auto server = mServer.lock();
    auto config = std::dynamic_pointer_cast<objects::WorldConfig>(
        server->GetConfig())->GetWorldSharedConfig();

    int32_t qWait = (int32_t)config->GetPvPQueueWait();

    uint8_t ghost = config->GetPvPGhosts((size_t)type);
    if(checkStop && (readyPrevious.size() < 2 ||
        (size_t)(readyPrevious.size() + ghost) < minCount))
    {
        // Ready count dropped below min amount, reset the ready time
        mPvPReadyTimes[0][type] = time = 0;

        // Allow restart if new entry count meets minimum
        checkStart = true;
    }

    if(checkStart && (size_t)(soloEntries.size() + ghost) >= minCount)
    {
        // Entry count raised above min amount, queue match start
        mPvPReadyTimes[0][type] = time = (uint32_t)((int32_t)
            std::time(0) + qWait);

        server->GetTimerManager()->ScheduleEventIn((int)qWait, []
            (WorldSyncManager* pSyncManager, uint32_t pTime, uint8_t pType)
            {
                pSyncManager->StartPvPMatch(pTime, pType);
            }, this, time, type);
    }

    // Sync all current ready times
    bool queued = false;
    for(auto entry : soloEntries)
    {
        if(entry->GetReadyTime() != time)
        {
            entry->SetReadyTime(time);
            queued |= UpdateRecord(entry, "MatchEntry");
        }
    }

    return queued;
}

bool WorldSyncManager::DetermineTeamPvPMatch(uint8_t type,
    std::set<int32_t> updated)
{
    if(type >= 2)
    {
        // Not a standard PvP type
        return false;
    }

    uint32_t time = mPvPReadyTimes[1][type];

    bool checkStop = time != 0;
    bool checkStart = time == 0;

    size_t minCount = (size_t)((type == 0 ? PVP_FATE_PLAYER_MIN
        : PVP_VALHALLA_PLAYER_MIN) / 2);
    size_t maxCount = (size_t)((type == 0 ? PVP_FATE_PLAYER_MAX
        : PVP_VALHALLA_PLAYER_MAX) / 2);

    auto server = mServer.lock();
    auto config = std::dynamic_pointer_cast<objects::WorldConfig>(
        server->GetConfig())->GetWorldSharedConfig();

    int32_t qWait = (int32_t)config->GetPvPQueueWait();

    uint8_t ghost = config->GetPvPGhosts((size_t)type);
    uint8_t gAdjust = (uint8_t)((ghost + 1) / 2);

    auto teamEntries = GetMatchEntryTeams(type);
    teamEntries.erase(0);   // Drop solo entries

    bool queued = false;

    std::set<int32_t> readyTeams;
    std::set<int32_t> previousReadyTeams;
    for(auto& pair : teamEntries)
    {
        if((size_t)(pair.second.size() + gAdjust) >= minCount &&
            pair.second.size() <= maxCount)
        {
            readyTeams.insert(pair.first);
            previousReadyTeams.insert(pair.first);

            // If any entry was updated, remove the team from the ready set
            for(auto entry : pair.second)
            {
                if(updated.find(entry->GetWorldCID()) == updated.end())
                {
                    previousReadyTeams.erase(pair.first);
                    break;
                }
            }
        }
        else
        {
            // Drop all entries as the team is not a valid size
            for(auto entry : pair.second)
            {
                queued |= RemoveRecord(entry, "MatchEntry");
            }

            pair.second.clear();
        }
    }

    if(checkStop && previousReadyTeams.size() < 2)
    {
        // Ready team count dropped below min amount, reset the ready time
        mPvPReadyTimes[1][type] = time = 0;

        // Allow restart if new entry count meets minimum
        checkStart = true;
    }

    if(checkStart && readyTeams.size() >= 2)
    {
        // Ready team count raised above min amount, queue match start
        mPvPReadyTimes[1][type] = time = (uint32_t)((int32_t)
            std::time(0) + qWait);

        server->GetTimerManager()->ScheduleEventIn((int)qWait, []
            (WorldSyncManager* pSyncManager, uint32_t pTime, uint8_t pType)
            {
                pSyncManager->StartTeamPvPMatch(pTime, pType);
            }, this, time, type);
    }

    // Sync all current ready times
    for(auto& pair : teamEntries)
    {
        for(auto entry : pair.second)
        {
            if(entry->GetReadyTime() != time)
            {
                entry->SetReadyTime(time);
                queued |= UpdateRecord(entry, "MatchEntry");
            }
        }
    }

    return queued;
}

std::shared_ptr<objects::PvPMatch> WorldSyncManager::CreatePvPMatch(
    int8_t type, std::shared_ptr<objects::PvPMatch> existing)
{
    uint32_t matchID = 0;
    {
        std::lock_guard<std::mutex> lock(mLock);
        matchID = ++mNextMatchID;
    }

    uint32_t matchReady = (uint32_t)((uint32_t)std::time(0) + 30);

    auto match = existing ? existing : std::make_shared<objects::PvPMatch>();
    match->SetID(matchID);
    match->SetType(type);
    match->SetReadyTime(matchReady);

    uint8_t channelID = 0;

    /// @todo: replace with dedicated channel logic
    for(auto& cPair : mServer.lock()->GetChannels())
    {
        channelID = cPair.second->GetID();
        break;
    }

    match->SetChannelID(channelID);

    return match;
}

std::unordered_map<int32_t, std::list<std::shared_ptr<
    objects::MatchEntry>>> WorldSyncManager::GetMatchEntryTeams(uint8_t type)
{
    std::unordered_map<int32_t,
        std::list<std::shared_ptr<objects::MatchEntry>>> entryTeams;
    for(auto& pair : mMatchEntries)
    {
        auto entry = pair.second;
        if((uint8_t)entry->GetMatchType() == type &&
            !entry->GetMatchID())
        {
            entryTeams[entry->GetTeamID()].push_back(entry);
        }
    }

    return entryTeams;
}
