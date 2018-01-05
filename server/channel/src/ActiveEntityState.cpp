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

#include "ActiveEntityState.h"

// libcomp Includes
#include <Constants.h>
#include <DefinitionManager.h>
#include <ServerConstants.h>

// C++ Standard Includes
#include <cmath>
#include <limits>

// objects Includes
#include <CalculatedEntityState.h>
#include <Character.h>
#include <Clan.h>
#include <Demon.h>
#include <Enemy.h>
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
#include <Tokusei.h>
#include <TokuseiAspect.h>
#include <TokuseiCorrectTbl.h>

// channel Includes
#include "AIState.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"
#include "Zone.h"

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<ActiveEntityState>()
    {
        if(!BindingExists("ActiveEntityState"))
        {
            Using<AIState>();
            Using<objects::ActiveEntityStateObject>();

            // Active entities can rotate or stop directly from the script
            // but movement must be handled via the AIManager
            Sqrat::DerivedClass<ActiveEntityState,
                objects::ActiveEntityStateObject> binding(mVM, "ActiveEntityState");
            binding
                .Func<void (ActiveEntityState::*)(float, uint64_t)>(
                    "Rotate", &ActiveEntityState::Rotate)
                .Func<void (ActiveEntityState::*)(uint64_t)>(
                    "Stop", &ActiveEntityState::Stop)
                .Func<bool (ActiveEntityState::*)(void) const>(
                    "IsMoving", &ActiveEntityState::IsMoving)
                .Func<bool (ActiveEntityState::*)(void) const>(
                    "IsRotating", &ActiveEntityState::IsRotating)
                .Func<std::shared_ptr<AIState>(ActiveEntityState::*)() const>(
                    "GetAIState", &ActiveEntityState::GetAIState)
                .Func<uint64_t (ActiveEntityState::*)(const libcomp::String&)>(
                    "GetActionTime", &ActiveEntityState::GetActionTime)
                .Func<void (ActiveEntityState::*)(const libcomp::String&, uint64_t)>(
                    "SetActionTime", &ActiveEntityState::SetActionTime);

            Bind<ActiveEntityState>("ActiveEntityState", binding);
        }

        return *this;
    }
}

ActiveEntityState::ActiveEntityState() : mCurrentZone(0),
    mEffectsActive(false), mAlive(true), mInitialCalc(false),
    mLastRefresh(0), mNextActivatedAbilityID(1)
{
}

ActiveEntityState::ActiveEntityState(const ActiveEntityState& other)
{
    (void)other;
}

int16_t ActiveEntityState::GetCorrectValue(CorrectTbl tableID,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    return (calcState ? calcState : GetCalculatedState())
        ->GetCorrectTbl((size_t)tableID);
}

const libobjgen::UUID ActiveEntityState::GetEntityUUID()
{
    return NULLUUID;
}

void ActiveEntityState::Move(float xPos, float yPos, uint64_t now)
{
    if(CanMove())
    {
        SetOriginX(GetCurrentX());
        SetOriginY(GetCurrentY());
        SetOriginTicks(now);

        // Rotate instantly
        float destRot = (float)atan2(GetCurrentY() - yPos,
            GetCurrentX() - xPos);
        SetOriginRotation(destRot);
        SetDestinationRotation(destRot);

        uint64_t addMicro = (uint64_t)(
            (double)(GetDistance(xPos, yPos) /
            GetMovementSpeed()) * 1000000.0);

        SetDestinationX(xPos);
        SetDestinationY(yPos);
        SetDestinationTicks((uint64_t)(now + addMicro));
    }
}

void ActiveEntityState::Rotate(float rot, uint64_t now)
{
    if(CanMove())
    {
        SetOriginX(GetCurrentX());
        SetOriginY(GetCurrentY());
        SetOriginRotation(GetCurrentRotation());
        SetOriginTicks(now);

        SetDestinationRotation(CorrectRotation(rot));

        // One complete rotation takes 1650ms at 300.0f speed
        uint64_t addMicro = (uint64_t)(495000.0f /
            GetMovementSpeed(mOpponentIDs.size() > 0)) * 1000;
        SetDestinationTicks(now + addMicro);
    }
}

void ActiveEntityState::Stop(uint64_t now)
{
    SetDestinationX(GetCurrentX());
    SetDestinationY(GetCurrentY());
    SetDestinationRotation(GetCurrentRotation());
    SetDestinationTicks(now);
    SetOriginX(GetCurrentX());
    SetOriginY(GetCurrentY());
    SetOriginRotation(GetCurrentRotation());
    SetOriginTicks(now);
}

bool ActiveEntityState::IsAlive() const
{
    return mAlive;
}

bool ActiveEntityState::IsMoving() const
{
    return GetCurrentX() != GetDestinationX() ||
        GetCurrentY() != GetDestinationY();
}

bool ActiveEntityState::IsRotating() const
{
    return GetCurrentRotation() != GetDestinationRotation();
}

bool ActiveEntityState::CanAct()
{
    return mAlive && (CanMove() || CurrentSkillsCount() > 0);
}

bool ActiveEntityState::CanMove()
{
    if(!mAlive || GetCorrectValue(CorrectTbl::MOVE1) == 0)
    {
        return false;
    }

    auto statusTimes = GetStatusTimes();
    if(statusTimes.size() > 0)
    {
        for(uint8_t lockState : { STATUS_CHARGING, STATUS_KNOCKBACK,
            STATUS_HIT_STUN, STATUS_IMMOBILE, STATUS_RESTING })
        {
            if(statusTimes.find(lockState) != statusTimes.end())
            {
                return false;
            }
        }
    }

    return true;
}

float ActiveEntityState::CorrectRotation(float rot)
{
    if(rot > 3.14f)
    {
        return rot - 6.28f;
    }
    else if(rot < -3.14f)
    {
        return -rot - 3.14f;
    }

    return rot;
}

float ActiveEntityState::GetDistance(float x, float y, bool squared)
{
    float dSquared = (float)(std::pow((GetCurrentX() - x), 2)
        + std::pow((GetCurrentY() - y), 2));
    return squared ? dSquared : std::sqrt(dSquared);
}

float ActiveEntityState::GetMovementSpeed(bool altSpeed)
{
    return (float)(GetCorrectValue(
        altSpeed ? CorrectTbl::MOVE1 : CorrectTbl::MOVE2) * 10.f);
}

void ActiveEntityState::RefreshCurrentPosition(uint64_t now)
{
    if(now != mLastRefresh)
    {
        uint64_t destTicks = GetDestinationTicks();
        if(destTicks < mLastRefresh)
        {
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

        if(!xDiff && !yDiff && !rotDiff)
        {
            // Already up to date
            return;
        }

        if(now >= destTicks)
        {
            SetCurrentX(destX);
            SetCurrentY(destY);
            SetCurrentRotation(destRot);
        }
        else
        {
            float originX = GetOriginX();
            float originY = GetOriginY();
            float originRot = GetOriginRotation();
            uint64_t originTicks = GetOriginTicks();

            uint64_t elapsed = now - originTicks;
            uint64_t total = destTicks - originTicks;
            if(total == 0)
            {
                SetCurrentX(destX);
                SetCurrentY(destY);
                SetCurrentRotation(destRot);
                return;
            }

            double prog = (double)((double)elapsed/(double)total);
            if(xDiff || yDiff)
            {
                float newX = (float)(originX + (prog * (destX - originX)));
                float newY = (float)(originY + (prog * (destY - originY)));

                SetCurrentX(newX);
                SetCurrentY(newY);
            }

            if(rotDiff)
            {
                // Bump both origin and destination by 3.14 to range from
                // 0-+6.28 instead of -3.14-+3.14 for simpler math
                originRot += 3.14f;
                destRot += 3.14f;

                float newRot = (float)(originRot + (prog * (destRot - originRot)));

                SetCurrentRotation(CorrectRotation(newRot));
            }
        }
    }
}

void ActiveEntityState::ExpireStatusTimes(uint64_t now)
{
    auto statusTimes = GetStatusTimes();
    if(statusTimes.size() > 0)
    {
        for(auto pair : statusTimes)
        {
            if(pair.second != 0 && pair.second <= now)
            {
                RemoveStatusTimes(pair.first);
            }
        }
    }
}

std::shared_ptr<AIState> ActiveEntityState::GetAIState() const
{
    return mAIState;
}

void ActiveEntityState::SetAIState(const std::shared_ptr<AIState>& aiState)
{
    mAIState = aiState;
}

uint64_t ActiveEntityState::GetActionTime(const libcomp::String& action)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mActionTimes.find(action.C());
    if(it != mActionTimes.end())
    {
        return it->second;
    }

    return 0;
}

void ActiveEntityState::SetActionTime(const libcomp::String& action, uint64_t time)
{
    std::lock_guard<std::mutex> lock(mLock);
    mActionTimes[action.C()] = time;
}

void ActiveEntityState::RefreshKnockback(uint64_t now)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto kb = GetKnockbackResist();
    float kbMax = (float)GetCorrectValue(CorrectTbl::KNOCKBACK_RESIST);
    if(kb < kbMax)
    {
        // Knockback refreshes at a rate of 15/s (or 0.015/ms)
        kb = kb + (float)((double)(now - GetKnockbackTicks()) * 0.001 * 0.015);
        if(kb > kbMax)
        {
            kb = kbMax;
        }
        else if(kb < 0.f)
        {
            // Sanity check
            kb = 0.f;
        }

        SetKnockbackResist(kb);
        if(kb == kbMax)
        {
            // Reset to no time
            SetKnockbackTicks(0);
        }
    }
}

float ActiveEntityState::UpdateKnockback(uint64_t now, float decrease)
{
    // Always get up to date first
    RefreshKnockback(now);

    std::lock_guard<std::mutex> lock(mLock);

    auto kb = GetKnockbackResist();
    if(kb > 0)
    {
        kb -= decrease;
        if(kb <= 0)
        {
            kb = 0.f;
        }

        SetKnockbackResist(kb);
        SetKnockbackTicks(now);
    }

    return kb;
}

std::shared_ptr<Zone> ActiveEntityState::GetZone() const
{
    return mCurrentZone;
}

void ActiveEntityState::SetZone(const std::shared_ptr<Zone>& zone, bool updatePrevious)
{
    if(updatePrevious && mCurrentZone)
    {
        mCurrentZone->SetNextStatusEffectTime(0, GetEntityID());
    }

    mCurrentZone = zone;

    RegisterNextEffectTime();
}

bool ActiveEntityState::SetHPMP(int32_t hp, int32_t mp, bool adjust,
    bool canOverflow)
{
    int32_t hpAdjusted, mpAdjusted;
    return SetHPMP(hp, mp, adjust, canOverflow, hpAdjusted, mpAdjusted);
}

bool ActiveEntityState::SetHPMP(int32_t hp, int32_t mp, bool adjust,
    bool canOverflow, int32_t& hpAdjusted, int32_t& mpAdjusted)
{
    hpAdjusted = mpAdjusted = 0;

    auto cs = GetCoreStats();
    if(cs == nullptr || (!adjust && hp < 0 && mp < 0))
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mLock);
    auto maxHP = GetMaxHP();
    auto maxMP = GetMaxMP();

    // If the amount of damage dealt can overflow, do not
    // use the actual damage dealt, allow it to go
    // over the actual amount dealt
    if(canOverflow)
    {
        hpAdjusted = hp;
        mpAdjusted = mp;
    }

    if(adjust)
    {
        hp = (int32_t)(cs->GetHP() + hp);
        mp = (int32_t)(cs->GetMP() + mp);

        if(!canOverflow)
        {
            // If the adjusted damage cannot overflow
            // stop it from doing so
            if(cs->GetHP() && hp <= 0)
            {
                hp = 1;
            }
            else if(!mAlive && hp > 0)
            {
                hp = 0;
            }
        }
        
        // Make sure we don't go under the limit
        if(hp < 0)
        {
            hp = 0;
        }

        if(mp < 0)
        {
            mp = 0;
        }
    }

    bool result = false;
    bool returnDamaged = !adjust || !canOverflow;
    if(hp >= 0)
    {
        auto newHP = hp > maxHP ? maxHP : hp;

        // Update if the entity is alive or not
        if(cs->GetHP() > 0 && newHP == 0)
        {
            mAlive = false;
            Stop(ChannelServer::GetServerTime());
            result = !returnDamaged;
        }
        else if(cs->GetHP() == 0 && newHP > 0)
        {
            mAlive = true;
            result = !returnDamaged;
        }

        result |= returnDamaged && newHP != cs->GetHP();
        
        if(!canOverflow)
        {
            hpAdjusted = (int32_t)(newHP - cs->GetHP());
        }

        cs->SetHP(newHP);
    }
    
    if(mp >= 0)
    {
        auto newMP = mp > maxMP ? maxMP : mp;
        result |= returnDamaged && newMP != cs->GetMP();
        
        if(!canOverflow)
        {
            mpAdjusted = (int32_t)(newMP - cs->GetMP());
        }

        cs->SetMP(newMP);
    }

    return result;
}

const std::unordered_map<uint32_t, std::shared_ptr<objects::StatusEffect>>&
    ActiveEntityState::GetStatusEffects() const
{
    return mStatusEffects;
}

void ActiveEntityState::SetStatusEffects(
    const std::list<std::shared_ptr<objects::StatusEffect>>& effects)
{
    std::lock_guard<std::mutex> lock(mLock);
    mStatusEffects.clear();
    mNRAShields.clear();
    mTimeDamageEffects.clear();
    mCancelConditions.clear();
    mNextEffectTimes.clear();

    RegisterNextEffectTime();

    for(auto effect : effects)
    {
        mStatusEffects[effect->GetEffect()] = effect;
    }
}

std::set<uint32_t> ActiveEntityState::AddStatusEffects(const AddStatusEffectMap& effects,
    libcomp::DefinitionManager* definitionManager, uint32_t now, bool queueChanges)
{
    std::set<uint32_t> removes;

    if(now == 0)
    {
        now = (uint32_t)std::time(0);
    }

    std::lock_guard<std::mutex> lock(mLock);
    for(auto ePair : effects)
    {
        bool isReplace = ePair.second.second;
        uint32_t effectType = ePair.first;
        uint8_t stack = ePair.second.first;

        auto def = definitionManager->GetStatusData(effectType);
        auto basic = def->GetBasic();
        auto cancel = def->GetCancel();
        auto maxStack = basic->GetMaxStack();

        if(stack > maxStack)
        {
            stack = maxStack;
        }

        bool add = true;
        std::shared_ptr<objects::StatusEffect> effect;
        std::shared_ptr<objects::StatusEffect> removeEffect;
        if(mStatusEffects.find(effectType) != mStatusEffects.end())
        {
            // Effect exists alreadly, should we modify time/stack or remove?
            auto existing = mStatusEffects[effectType];

            bool doReplace = isReplace;
            bool addStack = false;
            bool resetTime = false;
            switch(basic->GetApplicationLogic())
            {
            case 0:
                // Add always, replace only if higher/longer or zero (ex: sleep)
                doReplace = isReplace && ((existing->GetStack() < stack) || !stack);
                break;
            case 1:
                // Always set/add stack, reset time only if stack
                // represents time (misc)
                if(isReplace)
                {
                    existing->SetStack(stack);
                    if(basic->GetStackType() == 1)
                    {
                        resetTime = true;
                    }
                }
                else
                {
                    addStack = true;
                }
                break;
            case 2:
                // Always reset time, add old stack on add (ex: -kajas)
                addStack = !isReplace;
                resetTime = true;
                break;
            case 3:
                // Always reapply time and stack (ex: -karns)
                doReplace = resetTime = true;
                break;
            default:
                continue;
                break;
            }

            if(doReplace)
            {
                existing->SetStack(stack);
            }
            else if(addStack && existing->GetStack() < maxStack)
            {
                stack = (uint8_t)(stack + existing->GetStack());
                existing->SetStack(maxStack < stack ? maxStack : stack);
            }

            if(resetTime)
            {
                existing->SetExpiration(0);
            }

            if(existing->GetStack() > 0)
            {
                effect = existing;
            }
            else
            {
                removeEffect = existing;
            }

            add = false;
        }
        else
        {
            // Effect does not exist already, determine if it can be added
            auto common = def->GetCommon();

            // Map out existing effects and info to check for inverse cancellation
            bool canCancel = common->CorrectTblCount() > 0;
            libcomp::EnumMap<CorrectTbl, std::unordered_map<bool, uint8_t>> cancelMap;
            for(auto c : common->GetCorrectTbl())
            {
                if(c->GetValue() == 0 || c->GetType() == 1)
                {
                    canCancel = false;
                    cancelMap.clear();
                }
                else
                {
                    bool positive = c->GetValue() > 0;
                    std::unordered_map<bool, uint8_t>& m = cancelMap[c->GetID()];
                    m[positive] = (uint8_t)((m.find(positive) != m.end()
                        ? m[positive] : 0) + 1);
                }
            }

            std::set<uint32_t> inverseEffects;
            for(auto pair : mStatusEffects)
            {
                auto exDef = definitionManager->GetStatusData(pair.first);
                auto exBasic = exDef->GetBasic();
                if(exBasic->GetGroupID() == basic->GetGroupID())
                {
                    if(basic->GetGroupRank() >= exBasic->GetGroupRank())
                    {
                        // Replace the lower ranked effect in the same group
                        removeEffect = pair.second;
                    }
                    else
                    {
                        // Higher rank exists, do not add or replace
                        add = false;
                    }

                    canCancel = false;
                    break;
                }

                // Check if the existing effect is an inverse that should be cancelled instead.
                // For an effect to be inverse, both effects must have correct table entries
                // which are all numeric, none can have a zero value and the number of positive
                // values on one for each entry ID must match the number of negative values on
                // the other and vice-versa. The actual values themselves do NOT need to inversely
                // match.
                auto exCommon = exDef->GetCommon();
                if(canCancel && common->CorrectTblCount() == exCommon->CorrectTblCount())
                {
                    bool exCancel = true;
                    libcomp::EnumMap<CorrectTbl, std::unordered_map<bool, uint8_t>> exCancelMap;
                    for(auto c : exCommon->GetCorrectTbl())
                    {
                        if(c->GetValue() == 0 || c->GetType() == 1)
                        {
                            exCancel = false;
                            break;
                        }
                        else
                        {
                            bool positive = c->GetValue() > 0;
                            std::unordered_map<bool, uint8_t>& m = exCancelMap[c->GetID()];
                            m[positive] = (uint8_t)((m.find(positive) != m.end()
                                ? m[positive] : 0) + 1);
                        }
                    }

                    if(exCancel && cancelMap.size() == exCancelMap.size())
                    {
                        for(auto cPair : cancelMap)
                        {
                            if(exCancelMap.find(cPair.first) == exCancelMap.end())
                            {
                                exCancel = false;
                                break;
                            }

                            auto otherMap = exCancelMap[cPair.first];
                            for(auto mPair : cPair.second)
                            {
                                if(otherMap.find(!mPair.first) == otherMap.end() ||
                                    otherMap[!mPair.first] != mPair.second)
                                {
                                    exCancel = false;
                                    break;
                                }
                            }
                        }

                        // Correct table values are inversed, existing effect
                        // can be cancelled
                        if(exCancel)
                        {
                            inverseEffects.insert(pair.first);
                        }
                    }
                }
            }

            if(canCancel && inverseEffects.size() > 0)
            {
                // Should never be more than one but in case there is, the
                // lowest ID will be cancelled
                auto exEffect = mStatusEffects[*inverseEffects.begin()];
                if(exEffect->GetStack() == stack)
                {
                    // Cancel the old one, don't add anything
                    add = false;

                    removeEffect = exEffect;
                }
                else if(exEffect->GetStack() < stack)
                {
                    // Cancel the old one, add the new one with a lower stack
                    stack = (uint8_t)(stack - exEffect->GetStack());
                    add = true;

                    removeEffect = exEffect;
                }
                else
                {
                    // Reduce the stack of the existing one;
                    exEffect->SetStack((uint8_t)(exEffect->GetStack() - stack));
                    add = false;

                    // Application logic 2 effects have their expirations reset
                    // any time they are re-applied
                    auto exDef = definitionManager->GetStatusData(
                        exEffect->GetEffect());
                    if(exDef->GetBasic()->GetApplicationLogic() == 2)
                    {
                        exEffect->SetExpiration(0);
                    }

                    effect = exEffect;
                }
            }
        }
    
        if(add)
        {
            // Effect not set yet, build it now
            effect = libcomp::PersistentObject::New<objects::StatusEffect>(true);
            effect->SetEntity(GetEntityUUID());
            effect->SetEffect(effectType);
            effect->SetStack(stack);
        }

        // Perform insert or edit modifications
        if(effect)
        {
            if(effect->GetExpiration() == 0)
            {
                //Set the expiration
                uint32_t expiration = 0;
                bool absoluteTime = false;
                switch(cancel->GetDurationType())
                {
                case objects::MiCancelData::DurationType_t::MS:
                case objects::MiCancelData::DurationType_t::MS_SET:
                    // Milliseconds stored as relative countdown
                    expiration = cancel->GetDuration();
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
                default:
                    // None or invalid, nothing to do
                    break;
                }

                if(basic->GetStackType() == 1)
                {
                    // Stack scales time
                    expiration = expiration * effect->GetStack();
                }

                if(absoluteTime)
                {
                    expiration = (uint32_t)(now + expiration);
                }

                effect->SetExpiration(expiration);
            }
        }
    
        if(removeEffect)
        {
            removes.insert(removeEffect->GetEffect());
            mStatusEffects.erase(removeEffect->GetEffect());
            mTimeDamageEffects.erase(removeEffect->GetEffect());
            if(mEffectsActive && queueChanges)
            {
                // Non-system time 3 indicates removes
                mNextEffectTimes[3].insert(removeEffect->GetEffect());
            }
        }

        if(effect)
        {
            mStatusEffects[effect->GetEffect()] = effect;
            if(mEffectsActive)
            {
                if(add)
                {
                    ActivateStatusEffect(effect, definitionManager, now);
                }

                if(queueChanges)
                {
                    // Add non-system time for add or update
                    mNextEffectTimes[add ? 1 : 2].insert(effect->GetEffect());
                }
            }
        }
    }

    if(mEffectsActive)
    {
        RegisterNextEffectTime();
    }

    return removes;
}

void ActiveEntityState::ExpireStatusEffects(const std::set<uint32_t>& effectTypes)
{
    bool expired = false;
    std::lock_guard<std::mutex> lock(mLock);
    for(uint32_t effectType : effectTypes)
    {
        auto it = mStatusEffects.find(effectType);
        if(it != mStatusEffects.end())
        {
            mStatusEffects.erase(effectType);
            mNRAShields.erase(effectType);
            mTimeDamageEffects.erase(effectType);
            for(auto cPair : mCancelConditions)
            {
                cPair.second.erase(effectType);
            }

            if(mEffectsActive)
            {
                // Non-system time 3 indicates removes
                SetNextEffectTime(effectType, 0);
                mNextEffectTimes[3].insert(effectType);
                expired = true;
            }
        }
    }

    if(expired)
    {
        RegisterNextEffectTime();
    }
}

void ActiveEntityState::CancelStatusEffects(uint8_t cancelFlags)
{
    std::set<uint32_t> cancelled;
    if(mCancelConditions.size() > 0)
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto cPair : mCancelConditions)
        {
            if(cancelFlags & cPair.first)
            {
                for(uint32_t effectType : cPair.second)
                {
                    cancelled.insert(effectType);
                }
            }
        }
    }

    if(cancelled.size() > 0)
    {
        ExpireStatusEffects(cancelled);
    }
}

void ActiveEntityState::SetStatusEffectsActive(bool activate,
    libcomp::DefinitionManager* definitionManager, uint32_t now)
{
    if(now == 0)
    {
        now = (uint32_t)std::time(0);
    }

    // Already set
    if(mEffectsActive == activate)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mLock);
    mEffectsActive = activate;
    if(activate)
    {
        // Set regen
        SetNextEffectTime(0, now + 10);

        // Set status effect expirations
        for(auto pair : mStatusEffects)
        {
            ActivateStatusEffect(pair.second, definitionManager, now);
        }

        RegisterNextEffectTime();
    }
    else
    {
        mTimeDamageEffects.clear();
        mCancelConditions.clear();
        
        if(mCurrentZone)
        {
            mCurrentZone->SetNextStatusEffectTime(0, GetEntityID());
        }

        for(auto pair : mNextEffectTimes)
        {
            // Skip non-system times
            if(pair.first <= 3) continue;

            for(auto effectType : pair.second)
            {
                auto it = mStatusEffects.find(effectType);
                if(it == mStatusEffects.end()) continue;

                uint32_t exp = GetCurrentExpiration(it->second, definitionManager,
                    pair.first, now);
                it->second->SetExpiration(exp);
            }
        }
    }
}

bool ActiveEntityState::PopEffectTicks(libcomp::DefinitionManager* definitionManager,
    uint32_t time, int32_t& hpTDamage, int32_t& mpTDamage, std::set<uint32_t>& added,
    std::set<uint32_t>& updated, std::set<uint32_t>& removed)
{
    hpTDamage = 0;
    mpTDamage = 0;
    added.clear();
    updated.clear();
    removed.clear();

    std::lock_guard<std::mutex> lock(mLock);
    bool found = false;
    bool reregister = false;
    do
    {
        std::set<uint32_t> passed;
        std::unordered_map<uint32_t, uint32_t> next;
        for(auto pair : mNextEffectTimes)
        {
            if(pair.first > time) break;

            passed.insert(pair.first);
            
            // Check hardcoded added, updated, removed first
            if(pair.first == 1)
            {
                added = pair.second;
                continue;
            }
            else if(pair.first == 2)
            {
                updated = pair.second;
                continue;
            }
            else if(pair.first == 3)
            {
                removed = pair.second;
                continue;
            }

            if(pair.second.find(0) != pair.second.end())
            {
                // Adjust T-Damage if the entity is not dead
                if(mAlive)
                {
                    hpTDamage = (int32_t)(hpTDamage -
                        GetCorrectValue(CorrectTbl::HP_REGEN));
                    mpTDamage = (int32_t)(mpTDamage -
                        GetCorrectValue(CorrectTbl::MP_REGEN));

                    // Apply T-damage
                    for(auto effectType : mTimeDamageEffects)
                    {
                        auto se = definitionManager->GetStatusData(effectType);
                        auto damage = se->GetEffect()->GetDamage();

                        hpTDamage = (int32_t)(hpTDamage + damage->GetHPDamage());
                        mpTDamage = (int32_t)(mpTDamage + damage->GetMPDamage());
                    }
                }

                // T-Damage applies is every 10 seconds
                next[0] = pair.first + 10;

                pair.second.erase(0);
            }

            for(auto effectType : pair.second)
            {
                // Effect has ended
                auto effect = mStatusEffects[effectType];
                mStatusEffects.erase(effectType);
                mTimeDamageEffects.erase(effectType);
                removed.insert(effectType);

                for(auto cPair : mCancelConditions)
                {
                    cPair.second.erase(effectType);
                }
            }
        }

        for(auto t : passed)
        {
            mNextEffectTimes.erase(t);
        }

        for(auto pair : next)
        {
            SetNextEffectTime(pair.first, pair.second);
        }

        found = passed.size() > 0;
        reregister |= found;
    } while(found);

    if(reregister)
    {
        RegisterNextEffectTime();
    }

    // If anything was popped off the map, update the entity
    if(hpTDamage || mpTDamage || added.size() > 0 ||
        updated.size() > 0 || removed.size() > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::list<std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t>>
    ActiveEntityState::GetCurrentStatusEffectStates(
    libcomp::DefinitionManager* definitionManager, uint32_t now)
{
    if(now == 0)
    {
        now = (uint32_t)std::time(0);
    }

    std::list<std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t>> result;
    std::lock_guard<std::mutex> lock(mLock);

    if(!mEffectsActive)
    {
        // Just pull the stored values
        for(auto pair : mStatusEffects)
        {
            std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t> p(pair.second,
                pair.second->GetExpiration());
            result.push_back(p);
        }

        return result;
    }

    // Pull the times and transform the stored expiration
    std::unordered_map<uint32_t, uint32_t> nextTimes;
    for(auto pair : mNextEffectTimes)
    {
        // Skip non-system times
        if(pair.first <= 3) continue;

        for(auto effectType : pair.second)
        {
            nextTimes[effectType] = pair.first;
        }
    }
    
    for(auto pair : mStatusEffects)
    {
        uint32_t exp = pair.second->GetExpiration();

        auto it = nextTimes.find(pair.first);
        if(it != nextTimes.end())
        {
            exp = GetCurrentExpiration(pair.second, definitionManager,
                it->second, now);
        }

        std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t> p(pair.second,
            exp);
        result.push_back(p);
    }

    return result;
}

std::set<int32_t> ActiveEntityState::GetOpponentIDs() const
{
    return mOpponentIDs;
}

bool ActiveEntityState::HasOpponent(int32_t opponentID)
{
    std::lock_guard<std::mutex> lock(mLock);
    return opponentID == 0 ? mOpponentIDs.size() > 0
        : mOpponentIDs.find(opponentID) != mOpponentIDs.end();
}

size_t ActiveEntityState::AddRemoveOpponent(bool add, int32_t opponentID)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(add)
    {
        mOpponentIDs.insert(opponentID);
    }
    else
    {
        mOpponentIDs.erase(opponentID);
    }

    return mOpponentIDs.size();
}

int16_t ActiveEntityState::GetNRAChance(uint8_t nraIdx, CorrectTbl type,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    if(calcState == nullptr)
    {
        calcState = GetCalculatedState();
    }

    switch(nraIdx)
    {
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

std::set<uint8_t> ActiveEntityState::PopNRAShields(const std::list<
    CorrectTbl>& types)
{
    std::set<uint8_t> result;
    std::set<uint32_t> adjustEffects;
    std::set<uint32_t> expireEffects;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto pair : mNRAShields)
        {
            for(auto type : types)
            {
                if(pair.second.find(type) != pair.second.end())
                {
                    for(auto idx : pair.second[type])
                    {
                        result.insert(idx);
                    }
                    adjustEffects.insert(pair.first);
                }
            }
        }

        for(uint32_t effectID : adjustEffects)
        {
            auto effect = mStatusEffects[effectID];

            uint8_t newStack = (uint8_t)(effect->GetStack() - 1);
            effect->SetStack(newStack);
            if(newStack == 0)
            {
                expireEffects.insert(effectID);
            }
        }
    }

    if(expireEffects.size() > 0)
    {
        ExpireStatusEffects(expireEffects);
    }

    return result;
}

uint8_t ActiveEntityState::GetNextActivatedAbilityID()
{
    std::lock_guard<std::mutex> lock(mLock);
    uint8_t next = mNextActivatedAbilityID;

    mNextActivatedAbilityID = (uint8_t)((mNextActivatedAbilityID + 1) % 128);
    if(mNextActivatedAbilityID == 0)
    {
        mNextActivatedAbilityID = 1;
    }

    return next;
}

void ActiveEntityState::SetStatusEffects(
    const std::list<libcomp::ObjectReference<objects::StatusEffect>>& effects)
{
    std::list<std::shared_ptr<objects::StatusEffect>> l;
    for(auto e : effects)
    {
        l.push_back(e.Get());
    }
    SetStatusEffects(l);
}

void ActiveEntityState::ActivateStatusEffect(
    const std::shared_ptr<objects::StatusEffect>& effect,
    libcomp::DefinitionManager* definitionManager, uint32_t now)
{
    auto effectType = effect->GetEffect();

    auto se = definitionManager->GetStatusData(effectType);
    auto cancel = se->GetCancel();
    switch(cancel->GetDurationType())
    {
        case objects::MiCancelData::DurationType_t::MS:
        case objects::MiCancelData::DurationType_t::MS_SET:
            {
                // Force next tick time to duration
                uint32_t time = (uint32_t)(now + (effect->GetExpiration() * 0.001));
                mNextEffectTimes[time].insert(effectType);
            }
            break;
        default:
            mNextEffectTimes[effect->GetExpiration()].insert(effectType);
            break;
    }

    // Mark the cancel conditions
    for(uint16_t x = 0x0001; x < 0x00100;)
    {
        uint8_t x8 = (uint8_t)x;
        if(cancel->GetCancelTypes() & x8)
        {
            mCancelConditions[x8].insert(effectType);
        }

        x = (uint16_t)(x << 1);
    }

    // Add to timed damage effect set if T-Damage is specified
    auto basic = se->GetBasic();
    auto damage = se->GetEffect()->GetDamage();
    if(damage->GetHPDamage() || damage->GetMPDamage())
    {
        // Ignore if the damage applies as part of the skill only
        if(!(basic->GetStackType() == 1 && basic->GetApplicationLogic() == 0))
        {
            mTimeDamageEffects.insert(effectType);
        }
    }

    // If the stack type is a counter and the effect is re-applied each time
    // check for NRA shields
    if(basic->GetStackType() == 0 && basic->GetApplicationLogic() == 3)
    {
        auto common = se->GetCommon();
        for(auto ct : common->GetCorrectTbl())
        {
            if((uint8_t)ct->GetID() >= (uint8_t)CorrectTbl::NRA_WEAPON &&
                (uint8_t)ct->GetID() <= (uint8_t)CorrectTbl::NRA_MAGIC)
            {
                mNRAShields[effectType][ct->GetID()].insert(
                    (uint8_t)ct->GetValue());
            }
        }
    }
}

void ActiveEntityState::SetNextEffectTime(uint32_t effectType, uint32_t time)
{
    for(auto pair : mNextEffectTimes)
    {
        // Skip non-system times
        if(pair.first <= 3) continue;

        if(pair.second.find(effectType) != pair.second.end())
        {
            if(time == 0)
            {
                pair.second.erase(effectType);
                if(pair.second.size() == 0)
                {
                    mNextEffectTimes.erase(pair.first);
                }
            }

            return;
        }
    }

    if(time != 0)
    {
        mNextEffectTimes[time].insert(effectType);
    }
}

void ActiveEntityState::RegisterNextEffectTime()
{
    if(mCurrentZone && mEffectsActive)
    {
        mCurrentZone->SetNextStatusEffectTime(mNextEffectTimes.size() > 0
            ? mNextEffectTimes.begin()->first : 0, GetEntityID());
    }
}

uint32_t ActiveEntityState::GetCurrentExpiration(
    const std::shared_ptr<objects::StatusEffect>& effect,
    libcomp::DefinitionManager* definitionManager, uint32_t nextTime,
    uint32_t now)
{
    uint32_t exp = effect->GetExpiration();

    if(exp > 0)
    {
        auto se = definitionManager->GetStatusData(effect->GetEffect());
        auto cancel = se->GetCancel();
        switch(cancel->GetDurationType())
        {
        case objects::MiCancelData::DurationType_t::MS:
        case objects::MiCancelData::DurationType_t::MS_SET:
            {
                // Convert back to milliseconds
                exp = (uint32_t)((nextTime - now) * 1000);
                if(effect->GetExpiration() < exp)
                {
                    exp = 0;
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

// "Abstract implementations" required for Sqrat usage
uint8_t ActiveEntityState::RecalculateStats(libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    (void)definitionManager;
    (void)calcState;
    return 0;
}

std::set<uint32_t> ActiveEntityState::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei)
{
    (void)definitionManager;
    (void)includeTokusei;
    return std::set<uint32_t>();
}

uint8_t ActiveEntityState::GetLNCType(libcomp::DefinitionManager* definitionManager)
{
    (void)definitionManager;
    return 0;
}

std::shared_ptr<objects::EntityStats> ActiveEntityState::GetCoreStats()
{
    return nullptr;
}

bool ActiveEntityState::Ready()
{
    return false;
}

namespace channel
{

template<>
ActiveEntityStateImp<objects::Character>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::CHARACTER);
    SetFaction(objects::ActiveEntityStateObject::Faction_t::PLAYER);
}

template<>
ActiveEntityStateImp<objects::Demon>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::PARTNER_DEMON);
    SetFaction(objects::ActiveEntityStateObject::Faction_t::PLAYER);
}

template<>
ActiveEntityStateImp<objects::Enemy>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::ENEMY);
    SetFaction(objects::ActiveEntityStateObject::Faction_t::ENEMY);
}

template<>
void ActiveEntityStateImp<objects::Character>::SetEntity(
    const std::shared_ptr<objects::Character>& entity)
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEntity = entity;
    }

    std::list<libcomp::ObjectReference<objects::StatusEffect>> effects;
    if(entity)
    {
        // Character should always be set but check just in case
        effects = entity->GetStatusEffects();
        mAlive = entity->GetCoreStats()->GetHP() > 0;
    }

    SetStatusEffects(effects);

    // Reset knockback and let refresh correct
    SetKnockbackResist(0);
    mInitialCalc = false;
}

template<>
void ActiveEntityStateImp<objects::Demon>::SetEntity(
    const std::shared_ptr<objects::Demon>& entity)
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEntity = entity;
    }

    std::list<libcomp::ObjectReference<objects::StatusEffect>> effects;
    if(entity)
    {
        effects = entity->GetStatusEffects();
        mAlive = entity->GetCoreStats()->GetHP() > 0;
    }

    SetStatusEffects(effects);

    auto calcState = GetCalculatedState();
    calcState->ClearActiveTokuseiTriggers();
    calcState->ClearEffectiveTokusei();

    // Reset knockback and let refresh correct
    SetKnockbackResist(0);
    mInitialCalc = false;
}

template<>
void ActiveEntityStateImp<objects::Enemy>::SetEntity(
    const std::shared_ptr<objects::Enemy>& entity)
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEntity = entity;
    }
    
    if(entity)
    {
        mAlive = entity->GetCoreStats()->GetHP() > 0;
    }

    // Reset knockback and let refresh correct
    SetKnockbackResist(0);
    mInitialCalc = false;
}

template<>
const libobjgen::UUID ActiveEntityStateImp<objects::Character>::GetEntityUUID()
{
    return mEntity->GetUUID();
}

template<>
const libobjgen::UUID ActiveEntityStateImp<objects::Demon>::GetEntityUUID()
{
    return mEntity ? mEntity->GetUUID() : NULLUUID;
}

template<>
const libobjgen::UUID ActiveEntityStateImp<objects::Enemy>::GetEntityUUID()
{
    return NULLUUID;
}

template<>
std::set<uint32_t> ActiveEntityStateImp<objects::Character>::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei)
{
    std::set<uint32_t> skillIDs;
    
    if(mEntity)
    {
        skillIDs = mEntity->GetLearnedSkills();

        auto clan = mEntity->GetClan().Get();
        if(clan)
        {
            int8_t clanLevel = clan->GetLevel();
            for(int8_t i = 0; i < clanLevel; i++)
            {
                for(uint32_t clanSkillID : SVR_CONST.CLAN_LEVEL_SKILLS[(size_t)i])
                {
                    skillIDs.insert(clanSkillID);
                }
            }
        }

        if(includeTokusei)
        {
            for(uint32_t skillID : GetEffectiveTokuseiSkills(definitionManager))
            {
                skillIDs.insert(skillID);
            }
        }
    }

    return skillIDs;
}

template<>
std::set<uint32_t> ActiveEntityStateImp<objects::Demon>::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei)
{
    std::set<uint32_t> skillIDs;

    if(mEntity)
    {
        for(uint32_t skillID : mEntity->GetLearnedSkills())
        {
            if(skillID)
            {
                skillIDs.insert(skillID);
            }
        }

        auto demonData = definitionManager->GetDevilData(mEntity->GetType());

        auto growth = demonData->GetGrowth();
        for(uint32_t traitID : growth->GetTraits())
        {
            if(traitID)
            {
                skillIDs.insert(traitID);
            }
        }

        if(includeTokusei)
        {
            for(uint32_t skillID : GetEffectiveTokuseiSkills(definitionManager))
            {
                skillIDs.insert(skillID);
            }
        }
    }

    return skillIDs;
}

template<>
std::set<uint32_t> ActiveEntityStateImp<objects::Enemy>::GetAllSkills(
    libcomp::DefinitionManager* definitionManager, bool includeTokusei)
{
    std::set<uint32_t> skillIDs;

    if(mEntity)
    {
        auto demonData = definitionManager->GetDevilData(mEntity->GetType());

        auto previousSkills = GetCurrentSkills();
        ClearCurrentSkills();

        auto growth = demonData->GetGrowth();
        for(auto skillSet : { growth->GetSkills(),
            growth->GetEnemyOnlySkills() })
        {
            for(uint32_t skillID : skillSet)
            {
                if(skillID)
                {
                    skillIDs.insert(skillID);
                }
            }
        }

        for(uint32_t traitID : growth->GetTraits())
        {
            if(traitID)
            {
                skillIDs.insert(traitID);
            }
        }

        if(includeTokusei)
        {
            for(uint32_t skillID : GetEffectiveTokuseiSkills(definitionManager))
            {
                skillIDs.insert(skillID);
            }
        }
    }

    return skillIDs;
}

template<>
uint8_t ActiveEntityStateImp<objects::Character>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    uint8_t result = 0;

    std::lock_guard<std::mutex> lock(mLock);

    auto c = GetEntity();
    auto cs = c->GetCoreStats().Get();

    bool selfState = calcState == nullptr;
    if(selfState)
    {
        calcState = GetCalculatedState();

        // Calculate current skills, only matters if calculating
        // for the default entity state
        auto previousSkills = GetCurrentSkills();
        SetCurrentSkills(GetAllSkills(definitionManager, true));

        bool skillsChanged = previousSkills.size() != CurrentSkillsCount();
        if(!skillsChanged)
        {
            for(uint32_t skillID : previousSkills)
            {
                if(!CurrentSkillsContains(skillID))
                {
                    skillsChanged = true;
                    break;
                }
            }
        }
        result = skillsChanged ? ENTITY_CALC_SKILL : 0x00;

        // Remove any switch skills no longer available
        RemoveInactiveSwitchSkills();
    }

    auto stats = CharacterManager::GetCharacterBaseStatMap(cs);
    if(selfState && !mInitialCalc)
    {
        SetKnockbackResist((float)stats[CorrectTbl::KNOCKBACK_RESIST]);
        mInitialCalc = true;
    }

    // Calculate based on adjustments
    std::list<std::shared_ptr<objects::MiCorrectTbl>> correctTbls;
    std::list<std::shared_ptr<objects::MiCorrectTbl>> nraTbls;
    for(auto equip : c->GetEquippedItems())
    {
        if(!equip.IsNull())
        {
            auto itemData = definitionManager->GetItemData(equip->GetType());
            for(auto ct : itemData->GetCommon()->GetCorrectTbl())
            {
                if((uint8_t)ct->GetID() >= (uint8_t)CorrectTbl::NRA_WEAPON &&
                    (uint8_t)ct->GetID() <= (uint8_t)CorrectTbl::NRA_MAGIC)
                {
                    nraTbls.push_back(ct);
                }
                else
                {
                    correctTbls.push_back(ct);
                }
            }
        }
    }

    GetAdditionalCorrectTbls(definitionManager, calcState, correctTbls);

    UpdateNRAChances(stats, calcState, nraTbls);
    AdjustStats(correctTbls, stats, calcState, true);
    BaseStatsCalculated(definitionManager, calcState, stats, correctTbls);

    CharacterManager::CalculateDependentStats(stats, cs->GetLevel(), false);

    AdjustStats(correctTbls, stats, calcState, false);

    if(selfState)
    {
        return result | CompareAndResetStats(stats);
    }
    else
    {
        for(auto statPair : stats)
        {
            calcState->SetCorrectTbl((size_t)statPair.first, statPair.second);
        }

        return result;
    }
}

template<>
uint8_t ActiveEntityStateImp<objects::Demon>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    if(!mEntity)
    {
        return true;
    }

    if(calcState == nullptr)
    {
        // Calculating default entity state
        calcState = GetCalculatedState();

        auto previousSkills = GetCurrentSkills();
        SetCurrentSkills(GetAllSkills(definitionManager, true));

        bool skillsChanged = previousSkills.size() != CurrentSkillsCount();
        if(!skillsChanged)
        {
            for(uint32_t skillID : previousSkills)
            {
                if(!CurrentSkillsContains(skillID))
                {
                    skillsChanged = true;
                    break;
                }
            }
        }

        if(skillsChanged && mAIState)
        {
            mAIState->ResetSkillsMapped();
        }
    }

    return RecalculateDemonStats(definitionManager, calcState, mEntity->GetType());
}

template<>
uint8_t ActiveEntityStateImp<objects::Enemy>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    if(!mEntity)
    {
        return true;
    }
    
    if(calcState == nullptr)
    {
        // Calculating default entity state
        calcState = GetCalculatedState();

        if(!mInitialCalc)
        {
            auto previousSkills = GetCurrentSkills();
            SetCurrentSkills(GetAllSkills(definitionManager, true));

            bool skillsChanged = previousSkills.size() != CurrentSkillsCount();
            if(!skillsChanged)
            {
                for(uint32_t skillID : previousSkills)
                {
                    if(!CurrentSkillsContains(skillID))
                    {
                        skillsChanged = true;
                        break;
                    }
                }
            }

            if(skillsChanged && mAIState)
            {
                mAIState->ResetSkillsMapped();
            }
        }
    }

    return RecalculateDemonStats(definitionManager, calcState, mEntity->GetType());
}

template<>
uint8_t ActiveEntityStateImp<objects::Character>::GetLNCType(
    libcomp::DefinitionManager* definitionManager)
{
    (void)definitionManager;

    return CalculateLNCType(mEntity->GetLNC());
}

template<>
uint8_t ActiveEntityStateImp<objects::Demon>::GetLNCType(
    libcomp::DefinitionManager* definitionManager)
{
    int16_t lncPoints = 0;
    if(mEntity)
    {
        auto demonData = definitionManager->GetDevilData(
            mEntity->GetType());
        lncPoints = demonData->GetBasic()->GetLNC();
    }

    return CalculateLNCType(lncPoints);
}

template<>
uint8_t ActiveEntityStateImp<objects::Enemy>::GetLNCType(
    libcomp::DefinitionManager* definitionManager)
{
    int16_t lncPoints = 0;
    if(mEntity)
    {
        auto demonData = definitionManager->GetDevilData(
            mEntity->GetType());
        lncPoints = demonData->GetBasic()->GetLNC();
    }

    return CalculateLNCType(lncPoints);
}

}

const std::set<CorrectTbl> BASE_STATS =
    {
        CorrectTbl::STR,
        CorrectTbl::MAGIC,
        CorrectTbl::VIT,
        CorrectTbl::INT,
        CorrectTbl::SPEED,
        CorrectTbl::LUCK
    };

void ActiveEntityState::AdjustStats(
    const std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments,
    libcomp::EnumMap<CorrectTbl, int16_t>& stats,
    std::shared_ptr<objects::CalculatedEntityState> calcState, bool baseMode)
{
    std::set<CorrectTbl> removed;
    for(auto ct : adjustments)
    {
        auto tblID = ct->GetID();

        // Only adjust base or calculated stats depending on mode
        if(baseMode != (BASE_STATS.find(tblID) != BASE_STATS.end())) continue;

        // If a value is reduced to 0%, leave it
        if(removed.find(tblID) != removed.end()) continue;

        uint8_t effectiveType = ct->GetType();
        int32_t effectiveValue = (int32_t)ct->GetValue();
        if(effectiveType >= 100)
        {
            // This is actually a TokuseiCorrectTbl, check for attributes
            // like multipliers etc and adjust the value accordingly
            effectiveType = (uint8_t)(effectiveType - 100);

            auto tct = std::dynamic_pointer_cast<objects::TokuseiCorrectTbl>(ct);
            if(tct)
            {
                effectiveValue = (int32_t)TokuseiManager::CalculateAttributeValue(
                    this, tct->GetValue(), effectiveValue, tct->GetAttributes());
            }
        }

        if((uint8_t)tblID >= (uint8_t)CorrectTbl::NRA_WEAPON &&
            (uint8_t)tblID <= (uint8_t)CorrectTbl::NRA_MAGIC)
        {
            // NRA is calculated differently from everything else
            if(effectiveType)
            {
                // For type 0, the NRA value becomes 100% and CANNOT be reduced.
                switch(effectiveValue)
                {
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
            }
            else
            {
                // For other types, reduce the value by 2 to get the NRA index
                // and add the value supplied.
                switch(effectiveType)
                {
                case (NRA_NULL + 2):
                    calcState->SetNullChances((int16_t)tblID, (int16_t)(
                        calcState->GetNullChances((int16_t)tblID) + effectiveValue));
                    break;
                case (NRA_REFLECT + 2):
                    calcState->SetReflectChances((int16_t)tblID, (int16_t)(
                        calcState->GetReflectChances((int16_t)tblID) + effectiveValue));
                    break;
                case (NRA_ABSORB + 2):
                    calcState->SetAbsorbChances((int16_t)tblID, (int16_t)(
                        calcState->GetAbsorbChances((int16_t)tblID) + effectiveValue));
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            bool allowNegate = false;

            int16_t adjusted = stats[tblID];
            switch(effectiveType)
            {
            case 1:
                // Percentage sets can either be an immutable set to zero
                // or an increase/decrease by a set amount
                if(effectiveValue == 0)
                {
                    removed.insert(tblID);
                    adjusted = 0;
                    allowNegate = true;
                }
                else
                {
                    adjusted = (int16_t)(adjusted +
                        (int16_t)(adjusted * (effectiveValue * 0.01)));
                    allowNegate = ct->GetValue() < 0;
                }
                break;
            case 0:
                adjusted = (int16_t)(adjusted + effectiveValue);
                allowNegate = (effectiveValue < 0) != (stats[tblID] < 0);
                break;
            default:
                break;
            }

            // Prevent overflow
            if(!allowNegate && (stats[tblID] < 0) != (adjusted < 0))
            {
                if(adjusted >= 0)
                {
                    // Negative overflow
                    stats[tblID] = std::numeric_limits<int16_t>::min();
                }
                else
                {
                    // Positive overflow
                    stats[tblID] = std::numeric_limits<int16_t>::max();
                }
            }
            else
            {
                stats[tblID] = adjusted;
            }
        }
    }

    CharacterManager::AdjustStatBounds(stats);
}

void ActiveEntityState::BaseStatsCalculated(libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    libcomp::EnumMap<CorrectTbl, int16_t>& stats,
    std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments)
{
    (void)definitionManager;
    (void)stats;
    (void)adjustments;

    calcState->SetEffectiveTokuseiFinal(calcState->GetEffectiveTokusei());
    calcState->SetPendingSkillTokuseiFinal(calcState->GetPendingSkillTokusei());
}

void ActiveEntityState::UpdateNRAChances(libcomp::EnumMap<CorrectTbl, int16_t>& stats,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    const std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments)
{
    std::unordered_map<int16_t, int16_t> mNullMap;
    std::unordered_map<int16_t, int16_t> mReflectMap;
    std::unordered_map<int16_t, int16_t> mAbsorbMap;
    
    // Set from base
    for(uint8_t x = (uint8_t)CorrectTbl::NRA_WEAPON;
        x <= (uint8_t)CorrectTbl::NRA_MAGIC; x++)
    {
        CorrectTbl tblID = (CorrectTbl)x;
        int16_t val = stats[tblID];
        if(val > 0)
        {
            // Natural NRA is stored as NRA index in the 1s place and
            // perccentage of success as the rest
            double shift = (double)(val * 0.1);
            uint8_t nraIdx = (uint8_t)((shift - (double)floorl((double)val / 10.0)) * 10.0);
            val = (int16_t)floorl(val / 10);
            switch(nraIdx)
            {
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
    for(auto ct : adjustments)
    {
        int16_t tblID = (int16_t)ct->GetID();
        switch(ct->GetType())
        {
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
    std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments)
{
    // 1) Gather skill adjustments
    for(auto skillID : GetCurrentSkills())
    {
        auto skillData = definitionManager->GetSkillData(skillID);
        auto common = skillData->GetCommon();

        bool include = false;
        switch(common->GetCategory()->GetMainCategory())
        {
        case 0:
            // Passive
            include = true;
            break;
        case 2:
            // Switch
            include = ActiveSwitchSkillsContains(skillID);
            break;
        default:
            break;
        }

        if(include)
        {
            for(auto ct : common->GetCorrectTbl())
            {
                adjustments.push_back(ct);
            }
        }
    }

    // 2) Gather status effect adjustments
    for(auto ePair : GetStatusEffects())
    {
        auto statusData = definitionManager->GetStatusData(ePair.first);
        for(auto ct : statusData->GetCommon()->GetCorrectTbl())
        {
            uint8_t multiplier = (statusData->GetBasic()->GetStackType() == 2)
                ? ePair.second->GetStack() : 1;
            for(uint8_t i = 0; i < multiplier; i++)
            {
                adjustments.push_back(ct);
            }
        }
    }

    // 3) Gather tokusei effective adjustments
    for(auto tPair : calcState->GetEffectiveTokusei())
    {
        auto tokusei = definitionManager->GetTokuseiData(tPair.first);
        if(tokusei && tokusei->CorrectValuesCount() > 0)
        {
            // Add the entries once for each source applying them
            for(uint16_t i = 0; i < tPair.second; i++)
            {
                for(auto ct : tokusei->GetCorrectValues())
                {
                    adjustments.push_back(ct);
                }

                for(auto ct : tokusei->GetTokuseiCorrectValues())
                {
                    adjustments.push_back(ct);
                }
            }
        }
    }

    // Sort the adjustments, set to 0% first, non-zero percents next, numeric last
    adjustments.sort([](const std::shared_ptr<objects::MiCorrectTbl>& a,
        const std::shared_ptr<objects::MiCorrectTbl>& b)
    {
        return (a->GetType() == 1 || a->GetType() == 101) &&
            (a->GetValue() == 0 ||
            (b->GetType() != 1 && b->GetType() != 101));
    });
}

uint8_t ActiveEntityState::RecalculateDemonStats(
    libcomp::DefinitionManager* definitionManager,
    std::shared_ptr<objects::CalculatedEntityState> calcState, uint32_t demonID)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto demonData = definitionManager->GetDevilData(demonID);
    auto battleData = demonData->GetBattleData();
    auto cs = GetCoreStats();

    libcomp::EnumMap<CorrectTbl, int16_t> stats;
    for(size_t i = 0; i < 126; i++)
    {
        CorrectTbl tblID = (CorrectTbl)i;
        stats[tblID] = battleData->GetCorrect((size_t)i);
    }

    stats[CorrectTbl::STR] = cs->GetSTR();
    stats[CorrectTbl::MAGIC] = cs->GetMAGIC();
    stats[CorrectTbl::VIT] = cs->GetVIT();
    stats[CorrectTbl::INT] = cs->GetINTEL();
    stats[CorrectTbl::SPEED] = cs->GetSPEED();
    stats[CorrectTbl::LUCK] = cs->GetLUCK();

    bool selfState = calcState == GetCalculatedState();

    if(selfState && !mInitialCalc)
    {
        SetKnockbackResist((float)stats[CorrectTbl::KNOCKBACK_RESIST]);
        mInitialCalc = true;
    }

    std::list<std::shared_ptr<objects::MiCorrectTbl>> correctTbls;
    GetAdditionalCorrectTbls(definitionManager, calcState, correctTbls);

    UpdateNRAChances(stats, calcState);
    AdjustStats(correctTbls, stats, calcState, true);
    BaseStatsCalculated(definitionManager, calcState, stats, correctTbls);

    CharacterManager::CalculateDependentStats(stats, cs->GetLevel(), true);

    AdjustStats(correctTbls, stats, calcState, false);

    int32_t extraHP = 0;
    if(GetEntityType() == objects::EntityStateObject::EntityType_t::ENEMY)
    {
        extraHP = demonData->GetBattleData()->GetEnemyHP(0);
    }

    if(selfState)
    {
        return CompareAndResetStats(stats, extraHP);
    }
    else
    {
        for(auto statPair : stats)
        {
            calcState->SetCorrectTbl((size_t)statPair.first, statPair.second);
        }

        return 0;
    }
}

uint8_t ActiveEntityState::CalculateLNCType(int16_t lncPoints) const
{
    if(lncPoints >= 5000)
    {
        return LNC_CHAOS;
    }
    else if(lncPoints <= -5000)
    {
        return LNC_LAW;
    }
    else
    {
        return LNC_NEUTRAL;
    }
}

void ActiveEntityState::RemoveInactiveSwitchSkills()
{
    auto previousSwitchSkills = GetActiveSwitchSkills();
    for(uint32_t skillID : previousSwitchSkills)
    {
        if(!CurrentSkillsContains(skillID))
        {
            RemoveActiveSwitchSkills(skillID);
        }
    }
}

std::set<uint32_t> ActiveEntityState::GetEffectiveTokuseiSkills(
    libcomp::DefinitionManager* definitionManager)
{
    std::set<uint32_t> skillIDs;

    for(auto tPair : GetCalculatedState()->GetEffectiveTokusei())
    {
        auto tokusei = definitionManager->GetTokuseiData(tPair.first);
        if(tokusei)
        {
            for(auto aspect : tokusei->GetAspects())
            {
                if(aspect->GetType() == TokuseiAspectType::SKILL_ADD)
                {
                    skillIDs.insert((uint32_t)aspect->GetValue());
                }
            }
        }
    }

    return skillIDs;
}

const std::set<CorrectTbl> VISIBLE_STATS =
    {
        CorrectTbl::STR,
        CorrectTbl::MAGIC,
        CorrectTbl::VIT,
        CorrectTbl::INT,
        CorrectTbl::SPEED,
        CorrectTbl::LUCK,
        CorrectTbl::HP_MAX,
        CorrectTbl::MP_MAX,
        CorrectTbl::CLSR,
        CorrectTbl::LNGR,
        CorrectTbl::SPELL,
        CorrectTbl::SUPPORT,
        CorrectTbl::PDEF,
        CorrectTbl::MDEF
    };

uint8_t ActiveEntityState::CompareAndResetStats(libcomp::EnumMap<CorrectTbl, int16_t>& stats,
    int32_t extraHP)
{
    uint8_t result = 0;

    auto cs = GetCoreStats();
    int32_t hp = cs->GetHP();
    int32_t mp = cs->GetMP();
    int32_t newMaxHP = (int32_t)(extraHP + (int32_t)stats[CorrectTbl::HP_MAX]);
    int32_t newMaxMP = (int32_t)stats[CorrectTbl::MP_MAX];

    if(hp > newMaxHP)
    {
        hp = newMaxHP;
    }

    if(mp > newMaxMP)
    {
        mp = newMaxMP;
    }

    auto calcState = GetCalculatedState();
    for(auto statPair : stats)
    {
        calcState->SetCorrectTbl((size_t)statPair.first, statPair.second);
    }

    if(hp != cs->GetHP()
        || mp != cs->GetMP()
        || GetMaxHP() != newMaxHP
        || GetMaxMP() != newMaxMP)
    {
        result = ENTITY_CALC_STAT_WORLD |
            ENTITY_CALC_STAT_LOCAL;
    }
    else if(GetSTR() != stats[CorrectTbl::STR]
        || GetMAGIC() != stats[CorrectTbl::MAGIC]
        || GetVIT() != stats[CorrectTbl::VIT]
        || GetINTEL() != stats[CorrectTbl::INT]
        || GetSPEED() != stats[CorrectTbl::SPEED]
        || GetLUCK() != stats[CorrectTbl::LUCK]
        || GetCLSR() != stats[CorrectTbl::CLSR]
        || GetLNGR() != stats[CorrectTbl::LNGR]
        || GetSPELL() != stats[CorrectTbl::SPELL]
        || GetSUPPORT() != stats[CorrectTbl::SUPPORT]
        || GetPDEF() != stats[CorrectTbl::PDEF]
        || GetMDEF() != stats[CorrectTbl::MDEF])
    {
        result = ENTITY_CALC_STAT_LOCAL;
    }

    cs->SetHP(hp);
    cs->SetMP(mp);
    SetMaxHP(newMaxHP);
    SetMaxMP(newMaxMP);
    SetSTR(stats[CorrectTbl::STR]);
    SetMAGIC(stats[CorrectTbl::MAGIC]);
    SetVIT(stats[CorrectTbl::VIT]);
    SetINTEL(stats[CorrectTbl::INT]);
    SetSPEED(stats[CorrectTbl::SPEED]);
    SetLUCK(stats[CorrectTbl::LUCK]);
    SetCLSR(stats[CorrectTbl::CLSR]);
    SetLNGR(stats[CorrectTbl::LNGR]);
    SetSPELL(stats[CorrectTbl::SPELL]);
    SetSUPPORT(stats[CorrectTbl::SUPPORT]);
    SetPDEF(stats[CorrectTbl::PDEF]);
    SetMDEF(stats[CorrectTbl::MDEF]);

    return result;
}
