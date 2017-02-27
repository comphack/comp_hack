/**
 * @file server/channel/src/packets/game/COMPSlotUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update a slot in the COMP.
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

// channel Includes
#include "ChannelServer.h"

// objects Includes
#include <Character.h>
#include <Demon.h>

using namespace channel;

void UpdateCOMPSlots(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int8_t compID, int64_t demonID, int8_t unknown, int8_t destSlot)
{
    (void)unknown;

    if(compID != 0)
    {
        //Only COMP supported currently
        return;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetCharacter().Get();
    auto srcDemon = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID)));
    
    int8_t srcSlot = -1;
    for(size_t i = 0; i < 10; i++)
    {
        if(character->GetCOMP(i).Get() == srcDemon)
        {
            srcSlot = (int8_t)i;
            break;
        }
    }

    if(srcSlot == -1)
    {
        return;
    }

    auto destDemon = character->GetCOMP((size_t)destSlot).Get();
    character->SetCOMP((size_t)srcSlot, destDemon);
    character->SetCOMP((size_t)destSlot, srcDemon);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_COMP_SLOT_UPDATED);
    reply.WriteS8(compID);

    reply.WriteS32Little(2);    //Slots updated
    server->GetCharacterManager()->GetCOMPSlotPacketData(reply, client, (size_t)srcSlot);
    server->GetCharacterManager()->GetCOMPSlotPacketData(reply, client, (size_t)destSlot);
    reply.WriteS8(10);   //Total COMP slots

    client->SendPacket(reply);
}

bool Parsers::COMPSlotUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 11)
    {
        return false;
    }

    int8_t compID = p.ReadS8();
    int64_t demonID = p.ReadS64Little();
    int8_t unknown = p.ReadS8();
    int8_t destSlot = p.ReadS8();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetCharacter().Get();

    if(state->GetObjectUUID(demonID).IsNull())
    {
        return false;
    }

    server->QueueWork(UpdateCOMPSlots, server, client, compID, demonID, unknown, destSlot);

    return true;
}
