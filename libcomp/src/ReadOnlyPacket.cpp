/**
 * @file libcomp/src/ReadOnlyPacket.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Read only packet parsing and creation class implementation.
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

#include "ReadOnlyPacket.h"

#include "Endian.h"
#include "Log.h"
#include "PacketException.h"
#include "ScriptEngine.h"

#include <array>

#ifdef _WIN32
#include <windows.h>
#else // _WIN32
#include <sys/socket.h>
#endif // _WIN32

#include <cstring>
#include <cstdio>

using namespace libcomp;

ReadOnlyPacket::ReadOnlyPacket() : mPosition(0), mSize(0), mData(nullptr)
{
    // The max packet size should be evenly divisible by 4 bytes.
    static_assert(0 == (MAX_PACKET_SIZE % 4),
        "MAX_PACKET_SIZE not a multiple of 4");
}

ReadOnlyPacket::ReadOnlyPacket(uint32_t position, uint32_t size,
    uint8_t *pData, std::shared_ptr<PacketArray> dataRef) :
    mPosition(position), mSize(size), mData(pData), mDataRef(dataRef)
{
}

ReadOnlyPacket::ReadOnlyPacket(const ReadOnlyPacket& other) :
    mPosition(other.mPosition), mSize(other.mSize), mData(other.mData),
    mDataRef(other.mDataRef)
{
}

ReadOnlyPacket::ReadOnlyPacket(const ReadOnlyPacket& other,
    uint32_t start, uint32_t size) : mPosition(0), mSize(size),
    mData(&other.mData[start]), mDataRef(other.mDataRef)
{
    if((start + size) > other.mSize)
    {
        PACKET_EXCEPTION("Attempted to copy part of a packet that does "
            "not exist.", this);
    }
}

ReadOnlyPacket::ReadOnlyPacket(Packet&& other) :
    mPosition(other.mPosition), mSize(other.mSize), mData(other.mData),
    mDataRef(other.mDataRef)
{
    other.mPosition = 0;
    other.mSize = 0;
    other.mDataRef.reset();
    other.mData = nullptr;

    // Ensure the ReadOnlypacket is clear and the variables are set.
    other.Clear();
}

ReadOnlyPacket::~ReadOnlyPacket()
{
}

void ReadOnlyPacket::Allocate()
{
    // Ensure the packet data buffer is allocated.
    if(nullptr == mData)
    {
        mDataRef = std::shared_ptr<PacketArray>(new PacketArray);
        mData = mDataRef.get()->data();
    }
}

void ReadOnlyPacket::Seek(uint32_t pos)
{
    // If the position is past the max ReadOnlypacket size, thrown an exception.
    if(MAX_PACKET_SIZE < pos)
    {
        PACKET_EXCEPTION("Attempted to seek past the MAX_PACKET_SIZE", this);
    }

    // Set the new position.
    mPosition = pos;
}

void ReadOnlyPacket::Skip(uint32_t sz)
{
    // If we are skipping 0 bytes, do nothing.
    if(0 == sz)
    {
        return;
    }

    // If advancing the position by sz bytes would place the current position
    // past the max ReadOnlypacket size, thrown an exception.
    if(MAX_PACKET_SIZE < (mPosition + sz))
    {
        PACKET_EXCEPTION(String("Attempted to skip %1 bytes in the ReadOnlypacket; "
            "however, doing so would exceed MAX_PACKET_SIZE").Arg(sz), this);
    }

    // Advance the current position by sz bytes.
    mPosition += sz;
}

void ReadOnlyPacket::Rewind()
{
    // Reset the current position to the beginning of the ReadOnlypacket.
    mPosition = 0;
}

void ReadOnlyPacket::Rewind(uint32_t bytes)
{
    // If we are rewinding 0 bytes, do nothing.
    if(bytes == 0)
    {
        return;
    }

    // If rewinding more bytes than the current position, thrown an exception.
    if(bytes > mPosition)
    {
        PACKET_EXCEPTION(String("Attempted to rewind %1 bytes in the ReadOnlypacket; "
            "however, doing so would rewind past the beginning of the "
            "ReadOnlypacket").Arg(bytes), this);
    }

    // Rewind the current position by bytes.
    mPosition -= bytes;
}

std::vector<char> ReadOnlyPacket::ReadArray(uint32_t sz)
{
    std::vector<char> data;

    // If the number of bytes to read is 0, do nothing.
    if(0 == sz)
    {
        return data;
    }

    // If there are more bytes to read than bytes left in the ReadOnlypacket data,
    // thrown an exception.
    if((mPosition + sz) > mSize)
    {
        PACKET_EXCEPTION(String("Attempted to read an array of %1 bytes; "
            "however, doing so would read more data then is remaining "
            "in the ReadOnlypacket").Arg(sz), this);
    }

    // Copy the ReadOnlypacket data into the array and advance the current position
    // past the array that was just read.
    data.insert(data.begin(), reinterpret_cast<char*>(mData + mPosition),
        reinterpret_cast<char*>(mData + mPosition + sz));
    Skip(sz);

    return data;
}

void ReadOnlyPacket::ReadArray(void *buffer, uint32_t sz)
{
    // If the number of bytes to read is 0, do nothing.
    if(0 == sz)
    {
        return;
    }

    // If there are more bytes to read than bytes left in the ReadOnlypacket data,
    // thrown an exception.
    if((mPosition + sz) > mSize)
    {
        PACKET_EXCEPTION(String("Attempted to read an array of %1 bytes; "
            "however, doing so would read more data then is remaining "
            "in the ReadOnlypacket").Arg(sz), this);
    }

    // Copy the ReadOnlypacket data into the array and advance the current position
    // past the array that was just read.
    memcpy(buffer, mData + mPosition, sz);
    Skip(sz);
}

String ReadOnlyPacket::ReadString(Convert::Encoding_t encoding)
{
    uint32_t sz = 0;

    // Search for the null terminator and calculate the size of the string.
    while(Left() > 0 && ReadU8() != 0)
    {
        sz++;
    }

    // Return the current position to the beginning of the string.
    Rewind(++sz);

    // Read the string into the buffer.
    std::vector<char> buffer = ReadArray(sz);

    // Convert the string to the requested encoding.
    return Convert::FromEncoding(encoding, buffer);
}

String ReadOnlyPacket::ReadString16(Convert::Encoding_t encoding,
    bool trimNull)
{
    // Read the size of the string.
    uint16_t sz = ReadU16();

    // Read the string into the buffer.
    std::vector<char> buffer = ReadArray(sz);
    if(trimNull && buffer.size() > 0 && buffer.back() == 0)
    {
        buffer.pop_back();
    }

    // Convert the string to the requested encoding.
    return Convert::FromEncoding(encoding, buffer);
}

String ReadOnlyPacket::ReadString16Big(Convert::Encoding_t encoding,
    bool trimNull)
{
    // Read the size of the string.
    uint16_t sz = ReadU16Big();

    // Read the string into the buffer.
    std::vector<char> buffer = ReadArray(sz);
    if(trimNull && buffer.size() > 0 && buffer.back() == 0)
    {
        buffer.pop_back();
    }

    // Convert the string to the requested encoding.
    return Convert::FromEncoding(encoding, buffer);
}

String ReadOnlyPacket::ReadString16Little(Convert::Encoding_t encoding,
    bool trimNull)
{
    // Read the size of the string.
    uint16_t sz = ReadU16Little();

    // Read the string into the buffer.
    std::vector<char> buffer = ReadArray(sz);
    if(trimNull && buffer.size() > 0 && buffer.back() == 0)
    {
        buffer.pop_back();
    }

    // Convert the string to the requested encoding.
    return Convert::FromEncoding(encoding, buffer);
}

String ReadOnlyPacket::ReadString32(Convert::Encoding_t encoding,
    bool trimNull)
{
    // Read the size of the string.
    uint32_t sz = ReadU32();

    // Read the string into the buffer.
    std::vector<char> buffer = ReadArray(sz);
    if(trimNull && buffer.size() > 0 && buffer.back() == 0)
    {
        buffer.pop_back();
    }

    // Convert the string to the requested encoding.
    return Convert::FromEncoding(encoding, buffer);
}

String ReadOnlyPacket::ReadString32Big(Convert::Encoding_t encoding,
    bool trimNull)
{
    // Read the size of the string.
    uint32_t sz = ReadU32Big();

    // Read the string into the buffer.
    std::vector<char> buffer = ReadArray(sz);
    if(trimNull && buffer.size() > 0 && buffer.back() == 0)
    {
        buffer.pop_back();
    }

    // Convert the string to the requested encoding.
    return Convert::FromEncoding(encoding, buffer);
}

String ReadOnlyPacket::ReadString32Little(Convert::Encoding_t encoding,
    bool trimNull)
{
    // Read the size of the string.
    uint32_t sz = ReadU32Little();

    // Read the string into the buffer.
    std::vector<char> buffer = ReadArray(sz);
    if(trimNull && buffer.size() > 0 && buffer.back() == 0)
    {
        buffer.pop_back();
    }

    // Convert the string to the requested encoding.
    return Convert::FromEncoding(encoding, buffer);
}

uint8_t ReadOnlyPacket::PeekU8()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 1) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 1 byte from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return.
    uint8_t value;
    memcpy(&value, mData + mPosition, 1);

    return value;
}

int8_t ReadOnlyPacket::PeekS8()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 1) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 1 byte from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return.
    int8_t value;
    memcpy(&value, mData + mPosition, 1);

    return value;
}

uint16_t ReadOnlyPacket::PeekU16()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 2) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 2 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return.
    uint16_t value;
    memcpy(&value, mData + mPosition, 2);

    return value;
}

int16_t ReadOnlyPacket::PeekS16()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 2) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 2 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return.
    int16_t value;
    memcpy(&value, mData + mPosition, 2);

    return value;
}

uint16_t ReadOnlyPacket::PeekU16Big()
{
    // Return the value converted from big endian byte order.
    return be16toh(PeekU16());
}

int16_t ReadOnlyPacket::PeekS16Big()
{
    // Return the value converted from big endian byte order.
    return (int16_t)be16toh(PeekS16());
}

uint16_t ReadOnlyPacket::PeekU16Little()
{
    // Return the value converted from little endian byte order.
    return le16toh(PeekU16());
}

int16_t ReadOnlyPacket::PeekS16Little()
{
    // Return the value converted from little endian byte order.
    return (int16_t)le16toh(PeekS16());
}

uint32_t ReadOnlyPacket::PeekU32()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 4) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 4 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return.
    uint32_t value;
    memcpy(&value, mData + mPosition, 4);

    return value;
}

int32_t ReadOnlyPacket::PeekS32()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 4) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 4 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return.
    int32_t value;
    memcpy(&value, mData + mPosition, 4);

    return value;
}

uint32_t ReadOnlyPacket::PeekU32Big()
{
    // Return the value converted from big endian byte order.
    return be32toh(PeekU32());
}

int32_t ReadOnlyPacket::PeekS32Big()
{
    // Return the value converted from big endian byte order.
    return (int32_t)be32toh((uint32_t)PeekS32());
}

uint32_t ReadOnlyPacket::PeekU32Little()
{
    // Return the value converted from little endian byte order.
    return le32toh(PeekU32());
}

int32_t ReadOnlyPacket::PeekS32Little()
{
    // Return the value converted from little endian byte order.
    return (int32_t)le32toh(PeekS32());
}

uint8_t ReadOnlyPacket::ReadU8()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 1) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 1 byte from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    uint8_t value;
    memcpy(&value, mData + mPosition, 1);
    Skip(1);

    return value;
}

int8_t ReadOnlyPacket::ReadS8()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 1) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 1 byte from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    int8_t value;
    memcpy(&value, mData + mPosition, 1);
    Skip(1);

    return value;
}

uint16_t ReadOnlyPacket::ReadU16()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 2) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 2 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    uint16_t value;
    memcpy(&value, mData + mPosition, 2);
    Skip(2);

    return value;
}

uint16_t ReadOnlyPacket::ReadU16Big()
{
    // Return the value converted from big endian byte order.
    return be16toh(ReadU16());
}

uint16_t ReadOnlyPacket::ReadU16Little()
{
    // Return the value converted from little endian byte order.
    return le16toh(ReadU16());
}

int16_t ReadOnlyPacket::ReadS16()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 2) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 2 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    int16_t value;
    memcpy(&value, mData + mPosition, 2);
    Skip(2);

    return value;
}

int16_t ReadOnlyPacket::ReadS16Big()
{
    // Return the value converted from big endian byte order.
    return (int16_t)be16toh(ReadS16());
}

int16_t ReadOnlyPacket::ReadS16Little()
{
    // Return the value converted from little endian byte order.
    return (int16_t)le16toh(ReadS16());
}

uint32_t ReadOnlyPacket::ReadU32()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 4) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 4 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    uint32_t value;
    memcpy(&value, mData + mPosition, 4);
    Skip(4);

    return value;
}

uint32_t ReadOnlyPacket::ReadU32Big()
{
    // Return the value converted from big endian byte order.
    return be32toh(ReadU32());
}

uint32_t ReadOnlyPacket::ReadU32Little()
{
    // Return the value converted from little endian byte order.
    return le32toh(ReadU32());
}

int32_t ReadOnlyPacket::ReadS32()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 4) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 4 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    int32_t value;
    memcpy(&value, mData + mPosition, 4);
    Skip(4);

    return value;
}

int32_t ReadOnlyPacket::ReadS32Big()
{
    // Return the value converted from big endian byte order.
    return (int32_t)be32toh((uint32_t)ReadS32());
}

int32_t ReadOnlyPacket::ReadS32Little()
{
    // Return the value converted from little endian byte order.
    return (int32_t)le32toh(ReadS32());
}

uint64_t ReadOnlyPacket::ReadU64()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 8) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 8 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    uint64_t value;
    memcpy(&value, mData + mPosition, 8);
    Skip(8);

    return value;
}

uint64_t ReadOnlyPacket::ReadU64Big()
{
    // Return the value converted from big endian byte order.
    return be64toh(ReadU64());
}

uint64_t ReadOnlyPacket::ReadU64Little()
{
    // Return the value converted from little endian byte order.
    return le64toh(ReadU64());
}

int64_t ReadOnlyPacket::ReadS64()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 8) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 8 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    int64_t value;
    memcpy(&value, mData + mPosition, 8);
    Skip(8);

    return value;
}

int64_t ReadOnlyPacket::ReadS64Big()
{
    // Return the value converted from big endian byte order.
    return (int64_t)be64toh((uint64_t)ReadS64());
}

int64_t ReadOnlyPacket::ReadS64Little()
{
    // Return the value converted from little endian byte order.
    return (int64_t)le64toh(ReadS64());
}

float ReadOnlyPacket::ReadFloat()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 4) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 4 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    float value;
    memcpy(&value, mData + mPosition, 4);
    Skip(4);

    return value;
}

double ReadOnlyPacket::ReadDouble()
{
    // Check if there is enough data left in the ReadOnlypacket to read the value; if
    // not, throw an exception.
    if((mPosition + 8) > mSize)
    {
        PACKET_EXCEPTION("Attempted to read 8 bytes from the ReadOnlypacket; however, "
            "this would read past the end of the ReadOnlypacket.", this);
    }

    // Copy the value into a variable to return and then advance the current
    // position by the size of the value.
    double value;
    memcpy(&value, mData + mPosition, 8);
    Skip(8);

    return value;
}

void ReadOnlyPacket::End()
{
    // Set the current position to the size of the ReadOnlypacket data.
    mPosition = mSize;
}

uint32_t ReadOnlyPacket::Tell() const
{
    // Return the current position.
    return mPosition;
}

uint32_t ReadOnlyPacket::Left() const
{
    // Calculate how much data is between the current position and the end of
    // the ReadOnlypacket data.
    return mSize - mPosition;
}

uint32_t ReadOnlyPacket::Free() const
{
    return MAX_PACKET_SIZE - mSize;
}

uint32_t ReadOnlyPacket::Size() const
{
    // Return the size of the data in the ReadOnlypacket.
    return mSize;
}

uint32_t ReadOnlyPacket::Length() const
{
    // Alias of size(); return the size of the data in the ReadOnlypacket.
    return mSize;
}

const char* ReadOnlyPacket::ConstData() const
{
    // Return a pointer to the ReadOnlypacket data.
    return reinterpret_cast<const char*>(mData);
}

void ReadOnlyPacket::HexDump() const
{
    // Generate the hex dump string and log it.
    LOG_INFO("==========================================="
        "==============================\n");
    LOG_INFO(Dump() + "\n");
    LOG_INFO("==========================================="
        "==============================\n");
}

String ReadOnlyPacket::Dump() const
{
    // Do not generate a hex dump if there is not data.
    if(0 == mSize)
    {
        return String();
    }

    uint32_t line = 0;

    std::list<String> final;

    // Loop for each line of the hex dump.
    while(line < mSize)
    {
        libcomp::String buffer;

        // Print line number (add it to the working buffer). If the current
        // position is on the first byte of this line, print the left side of
        // the position marker.
        buffer += libcomp::String(mPosition == line ? "%1 {" : "%1  ").Arg(
            line, 4, 16, '0');

        // Loop and print 8 bytes of the ReadOnlypacket. If the current byte is past
        // the end of the ReadOnlypacket, blank spaces will be written. If the current
        // position is the next byte, print the left side of the position
        // marker. If the current byte is the current position, print the byte
        // followed by the right size of the position marker. Also, place a
        // single space between each byte in the dump.
        for(uint32_t i = line; i < (line + 8); ++i)
        {
            if(i >= mSize)
            {
                buffer += mPosition == i ? "  }" : "   ";
            }
            else if(mPosition == i)
            {
                buffer += libcomp::String("%1}").Arg(mData[i], 2, 16, '0');
            }
            else if(mPosition == (i + 1) && (i + 1) != (line + 8))
            {
                buffer += libcomp::String("%1{").Arg(mData[i], 2, 16, '0');
            }
            else
            {
                buffer += libcomp::String("%1 ").Arg(mData[i], 2, 16, '0');
            }
        }

        // Place a gap between the two sets of 8 bytes. Also place the left
        // side of the current position marker if the current position is the
        // first byte in the next set of 8 bytes.
        buffer += mPosition == (line + 8) ? "{" : " ";

        // Loop and print 8 bytes of the ReadOnlypacket. If the current byte is past
        // the end of the ReadOnlypacket, blank spaces will be written. If the current
        // position is the next byte, print the left side of the position
        // marker. If the current byte is the current position, print the byte
        // followed by the right size of the position marker. Also, place a
        // single space between each byte in the dump.
        for(uint32_t i = (line + 8); i < (line + 16); ++i)
        {
            if(i >= mSize)
            {
                buffer += mPosition == i ? "  }" : "   ";
            }
            else if(mPosition == i)
            {
                buffer += libcomp::String("%1}").Arg(mData[i], 2, 16, '0');
            }
            else if(mPosition == (i + 1) && (i + 1) != (line + 16))
            {
                buffer += libcomp::String("%1{").Arg(mData[i], 2, 16, '0');
            }
            else
            {
                buffer += libcomp::String("%1 ").Arg(mData[i], 2, 16, '0');
            }
        }

        // Print a space between the hex dump and the ASCII representation.
        buffer += " ";

        // Print the ASCII character the the byte represents. Only printable
        // ASCII is included; all other characters are printed as a dot.
        for(uint32_t i = line; i < (line + 8) && i < mSize; ++i)
        {
            uint8_t val = mData[i];

            buffer += libcomp::String(1, (val >= 0x20 && val < 0x7f) ?
                static_cast<char>(val) : '.');
        }

        // Print a space between the two sets of 8 ASCII characters.
        buffer += " ";

        // Print the ASCII character the the byte represents. Only printable
        // ASCII is included; all other characters are printed as a dot.
        for(uint32_t i = (line + 8); i < (line + 16) && i < mSize; ++i)
        {
            uint8_t val = mData[i];

            buffer += libcomp::String(1, (val >= 0x20 && val < 0x7f) ?
                static_cast<char>(val) : '.');
        }

        // Advance the line by 16 bytes
        line += 16;

        // Append the generated line to the output string.
        final.push_back(buffer);
    }

    // If the cursor is at the end and there is a multiple of 16 bytes in the
    // packet, make another line for the cursor position.
    if(mPosition == mSize && 0 == (mSize % 16))
    {
        final.push_back(libcomp::String("%1 {  }").Arg(line, 4, 16, '0'));
    }

    // Join all the lines of the hex dump together and return the final string.
    return String::Join(final, "\n");
}

ReadOnlyPacket& ReadOnlyPacket::operator=(ReadOnlyPacket& other)
{
    mPosition = other.mPosition;
    mSize = other.mSize;
    mDataRef = other.mDataRef;
    mData = other.mData;

    return *this;
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<ReadOnlyPacket>()
    {
        if(!BindingExists("ReadOnlyPacket"))
        {
            Sqrat::Class<ReadOnlyPacket> binding(mVM, "ReadOnlyPacket");
            binding
                .Func("Size", &ReadOnlyPacket::Size)
                .Func<std::vector<char>(ReadOnlyPacket::*)(uint32_t)>(
                    "ReadArray", &Packet::ReadArray)
                .Overload<void (ReadOnlyPacket::*)()>(
                    "Rewind", &ReadOnlyPacket::Rewind)
                .Overload<void (ReadOnlyPacket::*)(uint32_t)>(
                    "Rewind", &ReadOnlyPacket::Rewind)
                .Func("HexDump", &Packet::HexDump)
                ; // Last call to binding

            Bind<ReadOnlyPacket>("ReadOnlyPacket", binding);
        }

        return *this;
    }
}
