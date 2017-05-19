/**
 * @file server/channel/src/packets/game/DismissDemon.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to dismiss a demon.
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

using namespace channel;

void DemonDismiss(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int64_t demonID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetEntity();
    auto demon = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID)));

    if(nullptr == demon)
    {
        return;
    }

    int8_t slot = -1;
    for(size_t i = 0; i < 10; i++)
    {
        if(character->GetCOMP(i).Get() == demon)
        {
            slot = (int8_t)i;
            break;
        }
    }

    if(slot == -1)
    {
        return;
    }

    if(dState->GetEntity() == demon)
    {
        server->GetCharacterManager()->StoreDemon(client);
    }

    character->SetCOMP((size_t)slot, NULLUUID);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_COMP_SLOT_UPDATED);
    reply.WriteS8(0);   //Unknown

    reply.WriteS32Little(1);    //Slots updated
    server->GetCharacterManager()->GetCOMPSlotPacketData(reply, client, (size_t)slot);
    reply.WriteS8(10);   //Total COMP slots

    client->SendPacket(reply);

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
    dbChanges->Update(character);
    dbChanges->Delete(demon->GetCoreStats().Get());
    dbChanges->Delete(demon);
    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
}

bool Parsers::DismissDemon::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    int64_t demonID = p.ReadS64Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    if(client->GetClientState()->GetObjectUUID(demonID).IsNull())
    {
        return false;
    }

    server->QueueWork(DemonDismiss, server, client, demonID);

    return true;
}
