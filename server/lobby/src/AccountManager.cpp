/**
 * @file server/lobby/src/AccountManager.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to track accounts that are logged in.
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

#include "AccountManager.h"

// libcomp Includes
#include <Log.h>

// Standard C++11 Includes
#include <ctime>

// objects Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// lobby Includes
#include "LobbyServer.h"

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif // HAVE_SYSTEMD

using namespace lobby;

bool AccountManager::IsLoggedIn(const libcomp::String& username)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair)
    {
        result = true;
    }

    return result;
}

bool AccountManager::IsLoggedIn(const libcomp::String& username,
    int8_t& world)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair)
    {
        result = true;

        world = pair->second->GetCharacterLogin()->GetWorldID();
    }

    return result;
}

bool AccountManager::LoginUser(const libcomp::String& username,
    std::shared_ptr<objects::AccountLogin> login)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() == pair)
    {
        if(nullptr == login)
        {
            login = std::shared_ptr<objects::AccountLogin>(
                new objects::AccountLogin);
        }

        auto res = mAccountMap.insert(std::make_pair(lookup, login));

        // This pair is the iterator (first) and a bool indicating it was
        // inserted into the map (second).
        result = res.second;
    }

#ifdef HAVE_SYSTEMD
    sd_notifyf(0, "STATUS=Server is up with %d connected user(s).",
        (int)mAccountMap.size());
#endif // HAVE_SYSTEMD

    return result;
}

bool AccountManager::UpdateSessionID(const libcomp::String& username,
    const libcomp::String& sid)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() == pair)
    {
        pair->second->SetSessionID(sid);
        result = true;
    }

    return result;
}

std::shared_ptr<objects::AccountLogin> AccountManager::GetUserLogin(
    const libcomp::String& username)
{
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto pair = mAccountMap.find(lookup);
    return pair != mAccountMap.end() ? pair->second : nullptr;
}

bool AccountManager::LogoutUser(const libcomp::String& username, int8_t world)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() != pair && world == pair->second->
        GetCharacterLogin()->GetWorldID())
    {
        (void)mAccountMap.erase(pair);

        result = true;
    }

#ifdef HAVE_SYSTEMD
    sd_notifyf(0, "STATUS=Server is up with %d connected user(s).",
        (int)mAccountMap.size());
#endif // HAVE_SYSTEMD

    return result;
}

std::list<libcomp::String> AccountManager::LogoutUsersInWorld(int8_t world,
    int8_t channel)
{
    if(0 > world)
    {
        return std::list<libcomp::String>();
    }

    std::lock_guard<std::mutex> lock(mAccountLock);

    std::list<libcomp::String> usernames;
    for(auto pair : mAccountMap)
    {
        auto charLogin = pair.second->GetCharacterLogin();
        if(charLogin->GetWorldID() == world &&
            (channel < 0 || charLogin->GetChannelID() == channel))
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

bool AccountManager::UpdateKillTime(const libcomp::String& username,
    uint8_t cid, std::shared_ptr<LobbyServer>& server)
{
    auto config = std::dynamic_pointer_cast<objects::LobbyConfig>(
        server->GetConfig());

    auto login = GetUserLogin(username);
    auto account = login->GetAccount();
    auto character = account->GetCharacters(cid);
    if(character.Get())
    {
        auto world = server->GetWorldByID(character->GetWorldID());
        auto worldDB = world->GetWorldDatabase();

        if(character->GetKillTime() > 0)
        {
            // Clear the kill time
            character->SetKillTime(0);
        }
        else
        {
            auto deleteMinutes = config->GetCharacterDeletionDelay();
            if(deleteMinutes > 0)
            {
                // Set the kill time
                time_t now = time(0);
                character->SetKillTime(static_cast<uint32_t>(now + (deleteMinutes * 60)));
            }
            else
            {
                // Delete the character now
                return DeleteCharacter(username, cid, server);
            }
        }

        if(!character->Update(worldDB))
        {
            LOG_DEBUG("Character kill time failed to save.\n");
            return false;
        }

        return true;
    }

    return false;
}

std::list<uint8_t> AccountManager::GetCharactersForDeletion(const libcomp::String& username)
{
    std::list<uint8_t> cids;

    auto login = GetUserLogin(username);
    auto account = login->GetAccount();
    auto characters = account->GetCharacters();

    time_t now = time(0);
    for(auto character : characters)
    {
        if(character.Get() && character->GetKillTime() != 0 &&
            character->GetKillTime() <= (uint32_t)now)
        {
            cids.push_back(character->GetCID());
        }
    }

    return cids;
}

bool AccountManager::DeleteCharacter(const libcomp::String& username, uint8_t cid,
    std::shared_ptr<LobbyServer>& server)
{
    auto login = GetUserLogin(username);
    auto account = login->GetAccount();
    auto characters = account->GetCharacters();
    auto deleteCharacter = characters[cid];
    if(deleteCharacter.Get())
    {
        auto world = server->GetWorldByID(deleteCharacter->GetWorldID());
        auto worldDB = world->GetWorldDatabase();

        if(!deleteCharacter->Delete(worldDB))
        {
            LOG_ERROR(libcomp::String("Character failed to delete: %1\n").Arg(
                deleteCharacter->GetUUID().ToString()));
            return false;
        }

        characters[cid].SetReference(nullptr);
        account->SetCharacters(characters);

        if(!account->Update(server->GetMainDatabase()))
        {
            LOG_ERROR(libcomp::String("Account failed to update after character"
                " deletion: %1\n").Arg(deleteCharacter->GetUUID().ToString()));
            return false;
        }

        return true;
    }

    return false;
}
