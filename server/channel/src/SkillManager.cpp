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
#include <MiBattleDamageData.h>
#include <MiCastBasicData.h>
#include <MiCastData.h>
#include <MiConditionData.h>
#include <MiCostTbl.h>
#include <MiDamageData.h>
#include <MiDevilData.h>
#include <MiNPCBasicData.h>
#include <MiSkillData.h>
#include <MiSummonData.h>
#include <MiTargetData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

const uint32_t SKILL_SUMMON_DEMON = 0x00001648;
const uint32_t SKILL_STORE_DEMON = 0x00001649;
const uint32_t SKILL_EQUIP_ITEM = 0x00001654;

const uint8_t DAMAGE_TYPE_GENERIC = 0;
const uint8_t DAMAGE_TYPE_HEALING = 1;
const uint8_t DAMAGE_TYPE_NONE = 2;
const uint8_t DAMAGE_TYPE_MISS = 3;
const uint8_t DAMAGE_TYPE_COMBAT = 4;
const uint8_t DAMAGE_TYPE_DRAIN = 5;

const uint16_t FLAG1_LETHAL = 1;
const uint16_t FLAG1_CRITICAL = 1 << 6;
const uint16_t FLAG1_WEAKPOINT = 1 << 7;
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
    bool AilmentDamaged = false;
    int32_t AilmentDamageAmount = 0;
    uint16_t DamageFlags2 = 0;
    int32_t TechnicalDamage = 0;
    int32_t PursuitDamage = 0;
};

bool CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
    uint32_t hpCost, uint32_t mpCost, SkillTargetResult& target,
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

    if(chargeTime == 0)
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

    // Check conditions
    /// @todo: check more than just costs
    uint32_t hpCost = 0, mpCost = 0;
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
                        hpCost += num;
                    }
                    break;
                case objects::MiCostTbl::Type_t::MP:
                    if(percentCost)
                    {
                        mpCostPercent = (uint16_t)(mpCostPercent + num);
                    }
                    else
                    {
                        mpCost += num;
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

    hpCost = (uint32_t)(hpCost + ceil(((float)hpCostPercent * 0.01f) *
        (float)sourceState->GetMaxHP()));
    mpCost = (uint32_t)(mpCost + ceil(((float)mpCostPercent * 0.01f) *
        (float)sourceState->GetMaxMP()));

    auto sourceStats = sourceState->GetCoreStats();
    bool canPay = ((hpCost == 0) || hpCost < (uint32_t)sourceStats->GetHP()) &&
        ((mpCost == 0) || mpCost < (uint32_t)sourceStats->GetMP());
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
    sourceStats->SetHP(static_cast<int16_t>(sourceStats->GetHP() - (uint16_t)hpCost));
    sourceStats->SetMP(static_cast<int16_t>(sourceStats->GetMP() - (uint16_t)mpCost));
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
        default:
            return ExecuteNormalSkill(client, sourceState->GetEntityID(), activated,
                hpCost, mpCost);
    }

    if(success)
    {
        FinalizeSkillExecution(client, sourceState->GetEntityID(), activated, skillData, 0, 0);
        SendCompleteSkill(client, sourceState->GetEntityID(), activated);
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

    client->SendPacket(reply);
}

bool SkillManager::ExecuteNormalSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
    uint32_t hpCost, uint32_t mpCost)
{
    auto state = client->GetClientState();

    auto source = state->GetEntityState(sourceEntityID);
    if(nullptr == source)
    {
        return false;
    }

    auto server = mServer.lock();
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
                auto zone = zoneManager->GetZoneInstance(client);
                if(nullptr == zone)
                {
                    LOG_ERROR("Skill activation attempted outside of a zone.\n");
                    return false;
                }

                auto targetEntityID = (int32_t)activated->GetTargetObjectID();
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
    for(SkillTargetResult& target : targetResults)
    {
        if(battleDamageData->GetFormula() !=
            objects::MiBattleDamageData::Formula_t::NONE)
        {
            if(!CalculateDamage(source, hpCost, mpCost, target, battleDamageData))
            {
                LOG_ERROR(libcomp::String("Damage failed to calculate: %1\n")
                    .Arg(skillID));
                return false;
            }

            hasBattleDamage = true;
        }
    }

    // Apply calculation results
    for(auto target : targetResults)
    {
        if(hasBattleDamage)
        {
            int32_t hpAdjust = target.TechnicalDamage;
            int32_t mpAdjust = 0;

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
                            hpAdjust = (int32_t)(hpAdjust + val);
                        }
                        else
                        {
                            mpAdjust = (int32_t)(mpAdjust + val);
                        }
                        break;
                    default:
                        hpAdjust = (int32_t)(mpAdjust + val);
                        break;
                }
            }

            auto targetStats = target.EntityState->GetCoreStats();
            hpAdjust = static_cast<int32_t>(targetStats->GetHP() - hpAdjust);
            mpAdjust = static_cast<int32_t>(targetStats->GetMP() - mpAdjust);

            // Adjust for more than max or less than zero
            if(hpAdjust <= 0)
            {
                hpAdjust = 0;

                if(targetStats->GetHP() > 0)
                {
                    target.DamageFlags1 |= FLAG1_LETHAL;
                }
            }
            else if(hpAdjust > target.EntityState->GetMaxHP())
            {
                hpAdjust = target.EntityState->GetMaxHP();
            }
            
            if(mpAdjust < 0)
            {
                mpAdjust = 0;
            }
            else if(mpAdjust > target.EntityState->GetMaxMP())
            {
                mpAdjust = target.EntityState->GetMaxMP();
            }

            targetStats->SetHP(static_cast<int16_t>(hpAdjust));
            targetStats->SetMP(static_cast<int16_t>(mpAdjust));
        }

        target.EntityState->RecalculateStats(definitionManager);
    }

    FinalizeSkillExecution(client, sourceEntityID, activated, skillData, hpCost, mpCost);
    SendCompleteSkill(client, sourceEntityID, activated);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(skillID);
    reply.WriteS8((int8_t)activated->GetActivationID());

    reply.WriteU32Little((uint32_t)targetResults.size());
    for(auto target : targetResults)
    {
        reply.WriteS32Little(target.EntityState->GetEntityID());
        reply.WriteS32Little(abs(target.Damage1));
        reply.WriteU8(target.Damage1Type);
        reply.WriteS32Little(abs(target.Damage2));
        reply.WriteU8(target.Damage2Type);
        reply.WriteU16Little(target.DamageFlags1);

        reply.WriteU8(static_cast<uint8_t>(target.AilmentDamaged ? 1 : 0));
        reply.WriteS32Little(abs(target.AilmentDamageAmount));

        //Knockback location info?
        reply.WriteFloat(0);
        reply.WriteFloat(0);
        reply.WriteFloat(0);
        reply.WriteFloat(0);
        reply.WriteFloat(0);
        reply.WriteFloat(0);
        reply.WriteU8(0);

        uint32_t effectAddCount = 0;
        uint32_t effectCancelCount = 0;
        reply.WriteU32Little(effectAddCount);
        reply.WriteU32Little(effectCancelCount);
        for(uint32_t i = 0; i < effectAddCount; i++)
        {
            /// @todo: Added status effects
            reply.WriteU32Little(0);
            reply.WriteS32Little(0);
            reply.WriteU8(0);
        }
        
        for(uint32_t i = 0; i < effectCancelCount; i++)
        {
            /// @todo: Cancelled status effects
            reply.WriteU32Little(0);
        }

        reply.WriteU16Little(target.DamageFlags2);
        reply.WriteS32Little(target.TechnicalDamage);
        reply.WriteS32Little(target.PursuitDamage);
    }

    client->SendPacket(reply);

    return true;
}

bool CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
    uint32_t hpCost, uint32_t mpCost, SkillTargetResult& target,
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
                        (int16_t)hpCost));
                target.Damage2 = CalculateDamage_Percent(
                    damageData->GetModifier2(), target.Damage2Type,
                    static_cast<int16_t>(source->GetCoreStats()->GetMP() +
                        (int16_t)mpCost));
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
    std::shared_ptr<objects::MiSkillData> skillData, uint32_t hpCost, uint32_t mpCost)
{
    SendExecuteSkill(client, sourceEntityID, activated, skillData, hpCost, mpCost);

    mServer.lock()->GetCharacterManager()->UpdateExpertise(client, activated->GetSkillID());
}

bool SkillManager::EquipItem(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated)
{
    (void)sourceEntityID;

    auto itemID = activated->GetTargetObjectID();
    if(itemID == -1)
    {
        LOG_ERROR(libcomp::String("Invalid item specified to equip: %1\n")
            .Arg(itemID));
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
    if(demonID == -1)
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
    if(demonID == -1)
    {
        LOG_ERROR(libcomp::String("Invalid demon specified to store: %1\n")
            .Arg(demonID));
        return false;
    }

    mServer.lock()->GetCharacterManager()->StoreDemon(client);

    return true;
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

    client->SendPacket(reply);
}

void SkillManager::SendExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<objects::MiSkillData> skillData, uint32_t hpCost, uint32_t mpCost)
{
    auto state = client->GetClientState();
    auto conditionData = skillData->GetCondition();

    auto currentTime = state->ToClientTime(ChannelServer::GetServerTime());
    auto cooldownTime = currentTime + ((float)conditionData->GetCooldownTime() * 0.001f);
    /// @todo: figure out how to properly use lockOutTime
    auto lockOutTime = cooldownTime;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_EXECUTING);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(activated->GetSkillID());
    reply.WriteS8((int8_t)activated->GetActivationID());
    reply.WriteS32Little(0);   //Unknown
    reply.WriteFloat(cooldownTime);
    reply.WriteFloat(lockOutTime);
    reply.WriteU32Little(hpCost);
    reply.WriteU32Little(mpCost);
    reply.WriteU8(0);   //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteFloat(0);    //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown

    client->SendPacket(reply);
}

void SkillManager::SendCompleteSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_COMPLETED);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(activated->GetSkillID());
    reply.WriteS8((int8_t)activated->GetActivationID());
    reply.WriteFloat(0.0f);   //Unknown
    reply.WriteU8(1);   //Unknown
    reply.WriteFloat(300.0f);   //Run speed
    reply.WriteU8(0);   //Unknown

    client->SendPacket(reply);
}
