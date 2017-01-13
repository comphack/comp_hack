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
#include <Character.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

void AccountManager::Login(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const libcomp::String& username, uint32_t sessionKey)
{
    auto server = ChannelServer::GetRunningServer();
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();

    auto account = objects::Account::LoadAccountByUserName(lobbyDB, username);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_LOGIN_RESPONSE);

    bool success = false;
    if(nullptr != account)
    {
        /// @todo: load the information the lobby retrieved
        auto cid = (uint8_t)sessionKey;
        auto charactersByCID = account->GetCharactersByCID();

        auto character = charactersByCID.find(cid);

        if(character != charactersByCID.end() && character->second.Get(worldDB))
        {
            auto state = client->GetClientState();
            state->SetAccount(account);
            state->SetSessionKey(sessionKey);

            auto charState = state->GetCharacterState();
            charState->SetCharacter(character->second.GetCurrentReference());
            charState->RecalculateStats();

            success = true;
        }
    }

    if(success)
    {
        reply.WriteU32Little(1);
    }
    else
    {
        LOG_ERROR(libcomp::String("Invalid account username passed to the channel"
            " from the lobby: %1\n").Arg(username));
        reply.WriteU32Little(0);
    }

    client->SendPacket(reply);
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
