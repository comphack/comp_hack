/**
 * @file server/channel/src/packets/internal/ClanUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Parser to handle all clan focused actions between the world
 *  and the channel.
 *
 * This file is part of the Channel Server (channel).
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
#include <ActivatedAbility.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <Clan.h>
#include <Item.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ManagerConnection.h"
#include "SkillManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::ClanUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    if(p.Size() < 1)
    {
        LogClanErrorMsg("Invalid response received for ClanUpdate.\n");

        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    bool connectionsFound = false;
    uint8_t mode = p.ReadU8();

    auto clients = server->GetManagerConnection()
        ->GatherWorldTargetClients(p, connectionsFound);
    if(!connectionsFound)
    {
        LogClanErrorMsg("Connections not found for ClanUpdate.\n");

        return false;
    }

    switch((InternalPacketAction_t)mode)
    {
    case InternalPacketAction_t::PACKET_ACTION_ADD:
        // New clan formation
        if(clients.size() == 1)
        {
            auto client = clients.front();
            auto state = client->GetClientState();
            auto cState = state->GetCharacterState();
            auto character = cState->GetEntity();

            int32_t clanID = p.ReadS32Little();
            state->GetAccountLogin()->GetCharacterLogin()
                ->SetClanID(clanID);

            int8_t errorCode = 0;
            if(!clanID)
            {
                // Send generic failure
                errorCode = -5;
            }

            libcomp::Packet response;
            response.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_FORM);
            response.WriteS32Little(clanID);
            response.WriteS8(errorCode);

            client->SendPacket(response);

            // Execute or cancel the skill
            int8_t activationID = p.ReadS8();
            auto activatedAbility = cState->GetSpecialActivations(activationID);

            if(activatedAbility)
            {
                if(errorCode == 0)
                {
                    server->GetSkillManager()->ExecuteSkill(cState,
                        activationID, activatedAbility->GetActivationObjectID());
                }
                else
                {
                    server->GetSkillManager()->CancelSkill(cState, activationID);
                }
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_UPDATE:
        // Visible information updated
        {
            auto characterManager = server->GetCharacterManager();
            auto zoneManager = server->GetZoneManager();

            uint8_t updateFlags = p.ReadU8();

            // Always reload the clan
            auto worldDB = server->GetWorldDatabase();
            libobjgen::UUID uid(p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true).C());
            auto clan = !uid.IsNull() ? libcomp::PersistentObject::LoadObjectByUUID<
                objects::Clan>(worldDB, uid, true) : nullptr;

            bool nameUpdated = (updateFlags & 0x01) != 0;
            bool emblemUpdated = (updateFlags & 0x02) != 0;
            bool levelUpdated = (updateFlags & 0x04) != 0;
            bool clanUpdated = (updateFlags & 0x08) != 0;

            libcomp::String name;
            if(nameUpdated)
            {
                name = p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
            }

            std::vector<char> emblemDef;
            if(emblemUpdated)
            {
                emblemDef = p.ReadArray(8);
            }

            int8_t level = -1;
            if(levelUpdated)
            {
                level = p.ReadS8();
            }

            if(clanUpdated)
            {
                int32_t clanID = p.ReadS32Little();
                for(auto client : clients)
                {
                    auto state = client->GetClientState();
                    auto cState = state->GetCharacterState();
                    state->GetAccountLogin()->GetCharacterLogin()->SetClanID(clanID);

                    // The world will have already saved this but save again so we don't
                    // get into a weird state
                    auto character = cState->GetEntity();
                    character->SetClan(clan);
                    worldDB->QueueUpdate(character);

                    characterManager->RecalculateTokuseiAndStats(cState, client);
                }
            }

            std::set<std::shared_ptr<Zone>> zones;
            for(auto client : clients)
            {
                int8_t selfFlags = (int8_t)((emblemUpdated ? 0x04 : 0) | (levelUpdated ? 0x08 : 0));

                auto state = client->GetClientState();
                if(state->GetClanID() && selfFlags != 0)
                {
                    // Updates must be sent to the client in a different format
                    libcomp::Packet request;
                    request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_UPDATE);
                    request.WriteS32Little(state->GetClanID());
                    request.WriteS8(selfFlags);
                    if(emblemUpdated)
                    {
                        request.WriteArray(emblemDef);
                    }

                    if(levelUpdated)
                    {
                        request.WriteS8(level);
                    }

                    client->QueuePacket(request);
                }

                auto cState = state->GetCharacterState();
                if(nameUpdated)
                {
                    libcomp::Packet request;
                    request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_NAME_UPDATED);
                    request.WriteS32Little(cState->GetEntityID());
                    request.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                        name, true);

                    zoneManager->BroadcastPacket(client, request);
                }

                if(emblemUpdated)
                {
                    libcomp::Packet request;
                    request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_EMBLEM_UPDATED);
                    request.WriteS32Little(cState->GetEntityID());
                    request.WriteArray(emblemDef);

                    zoneManager->BroadcastPacket(client, request);
                }

                if(levelUpdated)
                {
                    libcomp::Packet request;
                    request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_LEVEL_UPDATED);
                    request.WriteS32Little(cState->GetEntityID());
                    request.WriteS8(level);

                    zoneManager->BroadcastPacket(client, request);

                    if(!clanUpdated)
                    {
                        characterManager->RecalculateTokuseiAndStats(cState, client);
                    }
                }
            }
        }
        break;
    default:
        break;
    }

    return true;
}
