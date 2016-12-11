/**
 * @file libcomp/src/PacketCodes.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Contains enums for internal and external packet codes.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#ifndef LIBCOMP_SRC_PACKETCODES_H
#define LIBCOMP_SRC_PACKETCODES_H

typedef enum
{
    PACKET_LOGIN = 0x0003,
    PACKET_AUTH = 0x0005,
    PACKET_START_GAME = 0x0007,
    PACKET_CHARACTER_LIST = 0x0009,
    PACKET_WORLD_LIST = 0x000B,
    PACKET_CREATE_CHARACTER = 0x000D,
    PACKET_DELETE_CHARACTER = 0x000F,
    PACKET_QUERY_PURCHASE_TICKET = 0x0011,
    PACKET_PURCHASE_TICKET = 0x0013,
} ClientPacketCode_t;

typedef enum
{
    PACKET_DESCRIBE_WORLD = 0x1001,
    PACKET_SET_WORLD_DESCRIPTION = 0x1002,
    PACKET_SET_CHANNEL_DESCRIPTION = 0x1003,
} InternalPacketCode_t;

typedef enum
{
    PACKET_ACTION_ADD = 1,
    PACKET_ACTION_UPDATE,
    PACKET_ACTION_REMOVE,
} InternalPacketAction_t;

#endif // LIBCOMP_SRC_PACKETCODES_H
