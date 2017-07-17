/**
 * @file server/channel/src/SkillManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages skill execution and logic.
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

#include "SkillManager.h"

// libcomp Includes
#include <Constants.h>
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ActivatedAbility.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiAddStatusTbl.h>
#include <MiBattleDamageData.h>
#include <MiCastBasicData.h>
#include <MiCastData.h>
#include <MiConditionData.h>
#include <MiCostTbl.h>
#include <MiDamageData.h>
#include <MiDevilData.h>
#include <MiDischargeData.h>
#include <MiDoTDamageData.h>
#include <MiEffectData.h>
#include <MiNPCBasicData.h>
#include <MiSkillData.h>
#include <MiStatusData.h>
#include <MiStatusBasicData.h>
#include <MiSummonData.h>
#include <MiTargetData.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"
#include "Zone.h"

using namespace channel;

/// @todo: figure out how many unique logic skills exist (and posibly script intead)
const uint32_t SKILL_SUMMON_DEMON = 0x00001648;
const uint32_t SKILL_STORE_DEMON = 0x00001649;
const uint32_t SKILL_EQUIP_ITEM = 0x00001654;
const uint32_t SKILL_TRAESTO = 0x00001405;
const uint32_t SKILL_TRAESTO_STONE = 0x0000280D;

const uint8_t DAMAGE_TYPE_GENERIC = 0;
const uint8_t DAMAGE_TYPE_HEALING = 1;
const uint8_t DAMAGE_TYPE_NONE = 2;
const uint8_t DAMAGE_TYPE_MISS = 3;
const uint8_t DAMAGE_TYPE_COMBAT = 4;
const uint8_t DAMAGE_TYPE_DRAIN = 5;

const uint16_t FLAG1_LETHAL = 1;
const uint16_t FLAG1_CRITICAL = 1 << 6;
const uint16_t FLAG1_WEAKPOINT = 1 << 7;
const uint16_t FLAG1_REVIVAL = 1 << 9;
const uint16_t FLAG1_REFLECT = 1 << 11; //Only displayed with DAMAGE_TYPE_NONE
const uint16_t FLAG1_BLOCK = 1 << 12;   //Only dsiplayed with DAMAGE_TYPE_NONE
const uint16_t FLAG1_PROTECT = 1 << 15;

const uint16_t FLAG2_LIMIT_BREAK = 1 << 5;
const uint16_t FLAG2_IMPOSSIBLE = 1 << 6;
const uint16_t FLAG2_BARRIER = 1 << 7;
const uint16_t FLAG2_INTENSIVE_BREAK = 1 << 8;
const uint16_t FLAG2_INSTANT_DEATH = 1 << 9;

struct SkillTargetResult
{
    std::shared_ptr<ActiveEntityState> EntityState;
    int32_t Damage1 = 0;
    uint8_t Damage1Type = DAMAGE_TYPE_NONE;
    int32_t Damage2 = 0;
    uint8_t Damage2Type = DAMAGE_TYPE_NONE;
    uint16_t DamageFlags1 = 0;
    uint8_t AilmentDamageType = 0;
    int32_t AilmentDamage = 0;
    uint16_t DamageFlags2 = 0;
    int32_t TechnicalDamage = 0;
    int32_t PursuitDamage = 0;
    bool Knockback = false;
    AddStatusEffectMap AddedStatuses;
    std::set<uint32_t> CancelledStatuses;
};

bool CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
    int16_t hpCost, int16_t mpCost, SkillTargetResult& target,
    std::shared_ptr<objects::MiBattleDamageData> damageData);

int32_t CalculateDamage_Normal(uint16_t mod, uint8_t& damageType,
    uint16_t off, uint16_t def, uint8_t critLevel);
int32_t CalculateDamage_Static(uint16_t mod, uint8_t& damageType);
int32_t CalculateDamage_Percent(uint16_t mod, uint8_t& damageType,
    int16_t current);
int32_t CalculateDamage_MaxPercent(uint16_t mod, uint8_t& damageType,
    int16_t max);

SkillManager::SkillManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

SkillManager::~SkillManager()
{
}

bool SkillManager::ActivateSkill(const std::shared_ptr<ChannelClientConnection> client,
    uint32_t skillID, int32_t sourceEntityID, int64_t targetObjectID)
{
    auto state = client->GetClientState();
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetSkillData(skillID);
    if(nullptr == def)
    {
        SendFailure(client, sourceEntityID, skillID);
        return false;
    }

    auto cast = def->GetCast();
    auto chargeTime = cast->GetBasic()->GetChargeTime();

    auto activationID = state->GetNextActivatedAbilityID();
    auto activatedTime = server->GetServerTime();
    // Charge time is in milliseconds, convert to microseconds
    auto chargedTime = activatedTime + (chargeTime * 1000);

    auto activated = std::shared_ptr<objects::ActivatedAbility>(
        new objects::ActivatedAbility);
    activated->SetSkillID(skillID);
    activated->SetTargetObjectID(targetObjectID);
    activated->SetActivationID(activationID);
    activated->SetActivationTime(activatedTime);
    activated->SetChargedTime(chargedTime);

    auto sourceState = state->GetEntityState(sourceEntityID);
    if(nullptr == sourceState)
    {
        SendFailure(client, sourceEntityID, skillID);
        return false;
    }
    auto sourceStats = sourceState->GetCoreStats();
    sourceState->SetActivatedAbility(activated);

    SendChargeSkill(client, sourceEntityID, activated);

    /// @todo: figure out what actually consitutes an instant cast and
    /// a client side delay
    bool delay = skillID == SKILL_TRAESTO || skillID == SKILL_TRAESTO_STONE;

    if(chargeTime == 0 && !delay)
    {
        // Cast instantly
        if(!ExecuteSkill(client, sourceState, activated))
        {
            SendFailure(client, sourceEntityID, skillID);
            sourceState->SetActivatedAbility(nullptr);
            return false;
        }
    }

    return true;
}

bool SkillManager::ExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, uint8_t activationID, int64_t targetObjectID)
{
    auto state = client->GetClientState();
    auto sourceState = state->GetEntityState(sourceEntityID);

    bool success = sourceState != nullptr;

    uint32_t skillID = 0;
    auto activated = success ? sourceState->GetActivatedAbility() : nullptr;
    if(nullptr == activated || activationID != activated->GetActivationID())
    {
        LOG_ERROR(libcomp::String("Unknown activation ID encountered: %1\n")
            .Arg(activationID));
        success = false;
    }
    else
    {
        activated->SetTargetObjectID(targetObjectID);
        skillID = activated->GetSkillID();
    }

    if(!success || !ExecuteSkill(client, sourceState, activated))
    {
        SendFailure(client, sourceEntityID, skillID);
    }

    return success;
}

bool SkillManager::ExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<ActiveEntityState> sourceState,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillID = activated->GetSkillID();
    auto skillData = definitionManager->GetSkillData(skillID);

    if(nullptr == skillData)
    {
        LOG_ERROR(libcomp::String("Unknown skill ID encountered: %1\n")
            .Arg(skillID));
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    
    // Check targets
    if(skillData->GetTarget()->GetType() == objects::MiTargetData::Type_t::DEAD_ALLY)
    {
        auto damageFormula = skillData->GetDamage()->GetBattleDamage()->GetFormula();
        bool isRevive = damageFormula == objects::MiBattleDamageData::Formula_t::HEAL_NORMAL
            || damageFormula == objects::MiBattleDamageData::Formula_t::HEAL_STATIC
            || damageFormula == objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT;

        // If the target is a character and they have not accepted revival, stop here
        auto targetEntityID = (int32_t)activated->GetTargetObjectID();
        auto targetClientState = ClientState::GetEntityClientState(targetEntityID);
        if(isRevive && (!targetClientState ||
            (!targetClientState->GetAcceptRevival() &&
            targetClientState->GetCharacterState()->GetEntityID() == targetEntityID)))
        {
            return false;
        }
    }

    // Check costs
    int16_t hpCost = 0, mpCost = 0;
    uint16_t hpCostPercent = 0, mpCostPercent = 0;
    std::unordered_map<uint32_t, uint16_t> itemCosts;
    if(skillID == SKILL_SUMMON_DEMON)
    {
        /*auto demon = std::dynamic_pointer_cast<objects::Demon>(
            libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(targetObjectID)));
        if(demon == nullptr)
        {
            return false;
        }

        uint32_t demonType = demon->GetType();
        auto demonStats = demon->GetCoreStats().Get();
        auto demonData = definitionManager->GetDevilData(demonType);

        int16_t characterLNC = character->GetLNC();
        int16_t demonLNC = demonData->GetBasic()->GetLNC();
        uint8_t magModifier = demonData->GetSummonData()->GetMagModifier();*/

        /// @todo: calculate MAG

        itemCosts[800] = 1;
    }
    else
    {
        auto costs = skillData->GetCondition()->GetCosts();
        for(auto cost : costs)
        {
            auto num = cost->GetCost();
            bool percentCost = cost->GetNumType() == objects::MiCostTbl::NumType_t::PERCENT;
            switch(cost->GetType())
            {
                case objects::MiCostTbl::Type_t::HP:
                    if(percentCost)
                    {
                        hpCostPercent = (uint16_t)(hpCostPercent + num);
                    }
                    else
                    {
                        hpCost = (int16_t)(hpCost + num);
                    }
                    break;
                case objects::MiCostTbl::Type_t::MP:
                    if(percentCost)
                    {
                        mpCostPercent = (uint16_t)(mpCostPercent + num);
                    }
                    else
                    {
                        mpCost = (int16_t)(mpCost + num);
                    }
                    break;
                case objects::MiCostTbl::Type_t::ITEM:
                    if(percentCost)
                    {
                        LOG_ERROR("Item percent cost encountered.\n");
                        return false;
                    }
                    else
                    {
                        auto itemID = cost->GetItem();
                        if(itemCosts.find(itemID) == itemCosts.end())
                        {
                            itemCosts[itemID] = 0;
                        }
                        itemCosts[itemID] = (uint16_t)(itemCosts[itemID] + num);
                    }
                    break;
                default:
                    LOG_ERROR(libcomp::String("Unsupported cost type encountered: %1\n")
                        .Arg((uint8_t)cost->GetType()));
                    return false;
            }
        }
    }

    hpCost = (int16_t)(hpCost + ceil(((float)hpCostPercent * 0.01f) *
        (float)sourceState->GetMaxHP()));
    mpCost = (int16_t)(mpCost + ceil(((float)mpCostPercent * 0.01f) *
        (float)sourceState->GetMaxMP()));

    auto sourceStats = sourceState->GetCoreStats();
    bool canPay = ((hpCost == 0) || hpCost < sourceStats->GetHP()) &&
        ((mpCost == 0) || mpCost < sourceStats->GetMP());
    auto characterManager = server->GetCharacterManager();
    for(auto itemCost : itemCosts)
    {
        auto existingItems = characterManager->GetExistingItems(character, itemCost.first);
        uint16_t itemCount = 0;
        for(auto item : existingItems)
        {
            itemCount = (uint16_t)(itemCount + item->GetStackSize());
        }

        if(itemCount < itemCost.second)
        {
            canPay = false;
            break;
        }
    }

    // Handle costs that can't be paid as expected errors
    if(!canPay)
    {
        return false;
    }

    // Pay the costs
    if(hpCost > 0 || mpCost > 0)
    {
        sourceState->SetHPMP((int16_t)-hpCost, (int16_t)-mpCost, true);
    }

    for(auto itemCost : itemCosts)
    {
        characterManager->AddRemoveItem(client, itemCost.first, itemCost.second,
            false, activated->GetTargetObjectID());
    }

    // Execute the skill
    bool success = false;
    switch(skillID)
    {
        case SKILL_EQUIP_ITEM:
            success = EquipItem(client, sourceState->GetEntityID(), activated);
            break;
        case SKILL_SUMMON_DEMON:
            success = SummonDemon(client, sourceState->GetEntityID(), activated);
            break;
        case SKILL_STORE_DEMON:
            success = StoreDemon(client, sourceState->GetEntityID(), activated);
            break;
        case SKILL_TRAESTO:
        case SKILL_TRAESTO_STONE:
            success = Traesto(client, sourceState->GetEntityID(), activated);
            break;
        default:
            return ExecuteNormalSkill(client, sourceState->GetEntityID(), activated,
                hpCost, mpCost);
    }

    characterManager->CancelStatusEffects(client, EFFECT_CANCEL_SKILL);

    if(success)
    {
        FinalizeSkillExecution(client, sourceState->GetEntityID(), activated, skillData, 0, 0);
    }

    SendCompleteSkill(client, sourceState->GetEntityID(), activated, !success);
    sourceState->SetActivatedAbility(nullptr);

    return success;
}

bool SkillManager::CancelSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, uint8_t activationID)
{
    auto state = client->GetClientState();
    auto sourceState = state->GetEntityState(sourceEntityID);

    bool success = sourceState != nullptr;

    auto activated = success ? sourceState->GetActivatedAbility() : nullptr;
    if(nullptr == activated || activationID != activated->GetActivationID())
    {
        LOG_ERROR(libcomp::String("Unknown activation ID encountered: %1\n")
            .Arg(activationID));
        success = false;
    }

    if(success)
    {
        SendCompleteSkill(client, sourceEntityID, activated, true);
        sourceState->SetActivatedAbility(nullptr);
    }

    return success;
}

void SkillManager::SendFailure(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, uint32_t skillID)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_FAILED);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(skillID);
    reply.WriteS8(-1);  //Unknown
    reply.WriteU8(0);  //Unknown
    reply.WriteU8(0);  //Unknown
    reply.WriteS32Little(-1);  //Unknown

    mServer.lock()->GetZoneManager()->BroadcastPacket(client, reply);
}

bool SkillManager::ExecuteNormalSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
    int16_t hpCost, int16_t mpCost)
{
    auto state = client->GetClientState();

    auto source = state->GetEntityState(sourceEntityID);
    if(nullptr == source)
    {
        return false;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto zoneManager = server->GetZoneManager();
    auto skillID = activated->GetSkillID();
    auto skillData = definitionManager->GetSkillData(skillID);
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    // Gather targets
    std::list<SkillTargetResult> targetResults;
    switch(skillData->GetTarget()->GetType())
    {
        case objects::MiTargetData::Type_t::NONE:
            {
                /// @todo: Do not include the source when MiEffectiveRangeData
                /// is being used
                SkillTargetResult target;
                target.EntityState = source;
                targetResults.push_back(target);
            }
            break;
        case objects::MiTargetData::Type_t::ALLY:
        case objects::MiTargetData::Type_t::DEAD_ALLY:
        case objects::MiTargetData::Type_t::PARTNER:
        case objects::MiTargetData::Type_t::PARTY:
        case objects::MiTargetData::Type_t::ENEMY:
        case objects::MiTargetData::Type_t::DEAD_PARTNER:
        case objects::MiTargetData::Type_t::OTHER_PLAYER:
        case objects::MiTargetData::Type_t::OTHER_DEMON:
        case objects::MiTargetData::Type_t::ALLY_PLAYER:
        case objects::MiTargetData::Type_t::ALLY_DEMON:
        case objects::MiTargetData::Type_t::PLAYER:
            {
                auto targetEntityID = (int32_t)activated->GetTargetObjectID();

                if(targetEntityID == -1)
                {
                    //No target
                    break;
                }

                auto zone = zoneManager->GetZoneInstance(client);
                if(nullptr == zone)
                {
                    LOG_ERROR("Skill activation attempted outside of a zone.\n");
                    return false;
                }

                auto targetEntity = zone->GetActiveEntityState(targetEntityID);
                if(nullptr == targetEntity || !targetEntity->Ready())
                {
                    LOG_ERROR(libcomp::String("Invalid target ID encountered: %1\n")
                        .Arg(targetEntityID));
                    return false;
                }

                SkillTargetResult target;
                target.EntityState = targetEntity;
                targetResults.push_back(target);

                activated->SetEntityTargeted(true);
            }
            break;
        case objects::MiTargetData::Type_t::OBJECT:
            LOG_ERROR("Skill object targets are not currently supported: %1\n");
            return false;
        default:
            LOG_ERROR(libcomp::String("Unknown target type encountered: %1\n")
                .Arg((uint8_t)skillData->GetTarget()->GetType()));
            return false;
    }

    /// @todo: Add MiEffectiveRangeData targets

    // Run calculations
    bool hasBattleDamage = false;
    auto battleDamageData = skillData->GetDamage()->GetBattleDamage();
    auto addStatuses = skillData->GetDamage()->GetAddStatuses();
    for(SkillTargetResult& target : targetResults)
    {
        if(battleDamageData->GetFormula() !=
            objects::MiBattleDamageData::Formula_t::NONE)
        {
            /// @todo: implement knockback properly
            target.Knockback = true;

            if(!CalculateDamage(source, hpCost, mpCost, target, battleDamageData))
            {
                LOG_ERROR(libcomp::String("Damage failed to calculate: %1\n")
                    .Arg(skillID));
                return false;
            }

            hasBattleDamage = true;
        }

        // Determine which status effects to apply
        for(auto addStatus : addStatuses)
        {
            if(addStatus->GetOnKnockback() && !target.Knockback) continue;

            uint16_t successRate = addStatus->GetSuccessRate();
            if(successRate >= 100 || (rand() % 99) <= successRate)
            {
                int8_t minStack = addStatus->GetMinStack();
                int8_t maxStack = addStatus->GetMaxStack();

                // Sanity check
                if(minStack > maxStack) continue;

                int8_t stack = (int8_t)(minStack + (rand() % (maxStack - minStack)));
                if(stack == 0) continue;

                target.AddedStatuses[addStatus->GetStatusID()] =
                    std::pair<uint8_t, bool>(stack, addStatus->GetIsReplace());

                // Check for status T-Damage to apply at the end of the skill
                auto statusDef = definitionManager->GetStatusData(
                    addStatus->GetStatusID());
                auto basicDef = statusDef->GetBasic();
                if(basicDef->GetStackType() == 1 && basicDef->GetApplicationLogic() == 0)
                {
                    auto tDamage = statusDef->GetEffect()->GetDamage();
                    if(tDamage->GetHPDamage() > 0)
                    {
                        /// @todo: transform properly
                        target.AilmentDamage += tDamage->GetHPDamage();
                    }
                }
            }
        }
    }

    std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
    if(hpCost > 0 || mpCost > 0)
    {
        displayStateModified.insert(source);
    }

    // Apply calculation results, keeping track of entities that may
    // need to update the world with their modified state
    std::set<std::shared_ptr<ActiveEntityState>> revived;
    std::set<std::shared_ptr<ActiveEntityState>> killed;
    std::unordered_map<std::shared_ptr<ActiveEntityState>, uint8_t> cancellations;
    for(SkillTargetResult& target : targetResults)
    {
        cancellations[target.EntityState] = target.Knockback
            ? EFFECT_CANCEL_KNOCKBACK : 0;
        if(hasBattleDamage)
        {
            int32_t hpDamage = target.TechnicalDamage + target.AilmentDamage;
            int32_t mpDamage = 0;

            for(int i = 0; i < 2; i++)
            {
                bool hpMode = i == 0;
                int32_t val = i == 0 ? target.Damage1 : target.Damage2;
                uint8_t type = i == 0 ? target.Damage1Type : target.Damage2Type;

                switch(type)
                {
                    case DAMAGE_TYPE_HEALING:
                    case DAMAGE_TYPE_DRAIN:
                        if(hpMode)
                        {
                            hpDamage = (int32_t)(hpDamage + val);
                        }
                        else
                        {
                            mpDamage = (int32_t)(mpDamage + val);
                        }
                        break;
                    default:
                        if(hpMode)
                        {
                            hpDamage = (int32_t)(hpDamage + val);
                        }
                        break;
                }
            }

            bool targetAlive = target.EntityState->IsAlive();

            int16_t hpAdjusted, mpAdjusted;
            if(target.EntityState->SetHPMP((int16_t)-hpDamage, (int16_t)-mpDamage, true,
                true, hpAdjusted, mpAdjusted))
            {
                // Changed from alive to dead or vice versa
                if(target.EntityState->GetEntityType() ==
                    objects::EntityStateObject::EntityType_t::CHARACTER)
                {
                    // Reset accept revival
                    auto targetClientState = ClientState::GetEntityClientState(
                        target.EntityState->GetEntityID());
                    targetClientState->SetAcceptRevival(false);
                }

                if(targetAlive)
                {
                    target.DamageFlags1 |= FLAG1_LETHAL;
                    killed.insert(target.EntityState);
                }
                else
                {
                    target.DamageFlags1 |= FLAG1_REVIVAL;
                    revived.insert(target.EntityState);
                }
            }
            
            if(hpAdjusted <= 0)
            {
                cancellations[target.EntityState] |= EFFECT_CANCEL_HIT |
                    EFFECT_CANCEL_DAMAGE;
            }

            switch(target.EntityState->GetEntityType())
            {
                case objects::EntityStateObject::EntityType_t::CHARACTER:
                case objects::EntityStateObject::EntityType_t::PARTNER_DEMON:
                    displayStateModified.insert(target.EntityState);
                    break;
                default:
                    break;
            }
        }

        characterManager->RecalculateStats(client, target.EntityState->GetEntityID());
    }

    for(auto cancelPair : cancellations)
    {
        if(cancelPair.second)
        {
            cancelPair.first->CancelStatusEffects(cancelPair.second);
        }
    }

    characterManager->CancelStatusEffects(client, EFFECT_CANCEL_SKILL);

    // Now that previous effects have been cancelled, add the new ones
    uint32_t effectTime = (uint32_t)std::time(0);
    for(SkillTargetResult& target : targetResults)
    {
        if(target.AddedStatuses.size() > 0)
        {
            auto removed = target.EntityState->AddStatusEffects(
                target.AddedStatuses, definitionManager, effectTime, false);
            for(auto r : removed)
            {
                target.CancelledStatuses.insert(r);
            }
        }
    }

    FinalizeSkillExecution(client, sourceEntityID, activated, skillData, hpCost, mpCost);
    SendCompleteSkill(client, sourceEntityID, activated, false);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(skillID);
    reply.WriteS8((int8_t)activated->GetActivationID());

    reply.WriteU32Little((uint32_t)targetResults.size());
    for(SkillTargetResult& target : targetResults)
    {
        reply.WriteS32Little(target.EntityState->GetEntityID());
        reply.WriteS32Little(abs(target.Damage1));
        reply.WriteU8(target.Damage1Type);
        reply.WriteS32Little(abs(target.Damage2));
        reply.WriteU8(target.Damage2Type);
        reply.WriteU16Little(target.DamageFlags1);

        reply.WriteU8(target.AilmentDamageType);
        reply.WriteS32Little(abs(target.AilmentDamage));

        //Knockback location info?
        reply.WriteFloat(0);
        reply.WriteFloat(0);
        reply.WriteFloat(0);

        /// @todo: Apply hit timing values properly
        reply.WriteFloat(0);
        reply.WriteFloat(0);
        reply.WriteFloat(0);

        reply.WriteU8(0);   // Unknown

        std::list<std::shared_ptr<objects::StatusEffect>> addedStatuses;
        std::set<uint32_t> cancelledStatuses;
        if(target.AddedStatuses.size() > 0)
        {
            // Make sure the added statuses didn't get removed/re-added
            // already for some reason
            auto effects = target.EntityState->GetStatusEffects();
            for(auto added : target.AddedStatuses)
            {
                if(effects.find(added.first) != effects.end())
                {
                    addedStatuses.push_back(effects[added.first]);
                }
            }

            for(auto cancelled : target.CancelledStatuses)
            {
                if(effects.find(cancelled) == effects.end())
                {
                    cancelledStatuses.insert(cancelled);
                }
            }
        }

        reply.WriteU32Little((uint32_t)addedStatuses.size());
        reply.WriteU32Little((uint32_t)cancelledStatuses.size());

        for(auto effect : addedStatuses)
        {
            reply.WriteU32Little(effect->GetEffect());
            reply.WriteS32Little((int32_t)effect->GetExpiration());
            reply.WriteU8(effect->GetStack());
        }

        for(auto cancelled : cancelledStatuses)
        {
            reply.WriteU32Little(cancelled);
        }

        reply.WriteU16Little(target.DamageFlags2);
        reply.WriteS32Little(target.TechnicalDamage);
        reply.WriteS32Little(target.PursuitDamage);
    }

    zoneManager->BroadcastPacket(client, reply);

    if(revived.size() > 0)
    {
        for(auto entity : revived)
        {
            characterManager->SendEntityRevival(client, entity, 6, true);
        }
    }

    if(killed.size() > 0)
    {
        for(auto entity : killed)
        {
            if(entity->GetEntityType() ==
                objects::EntityStateObject::EntityType_t::PARTNER_DEMON)
            {
                // If a partner demon was killed, decrease familiarity
                auto demonClient = server->GetManagerConnection()->
                    GetEntityClient(entity->GetEntityID());
                if(!demonClient) continue;

                /// @todo: verify this value more
                characterManager->UpdateFamiliarity(demonClient, -100, true);
            }
        }
    }

    /// @todo: Transform enemies killed into bodies

    if(displayStateModified.size() > 0)
    {
        characterManager->UpdateWorldDisplayState(displayStateModified);
    }

    return true;
}

bool CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
    int16_t hpCost, int16_t mpCost, SkillTargetResult& target,
    std::shared_ptr<objects::MiBattleDamageData> damageData)
{
    bool isHeal = false;

    auto formula = damageData->GetFormula();
    switch(formula)
    {
        case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
        case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
        case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT:
            isHeal = true;
            break;
        default:
            break;
    }

    switch(formula)
    {
        case objects::MiBattleDamageData::Formula_t::NONE:
            return true;
        case objects::MiBattleDamageData::Formula_t::DMG_NORMAL:
        case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
            {
                /// @todo: pull offense/defense values properly
                uint16_t off = (uint16_t)(isHeal ? source->GetSUPPORT()
                    : source->GetCLSR());
                uint16_t def = (uint16_t)(isHeal ? 0
                    : target.EntityState->GetPDEF());

                /// @todo: implement critical chances etc
                uint8_t critLevel = 0;

                target.Damage1 = CalculateDamage_Normal(
                    damageData->GetModifier1(), target.Damage1Type,
                    off, def, critLevel);
                target.Damage2 = CalculateDamage_Normal(
                    damageData->GetModifier2(), target.Damage2Type,
                    off, def, critLevel);

                switch(critLevel)
                {
                    case 1:
                        target.DamageFlags1 |= FLAG1_CRITICAL;
                        break;
                    case 2:
                        target.DamageFlags2 |= FLAG2_LIMIT_BREAK;
                        break;
                    default:
                        break;
                }
            }
            break;
        case objects::MiBattleDamageData::Formula_t::DMG_STATIC:
        case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
            {
                target.Damage1 = CalculateDamage_Static(
                    damageData->GetModifier1(), target.Damage1Type);
                target.Damage2 = CalculateDamage_Static(
                    damageData->GetModifier2(), target.Damage2Type);
            }
            break;
        case objects::MiBattleDamageData::Formula_t::DMG_PERCENT:
            {
                target.Damage1 = CalculateDamage_Percent(
                    damageData->GetModifier1(), target.Damage1Type,
                    target.EntityState->GetCoreStats()->GetHP());
                target.Damage2 = CalculateDamage_Percent(
                    damageData->GetModifier2(), target.Damage2Type,
                    target.EntityState->GetCoreStats()->GetMP());
            }
            break;
        case objects::MiBattleDamageData::Formula_t::DMG_SOURCE_PERCENT:
            {
                //Calculate using pre-cost values
                target.Damage1 = CalculateDamage_Percent(
                    damageData->GetModifier1(), target.Damage1Type,
                    static_cast<int16_t>(source->GetCoreStats()->GetHP() +
                        hpCost));
                target.Damage2 = CalculateDamage_Percent(
                    damageData->GetModifier2(), target.Damage2Type,
                    static_cast<int16_t>(source->GetCoreStats()->GetMP() +
                        mpCost));
            }
            break;
        case objects::MiBattleDamageData::Formula_t::DMG_MAX_PERCENT:
        case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT:
            {
                target.Damage1 = CalculateDamage_MaxPercent(
                    damageData->GetModifier1(), target.Damage1Type,
                    target.EntityState->GetMaxHP());
                target.Damage2 = CalculateDamage_MaxPercent(
                    damageData->GetModifier2(), target.Damage2Type,
                    target.EntityState->GetMaxMP());
            }
            break;
        /// @todo: figure out these two
        case objects::MiBattleDamageData::Formula_t::UNKNOWN_5:
        case objects::MiBattleDamageData::Formula_t::UNKNOWN_6:
        default:
            LOG_ERROR(libcomp::String("Unknown damage formula type encountered: %1\n")
                .Arg((uint8_t)formula));
            return false;
    }

    if(isHeal)
    {
        // If the damage was actually a heal, invert the amount and change the type
        target.Damage1 = target.Damage1 * -1;
        target.Damage2 = target.Damage2 * -1;
        target.Damage1Type = (target.Damage1Type == DAMAGE_TYPE_COMBAT) ?
            DAMAGE_TYPE_HEALING : target.Damage1Type;
        target.Damage2Type = (target.Damage2Type == DAMAGE_TYPE_COMBAT) ?
            DAMAGE_TYPE_HEALING : target.Damage2Type;
    }

    return true;
}

int32_t CalculateDamage_Normal(uint16_t mod, uint8_t& damageType,
    uint16_t off, uint16_t def, uint8_t critLevel)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        float scale = 0.f;
        switch(critLevel)
        {
            case 1:	    // Critical hit
                scale = 1.2f;
                break;
            case 2:	    // Limit Break
                scale = 1.5f;
                break;
            default:	// Normal hit, 80%-99% damage
                scale = 0.8f + ((float)(rand() % 19) * 0.01f);
                break;
        }

        // Start with offense stat * modifier/100
        float calc = (float)off * ((float)mod * 0.01f);

        // Add the expertise rank
        //calc = calc + (float)exp;

        // Substract the enemy defense, unless its a critical or limit break
        calc = calc - (float)(critLevel > 0 ? 0 : def);

        // Scale the current value by the critical, limit break or min to
        // max damage factor
        calc = calc * scale;

        // Multiply by 1 - resistance/100
        calc = calc * (float)(1 - 0/* @todo: Resistence */ * 0.01f);

        // Multiply by 1 + affinity/100
        calc = calc * (float)(1 - (0/* @todo: Affinity */ * 0.01f));

        // Multiply by 1 + remaining power boosts/100
        calc = calc * (float)(1 - (0/* @todo: Action+Racial+Skill Power */ * 0.01f));

        /// @todo: there is more to this calculation

        amount = (int32_t)ceil(calc);
        damageType = DAMAGE_TYPE_COMBAT;
    }

    return amount;
}

int32_t CalculateDamage_Static(uint16_t mod, uint8_t& damageType)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        amount = (int32_t)mod;
        damageType = DAMAGE_TYPE_COMBAT;
    }

    return amount;
}

int32_t CalculateDamage_Percent(uint16_t mod, uint8_t& damageType,
    int16_t current)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        amount = (int32_t)ceil((float)current * ((float)mod * 0.01f));
        damageType = DAMAGE_TYPE_COMBAT;
    }

    return amount;
}

int32_t CalculateDamage_MaxPercent(uint16_t mod, uint8_t& damageType,
    int16_t max)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        amount = (int32_t)ceil((float)max * ((float)mod * 0.01f));
        damageType = DAMAGE_TYPE_COMBAT;
    }

    return amount;
}

void SkillManager::FinalizeSkillExecution(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<objects::MiSkillData> skillData, int16_t hpCost, int16_t mpCost)
{
    SendExecuteSkill(client, sourceEntityID, activated, skillData, hpCost, mpCost);

    mServer.lock()->GetCharacterManager()->UpdateExpertise(client, activated->GetSkillID());
}

bool SkillManager::EquipItem(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated)
{
    (void)sourceEntityID;

    auto itemID = activated->GetTargetObjectID();
    if(itemID <= 0)
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->EquipItem(client, itemID);

    return true;
}

bool SkillManager::SummonDemon(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated)
{
    (void)sourceEntityID;

    auto demonID = activated->GetTargetObjectID();
    if(demonID <= 0)
    {
        LOG_ERROR(libcomp::String("Invalid demon specified to summon: %1\n")
            .Arg(demonID));
        return false;
    }

    mServer.lock()->GetCharacterManager()->SummonDemon(client, demonID);

    return true;
}

bool SkillManager::StoreDemon(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated)
{
    (void)sourceEntityID;

    auto demonID = activated->GetTargetObjectID();
    if(demonID <= 0)
    {
        LOG_ERROR(libcomp::String("Invalid demon specified to store: %1\n")
            .Arg(demonID));
        return false;
    }

    mServer.lock()->GetCharacterManager()->StoreDemon(client);

    return true;
}

bool SkillManager::Traesto(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated)
{
    (void)sourceEntityID;
    (void)activated;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto zoneID = character->GetHomepointZone();
    auto xCoord = character->GetHomepointX();
    auto yCoord = character->GetHomepointY();

    if(zoneID == 0)
    {
        LOG_ERROR(libcomp::String("Character with no homepoint set attempted to use"
            " Traesto: %1\n").Arg(character->GetName()));
        return false;
    }

    return mServer.lock()->GetZoneManager()->EnterZone(client, zoneID, xCoord, yCoord, 0, true);
}

void SkillManager::SendChargeSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto state = client->GetClientState();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_CHARGING);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(activated->GetSkillID());
    reply.WriteS8((int8_t)activated->GetActivationID());
    reply.WriteFloat(state->ToClientTime(activated->GetChargedTime()));
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteFloat(300.0f);   //Run speed during charge
    reply.WriteFloat(300.0f);   //Run speed after charge

    mServer.lock()->GetZoneManager()->BroadcastPacket(client, reply);
}

void SkillManager::SendExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<objects::MiSkillData> skillData, int16_t hpCost, int16_t mpCost)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto conditionData = skillData->GetCondition();
    auto dischargeData = skillData->GetDischarge();

    int32_t targetedEntityID = activated->GetEntityTargeted() ?
        (int32_t)activated->GetTargetObjectID() : sourceEntityID;

    auto currentTime = state->ToClientTime(ChannelServer::GetServerTime());
    auto cooldownTime = currentTime + ((float)conditionData->GetCooldownTime() * 0.001f);
    auto lockOutTime = currentTime + ((float)dischargeData->GetStiffness() * 0.001f);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_EXECUTING);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(activated->GetSkillID());
    reply.WriteS8((int8_t)activated->GetActivationID());
    reply.WriteS32Little(targetedEntityID);
    reply.WriteFloat(cooldownTime);
    reply.WriteFloat(lockOutTime);
    reply.WriteU32Little((uint32_t)hpCost);
    reply.WriteU32Little((uint32_t)mpCost);
    reply.WriteU8(0);   //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0xFF);   //Unknown

    mServer.lock()->GetZoneManager()->BroadcastPacket(client, reply);
}

void SkillManager::SendCompleteSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated, bool cancelled)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_COMPLETED);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(activated->GetSkillID());
    reply.WriteS8((int8_t)activated->GetActivationID());
    reply.WriteFloat(0);   //Unknown
    reply.WriteU8(1);   //Unknown
    reply.WriteFloat(300.0f);   //Run speed
    reply.WriteU8(cancelled ? 1 : 0);

    mServer.lock()->GetZoneManager()->BroadcastPacket(client, reply);
}
