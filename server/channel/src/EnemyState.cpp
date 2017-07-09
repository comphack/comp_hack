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

#include "EnemyState.h"

// libcomp Includes
#include <ServerDataManager.h>
#include <ScriptEngine.h>

using namespace channel;

std::unordered_map<std::string,
    std::shared_ptr<libcomp::ScriptEngine>> EnemyState::sPreparedScripts;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<EnemyState>()
    {
        if(!BindingExists("EnemyState"))
        {
            // Include the base class, skipping abstract classes
            Using<objects::ActiveEntityStateObject>();

            Sqrat::DerivedClass<EnemyState,
                objects::ActiveEntityStateObject> binding(mVM, "EnemyState");
            binding
                .Func<void (EnemyState::*)(float, float, uint64_t)>(
                    "Move", &EnemyState::Move)
                .Func<void (EnemyState::*)(float, uint64_t)>(
                    "Rotate", &EnemyState::Rotate)
                .Func<void (EnemyState::*)(uint64_t)>(
                    "Stop", &EnemyState::Stop)
                .Func<bool (EnemyState::*)(void) const>(
                    "IsMoving", &EnemyState::IsMoving)
                .Func<bool (EnemyState::*)(void) const>(
                    "IsRotating", &EnemyState::IsRotating)
                .Func<uint64_t (EnemyState::*)(const libcomp::String&)>(
                    "GetActionTime", &EnemyState::GetActionTime)
                .Func<void (EnemyState::*)(const libcomp::String&, uint64_t)>(
                    "SetActionTime", &EnemyState::SetActionTime);

            Bind<EnemyState>("EnemyState", binding);
        }

        return *this;
    }
}

EnemyState::EnemyState() : mAIScript(nullptr)
{
}

bool EnemyState::Prepare(const std::weak_ptr<EnemyState>& self, 
    const libcomp::String& aiType, libcomp::ServerDataManager* dataManager)
{
    mSelf = self;

    if(!aiType.IsEmpty())
    {
        auto it = sPreparedScripts.find(aiType.C());
        if(it == sPreparedScripts.end())
        {
            auto script = dataManager->GetAIScript(aiType);
            if(!script)
            {
                return false;
            }

            auto engine = std::make_shared<libcomp::ScriptEngine>();
            engine->Using<EnemyState>();

            if(!engine->Eval(script->Source))
            {
                return false;
            }

            mAIScript = sPreparedScripts[aiType.C()] = engine;
        }
        else
        {
            mAIScript = it->second;
        }
    }

    return true;
}

bool EnemyState::UpdateState(uint64_t now)
{
    if(!IsAlive() || !mAIScript)
    {
        return false;
    }

    RefreshCurrentPosition(now);

    /// @todo: allow multiple handlers based on AI specific state
    /// (ex: idling, wandering, fleeing)
    auto result = Sqrat::RootTable(mAIScript->GetVM()).GetFunction(
        "update").Evaluate<int>(mSelf.lock(), now);
    return result && (*result == 1);
}

uint64_t EnemyState::GetActionTime(const libcomp::String& action)
{
    auto it = mActionTimes.find(action.C());
    if(it != mActionTimes.end())
    {
        return it->second;
    }

    return 0;
}

void EnemyState::SetActionTime(const libcomp::String& action, uint64_t time)
{
    mActionTimes[action.C()] = time;
}
