/**
 * @file server/channel/src/TokuseiManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages tokusei specific logic for the server and validates
 *  the definitions read at run time.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

#include "TokuseiManager.h"

// libcomp Includes
#include <Log.h>

// object Includes
#include <CalculatedEntityState.h>
#include <Clan.h>
#include <EnchantSetData.h>
#include <Expertise.h>
#include <Item.h>
#include <MiCategoryData.h>
#include <MiDCategoryData.h>
#include <MiDevilCrystalData.h>
#include <MiDevilData.h>
#include <MiEnchantCharasticData.h>
#include <MiEnchantData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiNPCBasicData.h>
#include <MiSkillCharasticData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpecialConditionData.h>
#include <MiSStatusData.h>
#include <Party.h>
#include <Tokusei.h>
#include <TokuseiAttributes.h>
#include <TokuseiCondition.h>
#include <TokuseiCorrectTbl.h>
#include <TokuseiSkillCondition.h>

// C++ Standard Includes
#include <cmath>

// channel Includes
#include "ChannelServer.h"
#include "ServerConstants.h"
#include "Zone.h"

using namespace channel;

TokuseiManager::TokuseiManager(const std::weak_ptr<
    ChannelServer>& server) : mServer(server)
{
}

TokuseiManager::~TokuseiManager()
{
}

bool TokuseiManager::Initialize()
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    std::set<int32_t> skillGrantTokusei;
    auto allTokusei = definitionManager->GetAllTokuseiData();
    for(auto tPair : allTokusei)
    {
        // Sanity check to ensure that skill granting tokusei are not
        // 1) Conditional
        // 2) Inherited from secondary sources
        // 3) Chaining other skill granting effects
        std::set<uint32_t> skillIDs;
        for(auto aspect : tPair.second->GetAspects())
        {
            if(aspect->GetType() == TokuseiAspectType::SKILL_ADD)
            {
                if(tPair.second->GetTargetType() != objects::Tokusei::TargetType_t::SELF)
                {
                    LOG_ERROR(libcomp::String("Skill granting tokusei encountered"
                        " with target type other than 'self': %1\n").Arg(tPair.first));
                    return false;
                }
                else if(tPair.second->ConditionsCount() > 0 ||
                    tPair.second->SkillConditionsCount() > 0)
                {
                    LOG_ERROR(libcomp::String("Conditional skill granting tokusei"
                        " encountered: %1\n").Arg(tPair.first));
                    return false;
                }

                skillGrantTokusei.insert(tPair.first);
                skillIDs.insert((uint32_t)aspect->GetValue());
            }
            else if(aspect->GetType() == TokuseiAspectType::CONSTANT_STATUS)
            {
                // Also keep track of constant status effect sources
                mStatusEffectTokusei[(uint32_t)aspect->GetValue()].insert(tPair.first);
            }
        }

        for(uint32_t skillID : skillIDs)
        {
            auto skillData = definitionManager->GetSkillData(skillID);
            if(skillData)
            {
                for(int32_t tokuseiID : skillData->GetCharastic()->GetCharastic())
                {
                    auto it = skillGrantTokusei.find(tokuseiID);
                    if(it != skillGrantTokusei.end())
                    {
                        LOG_ERROR(libcomp::String("Skill granted from tokusei '%1'"
                            " contains a nested skill granting effect: '%2'\n")
                            .Arg(skillID).Arg(tokuseiID));
                        return false;
                    }
                }
            }
        }

        if(tPair.second->SkillConditionsCount() > 0)
        {
            // Make sure skill state conditions do not have a mix of target and source types
            // and only use equals/not equals comparisons
            bool skillTargetCondition = false;
            bool skillSourceCondition = false;
            for(auto condition : tPair.second->GetSkillConditions())
            {
                skillTargetCondition |= condition->GetTargetCondition();
                skillSourceCondition |= !condition->GetTargetCondition();

                if(condition->GetComparator() != objects::TokuseiCondition::Comparator_t::EQUALS &&
                    condition->GetComparator() != objects::TokuseiCondition::Comparator_t::NOT_EQUAL)
                {
                    LOG_ERROR(libcomp::String("Skill tokusei conditions can only compare"
                        " simple equals/not equal conditions: %1\n").Arg(tPair.first));
                    return false;
                }
            }

            if(skillTargetCondition && skillSourceCondition)
            {
                LOG_ERROR(libcomp::String("Skill tokusei encounterd with a both"
                    " source and target conditions: %1\n").Arg(tPair.first));
                return false;
            }

            // Make sure no skill based effects increase rates that are side-effects
            // rather than directly affecting the skill outcome and also do not grant
            // constant status effects
            bool invalidSkillRate = false;
            for(auto aspect : tPair.second->GetAspects())
            {
                if(aspect->GetType() == TokuseiAspectType::BETHEL_RATE ||
                    aspect->GetType() == TokuseiAspectType::CONSTANT_STATUS ||
                    aspect->GetType() == TokuseiAspectType::FAMILIARITY_UP_RATE ||
                    aspect->GetType() == TokuseiAspectType::FAMILIARITY_DOWN_RATE ||
                    aspect->GetType() == TokuseiAspectType::SOUL_POINT_RATE)
                {
                    invalidSkillRate = true;
                    break;
                }
            }

            const std::set<uint8_t> invalidCorrectTypes =
                {
                    (uint8_t)CorrectTbl::RATE_XP,
                    (uint8_t)CorrectTbl::RATE_MAG,
                    (uint8_t)CorrectTbl::RATE_MACCA,
                    (uint8_t)CorrectTbl::RATE_EXPERTISE
                };

            for(auto ct : tPair.second->GetCorrectValues())
            {
                if(invalidCorrectTypes.find(ct->GetType()) != invalidCorrectTypes.end())
                {
                    invalidSkillRate = true;
                    break;
                }
            }

            for(auto ct : tPair.second->GetTokuseiCorrectValues())
            {
                if(invalidCorrectTypes.find(ct->GetType()) != invalidCorrectTypes.end())
                {
                    invalidSkillRate = true;
                    break;
                }
            }

            if(invalidSkillRate)
            {
                LOG_ERROR(libcomp::String("Skill tokusei encounterd with an invalid"
                    " rate adjustment: %1\n").Arg(tPair.first));
                return false;
            }
        }

        if(!GatherTimedTokusei(tPair.second))
        {
            return false;
        }
    }

    // Verify conditional enchantment tokusei which are restricted from
    // doing any of the following when based upon core stat conditions:
    // 1) Contains additional non-skill processing conditions
    // 2) Affects a target other than the source
    // 3) Modifies core stats by a percentage (numeric is okay)
    // 4) Adds skills
    // This is critical in enforcing a reasonable tokusei calculation process
    // as all non-core stat conditions can be evaluated at tokusei recalc time.
    std::set<int32_t> baseStatTokuseiIDs;
    for(auto ePair : definitionManager->GetAllEnchantData())
    {
        for(auto cData : { ePair.second->GetDevilCrystal()->GetSoul(),
            ePair.second->GetDevilCrystal()->GetTarot() })
        {
            for(auto conditionData : cData->GetConditions())
            {
                int32_t conditionType = (int32_t)conditionData->GetType();
                if(conditionType >= (10 + (int32_t)CorrectTbl::STR) &&
                    conditionType < (10 + (int32_t)CorrectTbl::LUCK))
                {
                    for(uint32_t tokuseiID : conditionData->GetTokusei())
                    {
                        if(tokuseiID != 0)
                        {
                            baseStatTokuseiIDs.insert((int32_t)tokuseiID);
                        }
                    }
                }
            }
        }
    }

    for(auto ePair : definitionManager->GetAllEnchantSetData())
    {
        for(auto conditionData : ePair.second->GetConditions())
        {
            int32_t conditionType = (int32_t)conditionData->GetType();
            if(conditionType >= (10 + (int32_t)CorrectTbl::STR) &&
                conditionType < (10 + (int32_t)CorrectTbl::LUCK))
            {
                for(uint32_t tokuseiID : conditionData->GetTokusei())
                {
                    if(tokuseiID != 0)
                    {
                        baseStatTokuseiIDs.insert((int32_t)tokuseiID);
                    }
                }
            }
        }
    }

    for(int32_t tokuseiID : baseStatTokuseiIDs)
    {
        auto it = allTokusei.find(tokuseiID);
        if(it != allTokusei.end())
        {
            auto tokuseiData = it->second;

            if(tokuseiData->ConditionsCount() > 0)
            {
                LOG_ERROR(libcomp::String("Stat conditional enchantment tokusei"
                    " encountered with non-skill conditions: %1\n").Arg(it->first));
                return false;
            }

            if(tokuseiData->GetTargetType() !=
                objects::Tokusei::TargetType_t::SELF)
            {
                LOG_ERROR(libcomp::String("Stat conditional enchantment tokusei"
                    " encountered with non-source target type: %1\n").Arg(it->first));
                return false;
            }

            auto cTables = tokuseiData->GetCorrectValues();
            for(auto ct : tokuseiData->GetTokuseiCorrectValues())
            {
                cTables.push_back(ct);
            }

            for(auto ct : cTables)
            {
                if(ct->GetID() <= CorrectTbl::LUCK &&
                    (ct->GetType() == 1 || ct->GetType() == 101))
                {
                    LOG_ERROR(libcomp::String("Stat conditional enchantment tokusei"
                        " encountered with percentage core stat adjustment: %1\n")
                        .Arg(it->first));
                    return false;
                }
            }

            if(skillGrantTokusei.find(it->first) != skillGrantTokusei.end())
            {
                LOG_ERROR(libcomp::String("Skill granting stat conditional enchantment"
                    " tokusei encountered: %1\n").Arg(it->first));
                return false;
            }
        }
    }

    return true;
}

bool TokuseiManager::GatherTimedTokusei(const std::shared_ptr<
    objects::Tokusei>& tokusei)
{
    auto server = mServer.lock();

    // Verify and contruct the WorkClockTime equivalents of all timed tokusei
    std::unordered_map<uint8_t,
        std::list<std::shared_ptr<objects::TokuseiCondition>>> afterTime;
    std::unordered_map<uint8_t,
        std::list<std::shared_ptr<objects::TokuseiCondition>>> beforeTime;
    for(auto condition : tokusei->GetConditions())
    {
        switch(condition->GetType())
        {
        case objects::TokuseiCondition::Type_t::GAME_TIME:
        case objects::TokuseiCondition::Type_t::MOON_PHASE:
            switch(condition->GetComparator())
            {
            case objects::TokuseiCondition::Comparator_t::EQUALS:
                afterTime[condition->GetOptionGroupID()].push_back(condition);
                beforeTime[condition->GetOptionGroupID()].push_back(condition);
                break;
            case objects::TokuseiCondition::Comparator_t::GTE:
                afterTime[condition->GetOptionGroupID()].push_back(condition);
                break;
            case objects::TokuseiCondition::Comparator_t::LTE:
                beforeTime[condition->GetOptionGroupID()].push_back(condition);
                break;
            default:
                LOG_ERROR(libcomp::String("Invalid comparator"
                    " encountered on time restricted tokusei '%1'\n")
                    .Arg(tokusei->GetID()));
                return false;
                break;
            }
            break;
        default:
            break;
        }
    }

    if(afterTime.size() > 0 || beforeTime.size() > 0)
    {
        if(afterTime.size() != beforeTime.size())
        {
            LOG_ERROR(libcomp::String("Encountered time"
                " restricted tokusei with at least one condition"
                " option group that is not a timespan: '%1'\n")
                .Arg(tokusei->GetID()));
            return false;
        }

        for(auto timePair : beforeTime)
        {
            if(afterTime[timePair.first].size() == 0)
            {
                LOG_ERROR(libcomp::String("Encountered time"
                    " restricted tokusei with condition"
                    " option group that is not a timespan: '%1' (%2)\n")
                    .Arg(tokusei->GetID()).Arg(timePair.first));
                return false;
            }

            bool success = true;

            // Make sure the timespans are valid
            WorldClockTime before;
            for(auto condition : beforeTime[timePair.first])
            {
                success &= BuildWorldClockTime(condition, before);
            }

            WorldClockTime after;
            for(auto condition : afterTime[timePair.first])
            {
                success &= BuildWorldClockTime(condition, after);
            }

            if(!success)
            {
                LOG_ERROR(libcomp::String("Encountered time"
                    " restricted tokusei with invalid timespan"
                    " option group: '%1' (%2)\n")
                    .Arg(tokusei->GetID()).Arg(timePair.first));
                return false;
            }

            // Update existing registered times or add new
            for(WorldClockTime t : { before, after })
            {
                server->RegisterClockEvent(t, 2, 0);
            }
        }

        // Add to the set containing all timed tokusei
        mTimedTokusei[tokusei->GetID()] = false;
    }

    return true;
}

std::unordered_map<int32_t, bool> TokuseiManager::Recalculate(const std::shared_ptr<
    ActiveEntityState>& eState, std::set<TokuseiConditionType> changes)
{
    bool doRecalc = false;

    // Since anything pertaining to party members or summoning a new demon requires
    // a full recalculation check, only check another entity if a partner demon's
    // familiarity changed
    if(eState->GetEntityType() == objects::EntityStateObject::EntityType_t::PARTNER_DEMON &&
        changes.find(TokuseiConditionType::PARTNER_FAMILIARITY) != changes.end())
    {
        auto state = ClientState::GetEntityClientState(eState->GetEntityID(), false);
        if(state)
        {
            auto cState = state->GetCharacterState();
            auto triggers = cState->GetCalculatedState()->GetActiveTokuseiTriggers();
            doRecalc = triggers.find((int8_t)TokuseiConditionType::PARTNER_FAMILIARITY)
                != triggers.end();
        }
    }

    if(!doRecalc)
    {
        auto triggers = eState->GetCalculatedState()->GetActiveTokuseiTriggers();
        for(auto change : changes)
        {
            if(triggers.find((int8_t)change) != triggers.end())
            {
                doRecalc = true;
                break;
            }
        }
    }

    if(doRecalc)
    {
        return Recalculate(eState);
    }

    return std::unordered_map<int32_t, bool>();
}

std::unordered_map<int32_t, bool> TokuseiManager::Recalculate(const std::shared_ptr<
    ActiveEntityState>& eState, bool recalcStats, std::set<int32_t> ignoreStatRecalc)
{
    std::list<std::shared_ptr<ActiveEntityState>> entities = GetAllTokuseiEntities(eState);
    return Recalculate(entities, recalcStats, ignoreStatRecalc);
}

std::unordered_map<int32_t, bool> TokuseiManager::Recalculate(const std::list<std::shared_ptr<
    ActiveEntityState>>& entities, bool recalcStats, std::set<int32_t> ignoreStatRecalc)
{
    std::unordered_map<int32_t, bool> result;

    // Effects directly on the entity
    std::unordered_map<int32_t,
        std::unordered_map<bool, std::unordered_map<int32_t, uint16_t>>> newMaps;

    // Effects on the whole party
    std::unordered_map<int32_t,
        std::unordered_map<bool, std::unordered_map<int32_t, uint16_t>>> partyEffects;

    // Effects on an entity's partner or summoner
    std::unordered_map<int32_t,
        std::unordered_map<bool, std::unordered_map<int32_t, uint16_t>>> otherEffects;

    // Keep track of direct timed tokusei on all player entities
    std::unordered_map<int32_t, std::set<int32_t>> playerEntityTimedTokusei;

    for(auto eState : entities)
    {
        result[eState->GetEntityID()] = false;

        int32_t worldCID = 0;
        auto state = ClientState::GetEntityClientState(eState->GetEntityID(),
            false);
        if(state)
        {
            worldCID = state->GetWorldCID();

            // Make sure there's always an entry per player
            playerEntityTimedTokusei[worldCID];
        }

        std::set<int8_t> triggers;

        std::unordered_map<int32_t, bool> evaluated;
        for(auto tokusei : GetDirectTokusei(eState))
        {
            int32_t tokuseiID = tokusei->GetID();

            bool add = false;
            if(evaluated.find(tokuseiID) != evaluated.end())
            {
                add = evaluated[tokuseiID];
            }
            else
            {
                add = EvaluateTokuseiConditions(eState, tokusei);
                evaluated[tokuseiID] = add;

                if(worldCID &&
                    mTimedTokusei.find(tokuseiID) != mTimedTokusei.end())
                {
                    playerEntityTimedTokusei[worldCID].insert(tokuseiID);
                }

                for(auto condition : tokusei->GetConditions())
                {
                    triggers.insert((int8_t)condition->GetType());
                }
            }

            if(add)
            {
                bool skillTokusei = tokusei->SkillConditionsCount() > 0;

                std::unordered_map<int32_t, uint16_t>* map = 0;
                switch(tokusei->GetTargetType())
                {
                case objects::Tokusei::TargetType_t::PARTY:
                    map = &partyEffects[eState->GetEntityID()][skillTokusei];
                    break;
                case objects::Tokusei::TargetType_t::SUMMONER:
                    if(eState->GetEntityType() ==
                        objects::EntityStateObject::EntityType_t::PARTNER_DEMON)
                    {
                        map = &otherEffects[eState->GetEntityID()][skillTokusei];
                    }
                    break;
                case objects::Tokusei::TargetType_t::PARTNER:
                    if(eState->GetEntityType() ==
                        objects::EntityStateObject::EntityType_t::CHARACTER)
                    {
                        map = &otherEffects[eState->GetEntityID()][skillTokusei];
                    }
                    break;
                case objects::Tokusei::TargetType_t::SELF:
                default:
                    map = &newMaps[eState->GetEntityID()][skillTokusei];
                    break;
                }

                if(map)
                {
                    if(map->find(tokuseiID) == map->end())
                    {
                        (*map)[tokuseiID] = 1;
                    }
                    else
                    {
                        (*map)[tokuseiID]++;
                    }
                }
            }
        }

        eState->GetCalculatedState()->SetActiveTokuseiTriggers(triggers);
    }

    // Set or clear all timed tokusei for player entities
    if(playerEntityTimedTokusei.size() > 0)
    {
        std::lock_guard<std::mutex> lock(mTimeLock);
        for(auto& ePair : playerEntityTimedTokusei)
        {
            if(ePair.second.size() > 0)
            {
                mTimedTokuseiEntities[ePair.first] = ePair.second;
            }
            else
            {
                mTimedTokuseiEntities.erase(ePair.first);
            }
        }
    }

    // Loop back through and add all party/other effects
    for(auto eState : entities)
    {
        auto state = ClientState::GetEntityClientState(eState->GetEntityID(), false);
        if(!state) continue;

        auto cState = state->GetCharacterState();
        auto dState = state->GetDemonState();
        auto otherEntityID = eState == cState ? dState->GetEntityID() : cState->GetEntityID();

        if(otherEntityID)
        {
            auto& map = newMaps[otherEntityID];
            for(auto pair : otherEffects[eState->GetEntityID()])
            {
                for(auto bPair : pair.second)
                {
                    if(map[pair.first].find(bPair.first) == map[pair.first].end())
                    {
                        map[pair.first][bPair.first] = bPair.second;
                    }
                    else
                    {
                        map[pair.first][bPair.first] = (uint16_t)(
                            map[pair.first][bPair.first] + bPair.second);
                    }
                }
            }
        }

        // All characters in the zone (including the source) gain the effect
        if(state->GetParty())
        {
            for(auto e : entities)
            {
                if(e->GetEntityType() == objects::EntityStateObject::EntityType_t::CHARACTER
                    && e->GetZone() == eState->GetZone())
                {
                    auto& map = newMaps[e->GetEntityID()];
                    for(auto pair : partyEffects[eState->GetEntityID()])
                    {
                        for(auto bPair : pair.second)
                        {
                            if(map[pair.first].find(bPair.first) == map[pair.first].end())
                            {
                                map[pair.first][bPair.first] = bPair.second;
                            }
                            else
                            {
                                map[pair.first][bPair.first] = (uint16_t)(
                                    map[pair.first][bPair.first] + bPair.second);
                            }
                        }
                    }
                }
            }
        }
    }

    // Now that all tokusei have been calculated, compare and add them to their
    // respective entities
    std::list<std::shared_ptr<ActiveEntityState>> updatedEntities;
    for(auto eState : entities)
    {
        bool updated = false;

        auto calcState = eState->GetCalculatedState();
        for(bool skillMode : { false, true })
        {
            auto& selfMap = newMaps[eState->GetEntityID()][skillMode];
            auto currentTokusei = skillMode ? calcState->GetPendingSkillTokusei()
                : calcState->GetEffectiveTokusei();
            if(currentTokusei.size() != selfMap.size())
            {
                updated = true;
            }
            else
            {
                for(auto pair : selfMap)
                {
                    if(currentTokusei.find(pair.first) == currentTokusei.end() ||
                        currentTokusei[pair.first] != pair.second)
                    {
                        updated = true;
                        break;
                    }
                }
            }

            if(updated)
            {
                break;
            }
        }

        if(updated)
        {
            auto& effective = newMaps[eState->GetEntityID()][false];
            calcState->SetEffectiveTokusei(effective);
            calcState->SetPendingSkillTokusei(newMaps[eState->GetEntityID()][true]);
            calcState->ClearEffectiveTokuseiFinal();
            calcState->ClearPendingSkillTokuseiFinal();

            // Update constant status effects
            AddStatusEffectMap m;

            auto currentEffects = eState->GetStatusEffects();
            for(auto statusPair : mStatusEffectTokusei)
            {
                bool exists = currentEffects.find(statusPair.first) !=
                    currentEffects.end();

                bool apply = false;
                for(int32_t source : statusPair.second)
                {
                    if(effective.find(source) != effective.end())
                    {
                        apply = true;
                        break;
                    }
                }

                if(apply && !exists)
                {
                    m[statusPair.first] = std::pair<uint8_t, bool>(1, true);
                }
                else if(!apply && exists)
                {
                    m[statusPair.first] = std::pair<uint8_t, bool>(0, true);
                }
            }

            if(m.size() > 0)
            {
                eState->AddStatusEffects(m,
                    mServer.lock()->GetDefinitionManager());
            }

            updatedEntities.push_back(eState);
        }
    }

    if(recalcStats)
    {
        auto characterManager = mServer.lock()->GetCharacterManager();
        auto connectionManager = mServer.lock()->GetManagerConnection();
        for(auto eState : updatedEntities)
        {
            if(ignoreStatRecalc.find(eState->GetEntityID()) == ignoreStatRecalc.end())
            {
                auto client = connectionManager->GetEntityClient(eState->GetEntityID());
                characterManager->RecalculateStats(client, eState->GetEntityID());

                result[eState->GetEntityID()] = true;
            }
        }
    }

    return result;
}

std::unordered_map<int32_t, bool> TokuseiManager::RecalculateParty(const std::shared_ptr<
    objects::Party>& party)
{
    std::unordered_map<int32_t, bool> result;

    if(party)
    {
        std::list<std::shared_ptr<ActiveEntityState>> entities;
        for(auto memberID : party->GetMemberIDs())
        {
            auto state = ClientState::GetEntityClientState(memberID, true);
            auto cState = state ? state->GetCharacterState() : nullptr;

            if(cState && cState->Ready() && cState->GetZone())
            {
                entities.push_back(cState);

                auto dState = state->GetDemonState();
                if(dState && dState->Ready())
                {
                    entities.push_back(dState);
                }
            }
        }

        result = Recalculate(entities, true);
    }

    return result;
}

std::list<std::shared_ptr<ActiveEntityState>> TokuseiManager::GetAllTokuseiEntities(
    const std::shared_ptr<ActiveEntityState>& eState)
{
    std::list<std::shared_ptr<ActiveEntityState>> retval;

    auto state = ClientState::GetEntityClientState(eState->GetEntityID(), false);
    if(state)
    {
        retval.push_back(state->GetCharacterState());

        auto dState = state->GetDemonState();
        if(dState && dState->Ready())
        {
            retval.push_back(dState);
        }

        // Add party members also in the zone
        auto party = state->GetParty();
        if(party)
        {
            auto zone = eState->GetZone();
            for(auto memberID : party->GetMemberIDs())
            {
                if(memberID != state->GetWorldCID())
                {
                    auto state2 = ClientState::GetEntityClientState(memberID, true);
                    if(state2)
                    {
                        auto cState2 = state2->GetCharacterState();
                        if(cState2->GetZone() == zone && cState2->Ready())
                        {
                            retval.push_back(state2->GetCharacterState());

                            auto dState2 = state2->GetDemonState();
                            if(dState2 && dState2->Ready())
                            {
                                retval.push_back(dState2);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        retval.push_back(eState);
    }

    return retval;
}

std::list<std::shared_ptr<objects::Tokusei>> TokuseiManager::GetDirectTokusei(
    const std::shared_ptr<ActiveEntityState>& eState)
{
    std::list<std::shared_ptr<objects::Tokusei>> retval;

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    // Get non-tokusei skills
    std::set<uint32_t> skillIDs = eState->GetAllSkills(definitionManager, false);

    // Remove disabled skills
    for(uint32_t skillID : eState->GetDisabledSkills())
    {
        skillIDs.erase(skillID);
    }

    // Since skill granting tokusei only affect the source entity and cannot be
    // conditional, gather other skill IDs granted by tokusei effects before
    // pulling the skill tokusei

    std::list<int32_t> tokuseiIDs;
    switch(eState->GetEntityType())
    {
    case objects::EntityStateObject::EntityType_t::CHARACTER:
        {
            auto characterManager = server->GetCharacterManager();
            auto cState = std::dynamic_pointer_cast<CharacterState>(eState);
            auto character = cState->GetEntity();
            auto cs = cState->GetCoreStats();

            // Default to tokusei from equipment
            tokuseiIDs = cState->GetEquipmentTokuseiIDs();

            // Add any conditional tokusei
            for(auto condition : cState->GetConditionalTokusei())
            {
                bool add = false;

                int16_t p1 = condition->GetParams(0);
                int16_t p2 = condition->GetParams(1);

                int16_t conditionType = condition->GetType();
                if(conditionType == 1)
                {
                    // Level check
                    add = (p1 == 0 || (int16_t)cs->GetLevel() >= p1) &&
                        (p2 == 0 || (int16_t)cs->GetLevel() <= p2);
                }
                else if(conditionType == 2)
                {
                    // LNC check (inverted format)
                    switch(cState->GetLNCType())
                    {
                    case LNC_LAW:
                        add = ((p1 & 0x0004) != 0);
                        break;
                    case LNC_NEUTRAL:
                        add = ((p1 & 0x0002) != 0);
                        break;
                    case LNC_CHAOS:
                        add = ((p1 & 0x0001) != 0);
                    default:
                        break;
                    }
                }
                else if(conditionType >= 100 && conditionType <= 158)
                {
                    // Expertise #(type - 100) rank check
                    add = characterManager->GetExpertiseRank(cState,
                        (uint32_t)(conditionType - 100)) >= (uint8_t)p1;
                }

                if(add)
                {
                    for(auto tokuseiID : condition->GetTokusei())
                    {
                        if(tokuseiID != 0)
                        {
                            tokuseiIDs.push_back((int32_t)tokuseiID);
                        }
                    }
                }
            }
        }
        break;
    case objects::EntityStateObject::EntityType_t::PARTNER_DEMON:
        {
            auto dState = std::dynamic_pointer_cast<DemonState>(eState);

            tokuseiIDs = dState->GetCompendiumTokuseiIDs();
        }
        break;
    default:
        break;
    }

    // Get S-status effect tokusei
    for(auto pair : eState->GetStatusEffects())
    {
        auto sStatus = definitionManager->GetSStatusData(pair.first);
        if(sStatus)
        {
            for(int32_t tokuseiID : sStatus->GetTokusei())
            {
                tokuseiIDs.push_back(tokuseiID);
            }
        }
    }

    // Get any extra tokusei
    for(auto pair : eState->GetAdditionalTokusei())
    {
        for(uint16_t i = 0; i < pair.second; i++)
        {
            tokuseiIDs.push_back(pair.first);
        }
    }

    // Add each tokusei already identified to the result set and add any skills
    // added by these effects
    for(int32_t tokuseiID : tokuseiIDs)
    {
        auto tokusei = definitionManager->GetTokuseiData(tokuseiID);
        if(tokusei)
        {
            retval.push_back(tokusei);
            for(auto aspect : tokusei->GetAspects())
            {
                if(aspect->GetType() == TokuseiAspectType::SKILL_ADD)
                {
                    skillIDs.insert((uint32_t)aspect->GetValue());
                }
            }
        }
    }

    // Gather the remaining tokusei from the skills on the entity
    for(uint32_t skillID : skillIDs)
    {
        auto skillData = definitionManager->GetSkillData(skillID);
        if(skillData)
        {
            if(skillData->GetCommon()->GetCategory()->GetMainCategory() == 2 &&
                !eState->ActiveSwitchSkillsContains(skillID))
            {
                // Inactive switch skill
                continue;
            }

            for(int32_t tokuseiID : skillData->GetCharastic()->GetCharastic())
            {
                auto tokusei = definitionManager->GetTokuseiData(tokuseiID);
                if(tokusei)
                {
                    retval.push_back(tokusei);
                }
            }
        }
    }

    return retval;
}

bool TokuseiManager::EvaluateTokuseiConditions(const std::shared_ptr<
    ActiveEntityState>& eState, const std::shared_ptr<objects::Tokusei>& tokusei)
{
    if(tokusei->ConditionsCount() == 0)
    {
        return true;
    }
    else if(!eState->Ready())
    {
        return false;
    }

    int32_t tokuseiID = tokusei->GetID();

    // Compare singular (and) and option group (or) conditions and
    // only return true if the entire clause evaluates to true
    std::unordered_map<uint8_t, bool> optionGroups;
    for(auto condition : tokusei->GetConditions())
    {
        bool result = false;

        // If the option group has already had a condition pass, skip it
        uint8_t optionGroupID = condition->GetOptionGroupID();
        if(optionGroupID != 0)
        {
            if(optionGroups.find(optionGroupID) == optionGroups.end())
            {
                optionGroups[optionGroupID] = false;
            }
            else
            {
                result = optionGroups[optionGroupID];
            }
        }

        if(!result)
        {
            result = EvaluateTokuseiCondition(eState, tokuseiID, condition);
            if(optionGroupID != 0)
            {
                optionGroups[optionGroupID] |= result;
            }
            else if(!result)
            {
                return false;
            }
        }
    }

    for(auto pair : optionGroups)
    {
        if(!pair.second)
        {
            return false;
        }
    }

    return true;
}


bool TokuseiManager::EvaluateTokuseiCondition(const std::shared_ptr<ActiveEntityState>& eState,
    int32_t tokuseiID, const std::shared_ptr<objects::TokuseiCondition>& condition)
{
    bool numericCompare = condition->GetComparator() != objects::TokuseiCondition::Comparator_t::EQUALS &&
        condition->GetComparator() != objects::TokuseiCondition::Comparator_t::NOT_EQUAL;

    bool isPartnerCondition = false;
    switch(condition->GetType())
    {
    case TokuseiConditionType::CURRENT_HP:
    case TokuseiConditionType::CURRENT_MP:
        // Current HP or MP percent matches the comparison type and value
        {
            auto cs = eState->GetCoreStats();
            if(!cs)
            {
                return false;
            }

            int32_t currentValue = 0;
            if(condition->GetType() == TokuseiConditionType::CURRENT_HP)
            {
                currentValue = (int32_t)floor((float)cs->GetHP() / (float)eState->GetMaxHP() * 100.f);
            }
            else
            {
                currentValue = (int32_t)floor((float)cs->GetMP() / (float)eState->GetMaxMP() * 100.f);
            }

            return Compare(currentValue, condition, true);
        }
        break;
    case TokuseiConditionType::DIGITALIZED:
        // Entity is a character and is digitalized
        /// @todo: implement once digitalization is supported
        return false;
        break;
    case TokuseiConditionType::EQUIPPED:
        // Entity is a character and has the specified item equipped
        if(numericCompare ||
            eState->GetEntityType() != objects::EntityStateObject::EntityType_t::CHARACTER)
        {
            return false;
        }
        else
        {
            auto cState = std::dynamic_pointer_cast<CharacterState>(eState);

            bool equipped = false;
            for(auto equip : cState->GetEntity()->GetEquippedItems())
            {
                if(!equip.IsNull() && equip->GetType() == (uint32_t)condition->GetValue())
                {
                    equipped = true;
                    break;
                }
            }

            return equipped == (condition->GetComparator() ==
                objects::TokuseiCondition::Comparator_t::EQUALS);
        }
        break;
    case TokuseiConditionType::EQUIPPED_WEAPON_TYPE:
        // Entity is a character and has the specified weapon type equipped
        if(numericCompare ||
            eState->GetEntityType() != objects::EntityStateObject::EntityType_t::CHARACTER)
        {
            return false;
        }
        else
        {
            auto cState = std::dynamic_pointer_cast<CharacterState>(eState);

            auto equip = cState->GetEntity()->GetEquippedItems(
                (size_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON).Get();
            bool equipped = false;
            if(equip)
            {
                auto itemData = mServer.lock()->GetDefinitionManager()->GetItemData(
                    equip->GetType());
                equipped = itemData && (int32_t)itemData->GetBasic()->GetWeaponType()
                    == condition->GetValue();
            }

            return equipped == (condition->GetComparator() ==
                objects::TokuseiCondition::Comparator_t::EQUALS);
        }
        break;
    case TokuseiConditionType::EXPERTISE:
        // Entity is a character and has the specified expertise rank value
        if(eState->GetEntityType() != objects::EntityStateObject::EntityType_t::CHARACTER)
        {
            return false;
        }
        else
        {
            auto cState = std::dynamic_pointer_cast<CharacterState>(eState);

            // The 2 smallest digits are the expertise ID, the rest are the rank value
            int32_t expertiseID = (int32_t)(condition->GetValue() % 100);
            int32_t rankCompare = (int32_t)((condition->GetValue() - expertiseID) / 100);
            uint8_t rank = mServer.lock()->GetCharacterManager()->GetExpertiseRank(cState,
                (uint32_t)expertiseID);

            return Compare(rankCompare, (int32_t)rank, condition, true);
        }
        break;
    case TokuseiConditionType::LNC:
        // Entity is one of the listed LNC types (stored as flags)
        if(numericCompare)
        {
            return false;
        }
        else
        {
            bool containsLNC = (eState->GetLNCType() & condition->GetValue()) != 0;
            return containsLNC == (condition->GetComparator() ==
                objects::TokuseiCondition::Comparator_t::EQUALS);
        }
        break;
    case TokuseiConditionType::GENDER:
        // Entity is the specified gender
        {
            int32_t gender = (int32_t)objects::MiNPCBasicData::Gender_t::NONE;

            auto devilData = eState->GetDevilData();
            switch(eState->GetEntityType())
            {
            case objects::EntityStateObject::EntityType_t::CHARACTER:
                gender = (int32_t)std::dynamic_pointer_cast<CharacterState>(eState)
                    ->GetEntity()->GetGender();
                break;
            case objects::EntityStateObject::EntityType_t::PARTNER_DEMON:
            case objects::EntityStateObject::EntityType_t::ENEMY:
                if(devilData)
                {
                    gender = (int32_t)devilData->GetBasic()->GetGender();
                }
                break;
            default:
                return false;
            }

            return Compare(gender, condition, false);
        }
        break;
    case TokuseiConditionType::STATUS_ACTIVE:
        // Entity currently has the specified status effect active
        if(numericCompare)
        {
            return false;
        }
        else
        {
            auto statusEffects = eState->GetStatusEffects();

            bool exists = statusEffects.find((uint32_t)condition->GetValue())
                != statusEffects.end();
            return exists == (condition->GetComparator() ==
                objects::TokuseiCondition::Comparator_t::EQUALS);
        }
        break;
    case TokuseiConditionType::GAME_TIME:
    case TokuseiConditionType::MOON_PHASE:
        // Toggled by the server, just return true or false
        // (Always disable for non-player entities)
        return mTimedTokusei[tokuseiID] && eState->GetEntityType() !=
            objects::EntityStateObject::EntityType_t::ENEMY;
    case TokuseiConditionType::PARTY_DEMON_TYPE:
        // Entity is in a party with the specified demon type currently summoned
        if(numericCompare)
        {
            return false;
        }
        else
        {
            std::set<uint32_t> demonIDs;

            auto state = ClientState::GetEntityClientState(eState->GetEntityID(), false);
            auto party = state ? state->GetParty() : nullptr;
            if(party)
            {
                auto zone = eState->GetZone();
                for(auto memberID : party->GetMemberIDs())
                {
                    auto state2 = (memberID != state->GetWorldCID())
                        ? ClientState::GetEntityClientState(memberID, true)
                        : state;
                    if(state2)
                    {
                        auto dState2 = state2->GetDemonState();
                        auto demon = dState2->GetEntity();
                        if(dState2->GetZone() == zone && demon)
                        {
                            demonIDs.insert(demon->GetType());
                        }
                    }
                }
            }

            bool exists = demonIDs.find((uint32_t)condition->GetValue())
                != demonIDs.end();
            return exists == (condition->GetComparator() ==
                objects::TokuseiCondition::Comparator_t::EQUALS);
        }
        break;
    case TokuseiConditionType::SKILL_STATE:
        // Only valid during skill processing
        return false;
        break;
    case TokuseiConditionType::PARTNER_TYPE:
    case TokuseiConditionType::PARTNER_FAMILY:
    case TokuseiConditionType::PARTNER_RACE:
    case TokuseiConditionType::PARTNER_FAMILIARITY:
        isPartnerCondition = true;
        break;
    default:
        break;
    }

    if(!isPartnerCondition)
    {
        return false;
    }

    std::shared_ptr<objects::Demon> partner;
    std::shared_ptr<objects::MiDevilData> demonData;
    auto state = ClientState::GetEntityClientState(eState->GetEntityID(), false);
    if(state && state->GetCharacterState() == eState && state->GetDemonState()->Ready())
    {
        auto dState = state->GetDemonState();
        partner = dState->GetEntity();
        demonData = dState->GetDevilData();
    }

    if(partner == nullptr)
    {
        return false;
    }

    if(condition->GetType() == TokuseiConditionType::PARTNER_FAMILIARITY)
    {
        return Compare((int32_t)partner->GetFamiliarity(), condition, true);
    }

    if(!demonData || numericCompare)
    {
        return false;
    }

    int32_t partnerValue = 0;
    switch(condition->GetType())
    {
    case TokuseiConditionType::PARTNER_TYPE:
        // Partner matches the specified demon type
        partnerValue = (int32_t)partner->GetType();
        break;
    case TokuseiConditionType::PARTNER_FAMILY:
        partnerValue = (int32_t)demonData->GetCategory()->GetFamily();
        break;
    case TokuseiConditionType::PARTNER_RACE:
        partnerValue = (int32_t)demonData->GetCategory()->GetRace();
        break;
    default:
        break;
    }

    return Compare(partnerValue, condition, false);
}

double TokuseiManager::CalculateAttributeValue(ActiveEntityState* eState, int32_t value,
    int32_t base, const std::shared_ptr<objects::TokuseiAttributes>& attributes,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    double result = value * 1.0;

    if(!calcState)
    {
        calcState = eState->GetCalculatedState();
    }

    if(attributes)
    {
        uint8_t precision = attributes->GetPrecision();
        if(precision > 0)
        {
            result = (double)(result / std::pow(10.0, precision));
        }

        int32_t multValue = attributes->GetMultiplierValue();
        switch(attributes->GetMultiplier())
        {
        case objects::TokuseiAttributes::Multiplier_t::LEVEL:
        case objects::TokuseiAttributes::Multiplier_t::BASE_AND_LEVEL:
            // Multiply the value by the entities level
            {
                bool includeBase = attributes->GetMultiplier() !=
                    objects::TokuseiAttributes::Multiplier_t::LEVEL;

                auto cs = eState->GetCoreStats();
                if(cs)
                {
                    result = (double)(result * (double)cs->GetLevel());
                    if(includeBase)
                    {
                        result = (double)(result * (double)base);
                    }
                }
            }
            break;
        case objects::TokuseiAttributes::Multiplier_t::EXPERTISE:
            // Multiply the value by the current rank of the supplied expertise
            if(eState->GetEntityType() == objects::EntityStateObject::EntityType_t::CHARACTER)
            {
                auto character = ((CharacterState*)eState)->GetEntity();
                if(character)
                {
                    auto exp = character->GetExpertises((size_t)multValue);
                    int32_t points = exp ? exp->GetPoints() : 0;
                    double currentRank = (double)floorl((float)points * 0.0001f);

                    result = (double)(result * currentRank);
                }
                else
                {
                    result = 0.0;
                }
            }
            else
            {
                result = 0.0;
            }
            break;
        case objects::TokuseiAttributes::Multiplier_t::CORRECT_TABLE:
        case objects::TokuseiAttributes::Multiplier_t::CORRECT_TABLE_DIVIDE:
            // Multiply (or divide) the value by a correct table value
            {
                bool divide = attributes->GetMultiplier() !=
                    objects::TokuseiAttributes::Multiplier_t::CORRECT_TABLE;

                int16_t val = calcState->GetCorrectTbl((size_t)multValue);
                if(divide)
                {
                    result = val != 0 ? (double)(result / (double)val) : 0;
                }
                else
                {
                    result = (double)(result * (double)val);
                }
            }
            break;
        case objects::TokuseiAttributes::Multiplier_t::PARTY_SIZE:
            // Multiply the value by the number of party members in the zone
            {
                uint8_t memberCount = 0;

                auto state = ClientState::GetEntityClientState(eState->GetEntityID(), false);
                auto party = state ? state->GetParty() : nullptr;
                if(party)
                {
                    auto zone = eState->GetZone();
                    for(auto memberID : party->GetMemberIDs())
                    {
                        auto state2 = (memberID != state->GetWorldCID())
                            ? ClientState::GetEntityClientState(memberID, true)
                            : state;
                        auto cState = state2 ? state2->GetCharacterState() : nullptr;
                        if(cState && cState->GetZone() == zone)
                        {
                            memberCount++;
                        }
                    }
                }

                result = (double)(result * (double)memberCount);
            }
            break;
        case objects::TokuseiAttributes::Multiplier_t::DEMON_BOOK_DIVIDE:
            // Divide the value by the number of unique entries in the compendium
            {
                auto state = ClientState::GetEntityClientState(eState->GetEntityID(), false);
                auto dState = state ? state->GetDemonState() : nullptr;

                result = dState
                    ? (result * floor((double)dState->GetCompendiumCount() / (double)multValue))
                    : 0.0;
            }
            break;
        default:
            result = 0.0;
            break;
        }
    }

    return result;
}

double TokuseiManager::GetAspectSum(const std::shared_ptr<ActiveEntityState>& eState,
    TokuseiAspectType type, std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    double sum = 0.0;
    if(eState)
    {
        auto definitionManager = mServer.lock()->GetDefinitionManager();

        if(!calcState)
        {
            calcState = eState->GetCalculatedState();
        }

        auto effectiveTokusei = calcState->GetEffectiveTokuseiFinal();
        for(auto pair : effectiveTokusei)
        {
            auto tokusei = definitionManager->GetTokuseiData(pair.first);

            double val = 0.0;
            for(auto aspect : tokusei->GetAspects())
            {
                if(aspect->GetType() == type)
                {
                    val = CalculateAttributeValue(eState.get(), aspect->GetValue(),
                        0, aspect->GetAttributes());

                    for(uint16_t i = 0; i < pair.second; i++)
                    {
                        sum += val;
                    }
                }
            }
        }
    }

    return sum;
}

std::unordered_map<int32_t, double> TokuseiManager::GetAspectMap(
    const std::shared_ptr<ActiveEntityState>& eState, TokuseiAspectType type,
    std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    std::set<int32_t> allKeys;
    return GetAspectMap(eState, type, allKeys, calcState);
}

std::unordered_map<int32_t, double> TokuseiManager::GetAspectMap(
    const std::shared_ptr<ActiveEntityState>& eState, TokuseiAspectType type,
    std::set<int32_t> validKeys, std::shared_ptr<objects::CalculatedEntityState> calcState)
{
    std::unordered_map<int32_t, double> result;
    for(int32_t key : validKeys)
    {
        result[key] = 0.0;
    }

    if(eState)
    {
        auto definitionManager = mServer.lock()->GetDefinitionManager();

        if(!calcState)
        {
            calcState = eState->GetCalculatedState();
        }

        auto effectiveTokusei = calcState->GetEffectiveTokuseiFinal();
        for(auto pair : effectiveTokusei)
        {
            auto tokusei = definitionManager->GetTokuseiData(pair.first);

            double mod = 0.0;
            for(auto aspect : tokusei->GetAspects())
            {
                if(aspect->GetType() == type)
                {
                    int32_t value = aspect->GetValue();
                    if(validKeys.size() != 0 &&
                        validKeys.find(value) == validKeys.end()) continue;

                    auto it = result.find(value);
                    if(it == result.end())
                    {
                        result[value] = 0.0;
                        it = result.find(value);
                    }

                    mod = CalculateAttributeValue(eState.get(), aspect->GetModifier(),
                        0, aspect->GetAttributes());

                    for(uint16_t i = 0; i < pair.second; i++)
                    {
                        it->second += mod;
                    }
                }
            }
        }
    }

    return result;
}

std::list<double> TokuseiManager::GetAspectValueList(const std::shared_ptr<
    ActiveEntityState>& eState, TokuseiAspectType type, std::shared_ptr<
    objects::CalculatedEntityState> calcState)
{
    std::list<double> result;
    if(eState)
    {
        auto definitionManager = mServer.lock()->GetDefinitionManager();

        if(!calcState)
        {
            calcState = eState->GetCalculatedState();
        }

        auto effectiveTokusei = calcState->GetEffectiveTokuseiFinal();
        for(auto pair : effectiveTokusei)
        {
            auto tokusei = definitionManager->GetTokuseiData(pair.first);

            double val = 0.0;
            for(auto aspect : tokusei->GetAspects())
            {
                if(aspect->GetType() == type)
                {
                    val = CalculateAttributeValue(eState.get(), aspect->GetValue(),
                        0, aspect->GetAttributes());

                    for(uint16_t i = 0; i < pair.second; i++)
                    {
                        result.push_back(val);
                    }
                }
            }
        }
    }

    return result;
}

void TokuseiManager::RecalcTimedTokusei(WorldClock& clock)
{
    std::set<int32_t> updateCIDs;
    {
        std::set<int32_t> toggled;

        auto definitionManager = mServer.lock()->GetDefinitionManager();
        std::lock_guard<std::mutex> lock(mTimeLock);
        for(auto& tPair : mTimedTokusei)
        {
            bool setActive = true;
            bool isActive = tPair.second;

            auto tokusei = definitionManager->GetTokuseiData(tPair.first);
            for(auto condition : tokusei->GetConditions())
            {
                switch(condition->GetType())
                {
                case TokuseiConditionType::GAME_TIME:
                    // The current game time matches the specified time and
                    // comparison
                    {
                        setActive &= Compare((int32_t)(clock.Hour * 100 +
                            (int32_t)clock.Min), condition, true);
                    }
                    break;
                case TokuseiConditionType::MOON_PHASE:
                    // The current moon phase matches the specified phase and
                    // comparison
                    {
                        setActive &= Compare((int32_t)clock.MoonPhase,
                            condition, true);
                    }
                    break;
                default:
                    break;
                }

                if(!setActive)
                {
                    break;
                }
            }

            if(isActive != setActive)
            {
                tPair.second = setActive;
                toggled.insert(tPair.first);
            }
        }

        for(int32_t tokuseiID : toggled)
        {
            for(auto& pair : mTimedTokuseiEntities)
            {
                if(pair.second.find(tokuseiID) != pair.second.end())
                {
                    updateCIDs.insert(pair.first);
                }
            }
        }
    }

    // Now update each player with the tokusei
    for(int32_t worldCID : updateCIDs)
    {
        auto state = ClientState::GetEntityClientState(worldCID, true);
        if(state)
        {
            Recalculate(state->GetCharacterState(), true);
        }
    }
}

void TokuseiManager::RemoveTrackingEntities(int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mTimeLock);
    mTimedTokuseiEntities.erase(worldCID);
}

bool TokuseiManager::BuildWorldClockTime(std::shared_ptr<
    objects::TokuseiCondition> condition, WorldClockTime& time)
{
    switch(condition->GetType())
    {
    case objects::TokuseiCondition::Type_t::GAME_TIME:
        if(time.Min != -1 || time.Hour != -1)
        {
            // Do not set twice
            return false;
        }
        else if(condition->GetValue() < 0 ||
            condition->GetValue() > 2400 ||
            condition->GetValue() % 100 >= 60)
        {
            // Make sure its in the valid range
            return false;
        }
        else
        {
            time.Hour = (int8_t)floor(condition->GetValue() * 0.01);
            time.Min = (int8_t)(condition->GetValue() % 100);
            return true;
        }
        break;
    case objects::TokuseiCondition::Type_t::MOON_PHASE:
        if(time.MoonPhase != -1)
        {
            // Do not set twice
            return false;
        }
        else if(condition->GetValue() < 0 ||
            condition->GetValue() >= 16)
        {
            // Make sure its in the valid range
            return false;
        }
        else
        {
            time.MoonPhase = (int8_t)condition->GetValue();
            return true;
        }
        break;
    default:
        return false;
    }
}

bool TokuseiManager::Compare(int32_t value, std::shared_ptr<
    objects::TokuseiCondition> condition, bool numericCompare) const
{
    return Compare(value, condition->GetValue(), condition, numericCompare);
}

bool TokuseiManager::Compare(int32_t value1, int32_t value2, std::shared_ptr<
    objects::TokuseiCondition> condition, bool numericCompare) const
{
    switch(condition->GetComparator())
    {
    case objects::TokuseiCondition::Comparator_t::EQUALS:
        return value1 == value2;
        break;
    case objects::TokuseiCondition::Comparator_t::NOT_EQUAL:
        return value1 != value2;
        break;
    case objects::TokuseiCondition::Comparator_t::LTE:
        return numericCompare && value1 <= value2;
        break;
    case objects::TokuseiCondition::Comparator_t::GTE:
        return numericCompare && value1 >= value2;
        break;
    default:
        break;
    }

    return false;
}
