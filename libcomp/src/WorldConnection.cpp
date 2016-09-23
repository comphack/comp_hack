/**
 * @file libcomp/src/WorldConnection.cpp
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief World connection class.
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

#include "WorldConnection.h"

// libcomp Includes
#include "Constants.h"
#include "Decrypt.h"
#include "Exception.h"
#include "Log.h"
#include "MessagePacket.h"
#include "TcpServer.h"

using namespace libcomp;

WorldConnection::WorldConnection(asio::io_service& io_service) :
    libcomp::TcpConnection(io_service), mPacketParser(nullptr)
{
}

WorldConnection::WorldConnection(asio::ip::tcp::socket& socket,
    DH *pDiffieHellman) : libcomp::TcpConnection(socket, pDiffieHellman),
    mPacketParser(nullptr)
{
}

WorldConnection::~WorldConnection()
{
}

void WorldConnection::SocketError(const libcomp::String& errorMessage)
{
    if(STATUS_NOT_CONNECTED != GetStatus())
    {
        LOG_DEBUG(libcomp::String("Client disconnect: %1\n").Arg(
            GetRemoteAddress()));
    }

    TcpConnection::SocketError(errorMessage);

    mPacketParser = nullptr;
}

void WorldConnection::ConnectionSuccess()
{
    LOG_DEBUG(libcomp::String("Client connection: %1\n").Arg(
        GetRemoteAddress()));
}

void WorldConnection::ConnectionEncrypted()
{
    /// @todo Implement (send an event to the queue).
    LOG_DEBUG("Connection encrypted!\n");

    // Start reading until we have the packet sizes.
    if(!RequestPacket(2 * sizeof(uint32_t)))
    {
        SocketError("Failed to request more data.");
    }
}

void WorldConnection::PacketReceived(libcomp::Packet& packet)
{
    // Pass the packet along to the parser.
    if(nullptr != mPacketParser)
    {
        try
        {
            (*this.*mPacketParser)(packet);
        }
        catch(libcomp::Exception& e)
        {
            e.Log();

            // This connection is now bad; kill it.
            SocketError();
        }
    }
}