/**
 * @file libtester/src/HttpConnection.h
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief HTTP connection for the lobby login sequence.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
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

#include "HttpConnection.h"

// libcomp Includes
#include <MessagePacket.h>

using namespace libtester;

HttpConnection::HttpConnection(asio::io_service& io_service) :
    libcomp::TcpConnection(io_service)
{
}

void HttpConnection::SetMessageQueue(const std::shared_ptr<
    libcomp::MessageQueue<libcomp::Message::Message*>>& messageQueue)
{
    mMessageQueue = messageQueue;
}

void HttpConnection::ConnectionSuccess()
{
}

void HttpConnection::PacketReceived(libcomp::Packet& packet)
{
    // Promote to a shared pointer.
    std::shared_ptr<libcomp::TcpConnection> self = mSelf.lock();

    if(this == self.get() && nullptr != mMessageQueue)
    {
        // Copy the packet.
        libcomp::ReadOnlyPacket copy(std::move(packet));

        // Notify the task about the new packet.
        mMessageQueue->Enqueue(new libcomp::Message::Packet(self,
            0x0000u, copy));
    }
}
