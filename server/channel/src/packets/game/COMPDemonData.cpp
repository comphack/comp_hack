/**
 * @file server/channel/src/packets/game/COMPDemonData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to return a demon in the COMP's data.
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
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>
#include <Demon.h>
#include <EntityStats.h>
#include <InheritedSkill.h>
#include <StatusEffect.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

void SendCOMPDemonData(CharacterManager* characterManager,
    const std::shared_ptr<ChannelClientConnection>& client,
    int8_t box, int8_t slot, int64_t id)
{
    characterManager->SendCOMPDemonData(client, box, slot, id);
}

bool Parsers::COMPDemonData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 10)
    {
        return false;
    }

    int8_t box = p.ReadS8();
    int8_t slot = p.ReadS8();
    int64_t id = p.ReadS64Little();

    if(slot > 10)
    {
        LOG_ERROR(libcomp::String("Invalid COMP slot requested: %1\n")
            .Arg(slot));
        return false;
    }
    else if(box != 0)
    {
        LOG_ERROR("Non-COMP demon boxes are currently not supported.\n");
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>
        (pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);

    server->QueueWork(SendCOMPDemonData, server->GetCharacterManager(),
        client, box, slot, id);

    return true;
}
