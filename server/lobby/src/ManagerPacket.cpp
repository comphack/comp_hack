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

ManagerPacket::ManagerPacket(PacketManagerMode mode, std::shared_ptr<libcomp::BaseServer> server)
{
    mServer = server;

    if(mode == PacketManagerMode::MANAGER_CLIENT_FACING)
    {
        mPacketParsers[0x0003] = std::shared_ptr<PacketParser>(
            new Parsers::Login());
        mPacketParsers[0x0005] = std::shared_ptr<PacketParser>(
            new Parsers::Auth());
        mPacketParsers[0x0007] = std::shared_ptr<PacketParser>(
            new Parsers::StartGame());
        mPacketParsers[0x0009] = std::shared_ptr<PacketParser>(
            new Parsers::CharacterList());
        mPacketParsers[0x000B] = std::shared_ptr<PacketParser>(
            new Parsers::WorldList());
        mPacketParsers[0x000D] = std::shared_ptr<PacketParser>(
            new Parsers::CreateCharacter());
        mPacketParsers[0x000F] = std::shared_ptr<PacketParser>(
            new Parsers::DeleteCharacter());
        mPacketParsers[0x0011] = std::shared_ptr<PacketParser>(
            new Parsers::QueryPurchaseTicket());
        mPacketParsers[0x0013] = std::shared_ptr<PacketParser>(
            new Parsers::PurchaseTicket());
    }
    else
    {
        mPacketParsers[0x1001] = std::shared_ptr<PacketParser>(
            new Parsers::SetWorldDescription());
        mPacketParsers[0x1002] = std::shared_ptr<PacketParser>(
            new Parsers::SetChannelDescription());
    }
}

ManagerPacket::~ManagerPacket()
{
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

        CommandCode_t code = pPacketMessage->GetCommandCode();

        auto it = mPacketParsers.find(code);

        if(it == mPacketParsers.end())
        {
            LOG_ERROR(libcomp::String("Unknown packet with command code "
                "0x%1.\n").Arg(code, 4, 16, '0'));

            return false;
        }

        return it->second->Parse(this, pPacketMessage->GetConnection(), p);
    }
    else
    {
        return false;
    }
}

std::shared_ptr<libcomp::BaseServer> ManagerPacket::GetServer()
{
    return mServer;
}