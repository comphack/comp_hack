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

#include "AIState.h"

using namespace channel;
namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<AIState>()
    {
        if(!BindingExists("AIState", true))
        {
            Sqrat::Class<AIState> binding(mVM, "AIState");
            binding
                .Func<AIStatus_t (AIState::*)() const>(
                    "GetStatus", &AIState::GetStatus)
                .Func<void (AIState::*)(const libcomp::String&,
                    const libcomp::String&)>("OverrideAction",
                    &AIState::OverrideAction);

            Bind<AIState>("AIState", binding);
        }

        return *this;
    }
}

AIState::AIState() : mTargetEntityID(0), mSkillSettings(AI_SKILL_TYPES_ALL),
    mStatus(AIStatus_t::IDLE), mPreviousStatus(AIStatus_t::IDLE),
    mDefaultStatus(AIStatus_t::IDLE), mStatusChanged(false), mSkillsMapped(false)
{
    mLock = new std::mutex();
}

AIState::~AIState()
{
    delete mLock;
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

void AIState::SetStatus(AIStatus_t status, bool isDefault)
{
    mStatusChanged = mStatus != status;

    mPreviousStatus = mStatus;
    mStatus = status;

    if(isDefault)
    {
        mDefaultStatus = status;
    }
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
    mStatusChanged = false;
}

std::shared_ptr<libcomp::ScriptEngine> AIState::GetScript() const
{
    return mAIScript;
}

void AIState::SetScript(const std::shared_ptr<libcomp::ScriptEngine>& aiScript)
{
    mAIScript = aiScript;
}

std::shared_ptr<AICommand> AIState::GetCurrentCommand() const
{
    return mCurrentCommand;
}

void AIState::QueueCommand(const std::shared_ptr<AICommand>& command)
{
    mCommandQueue.push_back(command);

    if(mCommandQueue.size() == 1)
    {
        mCurrentCommand = command;
    }
}

void AIState::ClearCommands()
{
    mCommandQueue.clear();
    mCurrentCommand = nullptr;
}

std::shared_ptr<AICommand> AIState::PopCommand()
{
    if(mCommandQueue.size() > 0)
    {
        auto command = mCommandQueue.front();
        mCommandQueue.pop_front();
    }

    mCurrentCommand = nullptr;
    if(mCommandQueue.size() > 0)
    {
        mCurrentCommand = mCommandQueue.front();
    }

    return mCurrentCommand;
}

int32_t AIState::GetTarget() const
{
    return mTargetEntityID;
}

void AIState::SetTarget(int32_t targetEntityID)
{
    mTargetEntityID = targetEntityID;
}

uint16_t AIState::GetSkillSettings() const
{
    return mSkillSettings;
}

void AIState::SetSkillSettings(uint16_t skillSettings)
{
    if(mSkillSettings != skillSettings)
    {
        mSkillSettings = skillSettings;
        ResetSkillsMapped();
    }
}

bool AIState::SkillsMapped() const
{
    return mSkillsMapped;
}

void AIState::ResetSkillsMapped()
{
    std::lock_guard<std::mutex> lock(*mLock);
    mSkillsMapped = false;
    mSkillMap.clear();
}

std::unordered_map<uint16_t,
    std::vector<uint32_t>> AIState::GetSkillMap() const
{
    return mSkillMap;
}

void AIState::SetSkillMap(const std::unordered_map<uint16_t,
    std::vector<uint32_t>> &skillMap)
{
    std::lock_guard<std::mutex> lock(*mLock);
    mSkillMap = skillMap;
}

void AIState::OverrideAction(const libcomp::String& action,
    const libcomp::String& functionName)
{
    mActionOverrides[action.C()] = functionName;
}

bool AIState::IsOverriden(const libcomp::String& action)
{
    return mActionOverrides.find(action.C()) != mActionOverrides.end();
}

libcomp::String AIState::GetScriptFunction(const libcomp::String& action)
{
    auto it = mActionOverrides.find(action.C());
    return it != mActionOverrides.end() ? it->second : "";
}
