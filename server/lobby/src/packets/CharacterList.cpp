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

bool Parsers::CharacterList::Parse(ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    libcomp::Packet reply;
    reply.WriteU16Little(0x000A);

    // Time of last login (time_t).
    reply.WriteU32Little((uint32_t)time(0));

    // Number of character tickets.
    reply.WriteU8(1);

    // Number of characters.
    reply.WriteU8(1);

    {
        // Character ID.
        reply.WriteU8(0);

        // World ID.
        reply.WriteU8(0);

        // Name.
        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            "テスト", true);

        // Gender.
        reply.WriteU8(0);

        // Time when the character will be deleted.
        reply.WriteU32Little(0);

        // Cutscene to play on login (0 for none).
        reply.WriteU32Little(0x001EFC77);

        // Last channel used???
        reply.WriteS8(-1);

        // Level.
        reply.WriteU8(1);

        // Skin type.
        reply.WriteU8(0x65);

        // Hair type.
        reply.WriteU8(8);

        // Eye type.
        reply.WriteU8(1);

        // Face type.
        reply.WriteU8(1);

        // Hair color.
        reply.WriteU8(8);

        // Left eye color.
        reply.WriteU8(0x64);

        // Right eye color.
        reply.WriteU8(0x3F);

        // Unkown values.
        reply.WriteU8(0);
        reply.WriteU8(1);

        // Equipment
        for(int z = 0; z < 15; z++)
        {
            // None.
            reply.WriteU32Little(0x7FFFFFFF);
        }
    }

    connection->SendPacket(reply);

    return true;
}
