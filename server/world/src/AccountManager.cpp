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
#include <PacketCodes.h>
#include <Randomizer.h>

// object Includes
#include <Account.h>
#include <AccountWorldData.h>
#include <ChannelLogin.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <ClanMember.h>
#include <Demon.h>
#include <DemonBox.h>
#include <EntityStats.h>
#include <Expertise.h>
#include <FriendSettings.h>
#include <Hotbar.h>
#include <InheritedSkill.h>
#include <InstanceAccess.h>
#include <Item.h>
#include <ItemBox.h>
#include <PvPData.h>
#include <Quest.h>
#include <StatusEffect.h>
#include <WebGameSession.h>
#include <WorldConfig.h>
#include <WorldSharedConfig.h>

// world Includes
#include "AccountManager.h"
#include "CharacterManager.h"
#include "WorldServer.h"
#include "WorldSyncManager.h"

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
    auto account = login->LoadAccount(lobbyDB);

    if(!character || !account)
    {
        LOG_ERROR(libcomp::String("CharacterLogin encountered with no account"
            " or character loaded: %1.\n")
            .Arg(login->GetAccount().GetUUID().ToString()));
        return false;
    }

    auto worldChanges = libcomp::DatabaseChangeSet::Create();

    uint32_t lastLogin = character->GetLastLogin();

    // Get the relative day offset
    const static int32_t timeAdjust = (std::dynamic_pointer_cast<
        objects::WorldConfig>(server->GetConfig())->GetWorldSharedConfig()
        ->GetTimeOffset() * 60) % 86400;

    // Get the relative beginning of today
    time_t now = std::time(0);
    uint32_t today = (uint32_t)(now / 86400 * 86400);
    if(timeAdjust)
    {
        // If the actual day differs, adjust accordingly
        uint32_t adjustToday = (uint32_t)((now + timeAdjust) / 86400 * 86400);
        if(adjustToday > today)
        {
            // Add a day (so we don't check midnight for "yesterday")
            today = (uint32_t)(today + 86400);
        }
        else if(adjustToday < today)
        {
            // Subtract a day (so we don't check midnight for "tomorrow")
            today = (uint32_t)(today - 86400);
        }

        today = (uint32_t)((int32_t)today - timeAdjust);
    }

    if(lastLogin && today > lastLogin)
    {
        // This is the character's first login of the day, increase
        // their login points, mark COMP demons with quests and drop GP
        auto stats = character->LoadCoreStats(worldDB);
        auto sharedConfig = std::dynamic_pointer_cast<
            objects::WorldConfig>(server->GetConfig())->GetWorldSharedConfig();

        // Count any time before today as one day
        uint32_t daysSinceLogin = ((today - lastLogin) / (24 * 60 * 60)) + 1;

        // Only reset the demon quests if they were not reset since the
        // previous login (since the channel sets them while playing too)
        auto progress = character->LoadProgress(worldDB);
        if(!character->GetCOMP().IsNull() && progress &&
            progress->GetDemonQuestResetTime() < lastLogin)
        {
            if(character->LoadCOMP(worldDB))
            {
                auto demons = objects::Demon::LoadDemonListByDemonBox(worldDB,
                    character->GetCOMP().GetUUID());
                for(auto demon : demons)
                {
                    //Set the quest if familiarity is high enough
                    if(!demon->GetHasQuest() &&
                        demon->GetFamiliarity() >= 4001)
                    {
                        demon->SetHasQuest(true);
                        worldChanges->Update(demon);
                    }
                }

                // Free up the COMP
                Cleanup<objects::DemonBox>(character->GetCOMP().Get());
            }
            else
            {
                LOG_ERROR(libcomp::String("Failed to load COMP to update"
                    " demon quests on account: %1.\n")
                    .Arg(account->GetUsername()));
                return false;
            }

            // Reset the demon quest daily count
            progress->SetDemonQuestDaily(0);
            progress->SetDemonQuestResetTime((uint32_t)now);
            worldChanges->Update(progress);
        }

        int32_t gpLoss = (int32_t)sharedConfig->GetDailyGPLoss();
        if(gpLoss)
        {
            // A set number of points is lost every day
            gpLoss = gpLoss * (int32_t)daysSinceLogin;

            auto pvpData = character->LoadPvPData(worldDB);
            if(pvpData)
            {
                int32_t gp = pvpData->GetGP();
                if(gp > 0)
                {
                    pvpData->SetGP(gp < gpLoss ? 0 : (gp - gpLoss));
                    worldChanges->Update(pvpData);
                }
            }
        }

        if(stats->GetLevel() > 0)
        {
            int32_t points = character->GetLoginPoints();
            points = points + (int32_t)ceil((float)stats->GetLevel() * 0.2 *
                (1.f + sharedConfig->GetLoginPointBonus()));

            if(points > character->GetLoginPoints())
            {
                character->SetLoginPoints(points);

                // If the character is in a clan, queue up a recalculation of
                // the clan level and sending of the character updates
                if(cLogin->GetClanID())
                {
                    server->QueueWork([](std::shared_ptr<WorldServer> pServer,
                        std::shared_ptr<objects::CharacterLogin> pLogin,
                        int32_t pClanID)
                    {
                        auto characterManager = pServer->GetCharacterManager();
                        characterManager->SendClanMemberInfo(pLogin);
                        characterManager->RecalculateClanLevel(pClanID);
                    }, server, cLogin, cLogin->GetClanID());
                }
            }
        }
    }

    character->SetLastLogin((uint32_t)now);
    account->SetLastLogin((uint32_t)now);

    worldChanges->Update(character);

    if(!worldDB->ProcessChangeSet(worldChanges) || !account->Update(lobbyDB))
    {
        LOG_ERROR(libcomp::String("Failed to update character data"
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

    server->GetWorldSyncManager()->SyncRecordUpdate(cLogin, "CharacterLogin");

    return true;
}

bool AccountManager::SwitchChannel(
    std::shared_ptr<objects::AccountLogin> login,
    const std::shared_ptr<objects::ChannelLogin>& switchDef)
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

    PushChannelSwitch(username, switchDef);

    auto server = mServer.lock();
    auto cLogin = login->GetCharacterLogin();

    // Mark the expected location for when the connection returns
    cLogin->SetChannelID(switchDef->GetToChannel());
    cLogin->SetZoneID(0);

    server->GetWorldSyncManager()->SyncRecordUpdate(cLogin, "CharacterLogin");

    // Set the session key now but only update the lobby if the channel
    // switch actually occurs
    UpdateSessionKey(login);

    // Update the state regardless of if the channel honors its own request
    // so the timeout can occur
    login->SetState(objects::AccountLogin::State_t::CHANNEL_TO_CHANNEL);

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
    std::shared_ptr<objects::AccountLogin> result;

    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mLock);
    auto pair = mAccountMap.find(lookup);

    if (mAccountMap.end() != pair && (channel == -1 ||
        channel == pair->second->GetCharacterLogin()->GetChannelID()))
    {
        LOG_DEBUG(libcomp::String("Logging out user: '%1'\n").Arg(username));

        result = pair->second;
        Cleanup(result);
        mAccountMap.erase(pair);
        mWebGameSessions.erase(lookup);

        auto cLogin = result->GetCharacterLogin();

        if(cLogin && !cLogin->GetCharacter().IsNull())
        {
            auto server = mServer.lock();
            auto characterManager = server->GetCharacterManager();
            auto syncManager = server->GetWorldSyncManager();

            characterManager->PartyLeave(cLogin, nullptr);
            characterManager->TeamLeave(cLogin);
            syncManager->CleanUpCharacterLogin(cLogin->GetWorldCID(), true);

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

void AccountManager::HandleLobbyLogin(
    const std::shared_ptr<objects::AccountLogin>& login)
{
    auto server = mServer.lock();
    auto config = std::dynamic_pointer_cast<objects::WorldConfig>(
        server->GetConfig());

    // Check if there is a channel login pending from last session
    // which would only occur if the character login is already
    // registered here
    auto cLogin = server->GetCharacterManager()->GetCharacterLogin(login
        ->GetCharacterLogin()->GetCharacter().GetUUID());
    auto channelLogin = cLogin ? server->GetWorldSyncManager()
        ->PopRelogin(cLogin->GetWorldCID()) : nullptr;

    if(!channelLogin &&
        config->GetWorldSharedConfig()->ChannelDistributionCount())
    {
        // Dedicated channels exist and no channel login built already,
        // validate location with primary channel
        libcomp::Packet p;
        p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

        auto primaryChannel = server->GetChannelConnectionByID(0);
        if(primaryChannel)
        {
            p.WriteS8(2);   // Requesting login info
            login->SavePacket(p, false);

            primaryChannel->SendPacket(p);
        }
        else
        {
            // No primary channel, return failure
            auto account = login->GetAccount().Get(server
                ->GetLobbyDatabase());

            p.WriteS8(0);
            if(account)
            {
                p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                    account->GetUsername(), true);
            }

            server->GetLobbyConnection()->SendPacket(p);
        }
    }
    else
    {
        // No dedicated channels, login now
        CompleteLobbyLogin(login, channelLogin);
    }
}

void AccountManager::CompleteLobbyLogin(
    const std::shared_ptr<objects::AccountLogin>& login,
    const std::shared_ptr<objects::ChannelLogin>& channelLogin)
{
    bool ok = true;

    auto cLogin = login->GetCharacterLogin();

    auto server = mServer.lock();
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();
    auto account = login->GetAccount().Get(lobbyDB, true);

    if(!account)
    {
        LOG_ERROR(libcomp::String("Invalid account sent to world"
            " AccountLogin: %1\n").Arg(
            login->GetAccount().GetUUID().ToString()));

        ok = false;
    }
    else
    {
        auto cUUID = cLogin->GetCharacter().GetUUID();

        if(cUUID.IsNull() || !cLogin->GetCharacter().Get(worldDB, true))
        {
            LOG_ERROR(libcomp::String("Character UUID '%1' is not valid"
                " for this world.\n").Arg(cUUID.ToString()));

            ok = false;
        }
    }

    auto config = std::dynamic_pointer_cast<objects::WorldConfig>(
        server->GetConfig());
    auto characterManager = server->GetCharacterManager();

    uint8_t worldID = 0;
    int8_t channelID = -1;
    if(channelLogin)
    {
        channelID = channelLogin->GetToChannel();
    }
    else
    {
        // Always start in channel 0 for redundant channel mode or
        // only one channel
        channelID = 0;
    }

    ok &= channelID >= 0 &&
        server->GetChannelConnectionByID(channelID) != nullptr;
    if(ok)
    {
        // Remove any channel switches stored for whatever reason
        PopChannelSwitch(account->GetUsername());

        // Login now to get the session key
        if(!LobbyLogin(login))
        {
            LOG_ERROR(libcomp::String("Failed to login character '%1'. "
                "Here is the state of the login object now: %2\n").Arg(
                account->GetUsername()).Arg(login->GetXml()));
            ok = false;
        }
    }

    if(ok)
    {
        worldID = config->GetID();

        // Get the cached character login or register a new one
        cLogin = characterManager->RegisterCharacter(cLogin);

        // If a relogin still exists, pop that now that the destination is set
        server->GetWorldSyncManager()->PopRelogin(cLogin->GetWorldCID());

        if(channelLogin && channelLogin->GetFromChannel() == -1)
        {
            // If a relogin was supplied, push it as a channel switch so
            // it is returned to the channel once the connection arrives
            PushChannelSwitch(account->GetUsername(), channelLogin);
        }

        auto character = cLogin->GetCharacter().Get();
        if(!character->GetClan().IsNull())
        {
            // Load the clan
            auto clan = character->GetClan().Get();
            if(!clan)
            {
                clan = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Clan>(worldDB, character->GetClan().GetUUID());
            }

            if(clan)
            {
                // Load the members and store in the CharacterManager
                auto members = objects::ClanMember::LoadClanMemberListByClan(
                    worldDB, clan->GetUUID());
                auto clanInfo = characterManager->GetClan(clan->GetUUID());
                if(clanInfo)
                {
                    cLogin->SetClanID(clanInfo->GetID());
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                ok = false;
            }
        }

        // If the character is already logged in somehow, send a
        // disconnect request (should cover dead connections)
        characterManager->RequestChannelDisconnect(cLogin->GetWorldCID());
    }

    libcomp::Packet reply;
    reply.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

    if(ok)
    {
        reply.WriteS8(1); // Success

        cLogin->SetWorldID((int8_t)worldID);
        cLogin->SetChannelID(channelID);

        // Check if they were part of a party that was disbanded
        if(cLogin->GetPartyID() &&
            !characterManager->GetParty(cLogin->GetPartyID()))
        {
            cLogin->SetPartyID(0);
        }

        login->SetCharacterLogin(cLogin);
        login->SavePacket(reply, false);

        LOG_DEBUG(libcomp::String("Logging in account '%1' with session key"
            " %2\n").Arg(account->GetUsername()).Arg(login->GetSessionKey()));

        // Schedule channel login timeout
        server->GetTimerManager()->ScheduleEventIn(static_cast<int>(
            config->GetChannelConnectionTimeOut()), [server]
            (const std::shared_ptr<WorldServer> pServer,
             const libcomp::String& pUsername, uint32_t pKey)
        {
            pServer->GetAccountManager()->ExpireSession(pUsername, pKey);
        }, server, account->GetUsername(), login->GetSessionKey());
    }
    else
    {
        // Faiure, send the username back to disconnect
        reply.WriteS8(0);
        if(account)
        {
            reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                account->GetUsername(), true);
        }
    }

    server->GetLobbyConnection()->SendPacket(reply);
}

void AccountManager::HandleChannelLogin(
    const std::shared_ptr<libcomp::InternalConnection>& requestConnection,
    uint32_t sessionKey, const libcomp::String& username)
{
    bool ok = true;

    auto server = mServer.lock();
    auto channel = server->GetChannel(requestConnection);
    auto login = GetUserLogin(username);
    auto cLogin = login != nullptr ? login->GetCharacterLogin() : nullptr;
    if(nullptr == channel)
    {
        LOG_ERROR("AccountLogin request received"
            " from a connection not belonging to the lobby or any"
            " connected channel.\n");
        return;
    }
    else if(username.Length() == 0)
    {
        LOG_ERROR("No username passed to AccountLogin from the channel.\n");
        return;
    }
    else if(nullptr == login)
    {
        LOG_ERROR(libcomp::String("Account with username '%1'"
            " is not logged in to this world or has an expired session.\n")
            .Arg(username));
        ok = false;
    }
    else if(nullptr == cLogin)
    {
        LOG_ERROR(libcomp::String("Account with username '%1'"
            " is not logged in to this world (bad cLogin).\n").Arg(username));
        ok = false;
    }
    else if(channel->GetID() != (uint8_t)cLogin->GetChannelID())
    {
        LOG_ERROR("AccountLogin request received from a channel"
            " not matching the account's current login information.\n");
        ok = false;
    }
    else if(login->GetSessionKey() != sessionKey)
    {
        LOG_ERROR(libcomp::String("Invalid session key provided for"
            " account with username '%1': Expected %2, found %3\n")
            .Arg(username).Arg(login->GetSessionKey()).Arg(sessionKey));
        ok = false;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

    if(ok)
    {
        if(ChannelLogin(login))
        {
            reply.WriteS8(1); // Normal success

            // Update the lobby with the new connection info
            libcomp::Packet lobbyMessage;
            lobbyMessage.WritePacketCode(
                InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);
            lobbyMessage.WriteS8(1);    // Success
            login->SavePacket(lobbyMessage, false);

            server->GetLobbyConnection()->SendPacket(lobbyMessage);

            login->SavePacket(reply, false);

            // If a channel login exists, write it too
            auto switchDef = PopChannelSwitch(username);
            reply.WriteU8(switchDef ? 1 : 0);
            if(switchDef)
            {
                switchDef->SavePacket(reply);
            }
        }
        else
        {
            ok = false;
        }
    }

    if(!ok)
    {
        // Faiure, send the username back to disconnect
        reply.WriteS8(0);
        reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
            username, true);
    }

    requestConnection->SendPacket(reply);

    /*if(ok)
    {
        // Send party/team info if still in either
        if(cLogin->GetPartyID())
        {
            auto characterManager = server->GetCharacterManager();
            auto member = characterManager->GetPartyMember(cLogin
                ->GetWorldCID());
            if(member)
            {
                characterManager->SendPartyMember(member, cLogin->GetPartyID(),
                    false, true, requestConnection);
            }
        }

        if(cLogin->GetTeamID())
        {
            server->GetCharacterManager()->SendTeamInfo(cLogin->GetTeamID(),
                { cLogin->GetWorldCID() });
        }
    }*/
}

void AccountManager::UpdateSessionKey(std::shared_ptr<objects::AccountLogin> login)
{
    login->SetSessionKey(RNG(uint32_t, 1, (uint32_t)0x7FFFFFFF));
}

void AccountManager::PushChannelSwitch(const libcomp::String& username,
    const std::shared_ptr<objects::ChannelLogin>& switchDef)
{
    libcomp::String lookup = username.ToLower();
    mChannelSwitches[lookup] = switchDef;
}

bool AccountManager::ChannelSwitchPending(const libcomp::String& username,
    int8_t& channel)
{
    libcomp::String lookup = username.ToLower();
    std::lock_guard<std::mutex> lock(mLock);

    auto it = mChannelSwitches.find(lookup);
    if(it != mChannelSwitches.end())
    {
        channel = it->second->GetToChannel();
        return true;
    }

    return false;
}

std::shared_ptr<objects::ChannelLogin> AccountManager::PopChannelSwitch(
    const libcomp::String& username)
{
    libcomp::String lookup = username.ToLower();
    std::lock_guard<std::mutex> lock(mLock);

    auto it = mChannelSwitches.find(lookup);
    if(it != mChannelSwitches.end())
    {
        auto login = it->second;
        mChannelSwitches.erase(it);
        return login;
    }

    return nullptr;
}

void AccountManager::CleanupAccountWorldData()
{
    auto server = mServer.lock();
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();

    auto accountWorldDataList = objects::AccountWorldData::
        LoadAccountWorldDataListByCleanupRequired(worldDB, true);
    if(accountWorldDataList.size() == 0)
    {
        return;
    }

    LOG_DEBUG(libcomp::String("Cleaning up %1 AccountWorldData record(s)\n")
        .Arg(accountWorldDataList.size()));

    for(auto accountWorldData : accountWorldDataList)
    {
        auto account = libcomp::PersistentObject::LoadObjectByUUID<
            objects::Account>(lobbyDB, accountWorldData->GetAccount().GetUUID());

        if(account)
        {
            LOG_DEBUG(libcomp::String("Cleaning up AccountWorldData associated"
                " to account: %1\n").Arg(account->GetUUID().ToString()));

            auto characters = account->GetCharacters();

            // Mark the cleanup as being handled before doing any work
            accountWorldData->SetCleanupRequired(false);

            bool updateWorldData = true;
            for(auto character : objects::Character::LoadCharacterListByAccount(
                worldDB, account->GetUUID()))
            {
                // If the character has a kill time marked and they are not
                // in the account character list, delete them
                if(character->GetKillTime() != 0)
                {
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
                        // Not in the character list, delete it
                        updateWorldData &= DeleteCharacter(character);
                    }
                }
            }

            if(updateWorldData)
            {
                accountWorldData->Update(worldDB);
            }
        }
        else
        {
            LOG_ERROR(libcomp::String("AccountWorldData associated to invalid"
                " account: %1\n").Arg(accountWorldData->GetAccount().GetUUID()
                    .ToString()));
        }
    }
}

bool AccountManager::DeleteCharacter(const std::shared_ptr<
    objects::Character>& character)
{
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();

    auto db = server->GetWorldDatabase();
    auto characterUUID = character->GetUUID();

    LOG_DEBUG(libcomp::String("Deleting character '%1' on account: %2\n")
        .Arg(character->GetName()).Arg(character->GetAccount().ToString()));

    auto changes = libcomp::DatabaseChangeSet::Create();

    // Always free up the friend settings and clan information immediately
    auto cLogin = characterManager->GetCharacterLogin(
        character->GetName());

    int32_t clanID = 0;
    auto clanMember = objects::ClanMember::LoadClanMemberByCharacter(
        db, characterUUID);
    if(clanMember)
    {
        auto clan = libcomp::PersistentObject::LoadObjectByUUID<objects::Clan>(
            db, clanMember->GetClan());

        bool left = false;
        if(clan && cLogin)
        {
            auto clanInfo = characterManager->GetClan(clan->GetUUID());
            if(clanInfo)
            {
                clanID = clanInfo->GetID();
                left = characterManager->ClanLeave(cLogin, clanID, nullptr);
            }
        }

        if(!left)
        {
            LOG_ERROR(libcomp::String("Failed to remove %1 from their clan\n")
                .Arg(characterUUID.ToString()));
            return false;
        }
    }

    auto friendSettings = objects::FriendSettings::
        LoadFriendSettingsByCharacter(db, characterUUID);
    if(friendSettings && friendSettings->FriendsCount() > 0)
    {
        // Drop from other friend lists but let the other player get the
        // the update the next time they log on
        for(auto otherChar : friendSettings->GetFriends())
        {
            auto otherFriendSettings = objects::FriendSettings::
                LoadFriendSettingsByCharacter(db, otherChar);
            if(otherFriendSettings)
            {
                auto friends = otherFriendSettings->GetFriends();
                friends.remove_if([characterUUID](
                    const libcomp::ObjectReference<
                        libcomp::PersistentObject>& f)
                    {
                        return f.GetUUID() == characterUUID;
                    });
                otherFriendSettings->SetFriends(friends);

                changes->Update(otherFriendSettings);
            }
        }

        friendSettings->ClearFriends();
        changes->Update(friendSettings);
    }

    // If the character is somehow connected, send a disconnect request
    // and mark the character for deletion later
    if(cLogin && cLogin->GetChannelID() >= 0)
    {
        LOG_WARNING(libcomp::String("Deleting character '%1' is still logged"
            " in on account: %2\n").Arg(character->GetName())
            .Arg(character->GetAccount().ToString()));

        characterManager->RequestChannelDisconnect(
            cLogin->GetWorldCID());

        // Set (min) kill time and mark the AccountWorldData
        // to signify that the character needs to be cleaned up
        auto accountWorldData = objects::AccountWorldData::
            LoadAccountWorldDataByAccount(db, character->GetAccount());
        if(accountWorldData)
        {
            accountWorldData->SetCleanupRequired(true);
            changes->Update(accountWorldData);
        }
        else
        {
            LOG_ERROR(libcomp::String("Failed to delete logged in character"
                " without associated AccountWorlData: %1\n")
                .Arg(characterUUID.ToString()));
            return false;
        }

        character->SetKillTime(1);
        changes->Update(character);

        return db->ProcessChangeSet(changes);
    }

    // Load all associated records and add them to the same transaction
    // for deletion
    std::list<libobjgen::UUID> entityUIDs = { characterUUID };

    changes->Delete(character);

    // Delete items and item boxes
    for(auto itemBox : objects::ItemBox::LoadItemBoxListByCharacter(db,
        characterUUID))
    {
        for(auto item : objects::Item::LoadItemListByItemBox(db, itemBox
            ->GetUUID()))
        {
            changes->Delete(item);
            Cleanup(item);
        }

        changes->Delete(itemBox);
        Cleanup(itemBox);
    }

    // Delete demons, demon boxes and inherited skills
    for(auto demonBox : objects::DemonBox::LoadDemonBoxListByCharacter(db,
        characterUUID))
    {
        for(auto demon : objects::Demon::LoadDemonListByDemonBox(db, demonBox
            ->GetUUID()))
        {
            entityUIDs.push_back(demon->GetUUID());

            for(auto iSkill : objects::InheritedSkill::
                LoadInheritedSkillListByDemon(db, demon->GetUUID()))
            {
                changes->Delete(iSkill);
                Cleanup(iSkill);
            }

            changes->Delete(demon);
            Cleanup(demon);
        }

        changes->Delete(demonBox);
        Cleanup(demonBox);
    }

    // Delete expertise
    for(auto expertise : objects::Expertise::LoadExpertiseListByCharacter(db,
        characterUUID))
    {
        changes->Delete(expertise);
        Cleanup(expertise);
    }

    // Delete hotbar
    for(auto hotbar : objects::Hotbar::LoadHotbarListByCharacter(db,
        characterUUID))
    {
        changes->Delete(hotbar);
        Cleanup(hotbar);
    }

    // Delete quests
    for(auto quest : objects::Quest::LoadQuestListByCharacter(db,
        characterUUID))
    {
        changes->Delete(quest);
        Cleanup(quest);
    }

    // Delete entity stats and status effects
    for(libobjgen::UUID entityUID : entityUIDs)
    {
        auto entityStats = objects::EntityStats::
            LoadEntityStatsByEntity(db, entityUID);
        if(entityStats)
        {
            changes->Delete(entityStats);
            Cleanup(entityStats);
        }

        for(auto status : objects::StatusEffect::LoadStatusEffectListByEntity(
            db, entityUID))
        {
            changes->Delete(status);
            Cleanup(status);
        }
    }

    // Delete character progress
    auto progress = objects::CharacterProgress::
        LoadCharacterProgressByCharacter(db, characterUUID);
    if(progress)
    {
        changes->Delete(progress);
        Cleanup(progress);
    }

    // Delete friend settings
    if(friendSettings)
    {
        changes->Delete(friendSettings);
        Cleanup(friendSettings);
    }

    // Process the deletes all at once
    if(db->ProcessChangeSet(changes))
    {
        characterManager->UnregisterCharacter(cLogin);
        return true;
    }

    LOG_WARNING(libcomp::String("Failed to delete character '%1'"
        " on account: %2\n").Arg(character->GetName())
        .Arg(character->GetAccount().ToString()));
    return false;
}

bool AccountManager::StartWebGameSession(const std::shared_ptr<
    objects::WebGameSession>& gameSession)
{
    auto server = mServer.lock();
    auto account = gameSession->GetAccount().Get(server->GetLobbyDatabase());
    if(!account)
    {
        return false;
    }

    libcomp::String lookup = account->GetUsername().ToLower();

    std::lock_guard<std::mutex> lock(mLock);
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

    // Session is valid, generate the session ID and register it
    auto sessionID = libcomp::Crypto::GenerateRandom(20).ToLower();
    gameSession->SetSessionID(sessionID);

    mWebGameSessions[lookup] = gameSession;

    // Notify the lobby that the session has started and wait for
    // reply indicating that it is ready
    auto lobby = server->GetLobbyConnection();
    if(lobby)
    {
        libcomp::Packet notify;
        notify.WritePacketCode(InternalPacketCode_t::PACKET_WEB_GAME);
        notify.WriteU8((uint8_t)
            InternalPacketAction_t::PACKET_ACTION_ADD);
        notify.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8,
            account->GetUsername(), true);
        gameSession->SavePacket(notify);

        lobby->SendPacket(notify);
    }

    return true;
}

std::shared_ptr<objects::WebGameSession> AccountManager::GetGameSession(
    const libcomp::String& username)
{
    libcomp::String lookup = username.ToLower();

    std::lock_guard<std::mutex> lock(mLock);

    auto pair = mWebGameSessions.find(lookup);
    return pair != mWebGameSessions.end() ? pair->second : nullptr;
}

bool AccountManager::EndWebGameSession(const libcomp::String& username,
    bool notifyLobby, bool notifyChannel)
{
    bool existed = false;
    {
        libcomp::String lookup = username.ToLower();

        std::lock_guard<std::mutex> lock(mLock);

        auto sessionPair = mWebGameSessions.find(lookup);
        if(sessionPair != mWebGameSessions.end())
        {
            mWebGameSessions.erase(lookup);
            existed = true;
        }
    }

    if(notifyLobby)
    {
        libcomp::Packet notify;
        notify.WritePacketCode(InternalPacketCode_t::PACKET_WEB_GAME);
        notify.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_REMOVE);
        notify.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, username, true);

        auto lobby = mServer.lock()->GetLobbyConnection();
        if(lobby)
        {
            lobby->SendPacket(notify);
        }
    }

    if(notifyChannel)
    {
        auto login = GetUserLogin(username);
        auto cLogin = login ? login->GetCharacterLogin() : nullptr;

        // Only notify the channel if the session existed
        if(existed && cLogin)
        {
            libcomp::Packet notify;
            notify.WritePacketCode(InternalPacketCode_t::PACKET_WEB_GAME);
            notify.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_REMOVE);
            notify.WriteS32Little(cLogin->GetWorldCID());

            auto channel = mServer.lock()->GetChannelConnectionByID(
                cLogin->GetChannelID());
            if(channel)
            {
                channel->SendPacket(notify);
            }
        }
    }

    return existed;
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
    Cleanup<objects::PvPData>(cLogin->GetCharacter()
        ->GetPvPData().Get());
    Cleanup<objects::Account>(login->GetAccount().Get());
}
