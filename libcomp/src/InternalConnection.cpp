/**
 * @file libcomp/src/InternalConnection.cpp
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Internal server connection class.
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

#include "InternalConnection.h"

// libcomp Includes
#include "Constants.h"
#include "Decrypt.h"
#include "Exception.h"
#include "Log.h"
#include "MessagePacket.h"
#include "TcpServer.h"

using namespace libcomp;

InternalConnection::InternalConnection(asio::io_service& io_service) :
    libcomp::TcpConnection(io_service), mPacketParser(nullptr)
{
}

InternalConnection::InternalConnection(asio::ip::tcp::socket& socket,
    DH *pDiffieHellman) : libcomp::TcpConnection(socket, pDiffieHellman),
    mPacketParser(nullptr)
{
}

InternalConnection::~InternalConnection()
{
}

void InternalConnection::SocketError(const libcomp::String& errorMessage)
{
    if(STATUS_NOT_CONNECTED != GetStatus())
    {
        if (GetRole() == ROLE_CLIENT)
        {
            LOG_DEBUG(libcomp::String("Server connection lost: %1\n").Arg(
                GetRemoteAddress()));
        }
        else
        {
            LOG_DEBUG(libcomp::String("Client disconnect: %1\n").Arg(
                GetRemoteAddress()));
        }
    }

    TcpConnection::SocketError(errorMessage);

    mPacketParser = nullptr;
}

void InternalConnection::ConnectionSuccess()
{
    auto role = GetRole();
    LOG_DEBUG(libcomp::String(role == ROLE_CLIENT ? "Server connection: %1\n" : "Client connection: %1\n").Arg(
        GetRemoteAddress()));

    //todo: properly handle initial communication, this is just a test
    if (role == ROLE_CLIENT)
    {
        mPacketParser = &InternalConnection::ParseClientEncryptionStart;

        //Send ping, receive pong

        libcomp::Packet packet;

        packet.WriteU32Big(1);
        packet.WriteU32Big(8);

        // Send a packet after connecting.
        SendPacket(packet);

        if (!RequestPacket(strlen(DH_BASE_STRING) + 2 * DH_KEY_HEX_SIZE +
            4 * sizeof(uint32_t)))
        {
            SocketError("Failed to request more data.");
        }
    }
    else
    {
        mPacketParser = &InternalConnection::ParseServerEncryptionStart;

        //Receive ping, Send pong

        if (!RequestPacket(strlen(DH_BASE_STRING) + 2 * DH_KEY_HEX_SIZE +
            4 * sizeof(uint32_t)))
        {
            SocketError("Failed to request more data.");
        }

        libcomp::Packet packet;

        packet.WriteU32Big(1);
        packet.WriteU32Big(8);

        // Send a packet after connecting.
        SendPacket(packet);
    }
}

//Copied from LobbyConnection
void InternalConnection::ParseClientEncryptionStart(libcomp::Packet& packet)
{
    // Check if we have all the data.
    if ((strlen(DH_BASE_STRING) + 2 * DH_KEY_HEX_SIZE +
        4 * sizeof(uint32_t)) > packet.Size())
    {
        // Keep reading the first reply.
        if (!RequestPacket(strlen(DH_BASE_STRING) + 2 * DH_KEY_HEX_SIZE +
            4 * sizeof(uint32_t) - packet.Size()))
        {
            SocketError("Failed to request more data.");
        }
    }
    else
    {
        // Parsing status.
        bool status = true;

        // Sanity check the packet contents.
        if (0 != packet.ReadU32Big())
        {
            SocketError("Failed to parse encryption data.");
            status = false;
        }

        // Check the size of the base.
        if (status && strlen(DH_BASE_STRING) != packet.PeekU32Big())
        {
            SocketError("Failed to parse encryption base.");
            status = false;
        }

        libcomp::String base;

        if (status)
        {
            base = packet.ReadString32Big(libcomp::Convert::ENCODING_UTF8);
        }

        // Check the base matches what is expected.
        if (status && DH_BASE_STRING != base)
        {
            SocketError("Failed to parse encryption base (not "
                DH_BASE_STRING ").");
            status = false;
        }

        // Check the size of the prime.
        if (status && DH_KEY_HEX_SIZE != packet.PeekU32Big())
        {
            SocketError("Failed to parse encryption prime.");
            status = false;
        }

        libcomp::String prime;

        if (status)
        {
            prime = packet.ReadString32Big(libcomp::Convert::ENCODING_UTF8);
        }

        // Check the size of the server public.
        if (status && DH_KEY_HEX_SIZE != packet.PeekU32Big())
        {
            SocketError("Failed to parse encryption server public.");
            status = false;
        }

        libcomp::String serverPublic;

        if (status)
        {
            serverPublic = packet.ReadString32Big(
                libcomp::Convert::ENCODING_UTF8);
        }

        // Make sure we read the entire packet.
        if (status && 0 == packet.Left())
        {
            mStatus = STATUS_WAITING_ENCRYPTION;

            // Load the prime and base.
            mDiffieHellman = libcomp::TcpServer::LoadDiffieHellman(prime);

            // Generate the client public.
            libcomp::String clientPublic = GenerateDiffieHellmanPublic(
                mDiffieHellman);

            // Generate the shared data.
            std::vector<char> sharedData = GenerateDiffieHellmanSharedData(
                mDiffieHellman, serverPublic);

            if (DH_KEY_HEX_SIZE != clientPublic.Length() ||
                DH_SHARED_DATA_SIZE != sharedData.size())
            {
                // Get ready for the next packet.
                packet.Clear();

                SocketError("Failed to generate encryption client public and "
                    "shared data.");
            }
            else
            {
                libcomp::Packet reply;

                // Form the reply.
                reply.WriteString32Big(libcomp::Convert::ENCODING_UTF8,
                    clientPublic);

                // Send the reply.
                SendPacket(reply);

                // Set the encryption key.
                SetEncryptionKey(sharedData);

                // We are now encrypted.
                mStatus = STATUS_ENCRYPTED;

                // Use this packet parser now.
                mPacketParser = &InternalConnection::ParsePacket;

                // Get ready for the next packet.
                packet.Clear();

                // Callback.
                ConnectionEncrypted();
            }
        }
        else
        {
            // Get ready for the next packet.
            packet.Clear();

            SocketError("Read too much data for packet.");
        }
    }
}

//Copied from LobbyConnection
void InternalConnection::ParseServerEncryptionStart(libcomp::Packet& packet)
{
    // Check if we have all the data.
    if ((2 * sizeof(uint32_t)) > packet.Size())
    {
        // Keep reading the first packet.
        if (!RequestPacket(2 * sizeof(uint32_t) - packet.Size()))
        {
            SocketError("Failed to request more data.");
        }
    }
    else
    {
        uint32_t first = packet.ReadU32Big();
        uint32_t second = packet.ReadU32Big();

        if (0 == packet.Left() && 1 == first && 8 == second)
        {
            mStatus = STATUS_WAITING_ENCRYPTION;

            libcomp::Packet reply;

            reply.WriteBlank(4);
            reply.WriteString32Big(libcomp::Convert::ENCODING_UTF8,
                DH_BASE_STRING);
            reply.WriteString32Big(libcomp::Convert::ENCODING_UTF8,
                GetDiffieHellmanPrime(mDiffieHellman));
            reply.WriteString32Big(libcomp::Convert::ENCODING_UTF8,
                GenerateDiffieHellmanPublic(mDiffieHellman));

            SendPacket(reply);

            mPacketParser = &InternalConnection::ParseServerEncryptionFinish;

            // Get ready for the next packet.
            packet.Clear();

            // Wait for the client public.
            if (!RequestPacket(DH_KEY_HEX_SIZE + sizeof(uint32_t)))
            {
                SocketError("Failed to request more data.");
            }
        }
        else
        {
            // Get ready for the next packet.
            packet.Clear();

            SocketError("Read too much data for packet.");
        }
    }
}

//Copied from LobbyConnection
void InternalConnection::ParseServerEncryptionFinish(libcomp::Packet& packet)
{
    // Check if we have all the data.
    if ((DH_KEY_HEX_SIZE + sizeof(uint32_t)) > packet.Size())
    {
        // Keep reading the packet.
        if (!RequestPacket(DH_KEY_HEX_SIZE + sizeof(uint32_t) - packet.Size()))
        {
            SocketError("Failed to request more data.");
        }
    }
    else
    {
        // Parsing status.
        bool status = true;

        // Check the size of the client public.
        if (status && DH_KEY_HEX_SIZE != packet.PeekU32Big())
        {
            SocketError("Failed to parse encryption client public.");
            status = false;
        }

        libcomp::String clientPublic;

        if (status)
        {
            clientPublic = packet.ReadString32Big(
                libcomp::Convert::ENCODING_UTF8);
        }

        // Make sure we read the entire packet.
        if (status && 0 == packet.Left())
        {
            std::vector<char> sharedData = GenerateDiffieHellmanSharedData(
                mDiffieHellman, clientPublic);

            if (DH_SHARED_DATA_SIZE != sharedData.size())
            {
                // Get ready for the next packet.
                packet.Clear();

                SocketError("Failed to generate shared data.");
            }
            else
            {
                // Set the encryption key.
                SetEncryptionKey(sharedData);

                // We are now encrypted.
                mStatus = STATUS_ENCRYPTED;

                // Use this packet parser now.
                mPacketParser = &InternalConnection::ParsePacket;

                // Get ready for the next packet.
                packet.Clear();

                // Callback.
                ConnectionEncrypted();
            }
        }
        else
        {
            // Get ready for the next packet.
            packet.Clear();

            SocketError("Read too much data for packet.");
        }
    }
}

//Copied from LobbyConnection
void InternalConnection::ParsePacket(libcomp::Packet& packet)
{
    (void)packet;

    if (STATUS_ENCRYPTED == GetStatus())
    {
        // Check if we have all the data.
        if ((2 * sizeof(uint32_t)) > packet.Size())
        {
            // Keep reading until we have the packet sizes.
            if (!RequestPacket(2 * sizeof(uint32_t) - packet.Size()))
            {
                SocketError("Failed to request more data.");
            }
        }
        else
        {
            // Read the sizes.
            uint32_t paddedSize = packet.ReadU32Big();
            uint32_t realSize = packet.ReadU32Big();

            // Check for enough packet data (the sizes are not included).
            if ((paddedSize + 2 * sizeof(uint32_t)) > packet.Size())
            {
                // Keep reading until we have the packet.
                if (!RequestPacket(paddedSize + 2 * sizeof(uint32_t) -
                    packet.Size()))
                {
                    SocketError("Failed to request more data.");
                }
            }
            else
            {
                // We have a full packet, handle it now.
                ParsePacket(packet, paddedSize, realSize);

                // Get ready for the next packet.
                packet.Clear();
            }
        }
    }
    else
    {
        // Get ready for the next packet.
        packet.Clear();

        SocketError("Connection should be encrypted but isn't.");
    }
}

//Copied from LobbyConnection
void InternalConnection::ParsePacket(libcomp::Packet& packet,
    uint32_t paddedSize, uint32_t realSize)
{
    // Decrypt the packet
    Decrypt::DecryptPacket(mEncryptionKey, packet);

    // Move the packet into a read only copy.
    ReadOnlyPacket copy(packet);

    // Make sure we are at the right spot (right after the sizes).
    copy.Seek(2 * sizeof(uint32_t));

    // Calculate how much data is padding.
    uint32_t padding = paddedSize - realSize;

    // This will stop the command parsing.
    bool errorFound = false;

    // Keep reading each command (sometimes called a packet) inside the
    // decrypted packet from the network socket.
    while (!errorFound && copy.Left() > padding)
    {
        // Make sure there is enough data
        if (copy.Left() < 3 * sizeof(uint16_t))
        {
            SocketError("Corrupt packet (not enough data for command header).");

            errorFound = true;
        }
        else
        {
            // Skip over the big endian size (we think).
            copy.Skip(2);

            // Remember where this command started so we may advance over it
            // after it has been parsed.
            uint32_t commandStart = copy.Tell();
            uint16_t commandSize = copy.ReadU16Little();
            uint16_t commandCode = copy.ReadU16Little();

            // With no data, the command size is 4 bytes (code + a size).
            if (commandSize < 2 * sizeof(uint16_t))
            {
                SocketError("Corrupt packet (not enough data for command).");

                errorFound = true;
            }

            // Check there is enough packet left for the command data.
            if (!errorFound && copy.Left() < (uint32_t)(commandSize -
                2 * sizeof(uint16_t)))
            {
                copy.HexDump();
                SocketError("Corrupt packet (not enough data for "
                    "command data).");

                errorFound = true;
            }

            if (!errorFound && nullptr == mMessageQueue)
            {
                SocketError("No message queue for packet.");

                errorFound = true;
            }

            // Promote to a shared pointer.
            std::shared_ptr<libcomp::TcpConnection> self = mSelf.lock();

            if (!errorFound && this != self.get())
            {
                SocketError("Failed to obtain a shared pointer.");

                errorFound = true;
            }

            if (!errorFound)
            {
                // This is a shallow copy of the command data.
                ReadOnlyPacket command(copy, commandStart +
                    2 * static_cast<uint32_t>(sizeof(uint16_t)),
                    commandSize - 2 * static_cast<uint32_t>(
                        sizeof(uint16_t)));

                //todo: split between seperate worker/server queues

                // Notify the task about the new packet.
                mMessageQueue->Enqueue(new libcomp::Message::Packet(self,
                    commandCode, copy));
            }

            // Move to the next command.
            if (!errorFound)
            {
                copy.Seek(commandStart + commandSize);
            }
        }
    } // while(!errorFound && packet.Left() > padding)

    if (!errorFound)
    {
        // Skip the padding
        copy.Skip(padding);
    }

    if (!errorFound && copy.Left() != 0)
    {
        SocketError("Corrupt packet has extra data.");
    }
}

void InternalConnection::ConnectionEncrypted()
{
    /// @todo Implement (send an event to the queue).
    LOG_DEBUG("Connection encrypted!\n");

    // Start reading until we have the packet sizes.
    if(!RequestPacket(2 * sizeof(uint32_t)))
    {
        SocketError("Failed to request more data.");
    }
}

void InternalConnection::PacketReceived(libcomp::Packet& packet)
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

//Copied from LobbyConnection
void InternalConnection::SetMessageQueue(const std::shared_ptr<
    MessageQueue<libcomp::Message::Message* >> &messageQueue)
{
    mMessageQueue = messageQueue;
}