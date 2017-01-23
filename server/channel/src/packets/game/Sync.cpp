/**
 * @file server/channel/src/packets/Sync.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to sync with the server time.
 *
 * This file is part of the Channel Server (channel).
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

#include "Packets.h"

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::Sync::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    state->SyncReceived();

    uint32_t timeFromClient = p.ReadU32Little();
    ServerTime currentServerTime = ChannelServer::GetServerTime();
    ClientTime currentClientTime = state->ToClientTime(currentServerTime);

    // Respond with the time received from the client (appears to be based off
    // the executing system time) then the amount of time elapsed since the
    // client state connection started.
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_SYNC_RESPONSE);
    reply.WriteU32Little(timeFromClient);
    reply.WriteFloat(currentClientTime);

    connection->SendPacket(reply);

    return true;
}
