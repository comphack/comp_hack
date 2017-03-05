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

struct SkillTargetReport
{
    std::shared_ptr<objects::EntityStats> EntityStats;
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

bool CalculateDamage(SkillTargetReport& target,
    std::shared_ptr<objects::MiBattleDamageData> damageData);

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
        if(!ExecuteSkill(client, sourceState, activated, targetObjectID))
        {
            SendFailure(client, sourceEntityID, skillID);
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
        skillID = activated->GetSkillID();
    }

    if(!success || !ExecuteSkill(client, sourceState, activated, targetObjectID))
    {
        SendFailure(client, sourceEntityID, skillID);
    }

    return success;
}

bool SkillManager::ExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<ActiveEntityState> sourceState,
    std::shared_ptr<objects::ActivatedAbility> activated, int64_t targetObjectID)
{
    sourceState->SetActivatedAbility(nullptr);

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

    hpCost = (uint32_t)(hpCost + ceil(((float)hpCostPercent * 0.01f) * (float)sourceState->GetMaxHP()));
    mpCost = (uint32_t)(mpCost + ceil(((float)mpCostPercent * 0.01f) * (float)sourceState->GetMaxMP()));

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
            false, targetObjectID);
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

const uint8_t FORMULA_ADJUSTED_HEAL = 9;
const uint8_t FORMULA_STATIC_HEAL = 10;
const uint8_t FORMULA_PERCENTAGE_HEAL = 11;

bool CalculateDamage(SkillTargetReport& target,
    std::shared_ptr<objects::MiBattleDamageData> damageData)
{
    uint16_t mod1 = damageData->GetModifier1();
    uint16_t mod2 = damageData->GetModifier2();
    switch(damageData->GetFormula())
    {
        case FORMULA_STATIC_HEAL:
            {
                if(mod1 != 0)
                {
                    target.Damage1 = (int32_t)(mod1 * -1.0);
                    target.Damage1Type = DAMAGE_TYPE_HEALING;
                }

                if(mod2 != 0)
                {
                    target.Damage2 = (int32_t)(mod2 * -1.0);
                    target.Damage2Type = DAMAGE_TYPE_HEALING;
                }
            }
            break;
        case FORMULA_PERCENTAGE_HEAL:
            {
                if(mod1 != 0)
                {
                    auto maxHP = target.EntityState->GetMaxHP();
                    auto hp = target.EntityStats->GetHP();
                    target.Damage1 = (int32_t)ceil((float)maxHP * ((float)mod1 * 0.01f));
                    if((target.Damage1 + hp) > maxHP)
                    {
                        target.Damage1 = maxHP - hp;
                    }

                    target.Damage1 = (int32_t)(target.Damage1 * -1.0);
                    target.Damage1Type = DAMAGE_TYPE_HEALING;
                }

                if(mod2 != 0)
                {
                    auto maxMP = target.EntityState->GetMaxMP();
                    auto mp = target.EntityStats->GetMP();
                    target.Damage2 = (int32_t)ceil((float)maxMP * ((float)mod2 * 0.01f));
                    if((target.Damage2 + mp) > maxMP)
                    {
                        target.Damage2 = maxMP - mp;
                    }

                    target.Damage2 = (int32_t)(target.Damage2 * -1.0);
                    target.Damage2Type = DAMAGE_TYPE_HEALING;
                }
            }
            break;
        /// @todo: implement more
        default:
            LOG_ERROR(libcomp::String("Unknown damage formula type encountered: %1\n")
                .Arg(damageData->GetFormula()));
            return false;
    }

    return true;
}

bool SkillManager::ExecuteNormalSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
    uint32_t hpCost, uint32_t mpCost)
{
    auto state = client->GetClientState();

    auto sourceState = state->GetEntityState(sourceEntityID);
    if(nullptr == sourceState)
    {
        return false;
    }
    auto sourceStats = sourceState->GetCoreStats();

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillID = activated->GetSkillID();
    auto skillData = definitionManager->GetSkillData(skillID);
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    // Gather targets
    std::list<SkillTargetReport> targetReports;
    switch(skillData->GetTarget()->GetType())
    {
        case 0: // None, target source (if applicable)
            {
                SkillTargetReport targetReport;
                targetReport.EntityStats = sourceStats;
                targetReport.EntityState = sourceState;
                targetReports.push_back(targetReport);
            }
            break;
        case 3: // Character
            {
                SkillTargetReport targetReport;
                targetReport.EntityStats = character->GetCoreStats().Get();
                targetReport.EntityState = cState;
                targetReports.push_back(targetReport);
            }
            break;
        /// @todo: implement more
        default:
            LOG_ERROR(libcomp::String("Unknown target type encountered: %1\n")
                .Arg(skillData->GetTarget()->GetType()));
            return false;
    }

    // Run calculations
    bool hasBattleDamage = false;
    auto battleDamageData = skillData->GetDamage()->GetBattleDamage();
    if(battleDamageData->GetFormula() != 0)
    {
        for(SkillTargetReport& target : targetReports)
        {
            if(!CalculateDamage(target, battleDamageData))
            {
                LOG_ERROR(libcomp::String("Damage failed to calculate: %1\n")
                    .Arg(skillID));
                return false;
            }
        }
        hasBattleDamage = true;
    }

    // Apply calculation results
    for(auto target : targetReports)
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

            target.EntityStats->SetHP(
                static_cast<int16_t>(target.EntityStats->GetHP() - hpAdjust));
            target.EntityStats->SetMP(
                static_cast<int16_t>(target.EntityStats->GetHP() - mpAdjust));

            if(target.EntityStats->GetHP() == 0)
            {
                target.DamageFlags1 |= FLAG1_LETHAL;
            }
        }

        auto charState = std::dynamic_pointer_cast<CharacterState>(target.EntityState);
        auto demonState = std::dynamic_pointer_cast<DemonState>(target.EntityState);
        if(nullptr != charState)
        {
            charState->RecalculateStats(definitionManager);
        }
        else if(nullptr != demonState)
        {
            demonState->RecalculateStats(definitionManager);
        }
        /// @todo: add NPCs/enemies etc
    }

    FinalizeSkillExecution(client, sourceEntityID, activated, skillData, hpCost, mpCost);
    SendCompleteSkill(client, sourceEntityID, activated);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
    reply.WriteS32Little(sourceEntityID);
    reply.WriteU32Little(skillID);
    reply.WriteS8((int8_t)activated->GetActivationID());

    reply.WriteU32Little((uint32_t)targetReports.size());
    for(auto target : targetReports)
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
