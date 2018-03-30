/**
 * @file libcomp/src/RingBuffer.cpp
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

#include "RingBuffer.h"

#if !defined(_WIN32) && !defined(_WIN64)
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#endif // !WIN32

#ifdef _WIN32
#undef min
#endif // _WIN32

#include <cstring>
#include <algorithm>

using namespace libcomp;

RingBuffer::RingBuffer(int32_t capacity) : mBuffer(nullptr),
    mCapacity(0), mCapacityMask(0), mReadIndex(0), mWriteIndex(0)
{
    // Check if the capacity is over 1 byte.
    if(1 >= capacity)
    {
        throw RingBuffer::Exception("Capacity is not greater than one.");
    }

    // Get the page size to round the capacity with.
    int32_t pageSize = GetPageSize();

    // If the capacity is not a multiple of the page size, round it up.
    if(0 != (capacity % pageSize))
    {
        capacity += pageSize - (capacity % pageSize);
    }

    // Check that the capacity is a multiple of two.
    if(!IsMultipleOfTwo(capacity))
    {
        throw RingBuffer::Exception("Capacity is not a multiple of two.");
    }

    // Set the capacity and the wrap-around mask.
    mCapacity = capacity;
    mCapacityMask = capacity - 1;

#if defined(_WIN32) || defined(_WIN64)
    // Create the file mapping.
    mMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
        0, PAGE_READWRITE, 0, capacity * 2, 0);

    // Check if the file mapping was a success.
    if(INVALID_HANDLE_VALUE == mMapFile)
    {
        throw MemoryMapException();
    }

    mBuffer = reinterpret_cast<int8_t*>(MapViewOfFile(mMapFile,
        FILE_MAP_ALL_ACCESS, 0, 0, capacity * 2));

    if(nullptr == mBuffer)
    {
        CloseHandle(mMapFile);

        throw MemoryMapException();
    }
    else
    {
        UnmapViewOfFile(mBuffer);
    }

    // Get a pointer to the mapped file.
    mBuffer = reinterpret_cast<int8_t*>(MapViewOfFileEx(mMapFile,
        FILE_MAP_ALL_ACCESS, 0, 0, capacity, mBuffer));

    // Check the pointer to the mapped file.
    if(nullptr == mBuffer)
    {
        CloseHandle(mMapFile);

        throw MemoryMapException();
    }

    int8_t *pBuffer2 = &mBuffer[capacity];

    // Get a second pointer to the mapped file directly after the first.
    int8_t *pSecondBuffer = reinterpret_cast<int8_t*>(MapViewOfFileEx(mMapFile,
        FILE_MAP_ALL_ACCESS, 0, 0, capacity, pBuffer2));

    // Check the second pointer to the mapped file is correct.
    if(nullptr == pSecondBuffer || pSecondBuffer != &mBuffer[capacity])
    {
        UnmapViewOfFile(mBuffer);
        CloseHandle(mMapFile);

        mBuffer = nullptr;

        throw MemoryMapException();
    }
#else // !WIN32

    // Path to the temp file for the ring buffer.
    char szPath[] = "/dev/shm/ring-buffer-XXXXXX";

    // Create the temp file.
    int fd = mkstemp(szPath);

    // If the temp file was not created, throw an exception and stop.
    if(0 > fd)
    {
        throw MemoryMapException();
    }

    // Delete the file now that we have the descriptor to the memory.
    if(0 != unlink(szPath))
    {
        throw MemoryMapException();
    }

    // Adjust the size of the file to the capacity of the ring buffer.
    if(0 != ftruncate(fd, capacity))
    {
        throw MemoryMapException();
    }

    // Memory map the file once to reserve the memory for both mappings.
    mBuffer = reinterpret_cast<int8_t*>(mmap(0, (size_t)capacity * 2,
        PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));

    // Check if the memory was mapped.
    if(MAP_FAILED == mBuffer)
    {
        mBuffer = nullptr;

        throw MemoryMapException();
    }

    // Map the first copy of the buffer.
    int8_t *pFirstBuffer = reinterpret_cast<int8_t*>(mmap(mBuffer,
        (size_t)capacity, PROT_READ | PROT_WRITE, MAP_FIXED |
        MAP_SHARED, fd, 0));

    // Check the mapping of the first buffer.
    if(mBuffer != pFirstBuffer)
    {
        munmap(mBuffer, (size_t)capacity * 2);
        mBuffer = nullptr;

        throw MemoryMapException();
    }

    // Map the second copy of the buffer.
    int8_t *pSecondBuffer = reinterpret_cast<int8_t*>(mmap(&mBuffer[capacity],
        (size_t)capacity, PROT_READ | PROT_WRITE, MAP_FIXED |
        MAP_SHARED, fd, 0));

    // Check the mapping of the second buffer.
    if(&mBuffer[capacity] != pSecondBuffer)
    {
        munmap(mBuffer, (size_t)capacity * 2);
        mBuffer = nullptr;

        throw MemoryMapException();
    }

    // Close the file descriptor as it is no longer needed.
    if(0 != close(fd))
    {
        munmap(mBuffer, (size_t)capacity * 2);
        mBuffer = nullptr;

        throw MemoryMapException();
    }
#endif // WIN32
}

RingBuffer::~RingBuffer()
{
    // Nothing to do if there is no buffer.
    if(nullptr == mBuffer)
    {
        return;
    }

    // Cleanup the memory mapping.
#if defined(_WIN32) || defined(_WIN64)
    UnmapViewOfFile(mBuffer + mCapacity);
    UnmapViewOfFile(mBuffer);
    CloseHandle(mMapFile);
#else // !WIN32
    munmap(mBuffer, (size_t)mCapacity * 2);
#endif // WIN32
}

int32_t RingBuffer::Free() const
{
    return (mReadIndex - mWriteIndex - 1) & mCapacityMask;
}

int32_t RingBuffer::Available() const
{
    return (mCapacity - (mReadIndex - mWriteIndex)) & mCapacityMask;
}

int32_t RingBuffer::Capacity() const
{
    return mCapacity;
}

const void* RingBuffer::BeginRead(int32_t& size) const
{
    // This size should only ever increase if the consumer is adding data.
    // In this case, the extra data will not be read until the next read
    // operation and the size returned by EndRead may not be 100% synchronized.
    int32_t available = (mCapacity - (mReadIndex -
        mWriteIndex)) & mCapacityMask;

    // Ensure the request read size does not exceed the available bytes.
    size = std::min(size, available);

    // Return the buffer pointer if the consumer may read at least 1 byte.
    if(0 < size && nullptr != mBuffer)
    {
        return &mBuffer[mReadIndex];
    }
    else
    {
        return nullptr;
    }
}

int32_t RingBuffer::EndRead(int32_t& size)
{
    // This size should only ever increase if the consumer is adding data.
    // In this case, the extra data will not be read until the next read
    // operation and the size returned by EndRead may not be 100% synchronized.
    int32_t available = (mCapacity - (mReadIndex -
        mWriteIndex)) & mCapacityMask;

    // Ensure the request read size does not exceed the available bytes.
    size = std::min(size, available);

    // Update the read index. This is safe because it will only increase
    // the number of free bytes and there is only 1 consumer that will modify
    // this index (the callee).
    mReadIndex = (mReadIndex + size) & mCapacityMask;

    // Return the observed number of available bytes to read.
    // The true number of available bytes should never be less than this.
    return available - size;
}

int32_t RingBuffer::Read(void *pDestination, int32_t size)
{
    // Start the read operation.
    const void *pSource = BeginRead(size);

    // If bytes can be read, do so.
    if(0 < size)
    {
        memcpy(pDestination, pSource, (size_t)size);
    }

    // We do not care about how many bytes are still available.
    (void)EndRead(size);

    // Return the number of bytes read.
    return size;
}

void* RingBuffer::BeginWrite(int32_t& size) const
{
    // This size should only ever increase if the producer is removing data.
    // In this case, the extra free bytes will not be used until the next write
    // operation and the size returned by EndWrite may not be 100% synchronized.
    int32_t free = (mReadIndex - mWriteIndex - 1) & mCapacityMask;

    // Ensure the request write size does not exceed the free bytes.
    size = std::min(size, free);

    // Return the buffer pointer if the producer may write at least 1 byte.
    if(0 < size && nullptr != mBuffer)
    {
        return &mBuffer[mWriteIndex];
    }
    else
    {
        return nullptr;
    }
}

int32_t RingBuffer::EndWrite(int32_t& size)
{
    // This size should only ever increase if the producer is removing data.
    // In this case, the extra free bytes will not be used until the next write
    // operation and the size returned by EndWrite may not be 100% synchronized.
    int32_t free = (mReadIndex - mWriteIndex - 1) & mCapacityMask;

    // Ensure the request write size does not exceed the free bytes.
    size = std::min(size, free);

    // Update the write index. This is safe because it will only increase
    // the number of available bytes and there is only 1 producer that will
    // modify this index (the callee).
    mWriteIndex = (mWriteIndex + size) & mCapacityMask;

    // Return the observed number of free bytes to write.
    // The true number of free bytes should never be less than this.
    return free - size;
}

int32_t RingBuffer::Write(const void *pSource, int32_t size)
{
    // Start the write operation.
    void *pDestination = BeginWrite(size);

    // If bytes can be written, do so.
    if(0 < size)
    {
        memcpy(pDestination, pSource, (size_t)size);
    }

    // We do not care about how many bytes are still free.
    (void)EndWrite(size);

    // Return the number of bytes written.
    return size;
}

int32_t RingBuffer::GetPageSize()
{
#if defined(_WIN32) || defined(_WIN64)
    // Information structure that indicates the page size.
    SYSTEM_INFO info;

    // Get the system information to determine the page size.
    GetSystemInfo(&info);

    // Return the page size.
    return (int32_t)info.dwAllocationGranularity;
#else // !WIN32
    // Return the page size.
    return (int32_t)sysconf(_SC_PAGE_SIZE);
#endif // WIN32
}

bool RingBuffer::IsMultipleOfTwo(int32_t value)
{
    // The value must be positive for this check.
    if(value <= 0)
    {
        return false;
    }

    // Start counting the number of bits set.
    int32_t bits = 0;

    // Count how many bits are set.
    for (int32_t i = 0; i < 31; ++i) // 31 bits for a positive number.
    {
        // If the bit is set, increment the count.
        if(0 != (value & 1))
        {
            bits++;
        }

        // Shift the value down to check the next bit.
        value >>= 1;
    }

    // If only one bit is set, this is a multiple of two.
    return 1 == bits;
}

RingBuffer::Exception RingBuffer::MemoryMapException()
{
    return RingBuffer::Exception("Failed to memory map ring buffer.");
}

RingBuffer::Exception::Exception(const std::string& message) : mMessage(message)
{
}

std::string RingBuffer::Exception::Message() const
{
    return mMessage;
}
