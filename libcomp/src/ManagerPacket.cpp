/**
 * @file libcomp/src/ManagerPacket.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to handle packets.
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

#include "ManagerPacket.h"

// libcomp Includes
#include "Log.h"
#include "MessagePacket.h"
#include "PacketParser.h"
#include "Packets.h"

using namespace libcomp;

std::list<libcomp::Message::MessageType> ManagerPacket::sSupportedTypes =
    { libcomp::Message::MessageType::MESSAGE_TYPE_PACKET };

ManagerPacket::ManagerPacket(std::weak_ptr<libcomp::BaseServer> server)
    : mServer(server)
{
}

ManagerPacket::~ManagerPacket()
{
}

std::list<libcomp::Message::MessageType>
    ManagerPacket::GetSupportedTypes() const
{
    return sSupportedTypes;
}

bool ManagerPacket::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    const libcomp::Message::Packet *pPacketMessage = dynamic_cast<
        const libcomp::Message::Packet*>(pMessage);

    if(nullptr != pPacketMessage)
    {
        libcomp::ReadOnlyPacket p(pPacketMessage->GetPacket());
        p.Rewind();
        //p.HexDump();

        CommandCode_t code = pPacketMessage->GetCommandCode();

        auto it = mPacketParsers.find(code);

        if(it == mPacketParsers.end())
        {
            LOG_ERROR(libcomp::String("Unknown packet with command code "
                "0x%1.\n").Arg(code, 4, 16, '0'));

            return false;
        }

        auto connection = pPacketMessage->GetConnection();
        if(!ValidateConnectionState(connection, code))
        {
            connection->Close();
            return false;
        }

        if(!it->second->Parse(this, connection, p))
        {
            connection->Close();
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }
}

std::shared_ptr<libcomp::BaseServer> ManagerPacket::GetServer()
{
    return mServer.lock();
}

bool ManagerPacket::ValidateConnectionState(const std::shared_ptr<
    libcomp::TcpConnection>& connection, CommandCode_t commandCode) const
{
    (void)connection;
    (void)commandCode;

    return true;
}

bool Parsers::Placeholder::Parse(ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    // DO NOT ACTUALLY USE
    // This is required so the packet parser class is not seen
    // as incomplete within libcomp.
    (void)pPacketManager;
    (void)connection;
    (void)p;

    return false;
}