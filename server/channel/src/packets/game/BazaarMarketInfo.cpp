/**
 * @file server/channel/src/packets/game/BazaarMarketInfo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request for details about a specific bazaar market.
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

// object Includes
#include <BazaarData.h>
#include <BazaarItem.h>
#include <EventInstance.h>
#include <EventState.h>
#include <Item.h>

// channel Includes
#include "BazaarState.h"
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::BazaarMarketInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto worldDB = server->GetWorldDatabase();

    auto eventState = state->GetEventState();
    auto currentEvent = eventState ? eventState->GetCurrent() : nullptr;
    uint32_t marketID = currentEvent ? currentEvent->GetShopID() : 0;
    auto zone = cState->GetZone();

    bool invalidMarket = marketID == 0 || zone == nullptr;
    if(!invalidMarket)
    {
        std::shared_ptr<channel::BazaarState> bState;
        std::shared_ptr<objects::BazaarData> market;
        for(auto b : zone->GetBazaars())
        {
            market = b->GetCurrentMarket(marketID);
            if(market)
            {
                bState = b;
                break;
            }
        }

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_INFO);

        invalidMarket = market == nullptr ||
            market->GetAccount().GetUUID() == state->GetAccountUID();
        if(!invalidMarket)
        {
            reply.WriteS32Little(0);    // Success

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                market->GetComment(), true);

            int32_t itemCount = 0;
            for(auto bItem : market->GetItems())
            {
                if(!bItem.IsNull())
                {
                    itemCount++;
                }
            }

            reply.WriteS32Little(itemCount);
            for(size_t i = 0; i < 15; i++)
            {
                // Since bazaars exist in exactly one zone at a time, the
                // items can be lazy loaded and we do not need to worry
                // about the market itself having been updated while active
                // without the work being done on the same channel
                auto bItem = market->GetItems(i).Get(worldDB);
                if(bItem)
                {
                    auto item = bItem->GetItem().Get(worldDB);

                    bool available = item && !bItem->GetSold();

                    reply.WriteS8((int8_t)i);

                    // It the item exists and is not registered to the client
                    // with a unique object ID, register it now so a purchase
                    // request can get the correct UUID
                    int64_t objectID = available ? state->GetObjectID(
                        bItem->GetItem().GetUUID()) : -1;
                    if(available && objectID <= 0)
                    {
                        objectID = server->GetNextObjectID();
                        state->SetObjectID(bItem->GetItem().GetUUID(),
                            objectID);
                    }

                    reply.WriteS64Little(objectID);
                    reply.WriteS32Little((int32_t)(available ? bItem->GetCost() : 0));

                    reply.WriteU32Little(bItem->GetType());
                    reply.WriteU16Little(bItem->GetStackSize());

                    characterManager->GetItemDetailPacketData(reply, available
                        ? item : nullptr, 1);
                }
            }
        }
        else
        {
            reply.WriteS32Little(-1);    // Failure
        }

        client->SendPacket(reply);
    }

    return true;
}
