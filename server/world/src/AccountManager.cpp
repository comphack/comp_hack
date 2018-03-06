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

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>

// object Includes
#include <Account.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <EntityStats.h>
#include <FriendSettings.h>
#include <WorldConfig.h>

// world Includes
#include <WorldServer.h>

using namespace world;

AccountManager::AccountManager(const std::weak_ptr<WorldServer>& server)
    : mServer(server)
{
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

bool AccountManager::LobbyLogin(std::shared_ptr<objects::AccountLogin> login)
{
    bool result = false;

    libcomp::String lookup = login->GetAccount()->GetUsername().ToLower();

    std::lock_guard<std::mutex> lock(mLock);
    auto pair = mAccountMap.find(lookup);

    if(mAccountMap.end() == pair)
    {
        login->SetSessionKey(RNG(uint32_t, 1, (uint32_t)0x7FFFFFFF));

        auto res = mAccountMap.insert(std::make_pair(lookup, login));

        // This pair is the iterator (first) and a bool indicating it was
        // inserted into the map (second).
        result = res.second;
    }

    return result;
}

bool AccountManager::ChannelLogin(std::shared_ptr<objects::AccountLogin> login)
{
    auto server = mServer.lock();
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();

    auto cLogin = login->GetCharacterLogin();
    auto character = cLogin->GetCharacter().Get();
    auto account = login->LoadAccount(worldDB);

    uint32_t lastLogin = character->GetLastLogin();
    auto now = std::time(0);

    // Get the beginning of today (UTC)
    std::tm localTime = *std::localtime(&now);
    localTime.tm_hour = 0;
    localTime.tm_min = 0;
    localTime.tm_sec = 0;

    uint32_t today = (uint32_t)std::mktime(&localTime);
    if(today > lastLogin)
    {
        // This is the character's first login of the day, increase
        // their login points
        auto stats = character->LoadCoreStats(worldDB);

        if(stats->GetLevel() > 0)
        {
            int32_t points = character->GetLoginPoints();
            points = points + (int32_t)ceil((float)stats->GetLevel() * 0.2);
            character->SetLoginPoints(points);

            // If the character is in a clan, queue up a recalculation of
            // the clan level and sending of the character updates
            if(cLogin->GetClanID())
            {
                server->QueueWork([](std::shared_ptr<WorldServer> pServer,
                    std::shared_ptr<objects::CharacterLogin> pLogin, int32_t pClanID)
                {
                    auto characterManager = pServer->GetCharacterManager();
                    characterManager->SendClanMemberInfo(pLogin);
                    characterManager->RecalculateClanLevel(pClanID);
                }, server, cLogin, cLogin->GetClanID());
            }
        }
    }

    character->SetLastLogin((uint32_t)now);
    account->SetLastLogin((uint32_t)now);

    if(!character->Update(worldDB) || !account->Update(lobbyDB))
    {
        LOG_ERROR(libcomp::String("Failed to update character and account"
            " during channel login request for account: %1.\n")
            .Arg(account->GetUsername()));
        return false;
    }

    // Now that the login actions are complete, update the account and character
    // states
    std::lock_guard<std::mutex> lock(mLock);

    login->SetState(objects::AccountLogin::State_t::CHANNEL);
    cLogin->SetWorldID((int8_t)server->GetRegisteredWorld()->GetID());
    cLogin->SetStatus(objects::CharacterLogin::Status_t::ONLINE);

    return true;
}

bool AccountManager::SwitchChannel(std::shared_ptr<
    objects::AccountLogin> login, int8_t channelID)
{
    auto username = login->GetAccount()->GetUsername();

    std::lock_guard<std::mutex> lock(mLock);
    if(login->GetState() != objects::AccountLogin::State_t::CHANNEL)
    {
        LOG_ERROR(libcomp::String("Channel switch for account '%1' failed "
            "because it is not in the channel state.\n")
            .Arg(username));
        return false;
    }

    PushChannelSwitch(username, channelID);

    auto cLogin = login->GetCharacterLogin();

    // Mark the expected location for when the connection returns
    cLogin->SetChannelID(channelID);

    // Set the session key now but only update the lobby if the channel
    // switch actually occurs
    UpdateSessionKey(login);

    // Update the state regardless of if the channel honors its own request
    // so the timeout can occur
    login->SetState(objects::AccountLogin::State_t::CHANNEL_TO_CHANNEL);

    auto server = mServer.lock();
    auto config = std::dynamic_pointer_cast<objects::WorldConfig>(
        server->GetConfig());

    // Schedule channel switch timeout
    server->GetTimerManager()->ScheduleEventIn(static_cast<int>(
        config->GetChannelConnectionTimeOut()), [server]
        (const std::shared_ptr<WorldServer> pServer,
            const libcomp::String& pUsername, uint32_t pKey)
    {
        pServer->GetAccountManager()->ExpireSession(pUsername, pKey);
    }, server, login->GetAccount()->GetUsername(), login->GetSessionKey());

    return true;
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
    LOG_DEBUG(libcomp::String("Logging out user: '%1'\n").Arg(username));

    std::shared_ptr<objects::AccountLogin> result;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mLock);
    auto pair = mAccountMap.find(lookup);

    if (mAccountMap.end() != pair && (channel == -1 ||
        channel == pair->second->GetCharacterLogin()->GetChannelID()))
    {
        result = pair->second;
        Cleanup(result);
        mAccountMap.erase(pair);

        auto cLogin = result->GetCharacterLogin();

        if(cLogin && !cLogin->GetCharacter().IsNull())
        {
            auto server = mServer.lock();
            auto characterManager = server->GetCharacterManager();

            characterManager->PartyLeave(cLogin, nullptr, true);

            // Notify existing players
            std::list<std::shared_ptr<objects::CharacterLogin>> cLogOuts;
            cLogOuts.push_back(cLogin);

            characterManager->SendStatusToRelatedCharacters(cLogOuts,
                (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_BASIC);

            // Notify the lobby
            libcomp::Packet lobbyMessage;
            lobbyMessage.WritePacketCode(
                InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
            lobbyMessage.WriteString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, username);
            server->GetLobbyConnection()->SendPacket(lobbyMessage);
        }
    }

    return result;
}

bool AccountManager::ExpireSession(const libcomp::String& username,
    uint32_t key)
{
    bool expire = false;
    std::shared_ptr<objects::AccountLogin> login;

    libcomp::String lookup = username.ToLower();
    {
        std::lock_guard<std::mutex> lock(mLock);

        auto pair = mAccountMap.find(lookup);

        if(mAccountMap.end() != pair)
        {
            login = pair->second;

            // Check the account is waiting and matches the session key.
            if(login && objects::AccountLogin::State_t::CHANNEL !=
                login->GetState() && key == login->GetSessionKey())
            {
                LOG_DEBUG(libcomp::String("Session for username '%1' has "
                    "expired.\n").Arg(username));
                expire = true;
            }
        }
    }

    if(expire)
    {
        // Perform log out from any channel
        auto cLogin = login->GetCharacterLogin();
        if(cLogin)
        {
            mServer.lock()->GetCharacterManager()
                ->RequestChannelDisconnect(cLogin->GetWorldCID());
        }

        LogoutUser(username, -1);
    }

    return expire;
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

void AccountManager::UpdateSessionKey(std::shared_ptr<objects::AccountLogin> login)
{
    login->SetSessionKey(RNG(uint32_t, 1, (uint32_t)0x7FFFFFFF));
}

void AccountManager::PushChannelSwitch(const libcomp::String& username, int8_t channel)
{
    libcomp::String lookup = username.ToLower();
    mChannelSwitches[lookup] = channel;
}

bool AccountManager::PopChannelSwitch(const libcomp::String& username, int8_t& channel)
{
    libcomp::String lookup = username.ToLower();
    std::lock_guard<std::mutex> lock(mLock);

    auto it = mChannelSwitches.find(lookup);
    if(it != mChannelSwitches.end())
    {
        channel = it->second;
        mChannelSwitches.erase(it);
        return true;
    }

    return false;
}

void AccountManager::Cleanup(const std::shared_ptr<objects::AccountLogin>& login)
{
    auto cLogin = login->GetCharacterLogin();
    cLogin->SetStatus(objects::CharacterLogin::Status_t::OFFLINE);
    cLogin->SetWorldID(-1);
    cLogin->SetChannelID(-1);
    cLogin->SetZoneID(0);

    // Leave the character once loaded but drop other data referenced by it
    Cleanup<objects::FriendSettings>(cLogin->GetCharacter()
        ->GetFriendSettings().Get());
    Cleanup<objects::Account>(login->GetAccount().Get());
}
