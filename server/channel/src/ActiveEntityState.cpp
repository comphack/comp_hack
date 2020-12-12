/**
 * @file server/channel/src/ActiveEntityState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an active entity on the channel.
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

#include "ActiveEntityState.h"

// libcomp Includes
#include <Constants.h>
#include <DefinitionManager.h>
#include <Log.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// C++ Standard Includes
#include <cmath>
#include <limits>

// objects Includes
#include <ActivatedAbility.h>
#include <Ally.h>
#include <CalculatedEntityState.h>
#include <Character.h>
#include <Clan.h>
#include <Demon.h>
#include <Enemy.h>
#include <EnemyExtension.h>
#include <Item.h>
#include <MiCancelData.h>
#include <MiCategoryData.h>
#include <MiCorrectTbl.h>
#include <MiDevilBattleData.h>
#include <MiDevilData.h>
#include <MiDoTDamageData.h>
#include <MiEffectData.h>
#include <MiGrowthData.h>
#include <MiItemData.h>
#include <MiNPCBasicData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiStatusBasicData.h>
#include <MiStatusData.h>
#include <Spawn.h>
#include <Tokusei.h>
#include <TokuseiAspect.h>
#include <TokuseiAttributes.h>
#include <TokuseiCorrectTbl.h>

// channel Includes
#include "AIState.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"
#include "Zone.h"
#include "ZoneInstance.h"

using namespace channel;

namespace libcomp {
template <>
ScriptEngine& ScriptEngine::Using<ActiveEntityState>() {
  if (!BindingExists("ActiveEntityState", true)) {
    Using<objects::ActiveEntityStateObject>();
    Using<objects::EnemyBase>();

    // Active entities can rotate or stop directly from the script
    // but movement must be handled via the AIManager
    Sqrat::DerivedClass<ActiveEntityState, objects::ActiveEntityStateObject,
                        Sqrat::NoConstructor<ActiveEntityState>>
        binding(mVM, "ActiveEntityState");
    binding.Func("GetCoreStats", &ActiveEntityState::GetCoreStats)
        .Func("GetZone", &ActiveEntityState::GetZone)
        .Func("Rotate", &ActiveEntityState::Rotate)
        .Func("Stop", &ActiveEntityState::Stop)
        .Func("IsMoving", &ActiveEntityState::IsMoving)
        .Func("IsRotating", &ActiveEntityState::IsRotating)
        .Func("GetAIState", &ActiveEntityState::GetAIState)
        .Func("GetWorldCID", &ActiveEntityState::GetWorldCID)
        .Func("GetEnemyBase", &ActiveEntityState::GetEnemyBase)
        .Func("GetStatusEffects", &ActiveEntityState::GetStatusEffectsList)
        .Func("StatusEffectActive", &ActiveEntityState::StatusEffectActive)
        .Func("StatusEffectTimeLeft", &ActiveEntityState::StatusEffectTimeLeft);

    Bind<ActiveEntityState>("ActiveEntityState", binding);

    Using<AIState>();
    Using<Zone>();
  }

  return *this;
}
}  // namespace libcomp

ActiveEntityState::ActiveEntityState()
    : mCurrentZone(0),
      mEffectsActive(false),
      mAlive(true),
      mInitialCalc(false),
      mCloaked(false),
      mLastRefresh(0),
      mNextRegenSync(0),
      mNextUpkeep(0),
      mNextActivatedAbilityID(1) {}

libcomp::String ActiveEntityState::GetEntityLabel() const {
  auto devilData = GetDevilData();
  switch (GetEntityType()) {
    case EntityType_t::ALLY:
      return libcomp::String("ALLY(%1)[%2]")
          .Arg(devilData->GetBasic()->GetID())
          .Arg(GetEntityID());
    case EntityType_t::CHARACTER:
      return libcomp::String("CHARACTER[%1]").Arg(GetEntityID());
    case EntityType_t::ENEMY:
      return libcomp::String("ENEMY(%1)[%2]")
          .Arg(devilData->GetBasic()->GetID())
          .Arg(GetEntityID());
    case EntityType_t::PARTNER_DEMON:
      if (devilData) {
        return libcomp::String("DEMON(%1)[%2]")
            .Arg(devilData->GetBasic()->GetID())
            .Arg(GetEntityID());
      } else {
        return libcomp::String("DEMON[%1]").Arg(GetEntityID());
      }
    default:
      return libcomp::String("UNKNOWN[%1]").Arg(GetEntityID());
  }
}

int16_t ActiveEntityState::GetCorrectValue(
    CorrectTbl tableID,
    std::shared_ptr<objects::CalculatedEntityState> calcState) {
  return (calcState ? calcState : GetCalculatedState())
      ->GetCorrectTbl((size_t)tableID);
}

const libobjgen::UUID ActiveEntityState::GetEntityUUID() { return NULLUUID; }

int32_t ActiveEntityState::GetWorldCID() {
  auto state = ClientState::GetEntityClientState(GetEntityID());
  return state ? state->GetWorldCID() : 0;
}

bool ActiveEntityState::HasActiveEvent() const {
  auto state = ClientState::GetEntityClientState(GetEntityID());
  return state && state->HasActiveEvent();
}

std::shared_ptr<objects::EnemyBase> ActiveEntityState::GetEnemyBase() const {
  return nullptr;
}

void ActiveEntityState::Move(float xPos, float yPos, uint64_t now) {
  if (CanMove()) {
    SetOriginX(GetCurrentX());
    SetOriginY(GetCurrentY());
    SetOriginTicks(now);

    // Rotate instantly
    float destRot = (float)atan2(GetCurrentY() - yPos, GetCurrentX() - xPos);
    SetOriginRotation(destRot);
    SetDestinationRotation(destRot);

    uint64_t addMicro = (uint64_t)(
        (double)(GetDistance(xPos, yPos) / GetMovementSpeed()) * 1000000.0);

    SetDestinationX(xPos);
    SetDestinationY(yPos);
    SetDestinationTicks((uint64_t)(now + addMicro));
  }
}

void ActiveEntityState::Rotate(float rot, uint64_t now) {
  if (CanMove()) {
    SetOriginX(GetCurrentX());
    SetOriginY(GetCurrentY());
    SetOriginRotation(GetCurrentRotation());
    SetOriginTicks(now);

    SetDestinationRotation(CorrectRotation(rot));

    // One complete rotation takes 1650ms at 300.0f speed
    uint64_t addMicro = (uint64_t)(495000.0f / GetMovementSpeed()) * 1000;
    SetDestinationTicks(now + addMicro);
  }
}

void ActiveEntityState::Stop(uint64_t now) {
  SetDestinationX(GetCurrentX());
  SetDestinationY(GetCurrentY());
  SetDestinationRotation(GetCurrentRotation());
  SetDestinationTicks(now);
  SetOriginX(GetCurrentX());
  SetOriginY(GetCurrentY());
  SetOriginRotation(GetCurrentRotation());
  SetOriginTicks(now);
}

bool ActiveEntityState::IsAlive() const { return mAlive; }

bool ActiveEntityState::IsMoving() const {
  return GetCurrentX() != GetDestinationX() ||
         GetCurrentY() != GetDestinationY();
}

bool ActiveEntityState::IsRotating() const {
  return GetCurrentRotation() != GetDestinationRotation();
}

bool ActiveEntityState::CanAct() {
  return mAlive && !StatusRestrictActCount() &&
         !GetStatusTimes(STATUS_KNOCKBACK) &&
         !GetStatusTimes(STATUS_HIT_STUN) &&
         (CanMove() || CurrentSkillsCount() > 0);
}

bool ActiveEntityState::CanMove(bool ignoreSkill) {
  if (!mAlive || GetCorrectValue(CorrectTbl::MOVE1) == 0 ||
      StatusRestrictActCount() > 0 || StatusRestrictMoveCount() > 0) {
    return false;
  }

  bool charging = false;
  auto statusTimes = GetStatusTimes();
  if (statusTimes.size() > 0) {
    for (uint8_t lockState :
         {STATUS_CHARGING, STATUS_KNOCKBACK, STATUS_HIT_STUN, STATUS_IMMOBILE,
          STATUS_RESTING}) {
      if (statusTimes.find(lockState) != statusTimes.end()) {
        if (lockState == STATUS_CHARGING) {
          charging = true;
        } else {
          return false;
        }
      }
    }
  }

  if (!ignoreSkill) {
    auto activated = GetActivatedAbility();
    if (activated &&
        ((charging && activated->GetChargeMoveSpeed() == 0.f) ||
         (!charging && activated->GetChargeCompleteMoveSpeed() == 0.f))) {
      return false;
    }
  }

  return true;
}

bool ActiveEntityState::SameFaction(std::shared_ptr<ActiveEntityState> other,
                                    bool ignoreGroup) {
  return GetFaction() == other->GetFaction() &&
         (ignoreGroup ||
          // Ignore neutral within groups
          GetFactionGroup() == 0 || other->GetFactionGroup() == 0 ||
          GetFactionGroup() == other->GetFactionGroup());
}

float ActiveEntityState::CorrectRotation(float rot) {
  if (rot > libcomp::PI) {
    return rot - (float)(libcomp::PI * 2);
  } else if (rot < -libcomp::PI) {
    return -rot - (float)libcomp::PI;
  }

  return rot;
}

float ActiveEntityState::GetDistance(float x, float y, bool squared) {
  float dSquared = (float)(std::pow((GetCurrentX() - x), 2) +
                           std::pow((GetCurrentY() - y), 2));
  return squared ? dSquared : std::sqrt(dSquared);
}

float ActiveEntityState::GetDistance(std::shared_ptr<ActiveEntityState> other,
                                     bool squared) {
  if (other) {
    return GetDistance(other->GetCurrentX(), other->GetCurrentY(), squared);
  } else {
    return 0.f;
  }
}

bool ActiveEntityState::HasLineOfSight(std::shared_ptr<ActiveEntityState> other,
                                       uint64_t now) {
  auto zone = GetZone();
  if (!other || zone != other->GetZone()) {
    return false;
  }

  if (!now) {
    now = ChannelServer::GetServerTime();
  }

  RefreshCurrentPosition(now);
  other->RefreshCurrentPosition(now);

  Point src(GetCurrentX(), GetCurrentY());
  Point dest(other->GetCurrentX(), other->GetCurrentY());
  Point collidePoint;

  return !zone->Collides(Line(src, dest), collidePoint);
}

float ActiveEntityState::GetMovementSpeed(bool ignoreSkill, bool altSpeed) {
  int16_t speed = 0;
  auto activated = ignoreSkill ? nullptr : GetActivatedAbility();
  if (altSpeed) {
    // Get alternate "walk" speed
    speed = GetCorrectValue(CorrectTbl::MOVE1);
  } else if (activated) {
    // Current skill is always updated with combat state and is not
    // set on the entity without a charge time (even if its "now"),
    // therefore we only have the concept of charged or not
    if (activated->GetChargedTime() < ChannelServer::GetServerTime()) {
      return activated->GetChargeCompleteMoveSpeed();
    } else {
      return activated->GetChargeMoveSpeed();
    }
  } else if (GetCombatTimeOut() &&
             !GetCalculatedState()->ExistingTokuseiAspectsContains(
                 (int8_t)TokuseiAspectType::COMBAT_SPEED_NULL)) {
    // If in combat or using a skill and combat speed is not nullified
    // (which is a non-conditional tokusei), get the combat run speed
    // which should be equal to the default run speed of the entity
    speed = GetCombatRunSpeed();
  } else {
    // Get the normal run speed
    speed = GetCorrectValue(CorrectTbl::MOVE2);
  }

  return (float)(speed + GetSpeedBoost()) * 10.f;
}

uint32_t ActiveEntityState::GetHitboxSize() const {
  auto devilData = GetDevilData();
  return devilData ? devilData->GetBattleData()->GetHitboxSize()
                   : PLAYER_HITBOX_SIZE;
}

void ActiveEntityState::RefreshCurrentPosition(uint64_t now) {
  if (now != mLastRefresh) {
    std::lock_guard<std::mutex> lock(mLock);

    uint64_t destTicks = GetDestinationTicks();
    if (destTicks < mLastRefresh) {
      // If the entity hasn't moved more, quit now
      mLastRefresh = now;
      return;
    }

    mLastRefresh = now;

    float currentX = GetCurrentX();
    float currentY = GetCurrentY();
    float currentRot = GetCurrentRotation();

    float destX = GetDestinationX();
    float destY = GetDestinationY();
    float destRot = GetDestinationRotation();

    bool xDiff = currentX != destX;
    bool yDiff = currentY != destY;
    bool rotDiff = currentRot != destRot;

    if (!xDiff && !yDiff && !rotDiff) {
      // Already up to date
      return;
    }

    if (now >= destTicks) {
      SetCurrentX(destX);
      SetCurrentY(destY);
      SetCurrentRotation(destRot);
    } else {
      float originX = GetOriginX();
      float originY = GetOriginY();
      float originRot = GetOriginRotation();
      uint64_t originTicks = GetOriginTicks();

      uint64_t elapsed = now - originTicks;
      uint64_t total = (destTicks > originTicks) ? destTicks - originTicks : 0;
      if (total == 0 || now < originTicks) {
        SetCurrentX(destX);
        SetCurrentY(destY);
        SetCurrentRotation(destRot);
        return;
      }

      double prog = (double)((double)elapsed / (double)total);
      if (xDiff || yDiff) {
        float newX = (float)(originX + (prog * (destX - originX)));
        float newY = (float)(originY + (prog * (destY - originY)));

        SetCurrentX(newX);
        SetCurrentY(newY);
      }

      if (rotDiff) {
        // Bump both origin and destination by 3.14 to range from
        // 0-+6.28 instead of -3.14-+3.14 for simpler math
        originRot = (float)(originRot + libcomp::PI);
        destRot = (float)(destRot + libcomp::PI);

        float newRot = (float)(originRot + (prog * (destRot - originRot)));

        SetCurrentRotation(CorrectRotation(newRot));
      }
    }
  }
}

bool ActiveEntityState::UpdatePendingCombatants(int32_t entityID,
                                                uint64_t executeTime) {
  bool valid = false;

  std::lock_guard<std::mutex> lock(mLock);
  if (PendingCombatantsCount()) {
    // No entity should ever have 2 values
    RemovePendingCombatants(entityID);

    auto zone = GetZone();
    if (zone) {
      // Remove any invalid entries
      int64_t selfID = (int64_t)GetEntityID();
      auto pending = GetPendingCombatants();
      for (auto& pair : pending) {
        // Combatants are only valid if they're still in the zone
        // and still have an ability targeting this entity with the
        // same execution timestamp registered here and there is no
        // error on the skill
        auto eState = zone->GetActiveEntity(pair.first);
        auto activated = eState ? eState->GetActivatedAbility() : nullptr;
        if (!eState || !activated || activated->GetTargetObjectID() != selfID ||
            activated->GetExecutionRequestTime() != pair.second ||
            activated->GetErrorCode() != -1) {
          RemovePendingCombatants(pair.first);
        }
      }
    }

    if (!PendingCombatantsCount()) {
      // None left, always valid
      valid = true;
    } else {
      // Check against earliest time
      uint64_t first = 0;
      for (auto& pair : GetPendingCombatants()) {
        if (!first || pair.second < first) {
          first = pair.second;
        }
      }

      // The simultaneous hit window is 100ms
      if (!first || first >= executeTime || executeTime - first < 100000ULL) {
        valid = true;
      }
    }
  } else {
    // No other combatants, always valid
    valid = true;
  }

  if (valid) {
    SetPendingCombatants(entityID, executeTime);
  }

  return valid;
}

void ActiveEntityState::ExpireStatusTimes(uint64_t now) {
  std::unordered_map<uint8_t, uint64_t> statusTimes;
  std::unordered_map<uint32_t, uint64_t> cooldowns;
  {
    std::lock_guard<std::mutex> lock(mFieldLock);

    statusTimes = GetStatusTimes();
    cooldowns = GetSkillCooldowns();
  }

  for (auto& pair : statusTimes) {
    if (pair.second != 0 && pair.second <= now) {
      RemoveStatusTimes(pair.first);
    }
  }

  for (auto& pair : cooldowns) {
    if (pair.second <= now) {
      RemoveSkillCooldowns(pair.first);
    }
  }
}

std::shared_ptr<AIState> ActiveEntityState::GetAIState() const {
  return mAIState;
}

void ActiveEntityState::SetAIState(const std::shared_ptr<AIState>& aiState) {
  mAIState = aiState;
}

float ActiveEntityState::RefreshKnockback(uint64_t time, float recoveryBoost,
                                          bool setValue) {
  std::lock_guard<std::mutex> lock(mLock);

  auto kb = GetKnockbackResist();
  float kbMax = (float)GetCorrectValue(CorrectTbl::KNOCKBACK_RESIST);
  if (kb < kbMax) {
    // Knockback refreshes at a rate of 12.5/s (or 0.0125/ms)
    kb = kb + (float)((double)(time - GetKnockbackTicks()) * 0.001 *
                      (0.0125 * (1.0 + (double)recoveryBoost)));
    if (kb > kbMax) {
      kb = kbMax;
    } else if (kb < 0.f) {
      // Sanity check
      kb = 0.f;
    }

    if (setValue) {
      SetKnockbackResist(kb);
      if (kb == kbMax) {
        // Reset to no time
        SetKnockbackTicks(0);
      }
    }
  }

  return kb;
}

float ActiveEntityState::UpdateKnockback(uint64_t now, float decrease,
                                         float recoveryBoost) {
  // Always get up to date first
  RefreshKnockback(now, recoveryBoost);

  std::lock_guard<std::mutex> lock(mLock);

  auto kb = GetKnockbackResist();
  if (kb > 0) {
    if (decrease == -1.f) {
      kb = 0;
    } else {
      kb -= decrease;
      if (kb <= 0) {
        kb = 0.f;
      }
    }

    SetKnockbackResist(kb);
    SetKnockbackTicks(now);
  }

  return kb;
}

std::shared_ptr<Zone> ActiveEntityState::GetZone() const {
  return mCurrentZone;
}

void ActiveEntityState::SetZone(const std::shared_ptr<Zone>& zone,
                                bool updatePrevious) {
  if (updatePrevious && mCurrentZone) {
    mCurrentZone->SetNextStatusEffectTime(0, GetEntityID());
  }

  mCurrentZone = zone;

  RegisterNextEffectTime();
}

bool ActiveEntityState::SetHPMP(int32_t hp, int32_t mp, bool adjust,
                                bool canOverflow) {
  int32_t hpAdjusted, mpAdjusted;
  return SetHPMP(hp, mp, adjust, canOverflow, 0, hpAdjusted, mpAdjusted);
}

bool ActiveEntityState::SetHPMP(int32_t hp, int32_t mp, bool adjust,
                                bool canOverflow, int32_t clenchChance,
                                int32_t& hpAdjusted, int32_t& mpAdjusted) {
  hpAdjusted = mpAdjusted = 0;

  auto cs = GetCoreStats();
  if (cs == nullptr || (!adjust && hp < 0 && mp < 0)) {
    return false;
  }

  std::lock_guard<std::mutex> lock(mLock);
  auto maxHP = GetMaxHP();
  auto maxMP = GetMaxMP();

  // If the amount of damage dealt can overflow, do not
  // use the actual damage dealt, allow it to go
  // over the actual amount dealt
  if (canOverflow && adjust) {
    hpAdjusted = hp;
    mpAdjusted = mp;
  }

  int32_t startingHP = cs->GetHP();
  int32_t startingMP = cs->GetMP();
  if (adjust) {
    hp = (int32_t)(startingHP + hp);
    mp = (int32_t)(startingMP + mp);

    if (!canOverflow) {
      // If the adjusted damage cannot overflow
      // stop it from doing so
      if (startingHP && hp <= 0) {
        hp = 1;
      } else if (!mAlive && hp > 0) {
        hp = 0;
      }
    } else if (startingHP > 1 && hp <= 0 && clenchChance > 0) {
      if (clenchChance >= 10000 || RNG(int32_t, 1, 10000) <= clenchChance) {
        // Survived clench
        hpAdjusted = -(startingHP - 1);
        hp = 1;
      }
    }

    // Make sure we don't go under the limit
    if (hp < 0) {
      hp = 0;
    }

    if (mp < 0) {
      mp = 0;
    }
  } else {
    // Return exact HP/MP adjustment
    if (hp >= 0) {
      hpAdjusted = hp - startingHP;
    }

    if (mp >= 0) {
      mpAdjusted = mp - startingMP;
    }
  }

  bool result = false;
  if (hp >= 0) {
    auto newHP = hp > maxHP ? maxHP : hp;

    // Update if the entity is alive or not
    if (startingHP > 0 && newHP == 0) {
      mAlive = false;
      Stop(ChannelServer::GetServerTime());
      result = true;
    } else if (startingHP == 0 && newHP > 0) {
      mAlive = true;
      result = true;
    }

    result |= !canOverflow && newHP != startingHP;

    if (!canOverflow) {
      hpAdjusted = (int32_t)(newHP - startingHP);
    }

    cs->SetHP(newHP);
  }

  if (mp >= 0) {
    auto newMP = mp > maxMP ? maxMP : mp;
    result |= !canOverflow && newMP != startingMP;

    if (!canOverflow) {
      mpAdjusted = (int32_t)(newMP - startingMP);
    }

    cs->SetMP(newMP);
  }

  return result;
}

const std::unordered_map<uint32_t, std::shared_ptr<objects::StatusEffect>>&
ActiveEntityState::GetStatusEffects() const {
  return mStatusEffects;
}

std::list<std::shared_ptr<objects::StatusEffect>>
ActiveEntityState::GetStatusEffectsList() {
  std::list<std::shared_ptr<objects::StatusEffect>> effects;

  std::lock_guard<std::mutex> lock(mLock);
  for (auto& pair : mStatusEffects) {
    effects.push_back(pair.second);
  }

  return effects;
}

bool ActiveEntityState::StatusEffectActive(uint32_t effectType) {
  std::lock_guard<std::mutex> lock(mLock);
  auto it = mStatusEffects.find(effectType);
  return it != mStatusEffects.end();
}

uint32_t ActiveEntityState::StatusEffectTimeLeft(uint32_t effectType) {
  auto states = GetCurrentStatusEffectStates(0);
  for (auto& pair : states) {
    if (pair.first->GetEffect() == effectType) {
      return pair.second;
    }
  }

  return 0;
}

void ActiveEntityState::SetStatusEffects(
    const std::list<std::shared_ptr<objects::StatusEffect>>& effects,
    libcomp::DefinitionManager* definitionManager) {
  std::lock_guard<std::mutex> lock(mLock);
  mStatusEffects.clear();
  mStatusEffectDefs.clear();
  mNRAShields.clear();
  mTimeDamageEffects.clear();
  mCancelConditions.clear();
  mNextEffectTimes.clear();
  mNextRegenSync = 0;
  mNextUpkeep = 0;

  ClearStatusRestrictAct();
  ClearStatusRestrictKnockback();
  ClearStatusRestrictMove();
  ClearStatusRestrictMagic();
  ClearStatusRestrictSpecial();
  ClearStatusRestrictTalk();

  RegisterNextEffectTime();

  for (auto effect : effects) {
    RegisterStatusEffect(effect, definitionManager);
  }
}

std::set<uint32_t> ActiveEntityState::AddStatusEffects(
    const StatusEffectChanges& effects,
    libcomp::DefinitionManager* definitionManager, uint32_t now,
    bool queueChanges) {
  std::set<uint32_t> removes;

  if (now == 0) {
    now = (uint32_t)std::time(0);
  }

  std::lock_guard<std::mutex> lock(mLock);
  for (auto ePair : effects) {
    bool isReplace = ePair.second.IsReplace;
    uint32_t effectType = ePair.second.Type;
    int8_t stack = ePair.second.Stack;

    auto def = definitionManager->GetStatusData(effectType);
    auto basic = def->GetBasic();
    auto cancel = def->GetCancel();
    auto maxStack = basic->GetMaxStack();

    if (stack > (int8_t)maxStack) {
      stack = (int8_t)maxStack;
    }

    bool add = true;
    bool resetTime = false;
    std::shared_ptr<objects::StatusEffect> effect;
    std::shared_ptr<objects::StatusEffect> removeEffect;
    if (mStatusEffects.find(effectType) != mStatusEffects.end()) {
      // Effect exists already, should we modify time/stack or remove?
      auto existing = mStatusEffects[effectType];
      resetTime = stack != 0;

      bool doReplace = isReplace;
      bool addStack = false;
      switch (basic->GetApplicationLogic()) {
        case 0:
        case 1:  // Differs during skill processing
          // Always set/add stack
          doReplace =
              isReplace && (((int8_t)existing->GetStack() < stack) || !stack);
          addStack = !isReplace;
          break;
        case 2:
          // Always add unless stack is zero (ex: -kajas)
          addStack = true;
          doReplace = !stack;
          break;
        case 3:
          // Always reapply stack (ex: -karns)
          doReplace = resetTime = true;
          break;
        default:
          continue;
          break;
      }

      if (doReplace) {
        if (stack < 0) {
          existing->SetStack(0);
        } else {
          existing->SetStack((uint8_t)stack);
        }
      } else if (addStack && (stack < 0 || (int8_t)existing->GetStack() <
                                               (int8_t)maxStack)) {
        stack = (int8_t)(stack + (int8_t)existing->GetStack());
        if (stack > (int8_t)maxStack) {
          stack = (int8_t)maxStack;
        }

        if (stack < 0) {
          stack = 0;
        }

        existing->SetStack((uint8_t)stack);
      }

      if (existing->GetStack() > 0) {
        effect = existing;
      } else {
        removeEffect = existing;
      }

      add = false;
    } else if (stack <= 0) {
      // Effect does not exist, ignore removal/reduction attempt
      continue;
    } else {
      // Effect does not exist already, determine if it can be added
      auto common = def->GetCommon();

      // Map out existing effects and info to check for inverse
      // cancellation
      bool canCancel = common->CorrectTblCount() > 0;
      libcomp::EnumMap<CorrectTbl, std::unordered_map<bool, uint8_t>> cancelMap;
      for (auto c : common->GetCorrectTbl()) {
        if (c->GetValue() == 0 || c->GetType() == 1) {
          canCancel = false;
          cancelMap.clear();
        } else {
          bool positive = c->GetValue() > 0;
          std::unordered_map<bool, uint8_t>& m = cancelMap[c->GetID()];
          m[positive] =
              (uint8_t)((m.find(positive) != m.end() ? m[positive] : 0) + 1);
        }
      }

      std::set<uint32_t> inverseEffects;
      for (auto pair : mStatusEffects) {
        auto exDef = definitionManager->GetStatusData(pair.first);
        auto exBasic = exDef->GetBasic();
        if (exBasic->GetGroupID() == basic->GetGroupID()) {
          if (basic->GetGroupRank() >= exBasic->GetGroupRank()) {
            // Replace the lower ranked effect in the same group
            removeEffect = pair.second;
          } else {
            // Higher rank exists, do not add or replace
            add = false;
          }

          canCancel = false;
          break;
        }

        // Check if the existing effect is an inverse that should be cancelled
        // instead. For an effect to be inverse, both effects must have correct
        // table entries which are all numeric, none can have a zero value and
        // the number of positive values on one for each entry ID must match the
        // number of negative values on the other and vice-versa. The actual
        // values themselves do NOT need to inversely match.
        auto exCommon = exDef->GetCommon();
        if (canCancel &&
            common->CorrectTblCount() == exCommon->CorrectTblCount()) {
          bool exCancel = true;
          libcomp::EnumMap<CorrectTbl, std::unordered_map<bool, uint8_t>>
              exCancelMap;
          for (auto c : exCommon->GetCorrectTbl()) {
            if (c->GetValue() == 0 || c->GetType() == 1) {
              exCancel = false;
              break;
            } else {
              bool positive = c->GetValue() > 0;
              std::unordered_map<bool, uint8_t>& m = exCancelMap[c->GetID()];
              m[positive] = (uint8_t)(
                  (m.find(positive) != m.end() ? m[positive] : 0) + 1);
            }
          }

          if (exCancel && cancelMap.size() == exCancelMap.size()) {
            for (auto cPair : cancelMap) {
              if (exCancelMap.find(cPair.first) == exCancelMap.end()) {
                exCancel = false;
                break;
              }

              auto otherMap = exCancelMap[cPair.first];
              for (auto mPair : cPair.second) {
                if (otherMap.find(!mPair.first) == otherMap.end() ||
                    otherMap[!mPair.first] != mPair.second) {
                  exCancel = false;
                  break;
                }
              }
            }

            // Correct table values are inversed, existing effect
            // can be cancelled
            if (exCancel) {
              inverseEffects.insert(pair.first);
            }
          }
        }
      }

      if (canCancel && inverseEffects.size() > 0) {
        // Should never be more than one but in case there is, the
        // lowest ID will be cancelled
        auto exEffect = mStatusEffects[*inverseEffects.begin()];
        if (exEffect->GetStack() == (uint8_t)stack) {
          // Cancel the old one, don't add anything
          add = false;

          removeEffect = exEffect;
        } else if (exEffect->GetStack() < (uint8_t)stack) {
          // Cancel the old one, add the new one with a lower stack
          stack = (int8_t)(stack - (int8_t)exEffect->GetStack());
          add = true;

          removeEffect = exEffect;
        } else {
          // Reduce the stack of the existing one;
          exEffect->SetStack((uint8_t)(exEffect->GetStack() - (uint8_t)stack));
          add = false;

          // Application logic 2 effects have their expirations reset
          // any time they are re-applied (barring "set" durations)
          auto exDef = definitionManager->GetStatusData(exEffect->GetEffect());
          if (exDef->GetBasic()->GetApplicationLogic() == 2) {
            resetTime = true;
          }

          effect = exEffect;
        }
      }
    }

    // Only add the effect if its stack is greater than 0
    add &= stack > 0;

    if (add) {
      // Effect not set yet, build it now
      effect = libcomp::PersistentObject::New<objects::StatusEffect>(true);
      effect->SetEntity(GetEntityUUID());
      effect->SetEffect(effectType);
      effect->SetStack((uint8_t)stack);
      effect->SetIsConstant(ePair.second.IsConstant);
    }

    // Perform insert or edit modifications
    bool activateEffect = add;
    if (effect && (effect->GetExpiration() == 0 ||
                   (resetTime &&
                    cancel->GetDurationType() !=
                        objects::MiCancelData::DurationType_t::MS_SET &&
                    cancel->GetDurationType() !=
                        objects::MiCancelData::DurationType_t::DAY_SET))) {
      // Set/reset the expiration
      uint32_t expiration = 0;
      bool absoluteTime = false;
      bool durationOverride = false;
      bool durationRequired = true;
      switch (cancel->GetDurationType()) {
        case objects::MiCancelData::DurationType_t::MS:
        case objects::MiCancelData::DurationType_t::MS_SET:
          // Milliseconds stored as relative countdown
          expiration = ePair.second.Duration ? ePair.second.Duration
                                             : cancel->GetDuration();
          durationOverride = ePair.second.Duration != 0;
          break;
        case objects::MiCancelData::DurationType_t::HOUR:
          // Convert hours to absolute time in seconds
          expiration = (uint32_t)(cancel->GetDuration() * 3600);
          absoluteTime = true;
          break;
        case objects::MiCancelData::DurationType_t::DAY:
        case objects::MiCancelData::DurationType_t::DAY_SET:
          // Convert days to absolute time in seconds
          expiration = (uint32_t)(cancel->GetDuration() * 24 * 3600);
          absoluteTime = true;
          break;
        case objects::MiCancelData::DurationType_t::NONE:
          if (ePair.second.Duration) {
            // Set explicit expiration (in milliseconds)
            expiration = ePair.second.Duration;
            effect->SetIsConstant(false);
            durationOverride = true;
          } else {
            durationRequired = false;
          }
          break;
        default:
          // Invalid, nothing to do
          break;
      }

      if (basic->GetStackType() == 1 && !durationOverride) {
        // Stack scales time
        expiration = expiration * effect->GetStack();
      }

      if (absoluteTime) {
        expiration = (uint32_t)(now + expiration);
      }

      bool setTime = !resetTime || basic->GetApplicationLogic() >= 2;
      if (!setTime) {
        // Do not decrease existing time if we get here
        if (mEffectsActive) {
          uint32_t time = absoluteTime ? expiration
                                       : (uint32_t)(now + (expiration * 0.001));

          setTime = true;
          for (auto& pair : mNextEffectTimes) {
            if (pair.second.find(effectType) != pair.second.end()) {
              setTime = pair.first < time;
              break;
            }
          }
        } else {
          setTime = effect->GetExpiration() < expiration;
        }
      }

      if (durationRequired && !expiration) {
        // Do not actually add it if we have no time here
        effect = nullptr;
        activateEffect = false;
      } else if (setTime) {
        effect->SetExpiration(expiration);
        activateEffect = true;
      }
    }

    if (removeEffect) {
      uint32_t rEffectType = removeEffect->GetEffect();
      removes.insert(rEffectType);

      std::set<uint32_t> removeEffects = {rEffectType};
      RemoveStatusEffects(removeEffects);

      if (mEffectsActive) {
        // Remove any times associated to the status being removed
        for (auto& pair : mNextEffectTimes) {
          // Skip system times
          if (pair.first > 3) {
            pair.second.erase(rEffectType);
          }
        }

        // Then optionally queue its removal
        if (queueChanges) {
          // Non-system time 3 indicates removes
          mNextEffectTimes[3].insert(rEffectType);
        }
      }
    }

    if (effect) {
      RegisterStatusEffect(effect, definitionManager);

      if (mEffectsActive) {
        uint32_t modEffectType = effect->GetEffect();
        if (activateEffect) {
          ActivateStatusEffect(effect, definitionManager, now, add ? 0 : 1);
        }

        // If changes are being queued or an effect other than the one
        // we expected to add was affected (ex: inverse cancels), queue
        // the change up
        if (queueChanges || effectType != modEffectType) {
          // Add non-system time for add or update
          mNextEffectTimes[add ? 1 : 2].insert(modEffectType);
        }
      }
    }
  }

  if (mEffectsActive) {
    RegisterNextEffectTime();
  }

  return removes;
}

std::set<uint32_t> ActiveEntityState::ExpireStatusEffects(
    const std::set<uint32_t>& effectTypes) {
  std::lock_guard<std::mutex> lock(mLock);

  std::set<uint32_t> removeEffects;
  for (uint32_t effectType : effectTypes) {
    auto it = mStatusEffects.find(effectType);
    if (it != mStatusEffects.end()) {
      removeEffects.insert(effectType);
    }
  }

  // Effects identified, remove and update effect times (if active)
  if (removeEffects.size() > 0) {
    RemoveStatusEffects(removeEffects);

    if (mEffectsActive) {
      for (uint32_t effectType : removeEffects) {
        // Non-system time 3 indicates removes
        SetNextEffectTime(effectType, 0);
        mNextEffectTimes[3].insert(effectType);
      }

      RegisterNextEffectTime();
    }
  }

  if (mEffectsActive) {
    removeEffects.clear();
  }

  return removeEffects;
}

std::set<uint32_t> ActiveEntityState::CancelStatusEffects(
    uint8_t cancelFlags, const std::set<uint32_t>& keepEffects) {
  bool cancelled = false;
  return CancelStatusEffects(cancelFlags, cancelled, keepEffects);
}

std::set<uint32_t> ActiveEntityState::CancelStatusEffects(
    uint8_t cancelFlags, bool& cancelled,
    const std::set<uint32_t>& keepEffects) {
  cancelled = false;

  std::set<uint32_t> cancelEffects;
  if (mCancelConditions.size() > 0) {
    std::lock_guard<std::mutex> lock(mLock);

    for (auto cPair : mCancelConditions) {
      if (cancelFlags & cPair.first) {
        for (uint32_t effectType : cPair.second) {
          if (keepEffects.find(effectType) == keepEffects.end()) {
            cancelEffects.insert(effectType);
          }
        }
      }
    }
  }

  if (cancelEffects.size() > 0) {
    cancelled = true;
    return ExpireStatusEffects(cancelEffects);
  } else {
    return std::set<uint32_t>();
  }
}

void ActiveEntityState::SetStatusEffectsActive(
    bool activate, libcomp::DefinitionManager* definitionManager,
    uint32_t now) {
  if (activate && !EntityIsSet()) {
    // Do not activate effects if no entity is set yet
    return;
  }

  if (now == 0) {
    now = (uint32_t)std::time(0);
  }

  // Already set
  if (mEffectsActive == activate) {
    return;
  }

  std::lock_guard<std::mutex> lock(mLock);
  mEffectsActive = activate;
  if (activate) {
    // Set regen
    mNextRegenSync = now + 10;
    SetNextEffectTime(0, mNextRegenSync);

    auto activated = GetActivatedAbility();
    if (activated && activated->GetUpkeepCost() > 0) {
      // Set upkeep
      mNextUpkeep = now + 3;
      SetNextEffectTime(0, mNextUpkeep);
    }

    // Set status effect expirations
    for (auto pair : mStatusEffects) {
      ActivateStatusEffect(pair.second, definitionManager, now, 2);
    }

    RegisterNextEffectTime();
  } else {
    mTimeDamageEffects.clear();

    if (mCurrentZone) {
      mCurrentZone->SetNextStatusEffectTime(0, GetEntityID());
    }

    // Update expirations but leave next effect times until reactivated.
    // Originally it was thought that pausing these during zone change
    // actions etc didn't matter and would result in a more "fair" time
    // but certain effects with very visible timers such as digitalize
    // have proven this not to be game standard behavior.
    for (auto pair : mNextEffectTimes) {
      // Skip system times
      if (pair.first <= 3) continue;

      for (auto effectType : pair.second) {
        auto it = mStatusEffects.find(effectType);
        if (it == mStatusEffects.end()) continue;

        auto effect = it->second;
        uint32_t exp = GetCurrentExpiration(effect, pair.first, now);

        // Make sure the expiration is never frozen at 0 without
        // a normal in zone expiration or the client can desync
        if (effect->GetExpiration() && !exp) {
          exp = 1;
        }

        effect->SetExpiration(exp);
      }
    }
  }
}

uint8_t ActiveEntityState::PopEffectTicks(uint32_t time, int32_t& hpTDamage,
                                          int32_t& mpTDamage, int32_t& tUpkeep,
                                          std::set<uint32_t>& added,
                                          std::set<uint32_t>& updated,
                                          std::set<uint32_t>& removed) {
  hpTDamage = 0;
  mpTDamage = 0;
  tUpkeep = 0;
  added.clear();
  updated.clear();
  removed.clear();

  std::lock_guard<std::mutex> lock(mLock);

  // If effects are not active, stop now
  if (!mEffectsActive) {
    return 0;
  }

  bool found = false;
  bool reregister = false;
  bool tDamageSpecial = false;
  do {
    std::set<uint32_t> passed;
    std::list<std::pair<uint32_t, uint32_t>> next;
    for (auto& pair : mNextEffectTimes) {
      if (pair.first > time) break;

      passed.insert(pair.first);

      // Check hardcoded added, updated, removed first
      if (pair.first == 1) {
        added = pair.second;
        continue;
      } else if (pair.first == 2) {
        updated = pair.second;
        continue;
      } else if (pair.first == 3) {
        removed = pair.second;
        continue;
      }

      bool systemEffect = pair.second.find(0) != pair.second.end();
      bool doRegen = systemEffect && !mNextUpkeep;

      if (systemEffect && mNextUpkeep && time >= mNextUpkeep) {
        // Pay upkeep
        auto activated = GetActivatedAbility();
        if (activated && activated->GetUpkeepCost() > 0 && mAlive) {
          tUpkeep = (int32_t)(activated->GetUpkeepCost());

          // Upkeep cost occurs every 3 seconds
          mNextUpkeep = pair.first + 3;
          next.push_back(std::pair<uint32_t, uint32_t>(0, mNextUpkeep));
        } else {
          mNextUpkeep = 0;
        }
      }

      if (systemEffect && mNextRegenSync && time >= mNextRegenSync) {
        // Adjust T-Damage if the entity is not dead
        if (mAlive) {
          tDamageSpecial = HasSpecialTDamage();

          bool skipHPRegen = false;
          bool skipMPRegen = false;
          if (doRegen) {
            auto calcState = GetCalculatedState();
            if (calcState->ExistingTokuseiAspectsContains(
                    (int8_t)TokuseiAspectType::ZONE_INSTANCE_POISON)) {
              // Ignore all HP regen and take HP damage based
              // upon max HP and the instance's poison level
              auto zone = GetZone();
              auto instance = zone ? zone->GetInstance() : nullptr;
              if (instance && instance->GetPoisonLevel()) {
                skipHPRegen = true;
                hpTDamage = hpTDamage +
                            (int32_t)ceil((float)instance->GetPoisonLevel() *
                                          0.01f * (float)GetMaxHP());
              }
            }
          }

          // Apply T-damage
          for (auto effectType : mTimeDamageEffects) {
            auto se = mStatusEffectDefs[effectType];

            if (se) {
              auto damage = se->GetEffect()->GetDamage();

              skipHPRegen = damage->GetHPDamage() > 0 ? true : skipHPRegen;
              hpTDamage = (int32_t)(hpTDamage + damage->GetHPDamage());
              skipMPRegen = damage->GetMPDamage() > 0 ? true : skipMPRegen;
              mpTDamage = (int32_t)(mpTDamage + damage->GetMPDamage());
            } else {
              LogCharacterManagerError([&]() {
                return libcomp::String(
                           "%1: Failed to get effect %2 in "
                           "ActiveEntityState::PopEffectTicks\n")
                    .Arg(GetEntityLabel())
                    .Arg(effectType);
              });
            }
          }

          if (!skipHPRegen) {
            hpTDamage =
                (int32_t)(hpTDamage - GetCorrectValue(CorrectTbl::HP_REGEN));
          }

          if (!skipMPRegen) {
            mpTDamage =
                (int32_t)(mpTDamage - GetCorrectValue(CorrectTbl::MP_REGEN));
          }
        }

        // T-Damage application is every 10 seconds
        mNextRegenSync = pair.first + 10;
        next.push_back(std::pair<uint32_t, uint32_t>(0, mNextRegenSync));

        pair.second.erase(0);
      }

      // Remove effects that have ended
      RemoveStatusEffects(pair.second);
      for (auto effectType : pair.second) {
        removed.insert(effectType);
      }
    }

    for (auto t : passed) {
      mNextEffectTimes.erase(t);
    }

    for (auto pair : next) {
      SetNextEffectTime(pair.first, pair.second);
    }

    found = passed.size() > 0;
    reregister |= found;
  } while (found);

  if (reregister) {
    RegisterNextEffectTime();
  }

  // If anything was popped off the map, update the entity
  uint8_t result = tDamageSpecial ? 2 : 0;
  if (hpTDamage || mpTDamage || added.size() > 0 || updated.size() > 0 ||
      removed.size() > 0) {
    result |= 1;
  }

  return result;
}

bool ActiveEntityState::HasSpecialTDamage() { return false; }

libobjgen::UUID ActiveEntityState::GetResponsibleEntity() const { return {}; }

bool ActiveEntityState::ResetUpkeep() {
  std::lock_guard<std::mutex> lock(mLock);

  auto activated = GetActivatedAbility();
  if (activated && activated->GetUpkeepCost() > 0) {
    uint32_t now = (uint32_t)std::time(0);

    mNextUpkeep = (uint32_t)(now + 3);
    SetNextEffectTime(0, mNextUpkeep);

    RegisterNextEffectTime();

    return true;
  }

  // Clear time and let it expire on the next tick
  mNextUpkeep = 0;

  return false;
}

std::list<std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t>>
ActiveEntityState::GetCurrentStatusEffectStates(uint32_t now) {
  if (now == 0) {
    now = (uint32_t)std::time(0);
  }

  std::list<std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t>> result;
  std::lock_guard<std::mutex> lock(mLock);

  if (!mEffectsActive) {
    // Just pull the stored values
    for (auto pair : mStatusEffects) {
      std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t> p(
          pair.second, pair.second->GetExpiration());
      result.push_back(p);
    }

    return result;
  }

  // Pull the times and transform the stored expiration
  std::unordered_map<uint32_t, uint32_t> nextTimes;
  for (auto pair : mNextEffectTimes) {
    // Skip non-system times
    if (pair.first <= 3) continue;

    for (auto effectType : pair.second) {
      nextTimes[effectType] = pair.first;
    }
  }

  for (auto pair : mStatusEffects) {
    uint32_t exp = pair.second->GetExpiration();

    auto it = nextTimes.find(pair.first);
    if (it != nextTimes.end()) {
      exp = GetCurrentExpiration(pair.second, it->second, now);
    }

    std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t> p(pair.second,
                                                                  exp);
    result.push_back(p);
  }

  return result;
}

std::set<int32_t> ActiveEntityState::GetOpponentIDs() const {
  return mOpponentIDs;
}

bool ActiveEntityState::HasOpponent(int32_t opponentID) {
  std::lock_guard<std::mutex> lock(mLock);
  return opponentID == 0 ? mOpponentIDs.size() > 0
                         : mOpponentIDs.find(opponentID) != mOpponentIDs.end();
}

size_t ActiveEntityState::AddRemoveOpponent(bool add, int32_t opponentID) {
  std::lock_guard<std::mutex> lock(mLock);
  if (add) {
    mOpponentIDs.insert(opponentID);
  } else {
    mOpponentIDs.erase(opponentID);
  }

  return mOpponentIDs.size();
}

int16_t ActiveEntityState::GetNRAChance(
    uint8_t nraIdx, CorrectTbl type,
    std::shared_ptr<objects::CalculatedEntityState> calcState) {
  if (calcState == nullptr) {
    calcState = GetCalculatedState();
  }

  switch (nraIdx) {
    case NRA_NULL:
      return calcState->GetNullChances((int16_t)type);
    case NRA_REFLECT:
      return calcState->GetReflectChances((int16_t)type);
    case NRA_ABSORB:
      return calcState->GetAbsorbChances((int16_t)type);
    default:
      return 0;
  }
}

bool ActiveEntityState::GetNRAShield(uint8_t nraIdx, CorrectTbl type,
                                     bool reduce) {
  uint32_t adjustEffect = 0;
  bool expire = false;
  {
    std::lock_guard<std::mutex> lock(mLock);
    for (auto pair : mNRAShields) {
      auto it = pair.second.find(type);
      if (it != pair.second.end() &&
          it->second.find(nraIdx) != it->second.end()) {
        adjustEffect = pair.first;
        break;
      }
    }

    if (!reduce) {
      return adjustEffect != 0;
    }

    if (adjustEffect) {
      auto it = mStatusEffects.find(adjustEffect);
      if (it != mStatusEffects.end()) {
        auto effect = it->second;

        uint8_t newStack = (uint8_t)(effect->GetStack() - 1);
        effect->SetStack(newStack);
        expire = newStack == 0;
      }
    }
  }

  if (expire) {
    ExpireStatusEffects({adjustEffect});
  }

  return adjustEffect != 0;
}

int8_t ActiveEntityState::GetNextActivatedAbilityID() {
  std::lock_guard<std::mutex> lock(mLock);

  bool firstLoop = true;
  int8_t first = mNextActivatedAbilityID;
  int8_t next = mNextActivatedAbilityID;

  do {
    if (!firstLoop && next == first) {
      // All are being used, this should never happen but return
      // a default if for some reason it does
      return 0;
    } else {
      next = mNextActivatedAbilityID;
      mNextActivatedAbilityID = (int8_t)((mNextActivatedAbilityID + 1) % 128);
    }

    firstLoop = false;
  } while (SpecialActivationsKeyExists(next));

  return next;
}

void ActiveEntityState::RemoveStatusEffects(
    const std::set<uint32_t>& effectTypes) {
  std::set<uint8_t> cancelTypes;

  bool recalcIgnore = false;
  bool recalcAI = false;
  for (uint32_t effectType : effectTypes) {
    auto def = mStatusEffectDefs[effectType];
    recalcAI |= def && (def->GetEffect()->GetRestrictions() & 0x1D) != 0;

    mStatusEffects.erase(effectType);
    mStatusEffectDefs.erase(effectType);
    mNRAShields.erase(effectType);
    mTimeDamageEffects.erase(effectType);

    for (auto& cPair : mCancelConditions) {
      cPair.second.erase(effectType);
      cancelTypes.insert(cPair.first);
    }

    RemoveStatusRestrictAct(effectType);
    RemoveStatusRestrictKnockback(effectType);
    RemoveStatusRestrictMove(effectType);
    RemoveStatusRestrictMagic(effectType);
    RemoveStatusRestrictSpecial(effectType);
    RemoveStatusRestrictTalk(effectType);

    if (IsIgnoreEffect(effectType)) {
      recalcIgnore = true;

      if (effectType == SVR_CONST.STATUS_BIKE) {
        // Clear any existing boost flags in case it was removed
        // while still boosting
        RemoveAdditionalTokusei(SVR_CONST.TOKUSEI_BIKE_BOOST);
        if (GetDisplayState() == ActiveDisplayState_t::BIKE_BOOST) {
          SetDisplayState(ActiveDisplayState_t::ACTIVE);
        }

        auto state = ClientState::GetEntityClientState(GetEntityID());
        if (state) {
          state->SetBikeBoosting(false);
        }
      } else if ((effectType == SVR_CONST.STATUS_MOUNT ||
                  effectType == SVR_CONST.STATUS_MOUNT_SUPER) &&
                 GetDisplayState() == ActiveDisplayState_t::MOUNT) {
        SetDisplayState(ActiveDisplayState_t::ACTIVE);
      }
    }
  }

  // Clean up any now empty cancel conditions
  for (uint8_t cancelType : cancelTypes) {
    if (mCancelConditions[cancelType].size() == 0) {
      mCancelConditions.erase(cancelType);
    }
  }

  if (recalcAI && mAIState) {
    mAIState->ResetSkillsMapped();
  }

  if (GetIsHidden() &&
      mStatusEffects.find(SVR_CONST.STATUS_DEMON_ONLY) ==
          mStatusEffects.end() &&
      mStatusEffects.find(SVR_CONST.STATUS_STEALTH) == mStatusEffects.end()) {
    SetIsHidden(false);
  }

  if (mCloaked &&
      mStatusEffects.find(SVR_CONST.STATUS_CLOAK) == mStatusEffects.end()) {
    mCloaked = false;
    recalcIgnore = true;
  }

  if (recalcIgnore) {
    ResetAIIgnored();
  }
}

void ActiveEntityState::ActivateStatusEffect(
    const std::shared_ptr<objects::StatusEffect>& effect,
    libcomp::DefinitionManager* definitionManager, uint32_t now, uint8_t mode) {
  auto effectType = effect->GetEffect();

  bool timeOnly = mode == 1;
  if (timeOnly) {
    // Remove the current expiration
    for (auto& pair : mNextEffectTimes) {
      // Skip system times
      if (pair.first > 3) {
        pair.second.erase(effectType);
      }
    }
  } else if (mode == 2) {
    // Check for registration and quit if found
    for (auto& pair : mNextEffectTimes) {
      if (pair.second.find(effectType) != pair.second.end()) {
        return;
      }
    }
  }

  auto se = definitionManager->GetStatusData(effectType);
  auto cancel = se->GetCancel();
  if (!effect->GetIsConstant()) {
    switch (cancel->GetDurationType()) {
      case objects::MiCancelData::DurationType_t::MS:
      case objects::MiCancelData::DurationType_t::MS_SET: {
        // Force next tick time to duration
        uint32_t time = (uint32_t)(now + (effect->GetExpiration() * 0.001));
        mNextEffectTimes[time].insert(effectType);
      } break;
      case objects::MiCancelData::DurationType_t::NONE:
        if (effect->GetExpiration()) {
          // Force next tick time to duration but only if it has time
          uint32_t time = (uint32_t)(now + (effect->GetExpiration() * 0.001));
          mNextEffectTimes[time].insert(effectType);
        }
        break;
      default:
        if (effect->GetExpiration()) {
          // Next tick is absolute time
          mNextEffectTimes[effect->GetExpiration()].insert(effectType);
        } else {
          // Effect is not a constant, not type NONE and has no
          // expiration, remove at the earliest possible time
          mNextEffectTimes[3].insert(effectType);
        }
        break;
    }
  }

  // Store the definition for quick access later
  mStatusEffectDefs[effectType] = se;

  if (timeOnly) {
    // Nothing more to do
    return;
  }

  // Populate restrictions
  uint8_t restr = (uint8_t)se->GetEffect()->GetRestrictions();
  if (restr & 0x01) {
    // No (non-system) actions
    InsertStatusRestrictAct(effectType);
  }

  if (restr & 0x02) {
    // No movement
    InsertStatusRestrictMove(effectType);

    // Disable knockback if movement locked and hit result won't cancel
    // the status
    uint8_t cancelTypes = se->GetCancel()->GetCancelTypes();
    if ((cancelTypes & EFFECT_CANCEL_HIT) == 0 &&
        (cancelTypes & EFFECT_CANCEL_DAMAGE) == 0 &&
        (cancelTypes & EFFECT_CANCEL_KNOCKBACK) == 0) {
      InsertStatusRestrictKnockback(effectType);
    }
  }

  if (restr & 0x04) {
    // No magic skills
    InsertStatusRestrictMagic(effectType);
  }

  if (restr & 0x08) {
    // No special skills
    InsertStatusRestrictSpecial(effectType);
  }

  if (restr & 0x10) {
    // No taks skills (source or target)
    InsertStatusRestrictTalk(effectType);
  }

  if ((restr & 0x1D) != 0 && mAIState) {
    // One or more skill affecting effects added
    mAIState->ResetSkillsMapped();
  }

  // Add to timed damage effect set if T-Damage is specified
  auto common = se->GetCommon();
  auto basic = se->GetBasic();
  auto damage = se->GetEffect()->GetDamage();
  if (damage->GetHPDamage() || damage->GetMPDamage()) {
    // Ignore if the damage applies as part of the skill only
    if (common->GetCategory()->GetMainCategory() != STATUS_CATEGORY_STUN) {
      mTimeDamageEffects.insert(effectType);
    }
  }

  // If the stack type is a counter and the effect is re-applied each time
  // check for NRA shields
  if (basic->GetStackType() == 0 && basic->GetApplicationLogic() == 3) {
    for (auto ct : common->GetCorrectTbl()) {
      if ((uint8_t)ct->GetID() >= (uint8_t)CorrectTbl::NRA_WEAPON &&
          (uint8_t)ct->GetID() <= (uint8_t)CorrectTbl::NRA_MAGIC) {
        mNRAShields[effectType][ct->GetID()].insert((uint8_t)ct->GetValue());
      }
    }
  }

  if (!GetIsHidden() && (effect->GetEffect() == SVR_CONST.STATUS_DEMON_ONLY ||
                         effect->GetEffect() == SVR_CONST.STATUS_STEALTH)) {
    SetIsHidden(true);
  }

  if (!mCloaked && effect->GetEffect() == SVR_CONST.STATUS_CLOAK) {
    mCloaked = true;
  }

  if (IsIgnoreEffect(effect->GetEffect())) {
    ResetAIIgnored();
  }
}

bool ActiveEntityState::IsIgnoreEffect(uint32_t effectType) const {
  const static std::set<uint32_t> IGNORE_EFFECTS = {
      SVR_CONST.STATUS_BIKE,        SVR_CONST.STATUS_CLOAK,
      SVR_CONST.STATUS_DEMON_ONLY,  SVR_CONST.STATUS_MOUNT,
      SVR_CONST.STATUS_MOUNT_SUPER, SVR_CONST.STATUS_STEALTH};

  return IGNORE_EFFECTS.find(effectType) != IGNORE_EFFECTS.end();
}

void ActiveEntityState::ResetAIIgnored() {
  bool ignore = false;
  for (auto& pair : mStatusEffects) {
    if (IsIgnoreEffect(pair.first)) {
      ignore = true;
      break;
    }
  }

  if (ignore != GetAIIgnored()) {
    SetAIIgnored(ignore);
  }
}

void ActiveEntityState::SetNextEffectTime(uint32_t effectType, uint32_t time) {
  // Only erase if a non-system effect
  if (effectType) {
    for (auto& pair : mNextEffectTimes) {
      // Skip non-system times
      if (pair.first <= 3) continue;

      if (pair.second.find(effectType) != pair.second.end()) {
        if (time == 0) {
          pair.second.erase(effectType);
          if (pair.second.size() == 0) {
            mNextEffectTimes.erase(pair.first);
          }
        }

        return;
      }
    }
  }

  if (time != 0) {
    mNextEffectTimes[time].insert(effectType);
  }
}

void ActiveEntityState::RegisterStatusEffect(
    const std::shared_ptr<objects::StatusEffect>& effect,
    libcomp::DefinitionManager* definitionManager) {
  uint32_t effectType = effect->GetEffect();
  mStatusEffects[effectType] = effect;

  // Mark the cancel conditions
  auto se = definitionManager->GetStatusData(effectType);
  auto cancel = se->GetCancel();
  for (uint16_t x = 0x0001; x < 0x0100;) {
    uint8_t x8 = (uint8_t)x;
    if (cancel->GetCancelTypes() & x8) {
      mCancelConditions[x8].insert(effectType);
    }

    x = (uint16_t)(x << 1);
  }
}

void ActiveEntityState::RegisterNextEffectTime() {
  if (mCurrentZone && mEffectsActive) {
    mCurrentZone->SetNextStatusEffectTime(
        mNextEffectTimes.size() > 0 ? mNextEffectTimes.begin()->first : 0,
        GetEntityID());
  }
}

uint32_t ActiveEntityState::GetCurrentExpiration(
    const std::shared_ptr<objects::StatusEffect>& effect, uint32_t nextTime,
    uint32_t now) {
  uint32_t exp = effect->GetExpiration();

  if (exp > 0) {
    auto se = mStatusEffectDefs[effect->GetEffect()];

    if (!se) {
      LogCharacterManagerError([&]() {
        return libcomp::String(
                   "%1: Failed to get effect %2 in "
                   "ActiveEntityState::GetCurrentExpiration")
            .Arg(GetEntityLabel())
            .Arg(effect->GetEffect());
      });

      return 0;
    }

    auto cancel = se->GetCancel();
    switch (cancel->GetDurationType()) {
      case objects::MiCancelData::DurationType_t::MS:
      case objects::MiCancelData::DurationType_t::MS_SET:
      case objects::MiCancelData::DurationType_t::NONE:
        if (!effect->GetIsConstant()) {
          // Convert back to milliseconds
          uint32_t newExp = (uint32_t)((nextTime - now) * 1000);
          if (exp > newExp) {
            exp = newExp;
          }
        }
        break;
      default:
        // Time is absolute, nothing to do
        break;
    }
  }

  return exp;
}

bool ActiveEntityState::SkillAvailable(uint32_t skillID) {
  return CurrentSkillsContains(skillID) && !DisabledSkillsContains(skillID);
}

bool ActiveEntityState::IsLNCType(uint8_t lncType, bool invertFlag) {
  uint8_t lnc = GetLNCType();

  if (invertFlag) {
    // Inverted flag mode
    // L/N/C are 4/2/1 respectively with flags allowed
    switch (lnc) {
      case LNC_LAW:
        return (lncType & 0x04) != 0;
      case LNC_NEUTRAL:
        return (lncType & 0x02) != 0;
      case LNC_CHAOS:
        return (lncType & 0x01) != 0;
      default:
        return false;
    }
  } else {
    // Non-flag linear mode
    // L/N/C are 0/2/4 respectively
    // 1 is L or N
    // 3 is N or C
    // 5 is not N
    if (lncType == 1) {
      return lnc == LNC_LAW || lnc == LNC_NEUTRAL;
    } else if (lncType == 3) {
      return lnc == LNC_NEUTRAL || lnc == LNC_CHAOS;
    } else if (lncType == 5) {
      return lnc == LNC_LAW || lnc == LNC_CHAOS;
    }

    return lnc == lncType;
  }
}

// "Abstract implementations" required for Sqrat usage
uint8_t ActiveEntityState::RecalculateStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    std::shared_ptr<objects::MiSkillData> contextSkill) {
  (void)definitionManager;
  (void)calcState;
  (void)contextSkill;
  return 0;
}

bool ActiveEntityState::CopyToEnemy(
    const std::shared_ptr<ActiveEntityState>& eState,
    libcomp::DefinitionManager* definitionManager) {
  (void)definitionManager;

  auto eBase = eState ? eState->GetEnemyBase() : nullptr;
  auto cs = GetCoreStats();
  if (!eBase || !cs) {
    return false;
  }

  // Get/create enemy extension and set the stats and skills
  auto extension = eBase->GetExtension();
  if (!extension) {
    extension = std::make_shared<objects::EnemyExtension>();
    eBase->SetExtension(extension);
  }

  extension->ClearStatBoosts();

  auto devilData = GetDevilData();
  if (devilData) {
    auto battleData = devilData->GetBattleData();
    libcomp::EnumMap<CorrectTbl, int32_t> stats;

    for (size_t i = 0; i < 126; i++) {
      CorrectTbl tblID = (CorrectTbl)i;
      stats[tblID] = battleData->GetCorrect((size_t)i);
    }

    extension->SetOverrideStats(true);

    for (auto& cPair : stats) {
      extension->SetCorrectTbl((size_t)cPair.first, (int16_t)cPair.second);
    }
  }

  extension->SetNoDefaultSkills(true);
  extension->SetAddedSkills(GetCurrentSkills());

  eBase->GetCoreStats()->SetLevel(cs->GetLevel());
  eState->ClearAdditionalTokusei();

  return true;
}

std::set<uint32_t> ActiveEntityState::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei) {
  (void)definitionManager;
  (void)includeTokusei;
  return std::set<uint32_t>();
}

uint8_t ActiveEntityState::GetLNCType() { return 0; }

int8_t ActiveEntityState::GetGender() { return GENDER_NA; }

std::shared_ptr<objects::EntityStats> ActiveEntityState::GetCoreStats() {
  return nullptr;
}

int8_t ActiveEntityState::GetLevel() { return 0; }

bool ActiveEntityState::Ready(bool ignoreDisplayState) {
  (void)ignoreDisplayState;

  return false;
}

bool ActiveEntityState::EntityIsSet() const { return false; }

bool ActiveEntityState::IsClientVisible() { return false; }

bool ActiveEntityState::IsMounted() {
  return StatusEffectActive(SVR_CONST.STATUS_MOUNT) ||
         StatusEffectActive(SVR_CONST.STATUS_MOUNT_SUPER);
}

namespace channel {

template <>
ActiveEntityStateImp<objects::Character>::ActiveEntityStateImp() {
  SetEntityType(EntityType_t::CHARACTER);
  SetFaction(objects::ActiveEntityStateObject::Faction_t::PLAYER);
}

template <>
ActiveEntityStateImp<objects::Demon>::ActiveEntityStateImp() {
  SetEntityType(EntityType_t::PARTNER_DEMON);
  SetFaction(objects::ActiveEntityStateObject::Faction_t::PLAYER);
}

template <>
ActiveEntityStateImp<objects::Enemy>::ActiveEntityStateImp() {
  SetEntityType(EntityType_t::ENEMY);
  SetFaction(objects::ActiveEntityStateObject::Faction_t::ENEMY);
}

template <>
ActiveEntityStateImp<objects::Ally>::ActiveEntityStateImp() {
  SetEntityType(EntityType_t::ALLY);
  SetFaction(objects::ActiveEntityStateObject::Faction_t::PLAYER);
}

template <>
void ActiveEntityStateImp<objects::Character>::SetEntity(
    const std::shared_ptr<objects::Character>& entity,
    libcomp::DefinitionManager* definitionManager) {
  (void)definitionManager;

  {
    std::lock_guard<std::mutex> lock(mLock);
    mEntity = entity;
  }

  std::list<std::shared_ptr<objects::StatusEffect>> effects;
  if (entity) {
    // Character should always be set but check just in case
    for (auto e : entity->GetStatusEffects()) {
      effects.push_back(e.Get());
    }

    mAlive = entity->GetCoreStats()->GetHP() > 0;

    SetDisplayState(ActiveDisplayState_t::DATA_NOT_SENT);
  } else {
    SetDisplayState(ActiveDisplayState_t::NOT_SET);
  }

  // Remove any values from the previous entity
  SetIsHidden(false);
  SetAIIgnored(false);
  ClearStatusTimes();
  SetActivatedAbility(nullptr);
  ClearSpecialActivations();

  SetStatusEffects(effects, definitionManager);

  // Reset knockback and let refresh correct
  SetKnockbackResist(0);
  mInitialCalc = false;
}

template <>
void ActiveEntityStateImp<objects::Demon>::SetEntity(
    const std::shared_ptr<objects::Demon>& entity,
    libcomp::DefinitionManager* definitionManager) {
  {
    std::lock_guard<std::mutex> lock(mLock);
    mEntity = entity;
  }

  std::list<std::shared_ptr<objects::StatusEffect>> effects;
  if (entity) {
    for (auto e : entity->GetStatusEffects()) {
      effects.push_back(e.Get());
    }

    mAlive = entity->GetCoreStats()->GetHP() > 0;

    SetDisplayState(ActiveDisplayState_t::DATA_NOT_SENT);
  } else {
    SetDisplayState(ActiveDisplayState_t::NOT_SET);
  }

  SetStatusEffects(effects, definitionManager);
  SetDevilData(entity ? definitionManager->GetDevilData(entity->GetType())
                      : nullptr);

  auto calcState = GetCalculatedState();
  calcState->ClearActiveTokuseiTriggers();
  calcState->ClearEffectiveTokusei();
  ClearAdditionalTokusei();
  ClearSkillCooldowns();
  ClearPendingCombatants();

  // Reset knockback and let refresh correct
  SetKnockbackResist(0);
  mInitialCalc = false;
}

template <>
void ActiveEntityStateImp<objects::Enemy>::SetEntity(
    const std::shared_ptr<objects::Enemy>& entity,
    libcomp::DefinitionManager* definitionManager) {
  std::lock_guard<std::mutex> lock(mLock);
  mEntity = entity;

  if (entity) {
    mAlive = entity->GetCoreStats()->GetHP() > 0;

    SetDisplayState(ActiveDisplayState_t::DATA_NOT_SENT);

    auto spawn = entity->GetSpawnSource();
    if (spawn && spawn->GetFactionGroup()) {
      SetFactionGroup(spawn->GetFactionGroup());
    }
  } else {
    mEffectsActive = false;
    SetDisplayState(ActiveDisplayState_t::NOT_SET);
    SetFactionGroup(0);
  }

  SetDevilData(entity ? definitionManager->GetDevilData(entity->GetType())
                      : nullptr);

  // Reset knockback and let refresh correct
  SetKnockbackResist(0);
  mInitialCalc = false;
}

template <>
void ActiveEntityStateImp<objects::Ally>::SetEntity(
    const std::shared_ptr<objects::Ally>& entity,
    libcomp::DefinitionManager* definitionManager) {
  std::lock_guard<std::mutex> lock(mLock);
  mEntity = entity;

  if (entity) {
    mAlive = entity->GetCoreStats()->GetHP() > 0;

    SetDisplayState(ActiveDisplayState_t::DATA_NOT_SENT);

    auto spawn = entity->GetSpawnSource();
    if (spawn && spawn->GetFactionGroup()) {
      SetFactionGroup(spawn->GetFactionGroup());
    }
  } else {
    mEffectsActive = false;
    SetDisplayState(ActiveDisplayState_t::NOT_SET);
    SetFactionGroup(0);
  }

  SetDevilData(entity ? definitionManager->GetDevilData(entity->GetType())
                      : nullptr);

  // Reset knockback and let refresh correct
  SetKnockbackResist(0);
  mInitialCalc = false;
}

}  // namespace channel

const std::set<CorrectTbl> BASE_STATS = {CorrectTbl::STR,   CorrectTbl::MAGIC,
                                         CorrectTbl::VIT,   CorrectTbl::INT,
                                         CorrectTbl::SPEED, CorrectTbl::LUCK};

// The following are all percentage representations that always apply values
// as a numeric increase or decrease when represented as a normal percentage
const std::set<CorrectTbl> FORCE_NUMERIC = {CorrectTbl::RES_DEFAULT,
                                            CorrectTbl::RES_WEAPON,
                                            CorrectTbl::RES_SLASH,
                                            CorrectTbl::RES_THRUST,
                                            CorrectTbl::RES_STRIKE,
                                            CorrectTbl::RES_LNGR,
                                            CorrectTbl::RES_PIERCE,
                                            CorrectTbl::RES_SPREAD,
                                            CorrectTbl::RES_FIRE,
                                            CorrectTbl::RES_ICE,
                                            CorrectTbl::RES_ELEC,
                                            CorrectTbl::RES_ALMIGHTY,
                                            CorrectTbl::RES_FORCE,
                                            CorrectTbl::RES_EXPEL,
                                            CorrectTbl::RES_CURSE,
                                            CorrectTbl::RES_HEAL,
                                            CorrectTbl::RES_SUPPORT,
                                            CorrectTbl::RES_MAGICFORCE,
                                            CorrectTbl::RES_NERVE,
                                            CorrectTbl::RES_MIND,
                                            CorrectTbl::RES_WORD,
                                            CorrectTbl::RES_SPECIAL,
                                            CorrectTbl::RES_SUICIDE,
                                            CorrectTbl::COOLDOWN_TIME,
                                            CorrectTbl::RATE_XP,
                                            CorrectTbl::RATE_MAG,
                                            CorrectTbl::RATE_MACCA,
                                            CorrectTbl::RATE_EXPERTISE,
                                            CorrectTbl::RATE_CLSR,
                                            CorrectTbl::RATE_LNGR,
                                            CorrectTbl::RATE_SPELL,
                                            CorrectTbl::RATE_SUPPORT,
                                            CorrectTbl::RATE_HEAL,
                                            CorrectTbl::RATE_CLSR_TAKEN,
                                            CorrectTbl::RATE_LNGR_TAKEN,
                                            CorrectTbl::RATE_SPELL_TAKEN,
                                            CorrectTbl::RATE_SUPPORT_TAKEN,
                                            CorrectTbl::RATE_HEAL_TAKEN,
                                            CorrectTbl::BOOST_DEFAULT,
                                            CorrectTbl::BOOST_WEAPON,
                                            CorrectTbl::BOOST_SLASH,
                                            CorrectTbl::BOOST_THRUST,
                                            CorrectTbl::BOOST_STRIKE,
                                            CorrectTbl::BOOST_LNGR,
                                            CorrectTbl::BOOST_PIERCE,
                                            CorrectTbl::BOOST_SPREAD,
                                            CorrectTbl::BOOST_FIRE,
                                            CorrectTbl::BOOST_ICE,
                                            CorrectTbl::BOOST_ELEC,
                                            CorrectTbl::BOOST_ALMIGHTY,
                                            CorrectTbl::BOOST_FORCE,
                                            CorrectTbl::BOOST_EXPEL,
                                            CorrectTbl::BOOST_CURSE,
                                            CorrectTbl::BOOST_HEAL,
                                            CorrectTbl::BOOST_SUPPORT,
                                            CorrectTbl::BOOST_MAGICFORCE,
                                            CorrectTbl::BOOST_NERVE,
                                            CorrectTbl::BOOST_MIND,
                                            CorrectTbl::BOOST_WORD,
                                            CorrectTbl::BOOST_SPECIAL,
                                            CorrectTbl::BOOST_SUICIDE,
                                            CorrectTbl::LB_CHANCE,
                                            CorrectTbl::RATE_PC,
                                            CorrectTbl::RATE_DEMON,
                                            CorrectTbl::RATE_PC_TAKEN,
                                            CorrectTbl::RATE_DEMON_TAKEN,
                                            CorrectTbl::CHANT_TIME};

void ActiveEntityState::AdjustStats(
    const std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments,
    libcomp::EnumMap<CorrectTbl, int32_t>& stats,
    std::shared_ptr<objects::CalculatedEntityState> calcState, bool baseMode) {
  std::set<CorrectTbl> removed;
  libcomp::EnumMap<CorrectTbl, int32_t> maxPercents;

  if (!baseMode) {
    // If regen starts negative, lock at 0%
    if (stats[CorrectTbl::HP_REGEN] < 0) {
      stats[CorrectTbl::HP_REGEN] = 0;
      removed.insert(CorrectTbl::HP_REGEN);
    }

    if (stats[CorrectTbl::MP_REGEN] < 0) {
      stats[CorrectTbl::MP_REGEN] = 0;
      removed.insert(CorrectTbl::MP_REGEN);
    }
  }

  // Keep track of each increase to sum up and boost at the end. Percentages
  // are applied in 2 layers though most are typically in the first group.
  libcomp::EnumMap<CorrectTbl, int32_t> numericSums;
  std::array<libcomp::EnumMap<CorrectTbl, int32_t>, 2> percentSums;
  for (auto ct : adjustments) {
    auto tblID = ct->GetID();

    // Only adjust base or calculated stats depending on mode
    if (baseMode != (BASE_STATS.find(tblID) != BASE_STATS.end())) continue;

    // If a value is reduced to 0%, leave it
    if (removed.find(tblID) != removed.end()) continue;

    bool calcAdjust = false;
    uint8_t effectiveType = ct->GetType();
    int32_t effectiveValue = (int32_t)ct->GetValue();
    if (effectiveType >= 100) {
      // This is actually a TokuseiCorrectTbl, check for attributes
      // like multipliers etc and adjust the value accordingly
      effectiveType = (uint8_t)(effectiveType - 100);

      auto tct = std::dynamic_pointer_cast<objects::TokuseiCorrectTbl>(ct);
      if (tct) {
        auto attr = tct->GetAttributes();

        int32_t baseValue = 0;
        if (attr &&
            attr->GetMultiplierType() ==
                objects::TokuseiAttributes::MultiplierType_t::BASE_AND_LEVEL) {
          auto cs = GetCoreStats();
          switch (tblID) {
            case CorrectTbl::STR:
              baseValue = cs ? cs->GetSTR() : 0;
              break;
            case CorrectTbl::MAGIC:
              baseValue = cs ? cs->GetMAGIC() : 0;
              break;
            case CorrectTbl::VIT:
              baseValue = cs ? cs->GetVIT() : 0;
              break;
            case CorrectTbl::INT:
              baseValue = cs ? cs->GetINTEL() : 0;
              break;
            case CorrectTbl::SPEED:
              baseValue = cs ? cs->GetSPEED() : 0;
              break;
            case CorrectTbl::LUCK:
              baseValue = cs ? cs->GetLUCK() : 0;
              break;
            default:
              break;
          }
        }

        effectiveValue = (int32_t)TokuseiManager::CalculateAttributeValue(
            this, effectiveValue, baseValue, attr);
      }

      calcAdjust = true;
    }

    if (effectiveType == 1 &&
        FORCE_NUMERIC.find(tblID) != FORCE_NUMERIC.end()) {
      effectiveType = 0;
    }

    if ((uint8_t)tblID >= (uint8_t)CorrectTbl::NRA_WEAPON &&
        (uint8_t)tblID <= (uint8_t)CorrectTbl::NRA_MAGIC) {
      if (!calcState) continue;

      // NRA is calculated differently from everything else
      if (effectiveType == 0) {
        // For type 0, the NRA value becomes 100% and CANNOT be reduced.
        switch (effectiveValue) {
          case NRA_NULL:
            removed.insert(tblID);
            calcState->SetNullChances((int16_t)tblID, 100);
            break;
          case NRA_REFLECT:
            removed.insert(tblID);
            calcState->SetReflectChances((int16_t)tblID, 100);
            break;
          case NRA_ABSORB:
            removed.insert(tblID);
            calcState->SetAbsorbChances((int16_t)tblID, 100);
            break;
          default:
            break;
        }
      } else {
        // For other types, reduce the value by 2 to get the NRA index
        // and add the value supplied.
        switch (effectiveType) {
          case (NRA_NULL + 2):
            calcState->SetNullChances(
                (int16_t)tblID,
                (int16_t)(calcState->GetNullChances((int16_t)tblID) +
                          effectiveValue));
            break;
          case (NRA_REFLECT + 2):
            calcState->SetReflectChances(
                (int16_t)tblID,
                (int16_t)(calcState->GetReflectChances((int16_t)tblID) +
                          effectiveValue));
            break;
          case (NRA_ABSORB + 2):
            calcState->SetAbsorbChances(
                (int16_t)tblID,
                (int16_t)(calcState->GetAbsorbChances((int16_t)tblID) +
                          effectiveValue));
            break;
          default:
            break;
        }
      }
    } else {
      libcomp::EnumMap<CorrectTbl, int32_t>* map = 0;

      switch (effectiveType) {
        case 1:
        case 2:
          // Percentage sets can either be an immutable set to zero
          // or an increase/decrease by a set amount
          if (effectiveValue == 0) {
            // Ignore calculated values that set to 0%
            if (!calcAdjust) {
              removed.insert(tblID);
              stats[tblID] = 0;
              numericSums.erase(tblID);
              percentSums[0].erase(tblID);
              percentSums[1].erase(tblID);
              maxPercents.erase(tblID);
            }
          } else {
            size_t pIdx = (size_t)(effectiveType - 1);

            // Store max percents separately
            if (maxPercents.find(tblID) == maxPercents.end()) {
              maxPercents[tblID] = effectiveValue;
            } else if (maxPercents[tblID] < effectiveValue) {
              maxPercents[tblID] = effectiveValue;
            }

            map = &percentSums[pIdx];
          }
          break;
        case 0:
          map = &numericSums;
          break;
        default:
          break;
      }

      if (map) {
        if (map->find(tblID) == map->end()) {
          (*map)[tblID] = effectiveValue;
        } else {
          (*map)[tblID] += effectiveValue;
        }
      }
    }
  }

  // Now that we have all the sums, calculate stats in order
  for (size_t i = 0; i < 126; i++) {
    CorrectTbl tblID = (CorrectTbl)i;

    if (baseMode != (BASE_STATS.find(tblID) != BASE_STATS.end())) continue;

    // Stats are applied in a specific order. Starting with the base
    // value it is as follows:
    // 1) Percentage adjustments (layer 1)
    // 2) Numeric adjustments
    // 3) Percentage adjustments (layer 2)
    // HP/MP regen are special as they are dependent stats calculated from
    // Max HP/MP after step 2.
    for (size_t layer = 0; layer < 3; layer++) {
      if (layer == 1) {
        // Numeric adjust
        auto it = numericSums.find(tblID);
        if (it != numericSums.end()) {
          stats[tblID] = (int32_t)(stats[tblID] + it->second);
        }

        switch (tblID) {
          case CorrectTbl::HP_MAX:
            // Determine base HP regen (if not 0%)
            if (removed.find(CorrectTbl::HP_REGEN) == removed.end()) {
              int32_t hpMax = stats[CorrectTbl::HP_MAX];
              int32_t vit = stats[CorrectTbl::VIT];
              stats[CorrectTbl::HP_REGEN] =
                  (int32_t)(stats[CorrectTbl::HP_REGEN] +
                            (int32_t)floor(((vit * 3) + hpMax) * 0.01));
            }

            // Add enemy only HP if it exists (can be boosted by layer
            // 2 effects)
            if (GetEntityType() == EntityType_t::ALLY ||
                GetEntityType() == EntityType_t::ENEMY) {
              auto extension = GetEnemyBase()->GetExtension();
              if (!extension || !extension->GetOverrideStats()) {
                stats[CorrectTbl::HP_MAX] =
                    stats[CorrectTbl::HP_MAX] +
                    GetDevilData()->GetBattleData()->GetEnemyHP(0);
              }
            }
            break;
          case CorrectTbl::MP_MAX:
            // Determine base MP regen (if not 0%)
            if (removed.find(CorrectTbl::MP_REGEN) == removed.end()) {
              int32_t mpMax = stats[CorrectTbl::MP_MAX];
              int32_t intel = stats[CorrectTbl::INT];
              stats[CorrectTbl::MP_REGEN] =
                  (int32_t)(stats[CorrectTbl::MP_REGEN] +
                            (int32_t)floor(((intel * 3) + mpMax) * 0.01));
            }
            break;
          default:
            break;
        }
      } else {
        // Percentage adjust
        size_t idx = layer == 0 ? 0 : 1;
        auto it = percentSums[idx].find(tblID);
        if (it != percentSums[idx].end()) {
          int32_t sum = it->second;

          int32_t adjusted = stats[tblID];
          if (sum <= -100) {
            adjusted = 0;
          } else {
            adjusted = (int32_t)(adjusted + (int32_t)(adjusted * (sum * 0.01)));
          }

          stats[tblID] = adjusted;
        }
      }
    }
  }

  // Apply stat minimum bounds (and maximum if not an enemy)
  CharacterManager::AdjustStatBounds(stats,
                                     GetEntityType() != EntityType_t::ENEMY);

  // Limit max speed based on direct percentage boosts (player entities only)
  if (!baseMode && (GetEntityType() == EntityType_t::CHARACTER ||
                    GetEntityType() == EntityType_t::PARTNER_DEMON)) {
    for (CorrectTbl tblID : {CorrectTbl::MOVE1, CorrectTbl::MOVE2}) {
      int16_t maxSpeed = 0;
      auto moveIter = maxPercents.find(tblID);
      if (moveIter != maxPercents.end()) {
        maxSpeed = (int16_t)moveIter->second;
      }

      // Floor at max increase from default run speed
      const static int16_t MAX_SUM = (int16_t)SVR_CONST.MAX_MOVE_INCREASE_SUM;
      if (maxSpeed < MAX_SUM) {
        maxSpeed = MAX_SUM;
      }

      maxSpeed = (int16_t)ceil((double)STAT_DEFAULT_SPEED *
                               (1.0 + (double)maxSpeed * 0.01));
      if (stats[tblID] > maxSpeed) {
        stats[tblID] = maxSpeed;
      }
    }
  }
}

void ActiveEntityState::UpdateNRAChances(
    libcomp::EnumMap<CorrectTbl, int32_t>& stats,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    const std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments) {
  std::unordered_map<int16_t, int16_t> mNullMap;
  std::unordered_map<int16_t, int16_t> mReflectMap;
  std::unordered_map<int16_t, int16_t> mAbsorbMap;

  // Set from base
  for (uint8_t x = (uint8_t)CorrectTbl::NRA_WEAPON;
       x <= (uint8_t)CorrectTbl::NRA_MAGIC; x++) {
    CorrectTbl tblID = (CorrectTbl)x;
    int16_t val = (int16_t)stats[tblID];
    if (val > 0) {
      // Natural NRA is stored as NRA index in the 1s place and
      // percentage of success as the rest
      double shift = (double)(val * 0.1);
      uint8_t nraIdx =
          (uint8_t)((shift - (double)floorl((double)val / 10.0)) * 10.0);
      val = (int16_t)floorl(val / 10);
      switch (nraIdx) {
        case NRA_NULL:
          mNullMap[(int16_t)tblID] = val;
          break;
        case NRA_REFLECT:
          mReflectMap[(int16_t)tblID] = val;
          break;
        case NRA_ABSORB:
          mAbsorbMap[(int16_t)tblID] = val;
          break;
        default:
          break;
      }
    }
  }

  // Equipment adjustments use type equal to the NRA index and
  // relative value to add
  for (auto ct : adjustments) {
    int16_t tblID = (int16_t)ct->GetID();
    switch (ct->GetType()) {
      case NRA_NULL:
        mNullMap[tblID] = (int16_t)(mNullMap[tblID] + ct->GetValue());
        break;
      case NRA_REFLECT:
        mReflectMap[tblID] = (int16_t)(mReflectMap[tblID] + ct->GetValue());
        break;
      case NRA_ABSORB:
        mAbsorbMap[tblID] = (int16_t)(mAbsorbMap[tblID] + ct->GetValue());
        break;
      default:
        break;
    }
  }

  calcState->SetNullChances(mNullMap);
  calcState->SetReflectChances(mReflectMap);
  calcState->SetAbsorbChances(mAbsorbMap);
}

void ActiveEntityState::GetAdditionalCorrectTbls(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments,
    std::shared_ptr<objects::MiSkillData> contextSkill) {
  // 1) Gather skill adjustments
  auto currentSkillIDs = GetCurrentSkills();
  ApplySkillCorrectTbls(currentSkillIDs, definitionManager, adjustments);

  // 2) Gather status effect adjustments
  for (auto ePair : GetStatusEffects()) {
    auto statusData = definitionManager->GetStatusData(ePair.first);
    for (auto ct : statusData->GetCommon()->GetCorrectTbl()) {
      uint8_t multiplier = (statusData->GetBasic()->GetStackType() == 2)
                               ? ePair.second->GetStack()
                               : 1;
      for (uint8_t i = 0; i < multiplier; i++) {
        adjustments.push_back(ct);
      }
    }
  }

  // 3) Gather tokusei effective adjustments
  for (auto tPair : calcState->GetEffectiveTokusei()) {
    auto tokusei = definitionManager->GetTokuseiData(tPair.first);
    if (tokusei && (tokusei->CorrectValuesCount() > 0 ||
                    tokusei->TokuseiCorrectValuesCount() > 0)) {
      // Add the entries once for each source applying them
      for (uint16_t i = 0; i < tPair.second; i++) {
        for (auto ct : tokusei->GetCorrectValues()) {
          adjustments.push_back(ct);
        }

        for (auto ct : tokusei->GetTokuseiCorrectValues()) {
          adjustments.push_back(ct);
        }
      }
    }
  }

  // 4) Gather skill adjustments but only if applying to a skill contextual
  // calculated state
  if (contextSkill && calcState != GetCalculatedState()) {
    for (auto ct : contextSkill->GetCommon()->GetCorrectTbl()) {
      adjustments.push_back(ct);
    }
  }

  // Sort the adjustments, set to 0% first, non-zero percents next, numeric last
  adjustments.sort([](const std::shared_ptr<objects::MiCorrectTbl>& a,
                      const std::shared_ptr<objects::MiCorrectTbl>& b) {
    return ((a->GetType() % 100) > 0) &&
           (a->GetValue() == 0 || ((b->GetType() % 100) == 0));
  });
}

void ActiveEntityState::ApplySkillCorrectTbls(
    const std::set<uint32_t>& skillIDs,
    libcomp::DefinitionManager* definitionManager,
    std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments) {
  for (auto skillID : skillIDs) {
    auto skillData = definitionManager->GetSkillData(skillID);
    auto common = skillData->GetCommon();

    bool include = false;
    switch (common->GetCategory()->GetMainCategory()) {
      case SKILL_CATEGORY_PASSIVE:
        include = true;
        break;
      case SKILL_CATEGORY_SWITCH:
        include = ActiveSwitchSkillsContains(skillID);
        break;
      default:
        break;
    }

    if (include && !DisabledSkillsContains(skillID)) {
      for (auto ct : common->GetCorrectTbl()) {
        adjustments.push_back(ct);
      }
    }
  }
}

uint8_t ActiveEntityState::RecalculateDemonStats(
    libcomp::DefinitionManager* definitionManager,
    libcomp::EnumMap<CorrectTbl, int32_t>& stats,
    std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    std::shared_ptr<objects::MiSkillData> contextSkill) {
  bool selfState = calcState == GetCalculatedState();
  if (selfState) {
    // Combat run speed can change from unadjusted stats
    SetCombatRunSpeed((int16_t)stats[CorrectTbl::MOVE2]);

    if (!mInitialCalc) {
      SetKnockbackResist((float)stats[CorrectTbl::KNOCKBACK_RESIST]);
      mInitialCalc = true;
    }
  }

  GetAdditionalCorrectTbls(definitionManager, calcState, adjustments,
                           contextSkill);

  UpdateNRAChances(stats, calcState);
  AdjustStats(adjustments, stats, calcState, true);

  CharacterManager::CalculateDependentStats(stats, GetCoreStats()->GetLevel(),
                                            true);

  uint8_t result = 0;
  if (selfState) {
    result = CompareAndResetStats(stats, true);
  }

  AdjustStats(adjustments, stats, calcState, false);

  if (selfState) {
    return result | CompareAndResetStats(stats, false);
  } else {
    for (auto statPair : stats) {
      calcState->SetCorrectTbl((size_t)statPair.first,
                               (int16_t)statPair.second);
    }

    return 0;
  }
}

uint8_t ActiveEntityState::RecalculateEnemyStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    std::shared_ptr<objects::MiSkillData> contextSkill) {
  bool selfState = calcState == GetCalculatedState();
  if (calcState == nullptr) {
    // Calculating default entity state
    calcState = GetCalculatedState();
    selfState = true;
  }

  if (selfState) {
    auto previousSkills = GetCurrentSkills();
    SetCurrentSkills(GetAllSkills(definitionManager, true));

    bool skillsChanged = previousSkills.size() != CurrentSkillsCount();
    if (!skillsChanged) {
      for (uint32_t skillID : previousSkills) {
        if (!CurrentSkillsContains(skillID)) {
          skillsChanged = true;
          break;
        }
      }
    }

    // If this is an AI entity and the available skills changed or cost
    // adjust tokusei exist, remap skills to get new options and weights
    if (mAIState && (skillsChanged ||
                     calcState->ExistingTokuseiAspectsContains(
                         (int8_t)TokuseiAspectType::HP_COST_ADJUST) ||
                     calcState->ExistingTokuseiAspectsContains(
                         (int8_t)TokuseiAspectType::MP_COST_ADJUST))) {
      mAIState->ResetSkillsMapped();
    }
  }

  auto cs = GetCoreStats();
  auto devilData = GetDevilData();
  if (!cs || !devilData) {
    return true;
  }

  auto battleData = devilData->GetBattleData();

  libcomp::EnumMap<CorrectTbl, int32_t> stats;
  std::list<std::shared_ptr<objects::MiCorrectTbl>> adjustments;

  auto eBase = GetEnemyBase();
  auto extension = eBase ? eBase->GetExtension() : nullptr;
  if (extension && extension->GetOverrideStats()) {
    for (size_t i = 0; i < 126; i++) {
      CorrectTbl tblID = (CorrectTbl)i;
      stats[tblID] = extension->GetCorrectTbl((size_t)i);
    }

    adjustments = extension->GetStatBoosts();
  } else {
    for (size_t i = 0; i < 126; i++) {
      CorrectTbl tblID = (CorrectTbl)i;
      stats[tblID] = battleData->GetCorrect((size_t)i);
    }

    // Non-dependent stats will not change from growth calculation
    stats[CorrectTbl::STR] = cs->GetSTR();
    stats[CorrectTbl::MAGIC] = cs->GetMAGIC();
    stats[CorrectTbl::VIT] = cs->GetVIT();
    stats[CorrectTbl::INT] = cs->GetINTEL();
    stats[CorrectTbl::SPEED] = cs->GetSPEED();
    stats[CorrectTbl::LUCK] = cs->GetLUCK();
  }

  return RecalculateDemonStats(definitionManager, stats, adjustments, calcState,
                               contextSkill);
}

std::set<uint32_t> ActiveEntityState::GetAllEnemySkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei) {
  std::set<uint32_t> skillIDs;

  auto eBase = GetEnemyBase();
  auto extension = eBase ? eBase->GetExtension() : nullptr;
  if (extension) {
    skillIDs = extension->GetAddedSkills();
  }

  if (!extension || !extension->GetNoDefaultSkills()) {
    auto demonData = GetDevilData();

    auto growth = demonData->GetGrowth();
    for (auto skillSet : {growth->GetSkills(), growth->GetEnemyOnlySkills()}) {
      for (uint32_t skillID : skillSet) {
        if (skillID) {
          skillIDs.insert(skillID);
        }
      }
    }

    for (uint32_t traitID : growth->GetTraits()) {
      if (traitID) {
        skillIDs.insert(traitID);
      }
    }
  }

  if (includeTokusei) {
    for (uint32_t skillID : GetEffectiveTokuseiSkills(definitionManager)) {
      skillIDs.insert(skillID);
    }
  }

  return skillIDs;
}

uint8_t ActiveEntityState::CalculateLNCType(int16_t lncPoints) const {
  if (lncPoints >= 5000) {
    return LNC_CHAOS;
  } else if (lncPoints <= -5000) {
    return LNC_LAW;
  } else {
    return LNC_NEUTRAL;
  }
}

void ActiveEntityState::RemoveInactiveSwitchSkills() {
  auto previousSwitchSkills = GetActiveSwitchSkills();
  for (uint32_t skillID : previousSwitchSkills) {
    if (!CurrentSkillsContains(skillID)) {
      RemoveActiveSwitchSkills(skillID);
    }
  }
}

std::set<uint32_t> ActiveEntityState::GetEffectiveTokuseiSkills(
    libcomp::DefinitionManager* definitionManager) {
  std::set<uint32_t> skillIDs;

  for (auto tPair : GetCalculatedState()->GetEffectiveTokusei()) {
    auto tokusei = definitionManager->GetTokuseiData(tPair.first);
    if (tokusei) {
      for (auto aspect : tokusei->GetAspects()) {
        if (aspect->GetType() == TokuseiAspectType::SKILL_ADD) {
          skillIDs.insert((uint32_t)aspect->GetValue());
        }
      }
    }
  }

  return skillIDs;
}

const std::set<CorrectTbl> VISIBLE_STATS = {
    CorrectTbl::STR,    CorrectTbl::MAGIC,  CorrectTbl::VIT,
    CorrectTbl::INT,    CorrectTbl::SPEED,  CorrectTbl::LUCK,
    CorrectTbl::HP_MAX, CorrectTbl::MP_MAX, CorrectTbl::CLSR,
    CorrectTbl::LNGR,   CorrectTbl::SPELL,  CorrectTbl::SUPPORT,
    CorrectTbl::PDEF,   CorrectTbl::MDEF};

uint8_t ActiveEntityState::CompareAndResetStats(
    libcomp::EnumMap<CorrectTbl, int32_t>& stats, bool dependentBase) {
  // Limits are assumed to have already been applied at this point
  uint8_t result = 0;
  if (dependentBase) {
    if (GetCLSRBase() != (int16_t)stats[CorrectTbl::CLSR] ||
        GetLNGRBase() != (int16_t)stats[CorrectTbl::LNGR] ||
        GetSPELLBase() != (int16_t)stats[CorrectTbl::SPELL] ||
        GetSUPPORTBase() != (int16_t)stats[CorrectTbl::SUPPORT] ||
        GetPDEFBase() != (int16_t)stats[CorrectTbl::PDEF] ||
        GetMDEFBase() != (int16_t)stats[CorrectTbl::MDEF]) {
      result |= ENTITY_CALC_STAT_LOCAL;
    }

    SetCLSRBase((int16_t)stats[CorrectTbl::CLSR]);
    SetLNGRBase((int16_t)stats[CorrectTbl::LNGR]);
    SetSPELLBase((int16_t)stats[CorrectTbl::SPELL]);
    SetSUPPORTBase((int16_t)stats[CorrectTbl::SUPPORT]);
    SetPDEFBase((int16_t)stats[CorrectTbl::PDEF]);
    SetMDEFBase((int16_t)stats[CorrectTbl::MDEF]);

    return result;
  } else {
    auto cs = GetCoreStats();
    int32_t hp = cs->GetHP();
    int32_t mp = cs->GetMP();
    int32_t newMaxHP = stats[CorrectTbl::HP_MAX];
    int32_t newMaxMP = stats[CorrectTbl::MP_MAX];

    if (hp > newMaxHP) {
      hp = newMaxHP;
    }

    if (mp > newMaxMP) {
      mp = newMaxMP;
    }

    auto calcState = GetCalculatedState();
    if (calcState->GetCorrectTbl((size_t)CorrectTbl::MOVE1) !=
            (int16_t)stats[CorrectTbl::MOVE1] ||
        calcState->GetCorrectTbl((size_t)CorrectTbl::MOVE2) !=
            (int16_t)stats[CorrectTbl::MOVE2]) {
      result |= ENTITY_CALC_MOVE_SPEED;
    }

    for (auto statPair : stats) {
      if (statPair.second > (int32_t)std::numeric_limits<int16_t>::max()) {
        calcState->SetCorrectTbl((size_t)statPair.first,
                                 (int16_t)std::numeric_limits<int16_t>::max());
      } else {
        calcState->SetCorrectTbl((size_t)statPair.first,
                                 (int16_t)statPair.second);
      }
    }

    if (hp != cs->GetHP() || mp != cs->GetMP() || GetMaxHP() != newMaxHP ||
        GetMaxMP() != newMaxMP) {
      result |= ENTITY_CALC_STAT_WORLD | ENTITY_CALC_STAT_LOCAL;
    } else if (GetSTR() != (int16_t)stats[CorrectTbl::STR] ||
               GetMAGIC() != (int16_t)stats[CorrectTbl::MAGIC] ||
               GetVIT() != (int16_t)stats[CorrectTbl::VIT] ||
               GetINTEL() != (int16_t)stats[CorrectTbl::INT] ||
               GetSPEED() != (int16_t)stats[CorrectTbl::SPEED] ||
               GetLUCK() != (int16_t)stats[CorrectTbl::LUCK] ||
               GetCLSR() != (int16_t)stats[CorrectTbl::CLSR] ||
               GetLNGR() != (int16_t)stats[CorrectTbl::LNGR] ||
               GetSPELL() != (int16_t)stats[CorrectTbl::SPELL] ||
               GetSUPPORT() != (int16_t)stats[CorrectTbl::SUPPORT] ||
               GetPDEF() != (int16_t)stats[CorrectTbl::PDEF] ||
               GetMDEF() != (int16_t)stats[CorrectTbl::MDEF]) {
      result |= ENTITY_CALC_STAT_LOCAL;
    }

    cs->SetHP(hp);
    cs->SetMP(mp);
    SetMaxHP(newMaxHP);
    SetMaxMP(newMaxMP);
    SetSTR((int16_t)stats[CorrectTbl::STR]);
    SetMAGIC((int16_t)stats[CorrectTbl::MAGIC]);
    SetVIT((int16_t)stats[CorrectTbl::VIT]);
    SetINTEL((int16_t)stats[CorrectTbl::INT]);
    SetSPEED((int16_t)stats[CorrectTbl::SPEED]);
    SetLUCK((int16_t)stats[CorrectTbl::LUCK]);
    SetCLSR((int16_t)stats[CorrectTbl::CLSR]);
    SetLNGR((int16_t)stats[CorrectTbl::LNGR]);
    SetSPELL((int16_t)stats[CorrectTbl::SPELL]);
    SetSUPPORT((int16_t)stats[CorrectTbl::SUPPORT]);
    SetPDEF((int16_t)stats[CorrectTbl::PDEF]);
    SetMDEF((int16_t)stats[CorrectTbl::MDEF]);
  }

  return result;
}
