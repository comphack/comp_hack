/**
 * @file server/channel/src/packets/game/EquipmentSpiritFuse.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief
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

bool Parsers::EquipmentSpiritFuse::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 32)
    {
        return false;
    }

    int64_t mainID = p.ReadS64Little();
    int64_t basicID = p.ReadS64Little();
    int64_t specialID = p.ReadS64Little();
    int64_t assistID = p.ReadS64Little();

    const int32_t RESULT_FAILURE = 0;
    const int32_t RESULT_SUCCESS = 1;
    const int32_t RESULT_GREAT_SUCCESS = 2;

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

    auto mainItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(mainID)));
    auto basicItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(basicID)));
    auto specialItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(specialID)));
    auto assistItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(assistID)));

    // Check all required items and make sure equipment is not broken
    bool error = !mainItem || !basicItem || !specialItem ||
        (assistID != -1 && !assistItem) || !mainItem->GetMaxDurability() ||
        !basicItem->GetMaxDurability() || !specialItem->GetMaxDurability();

    auto equipType = objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE;

    uint32_t cost = 0;
    bool includesCPItem = false;
    if(!error)
    {
        auto mainDef = definitionManager->GetItemData(mainItem->GetType());
        auto basicDef = definitionManager->GetItemData(basicItem->GetType());
        auto specialDef = definitionManager->GetItemData(specialItem->GetType());

        if(mainDef)
        {
            equipType = mainDef->GetBasic()->GetEquipType();
        }

        // Check if any item is not specified or is spirit fusion disabled
        if(!mainDef || !basicDef || !specialDef ||
            ((mainDef->GetBasic()->GetFlags() & 0x0800) != 0) ||
            ((basicDef->GetBasic()->GetFlags() & 0x0800) != 0) ||
            ((specialDef->GetBasic()->GetFlags() & 0x0800) != 0))
        {
            LOG_ERROR(libcomp::String("EquipmentSpiritFuse request received"
                " with one or more invalid item type(s): %1, %2, %3\n")
                .Arg(mainItem->GetType()).Arg(basicItem->GetType())
                .Arg(specialItem->GetType()));
            error = true;
        }
        else if(equipType ==
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BULLETS ||
            equipType ==
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE)
        {
            LOG_ERROR(libcomp::String("EquipmentSpiritFuse request received"
                " with invalid equipment type item: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            error = true;
        }
        else if(equipType != basicDef->GetBasic()->GetEquipType() ||
            equipType != specialDef->GetBasic()->GetEquipType())
        {
            LOG_ERROR(libcomp::String("EquipmentSpiritFuse request received"
                " with equipment types that do not match: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            error = true;
        }
        else if(equipType !=
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON)
        {
            // Armor can only be one gender type
            std::set<uint8_t> genders;
            genders.insert(mainDef->GetRestriction()->GetGender());
            genders.insert(basicDef->GetRestriction()->GetGender());
            genders.insert(specialDef->GetRestriction()->GetGender());

            // Ignore "any"
            genders.erase(2);

            if(genders.size() > 1)
            {
                LOG_ERROR(libcomp::String("EquipmentSpiritFuse request"
                    " received with differing gender armor: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                error = true;
            }
        }

        if(!error)
        {
            // Validations passed, gather info and cost
            uint32_t costSum = 0;
            for(auto& itemDef : { mainDef, basicDef, specialDef })
            {
                if(characterManager->IsCPItem(itemDef))
                {
                    costSum = (uint32_t)(costSum + 100000);
                    includesCPItem = true;
                }
                else
                {
                    costSum = costSum + (uint32_t)itemDef->GetBasic()
                        ->GetBuyPrice();
                }
            }

            // Maximum fuse bonus across the items scales the cost
            int8_t maxBonus = 0;
            for(auto& item : { mainItem, basicItem, specialItem })
            {
                for(int8_t bonus : item->GetFuseBonuses())
                {
                    if(bonus > maxBonus)
                    {
                        maxBonus = bonus;
                    }
                }
            }

            double bonusMultiplier = 1.0;
            if(maxBonus >= 40)
            {
                bonusMultiplier = 3.4375;
            }
            else if(maxBonus >= 30)
            {
                bonusMultiplier = 3.125;
            }
            else if(maxBonus >= 20)
            {
                bonusMultiplier = 2.5;
            }
            else if(maxBonus >= 10)
            {
                bonusMultiplier = 1.5;
            }

            cost = (uint32_t)floor(floor((double)costSum / 3.0) * 0.8 *
                bonusMultiplier);
        }
    }

    std::list<std::shared_ptr<objects::Item>> insertItems;
    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> updateItems;
    if(!error && !characterManager->CalculateMaccaPayment(client,
        (uint64_t)cost, insertItems, updateItems))
    {
        LOG_ERROR(libcomp::String("EquipmentSpiritFuse request"
            " attempted with insufficient macca: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        error = true;
    }

    if(!error)
    {
        // Perform the fusion
        int32_t result = RESULT_FAILURE;

        // Boost chances from various expertise classes
        double chainBoost = (double)(2.5 *
            (floor(0.1 * cState->GetExpertiseRank(
                definitionManager, EXPERTISE_CHAIN_SWORDSMITH)) +
            floor(0.1 * cState->GetExpertiseRank(
                definitionManager, EXPERTISE_CHAIN_ARMS_MAKER))));
        double expBoost = (double)(5.0 / 3.0 *
            (floor(0.1 * cState->GetExpertiseRank(
                definitionManager, EXPERTISE_WEAPON_KNOWLEDGE)) +
            floor(0.1 * cState->GetExpertiseRank(
                definitionManager, EXPERTISE_GUN_KNOWLEDGE)) +
            floor(0.1 * cState->GetExpertiseRank(
                definitionManager, EXPERTISE_SURVIVAL))));

        // CP boosts success
        double cpBoost = includesCPItem ? 1.1 : 1.0;

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

        // Base rate is 30%
        double successRate = 30.0;

        // No base level boost applies if no expertise exists
        if(chainBoost > 1.0 || expBoost > 1.0)
        {
            successRate = successRate + (chainBoost + expBoost) *
                cpBoost * demonBoost;
        }

        // Lastly add bonus item rates (from equipment and fusion items)
        // Since great sucess is relative to success, do not stop at > 100%
        std::set<uint32_t> effectItems;
        effectItems.insert(mainItem->GetType());
        effectItems.insert(basicItem->GetType());
        effectItems.insert(specialItem->GetType());

        if(assistItem)
        {
            effectItems.insert(assistItem->GetType());
        }

        for(auto equip : character->GetEquippedItems())
        {
            if(!equip.IsNull())
            {
                effectItems.insert(equip->GetType());
            }
        }

        double gSuccessBoost = 0.0;
        for(uint32_t effectItem : effectItems)
        {
            auto it = SVR_CONST.SPIRIT_FUSION_BOOST.find(effectItem);
            if(it != SVR_CONST.SPIRIT_FUSION_BOOST.end())
            {
                successRate = successRate + (double)it->second[0];
                gSuccessBoost = gSuccessBoost + (double)it->second[1];
            }
            else
            {
                // CP spirit fusion crystals boost success by 100%
                auto itemData = definitionManager->GetItemData(effectItem);
                if(itemData &&
                    ((itemData->GetBasic()->GetFlags() & 0x1000) != 0) &&
                    characterManager->IsCPItem(itemData))
                {
                    successRate = successRate + 100.0;
                }
            }
        }

        // Default great success is 10% of the success rate
        double gSuccessRate = (successRate * 0.1) + gSuccessBoost;

        // Determine outcome
        if(successRate >= 100.00 ||
            RNG(int32_t, 1, 10000) <= (int32_t)(successRate * 100.0))
        {
            result = RESULT_SUCCESS;

            if(gSuccessRate >= 100.00 ||
                RNG(int32_t, 1, 10000) <= (int32_t)(gSuccessRate * 100.0))
            {
                result = RESULT_GREAT_SUCCESS;
            }
        }

        // Perform the fusion and pay the cost

        // Back up values so we can roll back main item if needed
        uint32_t basicEffectCurrent = mainItem->GetBasicEffect();
        uint32_t specialEffectCurrent = mainItem->GetSpecialEffect();
        auto fuseBonusesCurrent = mainItem->GetFuseBonuses();
        auto modSlotsCurrent = mainItem->GetModSlots();

        uint32_t basicEffect = basicItem->GetBasicEffect();
        mainItem->SetBasicEffect(basicEffect ? basicEffect
            : basicItem->GetType());
        mainItem->SetModSlots(basicItem->GetModSlots());

        uint32_t spEffect = specialItem->GetSpecialEffect();
        mainItem->SetSpecialEffect(spEffect ? spEffect
            : specialItem->GetType());

        std::set<std::shared_ptr<objects::Item>> fusionItems;
        fusionItems.insert(mainItem);
        fusionItems.insert(basicItem);
        fusionItems.insert(specialItem);

        for(auto& item : fusionItems)
        {
            // Add previous bonuses to main
            if(item != mainItem)
            {
                for(size_t i = 0; i < 3; i++)
                {
                    int8_t bonus = mainItem->GetFuseBonuses(i);
                    bonus = (int8_t)(bonus + item->GetFuseBonuses(i));
                    if(bonus > MAX_FUSE_BONUS)
                    {
                        bonus = MAX_FUSE_BONUS;
                    }

                    mainItem->SetFuseBonuses(i, bonus);
                }

                updateItems[item] = 0;
            }
        }

        if(result == RESULT_FAILURE)
        {
            // Fusion still happens but set the expiration for when it
            // can no longer be used
            int32_t expirationDays = RNG(int32_t, 10, 30);

            mainItem->SetRentalExpiration((uint32_t)(
                (int32_t)std::time(0) + expirationDays * 24 * 3600));
        }
        else
        {
            // Clear the expiration
            mainItem->SetRentalExpiration(0);

            if(result == RESULT_GREAT_SUCCESS)
            {
                // Add fusion bonuses, one point guaranteed, random chance
                // to increase the rest
                std::set<size_t> bonusSlots;
                
                switch(equipType)
                {
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON:
                    bonusSlots.insert(0);
                    bonusSlots.insert(1);
                    bonusSlots.insert(2);
                    break;
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_HEAD:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_ARMS:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FEET:
                    bonusSlots.insert(0);
                    bonusSlots.insert(1);
                    break;
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_RING:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EARRING:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EXTRA:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TALISMAN:
                    bonusSlots.insert(1);
                    break;
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FACE:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NECK:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_COMP:
                case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BACK:
                default:
                    // No bonuses
                    break;
                }

                // Exclude any at max
                for(size_t i = 0; i < 3; i++)
                {
                    if(mainItem->GetFuseBonuses(i) >= MAX_FUSE_BONUS)
                    {
                        bonusSlots.erase(i);
                    }
                }

                if(bonusSlots.size() > 0)
                {
                    // Randomly add one "guaranteed" slot
                    std::set<size_t> slots;
                    slots.insert(libcomp::Randomizer::GetEntry(bonusSlots));

                    // Randomly add other slots, chances decrease as bonus
                    // increases
                    for(size_t slot : bonusSlots)
                    {
                        if(slots.find(slot) == slots.end() &&
                            1 == RNG(int16_t, 1, (int16_t)(5 +
                                (int16_t)mainItem->GetFuseBonuses(slot))))
                        {
                            slots.insert(slot);
                        }
                    }

                    // Increase all by 1
                    for(size_t slot : slots)
                    {
                        mainItem->SetFuseBonuses(slot, (int8_t)(
                            mainItem->GetFuseBonuses(slot) + 1));
                    }
                }
            }
        }

        // Just save with the other items
        updateItems[mainItem] = 1;

        if(assistItem)
        {
            updateItems[assistItem] = (uint16_t)(
                assistItem->GetStackSize() - 1);
        }

        if(characterManager->UpdateItems(client, false, insertItems,
            updateItems))
        {
            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_EQUIPMENT_SPIRIT_FUSED);
            notify.WriteS32Little(result);
            notify.WriteS64Little(mainID);
            notify.WriteS64Little(basicID);
            notify.WriteS64Little(specialID);
            notify.WriteS64Little(assistID);

            client->QueuePacket(notify);
        }
        else
        {
            LOG_ERROR(libcomp::String("EquipmentSpiritFuse failed"
                " to update items: %1\n")
                .Arg(state->GetAccountUID().ToString()));

            // Roll it back
            mainItem->SetBasicEffect(basicEffectCurrent);
            mainItem->SetSpecialEffect(specialEffectCurrent);
            mainItem->SetFuseBonuses(fuseBonusesCurrent);
            mainItem->SetModSlots(modSlotsCurrent);

            error = true;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_EQUIPMENT_SPIRIT_FUSE);
    reply.WriteS32Little(error ? -1 : 0);
    reply.WriteS64Little(mainID);
    reply.WriteS64Little(basicID);
    reply.WriteS64Little(specialID);
    reply.WriteS64Little(assistID);

    client->SendPacket(reply);

    return true;
}
