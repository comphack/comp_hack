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

bool Parsers::CreateCharacter::Parse(ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() < 45)
    {
        return false;
    }

    uint8_t world = p.ReadU8();

    LOG_DEBUG(libcomp::String("World: %1\n").Arg(world));

    if(p.Size() != (uint32_t)(p.PeekU16Little() + 44))
    {
        return false;
    }

    libcomp::String name = p.ReadString16Little(
        libcomp::Convert::ENCODING_CP932);

    LOG_DEBUG(libcomp::String("Name: %1\n").Arg(name));

    /*
    uint8_t gender = p.ReadU8();

    uint32_t skinType  = p.ReadU32Little();
    uint32_t faceType  = p.ReadU32Little();
    uint32_t hairType  = p.ReadU32Little();
    uint32_t hairColor = p.ReadU32Little();
    uint32_t eyeColor  = p.ReadU32Little();

    uint32_t equipTop    = p.ReadU32Little();
    uint32_t equipBottom = p.ReadU32Little();
    uint32_t equipFeet   = p.ReadU32Little();
    uint32_t equipComp   = p.ReadU32Little();
    uint32_t equipWeapon = p.ReadU32Little();
    */

    libcomp::Packet reply;
    reply.WriteU16Little(0x000E);
    reply.WriteU32Little(0);

    connection->SendPacket(reply);

    return true;
}
