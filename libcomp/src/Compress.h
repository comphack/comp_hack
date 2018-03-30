/**
 * @file libcomp/src/Compress.h
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

#ifndef LIBCOMP_SRC_COMPRESS_H
#define LIBCOMP_SRC_COMPRESS_H

#include <stdint.h>

namespace libcomp
{

/**
 * Routines to compress and decompress data using zlib.
 */
namespace Compress
{

/**
 * @brief %Compress an input buffer into the output buffer.
 * %Compress @em inSize bytes of data from the input buffer @em in into the
 * output buffer @em out that has a size of @em outSize bytes. Compression
 * uses zlib. The optional compression level must be a value from 0 to 9
 * (inclusive). The lower the compression level value, the larger the
 * compressed data will be.
 * @param pIn Input buffer containing the data to be compressed.
 * @param pOut Output buffer to store the compressed data.
 * @param inSize Size of the input data to be compressed.
 * @param outSize Size of output buffer the compressed data will be written to.
 * @param compressionLevel Compression level to use. The compression level is
 * a value between 0 and 9. 0 indicates no compression and 9 indicates maximum
 * compression. A value of -1 will use Z_DEFAULT_COMPRESSION.
 * @returns Positive numbers indicate how many bytes of data have been written
 * to the output buffer; negative numbers indicate an error. The errors are:
 * @retval -1 Invalid arguments
 * @retval -2 Initialization error
 * @retval -3 Decompression error
 * @retval -4 Cleanup error
 */
int32_t Compress(void *pIn, void *pOut, int32_t inSize, int32_t outSize,
    int32_t compressionLevel = -1);

/**
 * @brief %Decompress an input buffer into the output buffer.
 * %Decompress @em inSize bytes of data from the input buffer @em in into the
 * output buffer @em out that has a size of @em outSize bytes. Decompression
 * uses zlib.
 * @param pIn Input buffer containing the data to be decompressed.
 * @param pOut Output buffer to store the decompressed data.
 * @param inSize Size of the input data to be decompressed.
 * @param outSize Size of output buffer the decompressed data
 * will be written to.
 * @returns Positive numbers indicate how many bytes of data have been written
 * to the output buffer; negative numbers indicate an error. The errors are:
 * @retval -1 Invalid arguments
 * @retval -2 Initialization error
 * @retval -3 Decompression error
 * @retval -4 Cleanup error
 */
int32_t Decompress(void *pIn, void *pOut, int32_t inSize, int32_t outSize);

} // namespace Compress

} // namespace libcomp

#endif // LIBCOMP_SRC_COMPRESS_H
