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

// object Includes
#include <BazaarItem.h>
#include <EventInstance.h>
#include <EventState.h>
#include <Item.h>
#include <ItemBox.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ManagerConnection.h"

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
    auto characterManager = server->GetCharacterManager();
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto zone = cState->GetZone();

    int8_t slot = p.ReadS8();
    int64_t itemID = p.ReadS64Little();
    int32_t price = p.ReadS32Little();

    auto currentEvent = state->GetEventState()->GetCurrent();
    uint32_t marketID = (uint32_t)state->GetCurrentMenuShopID();
    auto bState = currentEvent && zone
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
        auto market = bState->GetCurrentMarket(marketID);

        uint64_t totalMacca = characterManager->GetTotalMacca(character);

        std::set<size_t> freeSlots;
        bItem = bState->TryBuyItem(state, marketID, slot, itemID, price);
        if(bItem)
        {
            // Since the bazaar purchase response is so particular about format
            // auto stacking is NOT supported for this, find empty slot(s)
            freeSlots = characterManager->GetFreeSlots(client, inventory);
        }

        if(!bItem || totalMacca < (uint64_t)bItem->GetCost())
        {
            // No new error code for these
        }
        else if(freeSlots.size() == 0)
        {
            // No free slots
            errorCode = -2;
        }
        else if(freeSlots.size() == 1 && characterManager->GetExistingItemCount(
                character, SVR_CONST.ITEM_MACCA, inventory) < bItem->GetCost())
        {
            // If there is only one free space and not enough macca coins to
            // cover the cost, there is not enough space. Macca notes will be
            // split to pay bazaar costs but the new stack can fill the
            // inventory as well.
            errorCode = -2;

            auto accountUID = state->GetAccountUID();
            LogBazaarError([accountUID]()
                {
                    return libcomp::String("BazaarItemBuy failed due to"
                        " required macca splitting without enough space"
                        " available: %1\n").Arg(accountUID.ToString());
                });
        }
        else if(bState->BuyItem(bItem))
        {
            uint64_t cost = (uint64_t)bItem->GetCost();
            if(!characterManager->PayMacca(client, cost))
            {
                // Undo sale
                bItem->SetSold(false);
            }
            else
            {
                // Do not fail the item update at this point. Default to the
                // first free slot but grab the new free slots following the
                // payment. If all else fails, put the item in the box so
                // relogging will recover the item (under normal circumstances
                // this will not happen).
                int8_t destSlot = (int8_t)*freeSlots.begin();
                if(!inventory->GetItems((size_t)destSlot).IsNull())
                {
                    for(size_t freeSlot : characterManager->GetFreeSlots(
                        client, inventory))
                    {
                        destSlot = (int8_t)freeSlot;
                        break;
                    }
                }

                auto dbChanges = libcomp::DatabaseChangeSet::Create();

                if(inventory->GetItems((size_t)destSlot).IsNull())
                {
                    inventory->SetItems((size_t)destSlot, item);
                    dbChanges->Update(inventory);
                }

                item->SetItemBox(inventory->GetUUID());
                item->SetBoxSlot(destSlot);

                dbChanges->Update(bItem);
                dbChanges->Update(item);

                if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
                {
                    auto accountUID = state->GetAccountUID();
                    LogBazaarError([accountUID]()
                    {
                        return libcomp::String("BazaarItemBuy failed to "
                            "save: %1\n").Arg(accountUID.ToString());
                    });

                    client->Kill();
                    return true;
                }

                reply.WriteS8(destSlot);
                reply.WriteS32Little(0);    // Success
                success = true;

                std::list<uint16_t> updatedSlots = { (uint16_t)destSlot };
                server->GetCharacterManager()->SendItemBoxData(client,
                    inventory, updatedSlots);

                auto accountUID = state->GetAccountUID();
                auto ownerUID = market->GetAccount().GetUUID();
                LogBazaarDebug([item, bItem, cost, ownerUID, accountUID]()
                {
                    return libcomp::String("Item %1 (type %2) purchased for %3"
                        " macca from player %4 by player: %5\n")
                        .Arg(item->GetUUID().ToString()).Arg(item->GetType())
                        .Arg(cost).Arg(ownerUID.ToString())
                        .Arg(accountUID.ToString());
                });
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

        characterManager->GetItemDetailPacketData(relay, item, 1);

        // Purchased by
        relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
            cState->GetEntity()->GetName(), true);

        server->GetManagerConnection()->GetWorldConnection()->SendPacket(relay);
    }

    client->SendPacket(reply);

    return true;
}
