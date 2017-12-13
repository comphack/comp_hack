/**
 * @file server/channel/src/packets/game/ItemPrice.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to estimate the price of an item
 *  based upon several criteria.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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
#include <DefinitionManager.h>
#include <ManagerPacket.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>

// objects Includes
#include <MiItemData.h>
#include <MiItemBasicData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ItemPrice::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    LOG_WARNING("In ItemPrice\n");

    if(p.Size() != 42)
    {
        return false;
    }

    int32_t requestID = p.ReadS32Little();
    uint32_t itemType = p.ReadU32Little();
    uint16_t unknown = p.ReadU16Little();
    uint16_t durability = p.ReadU16Little();
    int8_t maxDurability = p.ReadS8();
    (void)unknown;
    (void)durability;
    (void)maxDurability;

    /// @todo: there is a lot more to a request

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto definitionManager = server->GetDefinitionManager();

    auto itemData = definitionManager->GetItemData(itemType);

    uint32_t price = 0;
    if(itemData)
    {
        // Price defaults to 100 times the normal purchase price if it were
        // to show up in stores and gets modified various aspects
        /// @todo: factor in trends and item modifications
        price = (uint32_t)(itemData->GetBasic()->GetBuyPrice() * 100);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_PRICE);
    reply.WriteS32Little(requestID);
    reply.WriteU32Little(price);
    reply.WriteS32Little(price != 0 ? 0 : -1);

    connection->SendPacket(reply);

    return true;
}
