/**
 * @file libcomp/src/LobbyConnection.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby connection class.
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

#include "LobbyConnection.h"

// libcomp Includes
#include "Log.h"
#include "MessagePong.h"
#include "MessageWorldNotification.h"

using namespace libcomp;

LobbyConnection::LobbyConnection(asio::io_service& io_service,
    ConnectionMode_t mode) : libcomp::EncryptedConnection(io_service),
    mMode(mode)
{
}

LobbyConnection::LobbyConnection(asio::ip::tcp::socket& socket,
    DH *pDiffieHellman) : libcomp::EncryptedConnection(socket, pDiffieHellman),
    mMode(ConnectionMode_t::MODE_NORMAL)
{
}

LobbyConnection::~LobbyConnection()
{
}

void LobbyConnection::ConnectionSuccess()
{
    if(ROLE_CLIENT != GetRole() || ConnectionMode_t::MODE_NORMAL == mMode)
    {

        EncryptedConnection::ConnectionSuccess();
    }
    else
    {
        LOG_DEBUG(libcomp::String("Client connection: %1\n").Arg(
            GetRemoteAddress()));

        libcomp::Packet packet;

        switch(mMode)
        {
            case ConnectionMode_t::MODE_PING:
            {
                mPacketParser = static_cast<PacketParser_t>(
                    &LobbyConnection::ParseExtension);

                // Read the PONG packet.
                if(!RequestPacket(2 * sizeof(uint32_t)))
                {
                    SocketError("Failed to request more data.");
                }

                packet.WriteU32Big(2);
                packet.WriteU32Big(8);

                // Send a packet after connecting.
                SendPacket(packet);

                break;
            }
            case ConnectionMode_t::MODE_WORLD_UP:
            {
                mPacketParser = static_cast<PacketParser_t>(
                    &LobbyConnection::ParseExtension);

                // Read the reply packet.
                if(!RequestPacket(2 * sizeof(uint32_t)))
                {
                    SocketError("Failed to request more data.");
                }

                /// @todo Read the server port from the configuraton
                /// or have the server pass it in.
                packet.WriteU32Big(3 | (18666 << 16));
                packet.WriteU32Big(8);

                // Send a packet after connecting.
                SendPacket(packet);

                break;
            }
            default:
                break;
        }
    }
}

bool LobbyConnection::ParseExtensionConnection(libcomp::Packet& packet)
{
    uint32_t first = packet.ReadU32Big();
    uint32_t second = packet.ReadU32Big();

    if(0 == packet.Left() && 2 == first && 8 == second)
    {
        // Remove the packet.
        packet.Clear();

        // This is a ping, issue a pong or notify pong received.
        if(ROLE_CLIENT == GetRole())
        {
            // This is a pong, notify it was received.
            LOG_DEBUG("Got a PONG from the server.\n");

            SendMessage([](const std::shared_ptr<libcomp::TcpConnection>&){
                return new libcomp::Message::Pong();
            });
        }
        else
        {
            // This is a ping, issue a pong.
            LOG_DEBUG("Got a PING from the client.\n");

            // Set the name of the connection.
            SetName("ping");

            libcomp::Packet reply;

            reply.WriteU32Big(2);
            reply.WriteU32Big(8);

            // Send the pong and then close the connection.
            SendPacket(reply, true);
        }

        return true;
    }
    else if(0 == packet.Left() && 3 == (first & 0xFFFF) && 8 == second)
    {
        // Remove the packet.
        packet.Clear();

        // This is a world server notification.
        if(ROLE_CLIENT == GetRole())
        {
            // This is a pong, notify it was received.
            LOG_DEBUG("Lobby server got the notification.\n");

            SendMessage([](const std::shared_ptr<libcomp::TcpConnection>&){
                return new libcomp::Message::WorldNotification(
                    libcomp::String(), 0);
            });
        }
        else
        {
            // This is a ping, issue a pong.
            LOG_DEBUG("Got a world server notification.\n");

            uint16_t worldServerPort = static_cast<uint16_t>(
                (first >> 16) & 0xFFFF);

            // Set the name of the connection to the world's port.
            SetName(String("world_port:%1").Arg(worldServerPort));

            SendMessage([&](const std::shared_ptr<libcomp::TcpConnection>&){
                return new libcomp::Message::WorldNotification(
                    GetRemoteAddress(), worldServerPort);
            });

            libcomp::Packet reply;

            reply.WriteU32Big(3);
            reply.WriteU32Big(8);

            // Send the pong and then close the connection.
            SendPacket(reply, true);
        }

        return true;
    }

    return false;
}

void LobbyConnection::ParseExtension(libcomp::Packet& packet)
{
    // Just parse the extension.
    (void)ParseExtensionConnection(packet);
}
