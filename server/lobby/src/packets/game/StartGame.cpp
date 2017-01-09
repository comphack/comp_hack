/**
 * @file server/lobby/src/packets/Login.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to handle lobby packets.
 *
 * This file is part of the Lobby Server (lobby).
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
#include <Decrypt.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>

// lobby Includes
#include "LobbyClientConnection.h"
#include "LobbyServer.h"
#include "ManagerPacket.h"

using namespace lobby;

bool Parsers::StartGame::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 2)
    {
        return false;
    }

    uint8_t cid = p.ReadU8();
    int8_t worldID = p.ReadS8();

    /// @todo: get this ID from the world
    auto channelID = 0;

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto world = server->GetWorldByID((uint8_t)worldID);
    auto channel = world->GetChannelByID((uint8_t)channelID);

    LOG_DEBUG(libcomp::String("Login character with ID %1 into world %2, channel %3\n"
        ).Arg(cid).Arg(worldID).Arg(channelID));

    libcomp::Packet reply;
    reply.WritePacketCode(LobbyClientPacketCode_t::PACKET_START_GAME_RESPONSE);

    // Some session key.
    reply.WriteU32Little(0);

    // Server address.
    reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
        libcomp::String("%1:%2").Arg(channel->GetIP()).Arg(channel->GetPort()), true);

    // Character ID.
    reply.WriteU8(cid);

    connection->SendPacket(reply);

    return true;
}
