/**
 * @file libcomp/src/VectorStream.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to use a std::vector<char> as a stream.
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

#ifndef LIBCOMP_SRC_VECTORSTREAM_H
#define LIBCOMP_SRC_VECTORSTREAM_H

// Standard C++11 Includes
#include <streambuf>
#include <vector>

namespace libcomp
{

template<typename CharT, typename TraitsT = std::char_traits<CharT>>
class VectorStream : public std::basic_streambuf<CharT, TraitsT>
{
private:
    std::vector<CharT>& mData;

public:
    VectorStream(std::vector<CharT>& data) : mData(data)
    {
        this->setg(data.data(), data.data(), data.data() + data.size());
    }

protected:
    virtual typename std::basic_streambuf<CharT, TraitsT>::int_type overflow(
        typename std::basic_streambuf<CharT, TraitsT>::int_type c =
            std::basic_streambuf<CharT, TraitsT>::traits_type::eof())
    {
        if(std::basic_streambuf<CharT, TraitsT>::traits_type::eof() != c)
        {
            mData.push_back(static_cast<CharT>(c));
        }

        return c;
    }

    virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        (void)which;

        pos_type pos;

        if(std::ios_base::beg == dir)
        {
            pos = static_cast<pos_type>(off);
        }
        else if(std::ios_base::end == dir)
        {
            pos = static_cast<pos_type>(
                (egptr() - eback()) + off);
        }
        else // std::ios_base::cur == dir
        {
            pos = static_cast<pos_type>(
                (gptr() - eback()) + off);
        }

        if(static_cast<pos_type>(egptr() - eback()) < pos)
        {
            return pos_type(off_type(-1));
        }

        setg(eback(), eback() + pos, egptr());

        return pos;
    }

    virtual pos_type seekpos(pos_type pos,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        (void)which;

        if(static_cast<pos_type>(egptr() - eback()) < pos)
        {
            return pos_type(off_type(-1));
        }

        setg(eback(), eback() + pos, egptr());

        return pos;
    }
};

} // namespace libcomp

#endif // LIBCOMP_SRC_VECTORSTREAM_H
