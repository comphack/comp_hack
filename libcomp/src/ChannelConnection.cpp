/**
 * @file libcomp/src/ChannelConnection.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
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

#include "ChannelConnection.h"

// libcomp Includes
#include "Decrypt.h"
#include "Log.h"

using namespace libcomp;

ChannelConnection::ChannelConnection(asio::io_service& io_service) :
    libcomp::EncryptedConnection(io_service)
{
}

ChannelConnection::ChannelConnection(asio::ip::tcp::socket& socket,
    DH *pDiffieHellman) : libcomp::EncryptedConnection(socket, pDiffieHellman)
{
}

ChannelConnection::~ChannelConnection()
{
}

void ChannelConnection::PreparePackets(std::list<ReadOnlyPacket>& packets)
{
    static const uint32_t headerSize = 6 * sizeof(uint32_t);

    if(STATUS_ENCRYPTED == mStatus)
    {
        int retryCount = 0;
        bool packetOK = false;

        Packet finalPacket;

        // We will do this 1-2 times depending on if it compressed right.
        while(!packetOK && 2 > retryCount++)
        {
            // Reserve space for the sizes.
            finalPacket.WriteBlank(headerSize);

            // Now add the packet data.
            for(auto& packet : packets)
            {
                finalPacket.WriteU16Big((uint16_t)(packet.Size() + 2));
                finalPacket.WriteU16Little((uint16_t)(packet.Size() + 2));
                finalPacket.WriteArray(packet.ConstData(), packet.Size());
            }

            int32_t originalSize = static_cast<int32_t>(
                finalPacket.Size() - headerSize);
            int compressedSize;

            // Compress the packet if this is the first try.
            if(1 == retryCount)
            {
                finalPacket.Seek(headerSize);

                // Attempt to compress the packet.
                compressedSize = finalPacket.Compress(originalSize);

                // If they are equal, this packet might be confused with an
                // uncompressed one. In such a case, do not compress.
                if(compressedSize < originalSize && 0 < compressedSize)
                {
                    // Packet is OK.
                    packetOK = true;
                }
                else
                {
                    // Erase the final packet and create it again.
                    finalPacket.Clear();
                    finalPacket.Rewind();
                }
            }
            else
            {
                // Same as the uncompressed size.
                compressedSize = originalSize;

                // Packet is OK.
                packetOK = true;
            }

            // Write the uncompressed and compressed sizes.
            if(packetOK)
            {
                // Move to where the uncompressed and compressed sizes are.
                finalPacket.Seek(2 * sizeof(uint32_t));

                // Write the sizes.
                finalPacket.WriteArray("gzip", 4);
                finalPacket.WriteS32Little(originalSize);
                finalPacket.WriteS32Little(compressedSize);
                finalPacket.WriteArray("lv6", 4);
            }
        }

        // If the packet is OK, encrypt and complete the procedure.
        if(packetOK)
        {
            // Encrypt the packet
            Decrypt::EncryptPacket(mEncryptionKey, finalPacket);

            mOutgoing = finalPacket;
        }
        else
        {
            // We should never get here.
            SocketError();
        }
    }
    else
    {
        // Just use the base class code.
        EncryptedConnection::PreparePackets(packets);
    }
}

bool ChannelConnection::DecompressPacket(libcomp::Packet& packet,
    uint32_t& paddedSize, uint32_t& realSize, uint32_t& dataStart)
{
    // Make sure we are at the right spot (right after the sizes).
    packet.Seek(2 * sizeof(uint32_t));

    // All packets that support compression have this.
    if(0x677A6970 != packet.ReadU32Big()) // "gzip"
    {
        SocketError();

        return false;
    }

    // Read the sizes.
    int32_t uncompressedSize = packet.ReadS32Little();
    int32_t compressedSize = packet.ReadS32Little();

    // Sanity check the sizes.
    if(0 > uncompressedSize || 0 > compressedSize)
    {
        SocketError();

        return false;
    }

    // Check that the compression is as expected.
    if(0x6C763600 != packet.ReadU32Big()) // "lv6\0"
    {
        SocketError();

        return false;
    }

    // Calculate how much data is padding
    uint32_t padding = paddedSize - realSize;

    // Make sure the packet is the right size.
    if(packet.Left() != (static_cast<uint32_t>(compressedSize) + padding))
    {
        SocketError();

        return false;
    }

    // Only decompress if the sizes are not the same.
    if(compressedSize != uncompressedSize)
    {
        // Attempt to decompress.
        int32_t sz = packet.Decompress(compressedSize);

        // Check the uncompressed size matches the recorded size.
        if(sz != uncompressedSize)
        {
            SocketError();

            return false;
        }

        // The is no padding anymore.
        paddedSize = realSize = static_cast<uint32_t>(sz);
    }

    // Skip over: gzip, lv0, uncompressedSize, compressedSize.
    dataStart += static_cast<uint32_t>(sizeof(uint32_t) * 4);

    return true;
}
