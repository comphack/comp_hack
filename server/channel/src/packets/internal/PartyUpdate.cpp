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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// channel Includes
#include "ChannelServer.h"

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
        reply.WriteU16Little(member->GetHP());
        reply.WriteU16Little(member->GetMaxHP());
        reply.WriteU16Little(member->GetMP());
        reply.WriteU16Little(member->GetMaxMP());

        // Seemingly unused, maybe this was previously status effects?
        int8_t unusedCount = 0;
        reply.WriteS8(unusedCount);
        for(int8_t i = 0; i < unusedCount; i++)
        {
            reply.WriteS32Little(0);
        }

        reply.WriteS32Little(localDemonEntityID);
        reply.WriteU32Little(partyDemon->GetDemonType());
        reply.WriteU16Little(partyDemon->GetHP());
        reply.WriteU16Little(partyDemon->GetMaxHP());

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

    switch((InternalPacketAction_t)mode)
    {
    case InternalPacketAction_t::PACKET_ACTION_YN_REQUEST:
        {
            bool isResponse = p.ReadU8() == 1;
            auto charName = p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

            if(isResponse)
            {
                if(p.Left() != 2)
                {
                    return false;
                }

                uint16_t responseCode = p.ReadU16Little();
                if(clients.size() == 1)
                {
                    libcomp::Packet response;
                    response.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_INVITE);
                    response.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
                        charName, true);
                    response.WriteU16Little(responseCode);

                    clients.front()->SendPacket(response);
                }
            }
            else
            {
                if(p.Left() != 4)
                {
                    return false;
                }

                uint32_t partyID = p.ReadU32Little();
                if(clients.size() == 1)
                {
                    libcomp::Packet request;
                    request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_INVITED);
                    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
                        charName, true);
                    request.WriteU32Little(partyID);

                    clients.front()->SendPacket(request);
                }
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES:
        {
            auto charName = p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
            uint16_t responseCode = p.ReadU16Little();
            if(clients.size() == 1)
            {
                libcomp::Packet response;
                response.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_JOIN);
                response.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
                    charName, true);
                response.WriteU16Little(responseCode);

                clients.front()->SendPacket(response);
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_NO:
        {
            auto charName = p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
            if(clients.size() == 1)
            {
                libcomp::Packet response;
                response.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_CANCEL);
                response.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
                    charName, true);

                clients.front()->SendPacket(response);
            }
        }
        break;
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
                auto state = client->GetClientState();
                if(!state->GetPartyID())
                {
                    state->GetAccountLogin()->GetCharacterLogin()->SetPartyID(partyID);
                }

                for(auto info : members)
                {
                    QueuePartyMemberInfo(client, info);
                }
                client->FlushOutgoing();
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE:
        {
            bool isResponse = p.ReadU8() == 1;

            if(isResponse)
            {
                if(p.Left() != 2)
                {
                    return false;
                }

                uint16_t responseCode = p.ReadU16Little();
                if(clients.size() == 1)
                {
                    libcomp::Packet response;
                    response.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_LEAVE);
                    response.WriteU16Little(responseCode);

                    auto client = clients.front();

                    // Whether leaving was "successful" or not, the party ID should now be empty
                    client->GetClientState()->GetAccountLogin()->GetCharacterLogin()
                        ->SetPartyID(0);
                    client->SendPacket(response);
                }
            }
            else
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
    case InternalPacketAction_t::PACKET_ACTION_GROUP_DISBAND:
        {
            bool isResponse = p.ReadU8() == 1;

            if(isResponse)
            {
                if(p.Left() != 2)
                {
                    return false;
                }

                uint16_t responseCode = p.ReadU16Little();
                if(clients.size() == 1)
                {
                    libcomp::Packet response;
                    response.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_DISBAND);
                    response.WriteU16Little(responseCode);

                    auto client = clients.front();

                    // Whether leaving was "successful" or not, the party ID should now be empty
                    client->GetClientState()->GetAccountLogin()->GetCharacterLogin()
                        ->SetPartyID(0);
                    client->SendPacket(response);
                }
            }
            else
            {
                for(auto client : clients)
                {
                    client->GetClientState()->GetAccountLogin()->GetCharacterLogin()
                        ->SetPartyID(0);
                }

                libcomp::Packet request;
                request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_DISBANDED);

                ChannelClientConnection::BroadcastPacket(clients, request);
            }
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEADER_UPDATE:
        {
            bool isResponse = p.ReadU8() == 1;

            if(isResponse)
            {
                if(p.Left() != 2)
                {
                    return false;
                }

                uint16_t responseCode = p.ReadU16Little();
                if(clients.size() == 1)
                {
                    libcomp::Packet response;
                    response.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_LEADER_UPDATE);
                    response.WriteU16Little(responseCode);

                    clients.front()->SendPacket(response);
                }
            }
            else
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
    case InternalPacketAction_t::PACKET_ACTION_PARTY_DROP_RULE:
        {
            bool isResponse = p.ReadU8() == 1;

            if(isResponse)
            {
                if(p.Left() != 2)
                {
                    return false;
                }

                uint16_t responseCode = p.ReadU16Little();
                if(clients.size() == 1)
                {
                    libcomp::Packet response;
                    response.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_DROP_RULE);
                    response.WriteU16Little(responseCode);

                    clients.front()->SendPacket(response);
                }
            }
            else
            {
                if(p.Left() != 1)
                {
                    return false;
                }

                uint8_t rule = p.ReadU8();

                libcomp::Packet request;
                request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_DROP_RULE_SET);
                request.WriteU8(rule);

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
                partyState->GetAccountLogin()->GetCharacterLogin()->SetPartyID(0);
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
