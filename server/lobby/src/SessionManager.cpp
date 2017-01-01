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

libcomp::String SessionManager::GenerateSID(uint8_t sid,
    const libcomp::String& username)
{
    if(1 < sid)
    {
        return {};
    }

    libcomp::String result = libcomp::Decrypt::GenerateRandom(300).ToLower();
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mSessionLock);

    auto entry = mSessionMap.find(username);

    if(mSessionMap.end() != entry)
    {
        if(0 == sid)
        {
            entry->second.first = result;
        }
        else // 1 == sid
        {
            entry->second.second = result;
        }
    }
    else
    {
        if(0 == sid)
        {
            auto res = mSessionMap.insert(std::make_pair(lookup,
                std::make_pair(result, libcomp::String())));

            // Check the insert.
            if(!res.second)
            {
                result.Clear();
            }
        }
        else // 1 == sid
        {
            auto res = mSessionMap.insert(std::make_pair(lookup,
                std::make_pair(libcomp::String(), result)));

            // Check the insert.
            if(!res.second)
            {
                result.Clear();
            }
        }
    }

    return result;
}

std::pair<libcomp::String, libcomp::String> SessionManager::GenerateSIDs(
    const libcomp::String& username)
{
    auto result = std::make_pair(
        libcomp::Decrypt::GenerateRandom(300).ToLower(),
        libcomp::Decrypt::GenerateRandom(300).ToLower());

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mSessionLock);

    auto entry = mSessionMap.find(username);

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

bool SessionManager::CheckSID(uint8_t sid, const libcomp::String& username,
    const libcomp::String& value)
{
    bool result = false;

    if(1 < sid)
    {
        return result;
    }

    libcomp::String nextSID = libcomp::Decrypt::GenerateRandom(300).ToLower();
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mSessionLock);

    auto entry = mSessionMap.find(username);

    if(mSessionMap.end() != entry)
    {
        if(0 == sid)
        {
            result = (entry->second.first == value);

            if(result)
            {
                entry->second.first = nextSID;
            }
        }
        else // 1 == sid
        {
            result = (entry->second.second == value);

            if(result)
            {
                entry->second.second = nextSID;
            }
        }
    }

    return result;
}
