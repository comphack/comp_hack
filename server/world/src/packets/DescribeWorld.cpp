/**
 * @file server/world/src/packets/Login.cpp
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
#include "Decrypt.h"
#include "Log.h"
#include "Packet.h"
#include "ReadOnlyPacket.h"
#include "TcpConnection.h"

// world Includes
#include "ManagerPacket.h"
#include "WorldServer.h"

using namespace world;

bool Parsers::DescribeWorld::Parse(ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());

    libcomp::Packet reply;

    reply.WriteU16Little(0x1001);

    //todo: replace with objgen out stream/vector
    libcomp::String name = server->GetName();
    reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8, name, true);

    connection->SendPacket(reply);

    return true;
}
