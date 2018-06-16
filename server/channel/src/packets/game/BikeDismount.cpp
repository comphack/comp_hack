/**
 * @file server/channel/src/packets/game/BikeDismount.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to dismount from a bike.
 *
 * This file is part of the Channel Server (channel).
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

#include "Packets.h"

// libcomp Includes
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::BikeDismount::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BIKE_DISMOUNT);
    reply.WriteS32Little(0);

    client->SendPacket(reply);

    // Just expire the status, no need to verify that its there
    std::set<uint32_t> effects = { SVR_CONST.STATUS_BIKE };
    cState->ExpireStatusEffects(effects);

    return true;
}
