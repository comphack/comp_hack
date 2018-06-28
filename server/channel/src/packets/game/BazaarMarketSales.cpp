/**
 * @file server/channel/src/packets/game/BazaarMarketSales.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to take sales macca from the player's bazaar market.
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
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// objects Includes
#include <BazaarData.h>
#include <BazaarItem.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::BazaarMarketSales::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 6)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    int8_t fromSlot = p.ReadS8();
    int32_t amount = p.ReadS32Little();
    int8_t toSlot = p.ReadS8();

    // Ignore the "to slot" so macca compression can be done
    (void)toSlot;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto inventory = cState->GetEntity()->GetItemBoxes(0);
    auto worldDB = server->GetWorldDatabase();

    // Always reload
    auto bazaarData = objects::BazaarData::LoadBazaarDataByAccount(worldDB,
        state->GetAccountUID());
    auto bItems = objects::BazaarItem::LoadBazaarItemListByAccount(worldDB,
        state->GetAccountUID());

    auto bItem = bazaarData->GetItems((size_t)fromSlot).Get();

    bool success = false;
    if(bItem && amount > 0)
    {
        int64_t newCostPaid = (int64_t)bItem->GetCost() - (int64_t)amount;
        if(newCostPaid >= 0)
        {
            std::unordered_map<uint32_t, uint32_t> itemCounts;

            uint32_t macca = (uint32_t)(amount % (int32_t)ITEM_MACCA_NOTE_AMOUNT);
            uint32_t notes = (uint32_t)floorl((double)(amount - (int32_t)macca) /
                (double)ITEM_MACCA_NOTE_AMOUNT);

            itemCounts[SVR_CONST.ITEM_MACCA] = macca;
            itemCounts[SVR_CONST.ITEM_MACCA_NOTE] = notes;

            success = server->GetCharacterManager()->AddRemoveItems(client,
                itemCounts, true);
        }

        if(success)
        {
            // Decrease the cost paid still on the item, remove if 0
            auto dbChanges = libcomp::DatabaseChangeSet::Create();

            bItem->SetCost((uint32_t)newCostPaid);
            if(newCostPaid == 0)
            {
                bazaarData->SetItems((size_t)fromSlot, NULLUUID);

                dbChanges->Update(bazaarData);
                dbChanges->Delete(bItem);
            }
            else
            {
                dbChanges->Update(bItem);
            }

            if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
            {
                LOG_ERROR(libcomp::String("BazaarMarketSales failed to save: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                client->Kill();
                return true;
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_SALES);
    reply.WriteS8(fromSlot);
    reply.WriteS32Little(amount);
    reply.WriteS32Little(success ? 0 : -1);

    client->SendPacket(reply);

    return true;
}
