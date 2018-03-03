/**
 * @file server/channel/src/packets/game/ShopRepair.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to repair an item at a shop.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// object Includes
#include <EventInstance.h>
#include <EventOpenMenu.h>
#include <EventState.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiModifiedEffectData.h>
#include <ServerShop.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ShopRepair::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 20)
    {
        return false;
    }

    int32_t shopID = p.ReadS32Little();
    int32_t cacheID = p.ReadS32Little();
    int64_t itemID = p.ReadS64Little();
    uint32_t pointDelta = p.ReadU32Little();
    (void)cacheID;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    auto shop = server->GetServerDataManager()->GetShopData((uint32_t)shopID);
    auto itemData = item ? definitionManager->GetItemData(item->GetType()) : nullptr;

    bool success = false;
    if(shop && itemData && item->GetItemBox().Get() == inventory)
    {
        bool kreuzRepair = false;

        auto eState = state->GetEventState();
        auto current = eState ? eState->GetCurrent() : nullptr;
        if(current)
        {
            auto menu = std::dynamic_pointer_cast<objects::EventOpenMenu>(
                current->GetEvent());
            if(menu && (uint32_t)menu->GetMenuType() == SVR_CONST.MENU_REPAIR_KZ)
            {
                kreuzRepair = true;
            }
        }

        int32_t repairBase = itemData->GetBasic()->GetRepairPrice();

        float enchantBoost = (item->GetTarot() > 0 ? 1.5f : 0.f) +
            (item->GetSoul() ? 3.f : 0.f);
        if(enchantBoost == 0.f)
        {
            enchantBoost = 1.f;
        }

        // Weapons can have repair reduction modifications that can reduce
        // the repair cost to 0
        float modReduction = 0.f;
        if(itemData->GetBasic()->GetEquipType() ==
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON)
        {
            for(size_t i = 0; i < 5; i++)
            {
                uint16_t effectID = item->GetModSlots(i);
                auto effect = definitionManager->GetModifiedEffectData(effectID);
                if(effect && effect->GetType() == MOD_SLOT_REPAIR_REDUCTION_TYPE)
                {
                    modReduction += 0.1f * (float)effect->GetSequenceID();
                }
            }
        }

        if(modReduction > 1.f)
        {
            modReduction = 1.f;
        }

        uint32_t cost = 0;
        if(kreuzRepair)
        {
            // Calculate with all adjustments at once
            cost = (uint32_t)ceil((float)repairBase * 10.5f *
                shop->GetRepairCostMultiplier() * enchantBoost *
                (1.f - modReduction) * (float)pointDelta);

            // Set minimum total cost of 1
            if(cost == 0)
            {
                cost = 1;
            }
        }
        else
        {
            // Calculate the cost per point
            uint32_t pointCost = (uint32_t)floor((float)repairBase *
                shop->GetRepairCostMultiplier() * enchantBoost);

            // Apply mod reduction
            if(modReduction > 0.f)
            {
                pointCost = (uint32_t)floor((float)pointCost * (1.f - modReduction));
            }

            // Set minimum point cost of 1 (not minimum total cost)
            if(pointCost == 0)
            {
                pointCost = 1;
            }

            // Now that we have the final point cost, calculate the total
            cost = (uint32_t)(pointCost * pointDelta);
        }

        std::unordered_map<uint32_t, uint32_t> payment;
        payment[kreuzRepair ? SVR_CONST.ITEM_KREUZ
            : SVR_CONST.ITEM_MACCA] = cost;

        if(characterManager->AddRemoveItems(client, payment, false))
        {
            int32_t currentUp = 0;
            int32_t maxDown = 0;

            uint16_t repairRate = (uint16_t)(shop->GetRepairRate() * 100.f);
            if(repairRate >= 10000)
            {
                // Do full repair
                currentUp = (int32_t)(pointDelta * 1000);
            }
            else
            {
                // Check rate once per point
                for(uint32_t i = 0; i < pointDelta; i++)
                {
                    if(RNG(uint16_t, 1, 10000) <= repairRate)
                    {
                        // Repair one point
                        currentUp += 1000;
                    }
                    else
                    {
                        // Drop max by one point
                        maxDown--;
                    }
                }
            }

            // Increase current durability
            if(currentUp)
            {
                characterManager->UpdateDurability(client, item, currentUp,
                    true, false, false);
            }

            // Decrease max durability
            if(maxDown)
            {
                characterManager->UpdateDurability(client, item, maxDown,
                    true, true, false);
            }

            success = true;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOP_REPAIR);
    reply.WriteS32Little(shopID);
    reply.WriteS64Little(itemID);
    reply.WriteU16Little(item ? item->GetDurability() : 0);
    reply.WriteS8(item ? item->GetMaxDurability() : 0);
    reply.WriteS32Little(success ? 0 : -5);

    client->QueuePacket(reply);

    if(success && item->GetBoxSlot() >= 0)
    {
        characterManager->SendItemBoxData(client, inventory,
            { (uint16_t)item->GetBoxSlot() });
    }

    client->FlushOutgoing();

    return true;
}
