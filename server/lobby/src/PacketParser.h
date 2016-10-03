/**
 * @file server/lobby/src/PacketParser.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class used to parse a client lobby packet.
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

#ifndef LIBCOMP_SRC_PACKETPARSER_H
#define LIBCOMP_SRC_PACKETPARSER_H

namespace libcomp
{

class ReadOnlyPacket;

} // namespace libcomp

namespace lobby
{

class ManagerPacket;

class PacketParser
{
public:
    PacketParser() { }
    virtual ~PacketParser() { }

    virtual bool Parse(ManagerPacket *pPacketManager,
        libcomp::ReadOnlyPacket& p) const = 0;
};

} // namespace libcomp

#define PACKET_PARSER_DECL(name) \
    class name : public PacketParser \
    { \
    public: \
        name() : PacketParser() { } \
        virtual ~name() { } \
        \
        virtual bool Parse(ManagerPacket *pPacketManager, \
            libcomp::ReadOnlyPacket& p) const; \
    }

#endif // LIBCOMP_SRC_PACKETPARSER_H
