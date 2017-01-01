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
#include <ChannelDescription.h>
#include <Decrypt.h>
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
    auto desc = std::shared_ptr<objects::ChannelDescription>(new objects::ChannelDescription);

    if(!desc->LoadPacket(p))
    {
        return false;
    }

    auto conn = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);

    if(nullptr == conn)
    {
        return false;
    }

    LOG_DEBUG(libcomp::String("Updating Channel Server description: (%1) %2\n").Arg(desc->GetID())
        .Arg(desc->GetName()));

    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());

    server->SetChannelDescription(desc, conn);

    //Forward the information to the lobby
    auto lobbyConnection = server->GetLobbyConnection();

    libcomp::Packet packet;
    packet.WritePacketCode(
        InternalPacketCode_t::PACKET_SET_CHANNEL_INFO);
    packet.WriteU8(to_underlying(
        InternalPacketAction_t::PACKET_ACTION_UPDATE));
    desc->SavePacket(packet);
    lobbyConnection->SendPacket(packet);

    return true;
}
