/**
 * @file server/channel/src/packets/game/EquipmentSpiritDefuse.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Item.h>
#include <MiDCategoryData.h>
#include <MiDevilData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiUseRestrictionsData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::EquipmentSpiritDefuse::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 12)
    {
        return false;
    }

    int64_t equipID = p.ReadS64Little();
    uint32_t fuseItemType = p.ReadU32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto dState = state->GetDemonState();

    auto equipment = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(equipID)));

    bool error = !equipment || (!equipment->GetBasicEffect() &&
        !equipment->GetSpecialEffect());

    std::list<std::shared_ptr<objects::Item>> insertItems;
    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> updateItems;
    if(!error)
    {
        // Calculate costs
        auto mainDef = definitionManager->GetItemData(equipment->GetType());
        auto basicDef = definitionManager->GetItemData(equipment
            ->GetBasicEffect());
        auto specialDef = definitionManager->GetItemData(equipment
            ->GetSpecialEffect());

        uint32_t costSum = 0;
        for(auto& itemDef : { mainDef, basicDef, specialDef })
        {
            if(itemDef)
            {
                if(characterManager->IsCPItem(itemDef))
                {
                    costSum = (uint32_t)(costSum + 100000);
                }
                else
                {
                    costSum = costSum + (uint32_t)itemDef->GetBasic()
                        ->GetBuyPrice();
                }
            }
        }

        uint32_t kzCost = (uint32_t)ceil((double)costSum /
            5000);
        uint32_t fuseItemCost = (uint32_t)(kzCost * 10);

        if(characterManager->CalculateItemRemoval(client, fuseItemType,
            fuseItemCost, updateItems) > 0)
        {
            LogItemError([&]()
            {
                return libcomp::String("EquipmentSpiritDefuse request"
                    " attempted with insufficient fusion item count: %1\n")
                    .Arg(state->GetAccountUID().ToString());
            });

            error = true;
        }
        else if(characterManager->CalculateItemRemoval(client,
            SVR_CONST.ITEM_KREUZ, kzCost, updateItems) > 0)
        {
            LogItemError([&]()
            {
                return libcomp::String("EquipmentSpiritDefuse request"
                    " attempted with insufficient kreuz: %1\n")
                    .Arg(state->GetAccountUID().ToString());
            });

            error = true;
        }
    }

    std::vector<std::pair<uint32_t, bool>> results;
    if(!error)
    {
        // Boost chances from various expertise classes
        double chainBoost = (double)(0.325 *
            (floor(0.1 * cState->GetExpertiseRank(
                EXPERTISE_CHAIN_SWORDSMITH, definitionManager)) +
            floor(0.1 * cState->GetExpertiseRank(
                EXPERTISE_CHAIN_ARMS_MAKER, definitionManager))));
        double expBoost = (double)(0.2166666 *
            (floor(0.1 * cState->GetExpertiseRank(
                EXPERTISE_WEAPON_KNOWLEDGE)) +
            floor(0.1 * cState->GetExpertiseRank(EXPERTISE_GUN_KNOWLEDGE)) +
            floor(0.1 * cState->GetExpertiseRank(EXPERTISE_SURVIVAL))));

        // Current partner demon can boost success too
        double demonBoost = 1.0;

        auto devilData = dState->GetDevilData();
        if(devilData)
        {
            switch(devilData->GetCategory()->GetRace())
            {
            case objects::MiDCategoryData::Race_t::EARTH_ELEMENT:
            case objects::MiDCategoryData::Race_t::NOCTURNE:
            case objects::MiDCategoryData::Race_t::EARTH_MOTHER:
                demonBoost = 1.2;
                break;
            default:
                break;
            }
        }

        size_t basicIdx = 1;

        // Back up values so we can roll back main item if needed
        uint32_t basicEffectCurrent = equipment->GetBasicEffect();
        uint32_t specialEffectCurrent = equipment->GetSpecialEffect();
        int16_t tarotCurrent = equipment->GetTarot();
        int16_t soulCurrent = equipment->GetSoul();
        uint16_t duraCurrent = equipment->GetDurability();
        int8_t maxDuraCurrent = equipment->GetMaxDurability();
        auto fuseBonusesCurrent = equipment->GetFuseBonuses();
        auto modSlotsCurrent = equipment->GetModSlots();

        results.push_back(std::make_pair(equipment->GetType(), false));
        results.push_back(std::make_pair(equipment->GetBasicEffect(), false));
        results.push_back(std::make_pair(equipment->GetSpecialEffect(), false));

        // If either are null (shouldnt happen) default to same item
        if(!results[1].first)
        {
            results[1].first = equipment->GetType();
        }

        if(!results[2].first)
        {
            results[2].first = equipment->GetType();
        }

        // If all 3 are the same item, create a secondary one
        // If one other item differs, only create the different one
        // If both other items differ, create both
        if(results[0].first == results[2].first ||
            results[1].first == results[2].first)
        {
            // If the main item or basic item are the same as the special item,
            // erase special and second item remains as the basic item
            results.pop_back();
        }
        else if(results[0].first == results[1].first)
        {
            // If the main item and basic item are the same, erase basic and
            // the main item becomes the basic item
            results.erase(results.begin() + 1);
            basicIdx--;
        }

        // Set results per entry
        for(auto& pair : results)
        {
            auto itemDef = definitionManager->GetItemData(pair.first);
            if((itemDef->GetBasic()->GetFlags() & 0x1000) != 0)
            {
                // Separating crystals can't fail
                pair.second = true;
                continue;
            }

            // Base rate is 26%
            double successRate = 26.0;

            // No base level boost applies if no expertise exists
            if(chainBoost > 1.0 || expBoost > 1.0)
            {
                // CP boosts success
                double cpBoost = characterManager->IsCPItem(itemDef)
                    ? 1.2 : 1.0;

                successRate = successRate + (chainBoost + expBoost) *
                    cpBoost * demonBoost;
            }

            if(successRate >= 100.00 ||
                RNG(int32_t, 1, 10000) <= (int32_t)(successRate * 100.0))
            {
                pair.second = true;
            }
        }

        for(size_t i = 0; i < results.size(); i++)
        {
            std::shared_ptr<objects::Item> item;
            if(i == 0)
            {
                item = equipment;
                updateItems[item] = 1;

                // Clear effects, bonuses and expiration
                item->SetBasicEffect(0);
                item->SetSpecialEffect(0);
                item->SetFuseBonuses(0, 0);
                item->SetFuseBonuses(1, 0);
                item->SetFuseBonuses(2, 0);
                item->SetRentalExpiration(0);
            }
            else
            {
                // Generate a new item
                item = characterManager->GenerateItem(results[i].first, 1);
                insertItems.push_back(item);
            }

            if(i == basicIdx)
            {
                if(item != equipment)
                {
                    // Move mod slots to the item
                    item->SetModSlots(equipment->GetModSlots());

                    // Reset the main item slots
                    auto itemDef = definitionManager->GetItemData(
                        equipment->GetType());
                    auto restr = itemDef->GetRestriction();

                    for(uint8_t k = 0; k < 5; k++)
                    {
                        equipment->SetModSlots((size_t)k, k < restr->GetModSlots()
                            ? MOD_SLOT_NULL_EFFECT : 0);
                    }
                }

                // Clear mod slots
                for(size_t k = 0; k < 5; k++)
                {
                    if(item->GetModSlots(k) &&
                        item->GetModSlots(k) != MOD_SLOT_NULL_EFFECT)
                    {
                        item->SetModSlots(k, MOD_SLOT_NULL_EFFECT);
                    }
                }
            }

            // Clear all tarot/soul effects (enable effect for basic item)
            item->SetTarot(i == basicIdx ? ENCHANT_ENABLE_EFFECT : 0);
            item->SetSoul(i == basicIdx ? ENCHANT_ENABLE_EFFECT : 0);

            if(!results[i].second && item->GetMaxDurability())
            {
                // Failure, halve max durability (floor 1)
                int8_t maxDura = (int8_t)floor(item->GetMaxDurability() /2);
                if(!maxDura)
                {
                    maxDura = 1;
                }

                item->SetMaxDurability(maxDura);
                if(item->GetDurability() > (uint16_t)(maxDura * 1000))
                {
                    item->SetDurability((uint16_t)(maxDura * 1000));
                }
            }
        }

        if(!characterManager->UpdateItems(client, false, insertItems,
            updateItems))
        {
            LogItemError([&]()
            {
                return libcomp::String("EquipmentSpiritDefuse failed"
                    " to update items: %1\n")
                    .Arg(state->GetAccountUID().ToString());
            });

            // Roll it back
            equipment->SetBasicEffect(basicEffectCurrent);
            equipment->SetSpecialEffect(specialEffectCurrent);
            equipment->SetTarot(tarotCurrent);
            equipment->SetSoul(soulCurrent);
            equipment->SetDurability(duraCurrent);
            equipment->SetMaxDurability(maxDuraCurrent);
            equipment->SetFuseBonuses(fuseBonusesCurrent);
            equipment->SetModSlots(modSlotsCurrent);

            error = true;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_EQUIPMENT_SPIRIT_DEFUSE);
    reply.WriteS32Little(error ? -1 : 0);
    reply.WriteS64Little(equipID);
    reply.WriteU32Little(fuseItemType);

    reply.WriteS32Little((int32_t)results.size());
    for(auto& pair : results)
    {
        reply.WriteU32Little(pair.first);
        reply.WriteS32Little(pair.second ? 0 : 1);
    }

    client->SendPacket(reply);

    return true;
}
