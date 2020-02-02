/**
 * @file server/channel/src/packets/game/EventResponse.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Response from the client that a player response has occurred
 *  relative to the current event.
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

// channel Includes
#include "ChannelServer.h"
#include "EventManager.h"

using namespace channel;

bool Parsers::EventResponse::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    int32_t optionID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    
    server->QueueWork([](
        const std::shared_ptr<ChannelServer>& pServer,
        const std::shared_ptr<ChannelClientConnection> pClient,
        int32_t pOptionID)
    {
        pServer->GetEventManager()->HandleResponse(pClient, pOptionID);
    }, server, client, optionID);

    return true;
}
