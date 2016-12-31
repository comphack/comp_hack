/**
 * @file server/lobby/src/packets/SetChannelInfo.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Parser to handle detailing a channel for the lobby.
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
#include <ChannelDescription.h>
#include <Decrypt.h>
#include <InternalConnection.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// lobby Includes
#include "LobbyServer.h"

using namespace lobby;

bool Parsers::SetChannelInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() == 0)
    {
        return false;
    }

    auto action = static_cast<InternalPacketAction_t>(p.ReadU8());

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());

    auto desc = std::shared_ptr<objects::ChannelDescription>(new objects::ChannelDescription);

    if(!desc->LoadPacket(p))
    {
        return false;
    }

    auto conn = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);

    auto world = server->GetWorldByConnection(conn);

    if(InternalPacketAction_t::PACKET_ACTION_REMOVE == action)
    {
        world->RemoveChannelDescriptionByID(desc->GetID());
    }
    else
    {
        LOG_DEBUG(libcomp::String("Updating Channel Server description: (%1) %2\n")
            .Arg(desc->GetID()).Arg(desc->GetName()));
        world->SetChannelDescription(desc);
    }

    return true;
}
