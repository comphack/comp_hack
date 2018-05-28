/**
 * @file server/channel/src/packets/game/DemonLock.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to lock or unlock a demon in the COMP.
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

// objects Includes
#include <Character.h>
#include <Demon.h>

using namespace channel;

void DemonLockSet(const std::shared_ptr<ChannelClientConnection> client,
    int64_t demonID, bool lock)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto demon = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID)));

    if(nullptr == demon)
    {
        return;
    }

    demon->SetLocked(lock);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_LOCK);
    reply.WriteS64Little(demonID);
    reply.WriteS8(static_cast<int8_t>(lock ? 1 : 0));
    reply.WriteS8(0);   //Unknown

    client->SendPacket(reply);
}

bool Parsers::DemonLock::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 9)
    {
        return false;
    }

    int64_t demonID = p.ReadS64Little();
    int8_t lock = p.ReadS8();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    if(client->GetClientState()->GetObjectUUID(demonID).IsNull())
    {
        return false;
    }

    server->QueueWork(DemonLockSet, client, demonID, lock == 1);

    return true;
}
