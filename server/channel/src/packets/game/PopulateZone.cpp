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

    int32_t entityID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    int32_t cEntityID = state->GetCharacterState()->GetEntityID();

    if(entityID != cEntityID)
    {
        LogZoneManagerError([&]()
        {
            return libcomp::String("PopulateZone request sent with an"
                " entity ID not matching the client connection: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        client->Close();
        return true;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    server->QueueWork([](ZoneManager* manager,
        const std::shared_ptr<ChannelClientConnection> pClient)
                {
                    if(!manager->SendPopulateZoneData(pClient))
                    {
                        auto pState = pClient->GetClientState();
                        auto uuid = pState ? pState->GetAccountUID() : NULLUUID;

                        LogZoneManagerError([&]()
                        {
                            return libcomp::String("PopulateZone response"
                                " failed to send data for account: %1\n")
                                .Arg(uuid.ToString());
                        });
                    }
                }, server->GetZoneManager(), client);

    return true;
}
