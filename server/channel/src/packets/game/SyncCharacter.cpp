/**
 * @file server/channel/src/packets/game/SyncCharacter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to sync the player character data. This
 *  happens if the client expected an expiration to take place that the
 *  server never sent.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::SyncCharacter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto cs = cState->GetCoreStats();

    if(cState->GetEntityID() != entityID)
    {
        LOG_ERROR(libcomp::String("Character not belonging to the client"
            " requested for SyncCharacter: %1\n").Arg(entityID));
        return true;
    }

    auto statusEffects = cState->GetCurrentStatusEffectStates(
        server->GetDefinitionManager());

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SYNC_CHARACTER);
    reply.WriteS32Little(entityID);
    reply.WriteS16Little(cs->GetHP());
    reply.WriteS16Little(cs->GetMP());
    reply.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
    for (auto ePair : statusEffects)
    {
        reply.WriteU32Little(ePair.first->GetEffect());
        reply.WriteS32Little((int32_t)ePair.second);
        reply.WriteU8(ePair.first->GetStack());
    }

    // Send back to the whole zone just in case
    server->GetZoneManager()->BroadcastPacket(client, reply);

    return true;
}
