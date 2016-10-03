/**
 * @file server/lobby/src/ManagerPacket.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to handle lobby packets.
 *
 * This file is part of the Lobby Server (lobby).
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

#include "ManagerPacket.h"

// libcomp Includes
#include "Log.h"
#include "MessagePacket.h"

// lobby Includes
#include "PacketParser.h"
#include "Packets.h"

using namespace lobby;

ManagerPacket::~ManagerPacket()
{
    mPacketParsers[0x0001] = std::shared_ptr<PacketParser>(new Parsers::Login());
}

std::list<libcomp::Message::MessageType>
    ManagerPacket::GetSupportedTypes() const
{
    std::list<libcomp::Message::MessageType> supportedTypes;

    supportedTypes.push_back(
        libcomp::Message::MessageType::MESSAGE_TYPE_PACKET);

    return supportedTypes;
}

bool ManagerPacket::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    const libcomp::Message::Packet *pPacketMessage = dynamic_cast<
        const libcomp::Message::Packet*>(pMessage);

    if(nullptr != pPacketMessage)
    {
        libcomp::ReadOnlyPacket p(pPacketMessage->GetPacket());
        p.Rewind();
        p.HexDump();

        CommandCode_t code = p.ReadU16Little();

        auto it = mPacketParsers.find(code);

        if(it == mPacketParsers.end())
        {
            LOG_ERROR(libcomp::String("Unknown packet with command code "
                "0x%1.\n").Arg(code, 8, 16, '0'));

            return false;
        }

        return it->second->Parse(this, p);
    }
    else
    {
        return false;
    }
}
