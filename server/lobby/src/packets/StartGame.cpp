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
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

using namespace lobby;

bool Parsers::StartGame::Parse(ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 2)
    {
        return false;
    }

    uint8_t cid = p.ReadU8();
    int8_t channel = p.ReadS8();

    LOG_DEBUG(libcomp::String("Login character with ID %1 into world %2\n"
        ).Arg(cid).Arg(channel));

    libcomp::Packet reply;
    reply.WriteU16Little(0x0008);

    // Some session key.
    reply.WriteU32Little(0);

    // Server address.
    reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
        "192.168.0.72:14666", true);

    // Character ID.
    reply.WriteU8(cid);

    connection->SendPacket(reply);

    return true;
}
