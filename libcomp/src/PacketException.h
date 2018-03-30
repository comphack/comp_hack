/**
 * @file libcomp/src/PacketException.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet exception class definition.
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

#ifndef LIBCOMP_SRC_PACKETEXCEPTION_H
#define LIBCOMP_SRC_PACKETEXCEPTION_H

#include "Exception.h"
#include "Packet.h"

namespace libcomp
{

/**
 * Exception in reading to or writing from a Packet object. This class is
 * thrown when bugs crop up in packet parsing or writing code. As a general
 * rule of thumb no data being read from a packet should be trusted. Start by
 * writing code to check the size of the packet. If the packet is not a fixed
 * size, check for a minimum size before reading in any values that determine
 * the dynamic size. Dynamic size values should be checked for sane values and
 * that the calculated size of the packet is exactly the size of the packet.
 * While a larger packet won't throw an exception, the code should not allow
 * for extra data tacked on to the end of a packet. When writing packets, be
 * aware of the maximum packet size (@ref MAX_PACKET_SIZE). This is especially
 * true when sending large packets like files to the console or a large number
 * of action reports from mob kills.
 *
 * Aside from sanity checking dynamic packet sizes, you should still sanity
 * check all values in the packet. For example, if you receive a movement
 * packet you should check the origin position is the last known location of
 * the character and the destination is valid. The path between these two
 * points should be checked for collisions. No time value for a movement packet
 * should exceed 1/2 second and the distance should be calculated and used to
 * determine the speed of the movement. This speed should be within the
 * allowable range of the character or object being moved. The client should
 * only be able to move their character or active demon (if one is summoned).
 * Thus, the object UID should be compared to these two UID values. Keeping
 * this in mind while parsing packets should help prevent packet exceptions and
 * bugs in your code.
 *
 * Like all exceptions, a packet exception will contain the backtrace as well
 * as the file and line the exception was generated from. A copy of the packet
 * is included in the exception to determine what went wrong in parsing or
 * generation. The position in the packet will be retained for this purpose.
 * To see the current state of the packet use the @ref packet method or
 * output it to the log using @ref log.
 */
class PacketException : public Exception
{
public:
    /**
     * Construct a packet exception. It's better to use @ref PACKET_EXCEPTION
     * instead of throwing this constructor.
     * @param msg Message describing the packet exception.
     * @param packet Packet that caused the exception.
     * @param file File the exception was thrown from.
     * @param line Line in the file the exception was thrown from.
     */
    PacketException(const String& msg, const ReadOnlyPacket *pPacket,
        const String& file, int line);

    /**
     * Copy of the packet in which the exception occured.
     * @returns Copy of the packet.
     */
    Packet& GetPacket();

    /**
     * Immutable copy of the packet in which the exception occured.
     * @returns Copy of the packet.
     */
    const Packet& GetPacket() const;

    /**
     * Log the exception. This will include a dump of the packet when the
     * exception occured.
     */
    virtual void Log() const;

private:
    /// Copy of the packet in which the exception occured.
    Packet mPacket;
};

} // namespace libcomp

/**
 * Throw a packet exception. This macro will include the current file and line
 * where the packet is thrown from. Include a message @em msg and the
 * @em packet that caused the exception.
 * @param msg Message describing the exception.
 * @param packet Packet that caused the exception.
 */
#define PACKET_EXCEPTION(msg, packet) throw libcomp::PacketException(msg, \
    packet, __FILE__, __LINE__)

#endif // LIBCOMP_SRC_PACKETEXCEPTION_H
