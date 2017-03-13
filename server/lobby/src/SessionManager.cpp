/**
 * @file server/lobby/src/SessionManager.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to track session keys.
 *
 * This file is part of the Lobby Server (lobby).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#include "SessionManager.h"

// libcomp Includes
#include <Decrypt.h>

using namespace lobby;

std::pair<libcomp::String, libcomp::String> SessionManager::GenerateSIDs(
    const libcomp::String& username)
{
    auto result = std::make_pair(
        libcomp::Decrypt::GenerateRandom(300).ToLower(),
        libcomp::Decrypt::GenerateRandom(300).ToLower());

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mSessionLock);

    auto entry = mSessionMap.find(lookup);

    if(mSessionMap.end() != entry)
    {
        entry->second = result;
    }
    else
    {
        auto res = mSessionMap.insert(std::make_pair(lookup, result));

        if(!res.second)
        {
            return {};
        }
    }

    return result;
}

bool SessionManager::CheckSID(const libcomp::String& username,
    const libcomp::String& value, libcomp::String& newSID)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mSessionLock);

    auto entry = mSessionMap.find(lookup);

    if(mSessionMap.end() != entry)
    {
        result = (entry->second.first == value);

        if(result)
        {
            mSessionMap[lookup].second = entry->second.first;
            newSID = libcomp::Decrypt::GenerateRandom(300).ToLower();
            mSessionMap[lookup].first = newSID;
        }
    }

    return result;
}

void SessionManager::ExpireSession(const libcomp::String& username)
{
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mSessionLock);
    mSessionMap.erase(lookup);
}
