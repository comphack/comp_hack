/**
 * @file server/channel/src/EnemyState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an enemy on the channel.
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

#include "EnemyState.h"

// libcomp Includes
#include <ScriptEngine.h>

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<EnemyState>()
    {
        if(!BindingExists("EnemyState", true))
        {
            Using<ActiveEntityState>();
            Using<objects::Enemy>();

            Sqrat::DerivedClass<EnemyState, ActiveEntityState,
                Sqrat::NoConstructor<EnemyState>> binding(mVM, "EnemyState");
            binding
                .Func<std::shared_ptr<objects::Enemy>
                (EnemyState::*)()>(
                    "GetEntity", &EnemyState::GetEntity);

            Bind<EnemyState>("EnemyState", binding);
        }

        return *this;
    }
}

EnemyState::EnemyState()
{
}

std::pair<uint8_t, uint8_t> EnemyState::GetTalkPoints(int32_t entityID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mTalkPoints.find(entityID);
    if(it == mTalkPoints.end())
    {
        mTalkPoints[entityID] = std::pair<uint8_t, uint8_t>(0, 0);
    }

    return mTalkPoints[entityID];
}

void EnemyState::SetTalkPoints(int32_t entityID,
    const std::pair<uint8_t, uint8_t>& points)
{
    std::lock_guard<std::mutex> lock(mLock);
    mTalkPoints[entityID] = points;
}
