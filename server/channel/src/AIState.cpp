/**
 * @file server/channel/src/AIState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Contains AI related data for an active entity on the channel.
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

#include "AIState.h"

// object Includes
#include <MiAIData.h>
#include <MiFindInfo.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<AIState>()
    {
        if(!BindingExists("AIState", true))
        {
            Using<objects::AIStateObject>();

            Sqrat::DerivedClass<AIState,
                objects::AIStateObject,
                Sqrat::NoConstructor<AIState>> binding(mVM, "AIState");
            binding
                .Func("GetStatus", &AIState::GetStatus)
                .Func("SetStatus", &AIState::SetStatus);

            Bind<AIState>("AIState", binding);

            Sqrat::Enumeration e(mVM);
            e.Const("IDLE", (int32_t)AIStatus_t::IDLE);
            e.Const("WANDERING", (int32_t)AIStatus_t::WANDERING);
            e.Const("AGGRO", (int32_t)AIStatus_t::AGGRO);
            e.Const("COMBAT", (int32_t)AIStatus_t::COMBAT);

            Sqrat::ConstTable(mVM).Enum("AIStatus_t", e);

            e = Sqrat::Enumeration(mVM);
            e.Const("CLSR", AI_SKILL_TYPE_CLSR);
            e.Const("LNGR", AI_SKILL_TYPE_LNGR);
            e.Const("DEF", AI_SKILL_TYPE_DEF);
            e.Const("HEAL", AI_SKILL_TYPE_HEAL);
            e.Const("SUPPORT", AI_SKILL_TYPE_SUPPORT);
            e.Const("ENEMY", AI_SKILL_TYPES_ENEMY);
            e.Const("ALLY", AI_SKILL_TYPES_ALLY);
            e.Const("ALL", AI_SKILL_TYPES_ALL);

            Sqrat::ConstTable(mVM).Enum("AISkillType_t", e);
        }

        return *this;
    }
}

AIState::AIState() :  mStatus(AIStatus_t::IDLE),
    mPreviousStatus(AIStatus_t::IDLE), mDefaultStatus(AIStatus_t::IDLE),
    mStatusChanged(false)
{
}

AIStatus_t AIState::GetStatus() const
{
    return mStatus;
}

AIStatus_t AIState::GetPreviousStatus() const
{
    return mPreviousStatus;
}

AIStatus_t AIState::GetDefaultStatus() const
{
    return mDefaultStatus;
}

bool AIState::SetStatus(AIStatus_t status, bool isDefault)
{
    // Disallow default setting to target dependent status
    if(isDefault &&
        (status == AIStatus_t::AGGRO || status == AIStatus_t::COMBAT))
    {
        return false;
    }

    bool statusChanged = false;
    {
        std::lock_guard<std::mutex> lock(mFieldLock);
        mStatusChanged = statusChanged = mStatus != status;

        mPreviousStatus = mStatus;
        mStatus = status;

        if(isDefault)
        {
            mDefaultStatus = status;
        }
    }

    if(statusChanged)
    {
        // Always reset next target time
        SetNextTargetTime(0);

        if(status == AIStatus_t::WANDERING)
        {
            uint64_t now = ChannelServer::GetServerTime();
            if(GetDespawnWhenLost())
            {
                // Most entities despawn when switching to wandering after 5
                // minutes if they don't make their way back to their spawn
                // location
                SetDespawnTimeout(now + 300000000ULL);
            }

            // Set next target time based on think speed
            SetNextTargetTime(now + (uint64_t)(GetThinkSpeed() * 1000));
        }
        else if(GetDespawnTimeout())
        {
            // Clear if switching to anything else
            SetDespawnTimeout(0);
        }
    }

    return true;
}

bool AIState::IsIdle() const
{
    return mStatus == AIStatus_t::IDLE;
}

bool AIState::StatusChanged() const
{
    return mStatusChanged;
}

void AIState::ResetStatusChanged()
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    mStatusChanged = false;
}

std::shared_ptr<libcomp::ScriptEngine> AIState::GetScript() const
{
    return mAIScript;
}

void AIState::SetScript(
    const std::shared_ptr<libcomp::ScriptEngine>& aiScript)
{
    mAIScript = aiScript;
}

float AIState::GetAggroValue(uint8_t mode, bool fov, float defaultVal)
{
    auto aiData = GetBaseAI();
    if(aiData && mode < 3)
    {
        auto fInfo = mode == 0 ? aiData->GetAggroNormal()
            : (mode == 1 ? aiData->GetAggroNight() : aiData->GetAggroCast());

        float val = fov
            ? ((float)fInfo->GetFOV() / 360.f * 3.14f)
            : (float)fInfo->GetDistance() * 10.f;
        return val * GetAwareness();
    }

    return defaultVal;
}

float AIState::GetDeaggroDistance(bool isNight)
{
    float dist = GetAggroValue(isNight ? 1 : 0, false, 0.f);

    // Enforce lower limit
    const static float LOWER_LIMIT = AI_DEFAULT_AGGRO_RANGE;
    if(!GetIgnoreDeaggroMin() && dist < LOWER_LIMIT)
    {
        dist = LOWER_LIMIT;
    }

    dist = (dist < 200.f ? 200.f : dist) * (float)GetDeaggroScale();

    // Enforce upper limit
    if(!GetIgnoreDeaggroMax() && dist > MAX_ENTITY_DRAW_DISTANCE)
    {
        return MAX_ENTITY_DRAW_DISTANCE;
    }

    return dist;
}

std::shared_ptr<AICommand> AIState::GetCurrentCommand() const
{
    return mCurrentCommand;
}

void AIState::QueueCommand(const std::shared_ptr<AICommand>& command,
    bool interrupt)
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    if(interrupt)
    {
        mCommandQueue.push_front(command);
        mCurrentCommand = command;
    }
    else
    {
        mCommandQueue.push_back(command);

        if(mCommandQueue.size() == 1)
        {
            mCurrentCommand = command;
        }
    }

}

void AIState::ClearCommands()
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    mCommandQueue.clear();
    mCurrentCommand = nullptr;
}

std::shared_ptr<AICommand> AIState::PopCommand(
    const std::shared_ptr<AICommand>& specific)
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    if(specific)
    {
        mCommandQueue.remove_if([specific]
            (const std::shared_ptr<AICommand>& cmd)
            {
                return cmd == specific;
            });
    }
    else
    {
        if(mCommandQueue.size() > 0)
        {
            auto command = mCommandQueue.front();
            mCommandQueue.pop_front();
        }
    }

    mCurrentCommand = nullptr;
    if(mCommandQueue.size() > 0)
    {
        mCurrentCommand = mCommandQueue.front();
    }

    return mCurrentCommand;
}

void AIState::ResetSkillsMapped()
{
    SetSkillsMapped(false);

    std::lock_guard<std::mutex> lock(mFieldLock);
    mSkillMap.clear();
}

AISkillMap_t AIState::GetSkillMap() const
{
    return mSkillMap;
}

void AIState::SetSkillMap(const AISkillMap_t& skillMap)
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    mSkillMap = skillMap;
}
