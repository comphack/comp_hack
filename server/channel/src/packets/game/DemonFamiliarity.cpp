/**
 * @file server/channel/src/packets/game/DemonFamiliarity.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to refresh demon familiarity info
 *  for all demons in the COMP.
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
#include <DemonBox.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::DemonFamiliarity::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    std::list<std::shared_ptr<objects::Demon>> demons;
    for(auto d : character->GetCOMP()->GetDemons())
    {
        if(!d.IsNull())
        {
            demons.push_back(d.Get());
        }
    }
    
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_FAMILIARITY);
    reply.WriteS8((int8_t)demons.size());
    for(auto demon : demons)
    {
        reply.WriteS64Little(state->GetObjectID(demon->GetUUID()));
        reply.WriteU16Little(demon->GetFamiliarity());
    }

    client->SendPacket(reply);

    return true;
}
