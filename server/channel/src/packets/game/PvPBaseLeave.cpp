/**
 * @file server/channel/src/packets/game/PvPBaseLeave.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to stop capturing a PvP base. This packet
 *  also serves the secondary purpose of being a "match over" indicator that
 *  sends when the results screen is closed.
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

// object Includes
#include <Match.h>

 // channel Includes
#include "ChannelServer.h"
#include "EventManager.h"
#include "MatchManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::PvPBaseLeave::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());

    int32_t baseID = state->GetEventSourceEntityID();
    server->GetMatchManager()->LeaveBase(client, baseID);

    auto zone = state->GetZone();
    if(zone && MatchManager::InPvPTeam(state->GetCharacterState()) &&
        !MatchManager::PvPActive(zone->GetInstance()))
    {
        // Match is over and this should be treated like the "end
        // confirmation" request so trigger complete actions
        server->GetZoneManager()->TriggerZoneActions(zone,
            { state->GetCharacterState() }, ZoneTrigger_t::ON_PVP_COMPLETE,
            client);
    }

    return true;
}
