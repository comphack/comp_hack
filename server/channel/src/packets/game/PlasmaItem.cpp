/**
 * @file server/channel/src/packets/game/PlasmaItem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to retrieve an item from a plasma
 *  point.
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
#include <DefinitionManager.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// objects Includes
#include <Item.h>
#include <ItemBox.h>
#include <Loot.h>
#include <LootBox.h>
#include <MiItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "PlasmaState.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::PlasmaItem::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 7)
    {
        return false;
    }

    int32_t plasmaID = p.ReadS32Little();
    int8_t pointID = p.ReadS8();
    int8_t slotID = p.ReadS8();
    int8_t unknown = p.ReadS8();    // Always -1?
    (void)unknown;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto zone = cState->GetZone();
    auto pState = zone ? std::dynamic_pointer_cast<PlasmaState>(
        zone->GetEntity(plasmaID)) : nullptr;
    auto point = pState ? pState->GetPoint((uint32_t)pointID) : nullptr;
    auto lBox = point ? point->GetLoot() : nullptr;

    std::list<int8_t> lootedSlots;
    std::unordered_map<uint32_t, uint32_t> lootedItems;
    if(lBox)
    {
        auto inventory = cState->GetEntity()->GetItemBoxes(0).Get();

        size_t freeSlots = 0;
        std::unordered_map<uint32_t, uint16_t> stacksFree;
        for(size_t i = 0; i < inventory->ItemsCount(); i++)
        {
            auto item = inventory->GetItems(i);
            if(item.IsNull())
            {
                freeSlots++;
            }
            else
            {
                uint32_t itemType = item->GetType();
                auto def = definitionManager->GetItemData(itemType);
                uint16_t maxStack = def->GetPossession()->GetStackSize();
                if(item->GetStackSize() < maxStack)
                {
                    uint16_t delta = (uint16_t)(maxStack -
                        item->GetStackSize());
                    auto it = stacksFree.find(itemType);
                    if(it == stacksFree.end())
                    {
                        stacksFree[itemType] = delta;
                    }
                    else
                    {
                        stacksFree[itemType] = (uint16_t)(
                            stacksFree[itemType] + delta);
                    }
                }
            }
        }

        if(freeSlots > 0)
        {
            std::set<int8_t> slots;
            if(slotID != -1)
            {
                slots.insert(slotID);
            }

            auto lootMap = zone->TakeLoot(lBox, slots, freeSlots,
                stacksFree);
            for(auto lPair : lootMap)
            {
                auto loot = lPair.second;
                lootedSlots.push_back((int8_t)lPair.first);

                if(lootedItems.find(loot->GetType()) == lootedItems.end())
                {
                    lootedItems[loot->GetType()] = (uint32_t)loot->GetCount();
                }
                else
                {
                    lootedItems[loot->GetType()] = (uint32_t)(
                        lootedItems[loot->GetType()] + loot->GetCount());
                }
            }

            if(pState->HideIfEmpty(point))
            {
                libcomp::Packet notify;
                pState->GetPointStatusData(notify, (uint32_t)point->GetID());
                server->GetZoneManager()->BroadcastPacket(client, notify, true);
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_ITEM);
    reply.WriteS32Little(plasmaID);
    reply.WriteS8(pointID);

    if(lootedSlots.size() > 0)
    {
        reply.WriteS32Little(0);    // Success
        reply.WriteS8((int8_t)lootedSlots.size());
        for(auto slot : lootedSlots)
        {
            reply.WriteS8(slot);
            reply.WriteS8(0);   // Target box slot, doesn't seem to actually matter
        }

        client->QueuePacket(reply);

        if(lootedItems.size() > 0)
        {
            characterManager->AddRemoveItems(client, lootedItems, true);
        }

        client->FlushOutgoing();
    }
    else
    {
        reply.WriteS32Little(-1);

        client->SendPacket(reply);
    }

    return true;
}
