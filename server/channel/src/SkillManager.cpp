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
#include <Randomizer.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ActivatedAbility.h>
#include <Item.h>
#include <ItemBox.h>
#include <ItemDrop.h>
#include <Loot.h>
#include <LootBox.h>
#include <MiAddStatusTbl.h>
#include <MiBattleDamageData.h>
#include <MiCancelData.h>
#include <MiCastBasicData.h>
#include <MiCastData.h>
#include <MiCategoryData.h>
#include <MiConditionData.h>
#include <MiCostTbl.h>
#include <MiDamageData.h>
#include <MiDCategoryData.h>
#include <MiDevilData.h>
#include <MiDevilFamiliarityData.h>
#include <MiDischargeData.h>
#include <MiDoTDamageData.h>
#include <MiEffectData.h>
#include <MiEffectiveRangeData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiKnockBackData.h>
#include <MiNegotiationDamageData.h>
#include <MiNegotiationData.h>
#include <MiNPCBasicData.h>
#include <MiPossessionData.h>
#include <MiSkillBasicData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSkillSpecialParams.h>
#include <MiStatusData.h>
#include <MiStatusBasicData.h>
#include <MiSummonData.h>
#include <MiTargetData.h>
#include <Party.h>
#include <ServerZone.h>
#include <Spawn.h>

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

const uint8_t TALK_RESPONSE_1 = 1;
//const uint8_t TALK_RESPONSE_2 = 2;
//const uint8_t TALK_RESPONSE_3 = 3;
const uint8_t TALK_RESPONSE_4 = 4;
const uint8_t TALK_JOIN = 5;
const uint8_t TALK_GIVE_ITEM = 6;
//const uint8_t TALK_STOP = 7;
const uint8_t TALK_LEAVE = 8;
const uint8_t TALK_JOIN_2 = 9;
const uint8_t TALK_GIVE_ITEM_2 = 10;
const uint8_t TALK_REJECT = 13;
//const uint8_t TALK_THREATENED = 14;

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
    uint8_t TalkFlags = 0;
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
    mSkillFunctions[SVR_CONST.SKILL_FAM_UP] = &SkillManager::FamiliarityUp;
    mSkillFunctions[SVR_CONST.SKILL_ITEM_FAM_UP] = &SkillManager::FamiliarityUpItem;
    mSkillFunctions[SVR_CONST.SKILL_MOOCH] = &SkillManager::Mooch;
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
    activated->SetActivationObjectID(targetObjectID);
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

        int64_t targetItem = 0;
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
                if(state->GetObjectID(item->GetUUID()) ==
                    activated->GetActivationObjectID())
                {
                    targetItem = activated->GetActivationObjectID();
                }
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

        if(bulletCost > 0)
        {
            itemCosts[bulletIDs.first] = bulletCost;
            targetItem = bulletIDs.second;
        }

        if(itemCosts.size() > 0)
        {
            characterManager->AddRemoveItems(client, itemCosts, false, targetItem);
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

bool SkillManager::ProcessSkillResult(std::shared_ptr<objects::ActivatedAbility> activated,
    bool applyStatusEffects)
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
        // Source is technically the primary target (though most of
        // these types of skills will filter it out)
        primaryTarget = source;
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

    auto skillRange = skillData->GetRange();
    std::list<std::shared_ptr<ActiveEntityState>> effectiveTargets;
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
    auto validType = skillRange->GetValidType();
    switch(validType)
    {
    case objects::MiEffectiveRangeData::ValidType_t::ENEMY:
        effectiveTargets.remove_if([effectiveSource](
            const std::shared_ptr<ActiveEntityState>& target)
            {
                return (target->GetFaction() == effectiveSource->GetFaction()) ||
                    !target->IsAlive();
            });
        break;
    case objects::MiEffectiveRangeData::ValidType_t::ALLY:
    case objects::MiEffectiveRangeData::ValidType_t::PARTY:
    case objects::MiEffectiveRangeData::ValidType_t::DEAD_ALLY:
    case objects::MiEffectiveRangeData::ValidType_t::DEAD_PARTY:
        {
            bool deadOnly = validType ==
                objects::MiEffectiveRangeData::ValidType_t::DEAD_ALLY ||
                validType == objects::MiEffectiveRangeData::ValidType_t::DEAD_PARTY;
            effectiveTargets.remove_if([effectiveSource, deadOnly](
                const std::shared_ptr<ActiveEntityState>& target)
                {
                    return (target->GetFaction() != effectiveSource->GetFaction())
                        || (deadOnly == target->IsAlive());
                });

            if(validType == objects::MiEffectiveRangeData::ValidType_t::PARTY ||
                validType == objects::MiEffectiveRangeData::ValidType_t::DEAD_PARTY)
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
    case objects::MiEffectiveRangeData::ValidType_t::SOURCE:
        effectiveTargets.remove_if([effectiveSource](
            const std::shared_ptr<ActiveEntityState>& target)
            {
                return target != effectiveSource;
            });
        break;
    default:
        LOG_ERROR(libcomp::String("Unsupported skill valid target type encountered: %1\n")
            .Arg((uint8_t)validType));
        return false;
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

    auto damageData = skillData->GetDamage();
    
    // Run calculations
    bool hasBattleDamage = false;
    if(damageData->GetBattleDamage()->GetFormula()
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

    // Get knockback info
    auto skillKnockback = damageData->GetKnockBack();
    int8_t kbMod = skillKnockback->GetModifier();
    uint8_t kbType = skillKnockback->GetKnockBackType();
    float kbDistance = (float)(skillKnockback->GetDistance() * 10);

    // Get negotiation damage
    auto talkDamage = damageData->GetNegotiationDamage();
    int8_t talkAffSuccess = talkDamage->GetSuccessAffability();
    int8_t talkAffFailure = talkDamage->GetFailureAffability();
    int8_t talkFearSuccess = talkDamage->GetSuccessFear();
    int8_t talkFearFailure = talkDamage->GetFailureFear();
    bool hasTalkDamage = talkAffSuccess != 0 || talkAffFailure != 0 ||
        talkFearSuccess != 0 || talkFearFailure != 0;

    // Get added status effects
    auto addStatuses = damageData->GetAddStatuses();

    auto now = server->GetServerTime();
    source->RefreshCurrentPosition(now);

    // Apply calculation results, keeping track of entities that may
    // need to update the world with their modified state
    std::set<std::shared_ptr<ActiveEntityState>> revived;
    std::set<std::shared_ptr<ActiveEntityState>> killed;
    std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
    std::list<std::pair<std::shared_ptr<ActiveEntityState>, uint8_t>> talkDone;
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
                case objects::EntityStateObject::EntityType_t::ENEMY:
                    if(hpDamage > 0)
                    {
                        // If an enemy is damage by a player character or their
                        // partner demon, keep track of the damage for the damage
                        // race drop rule
                        auto sourceState = ClientState::GetEntityClientState(
                            source->GetEntityID());
                        if(sourceState)
                        {
                            auto worldCID = sourceState->GetWorldCID();

                            auto eState = std::dynamic_pointer_cast<EnemyState>(
                                target.EntityState);
                            auto enemy = eState->GetEntity();
                            if(!enemy->DamageSourcesKeyExists(worldCID))
                            {
                                enemy->SetDamageSources(worldCID,
                                    (uint64_t)hpDamage);
                            }
                            else
                            {
                                uint64_t damage = enemy->GetDamageSources(
                                    worldCID);
                                damage = (uint64_t)(damage + (uint64_t)hpDamage);
                                enemy->SetDamageSources(worldCID, damage);
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        // Determine which status effects to apply
        if(applyStatusEffects &&
            (target.DamageFlags1 & (FLAG1_BLOCK | FLAG1_REFLECT | FLAG1_ABSORB)) == 0)
        {
            for(auto addStatus : addStatuses)
            {
                if(addStatus->GetOnKnockback() && !(target.DamageFlags1 & FLAG1_KNOCKBACK)) continue;

                int32_t successRate = (int32_t)addStatus->GetSuccessRate();
                if(successRate >= 100 || RNG(int32_t, 0, 99) <= successRate)
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

                    uint8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
                        addStatus->GetMaxStack());
                    if(stack == 0 && !addStatus->GetIsReplace()) continue;

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

        // Handle negotiation damage
        if(hasTalkDamage && target.EntityState->GetEntityType() ==
            objects::EntityStateObject::EntityType_t::ENEMY &&
            killed.find(target.EntityState) == killed.end())
        {
            auto eState = std::dynamic_pointer_cast<EnemyState>(
                target.EntityState);
            auto enemy = eState->GetEntity();
            if(enemy->GetCoreStats()->GetLevel() >
                source->GetCoreStats()->GetLevel())
            {
                // Enemies that are a higher level cannot be negotiated with
                break;
            }

            auto talkPoints = eState->GetTalkPoints(source->GetEntityID());
            auto demonData = definitionManager->GetDevilData(enemy->GetType());
            auto negData = demonData->GetNegotiation();
            uint8_t affThreshold = (uint8_t)(100 - negData->GetAffabilityThreshold());
            uint8_t fearThreshold = (uint8_t)(100 - negData->GetFearThreshold());

            if(talkPoints.first >= affThreshold ||
                talkPoints.second >= fearThreshold)
            {
                // Nothing left to do
                break;
            }

            /// @todo: properly handle negotiation chances and outcome
            bool success = RNG(uint16_t, 0, 100) <= 90;
            int16_t aff = (int16_t)(talkPoints.first +
                (success ? talkAffSuccess : talkAffFailure));
            int16_t fear = (int16_t)(talkPoints.second +
                (success ? talkFearSuccess : talkFearFailure));

            talkPoints.first = aff < 0 ? 0 : (uint8_t)aff;
            talkPoints.second = fear < 0 ? 0 : (uint8_t)fear;

            eState->SetTalkPoints(source->GetEntityID(), talkPoints);
                        
            if(talkPoints.first >= affThreshold ||
                talkPoints.second >= fearThreshold)
            {
                int32_t outcome = RNG(int32_t, 1, 6);
                switch(outcome)
                {
                case 1:
                    target.TalkFlags = TALK_JOIN;
                    break;
                case 2:
                    target.TalkFlags = TALK_JOIN_2;
                    break;
                case 3:
                    target.TalkFlags = TALK_GIVE_ITEM;
                    break;
                case 4:
                    target.TalkFlags = TALK_GIVE_ITEM_2;
                    break;
                case 5:
                    target.TalkFlags = TALK_REJECT;
                    break;
                case 6:
                default:
                    target.TalkFlags = TALK_LEAVE;
                    break;
                }

                auto spawn = enemy->GetSpawnSource();
                if((target.TalkFlags == TALK_GIVE_ITEM ||
                    target.TalkFlags == TALK_GIVE_ITEM) &&
                    (!spawn || spawn->GiftsCount() == 0))
                {
                    // No gifts mapped, leave instead
                    target.TalkFlags = TALK_LEAVE;
                }
                std::pair<std::shared_ptr<ActiveEntityState>,
                    uint8_t> pair(target.EntityState, target.TalkFlags);
                talkDone.push_back(pair);
            }
            else
            {
                target.TalkFlags = (success ? TALK_RESPONSE_1 : TALK_RESPONSE_4);
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

    // Send negotiation results first since some are dependent upon the
    // skill hit
    if(talkDone.size() > 0)
    {
        HandleNegotiations(source, zone, talkDone);
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

        reply.WriteU8(target.TalkFlags);

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
        HandleKills(source, zone, killed);
    }

    if(displayStateModified.size() > 0)
    {
        characterManager->UpdateWorldDisplayState(displayStateModified);
    }

    return true;
}

void SkillManager::HandleKills(std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<Zone> zone,
    std::set<std::shared_ptr<ActiveEntityState>> killed)
{
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto zoneManager = server->GetZoneManager();

    auto zConnections = zone->GetConnectionList();

    // Familiarity is reduced from death (0) or same demon kills (1)
    // and is dependent upon familiarity type
    const int16_t fTypeMap[17][2] =
        {
            { -100, -5 },   // Type 0
            { -20, -50 },   // Type 1
            { -20, -20 },   // Type 2
            { -50, -50 },   // Type 3
            { -100, -100 }, // Type 4
            { -100, -100 }, // Type 5
            { -20, -20 },   // Type 6
            { -50, -50 },   // Type 7
            { -100, -100 }, // Type 8
            { -100, -100 }, // Type 9
            { -50, -100 },  // Type 10
            { -50, 0 },     // Type 11
            { -100, -100 }, // Type 12
            { -120, -120 }, // Type 13
            { 0, 0 },       // Type 14 (invalid)
            { 0, 0 },       // Type 15 (invalid)
            { -100, -100 }  // Type 16
        };

    uint32_t sourceDemonType = (source->GetEntityType() ==
        objects::EntityStateObject::EntityType_t::PARTNER_DEMON)
        ? std::dynamic_pointer_cast<DemonState>(source)->GetEntity()->GetType()
        : 0;
    auto sourceDemonFType = sourceDemonType
        ? definitionManager->GetDevilData(sourceDemonType)->GetFamiliarity()
        ->GetFamiliarityType() : 0;

    std::unordered_map<int32_t, int32_t> adjustments;
    std::list<std::shared_ptr<EnemyState>> enemiesKilled;
    for(auto entity : killed)
    {
        // Remove all opponents
        characterManager->AddRemoveOpponent(false, entity, nullptr);

        // Determine familiarity adjustments
        bool partnerDeath = false;
        uint32_t dType = 0;
        switch(entity->GetEntityType())
        {
        case objects::EntityStateObject::EntityType_t::PARTNER_DEMON:
            dType = std::dynamic_pointer_cast<DemonState>(entity)
                ->GetEntity()->GetType();
            partnerDeath = true;
            break;
        case objects::EntityStateObject::EntityType_t::ENEMY:
            {
                auto eState = std::dynamic_pointer_cast<EnemyState>(entity);
                dType = eState->GetEntity()->GetType();
                enemiesKilled.push_back(eState);
            }
            break;
        default:
            break;
        }

        if(dType)
        {
            std::list<std::pair<int32_t, int32_t>> adjusts;
            if(partnerDeath)
            {
                // Partner demon has died
                adjusts.push_back(std::pair<int32_t, int32_t>(
                    entity->GetEntityID(), fTypeMap[(size_t)sourceDemonFType][0]));
            }

            if(entity != source && sourceDemonType == dType)
            {
                // Same demon type killed
                adjusts.push_back(std::pair<int32_t, int32_t>(
                    source->GetEntityID(), fTypeMap[(size_t)sourceDemonFType][1]));
            }

            for(auto aPair : adjusts)
            {
                if(adjustments.find(aPair.first) == adjustments.end())
                {
                    adjustments[aPair.first] = aPair.second;
                }
                else
                {
                    adjustments[aPair.first] = (int32_t)(
                        adjustments[aPair.first] + aPair.second);
                }
            }
        }
    }

    // Apply familiarity adjustments
    for(auto aPair : adjustments)
    {
        auto demonClient = server->GetManagerConnection()->
            GetEntityClient(aPair.first);
        if(!demonClient) continue;

        characterManager->UpdateFamiliarity(demonClient, aPair.second, true);
    }

    if(enemiesKilled.size() > 0)
    {
        auto sourceClient = server->GetManagerConnection()->
            GetEntityClient(source->GetEntityID());
        auto sourceState = sourceClient ? sourceClient->GetClientState() : nullptr;
        if(!sourceState)
        {
            // Not a player, nothing left to do
            return;
        }

        // Gather all enemy entity IDs
        std::list<int32_t> enemyIDs;
        for(auto eState : enemiesKilled)
        {
            zone->RemoveEntity(eState->GetEntityID());
            enemyIDs.push_back(eState->GetEntityID());
        }

        zoneManager->RemoveEntitiesFromZone(zone, enemyIDs, 4, true);

        // Transform enemies into loot bodies and gather quest kills
        std::unordered_map<std::shared_ptr<LootBoxState>,
            std::shared_ptr<EnemyState>> lStates;
        std::unordered_map<uint32_t, int32_t> questKills;
        for(auto eState : enemiesKilled)
        {
            auto enemy = eState->GetEntity();

            auto lootBody = std::make_shared<objects::LootBox>();
            lootBody->SetType(objects::LootBox::Type_t::BODY);
            lootBody->SetEnemy(enemy);

            auto lState = std::make_shared<LootBoxState>(lootBody);
            lState->SetCurrentX(eState->GetDestinationX());
            lState->SetCurrentY(eState->GetDestinationY());
            lState->SetCurrentRotation(eState->GetDestinationRotation());
            lState->SetEntityID(server->GetNextEntityID());
            lStates[lState] = eState;

            zone->AddLootBox(lState);

            uint32_t dType = eState->GetEntity()->GetType();
            if(sourceState && sourceState->QuestTargetEnemiesContains(dType))
            {
                if(questKills.find(dType) == questKills.end())
                {
                    questKills[dType] = 1;
                }
                else
                {
                    questKills[dType] = (questKills[dType] + 1);
                }
            }
        }

        // For each loot body generate and send loot and show the body
        // After this schedule all of the bodies for cleanup after their
        // loot time passes
        if(lStates.size() > 0)
        {
            uint64_t now = ChannelServer::GetServerTime();
            int16_t luck = source->GetLUCK();

            auto firstClient = zConnections.size() > 0 ? zConnections.front() : nullptr;
            auto sourceParty = sourceState->GetParty();
            std::set<int32_t> sourcePartyMembers = sourceParty
                ? sourceParty->GetMemberIDs() : std::set<int32_t>();

            std::unordered_map<uint64_t, std::list<int32_t>> lootTimeEntityIDs;
            std::unordered_map<uint64_t, std::list<int32_t>> delayedLootEntityIDs;
            for(auto lPair : lStates)
            {
                auto lState = lPair.first;
                auto eState = lPair.second;

                int32_t lootEntityID = lState->GetEntityID();

                auto lootBody = lState->GetEntity();
                auto enemy = lootBody->GetEnemy();
                auto spawn = enemy->GetSpawnSource();

                auto drops = GetItemDrops(enemy->GetType(), spawn);

                // Create loot based off drops and send if any was added
                uint64_t lootTime = 0;
                if(characterManager->CreateLootFromDrops(lootBody, drops, luck))
                {
                    // Bodies remain lootable for 120 seconds with loot
                    lootTime = (uint64_t)(now + 120000000);
                    
                    std::set<int32_t> validLooterIDs = { sourceState->GetWorldCID() };
                    if(sourceParty)
                    {
                        bool timedAdjust = true;
                        switch(sourceParty->GetDropRule())
                        {
                        case objects::Party::DropRule_t::DAMAGE_RACE:
                            {
                                // Highest damage dealer member wins
                                std::map<uint64_t, int32_t> damageMap;
                                for(auto pair : enemy->GetDamageSources())
                                {
                                    damageMap[pair.second] = pair.first;
                                }

                                if(damageMap.size() > 0)
                                {
                                    validLooterIDs = { damageMap.rbegin()->second };
                                }
                            }
                            break;
                        case objects::Party::DropRule_t::RANDOM_LOOT:
                            {
                                // Randomly pick a member
                                auto it = sourcePartyMembers.begin();
                                size_t offset = (size_t)RNG(uint16_t, 0,
                                    (uint16_t)(sourcePartyMembers.size() - 1));
                                std::advance(it, offset);

                                validLooterIDs = { *it };
                            }
                            break;
                        case objects::Party::DropRule_t::FREE_LOOT:
                            {
                                // Every member is valid
                                validLooterIDs = sourcePartyMembers;
                                timedAdjust = false;
                            }
                            break;
                        default:
                            break;
                        }

                        if(timedAdjust)
                        {
                            // The last 60 seconds are fair game for everyone
                            uint64_t delayedlootTime = (uint64_t)(now + 60000000);
                            delayedLootEntityIDs[delayedlootTime].push_back(lootEntityID);
                        }
                    }

                    lootBody->SetValidLooterIDs(validLooterIDs);
                }
                else
                {
                    // Bodies remain lootable for 10 seconds without loot
                    lootTime = (uint64_t)(now + 10000000);
                }

                lootBody->SetLootTime(lootTime);
                lootTimeEntityIDs[lootTime].push_back(lootEntityID);

                if(firstClient)
                {
                    zoneManager->SendLootBoxData(firstClient, lState, eState,
                        true, true);
                }
            }

            for(auto pair : lootTimeEntityIDs)
            {
                zoneManager->ScheduleEntityRemoval(pair.first, zone, pair.second, 13);
            }

            for(auto pair : delayedLootEntityIDs)
            {
                ScheduleFreeLoot(pair.first, zone, pair.second, sourcePartyMembers);
            }
        }

        // Update quest kill counts
        if(questKills.size() > 0)
        {
            server->GetEventManager()->UpdateQuestKillCount(sourceClient,
                questKills);
        }

        ChannelClientConnection::FlushAllOutgoing(zConnections);
    }
}

void SkillManager::HandleNegotiations(const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<Zone> zone,
    const std::list<std::pair<std::shared_ptr<ActiveEntityState>, uint8_t>> talkDone)
{
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto zoneManager = server->GetZoneManager();
    auto zConnections = zone->GetConnectionList();

    // Gather all enemy IDs that will be removed
    std::unordered_map<int32_t, std::list<int32_t>> removedEnemies;
    for(auto pair : talkDone)
    {
        if(pair.second != TALK_REJECT)
        {
            int32_t removeMode = 0;
            switch(pair.second)
            {
            case TALK_JOIN:
            case TALK_JOIN_2:
                removeMode = 5;
                break;
            case TALK_GIVE_ITEM:
            case TALK_GIVE_ITEM_2:
                removeMode = 6;
                break;
            case TALK_LEAVE:
                removeMode = 8;
                break;
            default:
                break;
            }

            // Remove all opponents
            characterManager->AddRemoveOpponent(false, pair.first, nullptr);
            zone->RemoveEntity(pair.first->GetEntityID(), removeMode == 8);
            removedEnemies[removeMode].push_back(pair.first->GetEntityID());
        }
    }

    for(auto pair : removedEnemies)
    {
        zoneManager->RemoveEntitiesFromZone(zone, pair.second, pair.first, true);
    }

    auto sourceClient = server->GetManagerConnection()->
        GetEntityClient(source->GetEntityID());
    auto sourceState = sourceClient ? sourceClient->GetClientState() : nullptr;
    if(!sourceState)
    {
        // Not a player, flush connections and stop
        ChannelClientConnection::FlushAllOutgoing(zConnections);
        return;
    }

    // Handle the results of negotiations that result in an enemy being removed
    std::unordered_map<std::shared_ptr<LootBoxState>,
        std::shared_ptr<EnemyState>> lStates;
    for(auto pair : talkDone)
    {
        auto eState = std::dynamic_pointer_cast<EnemyState>(pair.first);
        if(pair.second != TALK_LEAVE && pair.second != TALK_REJECT)
        {
            auto enemy = eState->GetEntity();

            /// @todo: handle the various outcomes properly
            std::shared_ptr<objects::LootBox> lBox;
            switch(pair.second)
            {
            case TALK_JOIN:
            case TALK_JOIN_2:
                {
                    lBox = std::make_shared<objects::LootBox>();
                    lBox->SetType(objects::LootBox::Type_t::EGG);
                    lBox->SetEnemy(enemy);

                    auto demonLoot = std::make_shared<objects::Loot>();
                    demonLoot->SetType(enemy->GetType());
                    demonLoot->SetCount(1);
                    lBox->SetLoot(0, demonLoot);
                }
                break;
            case TALK_GIVE_ITEM:
            case TALK_GIVE_ITEM_2:
                {
                    lBox = std::make_shared<objects::LootBox>();
                    lBox->SetType(objects::LootBox::Type_t::GIFT_BOX);
                    lBox->SetEnemy(enemy);

                    auto drops = GetItemDrops(enemy->GetType(),
                        enemy->GetSpawnSource(), true);
                    characterManager->CreateLootFromDrops(lBox, drops,
                        source->GetLUCK(), true);
                }
                break;
            default:
                break;
            }

            if(lBox)
            {
                auto lState = std::make_shared<LootBoxState>(lBox);
                lState->SetCurrentX(eState->GetDestinationX());
                lState->SetCurrentY(eState->GetDestinationY());
                lState->SetCurrentRotation(eState->GetDestinationRotation());
                lState->SetEntityID(server->GetNextEntityID());
                lStates[lState] = eState;

                zone->AddLootBox(lState);
            }
        }
    }

    // Show each look box and schedule them for cleanup after their
    // loot time passes
    if(lStates.size() > 0)
    {
        // Spawned boxes remain lootable for 120 seconds
        uint64_t now = ChannelServer::GetServerTime();

        auto firstClient = zConnections.size() > 0 ? zConnections.front() : nullptr;
        auto sourceParty = sourceState->GetParty();
        std::set<int32_t> sourcePartyMembers = sourceParty
            ? sourceParty->GetMemberIDs() : std::set<int32_t>();

        std::unordered_map<uint64_t, std::list<int32_t>> lootTimeEntityIDs;
        std::unordered_map<uint64_t, std::list<int32_t>> delayedLootEntityIDs;
        for(auto lPair : lStates)
        {
            auto lState = lPair.first;
            auto eState = lPair.second;

            auto lootBox = lState->GetEntity();
            lootBox->InsertValidLooterIDs(sourceState->GetWorldCID());

            uint64_t lootTime = 0;
            uint64_t delayedLootTime = 0;
            if(lootBox->GetType() == objects::LootBox::Type_t::EGG)
            {
                // Demon eggs remain lootable for 300 seconds
                lootTime = (uint64_t)(now + 300000000);

                // Free loot starts 120 seconds in
                delayedLootTime = (uint64_t)(now + 120000000);
            }
            else
            {
                // Gift boxes remain lootable for 120 seconds
                lootTime = (uint64_t)(now + 120000000);

                if(sourceParty)
                {
                    if(sourceParty->GetDropRule() ==
                        objects::Party::DropRule_t::FREE_LOOT)
                    {
                        lootBox->SetValidLooterIDs(sourcePartyMembers);
                    }
                    else
                    {
                        // Free loot starts 60 seconds in
                        delayedLootTime = (uint64_t)(now + 60000000);
                    }
                }
            }
            lootBox->SetLootTime(lootTime);

            if(firstClient)
            {
                zoneManager->SendLootBoxData(firstClient, lState, eState,
                    true, true);
            }

            int32_t lootEntityID = lState->GetEntityID();
            lootTimeEntityIDs[lootTime].push_back(lootEntityID);

            if(sourceParty && delayedLootTime)
            {
                delayedLootEntityIDs[delayedLootTime].push_back(lootEntityID);
            }
        }
        
        for(auto pair : lootTimeEntityIDs)
        {
            zoneManager->ScheduleEntityRemoval(pair.first, zone, pair.second, 13);
        }
        
        for(auto pair : delayedLootEntityIDs)
        {
            ScheduleFreeLoot(pair.first, zone, pair.second, sourcePartyMembers);
        }
    }

    ChannelClientConnection::FlushAllOutgoing(zConnections);
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
                    if(RNG(int16_t, 0, 100) <= critRate)
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
                scale = RNG_DEC(float, 0.8f, 0.99f, 2);
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
            if(chance > 0 && RNG(int16_t, 0, 100) <= chance)
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

uint8_t SkillManager::CalculateStatusEffectStack(int8_t minStack, int8_t maxStack) const
{
    // Sanity check
    if(minStack > maxStack)
    {
        return 0;
    }

    return minStack == maxStack ? (uint8_t)maxStack
        : (uint8_t)RNG(uint16_t, (uint16_t)minStack, (uint16_t)maxStack);
}

std::list<std::shared_ptr<objects::ItemDrop>> SkillManager::GetItemDrops(
    uint32_t enemyType, const std::shared_ptr<objects::Spawn>& spawn,
    bool giftMode) const
{
    (void)enemyType;

    std::list<std::shared_ptr<objects::ItemDrop>> drops;

    /// @todo: add global/family drops

    if(spawn)
    {
        for(auto drop : giftMode ? spawn->GetGifts() : spawn->GetDrops())
        {
            drops.push_back(drop);
        }
    }

    return drops;
}

void SkillManager::ScheduleFreeLoot(uint64_t time, const std::shared_ptr<Zone>& zone,
    const std::list<int32_t>& lootEntityIDs, const std::set<int32_t>& worldCIDs)
{
    auto server = mServer.lock();
    server->ScheduleWork(time, [](CharacterManager* characterManager,
        const std::shared_ptr<Zone> pZone, const std::list<int32_t> pLootEntityIDs,
        const std::set<int32_t> pWorldCIDs)
        {
            auto clients = pZone->GetConnectionList();
            for(int32_t lootEntityID : pLootEntityIDs)
            {
                auto lState = pZone->GetLootBox(lootEntityID);
                if(lState)
                {
                    lState->GetEntity()->SetValidLooterIDs(pWorldCIDs);
                    characterManager->SendLootItemData(clients, lState, true);
                }
            }

            ChannelClientConnection::FlushAllOutgoing(clients);
        }, server->GetCharacterManager(), zone, lootEntityIDs, worldCIDs);
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

    return ProcessSkillResult(activated);;
}

bool SkillManager::FamiliarityUp(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(!demon)
    {
        return false;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillData = definitionManager->GetSkillData(activated->GetSkillID());

    // Skills of this type add a "cooldown status effect". If the player character
    // already has it, do not allow the skill's usage
    auto statusEffects = cState->GetStatusEffects();
    for(auto addStatus : skillData->GetDamage()->GetAddStatuses())
    {
        if(statusEffects.find(addStatus->GetStatusID()) != statusEffects.end())
        {
            return false;
        }
    }

    auto demonData = definitionManager->GetDevilData(demon->GetType());
    int32_t fType = demonData
        ? demonData->GetFamiliarity()->GetFamiliarityType() : 0;

    if(!demonData || fType > 16)
    {
        return false;
    }

    // Familiarity is adjusted based on the demon's familiarity type
    // and if it shares the same alignment with the character
    const uint16_t fTypeMap[17][2] =
        {
            { 50, 25 },     // Type 0
            { 4000, 2000 }, // Type 1
            { 2000, 1000 }, // Type 2
            { 550, 225 },   // Type 3
            { 250, 125 },   // Type 4
            { 75, 40 },     // Type 5
            { 2000, 1500 }, // Type 6
            { 500, 375 },   // Type 7
            { 250, 180 },   // Type 8
            { 100, 75 },    // Type 9
            { 50, 38 },     // Type 10
            { 10, 10 },     // Type 11
            { 2000, 200 },  // Type 12
            { 650, 65 },    // Type 13
            { 0, 0 },       // Type 14 (invalid)
            { 0, 0 },       // Type 15 (invalid)
            { 5000, 5000 } // Type 16
        };

    /// @todo: receive items from demon

    bool sameLNC = cState->GetLNCType() == dState->GetLNCType(definitionManager);

    int32_t fPoints = (int32_t)fTypeMap[(size_t)fType][sameLNC ? 0 : 1];
    server->GetCharacterManager()->UpdateFamiliarity(client, fPoints, true);

    // Apply the status effects
    for(auto addStatus : skillData->GetDamage()->GetAddStatuses())
    {
        uint8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
            addStatus->GetMaxStack());
        if(stack == 0 && !addStatus->GetIsReplace()) continue;

        AddStatusEffectMap m;
        m[addStatus->GetStatusID()] =
            std::pair<uint8_t, bool>(stack, addStatus->GetIsReplace());

        cState->AddStatusEffects(m, definitionManager);
    }

    // Process the skill without status effects
    return ProcessSkillResult(activated, false);
}

bool SkillManager::FamiliarityUpItem(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto state = client->GetClientState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(!demon)
    {
        return false;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillData = definitionManager->GetSkillData(activated->GetSkillID());

    auto demonData = definitionManager->GetDevilData(demon->GetType());
    auto special = skillData->GetSpecial();

    int32_t maxFamiliarity = special->GetSpecialParams(0);
    float deltaPercent = (float)special->GetSpecialParams(1);
    int32_t minIncrease = special->GetSpecialParams(2);
    int32_t raceRestrict = special->GetSpecialParams(3);

    if(raceRestrict && (int32_t)demonData->GetCategory()->GetRace() != raceRestrict)
    {
        return false;
    }

    uint16_t currentVal = demon->GetFamiliarity();
    if(maxFamiliarity <= (int32_t)currentVal)
    {
        return true;
    }

    int32_t fPoints = 0;
    if(maxFamiliarity && deltaPercent)
    {
        fPoints = (int32_t)ceill(
            floorl((float)(maxFamiliarity - currentVal) * deltaPercent * 0.01f) - 1);
    }

    if(minIncrease && fPoints < minIncrease)
    {
        fPoints = minIncrease;
    }

    /// @todo: receive items from demon

    server->GetCharacterManager()->UpdateFamiliarity(client, fPoints, true);

    return ProcessSkillResult(activated);
}

bool SkillManager::Mooch(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    (void)activated;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(!demon)
    {
        return false;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillData = definitionManager->GetSkillData(activated->GetSkillID());

    // Skills of this type add a "cooldown status effect". If the player character
    // already has it, do not allow the skill's usage
    auto statusEffects = cState->GetStatusEffects();
    for(auto addStatus : skillData->GetDamage()->GetAddStatuses())
    {
        if(statusEffects.find(addStatus->GetStatusID()) != statusEffects.end())
        {
            return false;
        }
    }

    /// @todo: receive items from demon

    mServer.lock()->GetCharacterManager()->UpdateFamiliarity(client, -2000, true);

    // Apply the status effects
    for(auto addStatus : skillData->GetDamage()->GetAddStatuses())
    {
        uint8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
            addStatus->GetMaxStack());
        if(stack == 0 && !addStatus->GetIsReplace()) continue;

        AddStatusEffectMap m;
        m[addStatus->GetStatusID()] =
            std::pair<uint8_t, bool>(stack, addStatus->GetIsReplace());

        cState->AddStatusEffects(m, definitionManager);
    }

    // Process the skill without status effects
    return ProcessSkillResult(activated, false);
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

    ProcessSkillResult(activated);

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
