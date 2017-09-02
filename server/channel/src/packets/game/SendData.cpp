/**
 * @file server/channel/src/packets/game/SendData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to get updated data from the server.
 *  This can be thought of as the sign that the client is ready to receive
 *  regular updates but also involves certain packets that should
 *  always send at this point.
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
#include <PacketCodes.h>

// object includes
#include <ServerZone.h>
#include <ChannelConfig.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

void SendClientReadyData(std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client)
{
    auto characterManager = server->GetCharacterManager();
    auto zoneManager = server->GetZoneManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    // Send world time
    {
        int8_t phase, hour, min;
        server->GetWorldClockTime(phase, hour, min);

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_WORLD_TIME);
        p.WriteS8(phase);
        p.WriteS8(hour);
        p.WriteS8(min);

        client->QueuePacket(p);
    }

    // Send sync time relative to the client
    {
        ServerTime currentServerTime = ChannelServer::GetServerTime();
        ClientTime currentClientTime = state->ToClientTime(currentServerTime);

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SYNC_TIME);
        p.WriteFloat(currentClientTime);

        client->QueuePacket(p);
    }
      
    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(server->GetConfig());
    libcomp::String systemMessage = conf->GetSystemMessage();

    if(!systemMessage.IsEmpty()) 
    {
        server->SendSystemMessage(client,systemMessage,0,false);
    }
    
    /// @todo: send "world bonus" [0x0405]
    /// @todo: send player skill updates (toggleable abilities for example) [0x03B8]

    // Set character icon
    characterManager->SetStatusIcon(client);

    // Send zone information
    {
        // Default to last logout information first
        uint32_t zoneID = character->GetLogoutZone();
        float xCoord = character->GetLogoutX();
        float yCoord = character->GetLogoutY();
        float rotation = character->GetLogoutRotation();

        // Default to homepoint second
        if(zoneID == 0)
        {
            zoneID = character->GetHomepointZone();
            xCoord = character->GetHomepointX();
            yCoord = character->GetHomepointY();
            rotation = 0;
        }

        // Make sure the player can start in the zone
        if(zoneID != 0)
        {
            auto serverDataManager = server->GetServerDataManager();
            auto zoneData = serverDataManager->GetZoneData(zoneID);
            auto zoneLobbyData = zoneData && !zoneData->GetGlobal()
                ? serverDataManager->GetZoneData(zoneData->GetGroupID())
                : nullptr;
            if(zoneLobbyData)
            {
                zoneID = zoneLobbyData->GetID();
            }
        }

        // If all else fails start in the default zone
        if(zoneID == 0)
        {
            /// @todo: make this configurable?
            zoneID = 0x00004E85;
            xCoord = 0;
            yCoord = 0;
            rotation = 0;
        }

        if(!zoneManager->EnterZone(client, zoneID, xCoord, yCoord, rotation))
        {
            LOG_ERROR(libcomp::String("Failed to add client to zone"
                " %1. Closing the connection.\n").Arg(zoneID));
            client->Close();
            return;
        }
    }

    // Send active partner demon ID
    if(!character->GetActiveDemon().IsNull())
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_SUMMONED);
        p.WriteS64Little(state->GetObjectID(character->GetActiveDemon().GetUUID()));

        client->QueuePacket(p);
    }

    /// @todo: send skill cost rate [0x019E]
    /// @todo: send "connected commu SV" [0x015A]

    client->FlushOutgoing();
}

bool Parsers::SendData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)p;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    server->QueueWork(SendClientReadyData, server, client);

    return true;
}
