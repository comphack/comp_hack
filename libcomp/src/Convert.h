/**
 * @file libcomp/src/Convert.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Routines to convert strings between encodings.
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

#ifndef LIBCOMP_SRC_CONVERT_H
#define LIBCOMP_SRC_CONVERT_H

#include <CString.h>

namespace libcomp
{

namespace Convert
{

/**
 * Valid string encodings.
 */
typedef enum
{
    ENCODING_UTF8 = 0,
    ENCODING_CP932,
    ENCODING_CP1252,
} Encoding_t;

/**
 * Convert a string from the specified @em encoding to a String.
 * @param encoding Encoding to use. Can be one of:
 * - ENCODING_UTF8 (Unicode)
 * - ENCODING_CP932 (Japanese)
 * - ENCODING_CP1252 (US English)
 * @param szString Pointer to the string to convert.
 * @param size Optional size of the buffer. If a valid size is not specified,
 *   it is assumed that the string is null terminated.
 * @returns The converted string or an empty string if there was a conversion
 *   error.
 * @sa libcomp::Convert::ToEncoding
 * @sa libcomp::Convert::SizeEncoded
 */
String FromEncoding(Encoding_t encoding, const char *szString, int size = -1);

/**
 * Convert a string from the specified @em encoding to a String.
 * @param encoding Encoding to use. Can be one of:
 * - ENCODING_UTF8 (Unicode)
 * - ENCODING_CP932 (Japanese)
 * - ENCODING_CP1252 (US English)
 * @param str String data to convert.
 * @param size Optional size of the buffer. If a valid size is not specified,
 *   it is assumed that the string is null terminated.
 * @returns The converted string or an empty string if there was a conversion
 *   error.
 * @sa libcomp::Convert::ToEncoding
 * @sa libcomp::Convert::SizeEncoded
 */
String FromEncoding(Encoding_t encoding, const std::vector<char>& str);

/**
 * Convert a String to the specified @em encoding.
 * @param encoding Encoding to use. Can be one of:
 * - ENCODING_UTF8 (Unicode)
 * - ENCODING_CP932 (Japanese)
 * - ENCODING_CP1252 (US English)
 * @param str String to convert.
 * @param nullTerminator Indicates if a null terminator should be added.
 * @returns The converted string or an empty string if there was a conversion
 *   error.
 * @sa libcomp::Convert::FromEncoding
 * @sa libcomp::Convert::SizeEncoded
 */
std::vector<char> ToEncoding(Encoding_t encoding, const String& str,
    bool nullTerminator = true);

/**
 * Determine the size of a String if it was converted to the specified
 * @em encoding. If @em align is specified, the size will be rounded up to a
 * multiple of @em align.
 *
 * @param encoding Encoding to use. Can be one of:
 * - ENCODING_UTF8 (Unicode)
 * - ENCODING_CP932 (Japanese)
 * - ENCODING_CP1252 (US English)
 * @param str String that would be converted.
 * @param align Byte alignment to use when calculating the size of the string.
 *   For example a string of length 13 would return a length of 16 if align
 *   was set to 4.
 * @returns The size of the string if it was converted to the desired encoding
 *   with the optional byte alignment.
 */
size_t SizeEncoded(Encoding_t encoding, const String& str, size_t align = 0);

} // namespace Convert

} // namespace libcomp

#endif // LIBCOMP_SRC_CONVERT_H
