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
#include <Crypto.h>
#include <Log.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <ctime>

// objects Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <LobbyConfig.h>
#include <WebGameSession.h>

// lobby Includes
#include "ApiHandler.h"
#include "LobbyServer.h"
#include "LobbySyncManager.h"
#include "World.h"

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif // HAVE_SYSTEMD

using namespace lobby;

AccountManager::AccountManager(LobbyServer *pServer) : mServer(pServer)
{
}

ErrorCodes_t AccountManager::WebAuthLogin(const libcomp::String& username,
    const libcomp::String& password, uint32_t clientVersion,
    libcomp::String& sid, bool checkPassword)
{
    /// @todo Check if the server is full and return SERVER_FULL.

    LogAccountManagerDebug([&]()
    {
        return libcomp::String("Attempting to perform a web auth login for "
            "account '%1'.\n").Arg(username);
    });

    // Trust nothing.
    if(!mServer)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Web auth login for account '%1' failed "
                "with a system error.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Get the server config object.
    auto config = std::dynamic_pointer_cast<objects::LobbyConfig>(
        mServer->GetConfig());

    if(!config)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Web auth login for account '%1' failed "
                "with a system error.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Get the client version required for login.
    uint32_t requiredClientVersion = static_cast<uint32_t>(
        config->GetClientVersion() * 1000.0f + 0.5f);

    // Check the client version first.
    if(requiredClientVersion != clientVersion)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Web auth login for account '%1' failed "
                "with a wrong client version. Expected version %2.%3 but "
                "got version %4.%5.\n").Arg(username).Arg(
                requiredClientVersion / 1000).Arg(
                requiredClientVersion % 1000).Arg(
                clientVersion / 1000).Arg(clientVersion % 1000);
        });

        return ErrorCodes_t::WRONG_CLIENT_VERSION;
    }

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Web auth login for account '%1' failed "
                "with a system error.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Get the account database entry.
    auto account = login->GetAccount();

    // If the account was not loaded it's a bad username.
    if(!account)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Web auth login for account '%1' failed "
                "with a bad username (no account data found).\n").Arg(username);
        });

        // Remove the entry to save memory (esp. if someone is being a dick).
        EraseLogin(username);

        return ErrorCodes_t::BAD_USERNAME_PASSWORD;
    }

    // Get the account login state as we will need it in a second.
    auto state = login->GetState();

    // The API version of this function does not have to check the password.
    if(checkPassword)
    {
        // Tell them nothing about the account until they authenticate.
        if(account->GetPassword() != libcomp::Crypto::HashPassword(password,
            account->GetSalt()))
        {
            LogAccountManagerDebug([&]()
            {
                return libcomp::String("Web auth login for account '%1' failed "
                    "with a bad password.\n").Arg(username);
            });

            // Only erase the login if it was offline. This should prevent
            // a malicious user from blocking/corrupting a legitimate login.
            if(objects::AccountLogin::State_t::OFFLINE == state)
            {
                EraseLogin(username);
            }

            return ErrorCodes_t::BAD_USERNAME_PASSWORD;
        }
    }

    // Now check to see if the account is already online. We will accept
    // a re-submit of the web authentication. In this case the most recent
    // submission and session ID will be used for authentication.
    if(objects::AccountLogin::State_t::OFFLINE != state &&
        objects::AccountLogin::State_t::LOBBY_WAIT != state)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Web auth login for account '%1' failed "
                "because it is already online.\n").Arg(username);
        });

        // Do not erase the login as it's not ours.
        return ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN;
    }

    // Now that we know the account is not online check it is enabled.
    if(!account->GetEnabled())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Web auth login for account '%1' failed "
                "due to being disabled/banned.\n").Arg(username);
        });

        // The hammer of justice is swift.
        EraseLogin(username);

        return ErrorCodes_t::ACCOUNT_DISABLED;
    }

    // We are now ready. Generate the session ID and transition login state.
    sid = libcomp::Crypto::GenerateRandom(300).ToLower();
    login->SetState(objects::AccountLogin::State_t::LOBBY_WAIT);
    login->SetSessionID(sid);

    // Set the session to expire.
    mServer->GetTimerManager()->ScheduleEventIn(static_cast<int>(
        config->GetWebAuthTimeOut()), [this](const libcomp::String& _username,
        const libcomp::String& _sid)
    {
        ExpireSession(_username, _sid);
    }, username, sid);

    LogAccountManagerDebug([&]()
    {
        return libcomp::String("Web auth login for account '%1' has "
            "now passed web authentication.\n").Arg(username);
    });

    return ErrorCodes_t::SUCCESS;
}

ErrorCodes_t AccountManager::WebAuthLoginApi(const libcomp::String& username,
    uint32_t clientVersion, libcomp::String& sid)
{
    return WebAuthLogin(username, libcomp::String(),
        clientVersion, sid, false);
}

ErrorCodes_t AccountManager::LobbyLogin(const libcomp::String& username,
    const libcomp::String& sid, libcomp::String& sid2)
{
    LogAccountManagerDebug([&]()
    {
        return libcomp::String("Attempting to perform a login with SID for "
            "account '%1'.\n").Arg(username);
    });

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login (via web auth) for account '%1' "
                "failed with a system error.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // The provided SID must match the one given by the server.
    if(sid != login->GetSessionID())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login (via web auth) for account '%1' "
                "failed because it did not provide a correct SID.\n")
                .Arg(username);
        });

        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Theirs: %1\n").Arg(sid);
        });

        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Ours:   %1\n").Arg(login->GetSessionID());
        });

        return ErrorCodes_t::BAD_USERNAME_PASSWORD;
    }

    // For web authentication we must be in the lobby wait state.
    if(objects::AccountLogin::State_t::LOBBY_WAIT != login->GetState())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login (via web auth) for account '%1' "
                "failed because it did not request web auth.\n").Arg(username);
        });

        return ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN;
    }

    // We are now ready. Generate the session ID and transition to logged in.
    sid2 = libcomp::Crypto::GenerateRandom(300).ToLower();
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
    LogAccountManagerDebug([&]()
    {
        return libcomp::String("Attempting to perform a classic login for "
            "account '%1'.\n").Arg(username);
    });

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login (via web auth) for account '%1' "
                "failed with a system error.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Get the account database entry.
    auto account = login->GetAccount();

    // If the account was not loaded it's a bad username.
    if(!account)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Classic login for account '%1' failed "
                "with a bad username (no account data found).\n").Arg(username);
        });

        // Remove the entry to save memory (esp. if someone is being a dick).
        EraseLogin(username);

        return ErrorCodes_t::BAD_USERNAME_PASSWORD;
    }

    // Get the account login state as we will need it in a second.
    auto state = login->GetState();

    // Now check to see if the account is already online.
    if(objects::AccountLogin::State_t::OFFLINE != state &&
        objects::AccountLogin::State_t::LOBBY_WAIT != state)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Classic login for account '%1' failed "
                "because it is already online.\n").Arg(username);
        });

        // Do not erase the login as it's not ours.
        return ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN;
    }

    // Now that we know the account is not online check it is enabled.
    if(!account->GetEnabled())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Classic login for account '%1' failed "
                "due to being disabled/banned.\n").Arg(username);
        });

        // The hammer of justice is swift.
        EraseLogin(username);

        return ErrorCodes_t::ACCOUNT_DISABLED;
    }

    // We are now ready. Generate the session ID and transition to logged in.
    sid2 = libcomp::Crypto::GenerateRandom(300).ToLower();
    login->SetState(objects::AccountLogin::State_t::LOBBY);
    login->SetSessionID(sid2);

    return ErrorCodes_t::SUCCESS;
}

std::shared_ptr<objects::AccountLogin> AccountManager::StartChannelLogin(
    const libcomp::String& username,
    const std::shared_ptr<objects::Character>& character)
{
    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login to channel for account '%1' "
                "failed with a system error.\n").Arg(username);
        });

        return {};
    }

    // Now check to see if the account is online.
    if(objects::AccountLogin::State_t::LOBBY != login->GetState())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login to channel for account '%1' failed "
                "because it is not in the lobby state.\n").Arg(username);
        });

        return {};
    }

    auto cLogin = login->GetCharacterLogin();
    cLogin->SetCharacter(character);

    return login;
}

ErrorCodes_t AccountManager::SwitchToChannel(const libcomp::String& username,
    int8_t worldID, int8_t channelID)
{
    LogAccountManagerDebug([&]()
    {
        return libcomp::String("Attempting to perform a login to channel %1 "
            "on world %2 for account '%3'.\n").Arg(channelID).Arg(
            worldID).Arg(username);
    });

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login to channel for account '%1' "
                "failed with a system error.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Now check to see if the account is online.
    if(objects::AccountLogin::State_t::LOBBY != login->GetState())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login to channel for account '%1' failed "
                "because it is not in the lobby state.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Update the state of the login.
    login->SetState(objects::AccountLogin::State_t::LOBBY_TO_CHANNEL);

    auto cLogin = login->GetCharacterLogin();
    cLogin->SetWorldID(worldID);
    cLogin->SetChannelID(channelID);

    return ErrorCodes_t::SUCCESS;
}

ErrorCodes_t AccountManager::CompleteChannelLogin(
    const libcomp::String& username, int8_t worldID, int8_t channelID)
{
    LogAccountManagerDebug([&]()
    {
        return libcomp::String("Attempting to complete a login to channel %1 "
            "on world %2 for account '%3'.\n").Arg(channelID).Arg(
            worldID).Arg(username);
    });

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login to channel for account '%1' "
                "failed with a system error.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Now check to see if the account is online.
    if(objects::AccountLogin::State_t::LOBBY_TO_CHANNEL != login->GetState() &&
        objects::AccountLogin::State_t::CHANNEL_TO_CHANNEL != login->GetState())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login to channel for account '%1' failed "
                "because it is not in the lobby to channel or channel to "
                "channel state.\n").Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    auto cLogin = login->GetCharacterLogin();

    // Check the world and channel match.
    if(cLogin->GetWorldID() != worldID ||
        cLogin->GetChannelID() != channelID)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Login to channel for account '%1' failed "
                "because the completion is for a different world or channel.\n"
                ).Arg(username);
        });

        return ErrorCodes_t::SYSTEM_ERROR;
    }

    // Update the state of the login.
    login->SetState(objects::AccountLogin::State_t::CHANNEL);

    return ErrorCodes_t::SUCCESS;
}

bool AccountManager::ChannelToChannelSwitch(const libcomp::String& username,
    int8_t channelID, uint32_t sessionKey)
{
    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen.
    if(!login)
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Channel to channel switch for account "
                "'%1' failed with a system error.\n").Arg(username);
        });

        return false;
    }

    auto cLogin = login->GetCharacterLogin();

    if(!cLogin || objects::AccountLogin::State_t::CHANNEL != login->GetState())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Channel to channel for account '%1' failed "
                "because it is not in the channel state with a valid "
                "character.\n").Arg(username);
        });

        return false;
    }

    cLogin->SetChannelID(channelID);
    login->SetSessionKey(sessionKey);

    // Always clear the web-game session
    mWebGameSessions.erase(username.ToLower());
    mWebGameAPISessions.erase(username.ToLower());

    // Set channel to channel state but do not set expiration as the world is
    // responsible for completing this connection or disconnecting on timeout
    login->SetState(objects::AccountLogin::State_t::CHANNEL_TO_CHANNEL);

    return true;
}

bool AccountManager::Logout(const libcomp::String& username)
{
    LogAccountManagerDebug([&]()
    {
        return libcomp::String("Logging out account '%1'.\n").Arg(username);
    });

    auto config = std::dynamic_pointer_cast<objects::LobbyConfig>(
        mServer->GetConfig());

    // Lock the accounts now so this is thread safe.
    std::lock_guard<std::mutex> lock(mAccountLock);

    // Get the login object for this username.
    auto login = GetOrCreateLogin(username);

    // This should never happen but if it does ignore it.
    if(!login)
    {
        return false;
    }

    // If the account is offline ignore this logout.
    if(objects::AccountLogin::State_t::OFFLINE == login->GetState())
    {
        // Remove the entry to save memory.
        EraseLogin(username);

        return false;
    }
    else
    {
        // Always clear the web-game session
        mWebGameSessions.erase(username.ToLower());
        mWebGameAPISessions.erase(username.ToLower());
    }

    // Set the session to expire.
    mServer->GetTimerManager()->ScheduleEventIn(static_cast<int>(
        config->GetWebAuthTimeOut()), [this](const libcomp::String& _username,
        const libcomp::String& _sid)
    {
        ExpireSession(_username, _sid);
    }, username, login->GetSessionID());

    // Reset the character information
    auto cLogin = login->GetCharacterLogin();
    cLogin->SetCharacter(NULLUUID);
    cLogin->SetWorldID(-1);
    cLogin->SetChannelID(-1);
    cLogin->SetZoneID(0);

    // Let the account return to the lobby (if they did a logout to lobby).
    login->SetState(objects::AccountLogin::State_t::LOBBY_WAIT);

    return true;
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
            LogAccountManagerDebug([&]()
            {
                return libcomp::String("Session for username '%1' has "
                    "expired.\n").Arg(username);
            });

            // It's still set to expire so do so.
            mAccountMap.erase(pair);

            UpdateDebugStatus();
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

        UpdateDebugStatus();

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
    mWebGameSessions.erase(lookup);
    mWebGameAPISessions.erase(lookup);

    UpdateDebugStatus();
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

std::shared_ptr<objects::AccountLogin> AccountManager::GetUserLogin(
    const libcomp::String& username)
{
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto pair = mAccountMap.find(lookup);
    return pair != mAccountMap.end() ? pair->second : nullptr;
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

    UpdateDebugStatus();

    return usernames;
}

bool AccountManager::UpdateKillTime(const libcomp::String& username,
    uint8_t cid, std::shared_ptr<LobbyServer>& server)
{
    auto config = std::dynamic_pointer_cast<objects::LobbyConfig>(
        server->GetConfig());

    auto login = GetUserLogin(username);
    if(!login)
    {
        return false;
    }

    auto account = login->GetAccount().Get();
    auto character = account->GetCharacters(cid).Get();
    if(character)
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
                return DeleteCharacter(account, character);
            }
        }

        if(!character->Update(worldDB))
        {
            LogAccountManagerDebugMsg("Character kill time failed to save.\n");

            return false;
        }

        return true;
    }

    return false;
}

bool AccountManager::DeleteKillTimeExceededCharacters(uint8_t worldID)
{
    auto world = mServer->GetWorldByID(worldID);
    if(!world)
    {
        return false;
    }

    uint32_t now = (uint32_t)std::time(0);
    auto svr = world->GetRegisteredWorld();
    auto mainDB = mServer->GetMainDatabase();
    auto worldDB = world->GetWorldDatabase();

    LOG_DEBUG(libcomp::String("Loading kill time exceeded characters for"
        " world server: (%1) %2\n").Arg(svr->GetID()).Arg(svr->GetName()));

    std::list<std::shared_ptr<objects::Character>> exceeded;
    for(auto character : libcomp::PersistentObject::LoadAll<
        objects::Character>(worldDB))
    {
        if(character->GetKillTime() && character->GetKillTime() < now)
        {
            exceeded.push_back(character);
        }
    }

    if(exceeded.size() > 0)
    {
        LOG_DEBUG(libcomp::String("%1 character(s) found for deletion\n")
            .Arg(exceeded.size()));

        while(exceeded.size() > 0)
        {
            auto accountUID = exceeded.front()->GetAccount();

            std::list<std::shared_ptr<objects::Character>> subset;
            exceeded.remove_if([accountUID, &subset](
                const std::shared_ptr<objects::Character>& c)
                {
                    // Who says you ONLY have to remove here? :P
                    if(c->GetAccount() == accountUID)
                    {
                        subset.push_back(c);
                    }

                    return c->GetAccount() == accountUID;
                });

            auto account = libcomp::PersistentObject::LoadObjectByUUID<
                objects::Account>(mainDB, accountUID);
            if(account)
            {
                for(auto character : subset)
                {
                    DeleteCharacter(account, character);
                }
            }
            else
            {
                LOG_DEBUG(libcomp::String("Failed to load account %1"
                    " associated to kill time exceeded character(s)\n")
                    .Arg(accountUID.ToString()));
            }
        }

        LOG_DEBUG("Character deletions complete\n");
    }
    else
    {
        LOG_DEBUG("No characters deletions required\n");
    }

    return true;
}

bool AccountManager::SetCharacterOnAccount(
    const std::shared_ptr<objects::Account>& account,
    const std::shared_ptr<objects::Character>& character)
{
    // We need to be careful when creating characters so we
    // do not orphan any when inserting
    std::lock_guard<std::mutex> lock(mAccountLock);

    auto characters = account->GetCharacters();

    uint8_t nextCID = 0;
    for(; nextCID < MAX_CHARACTER; nextCID++)
    {
        if(characters[nextCID].IsNull())
        {
            // Character found
            break;
        }
    }

    if(nextCID == MAX_CHARACTER)
    {
        LogAccountManagerError([&]()
        {
            return libcomp::String("Character failed to be created on"
                " account: %1\n").Arg(account->GetUUID().ToString());
        });

        return false;
    }

    if(!account->SetCharacters(nextCID, character) ||
        !account->Update(mServer->GetMainDatabase()))
    {
        LogAccountManagerError([&]()
        {
            return libcomp::String("Account character array failed to save"
                " for account %1\n").Arg(account->GetUUID().ToString());
        });

        return false;
    }

    return true;
}

std::list<std::shared_ptr<objects::Character>>
    AccountManager::GetCharactersForDeletion(const std::shared_ptr<
        objects::Account>& account)
{
    std::list<std::shared_ptr<objects::Character>> deletes;

    auto characters = account->GetCharacters();

    time_t now = time(0);
    for(auto character : characters)
    {
        if(character.Get() && character->GetKillTime() != 0 &&
            character->GetKillTime() <= (uint32_t)now)
        {
            deletes.push_back(character.Get());
        }
    }

    return deletes;
}

bool AccountManager::DeleteCharacter(
    const std::shared_ptr<objects::Account>& account,
    const std::shared_ptr<objects::Character>& character)
{
    // We need to be careful when deleting characters so we
    // do not orphan any when reindexing etc
    std::lock_guard<std::mutex> lock(mAccountLock);

    auto characters = account->GetCharacters();

    uint8_t cid = 0;
    for(; cid < MAX_CHARACTER; cid++)
    {
        if(characters[cid].GetUUID() == character->GetUUID())
        {
            // Character found
            break;
        }
    }

    if(cid == MAX_CHARACTER)
    {
        LogAccountManagerError([&]()
        {
            return libcomp::String("Attempted to delete a character"
                " no longer associated to its parent account: %1\n").Arg(
                character->GetUUID().ToString());
        });

        return false;
    }

    // Bump all characters down the list
    for(; cid < MAX_CHARACTER; cid++)
    {
        if(cid == (size_t)(MAX_CHARACTER - 1))
        {
            characters[cid].SetReference(nullptr);
        }
        else
        {
            characters[cid] = characters[(size_t)(cid + 1)];
        }
    }

    account->SetCharacters(characters);

    // If there are no characters left make sure the account has a
    // character ticket.
    int count = 0;

    for(auto c : characters)
    {
        if(!c.IsNull())
        {
            count++;
        }
    }

    if(0 == count && 0 == account->GetTicketCount())
    {
        account->SetTicketCount(1);
    }

    if(!account->Update(mServer->GetMainDatabase()))
    {
        LogAccountManagerError([&]()
        {
            return libcomp::String("Account failed to update after character"
                " deletion: %1\n").Arg(character->GetUUID().ToString());
        });

        return false;
    }

    // Now that the account has had the character removed, send them to
    // the world to cleanup
    mServer->GetLobbySyncManager()->RemoveRecord(character, "Character");

    return true;
}

bool AccountManager::StartWebGameSession(const libcomp::String& username,
    const std::shared_ptr<objects::WebGameSession>& gameSession)
{
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);
    auto accountPair = mAccountMap.find(lookup);
    if(accountPair == mAccountMap.end())
    {
        // Not logged in
        return false;
    }

    auto sessionPair = mWebGameSessions.find(lookup);
    if(sessionPair != mWebGameSessions.end())
    {
        // Already has a session
        return false;
    }

    // Session is valid, register it
    LogAccountManagerDebug([&]()
    {
        return libcomp::String("Web-game session started for account: %1\n")
            .Arg(username);
    });

    mWebGameSessions[lookup] = gameSession;

    return true;
}

std::shared_ptr<WebGameApiSession> AccountManager::GetWebGameApiSession(
    const libcomp::String& username, const libcomp::String& sessionID,
    const libcomp::String& clientAddress)
{
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto it = mWebGameSessions.find(lookup);
    if(it != mWebGameSessions.end())
    {
        auto gameSession = it->second;
        if(gameSession->GetSessionID() == sessionID)
        {
            // Session is valid, get or create API session
            auto it2 = mWebGameAPISessions.find(lookup);
            if(it2 != mWebGameAPISessions.end())
            {
                if(it2->second->clientAddress != clientAddress)
                {
                    LogAccountManagerError([&]()
                    {
                        return libcomp::String("Second web-game session"
                            " attempted for account: %1\n").Arg(username);
                    });

                    return nullptr;
                }

                return it2->second;
            }

            auto apiSession = std::make_shared<WebGameApiSession>();
            apiSession->username = username;
            apiSession->webGameSession = gameSession;
            apiSession->clientAddress = clientAddress;

            mWebGameAPISessions[lookup] = apiSession;

            return apiSession;
        }
    }
    else
    {
        LogAccountManagerError([&]()
        {
            return libcomp::String("Web-game API session requested from account"
                " with no active web-game session: %1\n").Arg(username);
        });
    }

    return nullptr;
}

bool AccountManager::EndWebGameSession(const libcomp::String& username)
{
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mAccountLock);

    auto sessionPair = mWebGameSessions.find(lookup);
    if(sessionPair != mWebGameSessions.end())
    {
        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Web-game session ended for account: %1\n")
                .Arg(username);
        });

        mWebGameSessions.erase(lookup);
        mWebGameAPISessions.erase(lookup);
        return true;
    }

    return false;
}

void AccountManager::PrintAccounts() const
{
    LogAccountManagerDebugMsg("----------------------------------------\n");

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

        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Account:     %1\n").Arg(a.first);
        });

        LogAccountManagerDebug([&]()
        {
            return libcomp::String("State:       %1\n").Arg(state);
        });

        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Session ID:  %1\n")
                .Arg(login->GetSessionID());
        });

        LogAccountManagerDebug([&]()
        {
            return libcomp::String("Session Key: %1\n")
                .Arg(login->GetSessionKey());
        });

        LogAccountManagerDebugMsg("----------------------------------------\n");
    }
}

void AccountManager::UpdateDebugStatus() const
{
#ifdef HAVE_SYSTEMD
    sd_notifyf(0, "STATUS=Server is up with %d connected user(s).",
        (int)mAccountMap.size());
#endif // HAVE_SYSTEMD
}
