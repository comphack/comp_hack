/**
 * @file server/lobby/src/packets/game/DeleteCharacter.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to handle the lobby request to delete a character.
 *
 * This file is part of the Lobby Server (lobby).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Account.h>
#include <Character.h>

// lobby Includes
#include "AccountManager.h"
#include "LobbyClientConnection.h"
#include "LobbyServer.h"
#include "ManagerPacket.h"

using namespace lobby;

bool Parsers::DeleteCharacter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    uint8_t cid = p.ReadU8();

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto config = std::dynamic_pointer_cast<objects::LobbyConfig>(server->GetConfig());
    auto lobbyConnection = std::dynamic_pointer_cast<LobbyClientConnection>(connection);
    auto account = lobbyConnection->GetClientState()->GetAccount();

    //Every character should have already been loaded by the CharacterList
    auto deleteCharacter = account->GetCharacters(cid);
    if(deleteCharacter.Get())
    {
        auto world = server->GetWorldByID(deleteCharacter->GetWorldID());
        auto worldDB = world->GetWorldDatabase();

        server->QueueWork([](const std::shared_ptr<LobbyClientConnection> client,
            uint8_t c, std::shared_ptr<LobbyServer> s)
        {
            libcomp::Packet reply;
            reply.WritePacketCode(
                LobbyToClientPacketCode_t::PACKET_DELETE_CHARACTER);

            if(s->GetAccountManager()->UpdateKillTime(
                client->GetClientState()->GetAccount()->GetUsername(), c, s))
            {
                reply.WriteS8((int8_t)c);
            }
            else
            {
                //Send failure
                reply.WriteS8(-1);
            }

            client->SendPacket(reply);
        }, lobbyConnection, cid, server);
    }
    else
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Client tried to delete character with "
                "invalid CID %1\n").Arg(cid);
        });

        connection->Close();
    }

    return true;
}
