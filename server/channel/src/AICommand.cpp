/**
 * @file server/channel/src/AICommand.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents a command for an AI controllable entity on
 *  the channel.
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

#include "AICommand.h"

// object Includes
#include <ActivatedAbility.h>

using namespace channel;

AICommand::AICommand() : mType(AICommandType_t::NONE), mDelay(0),
    mStarted(false)
{
}

AICommand::~AICommand()
{
}

AICommandType_t AICommand::GetType() const
{
    return mType;
}

uint64_t AICommand::GetDelay() const
{
    return mDelay;
}

void AICommand::SetDelay(uint64_t delay)
{
    mDelay = delay;
}

bool AICommand::Started()
{
    return mStarted;
}

void AICommand::Start()
{
    mStarted = true;
}

AIMoveCommand::AIMoveCommand()
{
    mType = AICommandType_t::MOVE;
}

AIMoveCommand::~AIMoveCommand()
{
}

void AIMoveCommand::SetPathing(const std::list<std::pair<
    float, float>>& pathing)
{
    mPathing = pathing;
}

bool AIMoveCommand::GetCurrentDestination(float& x, float& y) const
{
    x = 0.f;
    y = 0.f;

    if(mPathing.size() > 0)
    {
        auto& loc = mPathing.front();
        x = loc.first;
        y = loc.second;

        return true;
    }
    else
    {
        return false;
    }
}

bool AIMoveCommand::GetEndDestination(float& x, float& y) const
{
    x = 0.f;
    y = 0.f;

    if(mPathing.size() > 0)
    {
        auto& loc = mPathing.back();
        x = loc.first;
        y = loc.second;

        return true;
    }
    else
    {
        return false;
    }
}

bool AIMoveCommand::SetNextDestination()
{
    if(mPathing.size() > 0)
    {
        mPathing.pop_front();
    }

    return mPathing.size() > 0;
}

AIUseSkillCommand::AIUseSkillCommand(uint32_t skillID, int64_t targetObjectID)
{
    mType = AICommandType_t::USE_SKILL;
    mSkillID = skillID;
    mTargetObjectID = targetObjectID;
}

AIUseSkillCommand::AIUseSkillCommand(
    const std::shared_ptr<objects::ActivatedAbility>& activated)
{
    mType = AICommandType_t::USE_SKILL;
    mActivated = activated;
    if(activated)
    {
        mSkillID = activated->GetSkillID();
        mTargetObjectID = activated->GetActivationObjectID();
    }
}

AIUseSkillCommand::~AIUseSkillCommand()
{
}

uint32_t AIUseSkillCommand::GetSkillID() const
{
    return mSkillID;
}

int64_t AIUseSkillCommand::GetTargetObjectID() const
{
    return mTargetObjectID;
}

void AIUseSkillCommand::SetActivatedAbility(const std::shared_ptr<
    objects::ActivatedAbility>& activated)
{
    mActivated = activated;
}

std::shared_ptr<objects::ActivatedAbility>
    AIUseSkillCommand::GetActivatedAbility() const
{
    return mActivated;
}

AIScriptedCommand::AIScriptedCommand(const libcomp::String& functionName)
    : mFunctionName(functionName)
{
    mType = AICommandType_t::SCRIPTED;
}

AIScriptedCommand::~AIScriptedCommand()
{
}

libcomp::String AIScriptedCommand::GetFunctionName() const
{
    return mFunctionName;
}
