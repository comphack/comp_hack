/**
 * @file server/channel/src/packets/game/DemonReunion.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the clent to reunion the summoned partner demon.
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
#include "CharacterManager.h"

using namespace channel;

bool Parsers::DemonReunion::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 13)
    {
        return false;
    }

    int64_t demonID = p.ReadS64Little();
    uint8_t growthType = p.ReadU8();
    uint32_t costItemType = p.ReadU32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);

    server->QueueWork([](const std::shared_ptr<ChannelServer> pServer,
        const std::shared_ptr<ChannelClientConnection> pClient,
        int64_t pDemonID, uint8_t pGrowthType, uint32_t pCostItemType)
        {
            pServer->GetCharacterManager()->ReunionDemon(pClient,
                pDemonID, pGrowthType, pCostItemType, true);
        }, server, client, demonID, growthType, costItemType);

    return true;
}
