/**
 * @file libcomp/src/MessagePacket.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet received message.
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

#ifndef LIBCOMP_SRC_MESSAGEPACKET_H
#define LIBCOMP_SRC_MESSAGEPACKET_H

// libcomp Includes
#include "Message.h"
#include "ReadOnlyPacket.h"

// Standard C++11 Includes
#include <memory>

namespace libcomp
{

class TcpConnection;

namespace Message
{

/**
 * Message containing a packet received by an internal server
 * or game client connection.
 * @sa ReadOnlyPacket
 */
class Packet : public Message
{
public:
    /**
     * Create the message.
     * @param connection The connection the packet came from
     * @param commandCode Integer value representing the command
     *  code to use when deciding which @ref ManagerPacket to use
     *  to handle the message
     * @param packet The packet received
     */
    Packet(const std::shared_ptr<TcpConnection>& connection,
        uint16_t commandCode, ReadOnlyPacket& packet);

    /**
     * Cleanup the message.
     */
    virtual ~Packet();

    /**
     * Get the received packet.
     * @return The received packet
     */
    const ReadOnlyPacket& GetPacket() const;

    /**
     * Get the received packet's command code.
     * @return The received packet's command code
     */
    uint16_t GetCommandCode() const;

    /**
     * Get the connection the packet came from.
     * @return The connection the packet came from
     */
    std::shared_ptr<TcpConnection> GetConnection() const;

    virtual MessageType GetType() const;

    virtual libcomp::String Dump() const override;

private:
    /// The received packet
    ReadOnlyPacket mPacket;

    /// The received packet's command code
    uint16_t mCommandCode;

    /// The connection the packet came from
    std::shared_ptr<TcpConnection> mConnection;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGEPACKET_H
