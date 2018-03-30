/**
 * @file libcomp/src/PacketStream.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to use a Packet as a stream.
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

#ifndef LIBCOMP_SRC_PACKETSTREAM_H
#define LIBCOMP_SRC_PACKETSTREAM_H

// libcomp Includes
#include "Packet.h"
#include "ReadOnlyPacket.h"

// Standard C++11 Includes
#include <streambuf>

namespace libcomp
{

/**
 * Abstract stream representing a data to be read from a packet.
 * The seekoff and seekpos overrides have been implemented so tellg
 * works properly when used in an istream.
 */
class BasicPacketStream : public std::basic_streambuf<char>
{
public:
    /**
     * Create the basic stream.
     */
    BasicPacketStream() { };
    
    /**
     * Clean up the basic stream.
     */
    virtual ~BasicPacketStream() { };

protected:
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

/**
 * Stream representing data written to a packet to be read.
 */
class PacketStream : public BasicPacketStream
{
private:
    /// Packet that the data came from
    Packet& mPacket;

public:
    /**
     * Create the stream and set its data from the packet.
     * @param p Packet to read the data from.
     */
    PacketStream(Packet& p) : BasicPacketStream(), mPacket(p)
    {
        this->setg(p.Data(),
            p.Data() + p.Tell(),
            p.Data() + p.Size());
    }

protected:
    virtual typename std::basic_streambuf<char>::int_type overflow(
        typename std::basic_streambuf<char>::int_type c =
            std::basic_streambuf<char>::traits_type::eof())
    {
        if(0 == mPacket.Free() && 0 == mPacket.Left())
        {
            return std::basic_streambuf<char>::traits_type::eof();
        }

        if(std::basic_streambuf<char>::traits_type::eof() != c)
        {
            mPacket.WriteS8(static_cast<char>(c));
        }

        return c;
    }
};

/**
 * Stream representing data written to a read only packet to be read.
 */
class ReadOnlyPacketStream : public BasicPacketStream
{
public:
    /**
     * Create the stream and set its data from the packet.
     * @param p Packet to read the data from.
     */
    ReadOnlyPacketStream(const ReadOnlyPacket& p) : BasicPacketStream()
    {
        this->setg(const_cast<char*>(p.ConstData()),
            const_cast<char*>(p.ConstData() + p.Tell()),
            const_cast<char*>(p.ConstData() + p.Size()));
    }
};

} // namespace libcomp

#endif // LIBCOMP_SRC_PACKETSTREAM_H
