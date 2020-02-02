/**
 * @file server/channel/src/packets/game/UBProceed.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to proceed past a UB round result display.
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

// objects Includes
#include <UBMatch.h>

// channel Includes
#include "ChannelServer.h"
#include "MatchManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::UBProceed::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    uint32_t matchSubType = p.ReadU32Little();
    (void)matchSubType; // Just check the match state

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto zone = state->GetZone();

    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    if(!ubMatch || ubMatch->GetState() == objects::UBMatch::State_t::COMPLETE)
    {
        auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
            ->GetServer());
        auto zoneManager = server->GetZoneManager();

        zoneManager->MoveToLobby(client);
    }

    return true;
}
