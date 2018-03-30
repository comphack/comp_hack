/**
 * @file libcomp/src/PacketParser.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class used to parse an internal or client packet.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

// Standard C++ Includes
#include <memory>

namespace libcomp
{

class ReadOnlyPacket;
class TcpConnection;

} // namespace libcomp

namespace libcomp
{

class ManagerPacket;

/**
 * Class responsible for parsing and handling packet.
 */
class PacketParser
{
public:
    /**
     * Create the parser.
     */
    PacketParser() { }

    /**
     * Clean up the parser.
     */
    virtual ~PacketParser() { }

    /**
     * Parse and handle a packet.  This should be implemented by
     * multiple different parser classes that are registerd using
     * the PACKET_PARSER_DECL(name) command.
     * @param pPacketManager Manager that received the packet
     * @param connection Connection that sent the pacekt
     * @param p Packet to parse and handle
     * @return true on success, false on failure
     */
    virtual bool Parse(ManagerPacket *pPacketManager,
        const std::shared_ptr<libcomp::TcpConnection>& connection,
        libcomp::ReadOnlyPacket& p) const = 0;
};

} // namespace libcomp

#define PACKET_PARSER_DECL(name) \
    class name : public libcomp::PacketParser \
    { \
    public: \
        name() : libcomp::PacketParser() { } \
        virtual ~name() { } \
        \
        virtual bool Parse(libcomp::ManagerPacket *pPacketManager, \
            const std::shared_ptr<libcomp::TcpConnection>& connection, \
            libcomp::ReadOnlyPacket& p) const; \
    }

#endif // LIBCOMP_SRC_PACKETPARSER_H
