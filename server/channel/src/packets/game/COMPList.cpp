/**
 * @file server/channel/src/packets/game/COMPList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to return the COMP's demon list.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>
#include <Demon.h>
#include <EntityStats.h>
#include <StatusEffect.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

void SendCOMPList(const std::shared_ptr<ChannelClientConnection>& client,
    int8_t unknown)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetCharacter();
    auto comp = character->GetCOMP();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_COMP_LIST);

    int32_t count = 0;

    for(size_t i = 0; i < 10; i++)
    {
        count += !comp[i].IsNull() ? 1 : 0;
    }

    reply.WriteS8(unknown);
    reply.WriteS32Little(0);   //Unknown
    reply.WriteS32Little(-1);   //Unknown
    reply.WriteS32Little(count);

    for(size_t i = 0; i < 10; i++)
    {
        auto demon = comp[i];
        if(comp[i].IsNull()) continue;

        auto cs = demon->GetCoreStats();

        reply.WriteS8(static_cast<int8_t>(i)); // Slot
        reply.WriteS64Little(
            state->GetObjectID(demon->GetUUID()));
        reply.WriteU32Little(demon->GetType());
        reply.WriteS16Little(cs->GetMaxHP());
        reply.WriteS16Little(cs->GetMaxMP());
        reply.WriteS16Little(cs->GetHP());
        reply.WriteS16Little(cs->GetMP());
        reply.WriteS8(cs->GetLevel());
        reply.WriteU8(demon->GetLocked() ? 1 : 0);

        size_t statusEffectCount = demon->StatusEffectsCount();
        reply.WriteS32Little(static_cast<int32_t>(statusEffectCount));
        for(auto effect : demon->GetStatusEffects())
        {
            reply.WriteU32Little(effect->GetEffect());
        }

        reply.WriteS8(0);   //Unknown

        //Epitaph/Mitama fusion flag
        reply.WriteS8(0);

        //Effect length in seconds
        reply.WriteS32Little(0);
        reply.WriteU8(0);   //Unknown
    }

    reply.WriteU8(10);  //Total COMP slots

    client->SendPacket(reply);
}

bool Parsers::COMPList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    int8_t unknown = p.ReadS8();    //Demon container? Is this ever not 0 for COMP?

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    server->QueueWork(SendCOMPList, client, unknown);

    return true;
}
