/**
 * @file server/lobby/src/packets/game/Login.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to a request to login to the lobby.
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

#include "Packets.h"

// lobby Includes
#include "LobbyClientConnection.h"
#include "LobbyServer.h"

// libcomp Includes
#include <Crypto.h>
#include <ErrorCodes.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// object Includes
#include <Account.h>
#include <PacketLogin.h>
#include <PacketLoginReply.h>

using namespace lobby;

static bool LoginError(const std::shared_ptr<
    libcomp::TcpConnection>& connection, ErrorCodes_t errorCode)
{
    libcomp::Packet reply;
    reply.WritePacketCode(LobbyToClientPacketCode_t::PACKET_LOGIN);
    reply.WriteS32Little(to_underlying(errorCode));

    connection->SendPacket(reply);

    return true;
}

bool Parsers::Login::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    objects::PacketLogin obj;

    if(!obj.LoadPacket(p))
    {
        return false;
    }

    // Update the connection name with the username of the account.
    connection->SetName(libcomp::String("%1:%2").Arg(
        connection->GetName()).Arg(obj.GetUsername()));

    // Check the client version.
    auto conf = config(pPacketManager);
    uint32_t clientVersion = static_cast<uint32_t>(
        conf->GetClientVersion() * 1000.0f + 0.5f);

    if(clientVersion != obj.GetClientVersion())
    {
        return LoginError(connection, ErrorCodes_t::WRONG_CLIENT_VERSION);
    }

    // Save the username for later.
    state(connection)->SetUsername(obj.GetUsername());

    // Generate a challenge for the client.
    uint32_t challenge = libcomp::Crypto::GenerateSessionKey();

    // Get a reference to the server.
    auto server = std::dynamic_pointer_cast<LobbyServer>(
        pPacketManager->GetServer());

    // Get the account from the database.
    auto account = objects::Account::LoadAccountByUsername(
        server->GetMainDatabase(), obj.GetUsername());

    // Save the account information.
    state(connection)->SetAccount(account);

    // Save the challenge for authentication.
    state(connection)->SetChallenge(challenge);

    // Send the reply.
    objects::PacketLoginReply reply;
    reply.SetCommandCode(to_underlying(
        LobbyToClientPacketCode_t::PACKET_LOGIN));
    reply.SetResponseCode(to_underlying(
        ErrorCodes_t::SUCCESS));
    reply.SetChallenge(challenge);

    // If the account exists, use the salt; otherwise, use a random one.
    if(account)
    {
        reply.SetSalt(account->GetSalt());
    }
    else
    {
        reply.SetSalt(server->GetFakeAccountSalt(obj.GetUsername()));
    }

    return connection->SendObject(reply);
}
