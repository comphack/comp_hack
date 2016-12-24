/**
 * @file libcomp/src/Packet.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parsing and creation class implementation.
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

#include "Packet.h"

#include "Compress.h"
#include "Endian.h"
#include "Log.h"
#include "PacketException.h"
#include "ScriptEngine.h"

#include <zlib.h>

#ifdef _WIN32
#include <windows.h>
#else // _WIN32
#include <sys/socket.h>
#endif // _WIN32

#include <cstring>
#include <cstdio>

using namespace libcomp;

Packet::Packet() : ReadOnlyPacket()
{
    // Ensure the packet is clear and the variables are set.
    Clear();
}

Packet::Packet(const Packet& other) : ReadOnlyPacket(other.mPosition,
    other.mSize, nullptr, nullptr)
{
    Allocate();

    // Make sure the data pointer is valid first.
    if(nullptr != mData)
    {
        memcpy(mData, other.mData, other.mSize);
    }
}

Packet::Packet(Packet&& other) : ReadOnlyPacket(other.mPosition, other.mSize,
    other.mData, other.mDataRef)
{
    other.mPosition = 0;
    other.mSize = 0;
    other.mDataRef.reset();
    other.mData = nullptr;

    // Ensure the packet is clear and the variables are set.
    other.Clear();
}

Packet::Packet(const std::vector<char>& data) : ReadOnlyPacket()
{
    // Ensure the packet is clear and the variables are set.
    Clear();

    // If there is data to be written, use writeArray() to write it.
    if(!data.empty())
    {
        // Allocate the packet data.
        Allocate();

        // Write the data.
        WriteArray(data);

        // Rewind the packet back to the beginning.
        Rewind();
    }
}

Packet::Packet(const void *pData, uint32_t sz) : ReadOnlyPacket()
{
    // Ensure the packet is clear and the variables are set.
    Clear();

    // If there is data to be written, use writeArray() to write it.
    if(0 < sz)
    {
        // Allocate the packet data.
        Allocate();

        // Write the data.
        WriteArray(pData, sz);

        // Rewind the packet back to the beginning.
        Rewind();
    }
}

Packet::~Packet()
{
}

void Packet::GrowPacket(uint32_t sz)
{
    // Allocate the packet data (if needed).
    Allocate();

    // Make sure the packet is growing.
    if(0 == sz)
    {
        PACKET_EXCEPTION("Attempted to grow the packet by 0 bytes", this);
    }

    // Calculate how big the packet will be if it can grow.
    uint32_t newSize = mPosition + sz;

    /* If the new size of the packet is less than the current size, keep the
     * packet at it's current size. If the new packet size is too big, throw a
     * packet exception. If the size is within the max packet size, set the new
     * size of the packet.
     */
    if(newSize < mSize)
    {
        // Keep the current size, do nothing.
    }
    else if(MAX_PACKET_SIZE < newSize || MAX_PACKET_SIZE < (mPosition + sz))
    {
        // The new size is too big, throw an exception.
        PACKET_EXCEPTION(String("Attempted to grow packet by %1 bytes; "
            "however, doing so would exceed MAX_PACKET_SIZE").Arg(sz), this);
    }
    else
    {
        // The new packet size is valid, set it.
        mSize = newSize;
    }
}

void Packet::WriteBlank(uint32_t count)
{
    // If we are writing 0 blank bytes, do nothing.
    if(0 == count)
    {
        return;
    }

    // Grow the packet by the number of bytes we intend to write, set the bytes
    // to 0, and advance the current position by that many bytes.
    GrowPacket(count);
    memset(mData + mPosition, 0, count);
    Skip(count);
}

void Packet::WriteArray(const std::vector<char>& data)
{
    // If we are writing an array of 0 bytes, do nothing.
    if(data.empty())
    {
        return;
    }

    // Grow the packet by the number of bytes in the array, copy the array into
    // the packet data at the current position, and advance the current
    // position by the number of bytes in the array.
    GrowPacket((uint32_t)data.size());
    memcpy(mData + mPosition, &data[0], (uint32_t)data.size());
    Skip((uint32_t)data.size());
}

void Packet::WriteArray(const void *pData, uint32_t sz)
{
    // If we are writing an array of 0 bytes, do nothing.
    if(0 == sz)
    {
        return;
    }

    // Grow the packet by the number of bytes in the array, copy the array into
    // the packet data at the current position, and advance the current
    // position by the number of bytes in the array.
    GrowPacket(sz);
    memcpy(mData + mPosition, pData, sz);
    Skip(sz);
}

void Packet::WriteString(Convert::Encoding_t encoding, const String& str,
    bool nullTerminate)
{
    // Convert the string to the requested encoding and write it.
    WriteArray(Convert::ToEncoding(encoding, str, nullTerminate));
}

void Packet::WriteString32(Convert::Encoding_t encoding, const String& str,
    bool nullTerminate)
{
    // Convert the string to the requested encoding.
    std::vector<char> strd = Convert::ToEncoding(encoding, str, nullTerminate);

    // Write the size of the string data and the string.
    WriteU32((uint32_t)strd.size());
    WriteArray(strd);
}

void Packet::WriteString32Big(Convert::Encoding_t encoding, const String& str,
    bool nullTerminate)
{
    // Convert the string to the requested encoding.
    std::vector<char> strd = Convert::ToEncoding(encoding, str, nullTerminate);

    // Write the size of the string data and the string.
    WriteU32Big((uint32_t)strd.size());
    WriteArray(strd);
}

void Packet::WriteString32Little(Convert::Encoding_t encoding,
    const String& str, bool nullTerminate)
{
    // Convert the string to the requested encoding.
    std::vector<char> strd = Convert::ToEncoding(encoding, str, nullTerminate);

    // Write the size of the string data and the string.
    WriteU32Little((uint32_t)strd.size());
    WriteArray(strd);
}

void Packet::WriteString16(Convert::Encoding_t encoding, const String& str,
    bool nullTerminate)
{
    // Convert the string to the requested encoding.
    std::vector<char> strd = Convert::ToEncoding(encoding, str, nullTerminate);

    // Write the size of the string data and the string.
    WriteU16((uint16_t)strd.size());
    WriteArray(strd);
}

void Packet::WriteString16Big(Convert::Encoding_t encoding, const String& str,
    bool nullTerminate)
{
    // Convert the string to the requested encoding.
    std::vector<char> strd = Convert::ToEncoding(encoding, str, nullTerminate);

    // Write the size of the string data and the string.
    WriteU16Big((uint16_t)strd.size());
    WriteArray(strd);
}

void Packet::WriteString16Little(Convert::Encoding_t encoding,
    const String& str, bool nullTerminate)
{
    // Convert the string to the requested encoding.
    std::vector<char> strd = Convert::ToEncoding(encoding, str, nullTerminate);

    // Write the size of the string data and the string.
    WriteU16Little((uint16_t)strd.size());
    WriteArray(strd);
}

void Packet::WriteU8(uint8_t value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(1);
    memcpy(mData + mPosition, &value, 1);
    Skip(1);
}

void Packet::WriteS8(int8_t value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(1);
    memcpy(mData + mPosition, &value, 1);
    Skip(1);
}

void Packet::WriteU16(uint16_t value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(2);
    memcpy(mData + mPosition, &value, 2);
    Skip(2);
}

void Packet::WriteU16Big(uint16_t value)
{
    // Convert the value to big endian byte order and then write it.
    WriteU16(htobe16(value));
}

void Packet::WriteU16Little(uint16_t value)
{
    // Convert the value to little endian byte order and then write it.
    WriteU16(htole16(value));
}

void Packet::WriteS16(int16_t value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(2);
    memcpy(mData + mPosition, &value, 2);
    Skip(2);
}

void Packet::WriteS16Big(int16_t value)
{
    // Convert the value to big endian byte order and then write it.
    WriteS16((int16_t)htobe16(value));
}

void Packet::WriteS16Little(int16_t value)
{
    // Convert the value to little endian byte order and then write it.
    WriteS16((int16_t)htole16(value));
}

void Packet::WriteU32(uint32_t value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(4);
    memcpy(mData + mPosition, &value, 4);
    Skip(4);
}

void Packet::WriteU32Big(uint32_t value)
{
    // Convert the value to big endian byte order and then write it.
    WriteU32(htobe32(value));
}

void Packet::WriteU32Little(uint32_t value)
{
    // Convert the value to little endian byte order and then write it.
    WriteU32(htole32(value));
}

void Packet::WriteS32(int32_t value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(4);
    memcpy(mData + mPosition, &value, 4);
    Skip(4);
}

void Packet::WriteS32Big(int32_t value)
{
    // Convert the value to big endian byte order and then write it.
    WriteS32((int32_t)htobe32((uint32_t)value));
}

void Packet::WriteS32Little(int32_t value)
{
    // Convert the value to little endian byte order and then write it.
    WriteS32((int32_t)htole32(value));
}

void Packet::WriteU64(uint64_t value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(8);
    memcpy(mData + mPosition, &value, 8);
    Skip(8);
}

void Packet::WriteU64Big(uint64_t value)
{
    // Convert the value to big endian byte order and then write it.
    WriteU64(htobe64(value));
}

void Packet::WriteU64Little(uint64_t value)
{
    // Convert the value to little endian byte order and then write it.
    WriteU64(htole64(value));
}

void Packet::WriteS64(int64_t value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(8);
    memcpy(mData + mPosition, &value, 8);
    Skip(8);
}

void Packet::WriteS64Big(int64_t value)
{
    // Convert the value to big endian byte order and then write it.
    WriteS64((int64_t)htobe64((uint64_t)value));
}

void Packet::WriteS64Little(int64_t value)
{
    // Convert the value to little endian byte order and then write it.
    WriteS64((int64_t)htole64(value));
}

void Packet::WriteFloat(float value)
{
    // Grow the packet by the size of the value, copy the value into the packet
    // data, and advance the current packet position by the size of the value.
    GrowPacket(4);
    memcpy(mData + mPosition, &value, 4);
    Skip(4);
}

void Packet::Clear()
{
    // Reset the position and size of the packet.
    mPosition = 0;
    mSize = 0;

#ifdef COMP_HACK_DEBUG
    // Make sure the buffer is allocated before we fill it.
    Allocate();

    uint32_t deadbeef = 0xEFBEADDE;

    // Fill the buffer with "dead beef" so you can see what is and isn't data.
    for(uint32_t i = 0; i < MAX_PACKET_SIZE; i += 4)
    {
        memcpy(mData + i, &deadbeef, 4);
    }
#endif // COMP_HACK_DEBUG
}

void Packet::EraseRight()
{
    // Set the size of the packet to the current position removing anything
    // after the current position.
    mSize = mPosition;
}

char* Packet::Data() const
{
    // Return a pointer to the packet data.
    return reinterpret_cast<char*>(mData);
}

char* Packet::Direct(uint32_t sz)
{
    // Make sure the requested size is not bigger than the max packet size
    if(MAX_PACKET_SIZE < sz)
    {
        PACKET_EXCEPTION(String("Attempted to directly access the first %1 "
            "bytes of the packet; however, this size exceeds the "
            "MAX_PACKET_SIZE").Arg(sz), this);
    }

    // Make sure that the current position will not be past the new size of the
    // packet. If so, thrown an exception.
    if(mPosition > sz)
    {
        PACKET_EXCEPTION(String("Attempted to directly access the first %1 "
            "bytes of the packet; however, this size exceeds the "
            "size of the packet").Arg(sz), this);
    }

    // Set the new size of the packet.
    mSize = sz;

    // Return the pointer to the packet data.
    return reinterpret_cast<char*>(mData);
}

void Packet::Split(Packet& other, uint32_t sz)
{
    // If there is no data to split simply clear the other packet and return.
    if(0 == sz)
    {
        other.Clear();

        return;
    }

    // If the request would read more data than contained in the packet, throw
    // an exception.
    if((mPosition + sz) > mSize)
    {
        PACKET_EXCEPTION(String("Attempted to split %1 bytes from the packet; "
            "however, this size exceeds the size of the packet").Arg(sz), this);
    }

    // Clear the other packet, write the data into it, and reset the current
    // position to the beginning of the packet.
    other.Clear();
    other.ReadArray(mData + mPosition, sz);
    other.Rewind();
}

int32_t Packet::Decompress(int32_t sz)
{
    // If there is no data to decompress, do nothing.
    if(0 >= sz)
    {
        return 0;
    }

    // Make sure that the data we wish to decompress is contained inside the
    // remaining packet data. If not, throw an exception.
    if((mPosition + (uint32_t)sz) > mSize)
    {
        PACKET_EXCEPTION(String("Attempted to decompress %1 bytes from the "
            "packet; however, this size exceeds the size of the "
            "packet").Arg(sz), this);
    }

    // Allocate the compressed copy.
    uint8_t *pData = new uint8_t[sz];

    if(nullptr == pData)
    {
        return 0;
    }

    // Copy the data to decompress.
    memcpy(pData, mData + mPosition, (size_t)sz);

    // Update the size of the packet.
    mSize = mPosition;

    // Decompress the data
    int32_t written = Compress::Decompress(pData, mData + mPosition,
        sz, (int32_t)(MAX_PACKET_SIZE - mSize));

    // Update the size.
    mSize += (uint32_t)written;

    // Free the compressed copy.
    delete[] pData;

    return written;
}

int32_t Packet::Compress(int32_t sz)
{
    // If there is no data to compress, do nothing.
    if(0 == sz)
    {
        return 0;
    }

    // Make sure that the data we wish to compress is contained inside the
    // remaining packet data. If not, throw an exception.
    if((mPosition + (uint32_t)sz) > mSize)
    {
        PACKET_EXCEPTION(String("Attempted to compress %1 bytes from the "
            "packet; however, this size exceeds the size of the "
            "packet").Arg(sz), this);
    }

    // Allocate the compressed copy.
    uint8_t *pData = new uint8_t[sz];

    if(nullptr == pData)
    {
        return 0;
    }

    // Copy the data to compress.
    memcpy(pData, mData + mPosition, (size_t)sz);

    // Update the size.
    mSize = mPosition;

    // Compress the data
    int32_t written = Compress::Compress(pData, mData + mPosition,
        sz, (int32_t)(MAX_PACKET_SIZE - mSize));

    // Update the size.
    mSize += (uint32_t)written;

    // Free the uncompressed copy.
    delete[] pData;

    return written;
}

Packet& Packet::operator=(Packet&& other)
{
    mPosition = other.mPosition;
    mSize = other.mSize;
    mDataRef = other.mDataRef;
    mData = other.mData;

    other.mPosition = 0;
    other.mSize = 0;
    other.mDataRef.reset();
    other.mData = nullptr;

    // Ensure the packet is clear and the variables are set.
    other.Clear();

    return *this;
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<Packet>()
    {
        // Include the base class
        Using<ReadOnlyPacket>();

        // Base class must be bound first.
        Sqrat::DerivedClass<Packet, ReadOnlyPacket> binding(mVM, "Packet");
        binding
            .Func("WriteBlank", &Packet::WriteBlank)
            .Func("WriteU16Little", &Packet::WriteU16Little)
            .Func<void (Packet::*)(const std::vector<char>&)>(
                "WriteArray", &Packet::WriteArray)
            ; // Last call to binding

        Sqrat::RootTable(mVM).Bind("Packet", binding);

        return *this;
    }
}
