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
#include "AccountManager.h"
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
    auto config = std::dynamic_pointer_cast<objects::LobbyConfig>(server->GetConfig());
    auto accountManager = server->GetAccountManager();
    auto lobbyDB = server->GetMainDatabase();
    auto lobbyConnection = std::dynamic_pointer_cast<LobbyClientConnection>(connection);
    auto account = lobbyConnection->GetClientState()->GetAccount().Get();

    std::set<std::shared_ptr<objects::Character>> characters;
    for(auto world : server->GetWorlds())
    {
        if(world->GetRegisteredWorld()->GetStatus()
            == objects::RegisteredWorld::Status_t::INACTIVE) continue;

        auto worldDB = world->GetWorldDatabase();
        auto characterList = objects::Character::LoadCharacterListByAccount(
            worldDB, account->GetUUID());
        for(auto character : characterList)
        {
            // Always reload
            bool loaded = character->GetCoreStats().Get(worldDB, true) != nullptr;

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
                LOG_ERROR(libcomp::String("Character could not be loaded"
                    " fully: %1\n").Arg(character->GetUUID().ToString()));
            }
            else
            {
                characters.insert(character);
            }
        }
    }

    auto deletes = accountManager->GetCharactersForDeletion(account);
    if(deletes.size() > 0)
    {
        // Handle deletes before continuing
        for(auto deleteChar : deletes)
        {
            accountManager->DeleteCharacter(account, deleteChar);
            characters.erase(deleteChar);
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyToClientPacketCode_t::PACKET_CHARACTER_LIST);

    // Time of last login.
    reply.WriteU32Little(account->GetLastLogin());

    // Number of character tickets.
    reply.WriteU8(account->GetTicketCount());

    // Double back later and write the number of characters afer they
    // have been successfully written to the packet
    uint8_t charCount = 0;
    reply.WriteU8(0);

    for(uint8_t cid = 0; cid < MAX_CHARACTER; cid++)
    {
        auto character = account->GetCharacters(cid).Get();

        // Skip if the character is not in a connected world or otherwise
        // not loaded
        if(!character) continue;

        auto stats = character->GetCoreStats().Get();

        if(!stats)
        {
            LOG_ERROR(libcomp::String("Character was loaded but stats are no"
                " longer loaded: %1\n").Arg(character->GetUUID().ToString()));
            continue;
        }

        // Increase the count to write to the packet
        charCount++;

        // Character ID.
        reply.WriteU8(cid);

        // World ID.
        reply.WriteU8(character->GetWorldID());

        // Name.
        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            character->GetName(), true);

        // Gender.
        reply.WriteU8((uint8_t)character->GetGender());

        // Time when the character will be deleted.
        reply.WriteU32Little(character->GetKillTime());

        auto level = stats->GetLevel();

        // Total play time? (0 shows opening cutscene)
        /// @todo: verify/implement properly
        reply.WriteU32Little(level == -1 && config->GetPlayOpeningMovie()
            ? 0 : 1);

        // Last channel used???
        reply.WriteS8(-1);

        // Level.
        reply.WriteS8(static_cast<int8_t>(level != -1 ? level : 1));

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

        // VA
        reply.WriteS32Little((int32_t)character->EquippedVACount());
        for(uint8_t i = 0; i <= MAX_VA_INDEX; i++)
        {
            uint32_t va = character->GetEquippedVA(i);
            if(va)
            {
                reply.WriteS8((int8_t)i);
                reply.WriteU32Little(va);
            }
        }
    }

    // Now write the character count
    reply.Seek(7);
    reply.WriteU8(charCount);

    connection->SendPacket(reply);

    return true;
}
