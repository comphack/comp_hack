/**
 * @file server/world/src/AccountManager.cpp
 * @ingroup world
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to track accounts that are logged in.
 *
 * This file is part of the World Server (world).
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

#include "AccountManager.h"

using namespace world;

bool AccountManager::IsLoggedIn(const libcomp::String& username) const
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair)
    {
        result = true;
    }

    return result;
}

bool AccountManager::IsLoggedIn(const libcomp::String& username,
    int8_t& channel) const
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair)
    {
        result = true;

        channel = pair->second;
    }

    return result;
}

bool AccountManager::LoginUser(const libcomp::String& username, int8_t channel)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() == pair)
    {
        auto res = mAccountMap.insert(std::make_pair(username, channel));

        // This pair is the iterator (first) and a bool indicating it was
        // inserted into the map (second).
        result = res.second;
    }

    return result;
}

bool AccountManager::LogoutUser(const libcomp::String& username, int8_t channel)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair && channel == pair->second)
    {
        (void)mAccountMap.erase(pair);

        result = true;
    }

    return result;
}
