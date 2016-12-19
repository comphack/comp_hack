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
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <PacketLogin.h>
#include <PacketResponseCode.h>

using namespace lobby;

bool Parsers::Login::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    objects::PacketLogin obj;

    if(!obj.LoadPacket(p))
    {
        return false;
    }

    // Read the client version.
    uint32_t clientVer = obj.GetClientVersion();
    uint32_t major = clientVer / 1000;
    uint32_t minor = clientVer % 1000;

    LOG_DEBUG(libcomp::String("Username: %1\n").Arg(obj.GetUsername()));
    LOG_DEBUG(libcomp::String("Client Version: %1.%2\n").Arg(major).Arg(minor));

    objects::PacketResponseCode reply;
    reply.SetCommandCode(ClientPacketCode_t::PACKET_LOGIN_RESPONSE);

    /*
     *  0   No error
     * -1   System error
     * -2   Protocol error
     * -3   Parameter error
     * -4   Unsupported feature
     * -5   Incorrect username or password
     * -6   Account still logged in
     * -7   Insufficient cash shop points
     * -8   Server currently down
     * -9   Not authorized to perform action
     * -10  Do not have character creation ticket
     * -11  No empty character slots
     * -12  You have already done that
     * -13  Server is currently full
     * -14  Feature can't be used yet
     * -15  You have too many characters
     * -16  Can't use that character name
     * -17  Server crowded (with popup)
     * -18  Wrong client version (and any gap)
     * -26  Currently can't use this account
     * -28  To log in you must re-cert (pops up login window)
     * -101 Account locked by antitheft function
     * -102 Account locked by antitheft function
     * -103 Connection has timed out
     */

    // Password salt (or error code).
    reply.SetResponseCode(0x3F5E2FB5);

    return connection->SendObject(reply);
}
