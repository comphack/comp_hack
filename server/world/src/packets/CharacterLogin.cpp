/**
 * @file server/world/src/packets/CharacterLogin.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle communicating character login information from
 *  the world to the channels.
 *
 * This file is part of the World Server (world).
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// world Includes
#include "CharacterManager.h"
#include "WorldServer.h"
#include "WorldSyncManager.h"

using namespace world;

bool Parsers::CharacterLogin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 5)
    {
        LogGeneralErrorMsg("Invalid packet data sent to CharacterLogin\n");

        return false;
    }

    int32_t cid = p.ReadS32Little();
    uint8_t updateFlags = p.ReadU8();

    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
    auto cLogin = server->GetCharacterManager()->GetCharacterLogin(cid);
    auto characterManager = server->GetCharacterManager();
    if(!cLogin)
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Invalid world CID sent to "
                "CharacterLogin: %1\n").Arg(cid);
        });

        return false;
    }

    if(!updateFlags)
    {
        // Special "channel refresh" request, send party/team info if still
        // in either
        if(cLogin->GetPartyID())
        {
            auto member = characterManager->GetPartyMember(cLogin
                ->GetWorldCID());
            if(member)
            {
                characterManager->SendPartyMember(member, cLogin->GetPartyID(),
                    false, true, connection);
            }
        }

        if(cLogin->GetTeamID())
        {
            characterManager->SendTeamInfo(cLogin->GetTeamID(),
                { cLogin->GetWorldCID() });
        }

        return true;
    }

    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_STATUS)
    {
        if(p.Left() < 1)
        {
            LogGeneralErrorMsg(
                "CharacterLogin status update sent with no status specified\n");

            return false;
        }

        int8_t statusID = p.ReadS8();
        cLogin->SetStatus((objects::CharacterLogin::Status_t)statusID);
    }

    uint32_t previousZoneID = cLogin->GetZoneID();
    int8_t previousChannelID = cLogin->GetChannelID();
    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE)
    {
        if(p.Left() < 4)
        {
            LogGeneralErrorMsg(
                "CharacterLogin zone update sent with no zone specified\n");

            return false;
        }

        uint32_t zoneID = p.ReadU32Little();
        cLogin->SetZoneID(zoneID);

        // If the character is going from no zone to some zone, reload
        // to pick up any channel login changes
        if(zoneID && !previousZoneID)
        {
            cLogin->GetCharacter().Get(server->GetWorldDatabase(), true);
        }
    }

    auto member = characterManager->GetPartyMember(cLogin->GetWorldCID());
    if(member)
    {
        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO)
        {
            if(!member->LoadPacket(p, true))
            {
                LogGeneralErrorMsg(
                    "CharacterLogin party member info failed to load\n");

                return false;
            }
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO)
        {
            auto partyDemon = member->GetDemon();
            if(!partyDemon->LoadPacket(p, true))
            {
                LogGeneralErrorMsg(
                    "CharacterLogin party demon info failed to load\n");

                return false;
            }
        }
    }

    // Everything has been updated on the world, figure out what to send back
    bool partyMove = updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE &&
        cLogin->GetPartyID();

    // Send all party flags if in a party and changing zones to members in that zone
    if(partyMove)
    {
        updateFlags = updateFlags | (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_FLAGS;
    }

    // Send the updates
    std::list<std::shared_ptr<objects::CharacterLogin>> logins = { cLogin };
    characterManager->SendStatusToRelatedCharacters(logins, updateFlags, true);

    // If changing zones and in a party, update party member visibility (inverse of previous)
    if(partyMove)
    {
        auto partyMembers = characterManager->GetRelatedCharacterLogins(
            cLogin, RELATED_PARTY);

        // Send all up to date info on party members in the zone they just entered or the left
        bool queued = false;
        for(auto login : partyMembers)
        {
            bool previousZone = login->GetZoneID() == previousZoneID &&
                login->GetChannelID() == previousChannelID;
            if((login->GetZoneID() != cLogin->GetZoneID() ||
                login->GetChannelID() != cLogin->GetChannelID()) && !previousZone)
            {
                continue;
            }

            uint8_t outFlags = previousZone
                ? ((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE
                    | (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO)
                : (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_FLAGS;

            libcomp::Packet reply;
            if(characterManager->GetStatusPacket(reply, login, outFlags) &&
                (previousZone || outFlags != (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE))
            {
                characterManager->ConvertToTargetCIDPacket(reply, 1, 1);
                reply.WriteS32Little(cLogin->GetWorldCID());
                connection->QueuePacket(reply);
                queued = true;
            }
        }

        if(queued)
        {
            connection->FlushOutgoing();
        }
    }

    // Sync with everyone else
    server->GetWorldSyncManager()->SyncRecordUpdate(cLogin, "CharacterLogin");

    return true;
}
