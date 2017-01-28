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

using namespace channel;

DemonState::DemonState() : objects::DemonStateObject()
{
}

DemonState::~DemonState()
{
}

bool DemonState::RecalculateStats()
{
    auto d = GetDemon().Get();
    if(nullptr == d)
    {
        return true;
    }

    auto cs = d->GetCoreStats();

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

bool DemonState::Ready()
{
    return !GetDemon().IsNull();
}
