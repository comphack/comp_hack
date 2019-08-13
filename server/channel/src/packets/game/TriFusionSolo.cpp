/**
 * @file server/channel/src/packets/game/TriFusionSolo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to perform a solo tri-fusion.
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

// channel Includes
#include "ChannelServer.h"
#include "FusionManager.h"

using namespace channel;

bool Parsers::TriFusionSolo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 31)
    {
        return false;
    }

    int32_t fusionType = p.ReadS32Little();
    int64_t demonID1 = p.ReadS64Little();
    int64_t demonID2 = p.ReadS64Little();
    int64_t demonID3 = p.ReadS64Little();
    uint16_t fusionItemType = p.ReadU16Little();
    uint8_t unknown = p.ReadU8();
    (void)fusionType;
    (void)unknown;

    if(fusionItemType != 1)
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Invalid solo TriFusion item type"
                " supplied: %1\n").Arg(fusionItemType);
        });

        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    server->QueueWork([](const std::shared_ptr<ChannelServer> pServer,
        const std::shared_ptr<ChannelClientConnection> pClient,
        int64_t pDemonID1, int64_t pDemonID2, int64_t pDemonID3)
        {
            pServer->GetFusionManager()->HandleTriFusion(pClient, pDemonID1,
                pDemonID2, pDemonID3, true);
        }, server, client, demonID1, demonID2, demonID3);

    return true;
}
