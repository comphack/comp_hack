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

// objects Includes
#include <Character.h>
#include <EntityStats.h>

using namespace channel;

CharacterState::CharacterState() : objects::CharacterStateObject()
{
}

CharacterState::~CharacterState()
{
}

bool CharacterState::RecalculateStats()
{
    auto c = GetCharacter().Get();
    auto cs = c->GetCoreStats();

    if(nullptr == c)
    {
        return false;
    }

    SetSTR(cs->GetSTR());
    SetMAGIC(cs->GetMAGIC());
    SetVIT(cs->GetVIT());
    SetINTEL(cs->GetINTEL());
    SetSPEED(cs->GetSPEED());
    SetLUCK(cs->GetLUCK());
    SetCLSR(cs->GetCLSR());
    SetLNGR(cs->GetLNGR());
    SetSPELL(cs->GetSPELL());
    SetSUPPORT(cs->GetSUPPORT());
    SetPDEF(cs->GetPDEF());
    SetMDEF(cs->GetMDEF());

    /// @todo: transform stats

    return true;
}

bool CharacterState::Ready()
{
    return !GetCharacter().IsNull();
}
