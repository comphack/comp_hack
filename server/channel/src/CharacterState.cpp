/**
 * @file server/channel/src/CharacterState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a character on the channel.
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

#include "CharacterState.h"

// libcomp Includes
#include <Constants.h>

// objects Includes
#include <Character.h>
#include <EntityStats.h>
#include <Item.h>
#include <MiCorrectTbl.h>
#include <MiItemData.h>
#include <MiSkillItemStatusCommonData.h>

// channel Includes
#include <CharacterManager.h>
#include <DefinitionManager.h>

using namespace channel;

CharacterState::CharacterState() : objects::CharacterStateObject()
{
}

CharacterState::~CharacterState()
{
}

bool CharacterState::RecalculateStats(libcomp::DefinitionManager* definitionManager)
{
    auto c = GetCharacter().Get();
    auto cs = c->GetCoreStats().Get();

    std::unordered_map<uint8_t, int16_t> correctMap = CharacterManager::GetCharacterBaseStatMap(cs);

    for(auto equip : c->GetEquippedItems())
    {
        if(!equip.IsNull())
        {
            auto itemData = definitionManager->GetItemData(equip->GetType());
            auto itemCommonData = itemData->GetCommon();

            for(auto ct : itemCommonData->GetCorrectTbl())
            {
                auto tblID = ct->GetID();
                switch (ct->GetType())
                {
                    case objects::MiCorrectTbl::Type_t::NUMERIC:
                        correctMap[tblID] = (int16_t)(correctMap[tblID] + ct->GetValue());
                        break;
                    case objects::MiCorrectTbl::Type_t::PERCENT:
                        /// @todo: apply in the right order
                        break;
                    default:
                        break;
                }
            }
        }
    }

    /// @todo: apply effects

    CharacterManager::CalculateDependentStats(correctMap, cs->GetLevel(), false);

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

bool CharacterState::Ready()
{
    return !GetCharacter().IsNull();
}
