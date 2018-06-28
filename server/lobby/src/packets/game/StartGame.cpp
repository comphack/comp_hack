/**
 * @file server/lobby/src/packets/game/StartGame.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to handle a lobby request to start the game.
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

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// lobby Includes
#include "AccountManager.h"
#include "LobbyClientConnection.h"
#include "LobbyServer.h"
#include "ManagerPacket.h"

using namespace lobby;

bool Parsers::StartGame::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    // Sanity check the packet size.
    if(2 != p.Size())
    {
        return false;
    }

    // Grab the character ID.
    uint8_t cid = p.ReadU8();

    // Grab the world ID.
    int8_t worldID = p.ReadS8();

    // We need all this jazz.
    auto client = std::dynamic_pointer_cast<LobbyClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto account = state->GetAccount();
    auto username = account->GetUsername();
    auto server = std::dynamic_pointer_cast<LobbyServer>(
        pPacketManager->GetServer());
    auto world = server->GetWorldByID((uint8_t)worldID);

    // Check the world is still there.
    if(!world)
    {
        LOG_ERROR(libcomp::String("User '%1' tried to loging to world %2 but "
            "that world is not active.\n").Arg(username).Arg(worldID));

        return false;
    }

    // We need all this jazz too.
    auto accountManager = server->GetAccountManager();
    auto character = account->GetCharacters(cid).Get();

    // What? Go away hacker.
    if(!character)
    {
        LOG_ERROR("Failed to get character?!\n");

        return false;
    }

    // Start the channel login process.
    auto login = accountManager->StartChannelLogin(username, character);

    if(!login)
    {
        return false;
    }

    LOG_DEBUG(libcomp::String("Start game request received for character"
        " '%1' from %2\n").Arg(character->GetName())
        .Arg(client->GetRemoteAddress()));

    // Let the world know what we want to do.
    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);
    login->SavePacket(request, false);

    world->GetConnection()->SendPacket(request);

    return true;
}
