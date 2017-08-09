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
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ActivatedAbility.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiAddStatusTbl.h>
#include <MiBattleDamageData.h>
#include <MiCancelData.h>
#include <MiCastBasicData.h>
#include <MiCastData.h>
#include <MiCategoryData.h>
#include <MiConditionData.h>
#include <MiCostTbl.h>
#include <MiDamageData.h>
#include <MiDevilData.h>
#include <MiDischargeData.h>
#include <MiDoTDamageData.h>
#include <MiEffectData.h>
#include <MiEffectiveRangeData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiKnockBackData.h>
#include <MiNPCBasicData.h>
#include <MiSkillBasicData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiStatusData.h>
#include <MiStatusBasicData.h>
#include <MiSummonData.h>
#include <MiTargetData.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"
#include "Zone.h"

using namespace channel;

const uint8_t DAMAGE_TYPE_GENERIC = 0;
const uint8_t DAMAGE_TYPE_HEALING = 1;
const uint8_t DAMAGE_TYPE_NONE = 2;
const uint8_t DAMAGE_TYPE_MISS = 3;
const uint8_t DAMAGE_TYPE_DRAIN = 5;

const uint16_t FLAG1_LETHAL = 1;
const uint16_t FLAG1_CRITICAL = 1 << 6;
const uint16_t FLAG1_WEAKPOINT = 1 << 7;
const uint16_t FLAG1_KNOCKBACK = 1 << 8;
const uint16_t FLAG1_REVIVAL = 1 << 9;
const uint16_t FLAG1_ABSORB = 1 << 10;  //Only displayed with DAMAGE_TYPE_HEALING
const uint16_t FLAG1_REFLECT = 1 << 11; //Only displayed with DAMAGE_TYPE_NONE
const uint16_t FLAG1_BLOCK = 1 << 12;   //Only dsiplayed with DAMAGE_TYPE_NONE
const uint16_t FLAG1_PROTECT = 1 << 15;

const uint16_t FLAG2_LIMIT_BREAK = 1 << 5;
const uint16_t FLAG2_IMPOSSIBLE = 1 << 6;
const uint16_t FLAG2_BARRIER = 1 << 7;
const uint16_t FLAG2_INTENSIVE_BREAK = 1 << 8;
const uint16_t FLAG2_INSTANT_DEATH = 1 << 9;

const uint8_t RES_OFFSET = (uint8_t)CorrectTbl::RES_WEAPON - 1;
const uint8_t BOOST_OFFSET = (uint8_t)CorrectTbl::BOOST_SLASH - 2;
const uint8_t NRA_OFFSET = (uint8_t)CorrectTbl::NRA_WEAPON - 1;
/*const uint8_t DAMAGE_TAKEN_OFFSET = (uint8_t)((uint8_t)CorrectTbl::RATE_CLSR -
    (uint8_t)CorrectTbl::RATE_CLSR_TAKEN);*/

class channel::ProcessingSkill
{
public:
    std::shared_ptr<objects::MiSkillData> Definition;
    uint8_t BaseAffinity;
    uint8_t EffectiveAffinity;
    uint8_t EffectiveDependencyType;
};

class channel::SkillTargetResult
{
public:
    std::shared_ptr<ActiveEntityState> EntityState;
    bool PrimaryTarget = false;
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
    AddStatusEffectMap AddedStatuses;
    std::set<uint32_t> CancelledStatuses;
};

SkillManager::SkillManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
    mSkillFunctions[SVR_CONST.SKILL_CLAN_FORM] = &SkillManager::SpecialSkill;
    mSkillFunctions[SVR_CONST.SKILL_EQUIP_ITEM] = &SkillManager::EquipItem;
    mSkillFunctions[SVR_CONST.SKILL_SUMMON_DEMON] = &SkillManager::SummonDemon;
    mSkillFunctions[SVR_CONST.SKILL_STORE_DEMON] = &SkillManager::StoreDemon;
    mSkillFunctions[SVR_CONST.SKILL_TRAESTO] = &SkillManager::Traesto;

    // Make sure anything not set is not pulled in to the mapping
    mSkillFunctions.erase(0);
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

    auto sourceState = state->GetEntityState(sourceEntityID);
    if(nullptr == sourceState)
    {
        SendFailure(client, sourceEntityID, skillID);
        return false;
    }

    auto cast = def->GetCast();
    auto chargeTime = cast->GetBasic()->GetChargeTime();

    auto activatedTime = server->GetServerTime();
    // Charge time is in milliseconds, convert to microseconds
    auto chargedTime = activatedTime + (chargeTime * 1000);

    auto activated = std::shared_ptr<objects::ActivatedAbility>(
        new objects::ActivatedAbility);
    activated->SetSkillID(skillID);
    activated->SetSourceEntity(sourceState);
    activated->SetTargetObjectID(targetObjectID);
    activated->SetActivationTime(activatedTime);
    activated->SetChargedTime(chargedTime);

    auto activationID = state->GetNextActivatedAbilityID();
    activated->SetActivationID(activationID);

    sourceState->SetActivatedAbility(activated);

    SendChargeSkill(client, activated);

    uint8_t activationType = def->GetBasic()->GetActivationType();
    bool executeNow = (activationType == 3 || activationType == 4)
        && chargeTime == 0;
    if(executeNow)
    {
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
    uint16_t functionID = skillData->GetDamage()->GetFunctionID();
    uint8_t skillCategory = skillData->GetCommon()->GetCategory()->GetMainCategory();

    if(skillCategory == 0)
    {
        return false;
    }

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

    // Verify the target now
    switch(skillData->GetTarget()->GetType())
    {
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

            auto zone = sourceState->GetZone();
            if(nullptr == zone)
            {
                LOG_ERROR("Skill activation attempted outside of a zone.\n");
                return false;
            }

            auto targetEntity = zone->GetActiveEntity(targetEntityID);
            if(nullptr == targetEntity || !targetEntity->Ready())
            {
                LOG_ERROR(libcomp::String("Invalid target ID encountered: %1\n")
                    .Arg(targetEntityID));
                return false;
            }

            activated->SetEntityTargeted(true);
        }
        break;
    default:
        break;
    }

    auto source = std::dynamic_pointer_cast<ActiveEntityState>(
        activated->GetSourceEntity());

    // Check costs and pay costs (skip for switch deactivation)
    if(skillCategory == 1 || (skillCategory == 2 &&
        !source->ActiveSwitchSkillsContains(skillID)))
    {
        int16_t hpCost = 0, mpCost = 0;
        uint16_t hpCostPercent = 0, mpCostPercent = 0;
        uint16_t bulletCost = 0;
        std::unordered_map<uint32_t, uint16_t> itemCosts;
        if(functionID == SVR_CONST.SKILL_SUMMON_DEMON)
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
                case objects::MiCostTbl::Type_t::BULLET:
                    if(percentCost)
                    {
                        LOG_ERROR("Bullet percent cost encountered.\n");
                        return false;
                    }
                    else
                    {
                        bulletCost = (uint16_t)(bulletCost + num);
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
            auto existingItems = characterManager->GetExistingItems(character,
                itemCost.first);
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

        std::pair<uint32_t, int64_t> bulletIDs;
        if(bulletCost > 0)
        {
            auto bullets = character->GetEquippedItems((size_t)
                objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BULLETS);
            if(!bullets || bullets->GetStackSize() < bulletCost)
            {
                canPay = false;
            }
            else
            {
                bulletIDs = std::pair<uint32_t, int64_t>(bullets->GetType(),
                    state->GetObjectID(bullets->GetUUID()));
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
            activated->SetHPCost(hpCost);
            activated->SetMPCost(mpCost);

            std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
            displayStateModified.insert(sourceState);
            characterManager->UpdateWorldDisplayState(displayStateModified);
        }

        for(auto itemCost : itemCosts)
        {
            characterManager->AddRemoveItem(client, itemCost.first, itemCost.second,
                false, activated->GetTargetObjectID());
        }

        if(bulletCost > 0)
        {
            characterManager->AddRemoveItem(client, bulletIDs.first, bulletCost,
                false, bulletIDs.second);
        }
    }

    // Execute the skill
    auto fIter = mSkillFunctions.find(functionID);
    if(fIter == mSkillFunctions.end())
    {
        switch(skillCategory)
        {
        case 1:
            // Active
            return ExecuteNormalSkill(client, activated);
        case 2:
            // Switch
            return ToggleSwitchSkill(client, activated);
        case 0:
            // Passive, shouldn't happen
        default:
            return false;
            break;
        }
    }

    bool success = fIter->second(*this, client, activated);
    if(success)
    {
        FinalizeSkillExecution(client, activated, skillData);
    }
    else
    {
        SendCompleteSkill(client, activated, true);
        sourceState->SetActivatedAbility(nullptr);
    }

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
        SendCompleteSkill(client, activated, true);
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
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(
        activated->GetSourceEntity());
    if(!source)
    {
        return false;
    }

    auto zone = source->GetZone();
    if(!zone)
    {
        return false;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillID = activated->GetSkillID();
    auto skillData = definitionManager->GetSkillData(skillID);

    uint32_t projectileSpeed = skillData->GetDischarge()->GetProjectileSpeed();
    if(projectileSpeed == 0)
    {
        // Non-projectile skill, calculate damage and effects immediately
        FinalizeSkillExecution(client, activated, skillData);
        return ProcessSkillResult(activated);
    }
    else
    {
        // Check for the target
        auto targetEntityID = (int32_t)activated->GetTargetObjectID();
        auto target = zone->GetActiveEntity(targetEntityID);

        // If it isn't valid at this point, fail the skill
        if(!target)
        {
            return false;
        }

        // Determine time from projectile speed and distance
        uint64_t now = server->GetServerTime();

        source->RefreshCurrentPosition(now);
        target->RefreshCurrentPosition(now);

        float distance = source->GetDistance(target->GetCurrentX(),
            target->GetCurrentY());
        uint16_t maxTargetRange = (uint16_t)(400 + (skillData->GetTarget()->GetRange() * 10));
        if((float)maxTargetRange < distance)
        {
            // Out of range, fail execution
            return false;
        }

        // Complete the skill, calculate damage and effects when the projectile hits
        FinalizeSkillExecution(client, activated, skillData);

        /// @todo: figure out activate to projectile spawned delay for more accuracy
        // Projectile speed is measured in how many 10ths of a unit the projectile will
        // traverse per millisecond
        uint64_t addMicro = (uint64_t)((double)distance / (projectileSpeed * 10)) * 1000000;
        uint64_t processTime = now + addMicro;

        server->ScheduleWork(processTime, [](const std::shared_ptr<ChannelServer> pServer,
            const std::shared_ptr<objects::ActivatedAbility> pActivated)
            {
                pServer->GetSkillManager()->ProcessSkillResult(pActivated);
            }, server, activated);
    }

    return true;
}

bool SkillManager::ProcessSkillResult(std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());
    auto zone = source->GetZone();
    if(!zone)
    {
        return false;
    }

    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto skillID = activated->GetSkillID();
    auto skillData = definitionManager->GetSkillData(skillID);

    ProcessingSkill skill;
    skill.Definition = skillData;
    skill.EffectiveDependencyType = skillData->GetBasic()->GetDependencyType();
    skill.BaseAffinity = skill.EffectiveAffinity = skillData->GetCommon()->GetAffinity();

    // Calculate effective dependency and affinity types if "weapon" is specified
    if(skill.EffectiveDependencyType == 4 || skill.BaseAffinity == 1)
    {
        auto cState = std::dynamic_pointer_cast<CharacterState>(source);
        auto weapon = cState ? cState->GetEntity()->GetEquippedItems((size_t)
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON).Get() : nullptr;
        auto weaponDef = weapon ? mServer.lock()->GetDefinitionManager()
            ->GetItemData(weapon->GetType()) : nullptr;

        // If at any point the type cannot be determined,
        // default to strike, close range (ex: no weapon/non-character source)
        skill.EffectiveAffinity = (uint8_t)CorrectTbl::RES_STRIKE - RES_OFFSET;
        skill.EffectiveDependencyType = 0;
        if(weaponDef)
        {
            if(skill.EffectiveAffinity == 1)
            {
                skill.EffectiveAffinity = weaponDef->GetCommon()->GetAffinity();
            }

            if(skill.EffectiveDependencyType == 4)
            {
                switch (weaponDef->GetBasic()->GetWeaponType())
                {
                case objects::MiItemBasicData::WeaponType_t::LONG_RANGE:
                    skill.EffectiveDependencyType = 1;
                    break;
                case objects::MiItemBasicData::WeaponType_t::CLOSE_RANGE:
                default:
                    // Already set
                    break;
                }
            }
        }
    }

    // Get the target of the spell
    uint16_t initialDamageFlags1 = 0;
    auto effectiveSource = source;
    std::shared_ptr<ActiveEntityState> primaryTarget;
    std::list<SkillTargetResult> targetResults;
    switch(skillData->GetTarget()->GetType())
    {
    case objects::MiTargetData::Type_t::NONE:
        // Source can be affected but it is not a target
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
            auto targetEntity = zone->GetActiveEntity(targetEntityID);
                
            if(!targetEntity)
            {
                // Target is not valid anymore
                /// @todo: what should we do in this instance?
                break;
            }

            SkillTargetResult target;
            target.EntityState = targetEntity;
            if(SetNRA(target, skill))
            {
                // The skill is reflected and the source becomes
                // the primary target
                primaryTarget = source;
                effectiveSource = targetEntity;
                targetResults.push_back(target);
            }
            else
            {
                primaryTarget = targetEntity;
            }

            initialDamageFlags1 = target.DamageFlags1;
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

    std::list<std::shared_ptr<ActiveEntityState>> effectiveTargets;
    if(primaryTarget)
    {
        effectiveTargets.push_back(primaryTarget);
    }

    auto skillRange = skillData->GetRange();
    if(skillRange->GetAreaType() != objects::MiEffectiveRangeData::AreaType_t::NONE)
    {
        // Determine area effects
        // Unlike damage calculations, this will use effectiveSource instead
        // of source since reflects may have changed the context of the skill

        double aoeRange = (double)(skillRange->GetAoeRange() * 10);

        switch(skillRange->GetAreaType())
        {
        case objects::MiEffectiveRangeData::AreaType_t::SOURCE:
            // Not exactly an area but skills targetting the source only should pass
            // both this check and area target type filtering for "Ally" or "Source"
            effectiveTargets.push_back(effectiveSource);
            break;
        case objects::MiEffectiveRangeData::AreaType_t::SOURCE_RADIUS:
            effectiveTargets = zone->GetActiveEntitiesInRadius(
                effectiveSource->GetCurrentX(), effectiveSource->GetCurrentY(), aoeRange);
            break;
        case objects::MiEffectiveRangeData::AreaType_t::TARGET_RADIUS:
            if(primaryTarget)
            {
                effectiveTargets = zone->GetActiveEntitiesInRadius(
                    primaryTarget->GetCurrentX(), primaryTarget->GetCurrentY(), aoeRange);
            }
            break;
        case objects::MiEffectiveRangeData::AreaType_t::FRONT_1:
        case objects::MiEffectiveRangeData::AreaType_t::FRONT_2:
        case objects::MiEffectiveRangeData::AreaType_t::FRONT_3:
        case objects::MiEffectiveRangeData::AreaType_t::STRAIGHT_LINE:
        case objects::MiEffectiveRangeData::AreaType_t::UNKNOWN_9:
        default:
            LOG_ERROR(libcomp::String("Unsupported skill area type encountered: %1\n")
                .Arg((uint8_t)skillRange->GetAreaType()));
            return false;
        }

        // Make sure the primary target isn't in here twice and it is also
        // at the front of the list
        if(primaryTarget)
        {
            effectiveTargets.remove_if([primaryTarget](
                const std::shared_ptr<ActiveEntityState>& target)
                {
                    return target == primaryTarget;
                });
            effectiveTargets.push_front(primaryTarget);
        }

        // Filter out invalid effective targets (including the primary target)
        /// @todo: implement a more complex faction system for PvP etc
        auto areaTargetType = skillRange->GetAreaTarget();
        switch(areaTargetType)
        {
        case objects::MiEffectiveRangeData::AreaTarget_t::ENEMY:
            effectiveTargets.remove_if([effectiveSource](
                const std::shared_ptr<ActiveEntityState>& target)
                {
                    return (target->GetFaction() == effectiveSource->GetFaction()) ||
                        !target->IsAlive();
                });
            break;
        case objects::MiEffectiveRangeData::AreaTarget_t::ALLY:
        case objects::MiEffectiveRangeData::AreaTarget_t::PARTY:
        case objects::MiEffectiveRangeData::AreaTarget_t::DEAD_ALLY:
        case objects::MiEffectiveRangeData::AreaTarget_t::DEAD_PARTY:
            {
                bool deadOnly = areaTargetType ==
                    objects::MiEffectiveRangeData::AreaTarget_t::DEAD_ALLY ||
                    areaTargetType == objects::MiEffectiveRangeData::AreaTarget_t::DEAD_PARTY;
                effectiveTargets.remove_if([effectiveSource, deadOnly](
                    const std::shared_ptr<ActiveEntityState>& target)
                    {
                        return (target->GetFaction() != effectiveSource->GetFaction())
                            || (deadOnly == target->IsAlive());
                    });

                if(areaTargetType == objects::MiEffectiveRangeData::AreaTarget_t::PARTY ||
                   areaTargetType == objects::MiEffectiveRangeData::AreaTarget_t::DEAD_PARTY)
                {
                    // This will result in an empty list if cast by an enemy, though
                    // technically it should in that instance
                    auto sourceState = ClientState::GetEntityClientState(effectiveSource->GetEntityID());
                    uint32_t sourcePartyID = sourceState ? sourceState->GetPartyID() : 0;

                    effectiveTargets.remove_if([sourcePartyID](
                        const std::shared_ptr<ActiveEntityState>& target)
                        {
                            auto state = ClientState::GetEntityClientState(target->GetEntityID());
                            return sourcePartyID == 0 || !state ||
                                state->GetPartyID() != sourcePartyID;
                        });
                }
            }
            break;
        case objects::MiEffectiveRangeData::AreaTarget_t::SOURCE:
            effectiveTargets.remove_if([effectiveSource](
                const std::shared_ptr<ActiveEntityState>& target)
                {
                    return target != effectiveSource;
                });
            break;
        default:
            LOG_ERROR(libcomp::String("Unsupported skill area target encountered: %1\n")
                .Arg((uint8_t)skillRange->GetAreaTarget()));
            return false;
        }
    }

    // Filter down to all valid targets, limited by AOE restrictions
    uint16_t aoeReflect = 0;
    int32_t aoeTargetCount = 0;
    int32_t aoeTargetMax = skillRange->GetAoeTargetMax();
    for(auto effectiveTarget : effectiveTargets)
    {
        bool isPrimaryTarget = effectiveTarget == primaryTarget;

        // Skip the primary target for the count which will always be first
        // in the list if it is still valid at this point
        if(!isPrimaryTarget &&
            aoeTargetMax > 0 && aoeTargetCount >= aoeTargetMax)
        {
            break;
        }

        SkillTargetResult target;
        target.PrimaryTarget = isPrimaryTarget;
        target.EntityState = effectiveTarget;

        // Set NRA
        // If the primary target is still in the set and a reflect did not
        // occur, apply the initially calculated flags first
        // If an AOE target that is not the source is in the set, increase
        // the number of AOE reflections as needed
        bool isSource = effectiveTarget == source;
        if(isPrimaryTarget && (initialDamageFlags1 & FLAG1_REFLECT) == 0)
        {
            target.DamageFlags1 = initialDamageFlags1;
        }
        else if(SetNRA(target, skill) && !isSource)
        {
            aoeReflect++;
        }

        targetResults.push_back(target);

        if(!isPrimaryTarget)
        {
            aoeTargetCount++;
        }
    }

    // For each time the skill was reflected by an AOE target, target the
    // source again as each can potentially have NRA and damage calculated
    for(uint16_t i = 0; i < aoeReflect; i++)
    {
        SkillTargetResult target;
        target.EntityState = source;
        targetResults.push_back(target);
        SetNRA(target, skill);
        targetResults.push_back(target);
    }

    // Exit if nothing will be affected by damage or effects
    if(targetResults.size() == 0)
    {
        return true;
    }
    
    // Run calculations
    bool hasBattleDamage = false;
    if(skillData->GetDamage()->GetBattleDamage()->GetFormula()
        != objects::MiBattleDamageData::Formula_t::NONE)
    {
        if(!CalculateDamage(source, activated, targetResults, skill))
        {
            LOG_ERROR(libcomp::String("Damage failed to calculate: %1\n")
                .Arg(skillID));
            return false;
        }
        hasBattleDamage = true;
    }

    auto skillKnockback = skillData->GetDamage()->GetKnockBack();
    int8_t kbMod = skillKnockback->GetModifier();
    uint8_t kbType = skillKnockback->GetKnockBackType();
    float kbDistance = (float)(skillKnockback->GetDistance() * 10);
    auto addStatuses = skillData->GetDamage()->GetAddStatuses();

    auto now = server->GetServerTime();
    source->RefreshCurrentPosition(now);

    // Apply calculation results, keeping track of entities that may
    // need to update the world with their modified state
    std::set<std::shared_ptr<ActiveEntityState>> revived;
    std::set<std::shared_ptr<ActiveEntityState>> killed;
    std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
    std::unordered_map<std::shared_ptr<ActiveEntityState>, uint8_t> cancellations;
    for(SkillTargetResult& target : targetResults)
    {
        target.EntityState->RefreshCurrentPosition(now);
        cancellations[target.EntityState] = 0;
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
                    cancellations[target.EntityState] |= EFFECT_CANCEL_DEATH;
                    killed.insert(target.EntityState);
                }
                else
                {
                    target.DamageFlags1 |= FLAG1_REVIVAL;
                    revived.insert(target.EntityState);
                }
            }
            
            if(hpAdjusted < 0)
            {
                if(kbMod)
                {
                    float kb = target.EntityState->UpdateKnockback(now, kbMod);
                    if(kb == 0.f)
                    {
                        target.DamageFlags1 |= FLAG1_KNOCKBACK;
                        cancellations[target.EntityState] |= EFFECT_CANCEL_KNOCKBACK;
                    }
                }

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

        // Determine which status effects to apply
        if((target.DamageFlags1 & (FLAG1_BLOCK | FLAG1_REFLECT | FLAG1_ABSORB)) == 0)
        {
            for(auto addStatus : addStatuses)
            {
                if(addStatus->GetOnKnockback() && !(target.DamageFlags1 & FLAG1_KNOCKBACK)) continue;

                uint16_t successRate = addStatus->GetSuccessRate();
                if(successRate >= 100 || (rand() % 99) <= successRate)
                {
                    auto statusDef = definitionManager->GetStatusData(
                        addStatus->GetStatusID());

                    auto cancelDef = statusDef->GetCancel();
                    if((target.DamageFlags1 & FLAG1_LETHAL) &&
                        (cancelDef->GetCancelTypes() & EFFECT_CANCEL_DEATH))
                    {
                        // If the target is killed and the status cancels on death,
                        // stop here and do not add
                        continue;
                    }

                    int8_t minStack = addStatus->GetMinStack();
                    int8_t maxStack = addStatus->GetMaxStack();

                    // Sanity check
                    if(minStack > maxStack) continue;

                    int8_t stack = minStack == maxStack ? maxStack
                        : (int8_t)(minStack + (rand() % (maxStack - minStack)));
                    if(stack == 0) continue;

                    target.AddedStatuses[addStatus->GetStatusID()] =
                        std::pair<uint8_t, bool>(stack, addStatus->GetIsReplace());

                    // Check for status T-Damage to apply at the end of the skill
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

        characterManager->RecalculateStats(nullptr, target.EntityState->GetEntityID());
    }

    for(auto cancelPair : cancellations)
    {
        if(cancelPair.second)
        {
            cancelPair.first->CancelStatusEffects(cancelPair.second);
        }
    }

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

    auto effectiveTarget = primaryTarget ? primaryTarget : effectiveSource;
    std::unordered_map<uint32_t, uint64_t> timeMap;
    uint64_t hitTimings[3];
    uint64_t completeTime = now +
        (skillData->GetDischarge()->GetStiffness() * 1000);
    uint64_t hitStopTime = completeTime +
        (skillData->GetDamage()->GetHitStopTime() * 1000);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
    reply.WriteS32Little(source->GetEntityID());
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

        if((target.DamageFlags1 & FLAG1_KNOCKBACK) && kbType != 2)
        {
            uint8_t kbEffectiveType = kbType;
            if(kbType == 1 && target.PrimaryTarget)
            {
                // Targets of AOE knockback are treated like default knockback
                kbEffectiveType = 0;
            }

            // Ignore knockback type 2 which is "None"
            switch(kbEffectiveType)
            {
            case 1:
                {
                    // Away from the effective target (ex: AOE explosion)
                    target.EntityState->MoveRelative(effectiveTarget->GetCurrentX(),
                        effectiveTarget->GetCurrentY(), kbDistance, true, now, hitStopTime);
                }
                break;
            case 4:
                /// @todo: To the front of the source
                break;
            case 5:
                /// @todo: To the source
                break;
            case 0:
            case 3: /// @todo: technically this has more spread than 0
            default:
                // Default if not specified, directly away from source
                target.EntityState->MoveRelative(effectiveSource->GetCurrentX(),
                    effectiveSource->GetCurrentY(), kbDistance, true, now, hitStopTime);
                break;
            }

            reply.WriteFloat(target.EntityState->GetDestinationX());
            reply.WriteFloat(target.EntityState->GetDestinationY());
        }
        else
        {
            reply.WriteBlank(8);
        }

        reply.WriteFloat(0);    // Unknown

        // Calculate hit timing
        hitTimings[0] = hitTimings[1] = hitTimings[2] = 0;
        if(target.Damage1Type == DAMAGE_TYPE_GENERIC)
        {
            if(target.Damage1)
            {
                // Damage dealt, apply hit stop
                hitTimings[0] = completeTime;
                hitTimings[1] = hitStopTime;

                if(!target.AilmentDamageType)
                {
                    // End after hit stop
                    hitTimings[2] = hitStopTime;
                }
                else
                {
                    // Apply ailment damage after hit stop
                    /// @todo: calculate
                    hitTimings[2] = hitStopTime;
                }
            }
            else
            {
                // No damage, just result displays
                hitTimings[2] = completeTime;
            }
        }

        for(size_t i = 0; i < 3; i++)
        {
            if(hitTimings[i])
            {
                timeMap[(uint32_t)(reply.Size() + (4 * i))] = hitTimings[i];
            }
        }

        // Double back at the end and write client specific times
        reply.WriteBlank(12);

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

    auto zConnections = zone->GetConnectionList();
    ChannelClientConnection::SendRelativeTimePacket(zConnections, reply, timeMap);

    if(revived.size() > 0)
    {
        for(auto entity : revived)
        {
            libcomp::Packet p;
            if(characterManager->GetEntityRevivalPacket(p, entity, 6))
            {
                zoneManager->BroadcastPacket(zone, p);
            }
        }
    }

    if(killed.size() > 0)
    {
        for(auto entity : killed)
        {
            // Remove all opponents
            characterManager->AddRemoveOpponent(false, entity, nullptr);

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

bool SkillManager::ToggleSwitchSkill(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    auto characterManager = server->GetCharacterManager();
    auto skillID = activated->GetSkillID();
    auto skillData = definitionManager->GetSkillData(skillID);

    bool toggleOn = false;
    if(source->ActiveSwitchSkillsContains(skillID))
    {
        source->RemoveActiveSwitchSkills(skillID);
    }
    else
    {
        source->InsertActiveSwitchSkills(skillID);
        toggleOn = true;
    }

    FinalizeSkillExecution(client, activated, skillData);

    if(client)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_SWITCH);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(skillID);
        p.WriteS8(toggleOn ? 1 : 0);

        client->QueuePacket(p);
    }

    characterManager->RecalculateStats(client, source->GetEntityID());

    if(client)
    {
        client->FlushOutgoing();
    }

    return true;
}

bool SkillManager::CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    std::list<SkillTargetResult>& targets, ProcessingSkill& skill)
{
    auto damageData = skill.Definition->GetDamage()->GetBattleDamage();
    auto formula = damageData->GetFormula();
    bool isHeal = formula == objects::MiBattleDamageData::Formula_t::HEAL_NORMAL
        || formula == objects::MiBattleDamageData::Formula_t::HEAL_STATIC
        || formula == objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT;

    uint8_t rateBoostIdx = 0;
    uint16_t off = 0;
    switch(skill.EffectiveDependencyType)
    {
    case 0:
        off = (uint16_t)source->GetCLSR();
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_CLSR;
        break;
    case 1:
        off = (uint16_t)source->GetLNGR();
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_LNGR;
        break;
    case 2:
        off = (uint16_t)source->GetSPELL();
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
        break;
    case 3:
        off = (uint16_t)source->GetSUPPORT();
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_SUPPORT;
        break;
    case 6:
        off = (uint16_t)(source->GetLNGR() + source->GetSPELL() / 2);
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_LNGR;
        break;
    case 7:
        off = (uint16_t)(source->GetSPELL() + source->GetCLSR() / 2);
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
        break;
    case 8:
        off = (uint16_t)(source->GetSPELL() + source->GetLNGR() / 2);
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
        break;
    case 9:
        off = (uint16_t)(source->GetCLSR() + source->GetLNGR()
            + source->GetSPELL());
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_CLSR;
        break;
    case 10:
        off = (uint16_t)(source->GetLNGR() + source->GetCLSR()
            + source->GetSPELL());
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_LNGR;
        break;
    case 11:
        off = (uint16_t)(source->GetSPELL() + source->GetCLSR()
            + source->GetLNGR());
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
        break;
    case 12:
        off = (uint16_t)(source->GetCLSR() + source->GetSPELL() / 2);
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_CLSR;
        break;
    case 5:
    default:
        LOG_ERROR(libcomp::String("Invalid dependency type for"
            " damage calculation encountered: %1\n")
            .Arg(skill.EffectiveDependencyType));
        return false;
    }

    // Apply source rate boosts
    if(rateBoostIdx != 0)
    {
        off = (uint16_t)(off *
            (source->GetCorrectValue((CorrectTbl)rateBoostIdx) * 0.01));
    }

    if(isHeal)
    {
        off = (uint16_t)(off *
            (source->GetCorrectValue(CorrectTbl::RATE_HEAL) * 0.01));
    }

    CorrectTbl boostCorrectType = (CorrectTbl)(skill.EffectiveAffinity + BOOST_OFFSET);
    CorrectTbl resistCorrectType = (CorrectTbl)(skill.EffectiveAffinity + RES_OFFSET);

    float boost = (float)(source->GetCorrectValue(boostCorrectType) * 0.01);
    if(boost < -100.f)
    {
        boost = -100.f;
    }

    int16_t critRate = source->GetCorrectValue(CorrectTbl::CRITICAL);

    for(SkillTargetResult& target : targets)
    {
        if(target.DamageFlags1 & (FLAG1_BLOCK | FLAG1_REFLECT)) continue;

        uint16_t mod1 = damageData->GetModifier1();
        uint16_t mod2 = damageData->GetModifier2();

        switch(formula)
        {
        case objects::MiBattleDamageData::Formula_t::NONE:
            return true;
        case objects::MiBattleDamageData::Formula_t::DMG_NORMAL:
        case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
            {
                uint16_t def = 0;
                switch(skill.EffectiveDependencyType)
                {
                case 0:
                case 1:
                case 6:
                case 9:
                case 10:
                case 12:
                    def = (uint16_t)target.EntityState->GetPDEF();
                    break;
                case 2:
                case 7:
                case 8:
                case 11:
                    def = (uint16_t)target.EntityState->GetMDEF();
                    break;
                case 3:
                case 5:
                default:
                    break;
                }

                uint8_t critLevel = 0;
                if(critRate > 0)
                {
                    /// @todo: implement limit break
                    if(rand() % 100 <= critRate)
                    {
                        critLevel = 1;
                    }
                }

                float resist = (float)
                    (target.EntityState->GetCorrectValue(resistCorrectType) * 0.01);
                if(target.DamageFlags1 & FLAG1_ABSORB)
                {
                    // Resistance is not applied during absorption
                    resist = 0;
                }

                target.Damage1 = CalculateDamage_Normal(
                    mod1, target.Damage1Type, off, def,
                    resist, boost, critLevel);
                target.Damage2 = CalculateDamage_Normal(
                    mod2, target.Damage2Type, off, def,
                    resist, boost, critLevel);

                // Crits, protect and weakpoint do not apply to healing
                if(!isHeal && (target.DamageFlags1 & FLAG1_ABSORB) == 0)
                {
                    // Set crit-level adjustment flags
                    switch(critLevel)
                    {
                        case 1:
                            target.DamageFlags1 |= FLAG1_CRITICAL;
                            break;
                        case 2:
                            if(target.Damage1 >= 30000 ||
                                target.Damage2 >= 30000)
                            {
                                target.DamageFlags2 |= FLAG2_INTENSIVE_BREAK;
                            }
                            else
                            {
                                target.DamageFlags2 |= FLAG2_LIMIT_BREAK;
                            }
                            break;
                        default:
                            break;
                    }

                    // Set resistence flags
                    if(resist >= 0.5f)
                    {
                        target.DamageFlags1 |= FLAG1_PROTECT;
                    }
                    else if(resist <= -0.5f)
                    {
                        target.DamageFlags1 |= FLAG1_WEAKPOINT;
                    }
                }
            }
            break;
        case objects::MiBattleDamageData::Formula_t::DMG_STATIC:
        case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
            {
                target.Damage1 = CalculateDamage_Static(
                    mod1, target.Damage1Type);
                target.Damage2 = CalculateDamage_Static(
                    mod2, target.Damage2Type);
            }
            break;
        case objects::MiBattleDamageData::Formula_t::DMG_PERCENT:
            {
                target.Damage1 = CalculateDamage_Percent(
                    mod1, target.Damage1Type,
                    target.EntityState->GetCoreStats()->GetHP());
                target.Damage2 = CalculateDamage_Percent(
                    mod2, target.Damage2Type,
                    target.EntityState->GetCoreStats()->GetMP());
            }
            break;
        case objects::MiBattleDamageData::Formula_t::DMG_SOURCE_PERCENT:
            {
                //Calculate using pre-cost values
                target.Damage1 = CalculateDamage_Percent(
                    mod1, target.Damage1Type,
                    static_cast<int16_t>(source->GetCoreStats()->GetHP() +
                        activated->GetHPCost()));
                target.Damage2 = CalculateDamage_Percent(
                    mod2, target.Damage2Type,
                    static_cast<int16_t>(source->GetCoreStats()->GetMP() +
                        activated->GetMPCost()));
            }
            break;
        case objects::MiBattleDamageData::Formula_t::DMG_MAX_PERCENT:
        case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT:
            {
                target.Damage1 = CalculateDamage_MaxPercent(
                    mod1, target.Damage1Type,
                    target.EntityState->GetMaxHP());
                target.Damage2 = CalculateDamage_MaxPercent(
                    mod2, target.Damage2Type,
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

        // Reduce for AOE and make sure at least 1 damage was dealt to each specified type
        if(mod1)
        {
            if(!target.PrimaryTarget && damageData->GetAoeReduction())
            {
                target.Damage1 = (uint16_t)((float)target.Damage1 *
                    (float)(1.f - (0.01f * (float)damageData->GetAoeReduction())));
            }

            if(target.Damage1 == 0)
            {
                target.Damage1 = 1;
            }
        }
    
        if(mod2)
        {
            if(!target.PrimaryTarget && damageData->GetAoeReduction())
            {
                target.Damage2 = (uint16_t)((float)target.Damage2 *
                    (float)(1.f - (0.01f * (float)damageData->GetAoeReduction())));
            }

            if(target.Damage2 == 0)
            {
                target.Damage2 = 1;
            }
        }

        // If the damage was actually a heal, invert the amount and change the type
        if(isHeal || (target.DamageFlags1 & FLAG1_ABSORB))
        {
            target.Damage1 = target.Damage1 * -1;
            target.Damage2 = target.Damage2 * -1;
            target.Damage1Type = (target.Damage1Type == DAMAGE_TYPE_GENERIC) ?
                DAMAGE_TYPE_HEALING : target.Damage1Type;
            target.Damage2Type = (target.Damage2Type == DAMAGE_TYPE_GENERIC) ?
                DAMAGE_TYPE_HEALING : target.Damage2Type;
        }
    }

    return true;
}

int32_t SkillManager::CalculateDamage_Normal(uint16_t mod, uint8_t& damageType,
    uint16_t off, uint16_t def, float resist, float boost, uint8_t critLevel)
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

        // Multiply by 100% + -resistance
        calc = calc * (1 + resist * -1.f);

        // Multiply by 100% + boost
        calc = calc * (1.f + boost);

        // Multiply by 1 + remaining power boosts/100
        calc = calc * (float)(1 - (0/* @todo: Action+Racial+Skill Power */ * 0.01f));

        /// @todo: there is more to this calculation

        amount = (int32_t)ceil(calc);
        damageType = DAMAGE_TYPE_GENERIC;
    }

    return amount;
}

int32_t SkillManager::CalculateDamage_Static(uint16_t mod, uint8_t& damageType)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        amount = (int32_t)mod;
        damageType = DAMAGE_TYPE_GENERIC;
    }

    return amount;
}

int32_t SkillManager::CalculateDamage_Percent(uint16_t mod, uint8_t& damageType,
    int16_t current)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        amount = (int32_t)ceil((float)current * ((float)mod * 0.01f));
        damageType = DAMAGE_TYPE_GENERIC;
    }

    return amount;
}

int32_t SkillManager::CalculateDamage_MaxPercent(uint16_t mod, uint8_t& damageType,
    int16_t max)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        amount = (int32_t)ceil((float)max * ((float)mod * 0.01f));
        damageType = DAMAGE_TYPE_GENERIC;
    }

    return amount;
}

bool SkillManager::SetNRA(SkillTargetResult& target, ProcessingSkill& skill)
{
    // Calculate affinity checks for both base and effective values if they differ
    std::list<CorrectTbl> affinities;
    if(skill.BaseAffinity != skill.EffectiveAffinity)
    {
        affinities.push_back((CorrectTbl)(skill.BaseAffinity + NRA_OFFSET));
    }
    affinities.push_back((CorrectTbl)(skill.EffectiveAffinity + NRA_OFFSET));

    for(CorrectTbl affinity : affinities)
    {
        for(auto nraIdx : { NRA_ABSORB, NRA_REFLECT, NRA_NULL })
        {
            int16_t chance = target.EntityState->GetNRAChance((uint8_t)nraIdx, affinity);
            if(chance > 0 && (rand() % 100) <= chance)
            {
                switch(nraIdx)
                {
                case NRA_NULL:
                    target.DamageFlags1 |= FLAG1_BLOCK;
                    return false;
                case NRA_ABSORB:
                    target.DamageFlags1 |= FLAG1_ABSORB;
                    return false;
                case NRA_REFLECT:
                default:
                    target.DamageFlags1 |= FLAG1_REFLECT;
                    return true;
                }
            }
        }
    }

    return false;
}

void SkillManager::FinalizeSkillExecution(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<objects::MiSkillData> skillData)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(
        activated->GetSourceEntity());
    auto zone = source->GetZone();
    auto characterManager = mServer.lock()->GetCharacterManager();

    if(skillData->GetBasic()->GetCombatSkill() && activated->GetEntityTargeted() && zone)
    {
        // Start combat if the target exists
        auto targetEntityID = (int32_t)activated->GetTargetObjectID();
        auto target = zone->GetActiveEntity(targetEntityID);
        if(target && target->GetFaction() != source->GetFaction())
        {
            characterManager->AddRemoveOpponent(true, source, target);
        }
    }

    SendExecuteSkill(client, activated, skillData);

    characterManager->UpdateExpertise(client, activated->GetSkillID());

    // Clean up and send the skill complete
    source->SetActivatedAbility(nullptr);
    SendCompleteSkill(client, activated, false);

    // Cancel any status effects that expire on skill execution
    characterManager->CancelStatusEffects(client, EFFECT_CANCEL_SKILL);
}

bool SkillManager::SpecialSkill(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    (void)client;
    (void)activated;

    return true;
}

bool SkillManager::EquipItem(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto itemID = activated->GetTargetObjectID();
    if(itemID <= 0)
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->EquipItem(client, itemID);

    return true;
}

bool SkillManager::SummonDemon(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
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
    std::shared_ptr<objects::ActivatedAbility> activated)
{
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
    std::shared_ptr<objects::ActivatedAbility> activated)
{
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
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto state = client->GetClientState();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_CHARGING);
    reply.WriteS32Little(activated->GetSourceEntity()->GetEntityID());
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
    std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<objects::MiSkillData> skillData)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto conditionData = skillData->GetCondition();
    auto dischargeData = skillData->GetDischarge();

    int32_t targetedEntityID = activated->GetEntityTargeted()
        ? (int32_t)activated->GetTargetObjectID()
        : activated->GetSourceEntity()->GetEntityID();

    auto cdTime = conditionData->GetCooldownTime();
    auto stiffness = dischargeData->GetStiffness();
    auto currentTime = state->ToClientTime(ChannelServer::GetServerTime());

    auto cooldownTime = cdTime ? (currentTime + ((float)cdTime * 0.001f)) : 0.f;
    auto lockOutTime = currentTime + ((float)stiffness * 0.001f);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_EXECUTING);
    reply.WriteS32Little(activated->GetSourceEntity()->GetEntityID());
    reply.WriteU32Little(activated->GetSkillID());
    reply.WriteS8((int8_t)activated->GetActivationID());
    reply.WriteS32Little(targetedEntityID);
    reply.WriteFloat(cooldownTime);
    reply.WriteFloat(lockOutTime);
    reply.WriteU32Little((uint32_t)activated->GetHPCost());
    reply.WriteU32Little((uint32_t)activated->GetMPCost());
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
    std::shared_ptr<objects::ActivatedAbility> activated, bool cancelled)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_COMPLETED);
    reply.WriteS32Little(activated->GetSourceEntity()->GetEntityID());
    reply.WriteU32Little(activated->GetSkillID());
    reply.WriteS8((int8_t)activated->GetActivationID());
    reply.WriteFloat(0);   //Unknown
    reply.WriteU8(1);   //Unknown
    reply.WriteFloat(300.0f);   //Run speed
    reply.WriteU8(cancelled ? 1 : 0);

    mServer.lock()->GetZoneManager()->BroadcastPacket(client, reply);
}
