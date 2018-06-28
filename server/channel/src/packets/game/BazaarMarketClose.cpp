/**
 * @file server/channel/src/packets/game/BazaarMarketClose.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to close the player's current open bazaar market.
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
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <EventInstance.h>
#include <EventState.h>
#include <ServerZone.h>

// channel Includes
#include "BazaarState.h"
#include "ChannelServer.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::BazaarMarketClose::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto zone = cState->GetZone();

    auto bState = state->GetBazaarState();
    auto worldData = state->GetAccountWorldData().Get();
    auto bazaarData = worldData->GetBazaarData().Get();
    uint32_t marketID = bazaarData ? bazaarData->GetMarketID() : 0;

    bool success = false;
    if(bState && bState->GetCurrentMarket(marketID) == bazaarData)
    {
        bazaarData->SetState(objects::BazaarData::State_t::BAZAAR_INACTIVE);
        bState->SetCurrentMarket(marketID, nullptr);

        if(!bazaarData->Update(server->GetWorldDatabase()))
        {
            LOG_ERROR(libcomp::String("BazaarMarketClose failed to save: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            client->Kill();
            return true;
        }

        success = true;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_CLOSE);
    reply.WriteS32Little(success ? 0 : -1);    // Success

    client->SendPacket(reply);

    if(success)
    {
        reply.Clear();
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_CLOSE);
        reply.WriteS32Little(0);

        client->SendPacket(reply);

        server->GetZoneManager()->SendBazaarMarketData(zone, bState, marketID);
    }

    return true;
}
