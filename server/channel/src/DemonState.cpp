/**
 * @file server/channel/src/DemonState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a demon on the channel.
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

// objects Includes
#include <Demon.h>
#include <EntityStats.h>
#include <MiDevilBattleData.h>
#include <MiDevilData.h>

// channel Includes
#include <CharacterManager.h>
#include <DefinitionManager.h>

using namespace channel;

DemonState::DemonState() : objects::DemonStateObject()
{
}

DemonState::~DemonState()
{
}

bool DemonState::RecalculateStats(libcomp::DefinitionManager* definitionManager)
{
    auto d = GetDemon().Get();
    if(nullptr == d)
    {
        return true;
    }

    auto cs = d->GetCoreStats().Get();
    auto demonData = definitionManager->GetDevilData(d->GetType());
    auto battleData = demonData->GetBattleData();
    
    std::unordered_map<uint8_t, int16_t> correctMap;
    correctMap[libcomp::CORRECT_STR] = cs->GetSTR();
    correctMap[libcomp::CORRECT_MAGIC] = cs->GetMAGIC();
    correctMap[libcomp::CORRECT_VIT] = cs->GetVIT();
    correctMap[libcomp::CORRECT_INTEL] = cs->GetINTEL();
    correctMap[libcomp::CORRECT_SPEED] = cs->GetSPEED();
    correctMap[libcomp::CORRECT_LUCK] = cs->GetLUCK();
    correctMap[libcomp::CORRECT_MAXHP] = battleData->GetCorrect(libcomp::CORRECT_MAXHP);
    correctMap[libcomp::CORRECT_MAXMP] = battleData->GetCorrect(libcomp::CORRECT_MAXMP);
    correctMap[libcomp::CORRECT_CLSR] = battleData->GetCorrect(libcomp::CORRECT_CLSR);
    correctMap[libcomp::CORRECT_LNGR] = battleData->GetCorrect(libcomp::CORRECT_LNGR);
    correctMap[libcomp::CORRECT_SPELL] = battleData->GetCorrect(libcomp::CORRECT_SPELL);
    correctMap[libcomp::CORRECT_SUPPORT] = battleData->GetCorrect(libcomp::CORRECT_SUPPORT);
    correctMap[libcomp::CORRECT_PDEF] = battleData->GetCorrect(libcomp::CORRECT_PDEF);
    correctMap[libcomp::CORRECT_MDEF] = battleData->GetCorrect(libcomp::CORRECT_MDEF);

    /// @todo: apply effects

    CharacterManager::CalculateDependentStats(correctMap, cs->GetLevel(), true);

    SetMaxHP(correctMap[libcomp::CORRECT_MAXHP]);
    SetMaxMP(correctMap[libcomp::CORRECT_MAXMP]);
    SetSTR(correctMap[libcomp::CORRECT_STR]);
    SetMAGIC(correctMap[libcomp::CORRECT_MAGIC]);
    SetVIT(correctMap[libcomp::CORRECT_VIT]);
    SetINTEL(correctMap[libcomp::CORRECT_INTEL]);
    SetSPEED(correctMap[libcomp::CORRECT_SPEED]);
    SetLUCK(correctMap[libcomp::CORRECT_LUCK]);
    SetCLSR(correctMap[libcomp::CORRECT_CLSR]);
    SetLNGR(correctMap[libcomp::CORRECT_LNGR]);
    SetSPELL(correctMap[libcomp::CORRECT_SPELL]);
    SetSUPPORT(correctMap[libcomp::CORRECT_SUPPORT]);
    SetPDEF(correctMap[libcomp::CORRECT_PDEF]);
    SetMDEF(correctMap[libcomp::CORRECT_MDEF]);

    return true;
}

bool DemonState::Ready()
{
    return !GetDemon().IsNull();
}
