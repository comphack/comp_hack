/**
 * @file server/lobby/src/packets/WorldDescription.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Response packet from the world describing base information.
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
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>
#include <WorldDescription.h>

// lobby Includes
#include "ManagerPacket.h"
#include "LobbyServer.h"

using namespace lobby;

bool Parsers::SetWorldDescription::Parse(ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    objects::WorldDescription obj;

    if(!obj.LoadPacket(p))
    {
        return false;
    }

    auto iConnection = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);

    if(nullptr == iConnection)
    {
        return false;
    }

    LOG_DEBUG(libcomp::String("Updating World Server description: (%1) %2\n").Arg(obj.GetID()).Arg(obj.GetName()));

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());

    auto world = server->GetWorldByConnection(iConnection);
    world->SetWorldDescription(obj);

    return true;
}
