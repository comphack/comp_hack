/**
 * @file server/channel/src/CharacterState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a player character on the channel.
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

#include "CharacterState.h"

// libcomp Includes
#include <Constants.h>
#include <DefinitionManager.h>
#include <ScriptEngine.h>

// objects Includes
#include <CalculatedEntityState.h>
#include <EnchantSetData.h>
#include <EnchantSpecialData.h>
#include <Expertise.h>
#include <Item.h>
#include <MiCategoryData.h>
#include <MiDevilCrystalData.h>
#include <MiEnchantCharasticData.h>
#include <MiEnchantData.h>
#include <MiEquipmentSetData.h>
#include <MiExpertChainData.h>
#include <MiExpertClassData.h>
#include <MiExpertData.h>
#include <MiExpertGrowthTbl.h>
#include <MiExpertRankData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiItemBasicData.h>
#include <MiModificationExtEffectData.h>
#include <MiModifiedEffectData.h>
#include <MiSItemData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpecialConditionData.h>
#include <Tokusei.h>
#include <TokuseiCorrectTbl.h>

// C++ Standard Includes
#include <cmath>

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<CharacterState>()
    {
        if(!BindingExists("CharacterState", true))
        {
            Using<ActiveEntityState>();
            Using<objects::Character>();

            Sqrat::DerivedClass<CharacterState,
                ActiveEntityState> binding(mVM, "CharacterState");
            binding
                .Func("GetEntity", &CharacterState::GetEntity);

            Bind<CharacterState>("CharacterState", binding);
        }

        return *this;
    }
}

CharacterState::CharacterState()
{
}

std::list<int32_t> CharacterState::GetEquipmentTokuseiIDs() const
{
    return mEquipmentTokuseiIDs;
}

std::list<std::shared_ptr<objects::MiSpecialConditionData>>
    CharacterState::GetConditionalTokusei() const
{
    return mConditionalTokusei;
}

void CharacterState::RecalcEquipState(libcomp::DefinitionManager* definitionManager)
{
    auto character = GetEntity();
    if(!character)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mLock);
    mEquipmentTokuseiIDs.clear();
    mConditionalTokusei.clear();
    mStatConditionalTokusei.clear();

    std::set<int16_t> allEffects;
    std::list<std::shared_ptr<objects::MiSpecialConditionData>> conditions;
    std::set<std::shared_ptr<objects::MiEquipmentSetData>> activeEquipSets;
    for(size_t i = 0; i < 15; i++)
    {
        auto equip = character->GetEquippedItems(i).Get();
        if(!equip || equip->GetDurability() == 0) continue;

        // Get item direct effects
        auto sItemData = definitionManager->GetSItemData(equip->GetType());
        if(sItemData)
        {
            for(int32_t tokuseiID : sItemData->GetTokusei())
            {
                mEquipmentTokuseiIDs.push_back(tokuseiID);
            }
        }

        // Check for mod slot effects
        bool isWeapon = i ==
            (size_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON;
        for(size_t k = 0; k < equip->ModSlotsCount(); k++)
        {
            uint16_t effectID = equip->GetModSlots(k);
            if(effectID != 0 && effectID != MOD_SLOT_NULL_EFFECT)
            {
                uint32_t tokuseiID = 0;
                if(isWeapon)
                {
                    auto effectData = definitionManager->GetModifiedEffectData(
                        effectID);
                    tokuseiID = effectData ? effectData->GetTokusei() : 0;
                }
                else
                {
                    auto itemData = definitionManager->GetItemData(
                        equip->GetType());
                    auto effectData = definitionManager->GetModificationExtEffectData(
                        itemData->GetCommon()->GetCategory()->GetSubCategory(),
                        (uint8_t)i, effectID);
                    tokuseiID = effectData ? effectData->GetTokusei() : 0;
                }

                if(tokuseiID != 0)
                {
                    mEquipmentTokuseiIDs.push_back((int32_t)tokuseiID);
                }
            }
        }

        // Gather enchantment effects
        std::unordered_map<bool, int16_t> stMap = {
            { false, equip->GetSoul() },
            { true, equip->GetTarot() }
        };

        for(auto pair : stMap)
        {
            if(pair.second == 0) continue;

            allEffects.insert(pair.second);

            auto enchantData = definitionManager->GetEnchantData(pair.second);
            if(enchantData)
            {
                auto crystalData = enchantData->GetDevilCrystal();
                auto cData = pair.first
                    ? crystalData->GetTarot() : crystalData->GetSoul();
                for(uint32_t tokuseiID : cData->GetTokusei())
                {
                    if(tokuseiID != 0)
                    {
                        mEquipmentTokuseiIDs.push_back((int32_t)tokuseiID);
                    }
                }

                for(auto condition : cData->GetConditions())
                {
                    conditions.push_back(condition);
                }
            }
        }

        // Gather equipment sets
        auto equipSets = definitionManager->GetEquipmentSetDataByItem(
            equip->GetType());
        for(auto s : equipSets)
        {
            bool invalid = activeEquipSets.find(s) != activeEquipSets.end();
            if(!invalid && i > 0)
            {
                // If the set has not already been added by an
                // earlier equipment slot and there are earlier
                // equipment slot pieces needed, ignore the set
                for(size_t k = i; k > 0;  k--)
                {
                    if(s->GetEquipment(k - 1))
                    {
                        invalid = true;
                        break;
                    }
                }
            }

            if(!invalid)
            {
                for(size_t k = i + 1; k < 15; k++)
                {
                    uint32_t sEquip = s->GetEquipment(k);
                    if(sEquip && (!character->GetEquippedItems(k) ||
                        character->GetEquippedItems(k)->GetType() != sEquip))
                    {
                        invalid = true;
                    }
                }

                if(!invalid)
                {
                    activeEquipSets.insert(s);
                }
            }
        }
    }

    // Apply equip sets
    for(auto equippedSet : activeEquipSets)
    {
        for(uint32_t tokuseiID : equippedSet->GetTokusei())
        {
            mEquipmentTokuseiIDs.push_back((int32_t)tokuseiID);
        }
    }

    // Apply enchant sets
    std::set<std::shared_ptr<objects::EnchantSetData>> activeEnchantSets;
    for(int16_t effectID : allEffects)
    {
        auto enchantSets = definitionManager->GetEnchantSetDataByEffect(
            effectID);
        for(auto s : enchantSets)
        {
            bool invalid = activeEnchantSets.find(s) != activeEnchantSets.end()
                && s->EffectsCount() <= allEffects.size();
            if(!invalid)
            {
                for(int16_t setEffectID : s->GetEffects())
                {
                    if(allEffects.find(setEffectID) == allEffects.end())
                    {
                        invalid = true;
                        break;
                    }
                }
            }

            if(!invalid)
            {
                activeEnchantSets.insert(s);

                for(uint32_t tokuseiID : s->GetTokusei())
                {
                    mEquipmentTokuseiIDs.push_back((int32_t)tokuseiID);
                }

                for(auto condition : s->GetConditions())
                {
                    conditions.push_back(condition);
                }
            }
        }
    }

    // Add all conditions to their correct collections
    for(auto condition : conditions)
    {
        switch(condition->GetType())
        {
        case 0:
            // No condition, skip
            break;
        case (int16_t)(10 + (int16_t)CorrectTbl::STR):
        case (int16_t)(10 + (int16_t)CorrectTbl::VIT):
        case (int16_t)(10 + (int16_t)CorrectTbl::INT):
        case (int16_t)(10 + (int16_t)CorrectTbl::SPEED):
        case (int16_t)(10 + (int16_t)CorrectTbl::LUCK):
            // Checked during stat calculation
            mStatConditionalTokusei.push_back(condition);
            break;
        default:
            // Checked during tokusei calculation
            mConditionalTokusei.push_back(condition);
            break;
        }
    }
}

uint8_t CharacterState::GetExpertiseRank(
    libcomp::DefinitionManager* definitionManager, uint32_t expertiseID)
{
    int32_t pointSum = 0;

    auto expData = definitionManager->GetExpertClassData(expertiseID);
    if(expData)
    {
        if(expData->GetIsChain())
        {
            for(uint8_t i = 0; i < expData->GetChainCount(); i++)
            {
                auto chainData = expData->GetChainData((size_t)i);

                float percent = chainData->GetChainPercent();
                if(percent > 0.f)
                {
                    auto exp = GetEntity()->GetExpertises(
                        (size_t)chainData->GetID());
                    pointSum = pointSum + (int32_t)(
                        (float)(exp ? exp->GetPoints() : 0) * percent);
                }
            }
        }
        else
        {
            auto exp = GetEntity()->GetExpertises(
                (size_t)expertiseID);
            pointSum = (int32_t)(exp ? exp->GetPoints() : 0);
        }
    }

    return (uint8_t)floor((float)pointSum * 0.0001f);
}

bool CharacterState::RecalcDisabledSkills(
    libcomp::DefinitionManager* definitionManager)
{
    auto character = GetEntity();
    if(!character)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mLock);

    // Find all skills the character has learned that they do not have the
    // expertise that would grant access to them
    std::set<uint32_t> disabledSkills;
    std::set<uint32_t> currentDisabledSkills = GetDisabledSkills();
    ClearDisabledSkills();

    std::set<uint32_t> learnedSkills = character->GetLearnedSkills();

    bool newSkillDisabled = false;

    uint32_t maxExpertise = (uint32_t)(EXPERTISE_COUNT + CHAIN_EXPERTISE_COUNT);
    for(uint32_t i = 0; i < maxExpertise; i++)
    {
        auto expertData = definitionManager->GetExpertClassData(i);

        if(expertData)
        {
            uint32_t currentRank = GetExpertiseRank(definitionManager, i);

            uint32_t cls = (uint32_t)(currentRank / 10);
            uint32_t rank = (uint32_t)(currentRank % 10);
            for(auto classData : expertData->GetClassData())
            {
                if(classData->GetID() < cls) continue;

                for(uint32_t k = (uint32_t)(rank + 1); k < 10; k++)
                {
                    auto rankData = classData->GetRankData(k);
                    if(rankData)
                    {
                        for(uint32_t skillID : rankData->GetSkill())
                        {
                            if(skillID &&
                                learnedSkills.find(skillID) != learnedSkills.end())
                            {
                                disabledSkills.insert(skillID);
                                newSkillDisabled |= currentDisabledSkills.find(skillID)
                                    == currentDisabledSkills.end();
                            }
                        }
                    }
                }
            }
        }
    }

    SetDisabledSkills(disabledSkills);

    return newSkillDisabled || disabledSkills.size() != currentDisabledSkills.size();
}

void CharacterState::BaseStatsCalculated(libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    libcomp::EnumMap<CorrectTbl, int16_t>& stats,
    std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments)
{
    if(calcState != GetCalculatedState())
    {
        // Do not calculate again, just use the base calculation mode
        ActiveEntityState::BaseStatsCalculated(definitionManager,
            calcState, stats, adjustments);
        return;
    }

    auto effectiveTokusei = calcState->GetEffectiveTokusei();
    auto pendingSkillTokusei = calcState->GetPendingSkillTokusei();

    // Keep track of any additional base stats that need to be adjusted
    // (run-time verified numeric adjust only) based on the current state
    // of the stats
    std::list<std::shared_ptr<objects::MiCorrectTbl>> conditionalStatAdjusts;

    for(auto condition : mStatConditionalTokusei)
    {
        int16_t stat = -1;
        switch(condition->GetType())
        {
        case (int16_t)(10 + (int16_t)CorrectTbl::STR):
        case (int16_t)(10 + (int16_t)CorrectTbl::VIT):
        case (int16_t)(10 + (int16_t)CorrectTbl::INT):
        case (int16_t)(10 + (int16_t)CorrectTbl::SPEED):
        case (int16_t)(10 + (int16_t)CorrectTbl::LUCK):
            stat = stats[(CorrectTbl)(condition->GetType() - 10)];
            break;
        default:
            // Should never happen
            break;
        }

        // If the stat is greater than or equal to the first param
        // the tokusei are active
        if(stat > -1 && (stat >= condition->GetParams(0)))
        {
            for(uint32_t conditionTokusei : condition->GetTokusei())
            {
                int32_t tokuseiID = (int32_t)conditionTokusei;

                auto tokusei = tokuseiID != 0
                    ? definitionManager->GetTokuseiData(tokuseiID)
                    : nullptr;
                if(tokusei)
                {
                    // Update the tokusei maps
                    std::unordered_map<int32_t, uint16_t>* map;
                    if(tokusei->SkillConditionsCount() > 0)
                    {
                        map = &pendingSkillTokusei;
                    }
                    else
                    {
                        map = &effectiveTokusei;
                    }

                    if(map->find(tokuseiID) == map->end())
                    {
                        (*map)[tokuseiID] = 1;
                    }
                    else
                    {
                        (*map)[tokuseiID] = (uint16_t)(
                            (*map)[tokuseiID] + 1);
                    }

                    // Add any correct tbl adjustments
                    for(auto ct : tokusei->GetCorrectValues())
                    {
                        if((uint8_t)ct->GetID() <= (uint8_t)CorrectTbl::LUCK)
                        {
                            conditionalStatAdjusts.push_back(ct);
                        }
                        else
                        {
                            adjustments.push_back(ct);
                        }
                    }

                    for(auto ct : tokusei->GetTokuseiCorrectValues())
                    {
                        if((uint8_t)ct->GetID() <= (uint8_t)CorrectTbl::LUCK)
                        {
                            conditionalStatAdjusts.push_back(ct);
                        }
                        else
                        {
                            adjustments.push_back(ct);
                        }
                    }
                }
            }
        }
    }

    calcState->SetEffectiveTokuseiFinal(effectiveTokusei);
    calcState->SetPendingSkillTokuseiFinal(pendingSkillTokusei);

    if(conditionalStatAdjusts.size() > 0)
    {
        AdjustStats(conditionalStatAdjusts, stats, calcState, true);
    }
}
