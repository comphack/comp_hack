/**
 * @file server/world/src/WorldSyncManager.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief 
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

// Standard C++11 Includes
#include <algorithm>

// object Includes
#include <Account.h>
#include <CharacterLogin.h>

// world Includes
#include <WorldServer.h>

using namespace world;

WorldSyncManager::WorldSyncManager(const std::weak_ptr<
    WorldServer>& server) : mServer(server)
{
}

WorldSyncManager::~WorldSyncManager()
{
}

bool WorldSyncManager::Initialize()
{
    auto lobbyDB = mServer.lock()->GetLobbyDatabase();

    // Build the configs
    auto cfg = std::make_shared<ObjectConfig>(
        "SearchEntry", true);
    cfg->BuildHandler = &DataSyncManager::New<objects::SearchEntry>;
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::SearchEntry>;

    mRegisteredTypes["SearchEntry"] = cfg;

    cfg = std::make_shared<ObjectConfig>(
        "Account", false, lobbyDB);
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::Account>;

    mRegisteredTypes["Account"] = cfg;

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
    bool result = DataSyncManager::RemoveRecord(record, type);

    std::list<std::shared_ptr<libcomp::Object>> additionalRemoves;
    if(type == "SearchEntry")
    {
        // If the record is a search entry, be sure to also remove all
        // child entries
        std::lock_guard<std::mutex> lock(mLock);

        auto entry = std::dynamic_pointer_cast<objects::SearchEntry>(record);
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
        result |= RemoveRecord(entry, "SearchEntry");
    }

    return result;
}

bool WorldSyncManager::CleanUpCharacterLogin(int32_t worldCID, bool flushOutgoing)
{
    bool result = false;

    std::list<std::shared_ptr<libcomp::Object>> records;
    if(mSearchEntryCounts.find(worldCID) != mSearchEntryCounts.end())
    {
        std::lock_guard<std::mutex> lock(mLock);

        // Drop all non-clan search entries
        for(auto entry : mSearchEntries)
        {
            if(entry->GetSourceCID() == worldCID &&
                entry->GetType() != objects::SearchEntry::Type_t::CLAN_JOIN &&
                entry->GetType() != objects::SearchEntry::Type_t::CLAN_RECRUIT)
            {
                entry->SetLastAction(
                    objects::SearchEntry::LastAction_t::REMOVE_LOGOFF);
                records.push_back(entry);
            }
        }
    }

    // Remove search entries
    if(records.size() > 0)
    {
        for(auto entry : records)
        {
            result |= RemoveRecord(entry, "SearchEntry");
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

    connection->FlushOutgoing();
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
