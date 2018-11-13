/**
 * @file server/channel/src/packets/game/DestinyLotto.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to start a Destiny box item lotto.
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
#include <Randomizer.h>
#include <ServerConstants.h>

// libcomp Includes
#include <DestinyBox.h>
#include <Loot.h>
#include <ZoneInstance.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::DestinyLotto::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 8)
    {
        return false;
    }

    uint32_t assistItemType = p.ReadU32Little();
    uint16_t bonusCount = p.ReadU16Little();

    bool slotSpecified = p.ReadU16Little() == 1;

    uint8_t itemSlot = 0;
    if(slotSpecified)
    {
        if(p.Left() == 1)
        {
            itemSlot = p.ReadU8();
        }
        else
        {
            return false;
        }
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto zone = state->GetZone();
    auto instance = zone ? zone->GetInstance() : nullptr;
    auto dBox = instance
        ? instance->GetDestinyBox(state->GetWorldCID()) : nullptr;

    bool success = false;
    std::list<std::shared_ptr<objects::Loot>> loot;
    std::shared_ptr<objects::Loot> specifiedLoot;
    if(dBox)
    {
        // Make sure the box is full, otherwise fail
        success = true;
        loot = dBox->GetLoot();

        uint8_t slot = 0;
        for(auto l : dBox->GetLoot())
        {
            if(!l)
            {
                success = false;
                break;
            }
            else if(slot == itemSlot && slotSpecified)
            {
                specifiedLoot = l;
            }

            slot = (uint8_t)(slot + 1);
        }
    }

    if(success && assistItemType != static_cast<uint32_t>(-1))
    {
        auto it = SVR_CONST.ADJUSTMENT_ITEMS.find(assistItemType);
        if(it == SVR_CONST.ADJUSTMENT_ITEMS.end() ||
            it->second[0] != 2 || it->second[2] < (int32_t)bonusCount)
        {
            LOG_ERROR(libcomp::String("Invalid Destiny lotto item or bonus"
                " count supplied: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            success = false;
        }
        else if(it->second[1] != 1 && slotSpecified)
        {
            LOG_ERROR(libcomp::String("Destiny lotto explicit selection"
                " attempted with invalid assist item selected: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            success = false;
        }
        else
        {
            // Consume one assist item per bonus added (or one for slot
            // specification)
            std::unordered_map<uint32_t, uint32_t> items;
            items[assistItemType] = slotSpecified == 1 ? 1 : bonusCount;
            success = characterManager->AddRemoveItems(client, items, false);
        }
    }
    else if(success && slotSpecified)
    {
        LOG_ERROR(libcomp::String("Destiny lotto explicit selection"
            " attempted with no assist item selected: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        success = false;
    }
    else if(success && bonusCount)
    {
        LOG_ERROR(libcomp::String("Destiny bonus count supplied with no"
            " assist item: %1\n").Arg(state->GetAccountUID().ToString()));
        success = false;
    }

    if(success)
    {
        std::set<uint8_t> removes;
        for(uint8_t i = 0; i < (uint8_t)loot.size(); i++)
        {
            removes.insert(i);
        }

        uint8_t newNext = 0;
        std::list<std::shared_ptr<objects::Loot>> empty;
        auto results = instance->UpdateDestinyBox(state->GetWorldCID(),
            newNext, empty, removes);
        if(removes.size() != loot.size())
        {
            success = false;
        }
        else if(specifiedLoot)
        {
            loot.clear();
            loot.push_back(specifiedLoot);
        }
        else
        {
            std::set<std::shared_ptr<objects::Loot>> allLoot;
            for(auto l : loot)
            {
                allLoot.insert(l);
            }

            loot.clear();

            size_t total = (size_t)(1 + bonusCount);
            for(size_t i = 0; i < total && allLoot.size() > 0; i++)
            {
                auto l = libcomp::Randomizer::GetEntry(allLoot);
                allLoot.erase(l);
                loot.push_back(l);
            }
        }
    }

    std::list<std::shared_ptr<ChannelClientConnection>> clients;
    if(dBox->GetOwnerCID() != 0 || !success)
    {
        clients.push_back(client);
    }
    else
    {
        // Everyone in the instance gets the items
        clients = instance->GetConnections();
    }

    for(auto c : clients)
    {
        // Make sure the character can receive all the items
        std::list<std::pair<uint32_t, uint16_t>> add;
        if(success)
        {
            auto freeSlots = characterManager->GetFreeSlots(c);
            for(auto l : loot)
            {
                if(add.size() == freeSlots.size()) break;

                add.push_back(std::make_pair(l->GetType(), l->GetCount()));
            }
        }

        libcomp::Packet reply;
        reply.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_DESTINY_LOTTO);
        reply.WriteS32Little(success ? 0 : -1);
        reply.WriteS32Little((int32_t)add.size());
        for(auto& pair : add)
        {
            reply.WriteU32Little(pair.first);
            reply.WriteU16Little(pair.second);
        }

        c->QueuePacket(reply);

        if(success)
        {
            std::unordered_map<uint32_t, uint32_t> items;
            for(auto& pair : add)
            {
                if(items.find(pair.first) != items.end())
                {
                    items[pair.first] = (uint32_t)(items[pair.first] +
                        pair.second);
                }
                else
                {
                    items[pair.first] = (uint32_t)pair.second;
                }
            }

            characterManager->AddRemoveItems(c, items, true);
        }

        c->FlushOutgoing();
    }

    return true;
}
