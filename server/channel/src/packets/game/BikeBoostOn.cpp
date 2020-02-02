/**
 * @file server/channel/src/packets/game/BikeBoostOn.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to start boosting on a bike.
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
#include <ServerConstants.h>

// object Includes
#include <ServerZone.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::BikeBoostOn::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto zone = cState->GetZone();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BIKE_BOOST_ON);

    // Boost activateable if on a bike, not boosting already
    if(cState->Ready(true) && zone &&
        cState->StatusEffectActive(SVR_CONST.STATUS_BIKE) &&
        !cState->AdditionalTokuseiKeyExists(SVR_CONST.TOKUSEI_BIKE_BOOST))
    {
        if(!zone->GetDefinition()->GetBikeBoostEnabled())
        {
            // Boost not enabled for the zone
            reply.WriteS32Little(-3);
        }
        else
        {
            // Boost valid
            reply.WriteS32Little(0);

            state->SetBikeBoosting(true);
            cState->SetAdditionalTokusei(SVR_CONST.TOKUSEI_BIKE_BOOST, 1);

            server->GetTokuseiManager()->Recalculate(cState, true);

            // Hide/remove for other players if configured
            if(server->GetWorldSharedConfig()->GetBikeBoostHide())
            {
                cState->SetDisplayState(ActiveDisplayState_t::BIKE_BOOST);

                auto zoneManager = server->GetZoneManager();
                auto zConnections = zoneManager->GetZoneConnections(client,
                    false);

                zoneManager->RemoveEntities(zConnections,
                    { cState->GetEntityID() }, 17);
            }
        }
    }
    else
    {
        // Generic failure
        reply.WriteS32Little(-1);
    }

    client->SendPacket(reply);

    return true;
}
