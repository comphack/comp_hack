/**
 * @file server/channel/src/packets/game/SendData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update data on the server.
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

// channel Includes
#include "ChannelServer.h"

using namespace channel;

void SendZoneChange(std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client)
{
    auto zoneManager = server->GetZoneManager();
    auto cState = client->GetClientState()->GetCharacterState();
    float xCoord = cState->GetOriginX();
    float yCoord = cState->GetOriginY();
    float rotation = cState->GetOriginRotation();

    /// @todo: replace with last zone information
    uint32_t zoneID = 0x00004E85;
    if(!zoneManager->EnterZone(client, zoneID, xCoord, yCoord, rotation))
    {
        LOG_ERROR(libcomp::String("Failed to add client to zone"
            " %1. Closing the connection.\n").Arg(zoneID));
        client->Close();
        return;
    }
}

bool Parsers::SendData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)p;

    /// @todo: A bunch of stuff

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CONFIRMATION);
    reply.WriteU32Little(0x00010000); // 1.0.0

    connection->SendPacket(reply);

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    server->QueueWork(SendZoneChange, server, client);

    return true;
}
