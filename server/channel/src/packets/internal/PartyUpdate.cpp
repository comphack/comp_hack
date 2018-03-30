/**
 * @file server/channel/src/packets/internal/PartyUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Parser to handle all party focused actions between the world
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
#include <Character.h>
#include <CharacterLogin.h>
#include <Party.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"
#include "TokuseiManager.h"

using namespace channel;

struct PartyMemberInfo
{
    std::shared_ptr<objects::PartyCharacter> Member;
    uint32_t ZoneID;
    bool IsLeader;
};

void QueuePartyMemberInfo(std::shared_ptr<ChannelClientConnection> client,
    PartyMemberInfo memberInfo)
{
    auto state = client->GetClientState();
    auto cLogin = state->GetAccountLogin()->GetCharacterLogin();
    auto member = memberInfo.Member;
    auto partyDemon = member->GetDemon();

    auto partyState = ClientState::GetEntityClientState(
        member->GetWorldCID(), true);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_INFO);
    if(memberInfo.ZoneID == cLogin->GetZoneID())
    {
        int32_t localEntityID = -1;
        int32_t localDemonEntityID = -1;
        if(partyState)
        {
            localEntityID = partyState->GetCharacterState()->GetEntityID();
            if(partyState->GetDemonState()->GetEntity())
            {
                localDemonEntityID = partyState->GetDemonState()->GetEntityID();
            }
        }

        reply.WriteS32Little(localEntityID);
        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            member->GetName(), true);
        reply.WriteU8(memberInfo.IsLeader ? 1 : 0);
        reply.WriteU8(member->GetLevel());
        reply.WriteU16Little((uint16_t)member->GetHP());
        reply.WriteU16Little((uint16_t)member->GetMaxHP());
        reply.WriteU16Little((uint16_t)member->GetMP());
        reply.WriteU16Little((uint16_t)member->GetMaxMP());

        // Seemingly unused, maybe this was previously status effects?
        int8_t unusedCount = 0;
        reply.WriteS8(unusedCount);
        for(int8_t i = 0; i < unusedCount; i++)
        {
            reply.WriteS32Little(0);
        }

        reply.WriteS32Little(localDemonEntityID);
        reply.WriteU32Little(partyDemon->GetDemonType());
        reply.WriteU16Little((uint16_t)partyDemon->GetHP());
        reply.WriteU16Little((uint16_t)partyDemon->GetMaxHP());

        reply.WriteS32Little((int32_t)memberInfo.ZoneID);

        /// @todo: figure out face icon values
        reply.WriteU8(0);
        reply.WriteU8(0);
        reply.WriteU8(0);
        reply.WriteU8(0);
        reply.WriteS8(0);
    }
    else
    {
        // Not in the same zone, send minimal info
        reply.WriteS32Little(-1);
        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            member->GetName(), true);
        reply.WriteU8(memberInfo.IsLeader ? 1 : 0);
        reply.WriteBlank(10);
        reply.WriteS32Little(-1);   // Demon entity ID
        reply.WriteU32Little(partyDemon->GetDemonType());
        reply.WriteBlank(4);
        reply.WriteS32Little((int32_t)memberInfo.ZoneID);
        reply.WriteBlank(5);
    }

    reply.WriteS32Little(member->GetWorldCID());

    client->QueuePacket(reply);
}

bool Parsers::PartyUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    if(p.Size() < 3)
    {
        LOG_ERROR("Invalid response received for PartyUpdate.\n");
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    bool connectionsFound = false;
    uint8_t mode = p.ReadU8();

    auto clients = server->GetManagerConnection()
        ->GatherWorldTargetClients(p, connectionsFound);
    if(!connectionsFound)
    {
        LOG_ERROR("Connections not found for CharacterLogin.\n");
        return false;
    }

    // The only packet types that can't be relayed directly from the world
    // are the local update and ones that require transformations to
    // local entity IDs
    switch((InternalPacketAction_t)mode)
    {
    case InternalPacketAction_t::PACKET_ACTION_ADD:
        {
            std::list<PartyMemberInfo> members;

            uint32_t partyID = p.ReadU32Little();
            uint8_t memberCount = p.ReadU8();
            for(uint8_t i = 0; i < memberCount; i++)
            {
                auto member = std::make_shared<objects::PartyCharacter>();
                if(!member->LoadPacket(p, false) || p.Left() < 5)
                {
                    return false;
                }

                uint32_t zoneID = p.ReadU32Little();
                bool leader = p.ReadU8() == 1;

                PartyMemberInfo info;
                info.Member = member;
                info.ZoneID = zoneID;
                info.IsLeader = leader;
                members.push_back(info);
            }
            
            for(auto client : clients)
            {
                if(client->GetClientState()->GetPartyID() == partyID)
                {
                    for(auto info : members)
                    {
                        QueuePartyMemberInfo(client, info);
                    }
                    client->FlushOutgoing();
                }
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_UPDATE:
        {
            uint32_t partyID = p.ReadU32Little();
            uint8_t exists = p.ReadU8() == 1;

            auto party = exists
                ? std::make_shared<objects::Party>() : nullptr;
            if(party && !party->LoadPacket(p))
            {
                return false;
            }

            for(auto client : clients)
            {
                auto state = client->GetClientState();
                if(party && party->MemberIDsContains(state->GetWorldCID()))
                {
                    // Adding/updating
                    state->GetAccountLogin()->GetCharacterLogin()
                        ->SetPartyID(partyID);
                    state->SetParty(party);
                }
                else if(state->GetPartyID() == partyID)
                {
                    // Removing
                    state->GetAccountLogin()->GetCharacterLogin()
                        ->SetPartyID(0);
                    state->SetParty(nullptr);
                }
            }

            // Recalculate all tokusei effects
            auto tokuseiManager = server->GetTokuseiManager();

            std::set<int32_t> recalcEntities;
            for(auto client : clients)
            {
                auto cState = client->GetClientState()->GetCharacterState();
                if(recalcEntities.find(cState->GetEntityID()) == recalcEntities.end())
                {
                    for(auto pair : tokuseiManager->Recalculate(cState, true))
                    {
                        recalcEntities.insert(pair.first);
                    }
                }
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE:
        {
            bool isResponse = p.ReadU8() == 1;

            if(!isResponse)
            {
                if(p.Left() != 4)
                {
                    return false;
                }

                int32_t worldCID = p.ReadS32Little();

                auto partyState = ClientState::GetEntityClientState(worldCID, true);

                int32_t localEntityID = -1;
                if(partyState)
                {
                    localEntityID = partyState->GetCharacterState()->GetEntityID();
                }

                libcomp::Packet request;
                request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_LEFT);
                request.WriteS32Little(localEntityID);
                request.WriteS32Little(worldCID);

                ChannelClientConnection::BroadcastPacket(clients, request);
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEADER_UPDATE:
        {
            bool isResponse = p.ReadU8() == 1;

            if(!isResponse)
            {
                if(p.Left() != 4)
                {
                    return false;
                }

                int32_t leaderCID = p.ReadS32Little();

                auto partyState = ClientState::GetEntityClientState(leaderCID, true);

                int32_t localEntityID = -1;
                if(partyState)
                {
                    localEntityID = partyState->GetCharacterState()->GetEntityID();
                }

                libcomp::Packet request;
                request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_LEADER_UPDATED);
                request.WriteS32Little(localEntityID);
                request.WriteS32Little(leaderCID);

                ChannelClientConnection::BroadcastPacket(clients, request);
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_KICK:
        {
            if(p.Left() != 4)
            {
                return false;
            }

            int32_t targetCID = p.ReadS32Little();

            auto partyState = ClientState::GetEntityClientState(targetCID, true);

            int32_t localEntityID = -1;
            if(partyState)
            {
                localEntityID = partyState->GetCharacterState()->GetEntityID();
            }

            libcomp::Packet request;
            request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_KICK);
            request.WriteS32Little(localEntityID);
            request.WriteS32Little(targetCID);

            ChannelClientConnection::BroadcastPacket(clients, request);

            request.Clear();
            request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_LEFT);
            request.WriteS32Little(localEntityID);
            request.WriteS32Little(targetCID);

            ChannelClientConnection::BroadcastPacket(clients, request);
        }
        break;
    default:
        break;
    }

    return true;
}
