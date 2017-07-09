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
#include <ScriptEngine.h>

// objects Includes
#include <Character.h>
#include <Demon.h>
#include <Enemy.h>
#include <Item.h>
#include <MiCancelData.h>
#include <MiCorrectTbl.h>
#include <MiDevilBattleData.h>
#include <MiDevilData.h>
#include <MiDoTDamageData.h>
#include <MiEffectData.h>
#include <MiItemData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiStatusBasicData.h>
#include <MiStatusData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "Zone.h"

using namespace channel;

ActiveEntityState::ActiveEntityState() : mCurrentZone(0),
    mEffectsActive(false), mAlive(true)
{
}

ActiveEntityState::ActiveEntityState(const ActiveEntityState& other)
{
    (void)other;
}

const libobjgen::UUID ActiveEntityState::GetEntityUUID()
{
    return NULLUUID;
}

void ActiveEntityState::Move(float xPos, float yPos, uint64_t now)
{
    if(IsAlive())
    {
        SetOriginX(GetCurrentX());
        SetOriginY(GetCurrentY());
        SetOriginRotation(GetCurrentRotation());
        SetOriginTicks(now);

        SetDestinationX(xPos);
        SetDestinationY(yPos);
        SetDestinationTicks(now + 500000);  /// @todo: modify for speed
    }
}

void ActiveEntityState::Rotate(float rot, uint64_t now)
{
    if(IsAlive())
    {
        SetOriginX(GetCurrentX());
        SetOriginY(GetCurrentY());
        SetOriginRotation(GetCurrentRotation());
        SetOriginTicks(now);

        SetDestinationRotation(CorrectRotation(rot));
        SetDestinationTicks(now + 500000);  /// @todo: modify for speed
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

void ActiveEntityState::RefreshCurrentPosition(uint64_t now)
{
    if(now != mLastRefresh)
    {
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

        uint64_t destTicks = GetDestinationTicks();

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

Zone* ActiveEntityState::GetZone() const
{
    return mCurrentZone;
}

void ActiveEntityState::SetZone(Zone* zone, bool updatePrevious)
{
    if(updatePrevious && mCurrentZone)
    {
        mCurrentZone->SetNextStatusEffectTime(0, GetEntityID());
    }

    mCurrentZone = zone;

    RegisterNextEffectTime();
}

bool ActiveEntityState::SetHPMP(int16_t hp, int16_t mp, bool adjust,
    bool canOverflow)
{
    int16_t hpAdjusted, mpAdjusted;
    return SetHPMP(hp, mp, adjust, canOverflow, hpAdjusted, mpAdjusted);
}

bool ActiveEntityState::SetHPMP(int16_t hp, int16_t mp, bool adjust,
    bool canOverflow, int16_t& hpAdjusted, int16_t& mpAdjusted)
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
        hp = (int16_t)(cs->GetHP() + hp);
        mp = (int16_t)(cs->GetMP() + mp);

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
            hpAdjusted = (int16_t)(newHP - cs->GetHP());
        }

        cs->SetHP(newHP);
    }
    
    if(mp >= 0)
    {
        auto newMP = mp > maxMP ? maxMP : mp;
        result |= returnDamaged && newMP != cs->GetMP();
        
        if(!canOverflow)
        {
            mpAdjusted = (int16_t)(newMP - cs->GetMP());
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
                if(c->GetValue() == 0 ||
                    c->GetType() == objects::MiCorrectTbl::Type_t::PERCENT)
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
                        if(c->GetValue() == 0 ||
                            c->GetType() == objects::MiCorrectTbl::Type_t::PERCENT)
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
    std::lock_guard<std::mutex> lock(mLock);
    for(uint32_t effectType : effectTypes)
    {
        auto it = mStatusEffects.find(effectType);
        if(it != mStatusEffects.end())
        {
            mStatusEffects.erase(effectType);
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
            }
        }
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
                    hpTDamage = (int32_t)(hpTDamage - GetHPRegen());
                    mpTDamage = (int32_t)(mpTDamage - GetMPRegen());

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
    auto damage = se->GetEffect()->GetDamage();
    if(damage->GetHPDamage() || damage->GetMPDamage())
    {
        // Ignore if the damage applies as part of the skill only
        auto basic = se->GetBasic();
        if(!(basic->GetStackType() == 1 && basic->GetApplicationLogic() == 0))
        {
            mTimeDamageEffects.insert(effectType);
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

float ActiveEntityState::CorrectRotation(float rot) const
{
    if(rot > 3.16f)
    {
        return rot - 6.32f;
    }
    else if(rot < -3.16f)
    {
        return -rot - 3.16f;
    }

    return rot;
}

namespace channel
{

template<>
ActiveEntityStateImp<objects::Character>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::CHARACTER);
}

template<>
ActiveEntityStateImp<objects::Demon>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::PARTNER_DEMON);
}

template<>
ActiveEntityStateImp<objects::Enemy>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::ENEMY);
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
}

template<>
const libobjgen::UUID ActiveEntityStateImp<objects::Character>::GetEntityUUID()
{
    return mEntity->GetUUID();
}

template<>
const libobjgen::UUID ActiveEntityStateImp<objects::Demon>::GetEntityUUID()
{
    return mEntity->GetUUID();
}

template<>
const libobjgen::UUID ActiveEntityStateImp<objects::Enemy>::GetEntityUUID()
{
    return NULLUUID;
}

template<>
uint8_t ActiveEntityStateImp<objects::Character>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto c = GetEntity();
    auto cs = c->GetCoreStats().Get();

    auto stats = CharacterManager::GetCharacterBaseStatMap(cs);

    std::list<std::shared_ptr<objects::MiCorrectTbl>> correctTbls;
    for(auto equip : c->GetEquippedItems())
    {
        if(!equip.IsNull())
        {
            auto itemData = definitionManager->GetItemData(equip->GetType());
            for(auto ct : itemData->GetCommon()->GetCorrectTbl())
            {
                correctTbls.push_back(ct);
            }
        }
    }

    GetStatusEffectCorrectTbls(definitionManager, correctTbls);

    AdjustStats(correctTbls, stats, true);
    CharacterManager::CalculateDependentStats(stats, cs->GetLevel(), false);
    AdjustStats(correctTbls, stats, false);

    return CompareAndResetStats(stats);
}

template<>
uint8_t ActiveEntityStateImp<objects::Demon>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager)
{
    auto d = GetEntity();
    if (nullptr == d)
    {
        return true;
    }

    return RecalculateDemonStats(definitionManager, d->GetType());
}

template<>
uint8_t ActiveEntityStateImp<objects::Enemy>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager)
{
    auto e = GetEntity();
    if (nullptr == e)
    {
        return true;
    }

    return RecalculateDemonStats(definitionManager, e->GetType());
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
    libcomp::EnumMap<CorrectTbl, int16_t>& stats, bool baseMode) const
{
    std::set<CorrectTbl> removed;
    for(auto ct : adjustments)
    {
        auto tblID = ct->GetID();

        // Only adjust base or calculated stats depending on mode
        if(baseMode != (BASE_STATS.find(tblID) != BASE_STATS.end())) continue;

        // If a value is reduced to 0%, leave it
        if(removed.find(tblID) != removed.end()) continue;

        switch(ct->GetType())
        {
        case objects::MiCorrectTbl::Type_t::PERCENT:
            // Percentage sets can either be an immutable set to zero
            // or an increase/decrease by a set amount
            if(ct->GetValue() == 0)
            {
                removed.insert(tblID);
                stats[tblID] = 0;
            }
            else
            {
                stats[tblID] = (int16_t)(stats[tblID] +
                    (int16_t)(stats[tblID] * (ct->GetValue() * 0.01)));
            }
            break;
        case objects::MiCorrectTbl::Type_t::NUMERIC:
            stats[tblID] = (int16_t)(stats[tblID] + ct->GetValue());
            break;
        default:
            break;
        }
    }

    CharacterManager::AdjustStatBounds(stats);
}

void ActiveEntityState::GetStatusEffectCorrectTbls(
    libcomp::DefinitionManager* definitionManager,
    std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments)
{
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

    // Sort the adjustments, set to 0% first, non-zero percents next, numeric last
    adjustments.sort([](const std::shared_ptr<objects::MiCorrectTbl>& a,
        const std::shared_ptr<objects::MiCorrectTbl>& b)
    {
        return a->GetType() == objects::MiCorrectTbl::Type_t::PERCENT &&
            (a->GetValue() == 0 ||
            b->GetType() != objects::MiCorrectTbl::Type_t::PERCENT);
    });
}

uint8_t ActiveEntityState::RecalculateDemonStats(
    libcomp::DefinitionManager* definitionManager, uint32_t demonID)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto demonData = definitionManager->GetDevilData(demonID);
    auto battleData = demonData->GetBattleData();
    auto cs = GetCoreStats();

    libcomp::EnumMap<CorrectTbl, int16_t> stats;
    stats[CorrectTbl::STR] = cs->GetSTR();
    stats[CorrectTbl::MAGIC] = cs->GetMAGIC();
    stats[CorrectTbl::VIT] = cs->GetVIT();
    stats[CorrectTbl::INT] = cs->GetINTEL();
    stats[CorrectTbl::SPEED] = cs->GetSPEED();
    stats[CorrectTbl::LUCK] = cs->GetLUCK();

    for(size_t i = 0; i < 126; i++)
    {
        CorrectTbl tblID = (CorrectTbl)i;
        if(BASE_STATS.find(tblID) == BASE_STATS.end())
        {
            stats[tblID] = battleData->GetCorrect((size_t)i);
        }
    }

    std::list<std::shared_ptr<objects::MiCorrectTbl>> correctTbls;
    GetStatusEffectCorrectTbls(definitionManager, correctTbls);

    AdjustStats(correctTbls, stats, true);
    CharacterManager::CalculateDependentStats(stats, cs->GetLevel(), true);
    AdjustStats(correctTbls, stats, false);

    return CompareAndResetStats(stats);
}

uint8_t ActiveEntityState::CompareAndResetStats(libcomp::EnumMap<CorrectTbl, int16_t>& stats)
{
    auto cs = GetCoreStats();
    int16_t hp = cs->GetHP();
    int16_t mp = cs->GetMP();
    if(hp > stats[CorrectTbl::HP_MAX])
    {
        hp = stats[CorrectTbl::HP_MAX];
    }

    if(mp > stats[CorrectTbl::MP_MAX])
    {
        mp = stats[CorrectTbl::HP_MAX];
    }

    bool worldChange =
        hp != cs->GetHP()
        || mp != cs->GetMP()
        || GetMaxHP() != stats[CorrectTbl::HP_MAX]
        || GetMaxMP() != stats[CorrectTbl::MP_MAX];

    bool anyChange = worldChange
        || GetSTR() != stats[CorrectTbl::STR]
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
        || GetMDEF() != stats[CorrectTbl::MDEF]
        || GetHPRegen() != stats[CorrectTbl::HP_REGEN]
        || GetMPRegen() != stats[CorrectTbl::MP_REGEN];

    cs->SetHP(hp);
    cs->SetMP(mp);
    SetMaxHP(stats[CorrectTbl::HP_MAX]);
    SetMaxMP(stats[CorrectTbl::MP_MAX]);
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
    SetHPRegen(stats[CorrectTbl::HP_REGEN]);
    SetMPRegen(stats[CorrectTbl::MP_REGEN]);

    return worldChange ? 2 : (anyChange ? 1 : 0);
}
