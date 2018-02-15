/**
 * @file server/channel/src/packets/game/VAChange.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to change the character's VA.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::VAChange::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 8)
    {
        return false;
    }

    int32_t unused = p.ReadS32Little();
    int32_t changeCount = p.ReadS32Little();
    (void)unused;

    if(p.Left() != (uint32_t)(changeCount * 5))
    {
        return false;
    }

    std::list<std::pair<int8_t, uint32_t>> changeMap;
    for(int32_t i = 0; i < changeCount; i++)
    {
        int8_t slot = p.ReadS8();
        uint32_t itemType = p.ReadU32Little();

        changeMap.push_back(std::pair<int8_t, uint32_t>(slot, itemType));
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_VA_CHANGE);
    reply.WriteS32Little(0);    // Success
    reply.WriteS32Little(0);

    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_VA_CHANGED);
    notify.WriteS32Little(cState->GetEntityID());

    reply.WriteS32Little((int32_t)changeMap.size());
    notify.WriteS32Little((int32_t)changeMap.size());
    for(auto pair : changeMap)
    {
        if(pair.second == static_cast<uint32_t>(-1))
        {
            character->RemoveEquippedVA((uint8_t)pair.first);
        }
        else
        {
            character->SetEquippedVA((uint8_t)pair.first, pair.second);
        }

        reply.WriteS8(pair.first);
        reply.WriteS32Little((int32_t)pair.second);

        notify.WriteS8(pair.first);
        notify.WriteS32Little((int32_t)pair.second);
    }

    client->SendPacket(reply);

    server->GetZoneManager()->BroadcastPacket(client, notify, false);

    server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

    return true;
}
