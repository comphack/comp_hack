/**
 * @file server/world/src/packets/AccountLogin.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle retrieving a channel for the client to log into.
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

#include "Packets.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// Standard C Includes
#include <cmath>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <Clan.h>
#include <ClanMember.h>
#include <EntityStats.h>

// world Includes
#include "AccountManager.h"
#include "WorldConfig.h"
#include "WorldServer.h"

using namespace world;

void LobbyLogin(std::shared_ptr<WorldServer> server,
    const std::shared_ptr<libcomp::InternalConnection> connection,
    std::shared_ptr<objects::AccountLogin> login)
{
    // The lobby is requesting a channel to log into.
    bool ok = true;

    auto cLogin = login->GetCharacterLogin();
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

    libcomp::Packet reply;
    reply.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

    auto config = std::dynamic_pointer_cast<objects::WorldConfig>(
        server->GetConfig());
    auto characterManager = server->GetCharacterManager();

    uint8_t worldID = 0;
    int8_t channelID = 0;
    auto loginChannel = server->GetLoginChannel();

    ok = loginChannel != nullptr;
    if(ok)
    {
        auto accountManager = server->GetAccountManager();

        // Remove any channel switches stored for whatever reason
        accountManager->PopChannelSwitch(login->GetAccount()
            ->GetUsername(), channelID);

        // Login now to get the session key
        if(!accountManager->LobbyLogin(login))
        {
            LOG_ERROR(libcomp::String("Failed to login character '%1'. "
                "Here is the state of the login object now:\n").Arg(
                account->GetUsername()).Arg(login->GetXml()));
            ok = false;
        }
    }

    if(ok)
    {
        worldID = config->GetID();
        if(channelID == 0)
        {
            channelID = (int8_t)loginChannel->GetID();
        }

        // Get the cached character login or register a new one
        cLogin = characterManager->RegisterCharacter(cLogin);

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
                auto members = objects::ClanMember::LoadClanMemberListByClan(worldDB, clan);
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

    if(ok)
    {
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
        reply.WriteS8(0); // Failure
    }

    connection->SendPacket(reply);
}

void ChannelLogin(std::shared_ptr<WorldServer> server,
    const std::shared_ptr<libcomp::InternalConnection> connection,
    uint32_t sesssionKey, const libcomp::String username)
{
    //The channel is requesting session info
    bool ok = true;
    auto channel = server->GetChannel(connection);
    auto accountManager = server->GetAccountManager();
    auto login = accountManager->GetUserLogin(username);
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
    else if(login->GetSessionKey() != sesssionKey)
    {
        LOG_ERROR(libcomp::String("Invalid session key provided for"
            " account with username '%1': Expected %2, found %3\n")
            .Arg(username).Arg(login->GetSessionKey()).Arg(sesssionKey));
        ok = false;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

    if(ok)
    {
        if(accountManager->ChannelLogin(login))
        {
            reply.WriteS8(1); // Success

            // Update the lobby with the new connection info
            auto lobbyConnection = server->GetLobbyConnection();

            libcomp::Packet lobbyMessage;
            lobbyMessage.WritePacketCode(
                InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

            login->SavePacket(lobbyMessage, false);
            lobbyConnection->SendPacket(lobbyMessage);
        }
        else
        {
            reply.WriteS8(0); // Failure
        }

        login->SavePacket(reply, false);
    }
    else
    {
        reply.WriteS8(0); // Failure
    }

    connection->SendPacket(reply);
}

bool Parsers::AccountLogin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto server = std::dynamic_pointer_cast<WorldServer>(
        pPacketManager->GetServer());
    auto iConnection = std::dynamic_pointer_cast<
        libcomp::InternalConnection>(connection);

    if(connection == server->GetLobbyConnection())
    {
        auto login = std::shared_ptr<objects::AccountLogin>(
            new objects::AccountLogin);

        if(!login->LoadPacket(p, false))
        {
            return false;
        }

        server->QueueWork(LobbyLogin, server, iConnection, login);
    }
    else
    {
        uint32_t sesssionKey = p.ReadU32();

        libcomp::String username = p.ReadString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

        server->QueueWork(ChannelLogin, server, iConnection,
            sesssionKey, username);
    }

    return true;
}
