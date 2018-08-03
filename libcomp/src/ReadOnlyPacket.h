/**
 * @file libcomp/src/ReadOnlyPacket.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Read only packet parsing and creation class definition.
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

#ifndef LIBCOMP_SRC_READONLYPACKET_H
#define LIBCOMP_SRC_READONLYPACKET_H

#include "Constants.h"
#include "Convert.h"
#include "CString.h"

#include <memory>
#include <vector>

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _WIN32
#include <unistd.h>
#endif // _WIN32

namespace libcomp
{

// This inherited class has full write ability.
class Packet;

// This class is thrown when an error occurs in the Packet class.
class PacketException;

/**
 * Convenience class to read and write packet data. This class is designed to
 * make it easy to read and write packet data. Strings can be converted between
 * the internal Unicode format and Windows code pages. All integer numbers can
 * be written in host byte order, big endian, or little endian. The packet data
 * is usually read or written in sequantial order but you can use random access
 * as well. Note that data is not inserted into the middle of the packet, it
 * is instead overwritten with the new data. Here is some example code:
 * @code
 * Packet p;
 * p.WriteU16Little(0x1234); // Code for this packet.
 * p.WriteBlank(4); // Number of items (blank for now).
 *
 * // Initialize the item count to 0.
 * int count = 0;
 *
 * // Loop through and write each item to the packet.
 * while(HaveItems())
 * {
 *     count++;
 *     p.WriteU32Little(NextItem());
 * }
 *
 * // Return to where the item count should be, write the count, and return
 * // the current position back to the end of the packet.
 * p.Seek(2);
 * p.WriteU32Little(count);
 * p.End();
 * @endcode
 */
class ReadOnlyPacket
{
private:
    typedef std::array<uint8_t, MAX_PACKET_SIZE> PacketArray;

public:
    /// This class needs to directly access data in the Packet class.
    friend class PacketException;

    /**
     * Initialize a blank Packet object without any data.
     */
    explicit ReadOnlyPacket();

    /**
     * @brief Copy the packet data from another ReadOnlyPacket object.
     * @param other ReadOnlyPacket object to copy the data from.
     */
    explicit ReadOnlyPacket(const ReadOnlyPacket& other);

    /**
     * @brief Copy part of the packet data from another ReadOnlyPacket object.
     * @param other ReadOnlyPacket object to copy the data from.
     * @param start Number of bytes into the packet data.
     * @param size Size in bytes of the data to copy.
     */
    explicit ReadOnlyPacket(const ReadOnlyPacket& other,
        uint32_t start, uint32_t size);

    /**
     * @brief Move the packet data from a Packet object.
     * @param other Packet object to move the data from.
     */
    explicit ReadOnlyPacket(Packet&& other);

    /*
     * Never copy a writable Packet
    */
    explicit ReadOnlyPacket(const Packet& other) = delete;

    /**
     * Destructor to clean-up the object (free the data).
     */
    ~ReadOnlyPacket();

    /**
     * Read @em sz bytes of data from the packet. After reading the data,
     * the current position in the packet will advance by @em sz bytes.
     * @param sz Number of bytes to read from the packet into the buffer.
     * @returns Buffer the data was read into.
     */
    std::vector<char> ReadArray(uint32_t sz);

    /**
     * Read @em sz bytes of data from the packet into @em buffer. After
     * reading the data, the current position in the packet will advance by
     * @em sz bytes.
     * @param buffer Buffer to store the data that was read from the packet.
     * @param sz Number of bytes to read from the packet into the buffer.
     */
    void ReadArray(void *buffer, uint32_t sz);

    /**
     * Read the string @em str encoded with @em encoding from the packet. The
     * string should contain a null terminator.
     * @param encoding String encoding used. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @returns The string that was read.
     * @sa WriteString
     * @sa ReadString16
     * @sa ReadString32
     */
    String ReadString(Convert::Encoding_t encoding);

    /**
     * Read the string @em str encoded with @em encoding from the packet. The
     * size of the string is determined by a 16-bit unsigned integer in host
     * byte order that is read first. This size includes any null terminator.
     * @param encoding String encoding used. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param trimNull When true the null terminator will be removed
     * @returns The string that was read.
     * @sa WriteString16
     * @sa ReadString16Big
     * @sa ReadString16Little
     */
    String ReadString16(Convert::Encoding_t encoding, bool trimNull = false);

    /**
     * Read the string @em str encoded with @em encoding from the packet. The
     * size of the string is determined by a 16-bit unsigned integer in big
     * endian byte order that is read first. This size includes any null
     * terminator.
     * @param encoding String encoding used. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param trimNull When true the null terminator will be removed
     * @returns The string that was read.
     * @sa WriteString16
     * @sa ReadString16
     * @sa ReadString16Little
     */
    String ReadString16Big(Convert::Encoding_t encoding, bool trimNull = false);

    /**
     * Read the string @em str encoded with @em encoding from the packet. The
     * size of the string is determined by a 16-bit unsigned integer in little
     * endian byte order that is read first. This size includes any null
     * terminator.
     * @param encoding String encoding used. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param trimNull When true the null terminator will be removed
     * @returns The string that was read.
     * @sa WriteString16
     * @sa ReadString16
     * @sa ReadString16Big
     */
    String ReadString16Little(Convert::Encoding_t encoding, bool trimNull = false);

    /**
     * Read the string @em str encoded with @em encoding from the packet. The
     * size of the string is determined by a 32-bit unsigned integer in host
     * byte order that is read first. This size includes any null terminator.
     * @param encoding String encoding used. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param trimNull When true the null terminator will be removed
     * @returns The string that was read.
     * @sa WriteString32
     * @sa ReadString32Big
     * @sa ReadString32Little
     */
    String ReadString32(Convert::Encoding_t encoding, bool trimNull = false);

    /**
     * Read the string @em str encoded with @em encoding from the packet. The
     * size of the string is determined by a 32-bit unsigned integer in big
     * endian byte order that is read first. This size includes any null
     * terminator.
     * @param encoding String encoding used. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param trimNull When true the null terminator will be removed
     * @returns The string that was read.
     * @sa WriteString32
     * @sa ReadString32
     * @sa ReadString32Little
     */
    String ReadString32Big(Convert::Encoding_t encoding, bool trimNull = false);

    /**
     * Read the string @em str encoded with @em encoding from the packet. The
     * size of the string is determined by a 32-bit unsigned integer in little
     * endian byte order that is read first. This size includes any null
     * terminator.
     * @param encoding String encoding used. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param trimNull When true the null terminator will be removed
     * @returns The string that was read.
     * @sa WriteString32
     * @sa ReadString32
     * @sa ReadString32Big
     */
    String ReadString32Little(Convert::Encoding_t encoding, bool trimNull = false);

    /**
     * Read a 8-bit unsigned integer from the packet but do not advance the
     * current position in the packet. Useful for determining a length (e.g. of
     * a string) or version of the packet before actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadU8
     */
    uint8_t PeekU8();

    /**
     * Read a 8-bit signed integer from the packet but do not advance the
     * current position in the packet. Useful for determining a length (e.g. of
     * a string) or version of the packet before actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadS8
     */
    int8_t PeekS8();

    /**
     * Read a 16-bit unsigned integer in host byte order from the packet but do
     * not advance the current position in the packet. Useful for determining a
     * length (e.g. of a string) or version of the packet before actually doing
     * read code.
     * @returns Value at the current position in the packet.
     * @sa ReadU16
     * @sa PeekU16Big
     * @sa PeekU16Little
     */
    uint16_t PeekU16();

    /**
     * Read a 16-bit signed integer in host byte order from the packet but do
     * not advance the current position in the packet. Useful for determining a
     * length (e.g. of a string) or version of the packet before actually doing
     * read code.
     * @returns Value at the current position in the packet.
     * @sa ReadS16
     * @sa PeekS16Big
     * @sa PeekS16Little
     */
    int16_t PeekS16();

    /**
     * Read a 16-bit unsigned integer in big endian byte order from the packet
     * but do not advance the current position in the packet. Useful for
     * determining a length (e.g. of a string) or version of the packet before
     * actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadU16
     * @sa PeekU16
     * @sa PeekU16Little
     */
    uint16_t PeekU16Big();

    /**
     * Read a 16-bit signed integer in big endian byte order from the packet
     * but do not advance the current position in the packet. Useful for
     * determining a length (e.g. of a string) or version of the packet before
     * actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadS16
     * @sa PeekS16
     * @sa PeekS16Little
     */
    int16_t PeekS16Big();

    /**
     * Read a 16-bit unsigned integer in little endian byte order from the
     * packet but do not advance the current position in the packet. Useful for
     * determining a length (e.g. of a string) or version of the packet before
     * actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadU16
     * @sa PeekU16
     * @sa PeekU16Big
     */
    uint16_t PeekU16Little();

    /**
     * Read a 16-bit signed integer in little endian byte order from the
     * packet but do not advance the current position in the packet. Useful for
     * determining a length (e.g. of a string) or version of the packet before
     * actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadS16
     * @sa PeekS16
     * @sa PeekS16Big
     */
    int16_t PeekS16Little();

    /**
     * Read a 32-bit unsigned integer in host byte order from the packet but do
     * not advance the current position in the packet. Useful for determining a
     * length (e.g. of a string) or version of the packet before actually doing
     * read code.
     * @returns Value at the current position in the packet.
     * @sa ReadU32
     * @sa PeekU32Big
     * @sa PeekU32Little
     */
    uint32_t PeekU32();

    /**
     * Read a 32-bit signed integer in host byte order from the packet but do
     * not advance the current position in the packet. Useful for determining a
     * length (e.g. of a string) or version of the packet before actually doing
     * read code.
     * @returns Value at the current position in the packet.
     * @sa ReadS32
     * @sa PeekS32Big
     * @sa PeekS32Little
     */
    int32_t PeekS32();

    /**
     * Read a 32-bit unsigned integer in big endian byte order from the packet
     * but do not advance the current position in the packet. Useful for
     * determining a length (e.g. of a string) or version of the packet before
     * actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadU32
     * @sa PeekU32
     * @sa PeekU32Little
     */
    uint32_t PeekU32Big();

    /**
     * Read a 32-bit signed integer in big endian byte order from the packet
     * but do not advance the current position in the packet. Useful for
     * determining a length (e.g. of a string) or version of the packet before
     * actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadS32
     * @sa PeekS32
     * @sa PeekS32Little
     */
    int32_t PeekS32Big();

    /**
     * Read a 32-bit unsigned integer in little endian byte order from the
     * packet but do not advance the current position in the packet. Useful for
     * determining a length (e.g. of a string) or version of the packet before
     * actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadU32
     * @sa PeekU32
     * @sa PeekU32Big
     */
    uint32_t PeekU32Little();

    /**
     * Read a 32-bit signed integer in little endian byte order from the
     * packet but do not advance the current position in the packet. Useful for
     * determining a length (e.g. of a string) or version of the packet before
     * actually doing read code.
     * @returns Value at the current position in the packet.
     * @sa ReadS32
     * @sa PeekS32
     * @sa PeekS32Big
     */
    int32_t PeekS32Little();

    /**
     * Read a 8-bit unsigned integer from the packet and advance the current
     * position in the packet.
     * @returns Value at the current position in the packet.
     * @sa PeekU8
     * @sa WriteU8
     */
    uint8_t ReadU8();

    /**
     * Read a 8-bit signed integer from the packet and advance the current
     * position in the packet.
     * @returns Value at the current position in the packet.
     * @sa PeekS8
     * @sa WriteS8
     */
    int8_t ReadS8();

    /**
     * Read a 16-bit unsigned integer in host byte order from the packet and
     * advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa PeekU16
     * @sa WriteU16
     * @sa ReadU16Big
     * @sa ReadU16Little
     */
    uint16_t ReadU16();

    /**
     * Read a 16-bit unsigned integer in big endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa PeekU16Big
     * @sa WriteU16Big
     * @sa ReadU16
     * @sa ReadU16Little
     */
    uint16_t ReadU16Big();

    /**
     * Read a 16-bit unsigned integer in little endian byte order from the
     * packet and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa PeekU16Little
     * @sa WriteU16Little
     * @sa ReadU16
     * @sa ReadU16Big
     */
    uint16_t ReadU16Little();

    /**
     * Read a 16-bit signed integer in host byte order from the packet and
     * advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS16
     * @sa ReadS16Big
     * @sa ReadS16Little
     */
    int16_t ReadS16();

    /**
     * Read a 16-bit signed integer in big endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS16Big
     * @sa ReadS16
     * @sa ReadS16Little
     */
    int16_t ReadS16Big();

    /**
     * Read a 16-bit signed integer in little endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS16Little
     * @sa ReadS16
     * @sa ReadS16Big
     */
    int16_t ReadS16Little();

    /**
     * Read a 32-bit unsigned integer in host byte order from the packet and
     * advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa PeekU32
     * @sa WriteU32
     * @sa ReadU32Big
     * @sa ReadU32Little
     */
    uint32_t ReadU32();

    /**
     * Read a 32-bit unsigned integer in big endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa PeekU32Big
     * @sa WriteU32Big
     * @sa ReadU32
     * @sa ReadU32Little
     */
    uint32_t ReadU32Big();

    /**
     * Read a 32-bit unsigned integer in little endian byte order from the
     * packet and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa PeekU32Little
     * @sa WriteU32Little
     * @sa ReadU32
     * @sa ReadU32Big
     */
    uint32_t ReadU32Little();

    /**
     * Read a 32-bit signed integer in host byte order from the packet and
     * advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS32
     * @sa ReadS32Big
     * @sa ReadS32Little
     */
    int32_t ReadS32();

    /**
     * Read a 32-bit signed integer in big endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS32Big
     * @sa ReadS32
     * @sa ReadS32Little
     */
    int32_t ReadS32Big();

    /**
     * Read a 32-bit signed integer in little endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS32Little
     * @sa ReadS32
     * @sa ReadS32Big
     */
    int32_t ReadS32Little();

    /**
     * Read a 64-bit unsigned integer in host byte order from the packet and
     * advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteU64
     * @sa ReadU64Big
     * @sa ReadU64Little
     */
    uint64_t ReadU64();

    /**
     * Read a 64-bit unsigned integer in big endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteU64Big
     * @sa ReadU64
     * @sa ReadU64Little
     */
    uint64_t ReadU64Big();

    /**
     * Read a 64-bit unsigned integer in little endian byte order from the
     * packet and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteU64Little
     * @sa ReadU64
     * @sa ReadU64Big
     */
    uint64_t ReadU64Little();

    /**
     * Read a 64-bit signed integer in host byte order from the packet and
     * advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS64
     * @sa ReadS64Big
     * @sa ReadS64Little
     */
    int64_t ReadS64();

    /**
     * Read a 64-bit signed integer in big endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS64Big
     * @sa ReadS64
     * @sa ReadS64Little
     */
    int64_t ReadS64Big();

    /**
     * Read a 64-bit signed integer in little endian byte order from the packet
     * and advance the current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteS64Little
     * @sa ReadS64
     * @sa ReadS64Big
     */
    int64_t ReadS64Little();

    /**
     * Read a 32-bit floating point number from the packet and advance the
     * current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteFloat
     */
    float ReadFloat();

    /**
     * Write a 64-bit double precision number from the packet and advance the
     * current position in the packet.
     * @returns Value at the current position in the packet.
     * @sa WriteDouble
     */
    double ReadDouble();

    /**
     * Seek to @em pos bytes from the beginning of the packet. The position
     * specified must be within the range of the data in the packet; otherwise,
     * a @ref PacketException will be thrown. The valid range is 0 bytes to
     * @ref Size() bytes (inclusive).
     * @param pos Number of bytes from the start of the packet data.
     */
    void Seek(uint32_t pos);

    /**
     * Advance @em bytes from the current position. This is essentially a seek
     * from the current position instead of the beginning of the packet. The
     * valid range of this method is from 0 bytes to @ref Left() bytes
     * (inclusive). If the value is outside this range, a @ref PacketException
     * will be thrown.
     * @param bytes Number of bytes to advance the current position.
     */
    void Skip(uint32_t bytes);

    /**
     * Rewind the current position to the beginning of the packet.
     */
    void Rewind();

    /**
     * Rewind the current position by @em bytes. The range of this method
     * should be from 0 bytes to @ref Tell() bytes. If the value is outside of
     * this range, a @ref PacketException will be thrown.
     * @param bytes Number of bytes to rewind the current position.
     */
    void Rewind(uint32_t bytes);

    /**
     * Set the current position to the end of the packet. This will set
     * @ref Tell() equal to @ref Size().
     */
    void End();

    /**
     * Produce a hex dump of the packet in the log.
     */
    void HexDump() const;

    /**
     * Generate a string that contains a hex dump of the packet.
     * @returns String that contains the hex dump.
     */
    String Dump() const;

    /**
     * Return the current position in the packet.
     * @returns Current position in the packet.
     */
    uint32_t Tell() const;

    /**
     * Return the number of bytes left to be read. This is the number of bytes
     * between the current position and the size of the packet.
     * @returns Number of bytes left in the packet.
     */
    uint32_t Left() const;

    /**
     * Return the number of bytes that may be added to the end of the packet.
     * @returns Number of bytes that may be added to the end of the packet.
     */
    uint32_t Free() const;

    /**
     * Returns the number of bytes in the packet.
     * @returns Number of bytes in the packet.
     */
    uint32_t Size() const;

    /**
     * Returns the number of bytes in the packet.
     * @returns Number of bytes in the packet.
     */
    uint32_t Length() const;

    /**
     * Return direct access to the underlying packet buffer. Avoid using this
     * function as modifying or deleting the buffer can cause bugs. With
     * direct access, you must do your own bounds checking. An alternative
     * is to use @ref seek() or @ref rewind() and then call @ref readArray()
     * to copy the data out of the packet.
     * @returns Pointer to the packet data.
     */
    const char* ConstData() const;

    /**
     * @brief Ensure the packet data buffer is allocated.
     */
    void Allocate();

    /**
     * @brief Copy the packet data from another ReadOnlyPacket object.
     * @param other ReadOnlyPacket object to move the data from.
     */
    ReadOnlyPacket& operator=(ReadOnlyPacket& other);

    /**
     * @brief Move the packet data from another ReadOnlyPacket object.
     * @param other ReadOnlyPacket object to move the data from.
     */
    ReadOnlyPacket& operator=(ReadOnlyPacket&& other) = delete;

protected:
    /// Protected constructor for use by subclasses.
    explicit ReadOnlyPacket(uint32_t position, uint32_t size,
        uint8_t *pData, std::shared_ptr<PacketArray> dataRef);

    /// Current position in the packet.
    uint32_t mPosition;

    /// Size of the packet.
    uint32_t mSize;

    /// Pointer to the packet data.
    uint8_t *mData;

    /// Reference to the underlying buffer (which could be shared between
    /// read only packets).
    std::shared_ptr<PacketArray> mDataRef;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_READONLYPACKET_H
