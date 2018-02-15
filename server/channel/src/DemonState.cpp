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

#include "DemonState.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <ScriptEngine.h>
#include <ServerConstants.h>

// object Includes
#include <Character.h>
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <MiDevilBookData.h>

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
                .Func<std::shared_ptr<objects::Demon>
                    (ActiveEntityStateImp<objects::Demon>::*)()>(
                    "GetEntity", &ActiveEntityStateImp<objects::Demon>::GetEntity);

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
