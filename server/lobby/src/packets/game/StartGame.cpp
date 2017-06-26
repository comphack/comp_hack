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
    (void)pPacketManager;

    if(p.Size() != 2)
    {
        return false;
    }

    uint8_t cid = p.ReadU8();
    int8_t worldID = p.ReadS8();

    auto client = std::dynamic_pointer_cast<LobbyClientConnection>(connection);
    auto state = client->GetClientState();
    auto username = state->GetAccount()->GetUsername();

    auto account = state->GetAccount();

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto world = server->GetWorldByID((uint8_t)worldID);
    auto accountManager = server->GetAccountManager();

    int8_t loginWorldID;
    if(!accountManager->IsLoggedIn(username, loginWorldID) || loginWorldID != -1)
    {
        LOG_ERROR(libcomp::String("User '%1' attempted to start a game but is not"
            " currently logged into the lobby.\n").Arg(username));
        return false;
    }

    // Expire session with current time, meaning until we hear back from the
    // channel that the login took place, do not consider the connection valid
    server->GetSessionManager()->ExpireSession(username, 0);

    auto login = accountManager->GetUserLogin(username);
    login->GetCharacterLogin()->SetCharacter(account->GetCharacters(cid));

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);
    login->SavePacket(request, false);

    world->GetConnection()->SendPacket(request);

    return true;
}
