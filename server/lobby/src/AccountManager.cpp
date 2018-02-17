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

#include "AccountManager.h"

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>
#include <ServerConstants.h>

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

AccountManager::AccountManager(LobbyServer *pServer) : mServer(pServer)
{
}

ErrorCodes_t AccountManager::WebAuthLogin(const libcomp::String& username,
    const libcomp::String& password, uint32_t clientVersion,
    libcomp::String& sid)
{
    /// @todo Check if the server is full and return SERVER_FULL.

    LOG_DEBUG(libcomp::String("Attempting to perform a web auth login for "
        "account '%1'.\n").Arg(username));

    // Trust nothing.
    if(!mServer)
    {
        LOG_DEBUG(libcomp::String("Web auth login for account '%1' failed "
            "with a system error.\n").Arg(username));

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Get the server config object.
    auto config = std::dynamic_pointer_cast<objects::LobbyConfig>(
        mServer->GetConfig());

    if(!config)
    {
        LOG_DEBUG(libcomp::String("Web auth login for account '%1' failed "
            "with a system error.\n").Arg(username));

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Get the client version required for login.
    uint32_t requiredClientVersion = static_cast<uint32_t>(
        config->GetClientVersion() * 1000.0f);

    // Check the client version first.
    if(requiredClientVersion != clientVersion)
    {
        LOG_DEBUG(libcomp::String("Web auth login for account '%1' failed "
            "with a wrong client version. Expected version %2.%3 but "
            "got version %4.%5.\n").Arg(username).Arg(
            requiredClientVersion / 1000).Arg(
            requiredClientVersion % 1000).Arg(
            clientVersion / 1000).Arg(clientVersion % 1000));

        return ErrorCodes_t::WRONG_CLIENT_VERSION;
    }

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LOG_DEBUG(libcomp::String("Web auth login for account '%1' failed "
            "with a system error.\n").Arg(username));

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Get the account database entry.
    auto account = login->GetAccount();

    // If the account was not loaded it's a bad username.
    if(!account)
    {
        LOG_DEBUG(libcomp::String("Web auth login for account '%1' failed "
            "with a bad username (no account data found).\n").Arg(username));

        // Remove the entry to save memory (esp. if someone is being a dick).
        EraseLogin(username);

        return ErrorCodes_t::BAD_USERNAME_PASSWORD;
    }

    // Get the account login state as we will need it in a second.
    auto state = login->GetState();

    // Tell them nothing about the account until they authenticate.
    if(account->GetPassword() != libcomp::Decrypt::HashPassword(password,
        account->GetSalt()))
    {
        LOG_DEBUG(libcomp::String("Web auth login for account '%1' failed "
            "with a bad password.\n").Arg(username));

        // Only erase the login if it was offline. This should prevent
        // a malicious user from blocking/corrupting a legitimate login.
        if(objects::AccountLogin::State_t::OFFLINE == state)
        {
            EraseLogin(username);
        }

        return ErrorCodes_t::BAD_USERNAME_PASSWORD;
    }

    // Now check to see if the account is already online. We will accept
    // a re-submit of the web authentication. In this case the most recent
    // submission and session ID will be used for authentication.
    if(objects::AccountLogin::State_t::OFFLINE != state &&
        objects::AccountLogin::State_t::LOBBY_WAIT != state)
    {
        LOG_DEBUG(libcomp::String("Web auth login for account '%1' failed "
            "because it is already online.\n").Arg(username));

        // Do not erase the login as it's not ours.
        return ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN;
    }

    // Now that we know the account is not online check it is enabled.
    if(!account->GetEnabled() || account->GetIsBanned())
    {
        LOG_DEBUG(libcomp::String("Web auth login for account '%1' failed "
            "due to being disabled/banned.\n").Arg(username));

        // The hammer of justice is swift.
        EraseLogin(username);

        return ErrorCodes_t::ACCOUNT_DISABLED;
    }

    // We are now ready. Generate the session ID and transition login state.
    sid = libcomp::Decrypt::GenerateRandom(300).ToLower();
    login->SetState(objects::AccountLogin::State_t::LOBBY_WAIT);
    login->SetSessionID(sid);

    // Set the session to expire.
    mServer->GetTimerManager()->ScheduleEventIn(static_cast<int>(
        SVR_CONST.WEBAUTH_TIMEOUT), [this](const libcomp::String& _username,
        const libcomp::String& _sid)
    {
        ExpireSession(_username, _sid);
    }, username, sid);

    LOG_DEBUG(libcomp::String("Web auth login for account '%1' has "
            "now passed web authentication.\n").Arg(username));

    return ErrorCodes_t::SUCCESS;
}

ErrorCodes_t AccountManager::LobbyLogin(const libcomp::String& username,
    const libcomp::String& sid, libcomp::String& sid2)
{
    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LOG_DEBUG(libcomp::String("ogin (via web auth) for account '%1' "
            "failed with a system error.\n").Arg(username));

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // The provided SID must match the one given by the server.
    if(sid != login->GetSessionID())
    {
        LOG_DEBUG(libcomp::String("Login (via web auth) for account '%1' "
            "failed because it did not provide a correct SID.\n").Arg(
            username));

        return ErrorCodes_t::BAD_USERNAME_PASSWORD;
    }

    // For web authentication we must be in the lobby wait state.
    if(objects::AccountLogin::State_t::LOBBY_WAIT != login->GetState())
    {
        LOG_DEBUG(libcomp::String("Login (via web auth) for account '%1' "
            "failed because it did not request web auth.\n").Arg(username));

        return ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN;
    }

    // We are now ready. Generate the session ID and transition to logged in.
    sid2 = libcomp::Decrypt::GenerateRandom(300).ToLower();
    login->SetState(objects::AccountLogin::State_t::LOBBY);
    login->SetSessionID(sid2);

    return ErrorCodes_t::SUCCESS;
}

ErrorCodes_t AccountManager::LobbyLogin(const libcomp::String& username,
    libcomp::String& sid2)
{
    /// @todo Check if the server is full and return SERVER_FULL.

    // We assume here the login code has checked the client version and
    // password hash. We still check if the account can login though.
    LOG_DEBUG(libcomp::String("Attempting to perform a classic login for "
        "account '%1'.\n").Arg(username));

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LOG_DEBUG(libcomp::String("ogin (via web auth) for account '%1' "
            "failed with a system error.\n").Arg(username));

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Get the account database entry.
    auto account = login->GetAccount();

    // If the account was not loaded it's a bad username.
    if(!account)
    {
        LOG_DEBUG(libcomp::String("Classic login for account '%1' failed "
            "with a bad username (no account data found).\n").Arg(username));

        // Remove the entry to save memory (esp. if someone is being a dick).
        EraseLogin(username);

        return ErrorCodes_t::BAD_USERNAME_PASSWORD;
    }

    // Now check to see if the account is already online.
    if(objects::AccountLogin::State_t::OFFLINE != login->GetState())
    {
        LOG_DEBUG(libcomp::String("Classic login for account '%1' failed "
            "because it is already online.\n").Arg(username));

        // Do not erase the login as it's not ours.
        return ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN;
    }

    // Now that we know the account is not online check it is enabled.
    if(!account->GetEnabled() || account->GetIsBanned())
    {
        LOG_DEBUG(libcomp::String("Classic login for account '%1' failed "
            "due to being disabled/banned.\n").Arg(username));

        // The hammer of justice is swift.
        EraseLogin(username);

        return ErrorCodes_t::ACCOUNT_DISABLED;
    }

    // We are now ready. Generate the session ID and transition to logged in.
    sid2 = libcomp::Decrypt::GenerateRandom(300).ToLower();
    login->SetState(objects::AccountLogin::State_t::LOBBY);
    login->SetSessionID(sid2);

    return ErrorCodes_t::SUCCESS;
}

void AccountManager::LogoutUser(const libcomp::String& username)
{
    LOG_DEBUG(libcomp::String("Logging out account '%1'.\n").Arg(username));

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Remove the entry to save memory. If it has to be created again the
    // account will be in the OFFLINE state.
    EraseLogin(username);
}

void AccountManager::ExpireSession(const libcomp::String& username,
    const libcomp::String& sid)
{
    // Convert the username to lowercase for lookup.
    libcomp::String lookup = username.ToLower();

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Look for the account in the map.
    auto pair = mAccountMap.find(lookup);

    // If it's there we have a previous login attempt.
    if(mAccountMap.end() != pair)
    {
        auto account = pair->second;

        // Check the account is waiting and matches the session ID.
        if(account && objects::AccountLogin::State_t::LOBBY_WAIT ==
            account->GetState() && sid == account->GetSessionID())
        {
            LOG_DEBUG(libcomp::String("Session for username '%1' has "
                "expired.\n").Arg(username));

            // It's still set to expire so do so.
            mAccountMap.erase(pair);
        }
    }
}

std::shared_ptr<objects::AccountLogin> AccountManager::GetOrCreateLogin(
    const libcomp::String& username)
{
    std::shared_ptr<objects::AccountLogin> login;

    // Convert the username to lowercase for lookup.
    libcomp::String lookup = username.ToLower();

    // Look for the account in the map.
    auto pair = mAccountMap.find(lookup);

    // If it's there we have a previous login attempt.
    if(mAccountMap.end() == pair)
    {
        // Create a new login object.
        login = std::shared_ptr<objects::AccountLogin>(
            new objects::AccountLogin);

        auto res = mAccountMap.insert(std::make_pair(lookup, login));

        // This pair is the iterator (first) and a bool indicating it was
        // inserted into the map (second).
        if(!res.second || !mServer)
        {
            login.reset();
        }
        else
        {
            // Load the account from the database and set the initial state
            // to offline.
            login->SetState(objects::AccountLogin::State_t::OFFLINE);
            login->SetAccount(objects::Account::LoadAccountByUsername(
                mServer->GetMainDatabase(), lookup));
        }
    }
    else
    {
        // Return the existing login object.
        login = pair->second;
    }

    return login;
}

void AccountManager::EraseLogin(const libcomp::String& username)
{
    // Convert the username to lowercase for lookup.
    libcomp::String lookup = username.ToLower();

    mAccountMap.erase(lookup);
}

bool AccountManager::IsLoggedIn(const libcomp::String& username)
{
    bool result = false;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    LOG_DEBUG(libcomp::String("Looking for account '%1'\n").Arg(username));

    PrintAccounts();

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

    LOG_DEBUG(libcomp::String("Looking for account '%1'\n").Arg(username));

    PrintAccounts();

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

        if(nullptr == login)
        {
            LOG_DEBUG(libcomp::String("Logged in account '%1' with new object.\n"));
        }
        else
        {
            LOG_DEBUG(libcomp::String("Logged in account '%1' with old object.\n"));
        }
    }
    else
    {
        LOG_DEBUG(libcomp::String("Failed to login account '%1' because it "
            "is already logged in.\n").Arg(username));
    }

    PrintAccounts();

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

    if(mAccountMap.end() != pair)
    {
        pair->second->SetSessionID(sid);

        LOG_DEBUG(libcomp::String("Updated session ID for account "
            "'%1' to %2\n").Arg(username).Arg(sid));

        result = true;
    }
    else
    {
        LOG_DEBUG(libcomp::String("Failed to update session ID for "
            "account '%1' to %2\n").Arg(username).Arg(sid));
    }

    PrintAccounts();

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

        LOG_DEBUG(libcomp::String("Logged out account '%1'\n").Arg(username));

        result = true;
    }
    else
    {
        LOG_DEBUG(libcomp::String("Account '%1' is not logged in so it "
            "was not logged out.\n").Arg(username));
    }

    PrintAccounts();

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

void AccountManager::PrintAccounts() const
{
    LOG_DEBUG("----------------------------------------\n");

    for(auto a : mAccountMap)
    {
        auto login = a.second;

        libcomp::String state;

        switch(login->GetState())
        {
            case objects::AccountLogin::State_t::OFFLINE:
                state = "OFFLINE";
                break;
            case objects::AccountLogin::State_t::LOBBY_WAIT:
                state = "LOBBY_WAIT";
                break;
            case objects::AccountLogin::State_t::LOBBY:
                state = "LOBBY";
                break;
            case objects::AccountLogin::State_t::LOBBY_TO_CHANNEL:
                state = "LOBBY_TO_CHANNEL";
                break;
            case objects::AccountLogin::State_t::CHANNEL_TO_LOBBY:
                state = "CHANNEL_TO_LOBBY";
                break;
            case objects::AccountLogin::State_t::CHANNEL:
                state = "CHANNEL";
                break;
            case objects::AccountLogin::State_t::CHANNEL_TO_CHANNEL:
                state = "CHANNEL_TO_CHANNEL";
                break;
            default:
                state = "ERROR";
                break;
        }

        LOG_DEBUG(libcomp::String("Account:     %1\n").Arg(a.first));
        LOG_DEBUG(libcomp::String("State:       %1\n").Arg(state));
        LOG_DEBUG(libcomp::String("Session ID:  %1\n").Arg(login->GetSessionID()));
        LOG_DEBUG(libcomp::String("Session Key: %1\n").Arg(login->GetSessionKey()));
        LOG_DEBUG("----------------------------------------\n");
    }
}
