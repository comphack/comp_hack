/**
 * @file server/channel/src/packets/game/DemonFusion.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to fuse a new demon.
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
#include <EventInstance.h>
#include <EventOpenMenu.h>
#include <EventState.h>

// channel Includes
#include "ChannelServer.h"
#include "FusionManager.h"

using namespace channel;

bool Parsers::DemonFusion::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 28)
    {
        return false;
    }

    int32_t fusionType = p.ReadS32Little();
    int64_t demonID1 = p.ReadS64Little();
    int64_t demonID2 = p.ReadS64Little();
    int64_t unknown = p.ReadS64Little();
    (void)fusionType;
    (void)unknown;

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto eState = state->GetEventState();
    auto current = eState ? eState->GetCurrent() : nullptr;

    uint32_t costItemType = SVR_CONST.ITEM_MACCA;

    // If the current menu the player is interacting with is the kreuz fusion
    // menu, update the cost item type
    if(current)
    {
        auto menu = std::dynamic_pointer_cast<objects::EventOpenMenu>(
            current->GetEvent());
        if(menu && (uint32_t)menu->GetMenuType() == SVR_CONST.MENU_FUSION_KZ)
        {
            costItemType = SVR_CONST.ITEM_KREUZ;
        }
    }

    server->QueueWork([](const std::shared_ptr<ChannelServer> pServer,
        const std::shared_ptr<ChannelClientConnection> pClient,
        int64_t pDemonID1, int64_t pDemonID2, uint32_t pCostItemType)
        {
            pServer->GetFusionManager()->HandleFusion(pClient, pDemonID1,
                pDemonID2, pCostItemType);
        }, server, client, demonID1, demonID2, costItemType);

    return true;
}
