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

AccountManager::AccountManager()
{
    mMaxSessionKey = 0;
}

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

        channel = pair->second->GetChannelID();
    }

    return result;
}

bool AccountManager::LoginUser(const libcomp::String& username,
    std::shared_ptr<objects::AccountLogin> login)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() == pair)
    {
        if(nullptr == login)
        {
            login = std::shared_ptr<objects::AccountLogin>(
                new objects::AccountLogin);
        }

        login->SetSessionKey(mMaxSessionKey++);

        auto res = mAccountMap.insert(std::make_pair(lookup, login));

        // This pair is the iterator (first) and a bool indicating it was
        // inserted into the map (second).
        result = res.second;
    }

    return result;
}

std::shared_ptr<objects::AccountLogin> AccountManager::GetUserLogin(
    const libcomp::String& username)
{
    libcomp::String lookup = username.ToLower();

    auto pair = mAccountMap.find(lookup);
    return pair != mAccountMap.end() ? pair->second : nullptr;
}

bool AccountManager::LogoutUser(const libcomp::String& username, int8_t channel)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair && channel == pair->second->GetChannelID())
    {
        (void)mAccountMap.erase(pair);

        result = true;
    }

    return result;
}

std::list<libcomp::String> AccountManager::LogoutUsersOnChannel(int8_t channel)
{
    if(0 > channel)
    {
        return std::list<libcomp::String>();
    }

    std::list<libcomp::String> usernames;
    for(auto pair : mAccountMap)
    {
        if(pair.second->GetChannelID() == channel)
        {
            usernames.push_back(pair.first);
        }
    }

    for(auto username : usernames)
    {
        mAccountMap.erase(username);
    }

    return usernames;
}
