/**
 * @file server/channel/src/packets/game/BazaarItemDrop.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to drop an item from the player's bazaar market. Item
 *  drops can be categorized into two types:
 *  1) On-site drops performed at the bazaar market itself
 *  2) Remote drops performed anywhere else
 *  If the player is on-site, items can be dropped at any point. If not
 *  items can only be dropped if the market is not currently active.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// objects Includes
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::BazaarItemDrop::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 10)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int8_t srcSlot = p.ReadS8();
    int64_t itemID = p.ReadS64Little();
    int8_t destSlot = p.ReadS8();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_ITEM_DROP);
    reply.WriteS8(srcSlot);
    reply.WriteS64Little(itemID);
    reply.WriteS8(destSlot);

    auto dbChanges = libcomp::DatabaseChangeSet::Create();
    if(channel::BazaarState::DropItemFromMarket(state, srcSlot, itemID, destSlot, dbChanges))
    {
        if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
        {
            LogBazaarError([&]()
            {
                return libcomp::String("BazaarItemDrop failed to save: %1\n")
                    .Arg(state->GetAccountUID().ToString());
            });

            client->Kill();
            return true;
        }

        auto cState = state->GetCharacterState();
        auto character = cState->GetEntity();
        auto inventory = character->GetItemBoxes(0).Get();

        std::list<uint16_t> updatedSlots = { (uint16_t)destSlot };
        server->GetCharacterManager()->SendItemBoxData(client, inventory,
            updatedSlots);

        reply.WriteS32Little(0);    // Success
    }
    else
    {
        reply.WriteS32Little(-1);   // Failure
    }

    client->SendPacket(reply);

    return true;
}
