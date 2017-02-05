/**
 * @file server/lobby/src/packets/game/CharacterList.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to return the lobby client's character list.
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
#include <Account.h>
#include <Character.h>
#include <EntityStats.h>
#include <Item.h>

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

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto accountManager = server->GetAccountManager();
    auto lobbyDB = server->GetMainDatabase();
    auto lobbyConnection = std::dynamic_pointer_cast<LobbyClientConnection>(connection);
    auto account = lobbyConnection->GetClientState()->GetAccount().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyToClientPacketCode_t::PACKET_CHARACTER_LIST);

    // Time of last login (time_t).
    reply.WriteU32Little((uint32_t)time(0));

    // Number of character tickets.
    reply.WriteU8(account->GetTicketCount());

    std::list<std::shared_ptr<objects::Character>> characters;
    for(auto world : server->GetWorlds())
    {
        if(world->GetRegisteredWorld()->GetStatus()
            == objects::RegisteredWorld::Status_t::INACTIVE) continue;

        auto worldDB = world->GetWorldDatabase();
        auto characterList = objects::Character::LoadCharacterListByAccount(worldDB, account);
        for(auto character : characterList)
        {
            bool loaded = character->LoadCoreStats(worldDB) != nullptr;

            if(loaded)
            {
                for(auto equip : character->GetEquippedItems())
                {
                    loaded &= equip.IsNull() || equip.Get(worldDB) != nullptr;
                    if(!loaded) break;
                }
            }

            if(!loaded)
            {
                LOG_ERROR(libcomp::String("Character CID %1 could not be loaded fully.\n")
                    .Arg(character->GetCID()));
            }
            else
            {
                characters.push_back(character);
            }
        }
    }
    characters.sort([](const std::shared_ptr<objects::Character>& a,
        const std::shared_ptr<objects::Character>& b) { return a->GetCID() < b->GetCID(); });

    //Correct the character array if needed
    if(characters.size() > 0)
    {
        bool updated = false;
        for(auto character : characters)
        {
            auto cid = character->GetCID();
            auto existing = account->GetCharacters(cid).Get();
            if(existing != nullptr)
            {
                if(existing->GetUUID() != character->GetUUID())
                {
                    LOG_ERROR(libcomp::String("Duplicate CID %1 encountered for account %2\n")
                        .Arg(cid)
                        .Arg(account->GetUUID().ToString()));
                    return false;
                }
            }
            else
            {
                account->SetCharacters(cid, character);
                updated = true;
            }
        }

        if(updated)
        {
            if(!account->Update(lobbyDB))
            {
                LOG_ERROR(libcomp::String("Account character map failed to save %1\n")
                    .Arg(account->GetUUID().ToString()));
                return false;
            }
        }
    }

    auto cidsToDelete = accountManager->GetCharactersForDeletion(account->GetUsername());

    characters.remove_if([cidsToDelete]
        (const std::shared_ptr<objects::Character>& character)
        {
            return std::find(cidsToDelete.begin(), cidsToDelete.end(), character->GetCID())
                != cidsToDelete.end();
        });

    // Number of characters.
    reply.WriteU8(static_cast<uint8_t>(characters.size()));

    for(auto character : characters)
    {
        auto stats = character->GetCoreStats();

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
        reply.WriteS8(stats->GetLevel());

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

        // Unknown values.
        reply.WriteU8(0);
        reply.WriteU8(1);

        // Equipment
        for(size_t i = 0; i < 15; i++)
        {
            auto equip = character->GetEquippedItems(i);

            if(!equip.IsNull())
            {
                reply.WriteU32Little(equip->GetType());
            }
            else
            {
                // None.
                reply.WriteU32Little(static_cast<uint32_t>(-1));
            }
        }

        // Unknown value
        reply.WriteBlank(4);
    }

    connection->SendPacket(reply);

    for(auto cid : cidsToDelete)
    {
        server->QueueWork([](const libcomp::String username,
            uint8_t c, std::shared_ptr<LobbyServer> s)
        {
            s->GetAccountManager()->DeleteCharacter(username,
                c, s);

        }, account->GetUsername(), cid, server);
    }

    return true;
}
