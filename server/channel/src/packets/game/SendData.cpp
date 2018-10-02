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
#include <PacketCodes.h>
#include <ServerConstants.h>
#include <ServerDataManager.h>

// object includes
#include <Account.h>
#include <ServerZone.h>
#include <ChannelConfig.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ZoneManager.h"

using namespace channel;

void SendClientReadyData(std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client)
{
    auto characterManager = server->GetCharacterManager();
    auto serverDataManager = server->GetServerDataManager();
    auto zoneManager = server->GetZoneManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    // Send world time
    {
        auto clock = server->GetWorldClockTime();

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_WORLD_TIME);
        p.WriteS8(clock.MoonPhase);
        p.WriteS8(clock.Hour);
        p.WriteS8(clock.Min);

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

    // Send any server system messages
    libcomp::String systemMessage = conf->GetSystemMessage();
    if(!systemMessage.IsEmpty()) 
    {
        server->SendSystemMessage(client, systemMessage,
            (int8_t)conf->GetSystemMessageColor(), false);
    }

    auto worldSharedConfig = conf->GetWorldSharedConfig();
    libcomp::String compShopMessage = worldSharedConfig->GetCOMPShopMessage();
    if(!compShopMessage.IsEmpty())
    {
        server->SendSystemMessage(client, compShopMessage, 4, false);
    }

    // Send client recognized world bonuses
    {
        /// @todo: identify more of these
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_WORLD_BONUS);
        p.WriteS32Little(1);

        p.WriteS32Little(2);    // Type
        p.WriteFloat(worldSharedConfig->GetDeathPenaltyDisabled() ? 0.f : 1.f);

        client->QueuePacket(p);
    }

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

        // Make sure the player can start in the zone
        if(zoneID != 0)
        {
            auto zoneData = serverDataManager->GetZoneData(zoneID, 0);
            if(!zoneData)
            {
                // Can't discern any information about the logout zone
                zoneID = 0;
            }
            else if(!zoneData->GetGlobal() || zoneData->GetRestricted())
            {
                // Determine which public zone to go to instead, defaulting
                // to the lobby matching the group ID
                uint32_t publicID = zoneData->GetGroupID();
                if(!publicID && character->GetPreviousZone())
                {
                    // If there is no group for the zone, return to
                    // the previous public zone
                    publicID = character->GetPreviousZone();
                }

                auto publicData = serverDataManager->GetZoneData(publicID, 0);
                if(publicData && publicData->GetGlobal())
                {
                    zoneID = publicData->GetID();
                    xCoord = publicData->GetStartingX();
                    yCoord = publicData->GetStartingY();
                    rotation = publicData->GetStartingRotation();
                }
                else
                {
                    // Correct it further down
                    zoneID = 0;
                }
            }
        }

        // Default to homepoint second
        if(zoneID == 0)
        {
            zoneID = character->GetHomepointZone();

            auto zoneData = server->GetServerDataManager()->GetZoneData(zoneID, 0);
            if(zoneData)
            {
                zoneManager->GetSpotPosition(zoneData->GetDynamicMapID(),
                    character->GetHomepointSpotID(), xCoord, yCoord, rotation);
            }
        }

        // If all else fails start in the default zone
        if(zoneID == 0)
        {
            auto zoneData = serverDataManager->GetZoneData(
                SVR_CONST.ZONE_DEFAULT, 0);
            if(zoneData)
            {
                zoneID = zoneData->GetID();
                xCoord = zoneData->GetStartingX();
                yCoord = zoneData->GetStartingY();
                rotation = zoneData->GetStartingRotation();
            }
        }

        if(!zoneManager->EnterZone(client, zoneID, 0, xCoord, yCoord, rotation))
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

    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_COMM_SERVER_STATE);
        p.WriteS32Little(1);    // Connected

        client->QueuePacket(p);
    }

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
