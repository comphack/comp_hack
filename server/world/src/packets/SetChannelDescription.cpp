/**
 * @file server/world/src/packets/SetChannelDescription.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle describing the world for the lobby.
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

bool Parsers::SetChannelDescription::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    objects::ChannelDescription obj;

    if(!obj.LoadPacket(p))
    {
        return false;
    }

    auto conn = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);

    if(nullptr == conn)
    {
        return false;
    }

    LOG_DEBUG(libcomp::String("Updating Channel Server description: (%1) %2\n").Arg(obj.GetID()).Arg(obj.GetName()));

    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());

    server->SetChannelDescription(obj, conn);

    //Forward the information to the lobby
    auto lobbyConnection = server->GetLobbyConnection();

    libcomp::Packet packet;
    packet.WriteU16Little(PACKET_SET_CHANNEL_DESCRIPTION);
    packet.WriteU8(PACKET_ACTION_UPDATE);
    obj.SavePacket(packet);
    lobbyConnection->SendPacket(packet);

    return true;
}
