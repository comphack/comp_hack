/**
 * @file server/channel/src/AllyState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an ally entity on the channel.
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

#include "AllyState.h"

// libcomp Includes
#include <ScriptEngine.h>

// objects Includes
#include <MiDevilData.h>
#include <MiNPCBasicData.h>

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<AllyState>()
    {
        if(!BindingExists("AllyState", true))
        {
            Using<ActiveEntityState>();
            Using<objects::Ally>();

            Sqrat::DerivedClass<AllyState, ActiveEntityState,
                Sqrat::NoConstructor<AllyState>> binding(mVM, "AllyState");
            binding
                .Func<std::shared_ptr<objects::Ally>
                (AllyState::*)() const>(
                    "GetEntity", &AllyState::GetEntity)
                .StaticFunc("Cast", &AllyState::Cast);

            Bind<AllyState>("AllyState", binding);
        }

        return *this;
    }
}

AllyState::AllyState()
{
}

std::shared_ptr<objects::EnemyBase> AllyState::GetEnemyBase() const
{
    return GetEntity();
}

std::set<uint32_t> AllyState::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei)
{
    return GetAllEnemySkills(definitionManager, includeTokusei);
}

uint8_t AllyState::RecalculateStats(
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

uint8_t AllyState::GetLNCType()
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

int8_t AllyState::GetGender()
{
    auto demonData = GetDevilData();
    if(demonData)
    {
        return (int8_t)demonData->GetBasic()->GetGender();
    }

    return 2;   // None
}

std::shared_ptr<AllyState> AllyState::Cast(
    const std::shared_ptr<EntityStateObject>& obj)
{
    return std::dynamic_pointer_cast<AllyState>(obj);
}
