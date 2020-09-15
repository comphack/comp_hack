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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>

// channel Includes
#include "ChannelServer.h"
#include "ZoneGeometry.h"

using namespace channel;

AICommand::AICommand()
    : mType(AICommandType_t::NONE),
      mStartTime(0),
      mDelay(0),
      mTargetEntityID(-1),
      mIgnoredDelay(false) {}

AICommand::~AICommand() {}

AICommandType_t AICommand::GetType() const { return mType; }

uint64_t AICommand::GetDelay() const { return mDelay; }

void AICommand::SetDelay(uint64_t delay) { mDelay = delay; }

bool AICommand::GetIgnoredDelay() const { return mIgnoredDelay; }

void AICommand::SetIgnoredDelay(bool ignore) { mIgnoredDelay = ignore; }

uint64_t AICommand::GetStartTime() { return mStartTime; }

void AICommand::Start() {
  if (!mStartTime) {
    mStartTime = ChannelServer::GetServerTime();
  }
}

int32_t AICommand::GetTargetEntityID() const { return mTargetEntityID; }

void AICommand::SetTargetEntityID(int32_t targetEntityID) {
  mTargetEntityID = targetEntityID;
}

AIMoveCommand::AIMoveCommand() {
  mType = AICommandType_t::MOVE;
  mMinimumTargetDistance = 0.f;
  mMaximumTargetDistance = 0.f;
}

AIMoveCommand::AIMoveCommand(int32_t targetEntityID, float minimumDistance,
                             float maximumDistance) {
  SetTargetEntityID(targetEntityID);
  mMinimumTargetDistance = minimumDistance;
  mMaximumTargetDistance = maximumDistance;
}

AIMoveCommand::~AIMoveCommand() {}

std::list<Point> AIMoveCommand::GetPathing() const { return mPathing; }

void AIMoveCommand::SetPathing(const std::list<Point>& pathing) {
  mPathing = pathing;
}

bool AIMoveCommand::GetCurrentDestination(Point& dest) const {
  dest.x = 0.f;
  dest.y = 0.f;

  if (mPathing.size() > 0) {
    dest = mPathing.front();

    return true;
  } else {
    return false;
  }
}

bool AIMoveCommand::GetEndDestination(Point& dest) const {
  dest.x = 0.f;
  dest.y = 0.f;

  if (mPathing.size() > 0) {
    dest = mPathing.back();

    return true;
  } else {
    return false;
  }
}

bool AIMoveCommand::SetNextDestination() {
  if (mPathing.size() > 0) {
    mPathing.pop_front();
  }

  return mPathing.size() > 0;
}

float AIMoveCommand::GetTargetDistance(bool min) const {
  return min ? mMinimumTargetDistance : mMaximumTargetDistance;
}

void AIMoveCommand::SetTargetDistance(float distance, bool min) {
  if (min) {
    mMinimumTargetDistance = distance;
  } else {
    mMaximumTargetDistance = distance;
  }
}

AIUseSkillCommand::AIUseSkillCommand(
    const std::shared_ptr<objects::MiSkillData>& skillData,
    int32_t targetEntityID) {
  mType = AICommandType_t::USE_SKILL;
  mSkillData = skillData;
  mTargetEntityID = targetEntityID;
}

AIUseSkillCommand::AIUseSkillCommand(
    const std::shared_ptr<objects::ActivatedAbility>& activated) {
  mType = AICommandType_t::USE_SKILL;
  mSkillData = activated ? activated->GetSkillData() : nullptr;
  mActivated = activated;
  if (activated) {
    // AI cannot target non-entities
    mTargetEntityID = (int32_t)activated->GetTargetObjectID();
  }
}

AIUseSkillCommand::~AIUseSkillCommand() {}

uint32_t AIUseSkillCommand::GetSkillID() const {
  return mSkillData ? mSkillData->GetCommon()->GetID() : 0;
}

std::shared_ptr<objects::MiSkillData> AIUseSkillCommand::GetSkillData() const {
  return mSkillData;
}

void AIUseSkillCommand::SetActivatedAbility(
    const std::shared_ptr<objects::ActivatedAbility>& activated) {
  mActivated = activated;
}

std::shared_ptr<objects::ActivatedAbility>
AIUseSkillCommand::GetActivatedAbility() const {
  return mActivated;
}

AIScriptedCommand::AIScriptedCommand(const libcomp::String& functionName)
    : mFunctionName(functionName) {
  mType = AICommandType_t::SCRIPTED;
}

AIScriptedCommand::~AIScriptedCommand() {}

libcomp::String AIScriptedCommand::GetFunctionName() const {
  return mFunctionName;
}
