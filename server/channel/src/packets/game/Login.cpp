/**
 * @file server/channel/src/packets/Login.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to log in.
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

#include "Packets.h"

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Account.h>
#include <Character.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"

using namespace channel;

bool Parsers::Login::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
	// Where to store the username and session key
	libcomp::String username;
	uint32_t sessionKey;

	// If the size of the login packet isn't right, this might be the new
	// Atlus login method.
	if(p.Size() < 6 || p.Size() != (uint32_t)(6 + p.PeekU16Little()))
	{
		// Check if the authentication string is there
        if(p.Size() < (uint16_t)(2 + p.PeekU16Little()))
        {
            return false;
        }

		// Read the authentication string
		/// @todo validate this key instead of ignoring it
		libcomp::String authKey = p.ReadString16Little(
            libcomp::Convert::ENCODING_UTF8);

		// Check if the session key is there
		if(p.Left() < 4)
        {
            return false;
        }

		// Read the session key
		sessionKey = p.ReadU32Little();

		// Check if the username is there
		if(p.Left() != (uint32_t)(2 + p.PeekU16Little()))
        {
            return false;
        }

		// Read the username
		username = p.ReadString16(libcomp::Convert::ENCODING_UTF8);
	}
	else
	{
		// Classic authentication method: username followed by the session key
		username = p.ReadString16(libcomp::Convert::ENCODING_UTF8);
		sessionKey = p.ReadU32Little();
	}

    // Remove null terminator
    username = username.C();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();

    auto account = objects::Account::LoadAccountByUserName(lobbyDB, username);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_LOGIN_RESPONSE);

    bool success = false;
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    if(nullptr != account)
    {
        /// @todo: load the information the lobby retrieved
        auto cid = sessionKey;

        std::shared_ptr<objects::Character> character;
        /// @todo: load character by uid via account map keyed on cid
        for(auto accountCharacter : objects::Character::LoadCharacterListByAccount(
            worldDB, account))
        {
            if(accountCharacter->GetCID() == cid)
            {
                character = accountCharacter;
                break;
            }
        }

        if(nullptr != character)
        {
            auto state = std::shared_ptr<ClientState>(new ClientState);
            state->SetAccount(account);
            state->SetSessionKey(sessionKey);
            state->SetCharacter(character);

            client->SetClientState(state);

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

    connection->SendPacket(reply);

    return true;
}
