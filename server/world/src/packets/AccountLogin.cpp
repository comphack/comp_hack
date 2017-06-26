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

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// world Includes
#include "AccountManager.h"
#include "WorldConfig.h"
#include "WorldServer.h"

using namespace world;

void LobbyLogin(std::shared_ptr<WorldServer> server,
    const std::shared_ptr<libcomp::InternalConnection> connection,
    std::shared_ptr<objects::AccountLogin> login)
{
    //The lobby is requesting a channel to log into
    bool ok = true;

    auto cLogin = login->GetCharacterLogin();
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();
    auto account = login->GetAccount().Get(lobbyDB, true);
    if(nullptr == account)
    {
        LOG_ERROR(libcomp::String("Invalid account sent to world"
            " AccountLogin: %1\n").Arg(
                login->GetAccount().GetUUID().ToString()));
        ok = false;
    }
    else
    {
        auto cUUID = cLogin->GetCharacter().GetUUID();
        if(cUUID.IsNull() ||
            nullptr == cLogin->GetCharacter().Get(worldDB, true))
        {
            LOG_ERROR(libcomp::String("Character UUID '%1' is not valid"
                " for this world.\n").Arg(cUUID.ToString()));
            ok = false;
        }
    }

    libcomp::Packet channelReply;
    channelReply.WritePacketCode(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

    if(ok)
    {
        auto channel = server->GetLoginChannel();
        if(nullptr != channel)
        {
            auto worldID = std::dynamic_pointer_cast<objects::WorldConfig>(
                server->GetConfig())->GetID();
            auto channelID = channel->GetID();
            auto characterManager = server->GetCharacterManager();

            // Login now to get the session key
            server->GetAccountManager()->LoginUser(login);

            // Get the cached character login or register a new one
            cLogin = characterManager->RegisterCharacter(cLogin);
            cLogin->SetWorldID((int8_t)worldID);
            cLogin->SetChannelID((int8_t)channelID);

            // Check if they were part of a party that was disbanded
            if(cLogin->GetPartyID() &&
                !characterManager->GetParty(cLogin->GetPartyID()))
            {
                cLogin->SetPartyID(0);
            }

            login->SetCharacterLogin(cLogin);
            login->SavePacket(channelReply, false);
        }
    }

    connection->SendPacket(channelReply);
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
    else if(nullptr == login || nullptr == cLogin)
    {
        LOG_ERROR(libcomp::String("Account with username '%1'"
            " is not logged in to this world.\n").Arg(username));
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
            " account with username '%1'\n").Arg(username));
        ok = false;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

    if(ok)
    {
        cLogin->SetWorldID((int8_t)server->GetRegisteredWorld()->GetID());
        cLogin->SetStatus(objects::CharacterLogin::Status_t::ONLINE);
        login->SavePacket(reply, false);

        //Update the lobby with the new connection info
        auto lobbyConnection = server->GetLobbyConnection();
        libcomp::Packet lobbyMessage;
        lobbyMessage.WritePacketCode(
            InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);

        login->SavePacket(lobbyMessage, false);
        lobbyConnection->SendPacket(lobbyMessage);
    }

    connection->SendPacket(reply);
}

bool Parsers::AccountLogin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
    auto iConnection = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);

    if(connection == server->GetLobbyConnection())
    {
        auto login = std::shared_ptr<objects::AccountLogin>(new objects::AccountLogin);
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
        server->QueueWork(ChannelLogin, server, iConnection, sesssionKey, username);
    }

    return true;
}
