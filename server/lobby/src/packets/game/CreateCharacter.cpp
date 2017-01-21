/**
 * @file server/lobby/src/packets/CreateCharacter.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to handle the lobby request to create a character.
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
#include <Character.h>

// lobby Includes
#include "LobbyClientConnection.h"
#include "LobbyServer.h"
#include "ManagerPacket.h"

using namespace lobby;

bool Parsers::CreateCharacter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() < 45)
    {
        return false;
    }

    uint8_t worldID = p.ReadU8();

    LOG_DEBUG(libcomp::String("World: %1\n").Arg(worldID));

    if(p.Size() != (uint32_t)(p.PeekU16Little() + 44))
    {
        return false;
    }

    libcomp::String name = p.ReadString16Little(
        libcomp::Convert::ENCODING_CP932);

    LOG_DEBUG(libcomp::String("Name: %1\n").Arg(name));

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto lobbyConnection = std::dynamic_pointer_cast<LobbyClientConnection>(connection);
    auto lobbyDB = server->GetMainDatabase();
    auto world = server->GetWorldByID(worldID);
    auto worldDB = world->GetWorldDatabase();
    auto account = lobbyConnection->GetClientState()->GetAccount().Get();
    auto charactersByCID = account->GetCharactersByCID();

    uint8_t nextCID = 0;
    for(size_t i = 0; i < charactersByCID.size(); i++)
    {
        if(charactersByCID.find(nextCID) == charactersByCID.end())
        {
            break;
        }

        nextCID++;
    }

    auto gender = (objects::Character::Gender_t)p.ReadU8();

    uint32_t skinType  = p.ReadU32Little();
    uint32_t faceType  = p.ReadU32Little();
    uint32_t hairType  = p.ReadU32Little();
    uint32_t hairColor = p.ReadU32Little();
    uint32_t eyeColor  = p.ReadU32Little();

    uint32_t equipTop    = p.ReadU32Little();
    uint32_t equipBottom = p.ReadU32Little();
    uint32_t equipFeet   = p.ReadU32Little();
    uint32_t equipComp   = p.ReadU32Little();
    uint32_t equipWeapon = p.ReadU32Little();

    uint8_t eyeType = (uint8_t)(gender == objects::Character::Gender_t::MALE ? 1 : 101);

    auto character = libcomp::PersistentObject::New<objects::Character>();
    character->SetCID(nextCID);
    character->SetName(name);
    character->SetGender(gender);
    character->SetSkinType((uint8_t)skinType);
    character->SetFaceType((uint8_t)faceType);
    character->SetHairType((uint8_t)hairType);
    character->SetHairColor((uint8_t)hairColor);
    character->SetEyeType((uint8_t)eyeType);
    character->SetLeftEyeColor((uint8_t)eyeColor);
    character->SetRightEyeColor((uint8_t)eyeColor);
    character->SetAccount(account);

    /// @todo
    (void)equipTop;
    (void)equipBottom;
    (void)equipFeet;
    (void)equipComp;
    (void)equipWeapon;

    if(!character->Register(character) || !character->Insert(worldDB))
    {
        LOG_DEBUG("Character failed to save.\n");
        return false;
    }

    charactersByCID[character->GetCID()] = character;
    account->SetCharactersByCID(charactersByCID);
    if (!account->Update(lobbyDB))
    {
        LOG_ERROR(libcomp::String("Account character map failed to save for account %1\n")
            .Arg(account->GetUUID().ToString()));
        return false;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyClientPacketCode_t::PACKET_CREATE_CHARACTER_RESPONSE);
    reply.WriteU32Little(0);

    connection->SendPacket(reply);

    return true;
}
