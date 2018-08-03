/**
 * @file server/channel/src/packets/game/ITimeData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the character's I-Time data.
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

// object Includes
#include <CharacterProgress.h>

// channel Includes
#include "ChannelClientConnection.h"

using namespace channel;

bool Parsers::ITimeData::Parse(libcomp::ManagerPacket *pPacketManager,
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
    auto character = cState->GetEntity();
    auto progress = character ? character->GetProgress().Get() : nullptr;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITIME_DATA);

    reply.WriteS8(progress ? 0 : -1);

    if(progress)
    {
        reply.WriteS8((int8_t)progress->ITimePointsCount());
        for(auto& pair : progress->GetITimePoints())
        {
            reply.WriteS8(pair.first);  // NPC ID
            reply.WriteS16Little(pair.second);  // Points
        }
    }

    client->SendPacket(reply);

    return true;
}
