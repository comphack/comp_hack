/**
 * @file libcomp/src/Compress.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Routines to compress and decompress data using zlib.
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

#include "Compress.h"

// zlib compression library (http://www.zlib.net)
#include <zlib.h>

using namespace libcomp;

int32_t Compress::Compress(void *pIn, void *pOut, int32_t inSize,
    int32_t outSize, int32_t compLvl)
{
    // Sanity check the arguments. We may not have null buffers and all sizes
    // must be positive numbers. The compression level must be between 0 and 9.
    if(nullptr == pIn || nullptr == pOut || 1 > inSize || 1 > outSize ||
        -1 > compLvl || 9 < compLvl)
    {
        return -1;
    }

    // If the compression level is -1, use the default compression level.
    if(-1 == compLvl)
    {
        compLvl = Z_DEFAULT_COMPRESSION;
    }

    // Allocate the zlib stream object to store the state of the compression.
    z_stream strm;

    // Initialize the unset variables to null.
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // Make sure the zlib stream initializes properly.
    if(Z_OK != deflateInit(&strm, compLvl))
    {
        return -2;
    }

    // Tell zlib about the input buffer and how many bytes it contains.
    strm.avail_in = (uInt)inSize;
    strm.next_in = (Bytef*)pIn;

    // Tell zlib about the output buffer and how many bytes it contains.
    strm.avail_out = (uInt)outSize;
    strm.next_out = (Bytef*)pOut;

    // Attempt to compress the data and return if an error occured.
    if(Z_STREAM_END != deflate(&strm, Z_FINISH))
    {
        return -3;
    }

    // Save how many bytes of the output buffer were written to.
    int32_t written = (int32_t)strm.total_out;

    // Cleanup the zlib stream object (so it may free memory).
    if(Z_OK != deflateEnd(&strm))
    {
        return -4;
    }

    // Success! Return how many bytes were written to the output buffer.
    return written;
}

int32_t Compress::Decompress(void *pIn, void *pOut,
    int32_t inSize, int32_t outSize)
{
    // Sanity check the arguments. We may not have null buffers and all sizes
    // must be positive numbers.
    if(nullptr == pIn || nullptr == pOut || 1 > inSize || 1 > outSize)
    {
        return -1;
    }

    // Allocate the zlib stream object to store the state of the decompression.
    z_stream strm;

    // Initialize the unset variables to null.
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // Tell zlib about the input buffer and how many bytes it contains.
    strm.avail_in = (uInt)inSize;
    strm.next_in = (Bytef*)pIn;

    // Make sure the zlib stream initializes properly.
    if(Z_OK != inflateInit(&strm))
    {
        return -2;
    }

    // Tell zlib about the output buffer and how many bytes it contains.
    strm.avail_out = (uInt)outSize;
    strm.next_out = (Bytef*)pOut;

    // Attempt to decompress the data and return if an error occured.
    if(Z_STREAM_END != inflate(&strm, Z_FINISH))
    {
        return -3;
    }

    // Save how many bytes of the output buffer were written to.
    int32_t written = (int32_t)strm.total_out;

    // Cleanup the zlib stream object (so it may free memory).
    if(Z_OK != inflateEnd(&strm))
    {
        return -4;
    }

    // Success! Return how many bytes were written to the output buffer.
    return written;
}
