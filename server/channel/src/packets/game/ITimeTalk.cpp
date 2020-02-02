/**
 * @file server/channel/src/packets/game/ITimeTalk.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to start or continue an I-Time conversation.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include "EventManager.h"

using namespace channel;

bool Parsers::ITimeTalk::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 2)
    {
        return false;
    }

    int8_t requestID = p.ReadS8();
    bool itemIncluded = p.ReadS8() == 1;

    int64_t itemID = -1;
    if(itemIncluded)
    {
        if(p.Left() < 8)
        {
            return false;
        }

        itemID = p.ReadS64Little();
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());

    server->QueueWork([](
        const std::shared_ptr<ChannelServer>& pServer,
        const std::shared_ptr<ChannelClientConnection> pClient,
        int8_t pRequestID, int64_t pItemID)
    {
        pServer->GetEventManager()->HandleResponse(pClient, pRequestID,
            pItemID);
    }, server, client, requestID, itemID);

    return true;
}
