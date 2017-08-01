/**
 * @file server/channel/src/packets/internal/Relay.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Response or request packet from the world with information
 *  from the world or another channel.
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

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::Relay::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    if(p.Size() < 5)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto connectionManager = server->GetManagerConnection();

    int32_t sourceCID = p.ReadS32Little();
    PacketRelayMode_t mode = (PacketRelayMode_t)p.ReadU8();
    if(mode == PacketRelayMode_t::RELAY_FAILURE)
    {
        // A failure is being returned, handle it if needed
        auto sourceClient = connectionManager->GetEntityClient(sourceCID, true);
        if(!sourceClient)
        {
            // Stop here, do not send a failure about a failure back
            LOG_ERROR("Packet relay failed and could not be reported"
                " back to the source.\n");
            return true;
        }

        auto state = sourceClient->GetClientState();

        // Retrieve list of character names for world visible failures
        uint16_t nameCount = p.ReadU16Little();

        std::list<libcomp::String> characterNames;
        for(uint16_t i = 0; i < nameCount; i++)
        {
            characterNames.push_back(p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true));
        }

        uint16_t packetCode = p.ReadU16Little();
        switch((ChannelToClientPacketCode_t)packetCode)
        {
        case ChannelToClientPacketCode_t::PACKET_CHAT:
            if(p.Left() > 2)
            {
                ChatType_t type = (ChatType_t)p.ReadU16Little();

                // Only tell has anything the source needs to be told
                if(type == ChatType_t::CHAT_TELL)
                {
                    // Ignore the names retrieved in case the character
                    // name was not even a real player
                    libcomp::String targetName = p.ReadString16Little(
                        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

                    // Tell failures are parsed client side as an empty tell
                    // message from the requested target
                    libcomp::Packet reply;
                    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHAT);
                    reply.WriteU16Little((uint16_t)ChatType_t::CHAT_TELL);
                    reply.WriteString16Little(state->GetClientStringEncoding(),
                        targetName, true);
                    reply.WriteString16Little(state->GetClientStringEncoding(),
                        "", true);
                    sourceClient->SendPacket(reply);
                }
            }
            break;
        default:
            // No handling needed
            break;
        }

        return true;
    }
    else if(mode != PacketRelayMode_t::RELAY_CIDS)
    {
        LOG_ERROR(libcomp::String("Invalid relay mode received from"
            " the world: %1\n").Arg((uint8_t)mode));
        return false;
    }

    uint16_t cidCount = p.ReadU16Little();
    if(p.Left() < (uint32_t)(cidCount * 4))
    {
        return false;
    }

    std::list<int32_t> worldCIDs;
    for(uint16_t i = 0; i < cidCount; i++)
    {
        worldCIDs.push_back(p.ReadS32Little());
    }

    // The rest is the packet itself
    if(p.Left() < 2)
    {
        return false;
    }

    auto packetData = p.ReadArray(p.Left());

    std::list<int32_t> failedCIDs;
    for(int32_t worldCID : worldCIDs)
    {
        auto targetClient = connectionManager->GetEntityClient(worldCID, true);
        if(targetClient)
        {
            libcomp::Packet relay;
            relay.WriteArray(packetData);

            targetClient->SendPacket(relay);
        }
        else
        {
            failedCIDs.push_back(worldCID);
        }
    }

    if(failedCIDs.size() > 0)
    {
        // Build a response message containing the world CIDs that are not
        // here and send it back to the world
        libcomp::Packet failure;
        failure.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
        failure.WriteS32Little(sourceCID);
        failure.WriteU8((uint8_t)PacketRelayMode_t::RELAY_FAILURE);
        failure.WriteU16Little((uint16_t)failedCIDs.size());
        for(auto worldCID : failedCIDs)
        {
            failure.WriteS32Little(worldCID);
        }

        failure.WriteArray(packetData);

        connectionManager->GetWorldConnection()->SendPacket(failure);
    }

    return true;
}
