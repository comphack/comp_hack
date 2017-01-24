/**
 * @file server/channel/src/AccountManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages accounts on the channel.
 *
 * This file is part of the Channel Server (channel).
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

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterProgress.h>
#include <EntityStats.h>
#include <Item.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

AccountManager::AccountManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

AccountManager::~AccountManager()
{
}

void AccountManager::HandleLoginRequest(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const libcomp::String& username, uint32_t sessionKey)
{
    auto server = mServer.lock();
    auto worldConnection = server->GetManagerConnection()->GetWorldConnection();

    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();

    auto account = objects::Account::LoadAccountByUsername(lobbyDB, username);

    if(nullptr != account)
    {
        auto state = client->GetClientState();
        auto login = state->GetAccountLogin();
        login->SetAccount(account);
        login->SetSessionKey(sessionKey);

        server->GetManagerConnection()->SetClientConnection(client);

        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);
        request.WriteU32(sessionKey);
        request.WriteString16Little(libcomp::Convert::ENCODING_UTF8, username);

        worldConnection->SendPacket(request);
    }
}

void AccountManager::HandleLoginResponse(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();
    auto state = client->GetClientState();
    auto login = state->GetAccountLogin();
    auto account = login->GetAccount();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_LOGIN_RESPONSE);
    
    auto cid = login->GetCID();
    auto character = account->GetCharacters(cid);

    // Load the character into the cache
    bool success = !character.IsNull() && character.Get(worldDB) &&
        character->LoadCoreStats(worldDB);
    if(success)
    {
        if(character->GetProgress().IsNull())
        {
            // Character has never been initialized past lobby values
            auto progress = libcomp::PersistentObject::New<
                objects::CharacterProgress>();

            success = progress->Register(progress) &&
                progress->Insert(worldDB) &&
                character->SetProgress(progress);
        }
        else if(!character->LoadProgress(worldDB))
        {
            success = false;
        }
    }

    if(success)
    {
        //If we're still good, load up the sub-collections
        for(auto equip : character->GetEquippedItems())
        {
            if(!equip.IsNull() && !equip.Get(worldDB))
            {
                success = false;
                break;
            }
        }
    }

    if(success)
    {
        auto charState = state->GetCharacterState();
        charState->SetCharacter(character);
        charState->SetEntityID(character->GetCID() + 1);    /// @todo
        charState->RecalculateStats();

        reply.WriteU32Little(1);
    }
    else
    {
        LOG_ERROR(libcomp::String("User account could not be logged in:"
            " %1\n").Arg(account->GetUsername()));
        reply.WriteU32Little(0);
    }

    client->SendPacket(reply);
}

void AccountManager::HandleLogoutRequest(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    LogoutCode_t code, uint8_t channel)
{
    std::list<libcomp::Packet> replies;
    switch(code)
    {
        case LogoutCode_t::LOGOUT_CODE_QUIT:
            {
                libcomp::Packet reply;
                reply.WritePacketCode(
                    ChannelClientPacketCode_t::PACKET_LOGOUT_RESPONSE);
                reply.WriteU32Little((uint32_t)10);
                replies.push_back(reply);

                reply = libcomp::Packet();
                reply.WritePacketCode(
                    ChannelClientPacketCode_t::PACKET_LOGOUT_RESPONSE);
                reply.WriteU32Little((uint32_t)13);
                replies.push_back(reply);
            }
            break;
        case LogoutCode_t::LOGOUT_CODE_SWITCH:
            (void)channel;
            /// @todo: handle switching code
            break;
        default:
            break;
    }

    for(libcomp::Packet& reply : replies)
    {
        client->SendPacket(reply);
    }
}

void AccountManager::Logout(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto managerConnection = server->GetManagerConnection();
    auto state = client->GetClientState();
    auto account = state->GetAccountLogin()->GetAccount().Get();
    auto character = state->GetCharacterState()->GetCharacter().Get();

    if(nullptr == account || nullptr == character)
    {
        return;
    }
    else
    {
        /// @todo: detach character from anything using it
        /// @todo: set logout information

        if(!character->Update(server->GetWorldDatabase()))
        {
            LOG_ERROR(libcomp::String("Character %1 failed to save on account"
                " %2.\n").Arg(character->GetUUID().ToString())
                .Arg(account->GetUUID().ToString()));
        }
    }

    //Remove the connection if it hasn't been removed already.
    managerConnection->RemoveClientConnection(client);

    libcomp::Packet p;
    p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
    p.WriteString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, account->GetUsername());
    managerConnection->GetWorldConnection()->SendPacket(p);
}

void AccountManager::Authenticate(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_AUTH_RESPONSE);

    if(nullptr != state)
    {
        state->SetAuthenticated(true);
        reply.WriteU32Little(0);
    }
    else
    {
        reply.WriteU32Little(1);
    }

    client->SendPacket(reply);
}
