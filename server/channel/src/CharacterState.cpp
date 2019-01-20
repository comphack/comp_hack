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
#include <ServerConstants.h>

// objects Includes
#include <AccountWorldData.h>
#include <CalculatedEntityState.h>
#include <CharacterProgress.h>
#include <Clan.h>
#include <DigitalizeState.h>
#include <EnchantSetData.h>
#include <EnchantSpecialData.h>
#include <EventCounter.h>
#include <Expertise.h>
#include <Item.h>
#include <MiCategoryData.h>
#include <MiDCategoryData.h>
#include <MiDevilBoostExtraData.h>
#include <MiDevilCrystalData.h>
#include <MiDevilData.h>
#include <MiDevilEquipmentItemData.h>
#include <MiEnchantCharasticData.h>
#include <MiEnchantData.h>
#include <MiEquipmentSetData.h>
#include <MiExpertChainData.h>
#include <MiExpertClassData.h>
#include <MiExpertData.h>
#include <MiExpertGrowthTbl.h>
#include <MiExpertRankData.h>
#include <MiGrowthData.h>
#include <MiGuardianAssistData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiItemBasicData.h>
#include <MiModificationExtEffectData.h>
#include <MiModifiedEffectData.h>
#include <MiQuestData.h>
#include <MiSItemData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpecialConditionData.h>
#include <MiUseRestrictionsData.h>
#include <Tokusei.h>
#include <TokuseiCorrectTbl.h>

// C++ Standard Includes
#include <cmath>

// channel Includes
#include <CharacterManager.h>

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
            Using<objects::DigitalizeState>();
            Using<objects::EventCounter>();

            Sqrat::DerivedClass<CharacterState, ActiveEntityState,
                Sqrat::NoConstructor<CharacterState>> binding(mVM, "CharacterState");
            binding
                .Func<std::shared_ptr<objects::Character>
                (CharacterState::*)() const>(
                    "GetEntity", &CharacterState::GetEntity)
                .Func("GetDigitalizeState",
                    &CharacterState::GetDigitalizeState)
                .Func("GetEventCounter",
                    &CharacterState::GetEventCounter)
                .Func("ActionCooldownActive",
                    &CharacterState::ActionCooldownActive)
                .Func("RefreshActionCooldowns",
                    &CharacterState::RefreshActionCooldowns);

            Bind<CharacterState>("CharacterState", binding);
        }

        return *this;
    }
}

CharacterState::CharacterState() : mNextEquipmentExpiration(0),
    mQuestBonusCount(0), mMaxFusionGaugeStocks(0)
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

uint32_t CharacterState::GetQuestBonusCount() const
{
    return mQuestBonusCount;
}

uint8_t CharacterState::GetMaxFusionGaugeStocks() const
{
    return mMaxFusionGaugeStocks;
}

std::list<int32_t> CharacterState::GetQuestBonusTokuseiIDs() const
{
    return mQuestBonusTokuseiIDs;
}

std::shared_ptr<objects::DigitalizeState>
    CharacterState::GetDigitalizeState() const
{
    return mDigitalizeState;
}

std::shared_ptr<objects::DigitalizeState> CharacterState::Digitalize(
    const std::shared_ptr<objects::Demon>& demon,
    libcomp::DefinitionManager* definitionManager)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto devilData = demon
        ? definitionManager->GetDevilData(demon->GetType()) : nullptr;
    if(!devilData)
    {
        mDigitalizeState = nullptr;
        return nullptr;
    }

    uint8_t raceID = (uint8_t)devilData->GetCategory()->GetRace();
    mDigitalizeState = std::make_shared<objects::DigitalizeState>();
    mDigitalizeState->SetDemon(demon);
    mDigitalizeState->SetRaceID(raceID);

    uint8_t dgAbilty = GetDigitalizeAbilityLevel();
    uint8_t statRate = (uint8_t)(dgAbilty == 2 ? 30 : 10);

    // Gather active assist values
    std::list<std::shared_ptr<objects::MiGuardianAssistData>> activeAssists;

    auto character = GetEntity();
    auto progress = character
        ? character->GetProgress().Get() : nullptr;
    if(progress)
    {
        auto assists = progress->GetDigitalizeAssists();
        for(size_t i = 0; i < assists.size(); i++)
        {
            uint8_t byte = assists[i];
            if(byte == 0) continue;

            for(size_t k = 0; k < 8; k++)
            {
                if(byte & (1 << k))
                {
                    auto assist = definitionManager->GetGuardianAssistData(
                        (uint32_t)(i * 8 + k));
                    if(assist && assist->GetRaceID() == raceID)
                    {
                        activeAssists.push_back(assist);
                    }
                }
            }
        }
    }

    // Adjust assist properties
    bool skillActives = false, skillPassives = false, skillTraits = false,
        affinities = false, mitamaSet = false;
    for(auto assist : activeAssists)
    {
        switch(assist->GetType())
        {
        case objects::MiGuardianAssistData::Type_t::STAT_RATE:
            if((statRate + assist->GetValue()) > 100)
            {
                statRate = 100;
            }
            else
            {
                statRate = (uint8_t)(statRate + assist->GetValue());
            }
            break;
        case objects::MiGuardianAssistData::Type_t::ACTIVES:
            skillActives = true;
            break;
        case objects::MiGuardianAssistData::Type_t::PASSIVES:
            skillPassives = true;
            break;
        case objects::MiGuardianAssistData::Type_t::TRAITS:
            skillTraits = true;
            break;
        case objects::MiGuardianAssistData::Type_t::AFFINITIES:
            affinities = true;
            break;
        case objects::MiGuardianAssistData::Type_t::FORCE_STACK:
            for(uint16_t stackID : demon->GetForceStack())
            {
                auto exData = stackID
                    ? definitionManager->GetDevilBoostExtraData(stackID)
                    : nullptr;
                if(exData)
                {
                    for(int32_t tokuseiID : exData->GetTokusei())
                    {
                        if(tokuseiID)
                        {
                            mDigitalizeState->AppendTokuseiIDs(tokuseiID);
                        }
                    }
                }
            }
            break;
        case objects::MiGuardianAssistData::Type_t::MITAMA_SET:
            mitamaSet = true;
            break;
        case objects::MiGuardianAssistData::Type_t::EXTEND_TIME:
            mDigitalizeState->SetTimeExtension(
                mDigitalizeState->GetTimeExtension() + assist->GetValue());
            break;
        case objects::MiGuardianAssistData::Type_t::REDUCE_WAIT:
            mDigitalizeState->SetCooldownReduction(
                mDigitalizeState->GetCooldownReduction() + assist->GetValue());
            break;
        default:
            break;
        }
    }

    // Add skills
    if(skillActives || skillPassives)
    {
        for(uint32_t skillID : demon->GetLearnedSkills())
        {
            auto skillData = skillID
                ? definitionManager->GetSkillData(skillID) : nullptr;
            if(skillData)
            {
                switch(skillData->GetCommon()->GetCategory()
                    ->GetMainCategory())
                {
                case 0:
                    if(skillPassives)
                    {
                        mDigitalizeState->InsertActiveSkills(skillID);
                    }
                    break;
                case 1:
                    if(skillActives)
                    {
                        mDigitalizeState->InsertPassiveSkills(skillID);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }

    if(skillTraits)
    {
        for(uint32_t skillID : CharacterManager::GetTraitSkills(demon,
            devilData, definitionManager))
        {
            mDigitalizeState->InsertPassiveSkills(skillID);
        }
    }

    // Calculate and add stats
    mDigitalizeState->SetStatRate(statRate);

    int8_t demonLvl = demon->GetCoreStats()->GetLevel();
    auto demonStats = CharacterManager::GetDemonBaseStats(devilData,
        definitionManager, demon->GetGrowthType(), demonLvl);

    CharacterManager::AdjustDemonBaseStats(demon, demonStats, true, true);
    CharacterManager::AdjustMitamaStats(demon, demonStats, definitionManager,
        0, 0, mitamaSet);

    // Add base stats and HP/MP
    for(uint8_t i = (size_t)CorrectTbl::STR; i <= (uint8_t)CorrectTbl::MP_MAX; i++)
    {
        mDigitalizeState->SetCorrectValues(i, (int16_t)(
            (double)statRate * 0.01 * (double)demonStats[(CorrectTbl)i]));
    }

    if(affinities)
    {
        // Add affinities
        for(uint8_t i = (size_t)CorrectTbl::RES_DEFAULT;
            i <= (uint8_t)CorrectTbl::NRA_MAGIC; i++)
        {
            mDigitalizeState->SetCorrectValues(i, demonStats[(CorrectTbl)i]);
        }
    }

    return mDigitalizeState;
}

uint8_t CharacterState::GetDigitalizeAbilityLevel()
{
    auto character = GetEntity();

    if(CharacterManager::HasValuable(character,
        SVR_CONST.VALUABLE_DIGITALIZE_LV2))
    {
        return 2;
    }
    else if(CharacterManager::HasValuable(character,
        SVR_CONST.VALUABLE_DIGITALIZE_LV1))
    {
        return 1;
    }

    // Digitalize not unlocked
    return 0;
}

void CharacterState::RecalcEquipState(
    libcomp::DefinitionManager* definitionManager)
{
    auto character = GetEntity();
    if(!character)
    {
        return;
    }

    // Keep track of the current system time for expired equipment
    uint32_t now = (uint32_t)std::time(0);

    std::lock_guard<std::mutex> lock(mLock);
    mEquipmentTokuseiIDs.clear();
    mConditionalTokusei.clear();
    mEquipFuseBonuses.clear();

    mNextEquipmentExpiration = 0;

    uint8_t maxStocks = CharacterManager::HasValuable(character,
        SVR_CONST.VALUABLE_FUSION_GAUGE) ? 1 : 0;

    std::set<int16_t> allEffects;
    std::list<std::shared_ptr<objects::MiSpecialConditionData>> conditions;
    std::set<std::shared_ptr<objects::MiEquipmentSetData>> activeEquipSets;
    for(size_t i = 0; i < 15; i++)
    {
        auto equip = character->GetEquippedItems(i).Get();
        if(!equip || equip->GetDurability() == 0) continue;

        uint32_t expiration = equip->GetRentalExpiration();
        if(expiration)
        {
            // No bonuses if its expired
            if(expiration <= now) continue;

            if(!mNextEquipmentExpiration ||
                expiration <= mNextEquipmentExpiration)
            {
                mNextEquipmentExpiration = expiration;
            }
        }

        auto itemData = definitionManager->GetItemData(
            equip->GetType());

        maxStocks = (uint8_t)(maxStocks + itemData->GetRestriction()
            ->GetStock());

        // Get item direct effects
        uint32_t specialEffect = equip->GetSpecialEffect();
        for(int32_t tokuseiID : definitionManager->GetSItemTokusei(
            specialEffect ? specialEffect : equip->GetType()))
        {
            mEquipmentTokuseiIDs.push_back(tokuseiID);
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

        AdjustFuseBonus(definitionManager, equip);
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
        if(condition->GetType() != 0)
        {
            mConditionalTokusei.push_back(condition);
        }
        else
        {
            // In some instances the conditional tokusei are used
            // as an additional effect section
            for(uint32_t tokuseiID : condition->GetTokusei())
            {
                if(tokuseiID)
                {
                    mEquipmentTokuseiIDs.push_back((int32_t)tokuseiID);
                }
            }
        }
    }

    mMaxFusionGaugeStocks = maxStocks;
}

bool CharacterState::EquipmentExpired(uint32_t now)
{
    std::lock_guard<std::mutex> lock(mLock);
    return mNextEquipmentExpiration && mNextEquipmentExpiration <= now;
}

bool CharacterState::UpdateQuestState(
    libcomp::DefinitionManager* definitionManager, uint32_t completedQuestID)
{
    auto character = GetEntity();
    auto progress = character ? character->GetProgress().Get() : nullptr;
    if(!progress)
    {
        return false;
    }

    uint32_t questBonusCount = mQuestBonusCount;
    if(completedQuestID)
    {
        size_t index;
        uint8_t shiftVal;
        CharacterManager::ConvertIDToMaskValues((uint16_t)completedQuestID,
            index, shiftVal);

        uint8_t indexVal = progress->GetCompletedQuests(index);
        bool completed = (shiftVal & indexVal) != 0;
        if(completed)
        {
            // Nothing new
            return false;
        }

        progress->SetCompletedQuests(index, (uint8_t)(shiftVal | indexVal));

        // Only quest types 0 and 1 apply bonuses (client SHOULD check
        // the bonus enabled flag but some others are enabled)
        auto questData = definitionManager->GetQuestData(completedQuestID);
        if(!questData || questData->GetType() > 1)
        {
            return false;
        }

        questBonusCount++;
    }
    else
    {
        questBonusCount = 0;

        uint32_t questID = 0;
        for(uint8_t qBlock : progress->GetCompletedQuests())
        {
            for(uint8_t i = 0; i < 8; i++)
            {
                if((qBlock & (0x01 << i)) != 0)
                {
                    auto questData = definitionManager->GetQuestData(questID);
                    if(questData && questData->GetType() <= 1)
                    {
                        questBonusCount++;
                    }
                }

                questID++;
            }
        }

    }

    if(questBonusCount != mQuestBonusCount)
    {
        // Recalculate quest based tokusei and set count
        std::list<int32_t> questBonusTokuseiIDs;

        for(auto bonusPair : SVR_CONST.QUEST_BONUS)
        {
            if(bonusPair.first <= questBonusCount)
            {
                questBonusTokuseiIDs.push_back(bonusPair.second);
            }
        }

        std::lock_guard<std::mutex> lock(mLock);
        mQuestBonusTokuseiIDs = questBonusTokuseiIDs;
        mQuestBonusCount = questBonusCount;

        return true;
    }

    return false;
}

int32_t CharacterState::GetExpertisePoints(
    uint32_t expertiseID, libcomp::DefinitionManager* definitionManager)
{
    int32_t pointSum = 0;

    auto expData = definitionManager
        ? definitionManager->GetExpertClassData(expertiseID) : nullptr;
    if(expData && expData->GetIsChain())
    {
        // Calculated chain expertise
        for(uint8_t i = 0; i < expData->GetChainCount(); i++)
        {
            auto chainData = expData->GetChainData((size_t)i);
            if(GetExpertiseRank(chainData->GetID(), definitionManager) <
                chainData->GetRankRequired())
            {
                // Chain expertise is not active
                return 0;
            }

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
        // Get as non-chain
        auto exp = GetEntity()->GetExpertises(
            (size_t)expertiseID);
        pointSum = (int32_t)(exp ? exp->GetPoints() : 0);
    }

    return pointSum;
}

uint8_t CharacterState::GetExpertiseRank(
    uint32_t expertiseID, libcomp::DefinitionManager* definitionManager)
{
    return (uint8_t)(GetExpertisePoints(expertiseID, definitionManager) /
        10000);
}

bool CharacterState::ActionCooldownActive(int32_t cooldownID,
    bool accountLevel, bool refresh)
{
    if(refresh)
    {
        RefreshActionCooldowns(accountLevel);
    }
    
    if(accountLevel)
    {
        // Account level
        auto state = ClientState::GetEntityClientState(GetEntityID());
        auto awd = state ? state->GetAccountWorldData().Get() : nullptr;
        return awd && awd->ActionCooldownsKeyExists(cooldownID);
    }
    else
    {
        // Character level
        auto character = GetEntity();
        return character && character->ActionCooldownsKeyExists(cooldownID);
    }
}

std::shared_ptr<objects::EventCounter> CharacterState::GetEventCounter(
    int32_t type)
{
    auto state = ClientState::GetEntityClientState(GetEntityID());
    return state ? state->GetEventCounters(type).Get() : nullptr;
}

void CharacterState::RefreshActionCooldowns(bool accountLevel, uint32_t time)
{
    if(!time)
    {
        time = (uint32_t)std::time(0);
    }

    std::set<int32_t> removes;

    if(accountLevel)
    {
        // Account level
        auto state = ClientState::GetEntityClientState(GetEntityID());
        auto awd = state ? state->GetAccountWorldData().Get() : nullptr;
        if(awd)
        {
            std::lock_guard<std::mutex> lock(mLock);
            for(auto pair : awd->GetActionCooldowns())
            {
                if(pair.second <= time)
                {
                    removes.insert(pair.first);
                }
            }

            for(int32_t remove : removes)
            {
                awd->RemoveActionCooldowns(remove);
            }
        }
    }
    else
    {
        // Character level
        auto character = GetEntity();
        if(character)
        {
            std::lock_guard<std::mutex> lock(mLock);
            for(auto pair : character->GetActionCooldowns())
            {
                if(pair.second <= time)
                {
                    removes.insert(pair.first);
                }
            }

            for(int32_t remove : removes)
            {
                character->RemoveActionCooldowns(remove);
            }
        }
    }
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

        if(!expertData) continue;

        uint32_t currentRank = GetExpertiseRank(i, definitionManager);

        uint32_t rank = 0;
        for(auto classData : expertData->GetClassData())
        {
            for(auto rankData : classData->GetRankData())
            {
                if(rank > currentRank)
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

                rank++;
            }
        }
    }

    SetDisabledSkills(disabledSkills);

    return newSkillDisabled || disabledSkills.size() != currentDisabledSkills.size();
}

const libobjgen::UUID CharacterState::GetEntityUUID()
{
    auto entity = GetEntity();
    return entity ? entity->GetUUID() : NULLUUID;
}

uint8_t CharacterState::RecalculateStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    uint8_t result = 0;

    std::lock_guard<std::mutex> lock(mLock);

    auto c = GetEntity();
    auto cs = GetCoreStats();

    bool selfState = calcState == nullptr;
    if(selfState)
    {
        calcState = GetCalculatedState();

        // Calculate current skills, only matters if calculating
        // for the default entity state
        auto previousSkills = GetCurrentSkills();
        SetCurrentSkills(GetAllSkills(definitionManager, true));

        bool skillsChanged = previousSkills.size() != CurrentSkillsCount();
        if(!skillsChanged)
        {
            for(uint32_t skillID : previousSkills)
            {
                if(!CurrentSkillsContains(skillID))
                {
                    skillsChanged = true;
                    break;
                }
            }
        }
        result = skillsChanged ? ENTITY_CALC_SKILL : 0x00;

        // Remove any switch skills no longer available
        RemoveInactiveSwitchSkills();
    }

    auto stats = CharacterManager::GetCharacterBaseStats(cs);

    // Adjust base stats based on digitalize
    auto dgState = mDigitalizeState;
    if(dgState)
    {
        for(auto& pair : dgState->GetCorrectValues())
        {
            stats[(CorrectTbl)pair.first] = (int16_t)(
                stats[(CorrectTbl)pair.first] + pair.second);
        }
    }

    if(selfState)
    {
        // Combat run speed can change from unadjusted stats (nothing
        // natively does this)
        SetCombatRunSpeed(stats[CorrectTbl::MOVE2]);

        if(!mInitialCalc)
        {
            SetKnockbackResist((float)stats[CorrectTbl::KNOCKBACK_RESIST]);
            mInitialCalc = true;
        }
    }

    // Keep track of the current system time for expired equipment
    uint32_t now = (uint32_t)std::time(0);

    // Calculate based on adjustments
    std::list<std::shared_ptr<objects::MiCorrectTbl>> correctTbls;
    std::list<std::shared_ptr<objects::MiCorrectTbl>> nraTbls;
    for(auto equip : c->GetEquippedItems())
    {
        if(!equip.IsNull() && equip->GetDurability() > 0 &&
            (!equip->GetRentalExpiration() || now < equip->GetRentalExpiration()))
        {
            uint32_t basicEffect = equip->GetBasicEffect();
            auto itemData = definitionManager->GetItemData(
                basicEffect ? basicEffect : equip->GetType());
            for(auto ct : itemData->GetCommon()->GetCorrectTbl())
            {
                if((uint8_t)ct->GetID() >= (uint8_t)CorrectTbl::NRA_WEAPON &&
                    (uint8_t)ct->GetID() <= (uint8_t)CorrectTbl::NRA_MAGIC)
                {
                    nraTbls.push_back(ct);
                }
                else
                {
                    correctTbls.push_back(ct);
                }
            }
        }
    }

    GetAdditionalCorrectTbls(definitionManager, calcState, correctTbls);

    UpdateNRAChances(stats, calcState, nraTbls);
    AdjustStats(correctTbls, stats, calcState, true);

    // Base stats calcualted, Apply equipment fusion bonuses now
    for(auto& pair : mEquipFuseBonuses)
    {
        stats[pair.first] = (int16_t)(stats[pair.first] + pair.second);
    }

    CharacterManager::CalculateDependentStats(stats, cs->GetLevel(), false);

    if(selfState)
    {
        result = result | CompareAndResetStats(stats, true);
    }

    AdjustStats(correctTbls, stats, calcState, false);

    if(GetStatusTimes(STATUS_RESTING))
    {
        // Apply (originally busted) Medical Sciences bonus of 10% more
        // regen per class
        uint8_t cls = (uint8_t)(GetExpertiseRank(EXPERTISE_MEDICAL_SCIENCES) /
            10);
        if(cls)
        {
            stats[CorrectTbl::HP_REGEN] = (int16_t)((double)
                stats[CorrectTbl::HP_REGEN] * (1.0 + 0.1 * (double)cls));
            stats[CorrectTbl::MP_REGEN] = (int16_t)((double)
                stats[CorrectTbl::MP_REGEN] * (1.0 + 0.1 * (double)cls));
        }
    }

    if(selfState)
    {
        return result | CompareAndResetStats(stats, false);
    }
    else
    {
        for(auto statPair : stats)
        {
            calcState->SetCorrectTbl((size_t)statPair.first, statPair.second);
        }

        return result;
    }
}

std::set<uint32_t> CharacterState::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei)
{
    std::set<uint32_t> skillIDs;
    
    auto character = GetEntity();
    if(character)
    {
        skillIDs = character->GetLearnedSkills();

        auto clan = character->GetClan().Get();
        if(clan)
        {
            int8_t clanLevel = clan->GetLevel();
            for(int8_t i = 0; i < clanLevel; i++)
            {
                for(uint32_t clanSkillID :
                    SVR_CONST.CLAN_LEVEL_SKILLS[(size_t)i])
                {
                    skillIDs.insert(clanSkillID);
                }
            }
        }

        if(includeTokusei)
        {
            for(uint32_t skillID : GetEffectiveTokuseiSkills(
                definitionManager))
            {
                skillIDs.insert(skillID);
            }
        }
    }

    auto dgState = mDigitalizeState;
    if(dgState)
    {
        for(uint32_t skillID : dgState->GetActiveSkills())
        {
            skillIDs.insert(skillID);
        }

        for(uint32_t skillID : dgState->GetPassiveSkills())
        {
            skillIDs.insert(skillID);
        }
    }

    return skillIDs;
}

uint8_t CharacterState::GetLNCType()
{
    auto entity = GetEntity();
    return CalculateLNCType(entity ? entity->GetLNC() : 0);
}

int8_t CharacterState::GetGender()
{
    auto entity = GetEntity();
    return entity ? (int8_t)entity->GetGender() : 2;
}

void CharacterState::AdjustFuseBonus(libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::Item> equipment)
{
    const size_t GROWTH_TABLE_SIZE = 16;

    // Default growth table, base values padded to match largest
    // needed table (weapon)
    const static int16_t minorGrowth[GROWTH_TABLE_SIZE][2] = {
        { 0, 1 },
        { 0, 1 },
        { 0, 1 },
        { 0, 1 },
        { 0, 1 },
        { 0, 1 },
        { 0, 1 },
        { 5, 2 },
        { 10, 3 },
        { 15, 4 },
        { 20, 5 },
        { 25, 7 },
        { 30, 10 },
        { 35, 13 },
        { 40, 16 },
        { 50, 20 }
    };

    int16_t correctTypes[] = { -1, -1, -1 };
    const int16_t (*growthTable)[GROWTH_TABLE_SIZE][2] = &minorGrowth;

    auto itemData = definitionManager->GetItemData(
        equipment->GetType());
    switch(itemData->GetBasic()->GetEquipType())
    {
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON:
        {
            const static int16_t growth[GROWTH_TABLE_SIZE][2] = {
                { 2, 2 },
                { 4, 3 },
                { 6, 4 },
                { 8, 5 },
                { 10, 6 },
                { 12, 7 },
                { 14, 8 },
                { 16, 9 },
                { 18, 12 },
                { 21, 15 },
                { 24, 20 },
                { 27, 25 },
                { 30, 30 },
                { 35, 35 },
                { 40, 40 },
                { 50, 45 }
            };

            // CLSR or LNGR based on weapon type
            if(itemData->GetBasic()->GetWeaponType() ==
                objects::MiItemBasicData::WeaponType_t::CLOSE_RANGE)
            {
                correctTypes[0] = (int8_t)CorrectTbl::CLSR;
            }
            else
            {
                correctTypes[0] = (int8_t)CorrectTbl::LNGR;
            }

            correctTypes[1] = (int8_t)CorrectTbl::SPELL;
            correctTypes[2] = (int8_t)CorrectTbl::SUPPORT;

            growthTable = &growth;
        }
        break;
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM:
        {
            const static int16_t growth[GROWTH_TABLE_SIZE][2] = {
                { 0, 1 },
                { 0, 1 },
                { 0, 1 },
                { 0, 1 },
                { 0, 1 },
                { 0, 1 },
                { 0, 1 },
                { 5, 2 },
                { 10, 3 },
                { 15, 5 },
                { 20, 7 },
                { 25, 10 },
                { 30, 13 },
                { 35, 16 },
                { 40, 19 },
                { 50, 25 }
            };

            correctTypes[0] = (int8_t)CorrectTbl::PDEF;
            correctTypes[1] = (int8_t)CorrectTbl::MDEF;

            growthTable = &growth;
        }
        break;
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_HEAD:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_ARMS:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FEET:
        correctTypes[0] = (int8_t)CorrectTbl::PDEF;
        correctTypes[1] = (int8_t)CorrectTbl::MDEF;
        break;
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_RING:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EARRING:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EXTRA:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TALISMAN:
        correctTypes[1] = (int8_t)CorrectTbl::MDEF;
        break;
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FACE:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NECK:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_COMP:
    case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BACK:
    default:
        // No bonuses
        return;
    }

    // Apply bonuses
    for(size_t i = 0; i < 3; i++)
    {
        int8_t bonus = equipment->GetFuseBonuses(i);
        if(bonus > 0 && correctTypes[i] != -1)
        {
            int16_t boost = 0;
            for(size_t k = 0; k < GROWTH_TABLE_SIZE; k++)
            {
                if((*growthTable)[k][0] == (int16_t)bonus)
                {
                    boost = (*growthTable)[k][1];
                    break;
                }
                else if((*growthTable)[k][0] > (int16_t)bonus)
                {
                    if(k > 0)
                    {
                        // Use previous
                        boost = (*growthTable)[(size_t)(k - 1)][1];
                    }
                    else
                    {
                        // Default to 1
                        boost = 1;
                    }
                    break;
                }
            }

            CorrectTbl correctType = (CorrectTbl)correctTypes[i];
            mEquipFuseBonuses[correctType] = (int16_t)(
                mEquipFuseBonuses[correctType] + boost);
        }
    }
}
