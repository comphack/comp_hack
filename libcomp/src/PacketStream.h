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

#ifndef LIBCOMP_SRC_PACKETSTREAM_H
#define LIBCOMP_SRC_PACKETSTREAM_H

// libcomp Includes
#include "Packet.h"
#include "ReadOnlyPacket.h"

// Standard C++11 Includes
#include <streambuf>

namespace libcomp
{

class PacketStream : public std::basic_streambuf<char>
{
private:
    Packet& mPacket;

public:
    PacketStream(Packet& p) : mPacket(p)
    {
        this->setg(p.Data(), p.Data(), p.Data() + p.Size());
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

class ReadOnlyPacketStream : public std::basic_streambuf<char>
{
public:
    ReadOnlyPacketStream(const ReadOnlyPacket& p)
    {
        this->setg(const_cast<char*>(p.ConstData()),
            const_cast<char*>(p.ConstData()),
            const_cast<char*>(p.ConstData() + p.Size()));
    }
};

} // namespace libcomp

#endif // LIBCOMP_SRC_PACKETSTREAM_H
