/**
 * @file server/channel/src/packets/game/PopulateZone.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to populate a zone with objects
 *  and entities.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::PopulateZone::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    int32_t characterUID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    int32_t clientCharacterUID = client->GetClientState()->GetCharacterState()->GetEntityID();
    if(clientCharacterUID != characterUID)
    {
        LOG_ERROR(libcomp::String("Populate zone request sent with a character UID not matching"
            " the client connection.\nClient UID: %1\nPacket UID: %2\n").Arg(clientCharacterUID)
            .Arg(characterUID));
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    server->QueueWork([](ZoneManager* manager, const std::shared_ptr<ChannelClientConnection> c)
                    {
                        manager->SendPopulateZoneData(c);
                    }, server->GetZoneManager(), client);

    return true;
}
