/**
 * @file server/channel/src/packets/game/UnionFlag.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to receive union information.
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
#include <Constants.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

// objects Includes
#include <Character.h>
#include <EntityStats.h>

using namespace channel;

bool Parsers::UnionFlag::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    auto entityID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    /// @todo: Properly implement
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_UNION_FLAG);
    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteU16Little(20);
    reply.WriteBlank(20);

    client->SendPacket(reply);

    return true;
}
