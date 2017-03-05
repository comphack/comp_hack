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
#include <StatusEffect.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

void SendCOMPList(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int8_t unknown)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
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

    auto characterManager = server->GetCharacterManager();
    for(size_t i = 0; i < 10; i++)
    {
        if(comp[i].IsNull()) continue;

        characterManager->GetCOMPSlotPacketData(reply, client, i);
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

    server->QueueWork(SendCOMPList, server, client, unknown);

    return true;
}
