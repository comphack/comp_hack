/**
 * @file server/lobby/src/packets/Login.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to handle lobby packets.
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
#include <LobbyServer.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>

// Lobby Includes
#include "LobbyClientConnection.h"

using namespace lobby;

bool Parsers::CharacterList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyClientPacketCode_t::PACKET_CHARACTER_LIST_RESPONSE);

    // Time of last login (time_t).
    reply.WriteU32Little((uint32_t)time(0));

    // Number of character tickets.
    reply.WriteU8(1);

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto lobbyConnection = std::dynamic_pointer_cast<LobbyClientConnection>(connection);
    auto account = lobbyConnection->GetClientState()->GetAccount();

    std::list<std::shared_ptr<objects::Character>> characters;
    for(auto world : server->GetWorlds())
    {
        auto worldDB = world->GetWorldDatabase();
        auto characterList = objects::Character::LoadCharacterListByAccount(worldDB, account);
        for(auto character : characterList)
        {
            characters.push_back(character);
        }
    }

    // Number of characters.
    reply.WriteU8(static_cast<uint8_t>(characters.size()));

    for(auto character : characters)
    {
        // Character ID.
        reply.WriteU8(character->GetCID());

        // World ID.
        reply.WriteU8(character->GetWorldID());

        // Name.
        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            character->GetName(), true);

        // Gender.
        reply.WriteU8((uint8_t)character->GetGender());

        // Time when the character will be deleted.
        reply.WriteU32Little(character->GetKillTime());

        // Cutscene to play on login (0 for none).
        reply.WriteU32Little(0x001EFC77);

        // Last channel used???
        reply.WriteS8(-1);

        // Level.
        reply.WriteU8(character->GetLevel());

        // Skin type.
        reply.WriteU8(character->GetSkinType());

        // Hair type.
        reply.WriteU8(character->GetHairType());

        // Eye type.
        reply.WriteU8(character->GetEyeType());

        // Face type.
        reply.WriteU8(character->GetFaceType());

        // Hair color.
        reply.WriteU8(character->GetHairColor());

        // Left eye color.
        reply.WriteU8(character->GetLeftEyeColor());

        // Right eye color.
        reply.WriteU8(character->GetRightEyeColor());

        // Unkown values.
        reply.WriteU8(0);
        reply.WriteU8(1);

        // Equipment
        for(int z = 0; z < 15; z++)
        {
            // None.
            reply.WriteU32Little(0x7FFFFFFF);
        }
    }

    connection->SendPacket(reply);

    return true;
}
