/**
 * @file server/channel/src/packets/game/LootItem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to loot an item from a loot box.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <Item.h>
#include <ItemBox.h>
#include <Loot.h>
#include <LootBox.h>
#include <MiItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::LootItem::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 10)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();
    int32_t lootEntityID = p.ReadS32Little();
    int8_t slotID = p.ReadS8();
    int8_t unknown = p.ReadS8();    // Always -1?
    (void)unknown;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto zone = cState->GetZone();
    auto lState = zone ? zone->GetLootBox(lootEntityID) : nullptr;
    auto lBox = lState ? lState->GetEntity() : nullptr;

    uint32_t demonType = 0;
    std::list<int8_t> lootedSlots;
    std::unordered_map<uint32_t, uint16_t> lootedItems;
    if(lBox && (lBox->ValidLooterIDsCount() == 0 ||
        lBox->ValidLooterIDsContains(state->GetWorldCID())))
    {
        if(lBox->GetType() == objects::LootBox::Type_t::EGG)
        {
            // Demon egg
            auto comp = character->GetCOMP().Get();
            auto progress = character->GetProgress();

            size_t freeSlots = 0;
            size_t maxSlots = (size_t)progress->GetMaxCOMPSlots();
            for(size_t i = 0; i < maxSlots; i++)
            {
                freeSlots += (size_t)(comp->GetDemons(i).IsNull() ? 1 : 0);
            }

            if(freeSlots > 0)
            {
                std::set<int8_t> slots = { 0 };

                auto lootMap = zone->TakeLoot(lState, slots, freeSlots);
                for(auto lPair : lootMap)
                {
                    // Should only be one
                    demonType = lPair.second->GetType();
                    lootedSlots.push_back((int8_t)lPair.first);
                }
            }
        }
        else
        {
            // Item box
            auto inventory = character->GetItemBoxes(0).Get();

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
                    auto def = definitionManager->GetItemData(
                        itemType);
                    uint16_t maxStack = def->GetPossession()
                        ->GetStackSize();
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

                auto lootMap = zone->TakeLoot(lState, slots, freeSlots,
                    stacksFree);
                for(auto lPair : lootMap)
                {
                    auto loot = lPair.second;
                    lootedSlots.push_back((int8_t)lPair.first);

                    if(lootedItems.find(loot->GetType()) == lootedItems.end())
                    {
                        lootedItems[loot->GetType()] = loot->GetCount();
                    }
                    else
                    {
                        lootedItems[loot->GetType()] = (uint16_t)(
                            lootedItems[loot->GetType()] + loot->GetCount());
                    }
                }
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOOT_ITEM);
    reply.WriteS32Little(entityID);
    reply.WriteS32Little(lootEntityID);

    if(lootedSlots.size() > 0)
    {
        reply.WriteS8(0);   // Success
        reply.WriteS8((int8_t)lootedSlots.size());
        for(auto slot : lootedSlots)
        {
            reply.WriteS8(slot);
            reply.WriteS8(0);   // Unknown
        }

        auto zConnections = zone->GetConnectionList();

        client->QueuePacket(reply);
        characterManager->SendLootItemData(zConnections, lState,
            true);

        if(lootedItems.size() > 0)
        {
            characterManager->AddRemoveItems(client, lootedItems, true);
        }

        if(demonType != 0)
        {
            characterManager->ContractDemon(client,
                definitionManager->GetDevilData(demonType),
                lState->GetEntityID());
        }

        bool remove = true;
        for(auto loot : lBox->GetLoot())
        {
            if(loot && loot->GetCount() > 0)
            {
                remove = false;
                break;
            }
        }

        if(remove)
        {
            std::list<int32_t> entityIDs = { lState->GetEntityID() };

            auto zoneManager = server->GetZoneManager();
            if(lBox->GetType() == objects::LootBox::Type_t::BODY)
            {
                // Bodies get removed 10 seconds after theyve been looted, or after
                // their loot time has passed, whichever comes first
                uint64_t removeTime = ChannelServer::GetServerTime() + 10000000;
                zoneManager->ScheduleEntityRemoval(removeTime, zone, entityIDs, 13);
            }
            else
            {
                int32_t removeMode = 0;
                if(lBox->GetType() == objects::LootBox::Type_t::EGG)
                {
                    removeMode = 3;
                }

                // Everything else is removed right away
                zone->RemoveEntity(lState->GetEntityID());
                server->GetZoneManager()->RemoveEntitiesFromZone(zone, entityIDs, removeMode);
            }
        }

        ChannelClientConnection::FlushAllOutgoing(zConnections);
    }
    else
    {
        reply.WriteS8(-1);

        client->SendPacket(reply);
    }

    return true;
}
