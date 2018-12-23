/**
 * @file libcomp/src/MessagePacket.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet received message.
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

#include "MessagePacket.h"

// libcomp Includes
#include "TcpConnection.h"

using namespace libcomp;

Message::Packet::Packet(const std::shared_ptr<TcpConnection>& connection,
    uint16_t commandCode, ReadOnlyPacket& packet) : mPacket(packet),
    mCommandCode(commandCode), mConnection(connection)
{
}

Message::Packet::~Packet()
{
}

const ReadOnlyPacket& Message::Packet::GetPacket() const
{
    return mPacket;
}

uint16_t Message::Packet::GetCommandCode() const
{
    return mCommandCode;
}

std::shared_ptr<TcpConnection> Message::Packet::GetConnection() const
{
    return mConnection;
}

Message::MessageType Message::Packet::GetType() const
{
    return MessageType::MESSAGE_TYPE_PACKET;
}

libcomp::String Message::Packet::Dump() const
{
    if(mConnection)
    {
        return libcomp::String("Message: Packet\nConnection: %1\n"
            "Command Code: 0x%2\n%3").Arg(mConnection->GetName()).Arg(
            mCommandCode, 4, 16, '0').Arg(mPacket.Dump());
    }
    else
    {
        return libcomp::String("Message: Packet\nConnection: unknown\n"
            "Command Code: 0x%1\n%2").Arg(mCommandCode, 4, 16, '0').Arg(
            mPacket.Dump());
    }
}
