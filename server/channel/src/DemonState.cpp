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
#include <Character.h>
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <InheritedSkill.h>
#include <MiDevilBookData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>

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

            Sqrat::DerivedClass<DemonState,
                ActiveEntityState> binding(mVM, "DemonState");
            binding
                .Func("GetEntity", &ActiveEntityStateImp<objects::Demon>::GetEntity);

            Bind<DemonState>("DemonState", binding);
        }

        return *this;
    }
}

DemonState::DemonState()
{
    mCompendiumCount = 0;
}

uint32_t DemonState::GetCompendiumCount() const
{
    return mCompendiumCount;
}

std::list<int32_t> DemonState::GetCompendiumTokuseiIDs() const
{
    return mCompendiumTokuseiIDs;
}

bool DemonState::UpdateSharedState(const std::shared_ptr<objects::Character>& character,
    libcomp::DefinitionManager* definitionManager)
{
    std::set<uint32_t> cShiftValues;

    if(character)
    {
        auto devilBook = character->GetProgress()->GetDevilBook();

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

    // With all shift values read, convert them into distinct entries
    std::set<uint32_t> compendiumEntries;
    if(cShiftValues.size() > 0)
    {
        size_t read = 0;
        for(auto dbPair : definitionManager->GetDevilBookData())
        {
            if(cShiftValues.find(dbPair.second->GetShiftValue()) !=
                cShiftValues.end())
            {
                compendiumEntries.insert(dbPair.second->GetEntryID());
                read++;

                if(read >= cShiftValues.size())
                {
                    break;
                }
            }
        }
    }

    if(mCompendiumTokuseiIDs.size() > 0 || compendiumEntries.size() > 0)
    {
        // Recalculate compendium based tokusei and set count
        std::list<int32_t> compendiumTokuseiIDs;

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

        std::lock_guard<std::mutex> lock(mLock);
        mCompendiumTokuseiIDs = compendiumTokuseiIDs;
        mCompendiumCount = (uint32_t)compendiumEntries.size();
    }

    return true;
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
    progress = (int16_t)(progress + points);

    if(progress > MAX_INHERIT_SKILL)
    {
        progress = MAX_INHERIT_SKILL;
    }

    iSkill->SetProgress(progress);

    return progress;
}
