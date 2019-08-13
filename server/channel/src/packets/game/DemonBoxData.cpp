/**
 * @file server/channel/src/packets/game/DemonBoxData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to return a demon in a demon box's data.
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
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <CharacterProgress.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::DemonBoxData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 10)
    {
        return false;
    }

    int8_t boxID = p.ReadS8();
    int8_t slot = p.ReadS8();
    int64_t demonID = p.ReadS64Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress();

    size_t maxSlots = boxID == 0 ? (size_t)progress->GetMaxCOMPSlots() : 50;
    if(slot < 0 || (size_t)slot >= maxSlots)
    {
        LogDemonError([&]()
        {
            return libcomp::String("Demon box slot exceeded the maximum "
                "available slots requested for demon data information.\n")
                .Arg(slot);
        });

        return false;
    }

    server->GetCharacterManager()->SendDemonData(client, boxID, slot, demonID);

    return true;
}
