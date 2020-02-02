/**
 * @file server/channel/src/packets/game/CultureData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the character's culture item data.
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
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <CultureData.h>
#include <Item.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"

using namespace channel;

bool Parsers::CultureData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto cultureData = character ? character->GetCultureData().Get() : nullptr;
    auto cultureItem = cultureData ? cultureData->GetItem().Get() : nullptr;

    bool active = cultureData && cultureData->GetActive();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CULTURE_DATA);

    reply.WriteS8(0);   // Success
    reply.WriteS32Little(active ? ChannelServer::GetExpirationInSeconds(
        cultureData->GetExpiration()) : 0);
    reply.WriteU32Little(active ? cultureItem->GetType()
        : static_cast<uint32_t>(-1));

    connection->SendPacket(reply);

    return true;
}
