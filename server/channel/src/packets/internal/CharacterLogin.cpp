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
#include <Clan.h>
#include <ClanMember.h>
#include <FriendSettings.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"

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

    bool connectionsFound = false;
    auto clients = server->GetManagerConnection()
        ->GatherWorldTargetClients(p, connectionsFound);
    if(!connectionsFound)
    {
        LOG_ERROR("Connections not found for CharacterLogin.\n");
        return false;
    }

    if(clients.size() == 0)
    {
        // Character(s) are not here anymore, exit now
        return true;
    }

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
        std::list<std::shared_ptr<ChannelClientConnection>> friendConnections;
        for(auto client : clients)
        {
            auto uuid = client->GetClientState()->GetCharacterState()
                ->GetEntity()->GetUUID();
            for(auto f : friends)
            {
                if(f.GetUUID() == uuid)
                {
                    friendConnections.push_back(client);
                    break;
                }
            }
        }

        libcomp::Packet packet;
        packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FRIEND_DATA);
        packet.WriteS32Little(login->GetWorldCID());
        packet.WriteU8(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_FLAGS);

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

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_MESSAGE)
        {
            packet.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                fSettings->GetFriendMessage(), true);
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_UNKNOWN)
        {
            packet.WriteU32Little((uint32_t)login->GetWorldCID());
            packet.WriteS8(0);   // Unknown
        }

        ChannelClientConnection::BroadcastPacket(friendConnections, packet);
    }
    
    // Update clan information
    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_BASIC)
    {
        int32_t clanID = p.ReadS32Little();

        // Load the character if they are not local
        auto character = login->GetCharacter().Get(worldDB);

        std::list<std::shared_ptr<ChannelClientConnection>> clanConnections;
        for(auto client : clients)
        {
            auto clientChar = client->GetClientState()->GetCharacterState()
                ->GetEntity();
            if(clientChar->GetClan().GetUUID() == character->GetClan().GetUUID())
            {
                clanConnections.push_back(client);
            }
        }

        libcomp::Packet packet;
        packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_DATA);
        packet.WriteS32Little(clanID);
        packet.WriteS32Little(login->GetWorldCID());
        packet.WriteS8(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_BASIC);

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

        ChannelClientConnection::BroadcastPacket(clanConnections, packet);
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

        std::shared_ptr<ChannelClientConnection> selfConnection;
        std::list<std::shared_ptr<ChannelClientConnection>> partyConnections;
        std::list<std::shared_ptr<ChannelClientConnection>> sameZoneConnections;
        std::list<std::shared_ptr<ChannelClientConnection>> differentZoneConnections;
        for(auto client : clients)
        {
            auto otherState = client->GetClientState();
            auto otherLogin = otherState->GetAccountLogin()->GetCharacterLogin();
            if(state == otherState)
            {
                selfConnection = client;
            }
            else if(otherState->GetPartyID() == login->GetPartyID())
            {
                partyConnections.push_back(client);
                if(otherLogin->GetZoneID() == zoneID &&
                    otherLogin->GetChannelID() == login->GetChannelID())
                {
                    sameZoneConnections.push_back(client);
                }
                else
                {
                    differentZoneConnections.push_back(client);
                }
            }
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE)
        {
            // Map connections by local entity visibility
            std::unordered_map<bool, std::list<std::shared_ptr<ChannelClientConnection>>> cMap;
            cMap[true] = sameZoneConnections;
            cMap[false] = differentZoneConnections;

            // The zone needs to be relayed back to the player (for some reason)
            if(selfConnection)
            {
                cMap[true].push_back(selfConnection);
            }

            for(auto pair : cMap)
            {
                if(pair.second.size() == 0) continue;

                libcomp::Packet packet;
                packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_ZONE);
                packet.WriteS32Little(pair.first ? localEntityID : -1);
                packet.WriteS32Little((int32_t)zoneID);
                packet.WriteS32Little(login->GetWorldCID());

                ChannelClientConnection::BroadcastPacket(pair.second, packet);
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

                ChannelClientConnection::BroadcastPacket(differentZoneConnections, packet);
            }
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO &&
            sameZoneConnections.size() > 0)
        {
            libcomp::Packet packet;
            packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_UPDATE);
            packet.WriteS32Little(localEntityID);
            packet.WriteU8(member->GetLevel());
            packet.WriteU16Little((uint16_t)member->GetHP());
            packet.WriteU16Little((uint16_t)member->GetMaxHP());
            packet.WriteU16Little((uint16_t)member->GetMP());
            packet.WriteU16Little((uint16_t)member->GetMaxMP());

            int8_t unknownCount = 0;
            packet.WriteS8(unknownCount);
            for(int8_t i = 0; i < unknownCount; i++)
            {
                packet.WriteS32Little(0);    // Unknown
            }

            packet.WriteS32Little(login->GetWorldCID());
            packet.WriteS8(0);  // Unknown

            ChannelClientConnection::BroadcastPacket(sameZoneConnections, packet);
        }
        
        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO &&
            sameZoneConnections.size() > 0)
        {
            libcomp::Packet packet;
            packet.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_MEMBER_PARTNER);
            packet.WriteS32Little(localEntityID);
            packet.WriteS32Little(localDemonEntityID);
            packet.WriteU32Little(partyDemon->GetDemonType());
            packet.WriteU16Little((uint16_t)partyDemon->GetHP());
            packet.WriteU16Little((uint16_t)partyDemon->GetMaxHP());
            packet.WriteS32Little(login->GetWorldCID());

            ChannelClientConnection::BroadcastPacket(sameZoneConnections, packet);
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

            ChannelClientConnection::BroadcastPacket(partyConnections, packet);
        }
    }

    return true;
}
