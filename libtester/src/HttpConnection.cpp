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
    size_t actualSize = 0;
    size_t requestSize = 9999; // over 9000!

    // Add the packet onto the request.
    if(0 < packet.Size())
    {
        mRequest += std::string(packet.ConstData(), packet.Size());
    }

    // Clear the packet.
    packet.Clear();

    // Find the sequence that indicates the end of the HTTP header.
    auto headerEnd = mRequest.find("\r\n\r\n");

    // Parse the header if it's been read.
    if(std::string::npos != headerEnd)
    {
        std::smatch match;

        // Check for the content length.
        if(std::regex_search(mRequest, match,
            std::regex("Content-Length: ([0-9]+)")))
        {
            bool ok = false;
            size_t contentLength = libcomp::String("%1").Arg(
                match.str(1)).ToInteger<size_t>(&ok);

            if(ok)
            {
                // Determine the size of the HTTP request.
                actualSize = headerEnd + strlen("\r\n\r\n") + contentLength;

                // Determine how much more needs to be read.
                if(mRequest.size() < actualSize)
                {
                    requestSize = actualSize - mRequest.size();
                }
                else
                {
                    requestSize = 0;
                }
            }
        }
    }

    // If more data is required, keep looking for more data.
    if(0 < requestSize)
    {
        (void)RequestPacket(requestSize);

        return;
    }

    // Remove the request from the buffer.
    std::string request = mRequest.substr(0, actualSize);
    mRequest = mRequest.substr(actualSize);

    // Save the request into a packet.
    libcomp::Packet requestPacket;
    requestPacket.WriteArray(request.c_str(), static_cast<uint32_t>(
        request.size()));
    requestPacket.Rewind();

    // Promote to a shared pointer.
    auto self = shared_from_this();

    if(this == self.get() && nullptr != mMessageQueue)
    {
        // Copy the packet.
        libcomp::ReadOnlyPacket copy(std::move(requestPacket));

        // Notify the task about the new packet.
        mMessageQueue->Enqueue(new libcomp::Message::Packet(self,
            0x0000u, copy));
    }
}
