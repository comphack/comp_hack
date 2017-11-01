/**
 * @file server/channel/src/packets/game/BazaarItemBuy.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to buy an item from a bazaar market.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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
#include <BazaarItem.h>
#include <EventInstance.h>
#include <EventState.h>
#include <Item.h>
#include <ItemBox.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::BazaarItemBuy::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 13)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto zone = cState->GetZone();

    int8_t slot = p.ReadS8();
    int64_t itemID = p.ReadS64Little();
    int32_t price = p.ReadS32Little();

    auto currentEvent = state->GetEventState()->GetCurrent();
    uint32_t marketID = currentEvent ? currentEvent->GetShopID() : 0;
    auto bState = currentEvent
        ? zone->GetBazaar(currentEvent->GetSourceEntityID()) : nullptr;

    // Load from the DB as the seller may not be on the channel so caching
    // isn't guaranteed
    auto item = libcomp::PersistentObject::LoadObjectByUUID<objects::Item>(
        server->GetWorldDatabase(), state->GetObjectUUID(itemID));

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_ITEM_BUY);
    reply.WriteS8(slot);
    reply.WriteS64Little(itemID);
    reply.WriteS32Little(price);

    std::shared_ptr<objects::BazaarItem> bItem;

    int32_t errorCode = -1; // Generic error

    bool success = false;
    if(bState && item)
    {
        auto character = cState->GetEntity();
        auto inventory = character->GetItemBoxes(0).Get();

        uint64_t totalMacca = server->GetCharacterManager()->GetTotalMacca(character);

        int8_t destSlot = -1;
        bItem = bState->TryBuyItem(state, marketID, slot, itemID, price);
        if(bItem)
        {
            // Since the bazaar purchase response is so particular about format
            // auto stacking is NOT supported for this, find an empty slot
            for(size_t i = 0; i < 50; i++)
            {
                if(inventory->GetItems(i).IsNull())
                {
                    destSlot = (int8_t)i;
                    break;
                }
            }
        }

        if(totalMacca < (uint64_t)bItem->GetCost())
        {
            // No new error code for this one
        }
        else if(destSlot == -1)
        {
            errorCode = -2; // Not enough space
        }
        else if(bState->BuyItem(bItem))
        {
            if(!server->GetCharacterManager()->PayMacca(client, (uint64_t)bItem->GetCost()))
            {
                // Undo sale
                bItem->SetSold(false);
            }
            else
            {
                auto dbChanges = libcomp::DatabaseChangeSet::Create();

                inventory->SetItems((size_t)destSlot, item);
                item->SetItemBox(inventory);
                item->SetBoxSlot(destSlot);

                dbChanges->Update(bItem);
                dbChanges->Update(inventory);
                dbChanges->Update(item);

                if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
                {
                    LOG_ERROR(libcomp::String("BazaarItemBuy failed to save: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                    state->SetLogoutSave(false);
                    client->Close();
                    return true;
                }

                reply.WriteS8(destSlot);
                reply.WriteS32Little(0);    // Success
                success = true;

                std::list<uint16_t> updatedSlots = { (uint16_t)destSlot };
                server->GetCharacterManager()->SendItemBoxData(client,
                    inventory, updatedSlots);
            }
        }
    }

    if(!success)
    {
        reply.WriteS8(-1);
        reply.WriteS32Little(errorCode);
    }
    else
    {
        libcomp::Packet relay;
        relay.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
        relay.WriteS32Little(state->GetWorldCID());
        relay.WriteU8((uint8_t)PacketRelayMode_t::RELAY_ACCOUNT);
        relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
            bItem->GetAccount().GetUUID().ToString(), true);

        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_ITEM_SOLD);
        relay.WriteS8(slot);
        relay.WriteS8(2); // Sold
        relay.WriteFloat(0.f);  // Unknown
        relay.WriteS64Little(-1);

        relay.WriteS32Little((int32_t)bItem->GetCost());
        relay.WriteU32Little(bItem->GetType());
        relay.WriteU16Little(bItem->GetStackSize());

        relay.WriteU16Little(item->GetDurability());
        relay.WriteS8(item->GetMaxDurability());

        relay.WriteS16Little(item->GetTarot());
        relay.WriteS16Little(item->GetSoul());

        for(auto modSlot : item->GetModSlots())
        {
            relay.WriteU16Little(modSlot);
        }

        relay.WriteS32Little(0);    // Unknown

        auto basicEffect = item->GetBasicEffect();
        relay.WriteU32Little(basicEffect ? basicEffect
            : static_cast<uint32_t>(-1));

        auto specialEffect = item->GetSpecialEffect();
        relay.WriteU32Little(specialEffect ? specialEffect
            : static_cast<uint32_t>(-1));

        for(auto bonus : item->GetFuseBonuses())
        {
            relay.WriteS8(bonus);
        }

        // Purchased by
        relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
            cState->GetEntity()->GetName(), true);

        server->GetManagerConnection()->GetWorldConnection()->SendPacket(relay);
    }

    client->SendPacket(reply);

    return true;
}
