/**
 * @file server/channel/src/packets/game/DemonForceStack.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the clent to add or remove the pending force stack
 *  effect.
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
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"

using namespace channel;

bool Parsers::DemonForceStack::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 9)
    {
        return false;
    }

    int64_t demonID = p.ReadS64Little();

    bool toStack = p.ReadS8() == 1;
    int8_t stackSlot = -1;
    if(toStack)
    {
        if(p.Left() == 1)
        {
            stackSlot = p.ReadS8();
        }
        else
        {
            return false;
        }
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto definitionManager = server->GetDefinitionManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto dState = state->GetDemonState();
    auto devilData = dState->GetDevilData();
    auto demon = dState->GetEntity();
    uint16_t pendingEffect = demon ? demon->GetForceStackPending() : 0;

    auto dfExtra = definitionManager->GetDevilBoostExtraData(pendingEffect);
    
    bool success = dfExtra && demon &&
        state->GetObjectID(demon->GetUUID()) == demonID &&
        (!toStack || (stackSlot >= -1 && stackSlot <= 7));
    if(success)
    {
        if(stackSlot >= 0)
        {
            // Set the new value
            demon->SetForceStack((size_t)stackSlot, pendingEffect);
        }

        demon->SetForceStackPending(0);

        server->GetWorldDatabase()->QueueUpdate(demon,
            state->GetAccountUID());
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_FORCE_STACK);
    reply.WriteS64Little(demonID);
    reply.WriteS8(success ? 0 : -1);
    reply.WriteS8(stackSlot);
    if(stackSlot >= 0)
    {
        reply.WriteU16Little(pendingEffect);
    }

    client->SendPacket(reply);

    if(success)
    {
        server->GetTokuseiManager()->Recalculate(state->GetCharacterState(),
            true, std::set<int32_t>{ dState->GetEntityID() });
        server->GetCharacterManager()->RecalculateStats(dState, client);
    }

    return true;
}
