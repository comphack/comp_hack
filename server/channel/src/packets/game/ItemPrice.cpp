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
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>

// objects Includes
#include <MiItemData.h>
#include <MiItemBasicData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::ItemPrice::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 42)
    {
        return false;
    }

    int32_t requestID = p.ReadS32Little();

    uint32_t itemType = p.ReadU32Little();
    uint16_t stackSize = p.ReadU16Little();

    uint16_t durability = p.ReadU16Little();
    int8_t maxDurability = p.ReadS8();
    int16_t tarot = p.ReadS16Little();
    int16_t soul = p.ReadS16Little();

    std::array<uint16_t, 5> modSlots;
    modSlots[0] = p.ReadU16Little();
    modSlots[1] = p.ReadU16Little();
    modSlots[3] = p.ReadU16Little();
    modSlots[4] = p.ReadU16Little();
    modSlots[5] = p.ReadU16Little();

    int32_t expiration = p.ReadS32Little();
    uint32_t basicEffect = p.ReadU32Little();
    uint32_t specialEffect = p.ReadU32Little();

    std::array<int8_t, 3> fuseBonuses;
    fuseBonuses[0] = p.ReadS8();
    fuseBonuses[1] = p.ReadS8();
    fuseBonuses[2] = p.ReadS8();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
    auto definitionManager = server->GetDefinitionManager();

    auto itemData = definitionManager->GetItemData(itemType);

    int32_t price = 0;
    if(itemData)
    {
        // Price starts at 100 times the normal purchase price if it were
        // to show up in stores and gets modified by various aspects
        int32_t basePrice = itemData->GetBasic()->GetBuyPrice();
        if(server->GetCharacterManager()->IsCPItem(itemData))
        {
            // CP items default to 100,000 just like Spirit Fusion adjustments
            basePrice = 100000;
        }

        price = basePrice * 100;

        double multiplier = 1.0;
        if(itemData->GetBasic()->GetEquipType() !=
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE)
        {
            // Apply equipment specific modifiers
            if(tarot == ENCHANT_ENABLE_EFFECT)
            {
                // +20% for tarot enabled
                multiplier += 0.2;
            }
            else if(tarot > 0)
            {
                // +50% for tarot effect
                multiplier += 0.5;
            }

            if(soul == ENCHANT_ENABLE_EFFECT)
            {
                // +30% for soul enabled
                multiplier += 0.3;
            }
            else if(soul > 0)
            {
                // +75% for soul effect
                multiplier += 0.75;
            }

            for(uint16_t mod : modSlots)
            {
                // +10% per mod slot
                if(mod && mod != MOD_SLOT_NULL_EFFECT)
                {
                    multiplier += 0.1;
                }
            }

            for(int8_t bonus : fuseBonuses)
            {
                for(size_t i = 0; i < 8; i++)
                {
                    // +2% increase per bonus rank
                    if((bonus >> i) & 0x01)
                    {
                        multiplier += 0.02;
                    }
                }
            }

            if(((int32_t)basicEffect != -1) ||
                ((int32_t)specialEffect != -1))
            {
                // +15% for SI (since it could be way worse)
                multiplier += 0.15;
            }

            // Apply increases before reducing
            price = (int32_t)(price * multiplier);

            int8_t itemDurMax = (int8_t)itemData->GetPossession()
                ->GetDurability();
            if(maxDurability < itemDurMax)
            {
                // Reduce directly from percent of max durability left
                price = (int32_t)((double)price * (double)maxDurability /
                    (double)itemDurMax);
            }

            if((int8_t)(durability / 1000) != maxDurability)
            {
                // Reduce by repair cost x2
                price = (int32_t)(price - (maxDurability -
                    (int8_t)(durability / 1000)) * itemData->GetBasic()
                    ->GetRepairPrice() * 2);
            }
        }

        // Set minimum of 1
        if(price <= 0)
        {
            price = 1;
        }

        if(expiration)
        {
            // Cut any item that can expire's price in half (or reduce to 1 if
            // it is already expired)
            int32_t now = (int32_t)std::time(0);
            if(now < expiration)
            {
                price = 1;
            }
            else
            {
                price = (int32_t)(price / 2);
            }
        }

        // Finally multiply by stack size
        price = (int32_t)(price * stackSize);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_PRICE);
    reply.WriteS32Little(requestID);
    reply.WriteU32Little((uint32_t)price);
    reply.WriteS32Little(price > 0 ? 0 : -1);

    connection->SendPacket(reply);

    return true;
}
