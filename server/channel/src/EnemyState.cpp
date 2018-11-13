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

// objects Includes
#include <MiDevilData.h>
#include <MiNPCBasicData.h>

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
                (EnemyState::*)() const>(
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

std::shared_ptr<objects::EnemyBase> EnemyState::GetEnemyBase() const
{
    return GetEntity();
}

uint8_t EnemyState::RecalculateStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto entity = GetEntity();
    if(!entity)
    {
        return true;
    }

    return RecalculateEnemyStats(definitionManager, calcState);
}

std::set<uint32_t> EnemyState::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei)
{
    return GetAllEnemySkills(definitionManager, includeTokusei);
}

uint8_t EnemyState::GetLNCType()
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

int8_t EnemyState::GetGender()
{
    auto demonData = GetDevilData();
    if(demonData)
    {
        return (int8_t)demonData->GetBasic()->GetGender();
    }

    return 2;   // None
}
