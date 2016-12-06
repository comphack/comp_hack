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
#include "Decrypt.h"
#include "Log.h"
#include "Packet.h"
#include "ReadOnlyPacket.h"
#include "TcpConnection.h"

// lobby Includes
#include "ManagerPacket.h"
#include "LobbyServer.h"

using namespace lobby;

bool Parsers::WorldList::Parse(ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    libcomp::Packet reply;
    reply.WriteU16Little(0x000C);

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());

    auto worlds = server->GetWorlds();

    // World count.
    reply.WriteU8(worlds.size());

    // Add each world to the list.
    for(auto world : worlds)
    {
        // ID for this world.
        reply.WriteU8(0);

        // Name of the world.
        reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
            world->GetName(), true);

        // Number of channels on this world.
        reply.WriteU8(1);

        // Add each channel for this world.
        {
            // Name of the channel. This used to be displayed in the channel
            // list that was hidden from the user.
            reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                "Channel 1", true);

            // Ping time??? Again, something that used to be in the list.
            reply.WriteU16Little(1);

            // 0 - Visible | 2 - Hidden (or PvP)
            // Pointless without the list.
            reply.WriteU8(0);
        }
    }

    connection->SendPacket(reply);

    return true;
}
