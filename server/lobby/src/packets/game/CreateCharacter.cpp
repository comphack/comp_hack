/**
 * @file server/lobby/src/packets/game/CreateCharacter.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to handle the lobby request to create a character.
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
#include <ErrorCodes.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Account.h>
#include <Character.h>
#include <EntityStats.h>
#include <Item.h>
#include <MiItemBasicData.h>

// lobby Includes
#include "AccountManager.h"
#include "LobbyClientConnection.h"
#include "LobbyServer.h"
#include "ManagerPacket.h"

using namespace lobby;

bool Parsers::CreateCharacter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 45)
    {
        return false;
    }

    uint8_t worldID = p.ReadU8();

    if(p.Size() != (uint32_t)(p.PeekU16Little() + 44))
    {
        return false;
    }

    libcomp::String name = p.ReadString16Little(
        libcomp::Convert::ENCODING_CP932);

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto lobbyConnection = std::dynamic_pointer_cast<LobbyClientConnection>(connection);
    auto world = server->GetWorldByID(worldID);

    if(!world)
    {
        LOG_ERROR(libcomp::String("Tried to create character on world with "
            "ID %1 but that world was not found.\n").Arg(worldID));

        return false;
    }

    auto worldDB = world->GetWorldDatabase();
    auto account = lobbyConnection->GetClientState()->GetAccount().Get();
    auto characters = account->GetCharacters();

    uint8_t nextCID = 0;
    for(size_t i = 0; i < characters.size(); i++)
    {
        if(characters[nextCID].IsNull())
        {
            break;
        }

        nextCID++;
    }

    uint8_t ticketCount = account->GetTicketCount();

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyToClientPacketCode_t::PACKET_CREATE_CHARACTER);

    if(nextCID == characters.size())
    {
        LOG_ERROR(libcomp::String("No new characters can be created for account %1\n")
            .Arg(account->GetUUID().ToString()));
        reply.WriteU32Little(static_cast<uint32_t>(ErrorCodes_t::NO_EMPTY_CHARACTER_SLOTS));
    }
    else if(ticketCount == 0)
    {
        LOG_ERROR(libcomp::String("No character tickets available for account %1\n")
            .Arg(account->GetUUID().ToString()));
        reply.WriteU32Little(static_cast<uint32_t>(ErrorCodes_t::NEED_CHARACTER_TICKET));
    }
    else if(nullptr != objects::Character::LoadCharacterByName(worldDB, name))
    {
        LOG_ERROR(libcomp::String("Invalid character name entered for account %1\n")
            .Arg(account->GetUUID().ToString()));
        reply.WriteU32Little(static_cast<uint32_t>(ErrorCodes_t::BAD_CHARACTER_NAME));
    }
    else
    {
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
        character->Register(character);

        std::unordered_map<size_t, std::shared_ptr<objects::Item>> equipMap;

        /// @todo: Build these properly from item data
        auto itemTop = libcomp::PersistentObject::New<objects::Item>();
        itemTop->SetType(equipTop);
        equipMap[(size_t)
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP] = itemTop;

        auto itemBottom = libcomp::PersistentObject::New<objects::Item>();
        itemBottom->SetType(equipBottom);
        equipMap[(size_t)
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM] = itemBottom;

        auto itemFeet = libcomp::PersistentObject::New<objects::Item>();
        itemFeet->SetType(equipFeet);
        equipMap[(size_t)
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FEET] = itemFeet;

        auto comp = libcomp::PersistentObject::New<objects::Item>();
        comp->SetType(equipComp);
        equipMap[(size_t)
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_COMP] = comp;

        auto weapon = libcomp::PersistentObject::New<objects::Item>();
        weapon->SetType(equipWeapon);
        equipMap[(size_t)
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON] = weapon;

        auto stats = libcomp::PersistentObject::New<objects::EntityStats>();
        stats->Register(stats);
        stats->SetEntity(std::dynamic_pointer_cast<
            libcomp::PersistentObject>(character));
        character->SetCoreStats(stats);

        bool equipped = true;
        for(auto pair : equipMap)
        {
            auto equip = pair.second;
            equipped &= equip->Register(equip) && equip->Insert(worldDB) &&
                character->SetEquippedItems(pair.first, equip);
        }

        if(!equipped)
        {
            LOG_ERROR(libcomp::String("Character item data failed to save for account %1\n")
                .Arg(account->GetUUID().ToString()));
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }
        else if(!stats->Insert(worldDB) || !character->Insert(worldDB))
        {
            LOG_ERROR(libcomp::String("Character failed to save for account %1\n")
                .Arg(account->GetUUID().ToString()));
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }
        else if(!account->SetTicketCount((uint8_t)(ticketCount - 1)) ||
            !server->GetAccountManager()->SetCharacterOnAccount(account, character))
        {
            account->SetTicketCount(ticketCount);   // Put the ticket back
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }
        else
        {
            reply.WriteU32Little(0);

            LOG_DEBUG(libcomp::String("Created character '%1' on world: %2\n")
                .Arg(name).Arg(worldID));
        }
    }

    connection->SendPacket(reply);

    return true;
}
