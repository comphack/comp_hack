/**
 * @file libcomp/src/Convert.cpp
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

#include "Convert.h"
#include "Endian.h"

// Lookup tables for CP-1252 and CP-932.
#include "LookupTableCP1252.h"
#include "LookupTableCP932.h"

#include <limits.h>
#include <stdint.h>

using namespace libcomp;

/**
 * Convert a CP-1252 encoded string to a @ref String.
 * @param szString The string to convert.
 * @param size Optional size of the string.
 * @returns The converted string.
 */
static String FromCP1252Encoding(const uint8_t *szString, int size);

/**
 * Convert a CP-932 encoded string to a @ref String.
 * @param szString The string to convert.
 * @param size Optional size of the string.
 * @returns The converted string.
 */
static String FromCP932Encoding(const uint8_t *szString, int size);

/**
 * Convert the @ref String to a CP-1252 encoded string.
 * @param str String to convert.
 * @param nullTerminator Indicates if a null terminator should be added.
 * @returns The converted string.
 */
static std::vector<char> ToCP1252Encoding(const String& str,
    bool nullTerminator = true);

/**
 * Convert the @ref String to a CP-932 encoded string.
 * @param str String to convert.
 * @param nullTerminator Indicates if a null terminator should be added.
 * @returns The converted string.
 */
static std::vector<char> ToCP932Encoding(const String& str,
    bool nullTerminator = true);

static String FromCP1252Encoding(const uint8_t *szString, int size)
{
    // If the size is 0, return an empty string. If the size is less than 0,
    // read as much as possible. In this case we will limit the string to the
    // max size of an integer (which is so huge it should not be reached).
    // Chances are the String class will barf if you try to create a string
    // that big.
    if(0 == size)
    {
        return String();
    }
    else if(0 > size)
    {
        size = INT_MAX;
    }

    // Obtain pointers to the lookup table so it may be used as an array of
    // unsigned 16-bit values.
    const uint16_t *pMappingTo = (uint16_t*)LookupTableCP1252;
    const uint16_t *pMappingFrom = pMappingTo + 65536;

    // String to store the converted string into.
    String final;

    // Loop over the string until the null terminator has been or the
    // requested size has been reached.
    while(0 < size-- && 0 != *szString)
    {
        // Retrieve the next byte of the string and determine the mapped code
        // point for the desired encoding. Advance the pointer to the next
        // value in the source string.
        uint16_t cp1252 = *(szString++);

        String::CodePoint unicode = pMappingFrom[cp1252];

        // If there is no mapped codec, return an empty string to indicate an
        // error.
        if(0 == unicode)
        {
            return String();
        }

        // Append the mapped code point to the string.
        final += String::FromCodePoint(unicode);
    }

    // Return the converted string.
    return final;
}

static String FromCP932Encoding(const uint8_t *szString, int size)
{
    // If the size is 0, return an empty string. If the size is less than 0,
    // read as much as possible. In this case we will limit the string to the
    // max size of an integer (which is so huge it should not be reached).
    // Chances are the String class will barf if you try to create a string
    // that big.
    if(0 == size)
    {
        return String();
    }
    else if(0 > size)
    {
        size = INT_MAX;
    }

    // Obtain pointers to the lookup table so it may be used as an array of
    // unsigned 16-bit values.
    const uint16_t *pMappingTo = (uint16_t*)LookupTableCP932;
    const uint16_t *pMappingFrom = pMappingTo + 65536;

    // String to store the converted string into.
    String final;

    // Loop over the string until the null terminator has been or the
    // requested size has been reached.
    while(0 < size-- && 0 != *szString)
    {
        // Retrieve the next byte of the string and determine the mapped code
        // point for the desired encoding. CP932 is a multi-byte format similar
        // to Shift-JIS. As such, if the most significant bit is set, another
        // byte needs to be read and added to the code point before conversion.
        // After each byte read from the string, the string pointer should be
        // advanced.
        uint16_t cp932 = *(szString++);

        // Certain byte values indicate multibyte characters.
        if( (0x81 <= cp932 && 0x9F >= cp932) ||
            (0xE0 <= cp932 && 0xFC >= cp932) )
        {
            // Sanity check that we can read the 2nd byte of the code point.
            // If not, we should return an empty string to indicate an error.
            /// @todo Consider throwing an exception as well (conversion
            /// exceptions should be enabled by a \#define).
            if(1 > size--)
            {
                return String();
            }

            // A multi-byte CP932 code point consists of the first byte in the
            // 8 most significant bits and the second byte in the 8 least
            // significant bits.
            cp932 = (uint16_t)( (cp932 << 8) | *(szString++) );
        }

        // If there is no mapped codec, return an empty string to indicate an
        // error.
        String::CodePoint unicode = pMappingFrom[cp932];

        if(0 == unicode)
        {
            return String();
        }

        // Append the mapped code point to the string.
        final += String::FromCodePoint(unicode);
    }

    // Return the converted string.
    return final;
}

static std::vector<char> ToCP1252Encoding(const String& str,
    bool nullTerminator)
{
    // Obtain a pointer to the lookup table so it may be used as an array of
    // unsigned 16-bit values.
    const uint16_t *pMappingTo = (uint16_t*)LookupTableCP1252;

    // Used to add a null terminator to the end of the byte array.
    char zero = 0;

    // String to store the converted string into.
    std::vector<char> final;

    // Loop over every character in the source string.
    for(size_t i = 0; i < str.Length(); ++i)
    {
        // Get the Unicode code point for the current character.
        String::CodePoint unicode = str.At(i);

        // Find the mapped code point for the desired encoding.
        uint16_t cp1252 = pMappingTo[unicode];

        // Add the converted character to the final string.
        final.push_back((char)(cp1252 & 0xFF));
    }

    // Append a null terminator to the end of the final string.
    if(nullTerminator)
    {
        final.push_back(zero);
    }

    // Return the converted string.
    return final;
}

static std::vector<char> ToCP932Encoding(const String& str,
    bool nullTerminator)
{
    // Obtain a pointer to the lookup table so it may be used as an array of
    // unsigned 16-bit values.
    const uint16_t *pMappingTo = (uint16_t*)LookupTableCP932;

    // Used to add a null terminator to the end of the byte array.
    char zero = 0;

    // String to store the converted string into.
    std::vector<char> final;

    // Loop over every character in the source string.
    for(size_t i = 0; i < str.Length(); ++i)
    {
        // Get the Unicode code point for the current character.
        String::CodePoint unicode = str.At(i);

        // Find the mapped code point for the desired encoding.
        uint16_t cp932 = pMappingTo[unicode];

        // If the most significant bit is set, this CP932 code point is a
        // multi-byte codepoint.
        if(0xFF < cp932)
        {
            // Double byte, ensure the value is in big endian host order.
            cp932 = htobe16(cp932);

            // Write two bytes to the final string.
            final.push_back((char)(cp932 & 0xFF));
            final.push_back((char)((cp932 >> 8) & 0xFF));
        }
        else
        {
            // Single byte, write one byte to the final string.
            final.push_back((char)(cp932 & 0xFF));
        }
    }

    // Append a null terminator to the end of the final string.
    if(nullTerminator)
    {
        final.push_back(zero);
    }

    // Return the converted string.
    return final;
}

String Convert::FromEncoding(Encoding_t encoding,
    const char *szString, int size)
{
    // Determine the function to call based on the encoding requested.
    switch(encoding)
    {
        case ENCODING_CP932:
            return FromCP932Encoding((uint8_t*)szString, size);
        case ENCODING_CP1252:
            return FromCP1252Encoding((uint8_t*)szString, size);
        default:
            break;
    }

    // Default to a UTF-8 encoded string.
    if(0 > size)
    {
        return String(szString);
    }
    else
    {
        return String(szString, (size_t)size);
    }
}

String Convert::FromEncoding(Encoding_t encoding, const std::vector<char>& str)
{
    return str.size() > 0 ? FromEncoding(encoding, &str[0], (int)str.size()) : String();
}

std::vector<char> Convert::ToEncoding(Encoding_t encoding, const String& str,
    bool nullTerminator)
{
    // Determine the function to call based on the encoding requested.
    switch(encoding)
    {
        case ENCODING_CP932:
            return ToCP932Encoding(str, nullTerminator);
        case ENCODING_CP1252:
            return ToCP1252Encoding(str, nullTerminator);
        default:
            break;
    }

    // Default to a UTF-8 encoded string.
    return str.Data(nullTerminator);
}

size_t Convert::SizeEncoded(Encoding_t encoding, const String& str,
    size_t align)
{
    // Convert the string to determine the size of the encoded result.
    std::vector<char> out = ToEncoding(encoding, str, false);

    // If the string should be aligned, calculate the aligned size.
    if(0 < align)
    {
        return ((out.size() + align - 1) / align) * align;
    }

    // Return the size of the encoded string without alignment.
    return out.size();
}
