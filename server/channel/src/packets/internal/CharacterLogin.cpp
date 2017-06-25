/**
 * @file server/channel/src/packets/internal/CharacterLogin.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Parser to handle receiving character login information from
 *  the world to the channel.
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
#include <FriendSettings.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::CharacterLogin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    if(p.Size() < 5)
    {
        LOG_ERROR("Invalid response received for CharacterLogin.\n");
        return false;
    }

    uint8_t updateFlags = p.ReadU8();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    // Pull all the logins
    auto worldDB = server->GetWorldDatabase();
    auto login = std::make_shared<objects::CharacterLogin>();
    if(!login->LoadPacket(p, false))
    {
        LOG_ERROR("Invalid character info received for CharacterLogin.\n");
        return false;
    }

    auto member = std::make_shared<objects::PartyCharacter>();
    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO &&
        !member->LoadPacket(p, true))
    {
        LOG_ERROR("Invalid party member character received for CharacterLogin.\n");
        return false;
    }
    
    auto partyDemon = std::make_shared<objects::PartyMember>();
    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO &&
        !partyDemon->LoadPacket(p, true))
    {
        LOG_ERROR("Invalid party member demon received for CharacterLogin.\n");
        return false;
    }

    // Affected CIDs are appended to the end
    uint16_t cidCount = p.ReadU16Little();

    if(p.Left() < (uint32_t)(cidCount * 4))
    {
        LOG_ERROR("Invalid CID count received for CharacterLogin.\n");
        return false;
    }

    std::list<int32_t> cids;
    for(uint16_t i = 0; i < cidCount; i++)
    {
        cids.push_back(p.ReadS32Little());
    }

    std::unordered_map<int32_t, std::shared_ptr<ChannelClientConnection>> connections;
    for(int32_t cid : cids)
    {
        auto state = ClientState::GetEntityClientState(cid, true);
        auto cState = state != nullptr ? state->GetCharacterState() : nullptr;
        auto client = cState->GetEntity() != nullptr ?
            server->GetManagerConnection()->GetClientConnection(
                cState->GetEntity()->GetAccount()->GetUsername()) : nullptr;
        if(client)
        {
            connections[cid] = client;
        }
    }

    if(connections.size() == 0)
    {
        // Character(s) are not here anymore, exit now
        return true;
    }

    // Update friend information
    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_FLAGS)
    {
        auto fSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, login->GetCharacter());
        if(!fSettings)
        {
            LOG_ERROR(libcomp::String("Character friend settings failed to load: %1\n")
                .Arg(login->GetCharacter().GetUUID().ToString()));
            return true;
        }

        auto friends = fSettings->GetFriends();
        std::list<std::shared_ptr<libcomp::TcpConnection>> friendConnections;
        for(auto cPair : connections)
        {
            auto uuid = cPair.second->GetClientState()->GetCharacterState()
                ->GetEntity()->GetUUID();
            for(auto f : friends)
            {
                if(f.GetUUID() == uuid)
                {
                    friendConnections.push_back(cPair.second);
                    break;
                }
            }
        }

        libcomp::Packet packet;
        packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FRIEND_DATA);
        packet.WriteS32Little(login->GetWorldCID());
        packet.WriteU8(updateFlags);

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_STATUS)
        {
            packet.WriteS8((int8_t)login->GetStatus());
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE)
        {
            packet.WriteS32Little((int32_t)login->GetZoneID());
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_CHANNEL)
        {
            packet.WriteS8(login->GetChannelID());
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_MESSAGE)
        {
            packet.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                fSettings->GetFriendMessage(), true);
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_UNKNOWN)
        {
            packet.WriteU32Little((uint32_t)login->GetWorldCID());
            packet.WriteS8(0);   // Unknown
        }

        libcomp::TcpConnection::BroadcastPacket(friendConnections, packet);
    }
    
    // Update local party information
    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_FLAGS
        && login->GetPartyID())
    {
        // Pull the local state if it still exists
        uint32_t zoneID = login->GetZoneID();
        auto state = ClientState::GetEntityClientState(login->GetWorldCID(), true);
        int32_t localEntityID = state ? state->GetCharacterState()->GetEntityID() : -1;
        int32_t localDemonEntityID = state ? state->GetDemonState()->GetEntityID() : -1;

        std::list<std::shared_ptr<libcomp::TcpConnection>> partyConnections;
        std::list<std::shared_ptr<libcomp::TcpConnection>> sameZoneConnections;
        std::list<std::shared_ptr<libcomp::TcpConnection>> differentZoneConnections;
        for(auto cPair : connections)
        {
            auto otherState = cPair.second->GetClientState();
            auto otherLogin = otherState->GetAccountLogin()->GetCharacterLogin();
            if(otherState->GetPartyID() == login->GetPartyID())
            {
                partyConnections.push_back(cPair.second);
                if(otherLogin->GetZoneID() == zoneID &&
                    otherLogin->GetChannelID() == login->GetChannelID())
                {
                    sameZoneConnections.push_back(cPair.second);
                }
                else
                {
                    differentZoneConnections.push_back(cPair.second);
                }
            }
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE)
        {
            // Map connections by local entity visibility
            std::unordered_map<bool, std::list<std::shared_ptr<libcomp::TcpConnection>>*> cMap;
            cMap[true] = &sameZoneConnections;
            cMap[false] = &differentZoneConnections;
            for(auto pair : cMap)
            {
                if(pair.second->size() == 0) continue;

                libcomp::Packet packet;
                packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_ZONE);
                packet.WriteS32Little(pair.first ? localEntityID : -1);
                packet.WriteS32Little((int32_t)zoneID);
                packet.WriteS32Little(login->GetWorldCID());

                libcomp::TcpConnection::BroadcastPacket(*pair.second, packet);
            }

            if(differentZoneConnections.size() > 0)
            {
                libcomp::Packet packet;
                packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_PARTNER);
                packet.WriteS32Little(-1);
                packet.WriteS32Little(-1);
                packet.WriteU32Little(partyDemon->GetDemonType());
                packet.WriteU16Little(0);
                packet.WriteU16Little(0);
                packet.WriteS32Little(login->GetWorldCID());

                libcomp::TcpConnection::BroadcastPacket(differentZoneConnections, packet);
            }
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO &&
            sameZoneConnections.size() > 0)
        {
            libcomp::Packet packet;
            packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_UPDATE);
            packet.WriteS32Little(localEntityID);
            packet.WriteU8(member->GetLevel());
            packet.WriteU16Little(member->GetHP());
            packet.WriteU16Little(member->GetMaxHP());
            packet.WriteU16Little(member->GetMP());
            packet.WriteU16Little(member->GetMaxMP());

            int8_t unknownCount = 0;
            packet.WriteS8(unknownCount);
            for(int8_t i = 0; i < unknownCount; i++)
            {
                packet.WriteS32Little(0);    // Unknown
            }

            packet.WriteS32Little(login->GetWorldCID());
            packet.WriteS8(0);  // Unknown

            libcomp::TcpConnection::BroadcastPacket(sameZoneConnections, packet);
        }
        
        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO &&
            sameZoneConnections.size() > 0)
        {
            libcomp::Packet packet;
            packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_PARTNER);
            packet.WriteS32Little(localEntityID);
            packet.WriteS32Little(localDemonEntityID);
            packet.WriteU32Little(partyDemon->GetDemonType());
            packet.WriteU16Little(partyDemon->GetHP());
            packet.WriteU16Little(partyDemon->GetMaxHP());
            packet.WriteS32Little(login->GetWorldCID());

            libcomp::TcpConnection::BroadcastPacket(sameZoneConnections, packet);
        }
        
        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_ICON)
        {
            libcomp::Packet packet;
            packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_ICON);
            packet.WriteS32Little(localEntityID);
            packet.WriteU8(0);
            packet.WriteU8(0);
            packet.WriteU8(0);
            packet.WriteS8(0);

            libcomp::TcpConnection::BroadcastPacket(partyConnections, packet);
        }
    }

    return true;
}
