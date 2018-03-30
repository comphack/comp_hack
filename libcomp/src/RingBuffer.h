/**
 * @file libcomp/src/RingBuffer.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Thread-safe ring buffer.
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

#ifndef LIBCOMP_SRC_RINGBUFFER_H
#define LIBCOMP_SRC_RINGBUFFER_H

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif // WIN32

#include <stdint.h>
#include <string>

namespace libcomp
{

/**
* @brief Single producer, single consumer, lock free, no wait ring buffer.
*
* The capacity of the ring buffer may be adjusted to a multiple of the system
* page size that is also a power of two. The reported/effective capacity of
* the ring buffer will be 1 less than the true capacity of the internal
* buffer. The capacity of the ring buffer will never be less than the
* requested capacity.
*
* Read and writes should be performed on the provided buffer without any
* wrap-around logic. This is handled internally with memory mapping.
*
* This ring buffer is thread safe if the following requirements are met:
* - A maximum of two threads access the ring buffer.
* - If two threads are used, one thread performs write operations (producer)
*   and the other thread performs read operations (consumer).
* - A thread must never change roles (producer/consumer).
* - @ref BeginRead must always be followed by @ref EndRead. No other calls to
*   the ring buffer should be made by the same thread in-between these calls.
* - @ref BeginWrite must always be followed by @ref EndWrite. No other calls to
*   the ring buffer should be made by the same thread in-between these calls.
* - The consumer should never read more bytes from the given buffer than
*   returned by @ref BeginRead.
* - The call to @ref EndRead should pass the size updated by @ref BeginRead.
* - The producer should never write more bytes into the given buffer than
*   returned by @ref BeginWrite.
* - The call to @ref EndWrite should pass the size updated by @ref BeginWrite.
* - Neither thread should use the ring buffer until the constructor has
*   returned and all threads should stop using the ring buffer before the
*   destructor is called. The ring buffer may be constructed and destructed
*   by any thread as long as this condition is met.
*/
class RingBuffer
{
public:
    /**
    * @brief Exception generated from the ring buffer during creation.
    */
    class Exception
    {
    public:
        /**
        * @brief Create a ring buffer exception.
        * @param message Error message for this exception.
        */
        Exception(const std::string& message);

        /**
        * @brief Get the exception message.
        * @returns The exception message.
        */
        std::string Message() const;

    private:
        /// The exception message.
        std::string mMessage;
    };

    /**
    * @brief Create a ring buffer with a minimum capacity.
    * @param capacity The minimum capacity of the ring buffer +1.
    * @note The capacity reported by the ring buffer may not be the same as
    *   the provided minimum capacity.
    */
    RingBuffer(int32_t capacity);

    /**
    * @brief Free the ring buffer.
    * @note No thread should be using the ring buffer when this is called.
    */
    ~RingBuffer();

    /**
    * @brief Number of bytes that may be written into the ring buffer.
    * @returns Number of bytes that may be written into the ring buffer.
    */
    int32_t Free() const;

    /**
    * @brief Number of bytes that may be read from the ring buffer.
    * @returns Number of bytes that may be read from the ring buffer.
    */
    int32_t Available() const;

    /**
    * @brief Capacity of the ring buffer (in bytes).
    * @returns Capacity of the ring buffer (in bytes).
    * @note This will either be 1 less than the capacity passed into the
    *   constructor or a larger value if the ring buffer was increased to
    *   meet a page size and/or power of 2 requirement.
    */
    int32_t Capacity() const;

    /**
    * @brief Start a read on the ring buffer.
    * @param size Requested read size that is updated with the number of bytes
    *   that may be read after the call returns.
    * @returns nullptr if the queue is empty or a pointer to the data to read.
    * @note You must re-check size for the maximum number of bytes to read
    *   from the returned buffer.
    */
    const void* BeginRead(int32_t& size) const;

    /**
    * @brief Finish a read on the ring buffer.
    * @param size Number of bytes that were read. This will be updated with
    *   the number of bytes that were removed from the buffer. This value
    *   should not change unless the ring buffer is used wrong.
    * @returns Number of bytes available after the read operation.
    */
    int32_t EndRead(int32_t& size);

    /**
    * @brief Read bytes from the ring buffer.
    * @param pDestination Pointer to the buffer to store the data in.
    * @param size Number of bytes to read from the ring buffer.
    * @returns Number of bytes read from the ring buffer.
    */
    int32_t Read(void *pDestination, int32_t size);

    /**
    * @brief Start a write on the ring buffer.
    * @param size Requested write size that is updated with the number of bytes
    *   that may be written after the call returns.
    * @returns nullptr if the queue is full or a pointer to the buffer to
    *   write into.
    * @note You must re-check size for the maximum number of bytes to write
    *   into the returned buffer.
    */
    void* BeginWrite(int32_t& size) const;

    /**
    * @brief Finish a write on the ring buffer.
    * @param size Number of bytes that were written. This will be updated with
    *   the number of bytes that were added into the buffer. This value
    *   should not change unless the ring buffer is used wrong.
    * @returns Number of bytes free after the write operation.
    */
    int32_t EndWrite(int32_t& size);

    /**
    * @brief Write bytes into the ring buffer.
    * @param pSource Pointer to the buffer to read the data from.
    * @param size Number of bytes to write into the ring buffer.
    * @returns Number of bytes written into the ring buffer.
    */
    int32_t Write(const void *pSource, int32_t size);

private:
    /**
    * @brief Determine if a number is a positive power of two.
    * @returns true if a number is a positive power of two; otherwise, false.
    */
    static bool IsMultipleOfTwo(int32_t value);

    /**
    * @brief Determine the number of bytes in a system page.
    * @returns Number of bytes in a system page.
    */
    static int32_t GetPageSize();

    /**
    * @brief Create a memory map exception to be thrown by the constructor.
    * @returns The exception to be thrown.
    */
    static Exception MemoryMapException();

    /// Pointer to the base of the memory mapped ring buffer data.
    int8_t *mBuffer;

    /// True capacity of the buffer.
    int32_t mCapacity;

    /// Mask used to wrap the read and write index around.
    int32_t mCapacityMask;

    /// Index into the buffer for the next read operation.
    int32_t mReadIndex;

    /// Index into the buffer for the next write operation.
    int32_t mWriteIndex;

#if defined(_WIN32) || defined(_WIN64)
    /// Windows handle to the memory mapped file.
    HANDLE mMapFile;
#endif // WIN32
};

} // namespace libcomp

#endif // LIBCOMP_SRC_RINGBUFFER_H
