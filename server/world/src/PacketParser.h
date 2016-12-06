/**
 * @file server/world/src/PacketParser.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Base class used to parse an internal world packet.
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

#ifndef SERVER_WORLD_SRC_PACKETPARSER_H
#define SERVER_WORLD_SRC_PACKETPARSER_H

// Standard C++ Includes
#include <memory>

namespace libcomp
{

class ReadOnlyPacket;
class TcpConnection;

} // namespace libcomp

namespace world
{

class ManagerPacket;

class PacketParser
{
public:
    PacketParser() { }
    virtual ~PacketParser() { }

    virtual bool Parse(ManagerPacket *pPacketManager,
        const std::shared_ptr<libcomp::TcpConnection>& connection,
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
            const std::shared_ptr<libcomp::TcpConnection>& connection, \
            libcomp::ReadOnlyPacket& p) const; \
    }

#endif // SERVER_WORLD_SRC_PACKETPARSER_H
