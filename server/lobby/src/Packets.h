/**
 * @file server/lobby/src/Packets.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Classes used to parse client lobby packets.
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

#ifndef LIBCOMP_SRC_PACKETS_H
#define LIBCOMP_SRC_PACKETS_H

// libcomp Includes
#include <PacketParser.h>

namespace lobby
{

namespace Parsers
{

PACKET_PARSER_DECL(Login);               // 0x0003
PACKET_PARSER_DECL(Auth);                // 0x0005
PACKET_PARSER_DECL(StartGame);           // 0x0007
PACKET_PARSER_DECL(CharacterList);       // 0x0009
PACKET_PARSER_DECL(WorldList);           // 0x000B
PACKET_PARSER_DECL(CreateCharacter);     // 0x000D
PACKET_PARSER_DECL(DeleteCharacter);     // 0x000F
PACKET_PARSER_DECL(QueryPurchaseTicket); // 0x0011
PACKET_PARSER_DECL(PurchaseTicket);      // 0x0013

//Internal Packets
PACKET_PARSER_DECL(SetWorldDescription);      // 0x1002
PACKET_PARSER_DECL(SetChannelDescription);    // 0x1003

} // namespace Parsers

} // namespace lobby

#endif // LIBCOMP_SRC_PACKETS_H
