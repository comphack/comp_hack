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
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include <ErrorCodes.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ActionSpawn.h>
#include <ActivatedAbility.h>
#include <CalculatedEntityState.h>
#include <CharacterProgress.h>
#include <DropSet.h>
#include <Item.h>
#include <ItemBox.h>
#include <ItemDrop.h>
#include <Loot.h>
#include <LootBox.h>
#include <MiAddStatusTbl.h>
#include <MiBattleDamageData.h>
#include <MiCancelData.h>
#include <MiCastBasicData.h>
#include <MiCastCancelData.h>
#include <MiCastData.h>
#include <MiCategoryData.h>
#include <MiConditionData.h>
#include <MiCostTbl.h>
#include <MiDamageData.h>
#include <MiDCategoryData.h>
#include <MiDevilBookData.h>
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
#include <SpawnGroup.h>
#include <Tokusei.h>
#include <TokuseiCondition.h>
#include <TokuseiSkillCondition.h>

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
const uint16_t FLAG1_GUARDED = 1 << 3;
const uint16_t FLAG1_COUNTERED = 1 << 4;
const uint16_t FLAG1_DODGED = 1 << 5;
const uint16_t FLAG1_CRITICAL = 1 << 6;
const uint16_t FLAG1_WEAKPOINT = 1 << 7;
const uint16_t FLAG1_KNOCKBACK = 1 << 8;
const uint16_t FLAG1_RUSH_MOVEMENT = 1 << 14;
const uint16_t FLAG1_PROTECT = 1 << 15;

// Only displayed with DAMAGE_TYPE_HEALING
const uint16_t FLAG1_REVIVAL = 1 << 9;
const uint16_t FLAG1_ABSORB = 1 << 10;

// Only displayed with DAMAGE_TYPE_NONE
const uint16_t FLAG1_REFLECT_PHYS = 1 << 9;
const uint16_t FLAG1_BLOCK_PHYS = 1 << 10;
const uint16_t FLAG1_REFLECT_MAGIC = 1 << 11;
const uint16_t FLAG1_BLOCK_MAGIC = 1 << 12;
//const uint16_t FLAG1_REFLECT_UNUSED = 1 << 13;

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
const uint8_t AIL_OFFSET = (uint8_t)((uint8_t)CorrectTbl::RES_FIRE -
    (uint8_t)CorrectTbl::RES_SLASH + 1);
/*const uint8_t DAMAGE_TAKEN_OFFSET = (uint8_t)((uint8_t)CorrectTbl::RATE_CLSR -
    (uint8_t)CorrectTbl::RATE_CLSR_TAKEN);*/

class channel::ProcessingSkill
{
public:
    uint32_t SkillID = 0;
    std::shared_ptr<objects::MiSkillData> Definition;
    std::shared_ptr<objects::ActivatedAbility> Activated;
    SkillExecutionContext* ExecutionContext = 0;
    uint8_t BaseAffinity = 0;
    uint8_t EffectiveAffinity = 0;
    uint8_t WeaponAffinity = 0;
    uint8_t EffectiveDependencyType = 0;
    uint16_t OffenseValue = 0;
    std::unordered_map<int32_t, uint16_t> OffenseValues;
    bool IsSuicide = false;
    bool IsItemSkill = false;

    std::shared_ptr<ActiveEntityState> EffectiveSource;
    std::list<channel::SkillTargetResult> Targets;
    std::shared_ptr<ActiveEntityState> PrimaryTarget;
    std::shared_ptr<objects::CalculatedEntityState> SourceExecutionState;
    std::unordered_map<int32_t,
        std::shared_ptr<objects::CalculatedEntityState>> SourceCalcStates;
    std::unordered_map<int32_t,
        std::shared_ptr<objects::CalculatedEntityState>> TargetCalcStates;
};

class channel::SkillTargetResult
{
public:
    std::shared_ptr<ActiveEntityState> EntityState;
    std::shared_ptr<objects::CalculatedEntityState> CalcState;
    bool PrimaryTarget = false;
    bool IndirectTarget = false;
    int32_t Damage1 = 0;
    uint8_t Damage1Type = DAMAGE_TYPE_NONE;
    int32_t Damage2 = 0;
    uint8_t Damage2Type = DAMAGE_TYPE_NONE;
    uint16_t Flags1 = 0;
    uint8_t TalkFlags = 0;
    uint8_t AilmentDamageType = 0;
    int32_t AilmentDamage = 0;
    uint64_t AilmentDamageTime = 0;
    uint16_t Flags2 = 0;
    int32_t TechnicalDamage = 0;
    int32_t PursuitDamage = 0;
    uint8_t PursuitAffinity = 0;
    AddStatusEffectMap AddedStatuses;
    std::set<uint32_t> CancelledStatuses;
    bool HitAvoided = false;
    uint8_t HitNull = 0;    // 0: None, 1: Physical, 2: Magic
    uint8_t HitReflect = 0; // 0: None, 1: Physical, 2: Magic
    bool HitAbsorb = false;
    bool CanHitstun = false;
    bool CanKnockback = false;
    uint16_t GuardModifier = 0;
};

SkillManager::SkillManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
    mSkillFunctions[SVR_CONST.SKILL_CLAN_FORM] = &SkillManager::SpecialSkill;
    mSkillFunctions[SVR_CONST.SKILL_DCM] = &SkillManager::DCM;
    mSkillFunctions[SVR_CONST.SKILL_EQUIP_ITEM] = &SkillManager::EquipItem;
    mSkillFunctions[SVR_CONST.SKILL_EQUIP_MOD_EDIT] = &SkillManager::SpecialSkill;
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

bool SkillManager::ActivateSkill(const std::shared_ptr<ActiveEntityState> source,
    uint32_t skillID, int64_t targetObjectID, std::shared_ptr<SkillExecutionContext> ctx)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetSkillData(skillID);
    if (nullptr == def)
    {
        return 0;
    }

    auto client = server->GetManagerConnection()->GetEntityClient(
        source->GetEntityID());

    auto cast = def->GetCast();
    auto activatedTime = ChannelServer::GetServerTime();

    auto activated = std::shared_ptr<objects::ActivatedAbility>(
        new objects::ActivatedAbility);
    activated->SetSkillID(skillID);
    activated->SetSourceEntity(source);
    activated->SetActivationObjectID(targetObjectID);
    activated->SetTargetObjectID(targetObjectID);
    activated->SetActivationTime(activatedTime);
    activated->SetActivationID(source->GetNextActivatedAbilityID());

    // If the skill needs to charge, see if any time adjustments exist.
    // This will never reduce to 0% time so storing the context is not
    // necessary.
    uint32_t chargeTime = cast->GetBasic()->GetChargeTime();
    if(chargeTime > 0)
    {
        auto pSkill = GetProcessingSkill(activated, nullptr, client);
        auto calcState = GetCalculatedState(source, pSkill, false, nullptr);

        int16_t chargeAdjust = source->GetCorrectValue(
            CorrectTbl::CHANT_TIME, calcState);
        if(chargeAdjust != 100)
        {
            chargeTime = (uint32_t)ceill(chargeTime *
                (chargeAdjust * 0.01));
        }
    }

    // Charge time is in milliseconds, convert to microseconds
    auto chargedTime = activatedTime + (chargeTime * 1000);

    activated->SetChargedTime(chargedTime);

    float chargeSpeed = 0.f, chargeCompleteSpeed = 0.f;

    // Send movement speed based off skill action type
    switch(def->GetBasic()->GetActionType())
    {
    case objects::MiSkillBasicData::ActionType_t::SPIN:
    case objects::MiSkillBasicData::ActionType_t::RAPID:
    case objects::MiSkillBasicData::ActionType_t::COUNTER:
    case objects::MiSkillBasicData::ActionType_t::DODGE:
        // No movement during or after
        break;
    case objects::MiSkillBasicData::ActionType_t::SHOT:
    case objects::MiSkillBasicData::ActionType_t::TALK:
    case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
    case objects::MiSkillBasicData::ActionType_t::SUPPORT:
        // Move after only
        chargeCompleteSpeed = source->GetMovementSpeed();
        break;
    case objects::MiSkillBasicData::ActionType_t::GUARD:
        // Move during and after charge (1/2 normal speed)
        chargeSpeed = chargeCompleteSpeed = (source->GetMovementSpeed() * 0.5f);
        break;
    case objects::MiSkillBasicData::ActionType_t::ATTACK:
    case objects::MiSkillBasicData::ActionType_t::RUSH:
    default:
        // Move during and after charge (normal speed)
        chargeSpeed = chargeCompleteSpeed = source->GetMovementSpeed();
        break;
    }

    activated->SetChargeMoveSpeed(chargeSpeed);
    activated->SetChargeCompleteMoveSpeed(chargeCompleteSpeed);

    source->SetActivatedAbility(activated);

    SendActivateSkill(activated, def);
    
    auto functionID = def->GetDamage()->GetFunctionID();
    if(functionID == SVR_CONST.SKILL_REST)
    {
        return Rest(activated, ctx, nullptr);
    }

    uint8_t activationType = def->GetBasic()->GetActivationType();
    bool executeNow = (activationType == 3 || activationType == 4)
        && chargeTime == 0;
    if(executeNow)
    {
        if(!ExecuteSkill(source, activated, client, ctx))
        {
            return false;
        }
    }
    else
    {
        source->SetStatusTimes(STATUS_CHARGING, chargedTime);
    }

    return true;
}

bool SkillManager::ExecuteSkill(const std::shared_ptr<ActiveEntityState> source,
    uint8_t activationID, int64_t targetObjectID, std::shared_ptr<SkillExecutionContext> ctx)
{
    auto client = mServer.lock()->GetManagerConnection()->GetEntityClient(
        source->GetEntityID());

    bool success = true;

    auto activated = source->GetActivatedAbility();
    if(nullptr == activated || activationID != activated->GetActivationID())
    {
        LOG_ERROR(libcomp::String("Unknown activation ID encountered: %1\n")
            .Arg(activationID));
        success = false;
    }
    else
    {
        activated->SetTargetObjectID(targetObjectID);
    }

    if(!success || !ExecuteSkill(source, activated, client, ctx))
    {
        success = false;
    }

    return success;
}

bool SkillManager::ExecuteSkill(std::shared_ptr<ActiveEntityState> source,
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<SkillExecutionContext> ctx)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillID = activated->GetSkillID();
    auto skillData = definitionManager->GetSkillData(skillID);

    if(nullptr == skillData)
    {
        LOG_ERROR(libcomp::String("Unknown skill ID encountered: %1\n")
            .Arg(skillID));
        SendFailure(source, skillID, client);
        return false;
    }

    uint16_t functionID = skillData->GetDamage()->GetFunctionID();
    uint8_t skillCategory = skillData->GetCommon()->GetCategory()->GetMainCategory();

    if(skillCategory == 0)
    {
        SendFailure(source, skillID, client);
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
            SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
            return false;
        }
    }

    // Verify the target now
    auto targetType = skillData->GetTarget()->GetType();
    if(targetType != objects::MiTargetData::Type_t::NONE &&
        targetType != objects::MiTargetData::Type_t::OBJECT)
    {
        auto targetEntityID = (int32_t)activated->GetTargetObjectID();

        if(targetEntityID <= 0)
        {
            //No target
            SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
            return false;
        }

        auto zone = source->GetZone();
        if(nullptr == zone)
        {
            LOG_ERROR("Skill activation attempted outside of a zone.\n");
            SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
            return false;
        }

        auto targetEntity = zone->GetActiveEntity(targetEntityID);
        if(nullptr == targetEntity || !targetEntity->Ready())
        {
            LOG_ERROR(libcomp::String("Invalid target ID encountered: %1\n")
                .Arg(targetEntityID));
            SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
            return false;
        }

        bool targetAlive = targetEntity->IsAlive();
        bool allies = targetEntity->GetFaction() == source->GetFaction();
        auto targetEntityType = targetEntity->GetEntityType();
        auto sourceState = ClientState::GetEntityClientState(source->GetEntityID());
        auto targetState = ClientState::GetEntityClientState(targetEntity->GetEntityID());

        // If using a (primary) talk skill on an entity with 100% talk resist
        // stop the skill
        switch(skillData->GetBasic()->GetActionType())
        {
        case objects::MiSkillBasicData::ActionType_t::TALK:
        case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
        case objects::MiSkillBasicData::ActionType_t::TAUNT:
            if(targetEntityType == objects::EntityStateObject::EntityType_t::ENEMY)
            {
                auto enemyState = std::dynamic_pointer_cast<EnemyState>(targetEntity);
                auto enemy = enemyState ? enemyState->GetEntity() : nullptr;
                auto spawn = enemy ? enemy->GetSpawnSource() : nullptr;
                if(!spawn || spawn->GetTalkResist() >= 100)
                {
                    SendFailure(source, skillID, client,
                        (uint8_t)SkillErrorCodes_t::TALK_INVALID);
                    return false;
                }

                auto cs = targetEntity->GetCoreStats();
                if(cs && cs->GetLevel() > source->GetCoreStats()->GetLevel())
                {
                    SendFailure(source, skillID, client,
                        (uint8_t)SkillErrorCodes_t::TALK_LEVEL);
                    return false;
                }
            }
            break;
        default:
            break;
        }

        bool targetInvalid = false;
        switch(targetType)
        {
        case objects::MiTargetData::Type_t::ALLY:
            targetInvalid = !allies || !targetAlive;
            break;
        case objects::MiTargetData::Type_t::DEAD_ALLY:
            targetInvalid = !allies || targetAlive;
            break;
        case objects::MiTargetData::Type_t::PARTNER:
            targetInvalid = !sourceState || sourceState->GetCharacterState() != source ||
                sourceState->GetDemonState() != targetEntity || !targetAlive;
            break;
        case objects::MiTargetData::Type_t::PARTY:
            targetInvalid = !sourceState || sourceState->GetPartyID() == 0 || !targetState ||
                sourceState->GetPartyID() != targetState->GetPartyID() || !targetAlive;
            break;
        case objects::MiTargetData::Type_t::ENEMY:
            targetInvalid = allies || !targetAlive;
            break;
        case objects::MiTargetData::Type_t::DEAD_PARTNER:
            targetInvalid = !sourceState || sourceState->GetCharacterState() != source ||
                sourceState->GetDemonState() != targetEntity || targetAlive;
            break;
        case objects::MiTargetData::Type_t::OTHER_PLAYER:
            targetInvalid = targetEntityType != objects::EntityStateObject::EntityType_t::CHARACTER ||
                sourceState == targetState || !allies || !targetAlive;
            break;
        case objects::MiTargetData::Type_t::OTHER_DEMON:
            targetInvalid = targetEntityType != objects::EntityStateObject::EntityType_t::PARTNER_DEMON ||
                (sourceState && sourceState->GetDemonState() != targetEntity) || !allies ||
                !targetAlive;
            break;
        case objects::MiTargetData::Type_t::ALLY_PLAYER:
            targetInvalid = targetEntityType != objects::EntityStateObject::EntityType_t::CHARACTER ||
                !allies || !targetAlive;
            break;
        case objects::MiTargetData::Type_t::ALLY_DEMON:
            targetInvalid = targetEntityType != objects::EntityStateObject::EntityType_t::PARTNER_DEMON ||
                !allies || !targetAlive;
            break;
        case objects::MiTargetData::Type_t::PLAYER:
            targetInvalid = !sourceState || sourceState->GetCharacterState() != targetEntity;
            break;
        default:
            break;
        }

        if(targetInvalid)
        {
            SendFailure(source, skillID, client, (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
            return false;
        }

        activated->SetEntityTargeted(true);
    }

    // Make sure we have an execution context
    if(!ctx)
    {
        ctx = std::make_shared<SkillExecutionContext>();
    }

    auto pSkill = GetProcessingSkill(activated, ctx, client);
    pSkill->SourceExecutionState = GetCalculatedState(source, pSkill, false, nullptr);

    // Check costs and pay costs (skip for switch deactivation)
    int32_t hpCost = 0, mpCost = 0;
    uint16_t bulletCost = 0;
    std::unordered_map<uint32_t, uint32_t> itemCosts;
    if(!ctx->FreeCast && (skillCategory == 1 || (skillCategory == 2 &&
        !source->ActiveSwitchSkillsContains(skillID))))
    {
        uint32_t hpCostPercent = 0, mpCostPercent = 0;
        if(functionID == SVR_CONST.SKILL_SUMMON_DEMON)
        {
            if(client)
            {
                auto state = client->GetClientState();
                auto character = state->GetCharacterState()->GetEntity();

                auto demon = std::dynamic_pointer_cast<objects::Demon>(
                    libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(
                    activated->GetActivationObjectID())));
                if(demon == nullptr)
                {
                    LOG_ERROR("Attempted to summon a demon that does not exist.\n");
                    SendFailure(source, skillID, client,
                        (uint8_t)SkillErrorCodes_t::SUMMON_INVALID);
                    return false;
                }

                // Calculate MAG cost
                uint32_t demonType = demon->GetType();
                auto demonStats = demon->GetCoreStats().Get();
                auto demonData = definitionManager->GetDevilData(demonType);

                int16_t characterLNC = character->GetLNC();
                int16_t demonLNC = demonData->GetBasic()->GetLNC();
                int8_t level = demonStats->GetLevel();
                uint8_t magMod = demonData->GetSummonData()->GetMagModifier();

                double lncAdjust = characterLNC == 0
                    ? pow(demonLNC, 2.0f)
                    : (pow(abs(characterLNC), -0.06f) * pow(characterLNC - demonLNC, 2.0f));
                double magAdjust = (double)(level * magMod);

                double mag = (magAdjust * lncAdjust / 18000000.0) + (magAdjust * 0.25);

                itemCosts[SVR_CONST.ITEM_MAGNETITE] = (uint32_t)round(mag);
            }
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
                        hpCostPercent = (uint32_t)(hpCostPercent + num);
                    }
                    else
                    {
                        hpCost = (int32_t)(hpCost + num);
                    }
                    break;
                case objects::MiCostTbl::Type_t::MP:
                    if(percentCost)
                    {
                        mpCostPercent = (uint32_t)(mpCostPercent + num);
                    }
                    else
                    {
                        mpCost = (int32_t)(mpCost + num);
                    }
                    break;
                case objects::MiCostTbl::Type_t::ITEM:
                    if(percentCost)
                    {
                        LOG_ERROR("Item percent cost encountered.\n");
                        SendFailure(source, skillID, client);
                        return false;
                    }
                    else
                    {
                        auto itemID = cost->GetItem();
                        if(itemCosts.find(itemID) == itemCosts.end())
                        {
                            itemCosts[itemID] = 0;
                        }
                        itemCosts[itemID] = (uint32_t)(itemCosts[itemID] + num);
                    }
                    break;
                case objects::MiCostTbl::Type_t::BULLET:
                    if(percentCost)
                    {
                        LOG_ERROR("Bullet percent cost encountered.\n");
                        SendFailure(source, skillID, client);
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
                    SendFailure(source, skillID, client);
                    return false;
                }
            }
        }

        auto tokuseiManager = server->GetTokuseiManager();

        // Get final HP cost
        if(hpCost != 0 || hpCostPercent != 0)
        {
            hpCost = (int32_t)(hpCost + ceil(((float)hpCostPercent * 0.01f) *
                (float)source->GetMaxHP()));

            double hpAdjustment = tokuseiManager->GetAspectSum(source,
                TokuseiAspectType::HP_COST_ADJUST, pSkill->SourceExecutionState) * 0.01;
            if(hpAdjustment != 0.0)
            {
                hpCost = (int32_t)(hpCost + floor((double)hpCost * hpAdjustment));
            }

            if(hpCost < 0)
            {
                hpCost = 0;
            }
        }

        // Get final MP cost
        if(mpCost != 0 || mpCostPercent != 0)
        {
            mpCost = (int32_t)(mpCost + ceil(((float)mpCostPercent * 0.01f) *
                (float)source->GetMaxMP()));

            double mpAdjustment = tokuseiManager->GetAspectSum(source,
                TokuseiAspectType::MP_COST_ADJUST, pSkill->SourceExecutionState) * 0.01;
            if(mpAdjustment != 0.0)
            {
                mpCost = (int32_t)(mpCost + floor((double)mpCost * mpAdjustment));
            }

            if(mpCost < 0)
            {
                mpCost = 0;
            }
        }

        auto sourceStats = source->GetCoreStats();
        bool canPay = ((hpCost == 0) || hpCost < sourceStats->GetHP()) &&
            ((mpCost == 0) || mpCost <= sourceStats->GetMP());

        auto characterManager = server->GetCharacterManager();

        if(itemCosts.size() > 0 || bulletCost > 0)
        {
            if(client)
            {
                auto state = client->GetClientState();
                auto cState = state->GetCharacterState();
                auto character = cState->GetEntity();

                for(auto itemCost : itemCosts)
                {
                    auto existingItems = characterManager->GetExistingItems(character,
                        itemCost.first);
                    uint32_t itemCount = 0;
                    for(auto item : existingItems)
                    {
                        itemCount = (uint32_t)(itemCount + item->GetStackSize());
                    }

                    if(itemCount < itemCost.second)
                    {
                        canPay = false;
                        break;
                    }
                }

                if(bulletCost > 0)
                {
                    auto bullets = character->GetEquippedItems((size_t)
                        objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BULLETS);
                    if(!bullets || bullets->GetStackSize() < bulletCost)
                    {
                        canPay = false;
                    }
                }
            }
            else
            {
                // Non-player entities cannot pay item-based costs
                canPay = false;
            }
        }

        // Handle costs that can't be paid as expected errors
        if(!canPay)
        {
            SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::GENERIC_COST);
            return false;
        }

        activated->SetHPCost(hpCost);
        activated->SetMPCost(mpCost);
        activated->SetBulletCost(bulletCost);
        activated->SetItemCosts(itemCosts);
    }

    activated->SetExecutionTime(ChannelServer::GetServerTime());

    // Execute the skill
    auto fIter = mSkillFunctions.find(functionID);
    if(fIter == mSkillFunctions.end())
    {
        switch(skillCategory)
        {
        case 1:
            // Active
            return ExecuteNormalSkill(client, activated, ctx);
        case 2:
            // Switch
            return ToggleSwitchSkill(client, activated, ctx);
        case 0:
            // Passive, shouldn't happen
        default:
            SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::GENERIC_USE);
            return false;
            break;
        }
    }

    // Only execute special function skills if the source was a player
    bool success = client && fIter->second(*this, activated, ctx, client);
    if(success)
    {
        FinalizeSkillExecution(client, ctx, activated);
    }
    else
    {
        SendCompleteSkill(activated, 1);
        source->SetActivatedAbility(nullptr);
    }

    return success;
}

bool SkillManager::CancelSkill(const std::shared_ptr<ActiveEntityState> source,
    uint8_t activationID)
{
    auto activated = source ? source->GetActivatedAbility() : nullptr;
    if(nullptr == activated || activationID != activated->GetActivationID())
    {
        LOG_ERROR(libcomp::String("Unknown activation ID encountered: %1\n")
            .Arg(activationID));
        return false;
    }
    else
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();
        auto skillData = definitionManager->GetSkillData(activated->GetSkillID());

        auto functionID = skillData->GetDamage()->GetFunctionID();
        if(functionID == SVR_CONST.SKILL_REST)
        {
            Rest(activated, nullptr, nullptr);
        }

        // If any executions have occurred, the cooldown needs to be activated
        if(activated->GetExecuteCount() > 0)
        {
            auto pSkill = GetProcessingSkill(activated, nullptr, nullptr);
            SetSkillCompleteState(pSkill, false);
        }

        SendCompleteSkill(activated, 1);
        source->SetActivatedAbility(nullptr);
        return true;
    }
}

void SkillManager::SendFailure(const std::shared_ptr<ActiveEntityState> source,
    uint32_t skillID, const std::shared_ptr<ChannelClientConnection> client,
    uint8_t errorCode)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_FAILED);
    p.WriteS32Little(source ? source->GetEntityID() : -1);
    p.WriteU32Little(skillID);
    p.WriteS8(-1);  //Unknown
    p.WriteU8(0);  //Unknown
    p.WriteU8(errorCode);
    p.WriteS32Little(-1);  //Unknown

    if(client)
    {
        client->SendPacket(p);
    }
    else if(source->GetZone())
    {
        auto zConnections = source->GetZone()->GetConnectionList();
        ChannelClientConnection::BroadcastPacket(zConnections, p);
    }
}

bool SkillManager::ExecuteNormalSkill(const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<SkillExecutionContext>& ctx)
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
        FinalizeSkillExecution(client, ctx, activated);
        if(!ProcessSkillResult(activated, ctx))
        {
            SendFailure(source, skillID, client);
            return false;
        }

        return true;
    }
    else
    {
        // Check for the target
        auto targetEntityID = (int32_t)activated->GetTargetObjectID();
        auto target = zone->GetActiveEntity(targetEntityID);

        // If it isn't valid at this point, fail the skill
        if(!target)
        {
            SendFailure(source, skillID, client);
            return false;
        }

        // Determine time from projectile speed and distance
        source->RefreshCurrentPosition(activated->GetExecutionTime());
        target->RefreshCurrentPosition(activated->GetExecutionTime());

        float distance = source->GetDistance(target->GetCurrentX(),
            target->GetCurrentY());
        uint16_t maxTargetRange = (uint16_t)(400 + (skillData->GetTarget()->GetRange() * 10));
        if((float)maxTargetRange < distance)
        {
            // Out of range, fail execution
            SendFailure(source, skillID, client);
            return false;
        }

        // Complete the skill, calculate damage and effects when the projectile hits
        FinalizeSkillExecution(client, ctx, activated);

        // Projectile speed is measured in how many 10ths of a unit the projectile will
        // traverse per millisecond (with a half second delay for the default cast to projectile
        // move speed)
        uint64_t addMicro = (uint64_t)((double)distance / (projectileSpeed * 10)) * 1000000;
        uint64_t processTime = (activated->GetExecutionTime() + addMicro) + 500000ULL;

        server->ScheduleWork(processTime, [](const std::shared_ptr<ChannelServer> pServer,
            const std::shared_ptr<objects::ActivatedAbility> pActivated,
            const std::shared_ptr<SkillExecutionContext> pCtx)
            {
                pServer->GetSkillManager()->ProcessSkillResult(pActivated, pCtx);
            }, server, activated, ctx);
    }

    return true;
}

bool SkillManager::ProcessSkillResult(std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<SkillExecutionContext> ctx)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());
    auto zone = source->GetZone();
    if(!zone)
    {
        return false;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillID = activated->GetSkillID();
    auto skillData = definitionManager->GetSkillData(skillID);

    auto pSkill = GetProcessingSkill(activated, ctx, nullptr);
    ProcessingSkill& skill = *pSkill.get();

    // Get the target of the spell, keeping track of initial NRA
    uint8_t initialHitNull = 0;
    uint8_t initialHitReflect = 0;
    bool initialHitAbsorb = false;
    switch(skillData->GetTarget()->GetType())
    {
    case objects::MiTargetData::Type_t::NONE:
        // Source is technically the primary target (though most of
        // these types of skills will filter it out)
        skill.PrimaryTarget = source;
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

            if(targetEntity != source)
            {
                // Rotate the source to face the target
                float destRot = (float)atan2(
                    source->GetCurrentY() - targetEntity->GetCurrentY(),
                    source->GetCurrentX() - targetEntity->GetCurrentX());
                source->SetCurrentRotation(destRot);
                source->SetOriginRotation(destRot);
                source->SetDestinationRotation(destRot);
            }

            SkillTargetResult target;
            target.EntityState = targetEntity;
            target.CalcState = GetCalculatedState(targetEntity, pSkill, true, source);
            GetCalculatedState(source, pSkill, false, targetEntity);

            if(SetNRA(target, skill))
            {
                // The skill is reflected and the source becomes
                // the primary target
                skill.PrimaryTarget = source;
                skill.EffectiveSource = targetEntity;
                skill.Targets.push_back(target);

                // Determine NRA for reflect
                SkillTargetResult selfTarget;
                selfTarget.EntityState = source;
                selfTarget.CalcState = GetCalculatedState(source, pSkill, true, source);
                GetCalculatedState(source, pSkill, false, source);
                SetNRA(selfTarget, skill);

                initialHitNull = selfTarget.HitNull;
                initialHitReflect = selfTarget.HitReflect;
                initialHitAbsorb = selfTarget.HitAbsorb;
            }
            else
            {
                skill.PrimaryTarget = targetEntity;

                initialHitNull = target.HitNull;
                initialHitAbsorb = target.HitAbsorb;

                // If it had been reflected we wouldn't be here!
                initialHitReflect = 0;
            }
        }
        break;
    case objects::MiTargetData::Type_t::OBJECT:
        // Nothing special to do (for now)
        break;
    default:
        LOG_ERROR(libcomp::String("Unknown target type encountered: %1\n")
            .Arg((uint8_t)skillData->GetTarget()->GetType()));
        return false;
    }

    auto effectiveSource = skill.EffectiveSource;
    auto primaryTarget = skill.PrimaryTarget;

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
            // If the primary target is set and NRA did not occur, gather other targets
            if(primaryTarget && !initialHitNull && !initialHitReflect && !initialHitAbsorb)
            {
                effectiveTargets = zone->GetActiveEntitiesInRadius(
                    primaryTarget->GetCurrentX(), primaryTarget->GetCurrentY(),
                    aoeRange);
            }
            break;
        case objects::MiEffectiveRangeData::AreaType_t::FRONT_1:
        case objects::MiEffectiveRangeData::AreaType_t::FRONT_2:
        case objects::MiEffectiveRangeData::AreaType_t::FRONT_3:
            {
                /// @todo: figure out how these 3 differ

                float sourceX = effectiveSource->GetCurrentX();
                float sourceY = effectiveSource->GetCurrentY();

                double maxTargetRange = (double)(skillData->GetTarget()->GetRange() * 10);

                // Get entities in range using the target distance
                auto potentialTargets = zone->GetActiveEntitiesInRadius(
                    sourceX, sourceY, maxTargetRange);

                // Center pointer of the arc
                float sourceRot = ActiveEntityState::CorrectRotation(
                    effectiveSource->GetCurrentRotation());

                // AoE range for this is the percentage of a half circle included on either side
                // (ex: 20 would mean 20% of a full radian on both sides is included and 100 would
                // behave like a source radius AoE)
                float maxRotOffset = (float)aoeRange * 0.001f * 3.14f;

                effectiveTargets = ZoneManager::GetEntitiesInFoV(potentialTargets, sourceX,
                    sourceY, sourceRot, maxRotOffset);
            }
            break;
        case objects::MiEffectiveRangeData::AreaType_t::STRAIGHT_LINE:
            if(primaryTarget)
            {
                // Create a rotated rectangle to represent the line with
                // a designated width equal to the AoE range

                Point src(effectiveSource->GetCurrentX(),
                    effectiveSource->GetCurrentY());

                Point dest(primaryTarget->GetCurrentX(),
                    primaryTarget->GetCurrentY());

                float lineWidth = (float)aoeRange * 0.5f;
                
                std::list<Point> rect;
                if(dest.y != src.y)
                {
                    // Set the line rectangle corner points from the source,
                    // destination and perpendicular slope

                    float pSlope = ((dest.x - src.x)/(dest.y - src.y)) * -1.f;
                    float denom = (float)std::sqrt(1.0f + std::pow(pSlope, 2));

                    float xOffset = (float)(lineWidth / denom);
                    float yOffset = (float)fabs((pSlope * lineWidth) / denom);

                    if(pSlope > 0)
                    {
                        rect.push_back(Point(src.x + xOffset, src.y + yOffset));
                        rect.push_back(Point(src.x - xOffset, src.y - yOffset));
                        rect.push_back(Point(dest.x - xOffset, dest.y - yOffset));
                        rect.push_back(Point(dest.x + xOffset, dest.y + yOffset));
                    }
                    else
                    {
                        rect.push_back(Point(src.x - xOffset, src.y + yOffset));
                        rect.push_back(Point(src.x + xOffset, src.y - yOffset));
                        rect.push_back(Point(dest.x - xOffset, dest.y + yOffset));
                        rect.push_back(Point(dest.x + xOffset, dest.y - yOffset));
                    }
                }
                else if(dest.x != src.x)
                {
                    // Horizontal line, add points directly to +Y/-Y
                    rect.push_back(Point(src.x, src.y + lineWidth));
                    rect.push_back(Point(src.x, src.y - lineWidth));
                    rect.push_back(Point(dest.x, dest.y - lineWidth));
                    rect.push_back(Point(dest.x, dest.y + lineWidth));
                }
                else
                {
                    // Same point, only add the target
                    effectiveTargets.push_back(primaryTarget);
                    break;
                }

                for(auto t : zone->GetActiveEntities())
                {
                    Point p(t->GetCurrentX(), t->GetCurrentY());
                    if(ZoneManager::PointInPolygon(p, rect))
                    {
                        effectiveTargets.push_back(t);
                    }
                }
            }
            break;
        case objects::MiEffectiveRangeData::AreaType_t::UNKNOWN_9:
        default:
            LOG_ERROR(libcomp::String("Unsupported skill area type encountered: %1\n")
                .Arg((uint8_t)skillRange->GetAreaType()));
            return false;
        }
    }

    // Remove all targets that are not ready
    effectiveTargets.remove_if([](
        const std::shared_ptr<ActiveEntityState>& target)
        {
            return !target->Ready();
        });

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
        target.CalcState = GetCalculatedState(effectiveTarget, pSkill, true,
            source);
        GetCalculatedState(source, pSkill, false, effectiveTarget);

        // Set NRA
        // If the primary target is still in the set and a reflect did not
        // occur, apply the initially calculated flags first
        // If an AOE target that is not the source is in the set, increase
        // the number of AOE reflections as needed
        bool isSource = effectiveTarget == source;
        if(isPrimaryTarget && !initialHitReflect)
        {
            target.HitNull = initialHitNull;
            target.HitAbsorb = initialHitAbsorb;
            target.HitAvoided = initialHitNull != 0;
        }
        else if(SetNRA(target, skill) && !isSource)
        {
            aoeReflect++;
        }

        skill.Targets.push_back(target);

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

        // Calculate the effects done to and from the source itself
        target.CalcState = GetCalculatedState(source, pSkill, true, source);
        GetCalculatedState(source, pSkill, false, source);

        skill.Targets.push_back(target);
        SetNRA(target, skill);
    }

    // If this is a counter, defer final processing to the skill being
    // countered (see below)
    if(ctx->CounteredSkill)
    {
        ctx->CounteredSkill->ExecutionContext->CounteringSkills.push_back(pSkill);
    }
    else
    {
        // Determine hit outcomes
        CheckSkillHits(source, pSkill);

        // Finalize the skill processing
        ProcessSkillResultFinal(pSkill);

        // Lastly if the skill was countered, finalize those too
        if(ctx->CounteringSkills.size() > 0)
        {
            for(auto counteringSkill : ctx->CounteringSkills)
            {
                ProcessSkillResultFinal(counteringSkill);

                // Now that we're done make sure we clean up context pointer
                counteringSkill->ExecutionContext = nullptr;
            }
        }

        // Clean up the related contexts as they are no longer needed
        ctx->CounteringSkills.clear();
        ctx->SubContexts.clear();
    }

    return true;
}

void SkillManager::ProcessSkillResultFinal(const std::shared_ptr<ProcessingSkill>& pSkill)
{
    ProcessingSkill& skill = *pSkill.get();

    auto activated = skill.Activated;
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());
    auto zone = source->GetZone();
    if(!zone)
    {
        // Somehow the source left the zone, quit out
        return;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto tokuseiManager = server->GetTokuseiManager();
    auto zoneManager = server->GetZoneManager();

    // Quit here if nothing will be affected by damage or effects
    if(skill.Targets.size() == 0)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(skill.SkillID);
        p.WriteS8(0);
        p.WriteU32Little(0);

        zoneManager->BroadcastPacket(zone, p);

        return;
    }

    auto damageData = skill.Definition->GetDamage();
    bool hasBattleDamage = damageData->GetBattleDamage()->GetFormula()
        != objects::MiBattleDamageData::Formula_t::NONE;
    if(hasBattleDamage)
    {
        auto battleDamage = damageData->GetBattleDamage();
        if(!CalculateDamage(source, pSkill))
        {
            LOG_ERROR(libcomp::String("Damage failed to calculate: %1\n")
                .Arg(skill.SkillID));
            return;
        }

        // Now that damage has been calculated, merge final NRA flags in
        for(SkillTargetResult& target : skill.Targets)
        {
            switch(target.HitNull)
            {
            case 1:
                target.Flags1 |= FLAG1_BLOCK_PHYS;
                break;
            case 2:
                target.Flags1 |= FLAG1_BLOCK_MAGIC;
                break;
            default:
                break;
            }

            switch(target.HitReflect)
            {
            case 1:
                target.Flags1 |= FLAG1_REFLECT_PHYS;
                break;
            case 2:
                target.Flags1 |= FLAG1_REFLECT_MAGIC;
                break;
            default:
                break;
            }

            if(target.HitAbsorb)
            {
                target.Flags1 |= FLAG1_ABSORB;
            }
        }

        // Now that damage is calculated, apply drain
        uint8_t hpDrainPercent = battleDamage->GetHPDrainPercent();
        uint8_t mpDrainPercent = battleDamage->GetMPDrainPercent();
        if(hpDrainPercent > 0 || mpDrainPercent > 0)
        {
            auto selfTarget = GetSelfTarget(source, skill.Targets, true);

            int32_t hpDrain = 0, mpDrain = 0;
            for(SkillTargetResult& target : skill.Targets)
            {
                if(target.Damage1Type == DAMAGE_TYPE_GENERIC && hpDrainPercent > 0)
                {
                    hpDrain = (int32_t)(hpDrain -
                        (int32_t)floorl((float)target.Damage1 * (float)hpDrainPercent * 0.01f));
                }
                
                if(target.Damage2Type == DAMAGE_TYPE_GENERIC && mpDrainPercent > 0)
                {
                    mpDrain = (int32_t)(mpDrain -
                        (int32_t)floorl((float)target.Damage2 * (float)mpDrainPercent * 0.01f));
                }
            }

            // Always heal HP even if value is 0
            selfTarget->Damage1Type = DAMAGE_TYPE_HEALING;
            selfTarget->Damage1 = hpDrain;

            // Heal MP only if the value is less than 0
            if(mpDrain < 0)
            {
                selfTarget->Damage2Type = DAMAGE_TYPE_HEALING;
                selfTarget->Damage2 = mpDrain;
            }
        }
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

    uint64_t now = ChannelServer::GetServerTime();
    source->RefreshCurrentPosition(now);

    // Apply calculation results, keeping track of entities that may
    // need to update the world with their modified state
    std::set<std::shared_ptr<ActiveEntityState>> revived;
    std::set<std::shared_ptr<ActiveEntityState>> killed;
    std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
    std::list<std::pair<std::shared_ptr<ActiveEntityState>, uint8_t>> talkDone;
    std::unordered_map<std::shared_ptr<ActiveEntityState>, uint8_t> cancellations;
    std::unordered_map<std::shared_ptr<ActiveEntityState>,
        std::set<TokuseiConditionType>> recalcEntities;
    for(SkillTargetResult& target : skill.Targets)
    {
        if(target.HitAvoided) continue;

        auto targetCalc = GetCalculatedState(target.EntityState, pSkill, true, source);

        target.EntityState->RefreshCurrentPosition(now);
        cancellations[target.EntityState] = 0;

        int32_t hpDamage = target.TechnicalDamage + target.PursuitDamage;
        int32_t mpDamage = 0;
        if(hasBattleDamage)
        {
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
        }

        // Determine which status effects to apply
        std::set<uint32_t> cancelOnKill;
        if(!(skill.ExecutionContext && !skill.ExecutionContext->ApplyStatusEffects) &&
            !target.IndirectTarget && !target.HitNull && !target.HitReflect && !target.HitAbsorb)
        {
            auto statusAdjusts = tokuseiManager->GetAspectMap(source,
                TokuseiAspectType::STATUS_INFLICT_ADJUST,
                GetCalculatedState(source, pSkill, false, target.EntityState));

            for(auto addStatus : addStatuses)
            {
                if(addStatus->GetOnKnockback() && !(target.Flags1 & FLAG1_KNOCKBACK)) continue;

                int32_t successRate = (int32_t)addStatus->GetSuccessRate();

                auto it = statusAdjusts.find((int32_t)addStatus->GetStatusID());
                if(it != statusAdjusts.end())
                {
                    successRate = (int32_t)(successRate + floor(it->second));
                }

                if(successRate >= 100 || RNG(int32_t, 1, 100) <= successRate)
                {
                    auto statusDef = definitionManager->GetStatusData(
                        addStatus->GetStatusID());

                    uint8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
                        addStatus->GetMaxStack());
                    if(!statusDef || (stack == 0 && !addStatus->GetIsReplace())) continue;

                    target.AddedStatuses[addStatus->GetStatusID()] =
                        std::pair<uint8_t, bool>(stack, addStatus->GetIsReplace());

                    // Check for status T-Damage to apply at the end of the skill
                    auto basicDef = statusDef->GetBasic();
                    if(basicDef->GetStackType() == 1 && basicDef->GetApplicationLogic() == 0)
                    {
                        auto tDamage = statusDef->GetEffect()->GetDamage();
                        if(tDamage->GetHPDamage() > 0)
                        {
                            uint8_t affinity = statusDef->GetCommon()->GetAffinity();
                            CorrectTbl resistCorrectType = (CorrectTbl)(affinity + RES_OFFSET);
                            float resist = (float)targetCalc->GetCorrectTbl((size_t)resistCorrectType)
                                * 0.01f;

                            /// @todo: figure this out
                            target.AilmentDamageType = (uint8_t)(affinity - AIL_OFFSET);
                            target.AilmentDamage += (int32_t)((float)(
                                tDamage->GetHPDamage() + (int16_t)stack) * (1.f + resist * -1.f));
                            hpDamage += target.AilmentDamage;

                            uint64_t ailmentTime = (uint64_t)((uint32_t)stack *
                                statusDef->GetCancel()->GetDuration()) * 1000;
                            if(ailmentTime > target.AilmentDamageTime)
                            {
                                target.AilmentDamageTime = ailmentTime;
                            }
                        }
                    }
                    else
                    {
                        auto cancelDef = statusDef->GetCancel();
                        if(cancelDef->GetCancelTypes() & EFFECT_CANCEL_DEATH)
                        {
                            cancelOnKill.insert(addStatus->GetStatusID());
                        }
                    }
                }
            }
        }

        // Perform knockback if there is normal damage but no damage potential
        // or if damage was dealt
        auto battleDamage = damageData->GetBattleDamage();
        bool applyKnockback = battleDamage->GetFormula()
            == objects::MiBattleDamageData::Formula_t::DMG_NORMAL &&
            battleDamage->GetModifier1() == 0 &&
            battleDamage->GetModifier2() == 0;
        if(hpDamage != 0 || mpDamage != 0)
        {
            bool targetAlive = target.EntityState->IsAlive();

            int32_t hpAdjusted, mpAdjusted;
            if(target.EntityState->SetHPMP(-hpDamage, -mpDamage, true,
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
                    target.Flags1 |= FLAG1_LETHAL;
                    cancellations[target.EntityState] |= EFFECT_CANCEL_DEATH;
                    killed.insert(target.EntityState);

                    for(uint32_t effectID : cancelOnKill)
                    {
                        target.AddedStatuses.erase(effectID);
                    }
                }
                else
                {
                    target.Flags1 |= FLAG1_REVIVAL;
                    revived.insert(target.EntityState);
                }
            }

            if(hpAdjusted != 0)
            {
                recalcEntities[target.EntityState].insert(TokuseiConditionType::CURRENT_HP);
            }

            if(mpAdjusted != 0)
            {
                recalcEntities[target.EntityState].insert(TokuseiConditionType::CURRENT_MP);
            }

            if(hpAdjusted < 0)
            {
                int32_t hitstunNull = (int32_t)tokuseiManager->GetAspectSum(source,
                    TokuseiAspectType::HITSTUN_NULL, targetCalc) * 100;
                target.CanHitstun = hitstunNull != 10000 &&
                    (hitstunNull < 0 || RNG(int32_t, 1, 10000) > hitstunNull);

                applyKnockback = true;

                cancellations[target.EntityState] |= EFFECT_CANCEL_HIT |
                    EFFECT_CANCEL_DAMAGE;
            }
            else
            {
                applyKnockback = false;
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
                        // If an enemy is damaged by a player character or their
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

        if(applyKnockback && kbMod)
        {
            int32_t kbNull = (int32_t)tokuseiManager->GetAspectSum(source,
                TokuseiAspectType::KNOCKBACK_NULL, targetCalc) * 100;
            target.CanKnockback = kbNull != 10000 &&
                (kbNull < 0 || RNG(int32_t, 1, 10000) > kbNull);

            if(target.CanKnockback)
            {
                float kb = target.EntityState->UpdateKnockback(now, kbMod);
                if(kb == 0.f)
                {
                    target.Flags1 |= FLAG1_KNOCKBACK;
                    cancellations[target.EntityState] |= EFFECT_CANCEL_KNOCKBACK;
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
            auto spawn = enemy->GetSpawnSource();
            if(enemy->GetCoreStats()->GetLevel() >
                source->GetCoreStats()->GetLevel())
            {
                // Enemies that are a higher level cannot be negotiated with
                break;
            }

            auto talkPoints = eState->GetTalkPoints(source->GetEntityID());
            auto demonData = eState->GetDevilData();
            auto negData = demonData->GetNegotiation();
            uint8_t affThreshold = (uint8_t)(100 - negData->GetAffabilityThreshold());
            uint8_t fearThreshold = (uint8_t)(100 - negData->GetFearThreshold());

            if(talkPoints.first >= affThreshold ||
                talkPoints.second >= fearThreshold)
            {
                // Nothing left to do
                break;
            }

            int32_t talkType = 0;
            switch(skill.Definition->GetBasic()->GetActionType())
            {
            case objects::MiSkillBasicData::ActionType_t::TALK:
                talkType = 1;
                break;
            case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
                talkType = 2;
                break;
            case objects::MiSkillBasicData::ActionType_t::TAUNT:
                talkType = 3;
                break;
            default:
                break;
            }

            double talkSuccess = spawn ? (double)(100 - spawn->GetTalkResist()) : 0.0;
            if(talkType != 0 && talkSuccess != 0.0)
            {
                auto calcState = GetCalculatedState(source, pSkill, false,
                    target.EntityState);

                auto adjust = tokuseiManager->GetAspectMap(source,
                    TokuseiAspectType::TALK_RATE,
                    std::set<int32_t>{ 0, talkType }, calcState);

                for(auto pair : adjust)
                {
                    talkSuccess += pair.second;
                }
            }

            bool success = talkSuccess > 0.0 &&
                RNG(uint16_t, 1, 100) <= (uint16_t)talkSuccess;
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
                // Determine which outcomes are valid and randomly
                // select one
                int32_t minVal = 1;
                int32_t maxVal = 6;

                bool canJoin = true;
                bool canGift = true;

                uint8_t talkResults = spawn ? spawn->GetTalkResults() : 3;
                if((talkResults & 0x01) == 0)
                {
                    canJoin = false;
                    maxVal -= 2;
                }

                if((talkResults & 0x02) == 0)
                {
                    canGift = false;
                    maxVal -= 2;
                }

                int32_t outcome = RNG(int32_t, minVal, maxVal);

                // Shift the outcome to the proper position if some
                // results are not available
                if(!canJoin)
                {
                    outcome += 2;
                }

                if(!canGift && outcome >= 3 && outcome <= 4)
                {
                    outcome += 2;
                }

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
    }

    for(auto cancelPair : cancellations)
    {
        if(cancelPair.second)
        {
            auto eState = cancelPair.first;
            uint8_t cancelFlags = cancelPair.second;

            cancelPair.first->CancelStatusEffects(cancelFlags);

            // Check for skills that need to be cancelled
            if(cancelFlags & (EFFECT_CANCEL_DAMAGE | EFFECT_CANCEL_KNOCKBACK))
            {
                auto tActivated = eState->GetActivatedAbility();
                auto tSkillData = tActivated
                    ? definitionManager->GetSkillData(tActivated->GetSkillID()) : nullptr;
                if(tSkillData)
                {
                    auto tCancel = tSkillData->GetCast()->GetCancel();
                    bool applyInterrupt = ((cancelFlags & EFFECT_CANCEL_DAMAGE) != 0 &&
                        tCancel->GetDamageCancel()) ||
                        ((cancelFlags & EFFECT_CANCEL_KNOCKBACK) != 0 &&
                            tCancel->GetKnockbackCancel());

                    if(applyInterrupt)
                    {
                        int32_t interruptNull = (int32_t)tokuseiManager->GetAspectSum(
                            source, TokuseiAspectType::CAST_INTERRUPT_NULL,
                            GetCalculatedState(eState, pSkill, true, source)) * 100;

                        bool cancelInterrupt = interruptNull >= 10000 ||
                            (interruptNull > 0 && RNG(int32_t, 1, 10000) <= interruptNull);

                        if(!cancelInterrupt)
                        {
                            CancelSkill(eState, tActivated->GetActivationID());
                        }
                    }
                }
            }

            recalcEntities[cancelPair.first].insert(TokuseiConditionType::STATUS_ACTIVE);
        }
    }

    // Now that previous effects have been cancelled, add the new ones
    uint32_t effectTime = (uint32_t)std::time(0);
    for(SkillTargetResult& target : skill.Targets)
    {
        if(target.AddedStatuses.size() > 0)
        {
            auto removed = target.EntityState->AddStatusEffects(
                target.AddedStatuses, definitionManager, effectTime, false);
            for(auto r : removed)
            {
                target.CancelledStatuses.insert(r);
            }

            recalcEntities[target.EntityState].insert(TokuseiConditionType::STATUS_ACTIVE);
        }
    }

    // Recalculate any effects that trigger from the skill effects
    if(recalcEntities.size() > 0)
    {
        std::unordered_map<int32_t, bool> effectRecalc;
        for(auto pair : recalcEntities)
        {
            std::unordered_map<int32_t, bool> result;

            // Anything with a status effect modified needs a full tokusei and stat recalc
            bool statusChanged = pair.second.find(TokuseiConditionType::STATUS_ACTIVE) !=
                pair.second.end();
            if(effectRecalc.find(pair.first->GetEntityID()) == effectRecalc.end())
            {
                if(statusChanged)
                {
                    result = tokuseiManager->Recalculate(pair.first, true);
                }
                else
                {
                    result = tokuseiManager->Recalculate(pair.first, pair.second);
                }
            }

            for(auto resultPair : result)
            {
                if(effectRecalc.find(resultPair.first) == effectRecalc.end())
                {
                    effectRecalc[resultPair.first] = resultPair.second;
                }
                else
                {
                    effectRecalc[resultPair.first] |= resultPair.second;
                }
            }

            if(statusChanged && !effectRecalc[pair.first->GetEntityID()])
            {
                characterManager->RecalculateStats(nullptr, pair.first->GetEntityID());
            }
        }
    }

    // Send negotiation results first since some are dependent upon the
    // skill hit
    if(talkDone.size() > 0)
    {
        HandleNegotiations(source, zone, talkDone);
    }

    bool doRush = skill.Definition->GetBasic()->GetActionType() ==
        objects::MiSkillBasicData::ActionType_t::RUSH && skill.PrimaryTarget;
    if(doRush)
    {
        SkillTargetResult* selfTarget = GetSelfTarget(source, skill.Targets, true);
        selfTarget->Flags1 |= FLAG1_RUSH_MOVEMENT;
    }

    auto effectiveSource = skill.EffectiveSource;
    auto primaryTarget = skill.PrimaryTarget;
    auto effectiveTarget = primaryTarget ? primaryTarget : effectiveSource;

    uint64_t hitTimings[3];
    uint64_t completeTime = now +
        (skill.Definition->GetDischarge()->GetStiffness() * 1000);
    uint64_t hitStopTime = now +
        (skill.Definition->GetDamage()->GetHitStopTime() * 1000);

    auto zConnections = zone->GetConnectionList();

    // The skill report packet can easily go over the max packet size so
    // the targets in the results need to be batched
    std::list<std::list<SkillTargetResult*>> targetBatches;
    std::list<SkillTargetResult*> currentBatch;
    int32_t currentBatchSize = 0;
    for(SkillTargetResult& target : skill.Targets)
    {
        int32_t currentTargetSize = (int32_t)(64 + (target.AddedStatuses.size() * 9)
            + (target.CancelledStatuses.size() * 4));

        // If the new list size + the header size is larger than the max
        // packet size, move on to the next batch
        if((uint32_t)(currentBatchSize + currentTargetSize + 15) > MAX_CHANNEL_PACKET_SIZE)
        {
            targetBatches.push_back(currentBatch);
            currentBatch.clear();
            currentBatchSize = currentTargetSize;
        }
        else
        {
            currentBatchSize += currentTargetSize;
        }

        currentBatch.push_back(&target);
    }

    // If we get here with an empty target list, send the empty list
    targetBatches.push_back(currentBatch);

    for(auto it = targetBatches.begin(); it != targetBatches.end(); it++)
    {
        if(it != targetBatches.begin())
        {
            // An execute packet must be sent once per report (even if its
            // identical) or the client starts ignoring the reports
            SendExecuteSkill(pSkill->Activated);
        }

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(skill.SkillID);
        p.WriteS8((int8_t)activated->GetActivationID());

        p.WriteU32Little((uint32_t)it->size());
        for(SkillTargetResult* skillTarget : *it)
        {
            SkillTargetResult& target = *skillTarget;

            p.WriteS32Little(target.EntityState->GetEntityID());
            p.WriteS32Little(abs(target.Damage1));
            p.WriteU8(target.Damage1Type);
            p.WriteS32Little(abs(target.Damage2));
            p.WriteU8(target.Damage2Type);
            p.WriteU16Little(target.Flags1);

            p.WriteU8(target.AilmentDamageType);
            p.WriteS32Little(abs(target.AilmentDamage));

            bool rushing = false, knockedBack = false;
            if((target.Flags1 & FLAG1_KNOCKBACK) && kbType != 2)
            {
                uint8_t kbEffectiveType = kbType;
                if(kbType == 1 && target.PrimaryTarget)
                {
                    // Targets of AOE knockback are treated like default knockback
                    kbEffectiveType = 0;
                }

                // Ignore knockback type 2 which is "None"
                Point kbPoint(target.EntityState->GetCurrentX(),
                    target.EntityState->GetCurrentY());
                switch(kbEffectiveType)
                {
                case 1:
                    {
                        // Away from the effective target (ex: AOE explosion)
                        kbPoint = zoneManager->MoveRelative(target.EntityState,
                            effectiveTarget->GetCurrentX(), effectiveTarget->GetCurrentY(),
                            kbDistance, true, now, hitStopTime);
                    }
                    break;
                case 4:
                    if(target.EntityState != effectiveTarget)
                    {
                        // Move to the same spot as the target entity
                        target.EntityState->SetOriginX(target.EntityState->GetCurrentX());
                        target.EntityState->SetOriginY(target.EntityState->GetCurrentY());
                        target.EntityState->SetOriginTicks(now);

                        target.EntityState->SetDestinationX(effectiveTarget->GetCurrentX());
                        target.EntityState->SetDestinationY(effectiveTarget->GetCurrentY());
                        target.EntityState->SetDestinationTicks(hitStopTime);
                    }
                    break;
                case 5:
                    {
                        // Oosition becomes source position
                        target.EntityState->SetOriginX(target.EntityState->GetCurrentX());
                        target.EntityState->SetOriginY(target.EntityState->GetCurrentY());
                        target.EntityState->SetOriginTicks(now);

                        target.EntityState->SetDestinationX(source->GetCurrentX());
                        target.EntityState->SetDestinationY(source->GetCurrentY());
                        target.EntityState->SetDestinationTicks(hitStopTime);
                    }
                    break;
                case 0:
                case 3: /// @todo: technically this has more spread than 0
                default:
                    // Default if not specified, directly away from source
                    kbPoint = zoneManager->MoveRelative(target.EntityState, effectiveSource
                        ->GetCurrentX(), effectiveSource->GetCurrentY(), kbDistance, true, now,
                        hitStopTime);
                    break;
                }

                target.EntityState->SetStatusTimes(STATUS_KNOCKBACK,
                    target.EntityState->GetDestinationTicks());

                p.WriteFloat(kbPoint.x);
                p.WriteFloat(kbPoint.y);

                knockedBack = true;
            }
            else if(target.EntityState == source && doRush)
            {
                // Set the new location of the rush user
                float dist = source->GetDistance(primaryTarget->GetCurrentX(),
                    primaryTarget->GetCurrentY());

                Point rushPoint = zoneManager->MoveRelative(source, primaryTarget->GetCurrentX(),
                    primaryTarget->GetCurrentY(), dist + 250.f, false, now, completeTime);

                p.WriteFloat(rushPoint.x);
                p.WriteFloat(rushPoint.y);

                rushing = true;
            }
            else
            {
                p.WriteBlank(8);
            }

            p.WriteFloat(0);    // Unknown

            // Calculate hit timing
            hitTimings[0] = hitTimings[1] = hitTimings[2] = 0;
            if(rushing)
            {
                hitTimings[0] = now;
                hitTimings[1] = now + 200000;
            }
            else if(target.Damage1Type == DAMAGE_TYPE_GENERIC || knockedBack)
            {
                if(target.Damage1)
                {
                    // Damage dealt, determine stun time
                    bool extendHitStun = target.AilmentDamageType != 0 || knockedBack;
                    if(extendHitStun)
                    {
                        // Apply extended hit stop and determine what else may be needed
                        hitTimings[0] = knockedBack ? now : completeTime;
                        hitTimings[1] = hitStopTime;

                        if(!target.AilmentDamageType)
                        {
                            // End after hit stop
                            hitTimings[2] = hitStopTime;
                        }
                        else
                        {
                            // Apply ailment damage after hit stop
                            hitTimings[2] = hitStopTime + target.AilmentDamageTime;
                        }
                    }
                    else
                    {
                        // Normal hit stop (or complete only if hit stun nulled)
                        hitTimings[2] = target.CanHitstun ? hitStopTime : completeTime;
                    }

                    if(target.CanHitstun)
                    {
                        target.EntityState->SetStatusTimes(STATUS_HIT_STUN,
                            hitTimings[2]);
                    }
                }
                else if(knockedBack)
                {
                    // Normal hit stop time to finish knockback
                    hitTimings[0] = now;
                    hitTimings[2] = hitTimings[1] = hitStopTime;

                    target.EntityState->SetStatusTimes(STATUS_HIT_STUN,
                        hitTimings[2]);
                }
                else
                {
                    // No damage, just result displays
                    hitTimings[2] = completeTime;
                }
            }

            for(size_t i = 0; i < 3; i++)
            {
                p.WriteFloat(ChannelServer::ToSyncTime(hitTimings[i]));
            }

            p.WriteU8(target.TalkFlags);

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

            p.WriteU32Little((uint32_t)addedStatuses.size());
            p.WriteU32Little((uint32_t)cancelledStatuses.size());

            for(auto effect : addedStatuses)
            {
                p.WriteU32Little(effect->GetEffect());
                p.WriteS32Little((int32_t)effect->GetExpiration());
                p.WriteU8(effect->GetStack());
            }

            for(auto cancelled : cancelledStatuses)
            {
                p.WriteU32Little(cancelled);
            }

            p.WriteU16Little(target.Flags2);
            p.WriteS32Little(target.TechnicalDamage);
            p.WriteS32Little(target.PursuitDamage);
        }

        ChannelClientConnection::BroadcastPacket(zConnections, p);
    }

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
}

std::shared_ptr<ProcessingSkill> SkillManager::GetProcessingSkill(
    std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<SkillExecutionContext> ctx,
    const std::shared_ptr<ChannelClientConnection> client)
{
    if(ctx && ctx->Skill)
    {
        return ctx->Skill;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillData = definitionManager->GetSkillData(activated->GetSkillID());
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    auto skill = std::make_shared<ProcessingSkill>();
    skill->SkillID = activated->GetSkillID();
    skill->Definition = skillData;
    skill->Activated = activated;
    skill->BaseAffinity = skill->EffectiveAffinity = skillData->GetCommon()->GetAffinity();
    skill->EffectiveDependencyType = skillData->GetBasic()->GetDependencyType();
    skill->EffectiveSource = source;
    skill->IsSuicide = skillData->GetDamage()->GetFunctionID() == SVR_CONST.SKILL_SUICIDE;

    if(client)
    {
        auto state = client->GetClientState();

        int64_t targetObjectID = activated->GetActivationObjectID();
        skill->IsItemSkill = targetObjectID != 0 &&
            std::dynamic_pointer_cast<objects::Item>(
                libcomp::PersistentObject::GetObjectByUUID(
                    state->GetObjectUUID(targetObjectID)));
    }

    // Calculate effective dependency and affinity types if "weapon" is specified
    if(skill->EffectiveDependencyType == 4 || skill->BaseAffinity == 1)
    {
        auto cState = std::dynamic_pointer_cast<CharacterState>(source);
        auto weapon = cState ? cState->GetEntity()->GetEquippedItems((size_t)
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON).Get() : nullptr;
        auto weaponDef = weapon ? definitionManager->GetItemData(weapon->GetType()) : nullptr;

        if(weaponDef)
        {
            if(skill->EffectiveDependencyType == 4)
            {
                switch(weaponDef->GetBasic()->GetWeaponType())
                {
                case objects::MiItemBasicData::WeaponType_t::LONG_RANGE:
                    skill->EffectiveDependencyType = 1;
                    break;
                case objects::MiItemBasicData::WeaponType_t::CLOSE_RANGE:
                default:
                    // Use default below
                    break;
                }
            }

            if(skill->EffectiveAffinity == 1)
            {
                if(weaponDef->GetBasic()->GetWeaponType() ==
                    objects::MiItemBasicData::WeaponType_t::LONG_RANGE)
                {
                    // If the bullet has an affinity, use that instead
                    auto bullet = cState ? cState->GetEntity()->GetEquippedItems((size_t)
                        objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BULLETS).Get() : nullptr;
                    auto bulletDef = bullet ? definitionManager->GetItemData(bullet->GetType()) : nullptr;
                    if(bulletDef && bulletDef->GetCommon()->GetAffinity() != 0)
                    {
                        skill->EffectiveAffinity = bulletDef->GetCommon()->GetAffinity();
                    }
                }

                if(skill->EffectiveAffinity == 1)
                {
                    skill->EffectiveAffinity = weaponDef->GetCommon()->GetAffinity();
                }

                // Take the lowest value applied tokusei affinity override if one exists
                auto tokuseiOverrides = server->GetTokuseiManager()->GetAspectValueList(source,
                    TokuseiAspectType::WEAPON_AFFINITY_OVERRIDE);
                if(tokuseiOverrides.size() > 0)
                {
                    tokuseiOverrides.sort();
                    skill->EffectiveAffinity = (uint8_t)tokuseiOverrides.front();
                }

                skill->WeaponAffinity = skill->EffectiveAffinity;
            }
        }

        // If at any point the type cannot be determined,
        // default to strike, close range (ex: no weapon/non-character source)
        if(skill->EffectiveAffinity == 1)
        {
            skill->EffectiveAffinity = (uint8_t)CorrectTbl::RES_STRIKE - RES_OFFSET;
        }

        if(skill->EffectiveDependencyType == 4)
        {
            skill->EffectiveDependencyType = 0;
        }
    }

    if(ctx)
    {
        skill->ExecutionContext = ctx.get();
        ctx->Skill = skill;
    }

    return skill;
}


std::shared_ptr<objects::CalculatedEntityState> SkillManager::GetCalculatedState(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<ProcessingSkill>& pSkill,
    bool isTarget, const std::shared_ptr<ActiveEntityState>& otherState)
{
    ProcessingSkill& skill = *pSkill.get();

    std::shared_ptr<objects::CalculatedEntityState> calcState;
    if(isTarget)
    {
        calcState = skill.TargetCalcStates[eState->GetEntityID()];
    }
    else if(otherState)
    {
        calcState = skill.SourceCalcStates[otherState->GetEntityID()];
    }

    if(!calcState)
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();

        // Determine which tokusei are active and don't need to be calculated again
        if(!isTarget && otherState && skill.SourceExecutionState)
        {
            // If we're calculating for a skill target, start with the execution state
            calcState = skill.SourceExecutionState;
        }
        else
        {
            // Otherwise start with the base calculated state
            calcState = eState->GetCalculatedState();
        }

        // Keep track of tokusei that are not valid for the skill conditions but
        // CAN become active given the correct target (only valid for source)
        std::unordered_map<int32_t, uint16_t> stillPendingSkillTokusei;

        auto effectiveTokusei = calcState->GetEffectiveTokuseiFinal();
        auto pendingSkillTokusei = calcState->GetPendingSkillTokuseiFinal();

        bool modified = false;
        for(auto pair : pendingSkillTokusei)
        {
            auto tokusei = definitionManager->GetTokuseiData(pair.first);
            if(tokusei)
            {
                bool add = true;
                bool canAdd = true;
                for(auto condition : tokusei->GetSkillConditions())
                {
                    canAdd &= condition->GetTargetCondition() == isTarget;
                    if(!canAdd || !EvaluateTokuseiSkillCondition(eState,
                        condition, pSkill, otherState))
                    {
                        add = false;
                        break;
                    }
                }

                if(add)
                {
                    effectiveTokusei[tokusei->GetID()] = pair.second;
                    modified = true;
                }
                else if(canAdd)
                {
                    stillPendingSkillTokusei[tokusei->GetID()] = pair.second;
                }
            }
        }

        if(modified)
        {
            // If the tokusei set was modified, calculate skill specific stats
            calcState = std::make_shared<objects::CalculatedEntityState>();
            calcState->SetEffectiveTokusei(effectiveTokusei);
            calcState->SetPendingSkillTokusei(stillPendingSkillTokusei);

            eState->RecalculateStats(definitionManager, calcState);
        }

        if(isTarget)
        {
            skill.TargetCalcStates[eState->GetEntityID()] = calcState;
        }
        else if(otherState)
        {
            skill.SourceCalcStates[otherState->GetEntityID()] = calcState;
        }
    }

    return calcState;
}

bool SkillManager::EvaluateTokuseiSkillCondition(const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::TokuseiSkillCondition>& condition,
    const std::shared_ptr<ProcessingSkill>& pSkill,
    const std::shared_ptr<ActiveEntityState>& otherState)
{
    ProcessingSkill& skill = *pSkill.get();

    // TokuseiSkillCondition comparators can only be equals or not equal
    bool negate = condition->GetComparator() != objects::TokuseiCondition::Comparator_t::NOT_EQUAL;

    switch(condition->GetSkillConditionType())
    {
    case objects::TokuseiSkillCondition::SkillConditionType_t::ANY_SKILL:
        // Used to bind conditions to skill processing time only
        return true;
    case objects::TokuseiSkillCondition::SkillConditionType_t::EXPLICIT_SKILL:
        // Current skill is the specified skill
        return (skill.SkillID == (uint32_t)condition->GetValue()) == !negate;
    case objects::TokuseiSkillCondition::SkillConditionType_t::ACTION_TYPE:
        // Current skill is the specified action type
        return ((int32_t)skill.Definition->GetBasic()->GetActionType() ==
            condition->GetValue()) == !negate;
    case objects::TokuseiSkillCondition::SkillConditionType_t::AFFINITY:
        // Current skill is the specified affinity type
        return ((int32_t)skill.BaseAffinity == condition->GetValue() ||
            (int32_t)skill.EffectiveAffinity == condition->GetValue()) == !negate;
    case objects::TokuseiSkillCondition::SkillConditionType_t::SKILL_CLASS:
        // Current skill is magic, physical or misc
        switch(skill.EffectiveDependencyType)
        {
        case 2:
        case 7:
        case 8:
        case 11:
            // Magic
            return (1 == condition->GetValue()) == !negate;
        case 0:
        case 1:
        case 6:
        case 9:
        case 10:
        case 12:
            // Physical
            return (2 == condition->GetValue()) == !negate;
        case 3:
        case 5:
        default:
            // Misc
            return (3 == condition->GetValue()) == !negate;
        }
    case objects::TokuseiSkillCondition::SkillConditionType_t::ENEMY_GENDER:
        // Enemy's gender matches the specified type (can be any target type)
        if(otherState)
        {
            // Error
            return false;
        }
        else
        {
            int32_t gender = (int32_t)objects::MiNPCBasicData::Gender_t::NONE;

            auto demonData = otherState->GetDevilData();
            if(demonData)
            {
                gender = demonData ? (int32_t)demonData->GetBasic()->GetGender() : gender;
            }
            else if(otherState->GetEntityType() == objects::EntityStateObject::EntityType_t::CHARACTER)
            {
                auto character = std::dynamic_pointer_cast<CharacterState>(otherState)->GetEntity();
                gender = character ? (int32_t)character->GetGender() : gender;
            }

            return (gender == condition->GetValue()) == !negate;
        }
    case objects::TokuseiSkillCondition::SkillConditionType_t::ENEMY_LNC:
        // Enemy's LNC matches the specified type (can be any target type)
        return otherState &&
            ((otherState->GetLNCType() & condition->GetValue()) != 0) == !negate;
    default:
        break;
    }

    // The remaining conditions depend on the other entity being a demon
    auto demonData = otherState ? otherState->GetDevilData() : nullptr;
    if(!demonData)
    {
        // Rather than return the negation value, this case will always fail as it is an error
        return false;
    }

    switch(condition->GetSkillConditionType())
    {
    case objects::TokuseiSkillCondition::SkillConditionType_t::DEMON_TYPE:
        // Demon is the specified type
        return ((int32_t)demonData->GetBasic()->GetID() == condition->GetValue()) == !negate;
    case objects::TokuseiSkillCondition::SkillConditionType_t::DEMON_FAMILY:
        // Demon is the specified family
        return ((int32_t)demonData->GetCategory()->GetFamily() == condition->GetValue()) == !negate;
    case objects::TokuseiSkillCondition::SkillConditionType_t::DEMON_RACE:
        // Demon is the specified race
        return ((int32_t)demonData->GetCategory()->GetRace() == condition->GetValue()) == !negate;
    case objects::TokuseiSkillCondition::SkillConditionType_t::DEMON_TITLE:
        // Demon has the specified title
        return ((int32_t)demonData->GetBasic()->GetTitle() == condition->GetValue()) == !negate;
    case objects::TokuseiSkillCondition::SkillConditionType_t::DEMON_PARTNER_MATCH:
        // Demon is the same family, race or type as the entity's partner demon
        {
            std::shared_ptr<objects::MiDevilData> partnerData;
            auto state = ClientState::GetEntityClientState(eState->GetEntityID(), false);
            if(state && state->GetCharacterState() == eState && state->GetDemonState()->Ready())
            {
                partnerData = state->GetDemonState()->GetDevilData();
            }

            if(!partnerData)
            {
                // Unlike the demon not existing, the partner not existing is not an error
                return negate;
            }

            switch(condition->GetValue())
            {
            case 0:
                // Same family
                return (partnerData->GetCategory()->GetFamily() ==
                    demonData->GetCategory()->GetFamily()) == !negate;
            case 1:
                // Same race
                return (partnerData->GetCategory()->GetRace() ==
                    demonData->GetCategory()->GetRace()) == !negate;
            case 2:
                // Same type
                return (partnerData->GetBasic()->GetID() ==
                    demonData->GetBasic()->GetID()) == !negate;
            default:
                return false;
            }
        }
        break;
    default:
        break;
    }

    return false;
}

uint16_t SkillManager::CalculateOffenseValue(const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<ActiveEntityState>& target,
    const std::shared_ptr<ProcessingSkill>& pSkill)
{
    ProcessingSkill& skill = *pSkill.get();
    if(skill.OffenseValues.find(target->GetEntityID()) != skill.OffenseValues.end())
    {
        return skill.OffenseValues[target->GetEntityID()];
    }

    uint16_t off = 0;

    auto damageData = skill.Definition->GetDamage()->GetBattleDamage();
    if(damageData->GetFormula() == objects::MiBattleDamageData::Formula_t::DMG_NORMAL_SIMPLE)
    {
        // Damage is determined entirely from mod value, use 1 if countered somehow
        off = 1;
    }
    else
    {
        auto calcState = GetCalculatedState(source, pSkill, false, target);

        int16_t clsr = calcState->GetCorrectTbl((size_t)CorrectTbl::CLSR);
        int16_t lngr = calcState->GetCorrectTbl((size_t)CorrectTbl::LNGR);
        int16_t spell = calcState->GetCorrectTbl((size_t)CorrectTbl::SPELL);
        int16_t support = calcState->GetCorrectTbl((size_t)CorrectTbl::SUPPORT);

        uint8_t rateBoostIdx = 0;
        switch(skill.EffectiveDependencyType)
        {
        case 0:
            off = (uint16_t)clsr;
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_CLSR;
            break;
        case 1:
            off = (uint16_t)lngr;
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_LNGR;
            break;
        case 2:
            off = (uint16_t)spell;
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
            break;
        case 3:
            off = (uint16_t)support;
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_SUPPORT;
            break;
        case 6:
            off = (uint16_t)(lngr + spell / 2);
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_LNGR;
            break;
        case 7:
            off = (uint16_t)(spell + clsr / 2);
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
            break;
        case 8:
            off = (uint16_t)(spell + lngr / 2);
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
            break;
        case 9:
            off = (uint16_t)(clsr + lngr + spell);
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_CLSR;
            break;
        case 10:
            off = (uint16_t)(lngr + clsr + spell);
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_LNGR;
            break;
        case 11:
            off = (uint16_t)(spell + clsr + lngr);
            rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
            break;
        case 12:
            off = (uint16_t)(clsr + spell / 2);
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
                (calcState->GetCorrectTbl((size_t)rateBoostIdx) * 0.01));
        }
    }

    if(skill.ExecutionContext->CounteredSkill)
    {
        // If countering, modify the offensive value with the offense value
        // of the original skill used
        uint16_t counterOff = CalculateOffenseValue(target, source,
            skill.ExecutionContext->CounteredSkill);

        off = (uint16_t)(off + (counterOff * 2));
    }

    skill.OffenseValues[target->GetEntityID()] = off;

    return off;
}

void SkillManager::CheckSkillHits(const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<ProcessingSkill>& pSkill)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    // Check for counter first and stop if one succeeds
    for(SkillTargetResult& target : pSkill->Targets)
    {
        auto tActivated = target.EntityState->GetActivatedAbility();
        if(tActivated && target.EntityState != source)
        {
            auto tSkillData = definitionManager->GetSkillData(tActivated->GetSkillID());
            switch(tSkillData->GetBasic()->GetActionType())
            {
            case objects::MiSkillBasicData::ActionType_t::GUARD:
                HandleGuard(source, target, pSkill);
                break;
            case objects::MiSkillBasicData::ActionType_t::COUNTER:
                HandleCounter(source, target, pSkill);
                break;
            case objects::MiSkillBasicData::ActionType_t::DODGE:
                HandleDodge(source, target, pSkill);
                break;
            default:
                // Cancellations occur based on knockback or damage later
                break;
            }
        }
    }
}

void SkillManager::HandleGuard(const std::shared_ptr<ActiveEntityState>& source,
    SkillTargetResult& target, const std::shared_ptr<ProcessingSkill>& pSkill)
{
    auto tActivated = target.EntityState->GetActivatedAbility();
    if(!tActivated)
    {
        return;
    }

    uint8_t activationID = tActivated->GetActivationID();
    if(pSkill->Definition->GetBasic()->GetDefensible())
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();
        auto tSkillData = definitionManager->GetSkillData(tActivated->GetSkillID());
        switch(pSkill->Definition->GetBasic()->GetActionType())
        {
        case objects::MiSkillBasicData::ActionType_t::ATTACK:
        case objects::MiSkillBasicData::ActionType_t::SPIN:
            if(tActivated->GetChargedTime() <= pSkill->Activated->GetExecutionTime())
            {
                target.Flags1 |= FLAG1_GUARDED;
                target.GuardModifier = tSkillData->GetDamage()->GetBattleDamage()->GetModifier1();
                ExecuteSkill(target.EntityState, activationID, (int64_t)source->GetEntityID());
                return;
            }
            break;
        case objects::MiSkillBasicData::ActionType_t::RUSH:
            /// @todo: Same as not guarding but with special animation
            break;
        default:
            break;
        }
    }

    CancelSkill(target.EntityState, tActivated->GetActivationID());
}

void SkillManager::HandleCounter(const std::shared_ptr<ActiveEntityState>& source,
    SkillTargetResult& target, const std::shared_ptr<ProcessingSkill>& pSkill)
{
    auto tActivated = target.EntityState->GetActivatedAbility();
    if(!tActivated)
    {
        return;
    }

    uint8_t activationID = tActivated->GetActivationID();
    if(pSkill->Definition->GetBasic()->GetDefensible())
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();
        auto tSkillData = definitionManager->GetSkillData(tActivated->GetSkillID());
        switch(pSkill->Definition->GetBasic()->GetActionType())
        {
        case objects::MiSkillBasicData::ActionType_t::ATTACK:
        case objects::MiSkillBasicData::ActionType_t::RUSH:
            if(tActivated->GetChargedTime() <= pSkill->Activated->GetExecutionTime())
            {
                target.Flags1 |= FLAG1_GUARDED;
                target.HitAvoided = true;

                auto counterCtx = std::make_shared<SkillExecutionContext>();
                counterCtx->CounteredSkill = pSkill;
                pSkill->ExecutionContext->SubContexts.push_back(counterCtx);

                ExecuteSkill(target.EntityState, activationID, (int64_t)source->GetEntityID(),
                    counterCtx);
                return;
            }
        default:
            break;
        }
    }

    CancelSkill(target.EntityState, activationID);
}

void SkillManager::HandleDodge(const std::shared_ptr<ActiveEntityState>& source,
    SkillTargetResult& target, const std::shared_ptr<ProcessingSkill>& pSkill)
{
    auto tActivated = target.EntityState->GetActivatedAbility();
    if(!tActivated)
    {
        return;
    }

    uint8_t activationID = tActivated->GetActivationID();
    if(pSkill->Definition->GetBasic()->GetDefensible())
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();
        auto tSkillData = definitionManager->GetSkillData(tActivated->GetSkillID());
        switch(pSkill->Definition->GetBasic()->GetActionType())
        {
        case objects::MiSkillBasicData::ActionType_t::SHOT:
        case objects::MiSkillBasicData::ActionType_t::RAPID:
            if(tActivated->GetChargedTime() <= pSkill->Activated->GetExecutionTime())
            {
                target.Flags1 |= FLAG1_DODGED;
                target.Damage1Type = target.Damage2Type = DAMAGE_TYPE_MISS;
                target.HitAvoided = true;
                ExecuteSkill(target.EntityState, activationID, (int64_t)source->GetEntityID());
                return;
            }
        default:
            break;
        }
    }

    CancelSkill(target.EntityState, activationID);
}

void SkillManager::HandleKills(std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<Zone>& zone,
    std::set<std::shared_ptr<ActiveEntityState>> killed)
{
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
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

    auto sourceDevilData = source->GetDevilData();
    uint32_t sourceDemonType = sourceDevilData
        ? sourceDevilData->GetBasic()->GetID() : 0;
    int32_t sourceDemonFType = sourceDevilData
        ? sourceDevilData->GetFamiliarity()->GetFamiliarityType() : 0;

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
        std::unordered_map<uint32_t, uint32_t> encounterIDs;
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

            encounterIDs[enemy->GetEncounterID()] = enemy->GetSpawnGroupID();
        }

        // For each loot body generate and send loot and show the body
        // After this schedule all of the bodies for cleanup after their
        // loot time passes
        uint64_t now = ChannelServer::GetServerTime();
        int16_t luck = source->GetLUCK();

        auto firstClient = zConnections.size() > 0 ? zConnections.front() : nullptr;
        auto sourceParty = sourceState ? sourceState->GetParty() : nullptr;
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
            auto damageSources = enemy->GetDamageSources();

            auto drops = GetItemDrops(enemy->GetType(), spawn);

            // Create loot based off drops and send if any was added
            uint64_t lootTime = 0;
            if(sourceState && characterManager->CreateLootFromDrops(lootBody, drops, luck))
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
                            for(auto pair : damageSources)
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

        // Update quest kill counts
        if(sourceClient && questKills.size() > 0)
        {
            server->GetEventManager()->UpdateQuestKillCount(sourceClient,
                questKills);
        }

        // Perform defeat actions for all empty encounters
        encounterIDs.erase(0);

        for(auto ePair : encounterIDs)
        {
            std::shared_ptr<objects::ActionSpawn> defeatActionSource;
            if(zone->EncounterDefeated(ePair.first, defeatActionSource))
            {
                // If the defeatActionSource has actions, those override the group's default
                if(defeatActionSource && defeatActionSource->DefeatActionsCount() > 0)
                {
                    server->GetActionManager()->PerformActions(sourceClient,
                        defeatActionSource->GetDefeatActions(), source->GetEntityID(),
                        zone, ePair.first);
                }
                else
                {
                    auto group = zone->GetDefinition()->GetSpawnGroups(ePair.second);
                    if(group && group->DefeatActionsCount() > 0)
                    {
                        server->GetActionManager()->PerformActions(sourceClient,
                            group->GetDefeatActions(), source->GetEntityID(), zone,
                            ePair.first);
                    }
                }
            }
        }

        ChannelClientConnection::FlushAllOutgoing(zConnections);

        // Loop through one last time and send all XP gained
        for(auto eState : enemiesKilled)
        {
            auto enemy = eState->GetEntity();
            HandleKillXP(enemy, zone);
        }
    }
}

void SkillManager::HandleKillXP(const std::shared_ptr<objects::Enemy>& enemy,
    const std::shared_ptr<Zone>& zone)
{
    auto spawn = enemy->GetSpawnSource();

    int64_t totalXP = 0;
    if(spawn && spawn->GetXP() >= 0)
    {
        totalXP = spawn->GetXP();
    }
    else
    {
        // All non-spawn enemies have a calculated value
        /// @todo: verify
        totalXP = (int64_t)(enemy->GetCoreStats()->GetLevel() * 20);
    }

    if(totalXP <= 0)
    {
        return;
    }
    else
    {
        // Apply zone XP multiplier
        totalXP = (int64_t)((double)totalXP *
            (double)zone->GetDefinition()->GetXPMultiplier());
    }

    // Determine XP distribution
    // -Individuals/single parties gain max XP
    // -Multiple individuals/parties have XP distributed by damage dealt
    // -Party members gain alloted XP - ((number of members in the zone - 1) * 10%)
    std::unordered_map<int32_t, uint64_t> playerDamage;
    std::unordered_map<uint32_t, uint64_t> partyDamage;
    std::unordered_map<uint32_t, std::shared_ptr<objects::Party>> parties;

    uint64_t totalDamage = 0;
    auto damageSources = enemy->GetDamageSources();
    for(auto damagePair : damageSources)
    {
        totalDamage = (uint64_t)(totalDamage + damagePair.second);
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto managerConnection = server->GetManagerConnection();

    std::unordered_map<int32_t,
        std::shared_ptr<ChannelClientConnection>> clientMap;
    for(auto damagePair : damageSources)
    {
        auto c = managerConnection->GetEntityClient(damagePair.first, true);
        if(c)
        {
            clientMap[damagePair.first] = c;

            uint64_t dmg = damagePair.second;
            auto s = c->GetClientState();
            auto party = s->GetParty();
            if(party)
            {
                uint32_t partyID = party->GetID();
                if(partyDamage.find(partyID) == partyDamage.end())
                {
                    parties[partyID] = party;
                    partyDamage[partyID] = dmg;
                }
                else
                {
                    partyDamage[partyID] = partyDamage[partyID] + dmg;
                }
            }
            else
            {
                if(s->GetCharacterState()->GetZone() == zone)
                {
                    playerDamage[s->GetWorldCID()] = dmg;
                }
                else
                {
                    // Since the player is not still in the zone,
                    // reduce the total damage since the player will not
                    // receive any XP
                    totalDamage = (totalDamage - dmg);
                }
            }
        }
    }

    // Find all party members that are active in the zone
    std::unordered_map<uint32_t, std::set<int32_t>> membersInZone;
    for(auto pPair : partyDamage)
    {
        for(int32_t memberID : parties[pPair.first]->GetMemberIDs())
        {
            auto c = clientMap[memberID];
            if(c == nullptr)
            {
                c = server->GetManagerConnection()
                    ->GetEntityClient(memberID, true);
                clientMap[memberID] = c;
            }

            if(c)
            {
                auto s = c->GetClientState();
                if(s->GetCharacterState()->GetZone() == zone)
                {
                    membersInZone[pPair.first].insert(memberID);
                }
            }
        }

        // No party members are in the zone
        if(membersInZone[pPair.first].size() == 0)
        {
            // Since no one in the party is still in the zone,
            // reduce the total damage since no member will
            // receive any XP
            totalDamage = (totalDamage - pPair.second);
        }
    }

    // Calculate the XP gains based on damage dealt by players
    // and parties still in the zone
    std::unordered_map<int32_t, int64_t> xpMap;
    for(auto pair : playerDamage)
    {
        xpMap[pair.first] = (int64_t)ceil((double)totalXP *
            (double)pair.second / (double)totalDamage);
    }

    for(auto pair : membersInZone)
    {
        double xp = (double)totalXP *
            (double)partyDamage[pair.first] / (double)totalDamage;

        int64_t partyXP = (int64_t)ceil(xp * 1.0 -
            ((double)(membersInZone.size() - 1) * 0.1));

        for(auto memberID : pair.second)
        {
            xpMap[memberID] = partyXP;
        }
    }

    // Apply the adjusted XP values to each player
    for(auto xpPair : xpMap)
    {
        auto c = clientMap[xpPair.first];
        if(c == nullptr) continue;

        auto s = c->GetClientState();
        std::list<std::shared_ptr<ActiveEntityState>>
            clientStates = { s->GetCharacterState() };
        clientStates.push_back(s->GetDemonState());
        for(auto cState : clientStates)
        {
            if(!cState->Ready()) continue;

            int64_t finalXP = (int64_t)ceil((double)xpPair.second *
                ((double)cState->GetCorrectValue(CorrectTbl::RATE_XP)
                * 0.01));

            characterManager->ExperienceGain(c, (uint64_t)finalXP,
                cState->GetEntityID());
        }
    }
}

void SkillManager::HandleNegotiations(const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<Zone>& zone,
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
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<SkillExecutionContext>& ctx)
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

    FinalizeSkillExecution(client, ctx, activated);

    if(client)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_SWITCH);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(skillID);
        p.WriteS8(toggleOn ? 1 : 0);

        client->QueuePacket(p);

        server->GetTokuseiManager()->Recalculate(source, true,
            std::set<int32_t>{ source->GetEntityID() });
        characterManager->RecalculateStats(client, source->GetEntityID());

        client->FlushOutgoing();
    }
    else
    {
        server->GetTokuseiManager()->Recalculate(source, false);
        source->RecalculateStats(definitionManager);
    }

    return true;
}

bool SkillManager::CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<ProcessingSkill>& pSkill)
{
    ProcessingSkill& skill = *pSkill.get();

    auto damageData = skill.Definition->GetDamage()->GetBattleDamage();
    auto formula = damageData->GetFormula();

    bool isHeal = formula == objects::MiBattleDamageData::Formula_t::HEAL_NORMAL
        || formula == objects::MiBattleDamageData::Formula_t::HEAL_STATIC
        || formula == objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT;
    bool isSimpleDamage = formula == objects::MiBattleDamageData::Formula_t::DMG_NORMAL_SIMPLE;

    uint16_t mod1 = damageData->GetModifier1();
    uint16_t mod2 = damageData->GetModifier2();

    for(SkillTargetResult& target : skill.Targets)
    {
        if(target.HitAvoided) continue;

        bool effectiveHeal = isHeal || target.HitAbsorb;

        switch(formula)
        {
        case objects::MiBattleDamageData::Formula_t::NONE:
            return true;
        case objects::MiBattleDamageData::Formula_t::DMG_NORMAL:
        case objects::MiBattleDamageData::Formula_t::DMG_NORMAL_SIMPLE:
        case objects::MiBattleDamageData::Formula_t::DMG_COUNTER:
        case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
            {
                auto calcState = GetCalculatedState(source, pSkill, false, target.EntityState);
                auto targetState = GetCalculatedState(target.EntityState, pSkill, true, source);

                int16_t sourceLuck = source->GetCorrectValue(CorrectTbl::LUCK, calcState);
                int16_t critValue = (int16_t)(source->GetCorrectValue(CorrectTbl::CRITICAL, calcState) +
                    sourceLuck);
                int16_t critFinal = source->GetCorrectValue(CorrectTbl::FINAL_CRIT_CHANCE, calcState);
                int16_t lbChance = source->GetCorrectValue(CorrectTbl::LB_CHANCE, calcState);

                uint8_t critLevel = 0;
                if(critValue > 0)
                {
                    int16_t critDef1 = targetState->GetCorrectTbl((size_t)CorrectTbl::CRIT_DEF);
                    if(sourceLuck < 50)
                    {
                        critDef1 = (int16_t)(critDef1 + targetState->GetCorrectTbl(
                            (size_t)CorrectTbl::LUCK));
                    }
                    else if(sourceLuck < 67)
                    {
                        critDef1 = (int16_t)(critDef1 + 50);
                    }
                    else
                    {
                        critDef1 = (int16_t)((float)critDef1 + floor(
                            (float)targetState->GetCorrectTbl((size_t)CorrectTbl::LUCK) * 0.75f));
                    }

                    int16_t critDef2 = (int16_t)(10 + floor(
                        (float)targetState->GetCorrectTbl((size_t)CorrectTbl::CRIT_DEF) * 0.1f));

                    float critRate = (float)(floor((float)critValue * 0.2f) *
                        (1.f + ((float)critValue * 0.01f)) / (float)(critDef1 * critDef2)
                        + (float)critFinal);

                    if(RNG(int16_t, 1, 10000) <= (int16_t)(critRate * 100.f))
                    {
                        critLevel = 1;

                        if(lbChance > 0 && RNG(int16_t, 1, 100) <= lbChance)
                        {
                            critLevel = 2;
                        }
                    }
                }

                CorrectTbl resistCorrectType = (CorrectTbl)(skill.EffectiveAffinity + RES_OFFSET);

                float resist = (float)
                    (targetState->GetCorrectTbl((size_t)resistCorrectType) * 0.01);
                if(target.HitAbsorb)
                {
                    // Resistance is not applied during absorption
                    resist = 0;
                }

                target.Damage1 = CalculateDamage_Normal(source, target, pSkill, mod1,
                    target.Damage1Type, skill.EffectiveAffinity, resist, critLevel, isHeal);
                target.Damage2 = CalculateDamage_Normal(source, target, pSkill, mod2,
                    target.Damage2Type, skill.EffectiveAffinity, resist, critLevel, isHeal);

                // Set crits, protect, weakpoint, if not healing
                if(!effectiveHeal)
                {
                    // Set crit-level adjustment flags
                    switch(critLevel)
                    {
                        case 1:
                            target.Flags1 |= FLAG1_CRITICAL;
                            break;
                        case 2:
                            if(target.Damage1 > 30000 ||
                                target.Damage2 > 30000)
                            {
                                target.Flags2 |= FLAG2_INTENSIVE_BREAK;
                            }
                            else
                            {
                                target.Flags2 |= FLAG2_LIMIT_BREAK;
                            }
                            break;
                        default:
                            break;
                    }

                    // Set resistence flags
                    if(resist >= 0.5f)
                    {
                        target.Flags1 |= FLAG1_PROTECT;
                    }
                    else if(resist <= -0.5f)
                    {
                        target.Flags1 |= FLAG1_WEAKPOINT;
                    }
                }

                // Determine pursuit/tech damage
                if(!effectiveHeal && !isSimpleDamage && target.Damage1 > 0)
                {
                    auto tokuseiManager = mServer.lock()->GetTokuseiManager();

                    int32_t pursuitRate = (int32_t)floor(tokuseiManager->GetAspectSum(
                        source, TokuseiAspectType::PURSUIT_RATE, calcState));
                    int32_t pursuitPow = (int32_t)floor(tokuseiManager->GetAspectSum(
                        source, TokuseiAspectType::PURSUIT_POWER, calcState));
                    if(pursuitPow > 0.0 && pursuitRate > 0.0 &&
                        (pursuitRate >= 100.0 || RNG(int32_t, 1, 100) <= pursuitRate))
                    {
                        target.PursuitAffinity = skill.EffectiveAffinity;

                        // Take the lowest value applied tokusei affinity override if one exists
                        auto affinityOverrides = tokuseiManager->GetAspectValueList(source,
                            TokuseiAspectType::PURSUIT_AFFINITY_OVERRIDE);
                        if(affinityOverrides.size() > 0)
                        {
                            affinityOverrides.sort();
                            target.PursuitAffinity = (uint8_t)affinityOverrides.front();
                        }

                        // If the result is weapon affinity, match it
                        if(target.PursuitAffinity == 1)
                        {
                            target.PursuitAffinity = skill.WeaponAffinity;
                        }

                        // If the pursuit affinity does not match the effective affinity,
                        // the damage must be recalculated first
                        if(target.PursuitAffinity != skill.EffectiveAffinity)
                        {
                            // Check NRA for pursuit affinity and stop if it is prevented
                            if(!GetNRAResult(target, skill, target.PursuitAffinity, true))
                            {
                                // Calculate the new enemy resistence and determine damage
                                resistCorrectType = (CorrectTbl)(
                                    target.PursuitAffinity + RES_OFFSET);

                                resist = (float)(targetState->GetCorrectTbl(
                                    (size_t)resistCorrectType) * 0.01);

                                uint8_t damageType;
                                target.PursuitDamage = CalculateDamage_Normal(source, target,
                                    pSkill, mod1, damageType, target.PursuitAffinity, resist,
                                    critLevel, false);
                            }
                        }
                        else
                        {
                            target.PursuitDamage = target.Damage1;
                        }

                        if(target.PursuitDamage > 0)
                        {
                            // Apply the rate adjustment
                            target.PursuitDamage = (int32_t)floor(
                                (double)target.PursuitDamage * pursuitPow * 0.01);

                            // Adjust for 100% limit
                            if(target.PursuitDamage > target.Damage1)
                            {
                                target.PursuitDamage = target.Damage1;
                            }
                        }
                    }

                    int32_t techRate = (int32_t)floor(tokuseiManager->GetAspectSum(
                        source, TokuseiAspectType::TECH_ATTACK_RATE, calcState));
                    double techPow = floor(tokuseiManager->GetAspectSum(
                        source, TokuseiAspectType::TECH_ATTACK_POWER, calcState));
                    if(techPow > 0.0 && techRate > 0 &&
                        (techRate >= 100 || RNG(int32_t, 1, 100) <= techRate))
                    {
                        // Calculate relative damage
                        target.TechnicalDamage = (int32_t)floor(
                            (double)target.Damage1 * techPow * 0.01);

                        // Apply limits
                        if(critLevel == 2)
                        {
                            // Cap at LB limit
                            int32_t maxLB = (int32_t)(30000 + floor(
                                tokuseiManager->GetAspectSum(source,
                                TokuseiAspectType::LIMIT_BREAK_MAX, calcState)));

                            if(target.TechnicalDamage > maxLB)
                            {
                                target.TechnicalDamage = maxLB;
                            }
                        }
                        else if(target.TechnicalDamage > 9999)
                        {
                            target.TechnicalDamage = 9999;
                        }
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
                    static_cast<int32_t>(source->GetCoreStats()->GetHP() +
                        skill.Activated->GetHPCost()));
                target.Damage2 = CalculateDamage_Percent(
                    mod2, target.Damage2Type,
                    static_cast<int32_t>(source->GetCoreStats()->GetMP() +
                        skill.Activated->GetMPCost()));
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
        default:
            LOG_ERROR(libcomp::String("Unknown damage formula type encountered: %1\n")
                .Arg((uint8_t)formula));
            return false;
        }

        // Reduce for AOE and make sure at least 1 damage was dealt to each specified type
        float aoeReduction = (float)damageData->GetAoeReduction();
        if(mod1)
        {
            if(!target.PrimaryTarget && aoeReduction)
            {
                target.Damage1 = (uint16_t)((float)target.Damage1 *
                    (float)(1.f - (0.01f * aoeReduction)));
            }

            if(target.Damage1 == 0)
            {
                target.Damage1 = 1;
            }
        }
    
        if(mod2)
        {
            if(!target.PrimaryTarget && aoeReduction)
            {
                target.Damage2 = (uint16_t)((float)target.Damage2 *
                    (float)(1.f - (0.01f * aoeReduction)));
            }

            if(target.Damage2 == 0)
            {
                target.Damage2 = 1;
            }
        }

        // If the damage was actually a heal, invert the amount and change the type
        if(effectiveHeal)
        {
            target.Damage1 = target.Damage1 * -1;
            target.Damage2 = target.Damage2 * -1;
            target.Damage1Type = (target.Damage1Type == DAMAGE_TYPE_GENERIC) ?
                (isHeal ? DAMAGE_TYPE_HEALING : DAMAGE_TYPE_DRAIN) : target.Damage1Type;
            target.Damage2Type = (target.Damage2Type == DAMAGE_TYPE_GENERIC) ?
                (isHeal ? DAMAGE_TYPE_HEALING : DAMAGE_TYPE_DRAIN) : target.Damage2Type;
        }
    }

    if(skill.IsSuicide)
    {
        auto selfTarget = GetSelfTarget(source, skill.Targets, true);

        selfTarget->Damage1 = source->GetCoreStats()->GetHP();
        selfTarget->Damage1Type = DAMAGE_TYPE_GENERIC;
    }

    return true;
}

int32_t SkillManager::CalculateDamage_Normal(const std::shared_ptr<
    ActiveEntityState>& source, SkillTargetResult& target,
    const std::shared_ptr<ProcessingSkill>& pSkill, uint16_t mod,
    uint8_t& damageType, uint8_t affinity, float resist, uint8_t critLevel, bool isHeal)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        ProcessingSkill& skill = *pSkill.get();
        auto damageData = skill.Definition->GetDamage()->GetBattleDamage();
        bool isSimpleDamage = damageData->GetFormula() ==
            objects::MiBattleDamageData::Formula_t::DMG_NORMAL_SIMPLE;

        auto calcState = GetCalculatedState(source, pSkill, false, target.EntityState);
        auto targetState = GetCalculatedState(target.EntityState, pSkill, true, source);

        uint16_t off = CalculateOffenseValue(source, target.EntityState, pSkill);
        if(isHeal)
        {
            off = (uint16_t)(off *
                (source->GetCorrectValue(CorrectTbl::RATE_HEAL, calcState) * 0.01));
        }

        CorrectTbl boostCorrectType = (CorrectTbl)(affinity + BOOST_OFFSET);

        float boost = (float)(source->GetCorrectValue(boostCorrectType, calcState) * 0.01);
        if(boost < -100.f)
        {
            boost = -100.f;
        }

        uint16_t def = 0;
        uint8_t rateDefBoostIdx = 0;
        switch(skill.EffectiveDependencyType)
        {
        case 0:
        case 9:
        case 12:
            def = (uint16_t)targetState->GetCorrectTbl((size_t)CorrectTbl::PDEF);
            rateDefBoostIdx = (uint8_t)CorrectTbl::RATE_CLSR_TAKEN;
            break;
        case 1:
        case 6:
        case 10:
            def = (uint16_t)targetState->GetCorrectTbl((size_t)CorrectTbl::PDEF);
            rateDefBoostIdx = (uint8_t)CorrectTbl::RATE_LNGR_TAKEN;
            break;
        case 2:
        case 7:
        case 8:
        case 11:
            def = (uint16_t)targetState->GetCorrectTbl((size_t)CorrectTbl::MDEF);
            rateDefBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL_TAKEN;
            break;
        case 3:
            // Do not defend against support by default
            def = 0;
            rateDefBoostIdx = (uint8_t)CorrectTbl::RATE_SUPPORT_TAKEN;
            break;
        case 5:
        default:
            break;
        }

        def = (uint16_t)(def + target.GuardModifier);

        int16_t dependencyTaken = 100;
        if(rateDefBoostIdx != 0)
        {
            dependencyTaken = targetState->GetCorrectTbl((size_t)rateDefBoostIdx);
        }

        if(dependencyTaken < -100)
        {
            dependencyTaken = -100;
        }

        auto tokuseiManager = mServer.lock()->GetTokuseiManager();

        float scale = 0.f;
        switch(critLevel)
        {
            case 1:	    // Critical hit
                scale = 1.2f;
                break;
            case 2:	    // Limit Break
                scale = 1.5f * (float)source->GetCorrectValue(
                    CorrectTbl::LB_DAMAGE, calcState) * 0.01f;
                break;
            default:	// Normal hit, 80%-99% damage
                scale = RNG_DEC(float, 0.8f, 0.99f, 2);
                break;
        }

        float calc = 0.f;
        if(isSimpleDamage)
        {
            // Simple damage starts with modifier/2
            calc = (float)mod * 0.5f;
        }
        else
        {
            // Normal damage starts with offense stat * modifier/100
            calc = (float)off * ((float)mod * 0.01f);
        }

        // Add the expertise rank
        //calc = calc + (float)exp;

        // Subtract the enemy defense, unless its a critical or limit break
        calc = calc - (float)(critLevel > 0 ? 0 : def);

        if(calc > 0.f)
        {
            // Scale the current value by the critical, limit break or min to
            // max damage factor
            calc = calc * scale;

            // Multiply by 100% + -resistance
            calc = calc * (1.f + resist * -1.f);

            // Multiply by 100% + boost
            calc = calc * (1.f + boost);

            // Multiply by 1 + remaining power boosts/100
            calc = calc * (float)(1.0 + tokuseiManager->GetAspectSum(source,
                TokuseiAspectType::DAMAGE_DEALT, calcState) * 0.01);

            // Multiply by 100% + dependency damage taken
            calc = calc * (float)(1.0 + dependencyTaken * 0.01);

            // Multiply by 100% + -general damage taken
            calc = calc * (float)(1.0 + tokuseiManager->GetAspectSum(
                target.EntityState, TokuseiAspectType::DAMAGE_TAKEN,
                targetState) * -0.01);

            /// @todo: there is more to this calculation

            amount = (int32_t)ceil(calc);
        }
        else
        {
            amount = 1;
        }

        damageType = DAMAGE_TYPE_GENERIC;

        if(critLevel == 2)
        {
            // Apply LB upper limit
            int32_t maxLB = (int32_t)(30000 + floor(tokuseiManager->GetAspectSum(source,
                TokuseiAspectType::LIMIT_BREAK_MAX, calcState)));

            if(amount > maxLB)
            {
                amount = maxLB;
            }
        }
        else if(amount > 9999)
        {
            amount = 9999;
        }
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
    int32_t current)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        amount = (int32_t)ceil((float)current * ((float)mod * 0.01f));
        damageType = DAMAGE_TYPE_GENERIC;
    }

    if(amount > 9999)
    {
        amount = 9999;
    }

    return amount;
}

int32_t SkillManager::CalculateDamage_MaxPercent(uint16_t mod, uint8_t& damageType,
    int32_t max)
{
    int32_t amount = 0;

    if(mod != 0)
    {
        amount = (int32_t)ceil((float)max * ((float)mod * 0.01f));
        damageType = DAMAGE_TYPE_GENERIC;
    }

    if(amount > 9999)
    {
        amount = 9999;
    }

    return amount;
}

SkillTargetResult* SkillManager::GetSelfTarget(const std::shared_ptr<ActiveEntityState>& source,
    std::list<SkillTargetResult>& targets, bool indirectDefault)
{
    for(SkillTargetResult& target : targets)
    {
        if(target.EntityState == source)
        {
            return &target;
        }
    }

    // Does not exist so create it
    SkillTargetResult target;
    target.EntityState = source;
    target.IndirectTarget = indirectDefault;
    targets.push_back(target);
    return &targets.back();
}

bool SkillManager::SetNRA(SkillTargetResult& target, ProcessingSkill& skill)
{
    uint8_t resultIdx = GetNRAResult(target, skill, skill.EffectiveAffinity);
    switch(resultIdx)
    {
    case NRA_NULL:
        switch(skill.EffectiveDependencyType)
        {
        case 0:
        case 1:
        case 6:
        case 9:
        case 10:
        case 12:
            target.HitNull = 1;	// Physical null
            break;
        default:
            target.HitNull = 2; // Magic null
            break;
        }
        target.HitAvoided = true;
        return false;
    case NRA_REFLECT:
        switch(skill.EffectiveDependencyType)
        {
        case 0:
        case 1:
        case 6:
        case 9:
        case 10:
        case 12:
            target.HitReflect = 1;  // Physical reflect
            break;
        default:
            target.HitReflect = 2;  // Magic reflect
            break;
        }
        target.HitAvoided = true;
        return true;
    case NRA_ABSORB:
        target.HitAbsorb = true;
        return false;
    default:
        return false;
    }
}

uint8_t SkillManager::GetNRAResult(SkillTargetResult& target, ProcessingSkill& skill,
    uint8_t effectiveAffinity, bool effectiveOnly)
{
    std::list<CorrectTbl> affinities;
    if(!effectiveOnly)
    {
        // Calculate affinity checks for physical vs magic and both base and effective
        // values if they differ
        if(effectiveAffinity != 11)
        {
            // Gather based on dependency type and base affinity if not almighty
            switch(skill.EffectiveDependencyType)
            {
            case 0:
            case 1:
            case 6:
            case 9:
            case 10:
            case 12:
                affinities.push_back(CorrectTbl::NRA_PHYS);
                break;
            case 2:
            case 7:
            case 8:
            case 11:
                affinities.push_back(CorrectTbl::NRA_MAGIC);
                break;
            case 3: // Support needs to be explicitly set
            case 5:
            default:
                break;
            }

            if(skill.BaseAffinity != effectiveAffinity)
            {
                affinities.push_back((CorrectTbl)(skill.BaseAffinity + NRA_OFFSET));
            }
        }
    }

    affinities.push_back((CorrectTbl)(effectiveAffinity + NRA_OFFSET));

    uint8_t resultIdx = 0;
    for(auto nraIdx : target.EntityState->PopNRAShields(affinities))
    {
        if(nraIdx > resultIdx)
        {
            resultIdx = nraIdx;
        }
    }

    if(resultIdx > 0)
    {
        return resultIdx;
    }

    // Check NRA chances
    auto calcState = skill.TargetCalcStates[target.EntityState->GetEntityID()];
    for(CorrectTbl affinity : affinities)
    {
        for(auto nraIdx : { NRA_ABSORB, NRA_REFLECT, NRA_NULL })
        {
            int16_t chance = target.EntityState->GetNRAChance((uint8_t)nraIdx,
                affinity, calcState);
            if(chance >= 100 || (chance > 0 && RNG(int16_t, 1, 100) <= chance))
            {
                return (uint8_t)nraIdx;
            }
        }
    }

    return 0;
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

    auto serverDataManager = mServer.lock()->GetServerDataManager();

    std::list<std::shared_ptr<objects::ItemDrop>> drops;
    if(giftMode)
    {
        for(auto drop : spawn->GetGifts())
        {
            drops.push_back(drop);
        }

        for(uint32_t dropSetID : spawn->GetGiftSetIDs())
        {
            auto dropSet = serverDataManager->GetDropSetData(dropSetID);
            if(dropSet)
            {
                for(auto drop : dropSet->GetDrops())
                {
                    drops.push_back(drop);
                }
            }
        }
    }
    else if(spawn)
    {
        // Add specific spawn drops, then drop sets, then global drops
        for(auto drop : spawn->GetDrops())
        {
            drops.push_back(drop);
        }

        for(uint32_t dropSetID : spawn->GetDropSetIDs())
        {
            auto dropSet = serverDataManager->GetDropSetData(dropSetID);
            if(dropSet)
            {
                for(auto drop : dropSet->GetDrops())
                {
                    drops.push_back(drop);
                }
            }
        }
    }

    /// @todo: add global drops

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
    const std::shared_ptr<SkillExecutionContext>& ctx,
    std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(
        activated->GetSourceEntity());
    auto zone = source->GetZone();
    auto pSkill = GetProcessingSkill(activated, ctx, client);
    auto skillData = pSkill->Definition;

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto tokuseiManager = server->GetTokuseiManager();

    // Now pay the costs
    int32_t hpCost = activated->GetHPCost();
    int32_t mpCost = activated->GetMPCost();
    bool hpMpCost = hpCost > 0 || mpCost > 0;
    if(hpMpCost)
    {
        source->SetHPMP((int32_t)-hpCost, (int32_t)-mpCost, true);
    }

    if(client)
    {
        if(hpMpCost)
        {
            std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
            displayStateModified.insert(source);
            characterManager->UpdateWorldDisplayState(displayStateModified);

            tokuseiManager->Recalculate(source,
                std::set<TokuseiConditionType>
                { TokuseiConditionType::CURRENT_HP, TokuseiConditionType::CURRENT_MP });
        }

        auto itemCosts = activated->GetItemCosts();
        uint16_t bulletCost = activated->GetBulletCost();

        int64_t targetItem = activated->GetActivationObjectID();
        if(bulletCost > 0)
        {
            auto state = client->GetClientState();
            auto character = state->GetCharacterState()->GetEntity();
            auto bullets = character->GetEquippedItems((size_t)
                objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BULLETS);
            if(bullets)
            {
                itemCosts[bullets->GetType()] = (uint32_t)bulletCost;
                targetItem = state->GetObjectID(bullets.GetUUID());
            }
        }

        if(itemCosts.size() > 0)
        {
            characterManager->AddRemoveItems(client, itemCosts, false, targetItem);
        }
    }

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

    bool end = SetSkillCompleteState(pSkill, true);

    SendExecuteSkill(activated);

    if(client && source->GetEntityType() ==
        objects::EntityStateObject::EntityType_t::CHARACTER)
    {
        auto calcState = GetCalculatedState(source, pSkill, false, nullptr);
        float multiplier = (float)(1.f + source->GetCorrectValue(CorrectTbl::RATE_EXPERTISE,
            calcState) * 0.01f);
        characterManager->UpdateExpertise(client, activated->GetSkillID(), multiplier);
    }

    // Update the execution count and remove and complete it from the entity
    // if its at max and not a guard
    if(end)
    {
        source->SetActivatedAbility(nullptr);
        SendCompleteSkill(activated, 0);
    }

    if(client)
    {
        // Cancel any status effects that expire on skill execution
        characterManager->CancelStatusEffects(client, EFFECT_CANCEL_SKILL);
    }
    else
    {
        source->CancelStatusEffects(EFFECT_CANCEL_SKILL);
    }
}

bool SkillManager::SetSkillCompleteState(const std::shared_ptr<
    channel::ProcessingSkill>& pSkill, bool executed)
{
    auto activated = pSkill->Activated;
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(
        activated->GetSourceEntity());
    auto skillData = pSkill->Definition;

    auto server = mServer.lock();
    auto tokuseiManager = server->GetTokuseiManager();

    uint64_t currentTime = activated->GetExecutionTime();

    auto calcState = GetCalculatedState(source, pSkill, false, nullptr);

    // Stack adjust is affected by 2 sources if not an item skill or just
    // explicit item including adjustments if it is an item skill
    uint8_t maxStacks = (uint8_t)(skillData->GetCast()->GetBasic()->GetUseCount() +
        tokuseiManager->GetAspectSum(source,
            TokuseiAspectType::SKILL_ITEM_STACK_ADJUST, calcState) +
        (!pSkill->IsItemSkill ? tokuseiManager->GetAspectSum(source,
            TokuseiAspectType::SKILL_STACK_ADJUST, calcState) : 0));

    uint8_t execCount = activated->GetExecuteCount();
    bool moreUses = execCount < maxStacks;

    // If the skill was executed, set lockout time and increase
    // the execution count
    if(executed)
    {
        auto dischargeData = skillData->GetDischarge();
        uint32_t stiffness = dischargeData->GetStiffness();

        uint64_t lockOutTime = currentTime + (uint64_t)(stiffness * 1000);
        source->SetStatusTimes(STATUS_IMMOBILE, lockOutTime);

        if(source->IsMoving())
        {
            server->GetZoneManager()->FixCurrentPosition(source, lockOutTime,
                currentTime);
        }

        activated->SetExecuteCount((uint8_t)(activated->GetExecuteCount() + 1));

        activated->SetLockOutTime(lockOutTime);
    }

    // Set the cooldown if no remaining uses are available
    uint32_t cdTime = skillData->GetCondition()->GetCooldownTime();

    uint64_t cooldownTime = currentTime;
    if(cdTime && (!moreUses || (execCount > 0 && !executed)))
    {
        cooldownTime = cooldownTime + (uint64_t)((double)(cdTime * 1000) *
            (source->GetCorrectValue(CorrectTbl::COOLDOWN_TIME, calcState) * 0.01));
    }

    activated->SetCooldownTime(cooldownTime);

    return !executed || !moreUses;
}

bool SkillManager::SpecialSkill(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    (void)activated;
    (void)ctx;
    (void)client;

    return true;
}

bool SkillManager::DCM(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    if(!client)
    {
        SendFailure(source, activated->GetSkillID(), nullptr);
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();
    auto demonData = dState->GetDevilData();

    if(!demon || !demonData)
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
        return false;
    }

    if(!dState->IsAlive())
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
        return false;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto bookData = definitionManager->GetDevilBookData(demon->GetType());
    if(!bookData)
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::GENERIC_USE);
        return false;
    }

    std::unordered_map<uint32_t, uint32_t> removeItems;
    removeItems[bookData->GetEntryID()] = 1;
    if(ProcessSkillResult(activated, ctx) && characterManager->AddRemoveItems(client,
        removeItems, false, activated->GetActivationObjectID()))
    {
        auto character = cState->GetEntity();
        auto progress = character->GetProgress().Get();

        size_t index;
        uint8_t shiftVal;
        characterManager->ConvertIDToMaskValues((uint16_t)bookData->GetShiftValue(),
            index, shiftVal);

        uint8_t currentVal = progress->GetDevilBook(index);
        uint8_t newVal = (uint8_t)(currentVal | shiftVal);

        if(newVal != currentVal)
        {
            progress->SetDevilBook(index, newVal);

            server->GetWorldDatabase()->QueueUpdate(progress, state->GetAccountUID());

            server->GetCharacterManager()->SendDevilBook(client);

            if(dState->UpdateSharedState(character, definitionManager))
            {
                // If this resulted in an update, recalculate tokusei
                server->GetTokuseiManager()->Recalculate(cState, true,
                    std::set<int32_t>{ dState->GetEntityID() });
            }

            // Always recalculate stats
            characterManager->RecalculateStats(client, dState->GetEntityID());
        }

        return true;
    }
    else
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::GENERIC_USE);
        return false;
    }
}

bool SkillManager::EquipItem(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    if(!client)
    {
        SendFailure(source, activated->GetSkillID(), nullptr);
        return false;
    }

    auto itemID = activated->GetTargetObjectID();
    if(itemID <= 0)
    {
        SendFailure(source, activated->GetSkillID(), client);
        return false;
    }

    mServer.lock()->GetCharacterManager()->EquipItem(client, itemID);

    return ProcessSkillResult(activated, ctx);
}

bool SkillManager::FamiliarityUp(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    if(!client)
    {
        SendFailure(source, activated->GetSkillID(), nullptr);
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();
    auto demonData = dState->GetDevilData();

    if(!demon || !demonData)
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
        return false;
    }

    if(!dState->IsAlive())
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
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
            SendFailure(source, activated->GetSkillID(), client);
            return false;
        }
    }

    int32_t fType = demonData->GetFamiliarity()->GetFamiliarityType();

    if(fType > 16)
    {
        SendFailure(source, activated->GetSkillID(), client);
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

    bool sameLNC = cState->GetLNCType() == dState->GetLNCType();

    int32_t fPoints = (int32_t)fTypeMap[(size_t)fType][sameLNC ? 0 : 1];
    server->GetCharacterManager()->UpdateFamiliarity(client, fPoints, true);

    // Apply the status effects
    AddStatusEffectMap m;
    for(auto addStatus : skillData->GetDamage()->GetAddStatuses())
    {
        uint8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
            addStatus->GetMaxStack());
        if(stack == 0 && !addStatus->GetIsReplace()) continue;

        m[addStatus->GetStatusID()] =
            std::pair<uint8_t, bool>(stack, addStatus->GetIsReplace());
    }

    if(m.size() > 0)
    {
        cState->AddStatusEffects(m, definitionManager);
        server->GetTokuseiManager()->Recalculate(cState,
            std::set<TokuseiConditionType> { TokuseiConditionType::STATUS_ACTIVE });
    }

    // Process the skill without status effects
    ctx->ApplyStatusEffects = false;
    return ProcessSkillResult(activated, ctx);
}

bool SkillManager::FamiliarityUpItem(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    if(!client)
    {
        SendFailure(source, activated->GetSkillID(), nullptr);
        return false;
    }

    auto state = client->GetClientState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();
    auto demonData = dState->GetDevilData();

    if(!demon || !demonData)
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
        return false;
    }

    if(!dState->IsAlive())
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
        return false;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillData = definitionManager->GetSkillData(activated->GetSkillID());

    auto special = skillData->GetSpecial();

    int32_t maxFamiliarity = special->GetSpecialParams(0);
    float deltaPercent = (float)special->GetSpecialParams(1);
    int32_t minIncrease = special->GetSpecialParams(2);
    int32_t raceRestrict = special->GetSpecialParams(3);

    if(raceRestrict && (int32_t)demonData->GetCategory()->GetRace() != raceRestrict)
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_INCOMPATIBLE);
        return false;
    }

    uint16_t currentVal = demon->GetFamiliarity();
    if(maxFamiliarity > (int32_t)currentVal)
    {
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
    }

    return ProcessSkillResult(activated, ctx);
}

bool SkillManager::Mooch(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    if(!client)
    {
        SendFailure(source, activated->GetSkillID(), nullptr);
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(!demon)
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
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
            SendFailure(source, activated->GetSkillID(), client,
                (uint8_t)SkillErrorCodes_t::GENERIC_USE);
            return false;
        }
    }

    /// @todo: receive items from demon

    server->GetCharacterManager()->UpdateFamiliarity(client, -2000, true);

    // Apply the status effects
    AddStatusEffectMap m;
    for(auto addStatus : skillData->GetDamage()->GetAddStatuses())
    {
        uint8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
            addStatus->GetMaxStack());
        if(stack == 0 && !addStatus->GetIsReplace()) continue;

        m[addStatus->GetStatusID()] =
            std::pair<uint8_t, bool>(stack, addStatus->GetIsReplace());
    }

    if(m.size() > 0)
    {
        cState->AddStatusEffects(m, definitionManager);
        server->GetTokuseiManager()->Recalculate(cState,
            std::set<TokuseiConditionType> { TokuseiConditionType::STATUS_ACTIVE });
    }

    // Process the skill without status effects
    ctx->ApplyStatusEffects = false;
    return ProcessSkillResult(activated, ctx);
}

bool SkillManager::Rest(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    (void)ctx;
    (void)client;

    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillData = definitionManager->GetSkillData(activated->GetSkillID());

    source->ExpireStatusTimes(ChannelServer::GetServerTime());
    if(source->StatusTimesKeyExists(STATUS_RESTING))
    {
        // Expire the status
        std::set<uint32_t> expire;
        for(auto addStatus : skillData->GetDamage()->GetAddStatuses())
        {
            expire.insert(addStatus->GetStatusID());
        }

        source->ExpireStatusEffects(expire);
        source->RemoveStatusTimes(STATUS_RESTING);
    }
    else
    {
        // Add the status
        AddStatusEffectMap m;
        for(auto addStatus : skillData->GetDamage()->GetAddStatuses())
        {
            uint8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
                addStatus->GetMaxStack());
            if(stack == 0 && !addStatus->GetIsReplace()) continue;

            m[addStatus->GetStatusID()] =
                std::pair<uint8_t, bool>(stack, addStatus->GetIsReplace());
        }
        source->AddStatusEffects(m, definitionManager);

        source->SetStatusTimes(STATUS_RESTING, 0);
    }

    return true;
}

bool SkillManager::SummonDemon(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    (void)ctx;

    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    if(!client)
    {
        SendFailure(source, activated->GetSkillID(), nullptr);
        return false;
    }

    auto demonID = activated->GetTargetObjectID();
    if(demonID <= 0)
    {
        LOG_ERROR(libcomp::String("Invalid demon specified to summon: %1\n")
            .Arg(demonID));

        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::SUMMON_INVALID);
        return false;
    }

    mServer.lock()->GetCharacterManager()->SummonDemon(client, demonID);

    return true;
}

bool SkillManager::StoreDemon(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    (void)ctx;

    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    if(!client)
    {
        SendFailure(source, activated->GetSkillID(), nullptr);
        return false;
    }

    auto demonID = activated->GetTargetObjectID();
    if(demonID <= 0)
    {
        LOG_ERROR(libcomp::String("Invalid demon specified to store: %1\n")
            .Arg(demonID));

        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
        return false;
    }

    mServer.lock()->GetCharacterManager()->StoreDemon(client);

    return true;
}

bool SkillManager::Traesto(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());

    if(!client)
    {
        SendFailure(source, activated->GetSkillID(), nullptr);
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    uint32_t zoneID = character->GetHomepointZone();
    if(zoneID == 0)
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::ZONE_INVALID);
        return false;
    }

    float xCoord = 0.f;
    float yCoord = 0.f;
    float rot = 0.f;

    auto zoneDef = mServer.lock()->GetServerDataManager()->GetZoneData(
        character->GetHomepointZone(), 0);
    if(!zoneDef && !mServer.lock()->GetZoneManager()->GetSpotPosition(
        zoneDef->GetDynamicMapID(), character->GetHomepointSpotID(), xCoord, yCoord, rot))
    {
        SendFailure(source, activated->GetSkillID(), client,
            (uint8_t)SkillErrorCodes_t::ZONE_INVALID);
        return false;
    }

    ProcessSkillResult(activated, ctx);

    return mServer.lock()->GetZoneManager()->EnterZone(client, zoneID, 0, xCoord, yCoord, rot, true);
}

void SkillManager::SendActivateSkill(std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<objects::MiSkillData> skillData)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());
    auto zone = source ? source->GetZone() : nullptr;
    auto zConnections = zone
        ? zone->GetConnectionList() : std::list<std::shared_ptr<ChannelClientConnection>>();
    if(zConnections.size() > 0)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_ACTIVATED);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(activated->GetSkillID());
        p.WriteS8((int8_t)activated->GetActivationID());

        p.WriteFloat(ChannelServer::ToSyncTime(activated->GetChargedTime()));

        uint8_t useCount = skillData->GetCast()->GetBasic()->GetUseCount();
        p.WriteU8(useCount);
        p.WriteU8(2);   //Unknown

        p.WriteFloat(activated->GetChargeMoveSpeed());
        p.WriteFloat(activated->GetChargeCompleteMoveSpeed());

        ChannelClientConnection::BroadcastPacket(zConnections, p);
    }
}

void SkillManager::SendExecuteSkill(std::shared_ptr<objects::ActivatedAbility> activated)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());
    auto zone = source ? source->GetZone() : nullptr;
    auto zConnections = zone
        ? zone->GetConnectionList() : std::list<std::shared_ptr<ChannelClientConnection>>();
    if(zConnections.size() > 0)
    {
        int32_t targetedEntityID = activated->GetEntityTargeted()
            ? (int32_t)activated->GetTargetObjectID()
            : source->GetEntityID();

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_EXECUTED);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(activated->GetSkillID());
        p.WriteS8((int8_t)activated->GetActivationID());
        p.WriteS32Little(targetedEntityID);

        p.WriteFloat(ChannelServer::ToSyncTime(activated->GetCooldownTime()));
        p.WriteFloat(ChannelServer::ToSyncTime(activated->GetLockOutTime()));

        p.WriteU32Little((uint32_t)activated->GetHPCost());
        p.WriteU32Little((uint32_t)activated->GetMPCost());
        p.WriteU8(0);   //Unknown
        p.WriteFloat(0);    //Unknown
        p.WriteFloat(0);    //Unknown
        p.WriteFloat(0);    //Unknown
        p.WriteFloat(0);    //Unknown
        p.WriteFloat(0);    //Unknown
        p.WriteU8(0);   //Unknown
        p.WriteU8(0xFF);   //Unknown

        ChannelClientConnection::BroadcastPacket(zConnections, p);
    }
}

void SkillManager::SendCompleteSkill(std::shared_ptr<objects::ActivatedAbility> activated,
    uint8_t mode)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());
    auto zone = source ? source->GetZone() : nullptr;
    auto zConnections = zone
        ? zone->GetConnectionList() : std::list<std::shared_ptr<ChannelClientConnection>>();
    if(zConnections.size() > 0)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_COMPLETED);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(activated->GetSkillID());
        p.WriteS8((int8_t)activated->GetActivationID());

        // Write the cooldown time if cancelling in case its set (mostly for multi-use skills)
        p.WriteFloat(ChannelServer::ToSyncTime(mode == 1 ? activated->GetCooldownTime() : 0));
        p.WriteU8(1);   // Unknown, always the same
        p.WriteFloat(source->GetMovementSpeed());
        p.WriteU8(mode);

        ChannelClientConnection::BroadcastPacket(zConnections, p);
    }
}
