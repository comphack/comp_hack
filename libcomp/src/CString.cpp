/**
 * @file libcomp/src/String.cpp
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

#include "CString.h"

#include <regex_ext>
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <numeric>
#include <iomanip>
#include <cctype>
#include <locale>
#include <regex>

using namespace libcomp;

bool String::mBadArgumentReporting = true;

/**
 * @internal
 * Shared string data oject.
 */
class String::StringData
{
public:
    /**
     * Construct an empty data object.
     */
    StringData();

    /**
     * Perform a deep copy of the string data.
     * @param other Data object to copy the data from.
     */
    StringData(const StringData& other);

    /**
     * Construct a data object with the given data.
     * @param str String data to use.
     * @param length Number of UTF-8 characters in the data.
     */
    StringData(const std::string& str, size_t length);

    /// Number of UTF-8 characters in the data.
    size_t mLength;

    /// UTF-8 encoded string data.
    std::string mString;
};

String::StringData::StringData() : mLength(0)
{
}

String::StringData::StringData(const StringData& other) :
    mLength(other.mLength), mString(other.mString)
{
}

String::StringData::StringData(const std::string& str, size_t length) :
    mLength(length), mString(str)
{
}

String::String() : d(new String::StringData)
{
}

String::String(const String& other) : d(other.d)
{
}

String::String(const std::string& str) : d(new String::StringData)
{
    d->mString = str;
    d->mLength = CalculateLength(d->mString);
}

String::String(const char *szString) : d(new String::StringData)
{
    d->mString = std::string(szString);
    d->mLength = CalculateLength(d->mString);
}

String::String(const char *szString, size_t bytes) : d(new String::StringData)
{
    d->mString = std::string(szString, bytes);
    d->mLength = CalculateLength(d->mString);
}

String::String(const char *szString, size_t offset, size_t bytes) :
    d(new String::StringData)
{
    d->mString = std::string(szString, offset, bytes);
    d->mLength = CalculateLength(d->mString);
}

String::String(size_t bytes, char character) : d(new String::StringData)
{
    d->mString = std::string(bytes, character);
    d->mLength = CalculateLength(d->mString);
}

String String::Left(size_t length) const
{
    if(0 == length)
    {
        return String();
    }
    else if(length >= d->mLength)
    {
        return String(*this);
    }
    else
    {
        std::string::const_iterator it;

        size_t len = length + 1;

        for(it = d->mString.cbegin(); 0 < len &&
            it != d->mString.cend(); ++it)
        {
            if((*it & 0xC0) != 0x80)
            {
                len--;
            }
        }

        StringData *pData = new StringData(std::string(
            d->mString.cbegin(), --it), length);

        return String(pData);
    }
}

String String::Right(size_t length) const
{
    if(0 == length)
    {
        return String();
    }
    else if(length >= d->mLength)
    {
        return String(*this);
    }
    else
    {
        std::string::const_reverse_iterator it;

        size_t len = length;

        for(it = d->mString.crbegin(); 0 < len &&
            it != d->mString.crend(); ++it)
        {
            if((*it & 0xC0) != 0x80)
            {
                len--;
            }
        }

        StringData *pData = new StringData(std::string(it.base(),
            d->mString.cend()), length);

        return String(pData);
    }
}

String String::LeftJustified(size_t width, char fill, bool truncate) const
{
    String s(*this);

    if(truncate)
    {
        s.Truncate(width);
    }

    // Now add filling.
    if(s.Length() < width)
    {
        s = s + std::string(width - s.Length(), fill);
    }

    return s;
}

String String::RightJustified(size_t width, char fill, bool truncate) const
{
    String s(*this);

    if(truncate)
    {
        s.Truncate(width);
    }

    // Now add filling.
    if(s.Length() < width)
    {
        s = std::string(width - s.Length(), fill) + s;
    }

    return s;
}

void String::Truncate(size_t position)
{
    if(0 >= position)
    {
        Clear();
    }
    else
    {
        *this = Left(position);
    }
}

String String::Mid(size_t position, size_t count) const
{
    if(position >= d->mLength)
    {
        return String();
    }
    else
    {
        size_t length;

        // Sanity check the count does not go past the end of the string.
        if((count + position) >= d->mLength)
        {
            count = 0;

            length = d->mLength - position;
        }
        else
        {
            length = count;
        }

        std::string::const_iterator begin;

        position++;

        for(begin = d->mString.cbegin(); 0 < position &&
            begin != d->mString.cend(); ++begin)
        {
            if((*begin & 0xC0) != 0x80)
            {
                position--;
            }
        }

        std::string::const_iterator end;

        if(0 != count)
        {
            for(end = begin; 0 < count && end != d->mString.cend(); ++end)
            {
                if((*end & 0xC0) != 0x80)
                {
                    count--;
                }
            }

            --end;
        }
        else
        {
            end = d->mString.cend();
        }

        StringData *pData = new StringData(std::string(--begin, end), length);

        return String(pData);
    }
}

String::CodePoint String::At(size_t position) const
{
    if(position >= d->mLength)
    {
        return 0;
    }
    else
    {
        std::string::const_iterator begin;

        position++;

        for(begin = d->mString.cbegin(); 0 < position &&
            begin != d->mString.cend(); ++begin)
        {
            if((*begin & 0xC0) != 0x80)
            {
                position--;
            }
        }

        CodePoint cp = 0;

        char bytes[4];

        bytes[0] = *(--begin);

        if(0 == (bytes[0] & 0x80))
        {
            cp = bytes[0] & 0x7F;
        }
        else if(0xC0 == (bytes[0] & 0xE0))
        {
            bytes[1] = *(++begin);

            cp = (CodePoint)(((bytes[0] & 0x1F) << 6) |
                (bytes[1] & 0x3F));
        }
        else if(0xE0 == (bytes[0] & 0xF0))
        {
            bytes[1] = *(++begin);
            bytes[2] = *(++begin);

            cp = (CodePoint)(((bytes[0] & 0x1F) << 12) |
                ((bytes[1] & 0x3F) << 6) |
                (bytes[2] & 0x3F));
        }
        else
        {
            bytes[1] = *(++begin);
            bytes[2] = *(++begin);
            bytes[3] = *(++begin);

            cp = (CodePoint)(((bytes[0] & 0x0F) << 18) |
                ((bytes[1] & 0x3F) << 12) |
                ((bytes[2] & 0x3F) << 6) |
                (bytes[3] & 0x3F));
        }

        return cp;
    }
}

std::list<String> String::Split(const String& delimiter) const
{
    // Used with fixes and integration.
    // Source: http://stackoverflow.com/questions/14265581/

    const std::string& str = d->mString;
    const std::string& delim = delimiter.d->mString;

    std::list<String> list;

    size_t last = 0;
    size_t next = 0;

    while(std::string::npos != (next = str.find(delim, last)))
    {
        list.push_back(String(str.substr(last, next - last)));

        last = next + delim.size();
    }

    list.push_back(String(str.substr(last)));

    return list;
}

bool String::operator==(const char *szString) const
{
    return std::string(szString) == d->mString;
}

bool String::operator==(const std::string& other) const
{
    return other == d->mString;
}

bool String::operator==(const String& other) const
{
    return (d == other.d) || (d->mString == other.d->mString);
}

bool String::operator!=(const char *szString) const
{
    return std::string(szString) != d->mString;
}

bool String::operator!=(const std::string& other) const
{
    return other != d->mString;
}

bool String::operator!=(const String& other) const
{
    return (d != other.d) && (d->mString != other.d->mString);
}

String& String::Append(const String& other)
{
    Detatch();

    d->mLength += other.d->mLength;
    d->mString += other.d->mString;

    return *this;
}

String& String::Prepend(const String& other)
{
    Detatch();

    d->mLength += other.d->mLength;
    d->mString = other.d->mString + d->mString;

    return *this;
}

String& String::operator+=(const String& other)
{
    return Append(other);
}

std::string String::ToUtf8() const
{
    return d->mString;
}

String::String(StringData *pData) : d(pData)
{
}

void String::Detatch()
{
    if(!d.unique())
    {
        d = std::shared_ptr<StringData>(new StringData(*d));
    }
}

size_t String::CalculateLength(const std::string& str) const
{
    size_t length = 0;

    for(auto it = str.cbegin(); it != str.cend(); ++it)
    {
        if((*it & 0xC0) != 0x80)
        {
            length++;
        }
    }

    return length;
}

size_t String::Length() const
{
    return d->mLength;
}

size_t String::Size() const
{
    return d->mString.size();
}

bool String::IsEmpty() const
{
    return 0 == d->mLength;
}

void String::Clear()
{
    d = std::shared_ptr<StringData>(new StringData);
}

bool String::Contains(const String& other) const
{
    return std::string::npos != d->mString.find(other.d->mString);
}

String String::LeftTrimmed() const
{
    std::string s = d->mString;

    // Source: http://stackoverflow.com/questions/216823/
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        std::not1(std::ptr_fun<int, int>(std::isspace))));

    return String(s);
}

String String::RightTrimmed() const
{
    std::string s = d->mString;

    // Source: http://stackoverflow.com/questions/216823/
    s.erase(std::find_if(s.rbegin(), s.rend(),
        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());

    return String(s);
}

String String::Trimmed() const
{
    std::string s = d->mString;

    // Source: http://stackoverflow.com/questions/216823/

    // Left
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        std::not1(std::ptr_fun<int, int>(std::isspace))));

    // Right
    s.erase(std::find_if(s.rbegin(), s.rend(),
        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());

    return String(s);
}

String String::Replace(const String& _search, const String& _replace) const
{
    std::string search = _search.d->mString;
    std::string replace = _replace.d->mString;
    std::string subject = d->mString;

    if(!search.empty())
    {
        size_t pos = 0;

        while(std::string::npos != (pos = subject.find(search, pos)))
        {
            subject.replace(pos, search.length(), replace);

            pos += replace.length();
        }
    }

    return String(subject);
}

String String::Arg(const String& a) const
{
    int matchCount = 0;

    auto callback = [&](const std::smatch& match)
    {
        int n = std::atoi(match.str(1).c_str());

        if(1 == n)
        {
            matchCount++;

            return a.d->mString;
        }
        else
        {
            return "%" + std::to_string(n - 1);
        }
    };

    std::regex re("\\%([0-9]+)");
    std::string s = std::regex_replace(d->mString.cbegin(),
        d->mString.cend(), re, callback);

    if(0 == matchCount && mBadArgumentReporting)
    {
        std::cerr << "Argument not found in string: " << s << std::endl;
    }

    return String(s);
}

String String::Arg(int16_t a, int fieldWidth, int base, char fillChar)
{
    std::stringstream ss;
    ss.width(fieldWidth);
    ss.fill(fillChar);

    ss << std::setbase(base) << a;

    return Arg(String(ss.str()));
}

String String::Arg(uint16_t a, int fieldWidth, int base, char fillChar)
{
    std::stringstream ss;
    ss.width(fieldWidth);
    ss.fill(fillChar);

    ss << std::setbase(base) << a;

    return Arg(String(ss.str()));
}

String String::Arg(int32_t a, int fieldWidth, int base, char fillChar)
{
    std::stringstream ss;
    ss.width(fieldWidth);
    ss.fill(fillChar);

    ss << std::setbase(base) << a;

    return Arg(String(ss.str()));
}

String String::Arg(uint32_t a, int fieldWidth, int base, char fillChar)
{
    std::stringstream ss;
    ss.width(fieldWidth);
    ss.fill(fillChar);

    ss << std::setbase(base) << a;

    return Arg(String(ss.str()));
}

String String::Arg(int64_t a, int fieldWidth, int base, char fillChar)
{
    std::stringstream ss;
    ss.width(fieldWidth);
    ss.fill(fillChar);

    ss << std::setbase(base) << a;

    return Arg(String(ss.str()));
}

String String::Arg(uint64_t a, int fieldWidth, int base, char fillChar)
{
    std::stringstream ss;
    ss.width(fieldWidth);
    ss.fill(fillChar);

    ss << std::setbase(base) << a;

    return Arg(String(ss.str()));
}

String String::Arg(float a, int fieldWidth, int base, char fillChar)
{
    std::stringstream ss;
    ss.width(fieldWidth);
    ss.fill(fillChar);

    ss << std::setbase(base) << a;

    return Arg(String(ss.str()));
}

String String::Arg(double a, int fieldWidth, int base, char fillChar)
{
    std::stringstream ss;
    ss.width(fieldWidth);
    ss.fill(fillChar);

    ss << std::setbase(base) << a;

    return Arg(String(ss.str()));
}

bool String::Matches(const libcomp::String& expression) const
{
    std::smatch match;
    std::string s = ToUtf8();

    if(std::regex_match(s, match, std::regex(expression.ToUtf8())))
    {
        return true;
    }

    return false;
}

bool String::Matches(const libcomp::String& expression,
    std::vector<libcomp::String>& matches) const
{
    std::smatch match;
    std::string s = ToUtf8();

    if(std::regex_match(s, match, std::regex(expression.ToUtf8())))
    {
        matches.resize(static_cast<std::vector<libcomp::String>::size_type>(
            match.size()));

        int i = 0;

        for(auto m : match)
        {
            matches[static_cast<std::vector<libcomp::String>::size_type
                >(i++)] = libcomp::String(m);
        }

        return true;
    }

    return false;
}

String String::ToUpper() const
{
    std::string s = d->mString;

    // Source: http://stackoverflow.com/questions/313970/
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);

    return String(new StringData(s, d->mLength));
}

String String::ToLower() const
{
    std::string s = d->mString;

    // Source: http://stackoverflow.com/questions/313970/
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    return String(new StringData(s, d->mLength));
}

std::vector<char> String::Data(bool nullTerminate) const
{
    std::vector<char> v(d->mString.cbegin(), d->mString.cend());

    if(nullTerminate)
    {
        v.push_back('\0');
    }

    return v;
}

const char* String::C() const
{
    return d->mString.c_str();
}

bool String::IsReportingBadArguments()
{
    return mBadArgumentReporting;
}

void String::SetBadArgumentReporting(bool enabled)
{
    mBadArgumentReporting = enabled;
}

String String::Join(const std::list<String>& strings, const String& delimiter)
{
    // Source: http://stackoverflow.com/questions/15347123/
    return std::accumulate(strings.cbegin(), strings.cend(), String(),
        [& delimiter](String& ss, const String& s) {
            return ss.IsEmpty() ? s : ss + delimiter + s;
        });
}

::std::ostream& libcomp::operator<<(::std::ostream& os, const String& str)
{
    return os << str.ToUtf8();
}

bool libcomp::operator==(const char *szString, const String& other)
{
    return other == szString;
}

bool libcomp::operator!=(const char *szString, const String& other)
{
    return other != szString;
}

bool libcomp::operator==(const std::string& str, const String& other)
{
    return other == str;
}

bool libcomp::operator!=(const std::string& str, const String& other)
{
    return other != str;
}

const String libcomp::operator+(const String& a, const String& b)
{
    String s = a;
    s += b;

    return s;
}

String String::FromCodePoint(CodePoint cp)
{
    unsigned char bytes[4];

    // For the UTF-8 encoding format, see: https://en.wikipedia.org/wiki/UTF-8
    if(0x80 > cp)
    {
        bytes[0] = cp & 0x7F;

        return String(reinterpret_cast<char*>(bytes), 1);
    }
    else if(0x800 > cp)
    {
        bytes[0] = static_cast<unsigned char>(0xC0 | ((cp >> 6) & 0x1F));
        bytes[1] = static_cast<unsigned char>(0x80 | (cp & 0x3F));

        return String(reinterpret_cast<char*>(bytes), 2);
    }
    else if(0x10000 > cp)
    {
        bytes[0] = static_cast<unsigned char>(0xE0 | ((cp >> 12) & 0x0F));
        bytes[1] = static_cast<unsigned char>(0x80 | ((cp >> 6) & 0x3F));
        bytes[2] = static_cast<unsigned char>(0x80 | (cp & 0x3F));

        return String(reinterpret_cast<char*>(bytes), 3);
    }
    else
    {
        bytes[0] = static_cast<unsigned char>(0xF0 | ((cp >> 18) & 0x07));
        bytes[1] = static_cast<unsigned char>(0x80 | ((cp >> 12) & 0x3F));
        bytes[2] = static_cast<unsigned char>(0x80 | ((cp >> 6) & 0x3F));
        bytes[3] = static_cast<unsigned char>(0x80 | (cp & 0x3F));

        return String(reinterpret_cast<char*>(bytes), 4);
    }
}
