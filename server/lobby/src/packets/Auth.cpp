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

using namespace lobby;

bool Parsers::Auth::Parse(ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 303 || p.PeekU16Little() != 301)
    {
        return false;
    }

    // Authentication token (session ID) provided by the web server.
    libcomp::String sid = p.ReadString16Little(
        libcomp::Convert::ENCODING_UTF8).ToLower();

    LOG_DEBUG(libcomp::String("SID: %1\n").Arg(sid));

    libcomp::Packet reply;
    reply.WriteU16Little(0x0006);

    // Status code (see the Login handler for a list).
    reply.WriteS32Little(0);

    libcomp::String sid2 = libcomp::Decrypt::GenerateRandom(300).ToLower();

    LOG_DEBUG(libcomp::String("SID2: %1\n").Arg(sid2));

    // Write a new session ID to be used when the client switches channels.
    reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8, sid2, true);

    connection->SendPacket(reply);

    return true;
}
