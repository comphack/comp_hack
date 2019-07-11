/**
 * @file server/world/src/packets/SetChannelInfo.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle detailing the world for the lobby.
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
#include <InternalConnection.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// world Includes
#include "WorldServer.h"

using namespace world;

bool Parsers::SetChannelInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto conn = std::dynamic_pointer_cast<libcomp::InternalConnection>(
        connection);
    if(nullptr == conn)
    {
        return false;
    }

    if(p.Size() == 0)
    {
        LOG_DEBUG("Channel Server connection sent an empty response."
            "  The connection will be closed.\n");
        connection->Close();
        return false;
    }

    uint8_t channelID = p.ReadU8();

    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager
        ->GetServer());
    if(server->GetChannelConnectionByID((int8_t)channelID))
    {
        LOG_DEBUG("The ID of the channel requesting a connection does not"
            " match an available channel ID.\n");
        connection->Close();
        return true;
    }

    auto worldDB = server->GetWorldDatabase();

    auto svr = objects::RegisteredChannel::LoadRegisteredChannelByID(worldDB,
        channelID);

    connection->SetName(libcomp::String("%1:%2:%3").Arg(
        connection->GetName()).Arg(svr->GetID()).Arg(svr->GetName()));

    LOG_DEBUG(libcomp::String("Updating Channel Server: (%1) %2\n")
        .Arg(svr->GetID())
        .Arg(svr->GetName()));

    // If the channel has already set the IP, it should be the externally
    // facing IP so we'll leave it alone
    if(svr->GetIP().IsEmpty())
    {
        svr->SetIP(connection->GetRemoteAddress());
        if(!svr->Update(worldDB))
        {
            LOG_DEBUG("Channel Server could not be updated with its address.\n");
            return false;
        }
    }

    server->RegisterChannel(svr, conn);

    //Forward the information to the lobby and other channels
    std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
    connections.push_back(server->GetLobbyConnection());

    for(auto kv : server->GetChannels())
    {
        if(kv.first != connection)
        {
            connections.push_back(kv.first);
        }
    }

    libcomp::Packet packet;
    packet.WritePacketCode(
        InternalPacketCode_t::PACKET_SET_CHANNEL_INFO);
    packet.WriteU8(to_underlying(
        InternalPacketAction_t::PACKET_ACTION_UPDATE));
    packet.WriteU8(channelID);

    libcomp::TcpConnection::BroadcastPacket(connections, packet);

    return true;
}
