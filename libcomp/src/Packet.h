/**
 * @file libcomp/src/Packet.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parsing and creation class definition.
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

#ifndef LIBCOMP_SRC_PACKET_H
#define LIBCOMP_SRC_PACKET_H

#include "ReadOnlyPacket.h"

/// Standard C++11 Includes
#include <type_traits>

namespace libcomp
{

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
class Packet : public ReadOnlyPacket
{
public:
    /// This class needs to directly access data in the Packet class.
    friend class PacketException;

    /**
     * Initialize a blank Packet object without any data.
     */
    explicit Packet();

    /**
     * @brief Copy the packet data from another Packet object into this one.
     * @param other Packet object to copy the data from.
     */
    explicit Packet(const Packet& other);

    /**
     * @brief Move the packet data from another Packet object into this one.
     * @param other Packet object to move the data from.
     */
    explicit Packet(Packet&& other);

    /**
     * Initialize a Packet object with @em data.
     * @param data Buffer of packet data to initalize the object with.
     */
    explicit Packet(const std::vector<char>& data);

    /**
     * Initialize a Packet object with @em pBuffer of @em size bytes.
     * @param pData Buffer of packet data to initalize the object with.
     * @param sz Number of bytes in the data buffer.
     */
    explicit Packet(const void *pData, uint32_t sz);

    /**
     * Destructor to clean-up the object (free the data).
     */
    ~Packet();

    /**
     * Write @em count blank bytes of value 0 into the packet.
     * @param count Number of blank bytes to write.
     */
    void WriteBlank(uint32_t count);

    /**
     * Copy the desired buffer into the packet.
     * @param data Data to write into the packet.
     */
    void WriteArray(const std::vector<char>& data);

    /**
     * Copy the desired buffer into the packet.
     * @param pData Data to write into the packet.
     * @param sz Size (in bytes) of the data to write.
     */
    void WriteArray(const void *pData, uint32_t sz);

    /**
     * Write the string @em str into the packet encoded with @em encoding.
     * There is no size written before the string so using this function
     * without a null terminator only makes sense if the string is of fixed
     * size.
     * @param encoding String encoding to use. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param str String to write into the packet.
     * @param nullTerminate If true, the string will end with a null
     * terminator; otherwise, the string will not contain a null byte at
     * the end.
     * @sa ReadString
     * @sa WriteString16
     * @sa WriteString32
     */
    void WriteString(Convert::Encoding_t encoding, const String& str,
        bool nullTerminate = false);

    /**
     * Write the string @em str into the packet encoded with @em encoding. A
     * 16-bit unsigned integer (host byte order) is written before the string
     * to indicate the size of the string (including any null terminator).
     * @param encoding String encoding to use. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param str String to write into the packet.
     * @param nullTerminate If true, the string will end with a null
     * terminator; otherwise, the string will not contain a null byte
     * at the end.
     * @sa WriteString
     * @sa WriteString16Big
     * @sa WriteString16Little
     */
    void WriteString16(Convert::Encoding_t encoding, const String& str,
        bool nullTerminate = false);

    /**
     * Write the string @em str into the packet encoded with @em encoding. A
     * 16-bit unsigned integer (big endian byte order) is written before the
     * string to indicate the size of the string (including any null
     * terminator).
     * @param encoding String encoding to use. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param str String to write into the packet.
     * @param nullTerminate If true, the string will end with a null
     * terminator; otherwise, the string will not contain a null byte
     * at the end.
     * @sa WriteString
     * @sa WriteString16
     * @sa WriteString16Little
     */
    void WriteString16Big(Convert::Encoding_t encoding, const String& str,
        bool nullTerminate = false);

    /**
     * Write the string @em str into the packet encoded with @em encoding. A
     * 16-bit unsigned integer (little endian byte order) is written before the
     * string to indicate the size of the string (including any null
     * terminator).
     * @param encoding String encoding to use. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param str String to write into the packet.
     * @param nullTerminate If true, the string will end with a null
     * terminator; otherwise, the string will not contain a null byte
     * at the end.
     * @sa WriteString
     * @sa WriteString16
     * @sa WriteString16Big
     */
    void WriteString16Little(Convert::Encoding_t encoding, const String& str,
        bool nullTerminate = false);

    /**
     * Write the string @em str into the packet encoded with @em encoding. A
     * 32-bit unsigned integer (host byte order) is written before the string
     * to indicate the size of the string (including any null terminator).
     * @param encoding String encoding to use. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param str String to write into the packet.
     * @param nullTerminate If true, the string will end with a null
     * terminator; otherwise, the string will not contain a null byte
     * at the end.
     * @sa WriteString
     * @sa WriteString32Big
     * @sa WriteString32Little
     */
    void WriteString32(Convert::Encoding_t encoding, const String& str,
        bool nullTerminate = false);

    /**
     * Write the string @em str into the packet encoded with @em encoding. A
     * 32-bit unsigned integer (big endian byte order) is written before the
     * string to indicate the size of the string (including any null
     * terminator).
     * @param encoding String encoding to use. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param str String to write into the packet.
     * @param nullTerminate If true, the string will end with a null
     * terminator; otherwise, the string will not contain a null byte
     * at the end.
     * @sa WriteString
     * @sa WriteString32
     * @sa WriteString32Little
     */
    void WriteString32Big(Convert::Encoding_t encoding, const String& str,
        bool nullTerminate = false);

    /**
     * Write the string @em str into the packet encoded with @em encoding. A
     * 32-bit unsigned integer (little endian byte order) is written before the
     * string to indicate the size of the string (including any null
     * terminator).
     * @param encoding String encoding to use. Can be one of:
     * - ENCODING_UTF8 (Unicode)
     * - ENCODING_CP932 (Japanese)
     * - ENCODING_CP1252 (US English)
     * @param str String to write into the packet.
     * @param nullTerminate If true, the string will end with a null
     * terminator; otherwise, the string will not contain a null byte
     * at the end.
     * @sa WriteString
     * @sa WriteString32
     * @sa WriteString32Big
     */
    void WriteString32Little(Convert::Encoding_t encoding, const String& str,
        bool nullTerminate = false);

    /**
     * Write an unsigned 8-bit integer @em value to the packet.
     * @param value 8-bit unsigned integer to write.
     */
    void WriteU8(uint8_t value);

    /**
     * Write a signed 8-bit integer @em value to the packet.
     * @param value 8-bit signed integer to write.
     */
    void WriteS8(int8_t value);

    /**
     * Write an unsigned 16-bit integer @em value to the packet in host byte
     * order.
     * @param value 16-bit unsigned integer to write.
     * @sa WriteU16Big
     * @sa WriteU16Little
     */
    void WriteU16(uint16_t value);

    /**
     * Write an unsigned 16-bit integer @em value to the packet in big endian
     * byte order.
     * @param value 16-bit unsigned integer to write.
     * @sa WriteU16
     * @sa WriteU16Little
     */
    void WriteU16Big(uint16_t value);

    /**
     * Write an unsigned 16-bit integer @em value to the packet in little
     * endian byte order.
     * @param value 16-bit unsigned integer to write.
     * @sa WriteU16
     * @sa WriteU16Big
     */
    void WriteU16Little(uint16_t value);

    /**
     * Write a signed 16-bit integer @em value to the packet in host byte
     * order.
     * @param value 16-bit signed integer to write.
     * @sa WriteS16Big
     * @sa WriteS16Little
     */
    void WriteS16(int16_t value);

    /**
     * Write a signed 16-bit integer @em value to the packet in big endian byte
     * order.
     * @param value 16-bit signed integer to write.
     * @sa WriteS16
     * @sa WriteS16Little
     */
    void WriteS16Big(int16_t value);

    /**
     * Write a signed 16-bit integer @em value to the packet in little endian
     * byte order.
     * @param value 16-bit signed integer to write.
     * @sa WriteS16
     * @sa WriteS16Big
     */
    void WriteS16Little(int16_t value);

    /**
     * Write an unsigned 32-bit integer @em value to the packet in host byte
     * order.
     * @param value 32-bit unsigned integer to write.
     * @sa WriteU32Big
     * @sa WriteU32Little
     */
    void WriteU32(uint32_t value);

    /**
     * Write an unsigned 32-bit integer @em value to the packet in big endian
     * byte order.
     * @param value 32-bit unsigned integer to write.
     * @sa WriteU32
     * @sa WriteU32Little
     */
    void WriteU32Big(uint32_t value);

    /**
     * Write an unsigned 32-bit integer @em value to the packet in little
     * endian byte order.
     * @param value 32-bit unsigned integer to write.
     * @sa WriteU32
     * @sa WriteU32Big
     */
    void WriteU32Little(uint32_t value);

    /**
     * Write a signed 32-bit integer @em value to the packet in host byte
     * order.
     * @param value 32-bit signed integer to write.
     * @sa WriteS32Big
     * @sa WriteS32Little
     */
    void WriteS32(int32_t value);

    /**
     * Write a signed 32-bit integer @em value to the packet in big endian
     * byte order.
     * @param value 32-bit signed integer to write.
     * @sa WriteS32
     * @sa WriteS32Little
     */
    void WriteS32Big(int32_t value);

    /**
     * Write a signed 32-bit integer @em value to the packet in little endian
     * byte order.
     * @param value 32-bit signed integer to write.
     * @sa WriteS32
     * @sa WriteS32Big
     */
    void WriteS32Little(int32_t value);

    /**
     * Write an unsigned 64-bit integer @em value to the packet in host byte
     * order.
     * @param value 64-bit unsigned integer to write.
     * @sa WriteU64Big
     * @sa WriteU64Little
     */
    void WriteU64(uint64_t value);

    /**
     * Write an unsigned 64-bit integer @em value to the packet in big endian
     * byte order.
     * @param value 64-bit unsigned integer to write.
     * @sa WriteU64
     * @sa WriteU64Little
     */
    void WriteU64Big(uint64_t value);

    /**
     * Write an unsigned 64-bit integer @em value to the packet in little
     * endian byte order.
     * @param value 64-bit unsigned integer to write.
     * @sa WriteU64
     * @sa WriteU64Big
     */
    void WriteU64Little(uint64_t value);

    /**
     * Write a signed 64-bit integer @em value to the packet in host byte
     * order.
     * @param value 64-bit signed integer to write.
     * @sa WriteS64Big
     * @sa WriteS64Little
     */
    void WriteS64(int64_t value);

    /**
     * Write a signed 64-bit integer @em value to the packet in big endian byte
     * order.
     * @param value 64-bit signed integer to write.
     * @sa WriteS64
     * @sa WriteS64Little
     */
    void WriteS64Big(int64_t value);

    /**
     * Write a signed 64-bit integer @em value to the packet in little endian
     * byte order.
     * @param value 64-bit signed integer to write.
     * @sa WriteS64
     * @sa WriteS64Big
     */
    void WriteS64Little(int64_t value);

    /**
     * Write a 32-bit floating point @em value to the packet.
     * @param value 32-bit floating point value to write.
     */
    void WriteFloat(float value);

    /**
     * Write the enumeration @em value to the packet as an unsigned 16-bit
     * integer in little endian byte order.
     * @param Enumeration to write into the packet.
     * @sa WriteU16Little
     */
    template<typename T>
    inline void WritePacketCode(T value)
    {
        WriteU16Little(static_cast<typename std::underlying_type<T>::type>(
            value));
    }

#ifdef COMP_HACK_PACKETEXT

    /**
     * Write a 32-bit integer @em value that represents the current game ticks.
     * This is a packet extension that calls Server::InternalToGameTicks()
     * to convert the internal time of the server to the on-the-wire time
     * format the client expects. You can call this method by hand and pass the
     * value to @ref WriteU32Little instead; this method is for convenience.
     * Since this method requires access to Server class, this extension
     * only works from within the channel server project.
     * @param value 32-bit floating point value to write.
     * @param value Internal server time to convert and write into the packet.
     */
    void WriteGameTicks(uint64_t value);

#endif // COMP_HACK_PACKETEXT

    /**
     * Erase all data after the current position effectively making the packet
     * size equal to the current position.
     */
    void EraseRight();

    /**
     * Empty the packet. This will set the size of the packet to zero and
     * rewind the current position back to the beginning of the packet.
     */
    void Clear();

    /**
     * Write @em sz bytes of packet data from the current position into the
     * packet @em other. If the number of bytes is less than @ref left(), a
     * PacketException will be thrown. If the other packet can't write @em sz
     * bytes, a PacketException will be thrown. The other packet will be
     * cleared and the current position will be reset to the beginning of the
     * packet.
     * @param other Packet to write the data into.
     * @param sz Number of bytes to copy into the other packet.
     */
    void Split(Packet& other, uint32_t sz);

    /**
     * Return direct access to the underlying packet buffer. Avoid using this
     * function as modifying or deleting the buffer can cause bugs. With
     * direct access, you must do your own bounds checking. An alternative
     * is to use @ref seek() or @ref rewind() and then call @ref readArray()
     * to copy the data out of the packet.
     * @returns Pointer to the packet data.
     */
    char* Data() const;

    /**
     * Return direct access to the underlying packet buffer and set the packet
     * size to @em sz bytes. Avoid using this function as modifying or deleting
     * the buffer can cause bugs. With direct access, you must do your own
     * bounds checking. An alternative is to use @ref Seek() or @ref Rewind()
     * and then call @ref ReadArray() or @ref WriteArray() to copy the data out
     * of or into the packet.
     * @param sz Number of bytes to set the packet size to.
     * @returns Pointer to the packet data.
     */
    char* Direct(uint32_t sz);

    /**
     * %Decompress from the cursor position @em sz bytes. After the
     * decompression the current position will remain the same.
     * @param sz Number of bytes to decompress.
     * @returns The uncompressed size or 0 if the decompression failed.
     */
    int32_t Decompress(int32_t sz);

    /**
     * %Compress from the cursor position @em sz bytes. After the compression
     * the current position will remain the same.
     * @param sz Number of bytes to compress.
     * @returns The compressed size or 0 if the compression failed.
     */
    int32_t Compress(int32_t sz);

    /**
     * @brief Move the packet data from another Packet object into this one.
     * @param other Packet object to move the data from.
     */
    Packet& operator=(Packet&& other);

private:
    /**
     * Add @em count bytes to the size of the packet. If this exceeds the
     * maximum size of the packet, a PacketException will be thrown.
     * @param count Number of bytes to add to the packet.
     */
    void GrowPacket(uint32_t count);
};

} // namespace libcomp

#endif // LIBCOMP_SRC_PACKET_H
