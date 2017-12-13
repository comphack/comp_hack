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
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

using namespace world;

WorldSyncManager::WorldSyncManager()
{
}

WorldSyncManager::WorldSyncManager(const std::weak_ptr<
    WorldServer>& server) : mServer(server)
{
}

WorldSyncManager::~WorldSyncManager()
{
}

bool WorldSyncManager::Initialize()
{
    // Build the configs
    auto cfg = std::make_shared<ObjectConfig>(
        "SearchEntry", true);
    cfg->BuildHandler = &DataSyncManager::New<objects::SearchEntry>;
    cfg->UpdateHandler = &DataSyncManager::Update<WorldSyncManager,
        objects::SearchEntry>;

    mRegisteredTypes["SearchEntry"] = cfg;

    return true;
}

namespace world
{
template<>
bool WorldSyncManager::Update<objects::SearchEntry>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove)
{
    (void)type;

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

            return true;
        }

        it++;
    }

    if(isRemove)
    {
        LOG_WARNING(libcomp::String("No SearchEntry with ID '%1' found"
            " for sync removal\n").Arg(entry->GetEntryID()));
        return false;
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

        return true;
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
