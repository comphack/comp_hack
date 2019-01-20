/**
 * @file server/channel/src/DemonState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a partner demon on the channel.
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

#include "DemonState.h"

// libcomp Includes
#include <Constants.h>
#include <DefinitionManager.h>
#include <ScriptEngine.h>
#include <ServerConstants.h>

// object Includes
#include <AccountWorldData.h>
#include <CalculatedEntityState.h>
#include <Character.h>
#include <DemonBox.h>
#include <InheritedSkill.h>
#include <MiDCategoryData.h>
#include <MiDevilBookData.h>
#include <MiDevilBoostExtraData.h>
#include <MiDevilData.h>
#include <MiGrowthData.h>
#include <MiMitamaReunionSetBonusData.h>
#include <MiNPCBasicData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <TokuseiAspect.h>

// libcomp Includes
#include "CharacterManager.h"

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<DemonState>()
    {
        if(!BindingExists("DemonState", true))
        {
            Using<ActiveEntityState>();
            Using<objects::Demon>();

            Sqrat::DerivedClass<DemonState, ActiveEntityState,
                Sqrat::NoConstructor<DemonState>> binding(mVM, "DemonState");
            binding
                .Func<std::shared_ptr<objects::Demon>
                (DemonState::*)() const>(
                    "GetEntity", &DemonState::GetEntity);

            Bind<DemonState>("DemonState", binding);
        }

        return *this;
    }
}

DemonState::DemonState()
{
    mCompendiumCount = 0;
}

uint16_t DemonState::GetCompendiumCount(uint8_t groupID, bool familyGroup)
{
    if(groupID == 0)
    {
        return mCompendiumCount;
    }

    std::lock_guard<std::mutex> lock(mSharedLock);
    if(familyGroup)
    {
        auto it = mCompendiumFamilyCounts.find(groupID);
        return it != mCompendiumFamilyCounts.end() ? it->second : 0;
    }
    else
    {
        auto it = mCompendiumRaceCounts.find(groupID);
        return it != mCompendiumRaceCounts.end() ? it->second : 0;
    }
}

std::list<int32_t> DemonState::GetCompendiumTokuseiIDs() const
{
    return mCompendiumTokuseiIDs;
}

std::list<int32_t> DemonState::GetDemonTokuseiIDs() const
{
    return mDemonTokuseiIDs;
}

bool DemonState::UpdateSharedState(const std::shared_ptr<objects::Character>& character,
    libcomp::DefinitionManager* definitionManager)
{
    std::set<uint32_t> cShiftValues;

    bool compendium2 = CharacterManager::HasValuable(character,
        SVR_CONST.VALUABLE_DEVIL_BOOK_V2);
    bool compendium1 = compendium2 || CharacterManager::HasValuable(character,
        SVR_CONST.VALUABLE_DEVIL_BOOK_V1);

    if(compendium1)
    {
        auto state = ClientState::GetEntityClientState(GetEntityID());
        auto worldData = state ? state->GetAccountWorldData().Get() : nullptr;
        if(worldData)
        {
            auto devilBook = worldData->GetDevilBook();

            const static size_t DBOOK_SIZE = devilBook.size();
            for(size_t i = 0; i < DBOOK_SIZE; i++)
            {
                uint8_t val = devilBook[i];
                for(uint8_t k = 0; k < 8; k++)
                {
                    if((val & (0x01 << k)) != 0)
                    {
                        cShiftValues.insert((uint32_t)((i * 8) + k));
                    }
                }
            }
        }
    }

    // With all shift values read, convert them into distinct entries
    std::set<uint32_t> compendiumEntries;
    std::unordered_map<uint8_t, uint16_t> compendiumFamilyCounts;
    std::unordered_map<uint8_t, uint16_t> compendiumRaceCounts;
    if(cShiftValues.size() > 0)
    {
        for(auto& dbPair : definitionManager->GetDevilBookData())
        {
            auto dBook = dbPair.second;
            if(cShiftValues.find(dBook->GetShiftValue()) !=
                cShiftValues.end() && dBook->GetUnk1() &&
                compendiumEntries.find(dBook->GetEntryID()) ==
                compendiumEntries.end())
            {
                auto devilData = definitionManager->GetDevilData(
                    dBook->GetBaseID1());
                if(devilData)
                {
                    uint8_t familyID = (uint8_t)devilData->GetCategory()
                        ->GetFamily();
                    uint8_t raceID = (uint8_t)devilData->GetCategory()
                        ->GetRace();

                    if(compendiumFamilyCounts.find(familyID) ==
                        compendiumFamilyCounts.end())
                    {
                        compendiumFamilyCounts[familyID] = 1;
                    }
                    else
                    {
                        compendiumFamilyCounts[familyID]++;
                    }

                    if(compendiumRaceCounts.find(raceID) ==
                        compendiumRaceCounts.end())
                    {
                        compendiumRaceCounts[raceID] = 1;
                    }
                    else
                    {
                        compendiumRaceCounts[raceID]++;
                    }
                }

                compendiumEntries.insert(dBook->GetEntryID());
            }
        }
    }

    // Recalculate compendium based tokusei and set count
    std::list<int32_t> compendiumTokuseiIDs;

    if(compendium2 && compendiumEntries.size() > 0)
    {
        for(auto bonusPair : SVR_CONST.DEMON_BOOK_BONUS)
        {
            if(bonusPair.first <= compendiumEntries.size())
            {
                for(int32_t tokuseiID : bonusPair.second)
                {
                    compendiumTokuseiIDs.push_back(tokuseiID);
                }
            }
        }
    }

    std::lock_guard<std::mutex> lock(mSharedLock);
    mCompendiumTokuseiIDs = compendiumTokuseiIDs;
    mCompendiumCount = (uint16_t)compendiumEntries.size();
    mCompendiumFamilyCounts = compendiumFamilyCounts;
    mCompendiumRaceCounts = compendiumRaceCounts;

    return true;
}

bool DemonState::UpdateDemonState(libcomp::DefinitionManager* definitionManager)
{
    auto demon = GetEntity();

    std::lock_guard<std::mutex> lock(mLock);

    mDemonTokuseiIDs.clear();
    mCharacterBonuses.clear();

    if(demon)
    {
        bool updated = false;

        auto state = ClientState::GetEntityClientState(GetEntityID());
        auto cState = state ? state->GetCharacterState() : nullptr;

        std::unordered_map<uint8_t, uint8_t> bonuses;
        std::set<uint32_t> setBonuses;
        if(demon->GetMitamaType() && CharacterManager::GetMitamaBonuses(demon,
            definitionManager, bonuses, setBonuses, false))
        {
            bool exBonus = cState && cState->SkillAvailable(
                SVR_CONST.MITAMA_SET_BOOST);

            for(auto& pair : definitionManager->GetMitamaReunionSetBonusData())
            {
                if(setBonuses.find(pair.first) != setBonuses.end())
                {
                    auto boost = exBonus ? pair.second->GetBonusEx()
                        : pair.second->GetBonus();
                    for(size_t i = 0; i < boost.size(); )
                    {
                        int32_t type = boost[i];
                        int32_t val = boost[(size_t)(i + 1)];
                        if(type == -1 && val)
                        {
                            mDemonTokuseiIDs.push_back(val);
                            updated = true;
                        }

                        i += 2;
                    }
                }
            }
        }

        for(uint16_t stackID : demon->GetForceStack())
        {
            auto exData = stackID
                ? definitionManager->GetDevilBoostExtraData(stackID) : nullptr;
            if(exData)
            {
                for(int32_t tokuseiID : exData->GetTokusei())
                {
                    if(tokuseiID)
                    {
                        mDemonTokuseiIDs.push_back(tokuseiID);
                        updated = true;
                    }
                }
            }
        }

        if(cState)
        {
            // Grant bonus XP based on expertise
            uint8_t fRank = cState->GetExpertiseRank(EXPERTISE_FUSION);
            uint8_t dRank = cState->GetExpertiseRank(EXPERTISE_DEMONOLOGY);

            int16_t xpBoost = (int16_t)(((fRank / 30) * 2) +
                ((dRank / 20) * 2));
            if(xpBoost > 0)
            {
                mCharacterBonuses[CorrectTbl::RATE_XP] = xpBoost;
                updated = true;
            }
        }

        return updated;
    }

    return false;
}

std::list<std::shared_ptr<objects::InheritedSkill>>
    DemonState::GetLearningSkills(uint8_t affinity)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mLearningSkills.find(affinity);
    return it != mLearningSkills.end()
        ? it->second : std::list<std::shared_ptr<objects::InheritedSkill>>();
}

void DemonState::RefreshLearningSkills(uint8_t affinity,
    libcomp::DefinitionManager* definitionManager)
{
    auto demon = GetEntity();
    std::lock_guard<std::mutex> lock(mLock);
    if(affinity == 0)
    {
        // Refresh all
        mLearningSkills.clear();
            
        if(!demon) return;

        for(auto iSkill : demon->GetInheritedSkills())
        {
            if(iSkill->GetProgress() < MAX_INHERIT_SKILL)
            {
                auto iSkillData = definitionManager->GetSkillData(
                    iSkill->GetSkill());
                mLearningSkills[iSkillData->GetCommon()
                    ->GetAffinity()].push_back(iSkill.Get());
            }
        }
    }
    else
    {
        // Refresh specific
        mLearningSkills.erase(affinity);

        // Shouldn't be used this way but whatever
        if(!demon) return;

        for(auto iSkill : demon->GetInheritedSkills())
        {
            if(iSkill->GetProgress() < MAX_INHERIT_SKILL)
            {
                auto iSkillData = definitionManager->GetSkillData(
                    iSkill->GetSkill());
                if(iSkillData->GetCommon()->GetAffinity() == affinity)
                {
                    mLearningSkills[affinity].push_back(iSkill.Get());
                }
            }
        }
    }
}

int16_t DemonState::UpdateLearningSkill(const std::shared_ptr<
    objects::InheritedSkill>& iSkill, uint16_t points)
{
    std::lock_guard<std::mutex> lock(mLock);

    int16_t progress = iSkill->GetProgress();

    // Check new value before casting in case of overflow
    if((progress + points) > MAX_INHERIT_SKILL)
    {
        progress = MAX_INHERIT_SKILL;
    }
    else
    {
        progress = (int16_t)(progress + points);
    }

    iSkill->SetProgress(progress);

    return progress;
}

const libobjgen::UUID DemonState::GetEntityUUID()
{
    auto entity = GetEntity();
    return entity ? entity->GetUUID() : NULLUUID;
}

uint8_t DemonState::RecalculateStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    std::lock_guard<std::mutex> lock(mLock);

    if(calcState == nullptr)
    {
        // Calculating default entity state
        calcState = GetCalculatedState();

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
    }

    auto entity = GetEntity();
    auto cs = GetCoreStats();
    auto devilData = GetDevilData();
    if(!entity || !cs || !devilData)
    {
        return true;
    }

    auto stats = CharacterManager::GetDemonBaseStats(devilData);

    // Non-dependent stats will not change from growth calculation
    stats[CorrectTbl::STR] = cs->GetSTR();
    stats[CorrectTbl::MAGIC] = cs->GetMAGIC();
    stats[CorrectTbl::VIT] = cs->GetVIT();
    stats[CorrectTbl::INT] = cs->GetINTEL();
    stats[CorrectTbl::SPEED] = cs->GetSPEED();
    stats[CorrectTbl::LUCK] = cs->GetLUCK();

    // Set character gained bonuses
    for(auto& pair : mCharacterBonuses)
    {
        stats[pair.first] = (int16_t)(stats[pair.first] + pair.second);
    }

    CharacterManager::AdjustDemonBaseStats(entity, stats, false);

    CharacterManager::AdjustMitamaStats(entity, stats, definitionManager,
        2, GetEntityID());

    auto levelRate = definitionManager->GetDevilLVUpRateData(
        devilData->GetGrowth()->GetGrowthType());
    CharacterManager::FamiliarityBoostStats(entity->GetFamiliarity(), stats,
        levelRate);

    return RecalculateDemonStats(definitionManager, stats, calcState);
}

std::set<uint32_t> DemonState::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei)
{
    std::set<uint32_t> skillIDs;

    auto entity = GetEntity();
    if(entity)
    {
        for(uint32_t skillID : entity->GetLearnedSkills())
        {
            if(skillID)
            {
                skillIDs.insert(skillID);
            }
        }

        auto demonData = GetDevilData();
        for(uint32_t skillID : CharacterManager::GetTraitSkills(entity,
            demonData, definitionManager))
        {
            skillIDs.insert(skillID);
        }

        if(includeTokusei)
        {
            for(uint32_t skillID : GetEffectiveTokuseiSkills(definitionManager))
            {
                skillIDs.insert(skillID);
            }
        }
    }

    return skillIDs;
}

uint8_t DemonState::GetLNCType()
{
    int16_t lncPoints = 0;

    auto entity = GetEntity();
    auto demonData = GetDevilData();
    if(entity && demonData)
    {
        lncPoints = demonData->GetBasic()->GetLNC();
    }

    return CalculateLNCType(lncPoints);
}

int8_t DemonState::GetGender()
{
    auto demonData = GetDevilData();
    if(demonData)
    {
        return (int8_t)demonData->GetBasic()->GetGender();
    }

    return 2;   // None
}

bool DemonState::HasSpecialTDamage()
{
    auto calcState = GetCalculatedState();
    return calcState->ExistingTokuseiAspectsContains((int8_t)
        objects::TokuseiAspect::Type_t::FAMILIARITY_REGEN);
}
