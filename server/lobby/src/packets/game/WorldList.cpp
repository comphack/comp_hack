/**
 * @file server/lobby/src/packets/WorldList.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to return the lobby's world list.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// lobby Includes
#include "LobbyServer.h"

using namespace lobby;

bool Parsers::WorldList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(LobbyToClientPacketCode_t::PACKET_WORLD_LIST);

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());

    auto worlds = server->GetWorlds();
    worlds.remove_if([](const std::shared_ptr<lobby::World>& world)
        {
            return world->GetRegisteredWorld()->GetStatus()
                == objects::RegisteredWorld::Status_t::INACTIVE;
        });

    // World count.
    reply.WriteU8((uint8_t)worlds.size());

    // Add each world to the list.
    for(auto world : worlds)
    {
        auto worldServer = world->GetRegisteredWorld();

        // ID for this world.
        reply.WriteU8(worldServer->GetID());

        // Name of the world.
        reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
            worldServer->GetName(), true);

        auto channels = world->GetChannels();

        // Number of channels on this world.
        reply.WriteU8((uint8_t)channels.size());

        // Add each channel for this world.
        for(auto channel : channels)
        {
            // Name of the channel. This used to be displayed in the channel
            // list that was hidden from the user.
            reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                channel->GetName(), true);

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
