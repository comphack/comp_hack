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

// object Includes
#include <Account.h>
#include <Character.h>
#include <CharacterLogin.h>

using namespace world;

AccountManager::AccountManager()
{
    mMaxSessionKey = 0;
}

bool AccountManager::IsLoggedIn(const libcomp::String& username,
    int8_t& channel)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mLock);
    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair)
    {
        result = true;

        channel = pair->second->GetCharacterLogin()->GetChannelID();
    }

    return result;
}

bool AccountManager::LoginUser(std::shared_ptr<objects::AccountLogin> login)
{
    bool result = false;

    libcomp::String lookup = login->GetAccount()->GetUsername().ToLower();

    std::lock_guard<std::mutex> lock(mLock);
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

    std::lock_guard<std::mutex> lock(mLock);
    auto pair = mAccountMap.find(lookup);
    return pair != mAccountMap.end() ? pair->second : nullptr;
}

std::shared_ptr<objects::AccountLogin> AccountManager::LogoutUser(
    const libcomp::String& username, int8_t channel)
{
    std::shared_ptr<objects::AccountLogin> result;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mLock);
    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair && channel == pair->second->
        GetCharacterLogin()->GetChannelID())
    {
        result = pair->second;
        Cleanup(result);
        (void)mAccountMap.erase(pair);
    }

    return result;
}

std::list<std::shared_ptr<objects::AccountLogin>>
    AccountManager::LogoutUsersOnChannel(int8_t channel)
{
    std::list<std::shared_ptr<objects::AccountLogin>> loggedOut;
    if(0 > channel)
    {
        return loggedOut;
    }

    std::list<libcomp::String> usernames;
    std::lock_guard<std::mutex> lock(mLock);
    for(auto pair : mAccountMap)
    {
        auto cLogin = pair.second->GetCharacterLogin();
        if(cLogin->GetChannelID() == channel)
        {
            usernames.push_back(pair.first);
            loggedOut.push_back(pair.second);
        }

        Cleanup(pair.second);
    }

    for(auto username : usernames)
    {
        mAccountMap.erase(username);
    }

    return loggedOut;
}

void AccountManager::Cleanup(const std::shared_ptr<objects::AccountLogin>& login)
{
    auto cLogin = login->GetCharacterLogin();
    cLogin->SetStatus(objects::CharacterLogin::Status_t::OFFLINE);
    cLogin->SetWorldID(-1);
    cLogin->SetChannelID(-1);
    cLogin->SetZoneID(0);
    Cleanup<objects::Character>(cLogin->GetCharacter().Get());
    Cleanup<objects::Account>(login->GetAccount().Get());
}
