/**
 * @file libcomp/src/ChannelConnection.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Channel connection class.
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

#ifndef LIBCOMP_SRC_CHANNELCONNECTION_H
#define LIBCOMP_SRC_CHANNELCONNECTION_H

// libcomp Includes
#include "MessageQueue.h"
#include "TcpConnection.h"

namespace libcomp
{

class ChannelConnection : public libcomp::TcpConnection
{
public:
    ChannelConnection(asio::io_service& io_service);
    ChannelConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);
    virtual ~ChannelConnection();

    virtual void ConnectionSuccess();

protected:
    typedef void (ChannelConnection::*PacketParser_t)(libcomp::Packet& packet);

    virtual void SocketError(const libcomp::String& errorMessage =
        libcomp::String());

    virtual void ConnectionEncrypted();

    virtual void PacketReceived(libcomp::Packet& packet);

    PacketParser_t mPacketParser;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_CHANNELCONNECTION_H
