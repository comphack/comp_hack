/**
 * @file server/channel/src/CultureMachineState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a culture machine on the channel.
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

#include "CultureMachineState.h"

// libcomp Includes
#include <DatabaseChangeSet.h>
#include <Log.h>

// objects Includes
#include <CultureData.h>
#include <ServerObjectBase.h>

using namespace channel;

CultureMachineState::CultureMachineState(uint32_t machineID,
    const std::shared_ptr<objects::ServerCultureMachineSet>& cmSet)
    : EntityState<objects::ServerCultureMachineSet>(cmSet), mMachineID(0)
{
    // Only set the machine if it exists
    if(cmSet)
    {
        for(auto& machine : cmSet->GetMachines())
        {
            if(machine->GetID() == machineID)
            {
                mMachineID = machineID;
            }
        }
    }
}

uint32_t CultureMachineState::GetMachineID()
{
    return mMachineID;
}

std::shared_ptr<objects::CultureData> CultureMachineState::GetRentalData()
{
    return mRentalData;
}

bool CultureMachineState::SetRentalData(const std::shared_ptr<
    objects::CultureData>& data)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(!mRentalData || !data)
    {
        mRentalData = data;
        return true;
    }

    return false;
}
