/**
 * @file libcomp/src/String.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Unicode string class.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_STRING_H
#define LIBCOMP_SRC_STRING_H

#include <stdint.h>

#include <iomanip>
#include <limits>
#include <list>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace libcomp
{

/**
 * UTF-8 encoded string object.
 */
class String
{
public:
    /**
     * Unicode code point type.
     */
    typedef uint32_t CodePoint;

    /**
     * Construct an empty string.
     */
    String();

    /**
     * Perform a shallow copy of another string.
     * @param other The string to copy.
     */
    String(const String& other);

    /**
     * Construct a string from a UTF-8 encoded STL string object.
     * @param str UTF-8 encoded STL string object.
     */
    String(const std::string& str);

    /**
     * Construct a string from a C-style UTF-8 encoded string.
     * @param szString C-style UTF-8 encoded string.
     */
    String(const char *szString);

    /**
     * Construct a string from a C-style UTF-8 encoded string.
     * @param szString C-style UTF-8 encoded string.
     * @param bytes Number of bytes in the C-style string.
     */
    String(const char *szString, size_t bytes);

    /**
     * Construct a string from a subsection of C-style UTF-8 encoded string.
     * @param szString C-style UTF-8 encoded string.
     * @param offset Number of bytes until the beginning of the subsection of
     *   the C-style string.
     * @param bytes Number of bytes in the subsection of the C-style string.
     */
    String(const char *szString, size_t offset, size_t bytes);

    /**
     * Create a string filled with an ASCII character.
     * @param bytes Number of bytes to fill.
     * @param character ASCII character to fill the string with.
     */
    String(size_t bytes, char character);

    /**
     * Return a sub-string of the specified length starting from the
     * beginning of the string. If the string is less than the length
     * specified, the entire string is returned. This operates on characters,
     * not bytes.
     * @param length Max number of characters (not bytes) to extract.
     * @returns The sub-string.
     */
    String Left(size_t length) const;

    /**
     * Return a sub-string of the specified length starting from the
     * end of the string. If the string is less than the length specified,
     * the entire string is returned. This operates on characters, not bytes.
     * This will count from the end of the string but will not reverse it.
     * @param length Max number of characters (not bytes) to extract.
     * @returns The sub-string.
     */
    String Right(size_t length) const;

    /**
     * Return the string followed by filling until the desired width is met.
     * @param width Desired length of the filled string.
     * @param fill Character to fill the rest of the string with.
     * @param truncate If longer strings should be truncated.
     * @returns Copy of the string filled to the desired width.
     */
    String LeftJustified(size_t width, char fill = ' ',
        bool truncate = false) const;

    /**
     * Return filling followed by the string until the desired width is met.
     * @param width Desired length of the filled string.
     * @param fill Character to fill the rest of the string with.
     * @param truncate If longer strings should be truncated.
     * @returns Copy of the string filled to the desired width.
     */
    String RightJustified(size_t width, char fill = ' ',
        bool truncate = false) const;

    /**
     * Truncate the string to the given size.
     * @param position Number of characters to truncate the string to.
     */
    void Truncate(size_t position);

    /**
     * Get a substring.
     * @param position Number of characters into the string to start at.
     * @param count Number of characters to extract or 0 for all.
     * @returns Requested sub-string.
     */
    String Mid(size_t position, size_t count = 0) const;

    /**
     * Get a Unicode character at the desired position.
     * @param position Number of characters into the string to get the
     *   character at.
     * @returns The Unicode code point at the desired position or 0.
     */
    CodePoint At(size_t position) const;

    /**
     * Split a string by a delimiter.
     * @param delimiter Sub-string to split the string by.
     * @returns All components on either side of each instance of delimiter.
     */
    std::list<String> Split(const String& delimiter) const;

    /**
     * Get the number of characters in the string.
     * @returns Number of characters in the string.
     */
    size_t Length() const;

    /**
     * Get the number of bytes in the string.
     * @returns Number of bytes in the string.
     */
    size_t Size() const;

    /**
     * Determine if the string is empty.
     * @returns true if the string is empty.
     */
    bool IsEmpty() const;

    /**
     * Empty the string.
     */
    void Clear();

    /**
     * Determine if a string contains the specified sub-string.
     * @param other Sub-string to look for.
     * @returns true if the sub-string is within the string.
     */
    bool Contains(const String& other) const;

    /**
     * Get the text preceding a sub-string.
     * @param other Sub-string to look for.
     * @reuturns Text preceding a sub-string.
     */
    String LeftOf(const String& other) const;

    /**
     * Get the text following a sub-string.
     * @param other Sub-string to look for.
     * @reuturns Text following a sub-string.
     */
    String RightOf(const String& other) const;

    /**
     * Return a copy of the string data.
     * @param nullTerminate If the string data should be null terminated.
     * @returns A copy of the string data.
     */
    std::vector<char> Data(bool nullTerminate = true) const;

    /**
     * Return a pointer to the string data for use in C-style string functions.
     * A null terminator will be added to the end of the buffer.
     * @returns C-style string data.
     * @note The pointer returned may be invalidated by further calls to other
     *   member functions that modify the object.
     */
    const char* C() const;

    /**
     * Remove all whitespace characters (as determined by std::isspace) from
     * the beginning of the string.
     * @returns Copy of the string with the whitespace removed.
     */
    String LeftTrimmed() const;

    /**
     * Remove all whitespace characters (as determined by std::isspace) from
     * the end of the string.
     * @returns Copy of the string with the whitespace removed.
     */
    String RightTrimmed() const;

    /**
     * Remove all whitespace characters (as determined by std::isspace) from
     * the beginning and end of the string.
     * @returns Copy of the string with the whitespace removed.
     */
    String Trimmed() const;

    /**
     * Replace all occurrences of substring with the given text.
     * @param search Substring to replace.
     * @param replace Text to replace the substring with.
     * @returns Copy of the string with the substring replaces.
     */
    String Replace(const String& search, const String& replace) const;

    /**
     * Replace the first argument of the string (%1) with the argument. All
     * other arguments will be shifted down by 1. No argument number may be
     * less than 1 or over 99. If an argument is used multiple times in the
     * string, all instances will be replaced. If the argument is not found
     * in the string, the string with all argument numbers shifted down one
     * is returned and an error is reported to standard error if
     * @ref SetBadArgumentReporting is set.
     *
     * @param a Argument to place into the string.
     * @returns String with the argument added.
     */
    String Arg(const String& a) const;

    /**
     * Convert the string into lowercase.
     * @returns Copy of the string in lowercase.
     */
    String ToUpper() const;

    /**
     * Convert the string into uppercase.
     * @returns Copy of the string in uppercase.
     */
    String ToLower() const;

    /**
     * Replace the first argument with a number.
     * @param a Number to place into the first argument.
     * @param fieldWidth Minimum number of characters.
     * @param base Base of the number (8, 10 or 16).
     * @param fillChar Character to pad the value with.
     * @returns String with the argument added.
     */
    String Arg(int16_t a, int fieldWidth = 0, int base = 10,
        char fillChar = ' ');

    /**
     * Replace the first argument with a number.
     * @param a Number to place into the first argument.
     * @param fieldWidth Minimum number of characters.
     * @param base Base of the number (8, 10 or 16).
     * @param fillChar Character to pad the value with.
     * @returns String with the argument added.
     */
    String Arg(uint16_t a, int fieldWidth = 0, int base = 10,
        char fillChar = ' ');

    /**
     * Replace the first argument with a number.
     * @param a Number to place into the first argument.
     * @param fieldWidth Minimum number of characters.
     * @param base Base of the number (8, 10 or 16).
     * @param fillChar Character to pad the value with.
     * @returns String with the argument added.
     */
    String Arg(int32_t a, int fieldWidth = 0, int base = 10,
        char fillChar = ' ');

    /**
     * Replace the first argument with a number.
     * @param a Number to place into the first argument.
     * @param fieldWidth Minimum number of characters.
     * @param base Base of the number (8, 10 or 16).
     * @param fillChar Character to pad the value with.
     * @returns String with the argument added.
     */
    String Arg(uint32_t a, int fieldWidth = 0, int base = 10,
        char fillChar = ' ');

    /**
     * Replace the first argument with a number.
     * @param a Number to place into the first argument.
     * @param fieldWidth Minimum number of characters.
     * @param base Base of the number (8, 10 or 16).
     * @param fillChar Character to pad the value with.
     * @returns String with the argument added.
     */
    String Arg(int64_t a, int fieldWidth = 0, int base = 10,
        char fillChar = ' ');

    /**
     * Replace the first argument with a number.
     * @param a Number to place into the first argument.
     * @param fieldWidth Minimum number of characters.
     * @param base Base of the number (8, 10 or 16).
     * @param fillChar Character to pad the value with.
     * @returns String with the argument added.
     */
    String Arg(uint64_t a, int fieldWidth = 0, int base = 10,
        char fillChar = ' ');

    /**
     * Replace the first argument with a number.
     * @param a Number to place into the first argument.
     * @param fieldWidth Minimum number of characters.
     * @param base Base of the number (8, 10 or 16).
     * @param fillChar Character to pad the value with.
     * @returns String with the argument added.
     */
    String Arg(float a, int fieldWidth = 0, int base = 10,
        char fillChar = ' ');

    /**
     * Replace the first argument with a number.
     * @param a Number to place into the first argument.
     * @param fieldWidth Minimum number of characters.
     * @param base Base of the number (8, 10 or 16).
     * @param fillChar Character to pad the value with.
     * @returns String with the argument added.
     */
    String Arg(double a, int fieldWidth = 0, int base = 10,
        char fillChar = ' ');

    /**
     * Determine if a string matches a regular expression.
     * @param expression Regular expression to match.
     * @returns true if the string matches, false otherwise.
     */
    bool Matches(const libcomp::String& expression) const;

    /**
     * Determine if a string matches a regular expression.
     * @param expression Regular expression to match.
     * @param matches The capture list. Index 0 is the full match.
     * @returns true if the string matches, false otherwise.
     */
    bool Matches(const libcomp::String& expression,
        std::vector<libcomp::String>& matches) const;

    /**
     * Compare the string to a C-style UTF-8 encoded string.
     * @param szString C-style UTF-8 encoded string to compare to.
     * @returns true if the string data matches exactly.
     */
    bool operator==(const char *szString) const;

    /**
     * Compare the string to a UTF-8 encoded STL string object.
     * @param other UTF-8 encoded STL string object to compare to.
     * @returns true if the string data matches exactly.
     */
    bool operator==(const std::string& other) const;

    /**
     * Compare the string to another string.
     * @param other String to compare to.
     * @returns true if the string data matches exactly.
     */
    bool operator==(const String& other) const;

    /**
     * Compare the string to a C-style UTF-8 encoded string.
     * @param szString C-style UTF-8 encoded string to compare to.
     * @returns true if the string data does not match.
     */
    bool operator!=(const char *szString) const;

    /**
     * Compare the string to a UTF-8 encoded STL string object.
     * @param other UTF-8 encoded STL string object to compare to.
     * @returns true if the string data does not match.
     */
    bool operator!=(const std::string& other) const;

    /**
     * Compare the string to another string.
     * @param other String to compare to.
     * @returns true if the string data does not match.
     */
    bool operator!=(const String& other) const;

    /**
     * Append another string to the end of this one.
     * @param other String to append to the end of this one.
     * @returns Reference to this string (to chain with other calls).
     */
    String& Append(const String& other);

    /**
     * Prepend another string to the beginning of this one.
     * @param other String to prepend to the beginning of this one.
     * @returns Reference to this string (to chain with other calls).
     */
    String& Prepend(const String& other);

    /**
     * Append another string to the end of this one.
     * @param other String to append to the end of this one.
     * @returns Reference to this string.
     */
    String& operator+=(const String& other);

    /**
     * Get the underlying string data as an STL string object.
     * @returns The UTF-8 encoded string as an STL string object.
     */
    std::string ToUtf8() const;

    /**
     * @brief Convert the string to an integer.
     * @param pOK If not null, this will set the variable pointed by pOK to
     *   true if the string was converted or false if it was not.
     * @returns 0 if the conversion failed or the value is 0 or the value of
     *   the string represented as an integer.
     * @note This works with hex (0x prefix) and octal (0 prefix) as well.
     */
    template<typename T>
    T ToInteger(bool *pOK = nullptr) const
    {
        T value = 0;
        bool ok = true;
        int base = 10;
        std::string s = ToUtf8();
        std::string sign;

        if(!s.empty() && ('-' == s[0] || '+' == s[0]))
        {
            sign = s[0];

            s = s.substr(1);
        }

        if(2 <= s.size() && '0' == s[0] && 'x' == s[1])
        {
            base = 16;

            s = s.substr(2);
        }
        else if(1 < s.size() && '0' == s[0])
        {
            base = 8;

            s = s.substr(1);
        }

        s = sign + s;

        bool negative = !s.empty() && '-' == s[0];

        if(ok)
        {
            if(!std::numeric_limits<T>::is_integer ||
                std::numeric_limits<T>::is_signed)
            {
                int64_t temp;

                std::stringstream ss(s);
                ss >> std::setbase(base) >> temp;

                if(ss && temp >= static_cast<int64_t>(
                    std::numeric_limits<T>::lowest()) &&
                    temp <= static_cast<int64_t>(
                    std::numeric_limits<T>::max()))
                {
                    value = static_cast<T>(temp);
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                uint64_t temp;

                std::stringstream ss(s);
                ss >> std::setbase(base) >> temp;

                if(ss && temp >= static_cast<uint64_t>(
                    std::numeric_limits<T>::lowest()) &&
                    temp <= static_cast<uint64_t>(
                    std::numeric_limits<T>::max()) &&
                    !negative)
                {
                    value = static_cast<T>(temp);
                }
                else
                {
                    ok = false;
                }
            }
        }

        if(nullptr != pOK)
        {
            *pOK = ok;
        }

        return value;
    }

    /**
     * @brief Convert the string to a decimal (float or double).
     * @param pOK If not null, this will set the variable pointed by pOK to
     *   true if the string was converted or false if it was not.
     * @returns 0 if the conversion failed or the value is 0 or the value of
     *   the string represented as an integer.
     */
    template<typename T>
    T ToDecimal(bool *pOK = nullptr) const
    {
        T value = 0;

        std::stringstream ss(ToUtf8());
        ss >> value;

        if(nullptr != pOK)
        {
            if(ss)
            {
                *pOK = true;
            }
            else
            {
                *pOK = false;
            }
        }

        return value;
    }

    /**
     * Get if argument errors will be reported. This will report the
     * error over the standard error stream.
     * @returns true if argument errors will be reported.
     * @sa SetBadArgumentReporting
     * @sa Arg
     */
    static bool IsReportingBadArguments();

    /**
     * Set if argument errors will be reported. This will report the
     * error over the standard error stream.
     * @param enable true if argument errors should be reported.
     * @sa IsReportingBadArguments
     * @sa Arg
     */
    static void SetBadArgumentReporting(bool enabled);

    static String Join(const std::list<String>& strings,
        const String& delimiter);

    /**
     * Convert a Unicode code point into a String object.
     * @param cp Unicode code point to transform into a String object.
     * @returns The Unicode code point wrapped in a String object.
     */
    static String FromCodePoint(CodePoint cp);

private:
    class StringData;

    /**
     * @internal
     * Construct a string with the given data.
     * @param pData Internal string data structure.
     */
    String(StringData *pData);

    /**
     * @internal
     * Perform a deep copy of the string data. This will take ownership
     * of the data if no other string references it.
     */
    void Detatch();

    /**
     * @internal
     * Calculate the number of UTF-8 characters in the string data.
     * @param str String data to count characters in.
     * @returns Number of UTF-8 characters in the string data.
     */
    size_t CalculateLength(const std::string& str) const;

    /**
     * @internal
     * Shared pointer to the string data.
     */
    std::shared_ptr<StringData> d;

    /**
     * @internal
     * Indicates if bad argument errors should be reported.
     * @sa Arg
     */
    static bool mBadArgumentReporting;
};

/**
 * STL stream operator for a String object.
 * @param os STL stream to add the string into.
 * @param str String to add into the STL stream.
 * @returns Reference to the STL stream.
 * @relates String
 */
::std::ostream& operator<<(::std::ostream& os, const String& str);

/**
 * Compare a string to a C-style UTF-8 encoded string.
 * @param szString C-style UTF-8 encoded string to compare to.
 * @param other String object to compare to.
 * @returns true if the string data matches exactly.
 * @relates String
 */
bool operator==(const char *szString, const String& other);

/**
 * Compare a string to a C-style UTF-8 encoded string.
 * @param szString C-style UTF-8 encoded string to compare to.
 * @param other String object to compare to.
 * @returns true if the string data does not match.
 * @relates String
 */
bool operator!=(const char *szString, const String& other);

/**
 * Compare a string to a UTF-8 encoded STL string object.
 * @param str String object to compare to.
 * @param other UTF-8 encoded STL string object to compare to.
 * @returns true if the string data matches exactly.
 * @relates String
 */
bool operator==(const std::string& str, const String& other);

/**
 * Compare a string to a UTF-8 encoded STL string object.
 * @param str String object to compare to.
 * @param other UTF-8 encoded STL string object to compare to.
 * @returns true if the string data does not match.
 * @relates String
 */
bool operator!=(const std::string& str, const String& other);

/**
 * Concatenate two strings together.
 * @param a First string to concatenate.
 * @param b Second string to concatenate.
 * @returns The first string concatenated with the second.
 * @relates String
 */
const String operator+(const String& a, const String& b);

/**
 * Operator to compare two strings (for sorting).
 * @param a First string to compare.
 * @param b Second string to compare.
 * @returns true if b < a
 * @relates String
 */
bool operator<(const libcomp::String& a, const libcomp::String& b);

} // namespace libcomp

namespace std
{
    template<>
    struct hash<libcomp::String>
    {
        typedef libcomp::String argument_type;
        typedef std::size_t result_type;

        result_type operator()(const argument_type& s) const
        {
            return std::hash<std::string>{}(s.ToUtf8());
        }
    };
} // namespace std

#endif // LIBCOMP_SRC_STRING_H
