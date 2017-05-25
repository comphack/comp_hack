/**
 * @file server/channel/src/packets/game/ItemBox.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for info about a specific item box.
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
#include <Item.h>
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

void SendItemBox(CharacterManager* characterManager,
    const std::shared_ptr<ChannelClientConnection>& client, int64_t boxID)
{
    characterManager->SendItemBoxData(client, boxID);
}

bool Parsers::ItemBox::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 9)
    {
        return false;
    }

    int8_t type = p.ReadS8();
    int64_t boxID = p.ReadS64Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    if(type == 0 && boxID == 0)
    {
        server->QueueWork(SendItemBox, server->GetCharacterManager(), client, boxID);
    }
    else
    {
        /// @todo
        LOG_ERROR("Item box request sent for a non-inventory item box.\n");
        return true;
    }

    return true;
}
