/**
 * @file server/world/src/Packets.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Classes used to parse internal world packets.
 *
 * This file is part of the World Server (world).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#ifndef SERVER_WORLD_SRC_PACKETS_H
#define SERVER_WORLD_SRC_PACKETS_H

// world Includes
#include "PacketParser.h"

namespace world
{

namespace Parsers
{

PACKET_PARSER_DECL(GetWorldInfo);      // 0x1001
PACKET_PARSER_DECL(SetChannelInfo);    // 0x1003
PACKET_PARSER_DECL(AccountLogin);      // 0x1004
PACKET_PARSER_DECL(AccountLogout);     // 0x1005
PACKET_PARSER_DECL(Relay);             // 0x1006
PACKET_PARSER_DECL(DataSync);          // 0x1007
PACKET_PARSER_DECL(CharacterLogin);    // 0x1008
PACKET_PARSER_DECL(FriendsUpdate);     // 0x1009
PACKET_PARSER_DECL(PartyUpdate);       // 0x100A
PACKET_PARSER_DECL(ClanUpdate);        // 0x100B
PACKET_PARSER_DECL(WebGame);           // 0x100C
PACKET_PARSER_DECL(TeamUpdate);        // 0x100D

} // namespace Parsers

} // namespace world

#endif // SERVER_WORLD_SRC_PACKETS_H
