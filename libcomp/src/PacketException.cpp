/**
 * @file libcomp/src/PacketException.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet exception class implmentation.
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

#include "PacketException.h"
#include "Log.h"

using namespace libcomp;

PacketException::PacketException(const String& msg,
    const ReadOnlyPacket *pPacket, const String& _file, int _line) :
    Exception(msg, _file, _line)
{
    // Copy the packet so it may be modified and read without changing the
    // original.
    if(pPacket && pPacket->mSize > 0)
    {
        mPacket.WriteArray(pPacket->mData, pPacket->mSize);
        mPacket.mPosition = pPacket->mPosition;
    }
}

Packet& PacketException::GetPacket()
{
    // Return the packet.
    return mPacket;
}

const Packet& PacketException::GetPacket() const
{
    // Return the packet.
    return mPacket;
}

void PacketException::Log() const
{
    // Log the exception and include a dump of the packet data.
    LOG_ERROR(String(
        "Packet exception at %1:%2\n"
        "========================================"
        "========================================\n"
        "%3\n"
        "----------------------------------------"
        "----------------------------------------\n"
        "Packet:\n"
        "%4\n"
        "\n"
        "Backtrace:\n"
        "%5\n"
        "========================================"
        "========================================\n"
    ).Arg(File()).Arg(Line()).Arg(Message()).Arg(
        GetPacket().Dump()).Arg(String::Join(Backtrace(), "\n")));
}
