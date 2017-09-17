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
#include <SpawnGroup.h>

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
const uint16_t FLAG1_DODGED = 1 << 5;
const uint16_t FLAG1_CRITICAL = 1 << 6;
const uint16_t FLAG1_WEAKPOINT = 1 << 7;
const uint16_t FLAG1_KNOCKBACK = 1 << 8;
const uint16_t FLAG1_REVIVAL = 1 << 9;
const uint16_t FLAG1_ABSORB = 1 << 10;  //Only displayed with DAMAGE_TYPE_HEALING
const uint16_t FLAG1_REFLECT = 1 << 11; //Only displayed with DAMAGE_TYPE_NONE
const uint16_t FLAG1_BLOCK = 1 << 12;   //Only dsiplayed with DAMAGE_TYPE_NONE
const uint16_t FLAG1_RUSH_MOVEMENT = 1 << 14;
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
const uint8_t AIL_OFFSET = (uint8_t)((uint8_t)CorrectTbl::RES_FIRE -
    (uint8_t)CorrectTbl::RES_SLASH + 1);
/*const uint8_t DAMAGE_TAKEN_OFFSET = (uint8_t)((uint8_t)CorrectTbl::RATE_CLSR -
    (uint8_t)CorrectTbl::RATE_CLSR_TAKEN);*/

class channel::ProcessingSkill
{
public:
    std::shared_ptr<objects::MiSkillData> Definition;
    SkillExecutionContext* ExecutionContext = 0;
    uint8_t BaseAffinity;
    uint8_t EffectiveAffinity;
    uint8_t EffectiveDependencyType;
    uint16_t OffenseValue = 0;
    bool IsSuicide = false;
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
    uint16_t Flags1 = 0;
    uint8_t TalkFlags = 0;
    uint8_t AilmentDamageType = 0;
    int32_t AilmentDamage = 0;
    uint64_t AilmentDamageTime = 0;
    uint16_t Flags2 = 0;
    int32_t TechnicalDamage = 0;
    int32_t PursuitDamage = 0;
    AddStatusEffectMap AddedStatuses;
    std::set<uint32_t> CancelledStatuses;
    bool HitAvoided = false;
    uint16_t GuardModifier = 0;
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

bool SkillManager::ActivateSkill(const std::shared_ptr<ActiveEntityState> source,
    uint32_t skillID, int64_t targetObjectID, const std::shared_ptr<SkillExecutionContext>& ctx)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetSkillData(skillID);
    if (nullptr == def)
    {
        return 0;
    }

    auto cast = def->GetCast();
    auto chargeTime = cast->GetBasic()->GetChargeTime();
    auto activatedTime = ChannelServer::GetServerTime();

    // Charge time is in milliseconds, convert to microseconds
    auto chargedTime = activatedTime + (chargeTime * 1000);

    auto activated = std::shared_ptr<objects::ActivatedAbility>(
        new objects::ActivatedAbility);
    activated->SetSkillID(skillID);
    activated->SetSourceEntity(source);
    activated->SetActivationObjectID(targetObjectID);
    activated->SetTargetObjectID(targetObjectID);
    activated->SetActivationTime(activatedTime);
    activated->SetChargedTime(chargedTime);
    activated->SetActivationID(source->GetNextActivatedAbilityID());

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
        auto client = server->GetManagerConnection()->GetEntityClient(
            source->GetEntityID());
        if(!ExecuteSkill(source, activated, client, ctx))
        {
            SendFailure(source, skillID);
            source->SetActivatedAbility(nullptr);
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
    uint8_t activationID, int64_t targetObjectID, const std::shared_ptr<SkillExecutionContext>& ctx)
{
    auto client = mServer.lock()->GetManagerConnection()->GetEntityClient(
        source->GetEntityID());

    bool success = true;

    uint32_t skillID = 0;
    auto activated = success ? source->GetActivatedAbility() : nullptr;
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

    if(!success || !ExecuteSkill(source, activated, client, ctx))
    {
        SendFailure(source, skillID);
    }

    return success;
}

bool SkillManager::ExecuteSkill(std::shared_ptr<ActiveEntityState> source,
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<ChannelClientConnection> client,
    const std::shared_ptr<SkillExecutionContext>& ctx)
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
    auto targetType = skillData->GetTarget()->GetType();
    if(targetType != objects::MiTargetData::Type_t::NONE &&
        targetType != objects::MiTargetData::Type_t::OBJECT)
    {
        auto targetEntityID = (int32_t)activated->GetTargetObjectID();

        if(targetEntityID <= 0)
        {
            //No target
            return false;
        }

        auto zone = source->GetZone();
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

        bool targetAlive = targetEntity->IsAlive();
        bool allies = targetEntity->GetFaction() == source->GetFaction();
        auto targetEntityType = targetEntity->GetEntityType();
        auto sourceState = ClientState::GetEntityClientState(source->GetEntityID());
        auto targetState = ClientState::GetEntityClientState(targetEntity->GetEntityID());
        switch(targetType)
        {
        case objects::MiTargetData::Type_t::ALLY:
            if(!allies || !targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::DEAD_ALLY:
            if(!allies || targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::PARTNER:
            if(!sourceState || sourceState->GetCharacterState() != source ||
                sourceState->GetDemonState() != targetEntity || !targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::PARTY:
            if(!sourceState || sourceState->GetPartyID() == 0 || !targetState ||
                sourceState->GetPartyID() != targetState->GetPartyID() || !targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::ENEMY:
            if(allies || !targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::DEAD_PARTNER:
            if(!sourceState || sourceState->GetCharacterState() != source ||
                sourceState->GetDemonState() != targetEntity || targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::OTHER_PLAYER:
            if(targetEntityType != objects::EntityStateObject::EntityType_t::CHARACTER ||
                sourceState == targetState || !allies || !targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::OTHER_DEMON:
            if(targetEntityType != objects::EntityStateObject::EntityType_t::PARTNER_DEMON ||
                (sourceState && sourceState->GetDemonState() != targetEntity) || !allies ||
                !targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::ALLY_PLAYER:
            if(targetEntityType != objects::EntityStateObject::EntityType_t::CHARACTER ||
                !allies || !targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::ALLY_DEMON:
            if(targetEntityType != objects::EntityStateObject::EntityType_t::PARTNER_DEMON ||
                !allies || !targetAlive)
            {
                return false;
            }
            break;
        case objects::MiTargetData::Type_t::PLAYER:
            if(!sourceState || sourceState->GetCharacterState() != targetEntity)
            {
                return false;
            }
            break;
        default:
            break;
        }

        activated->SetEntityTargeted(true);
    }

    // Check costs and pay costs (skip for switch deactivation)
    int32_t hpCost = 0, mpCost = 0;
    uint16_t bulletCost = 0;
    std::unordered_map<uint32_t, uint32_t> itemCosts;
    if(!(ctx && ctx->FreeCast) && (skillCategory == 1 || (skillCategory == 2 &&
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

        hpCost = (int32_t)(hpCost + ceil(((float)hpCostPercent * 0.01f) *
            (float)source->GetMaxHP()));
        mpCost = (int32_t)(mpCost + ceil(((float)mpCostPercent * 0.01f) *
            (float)source->GetMaxMP()));

        auto sourceStats = source->GetCoreStats();
        bool canPay = ((hpCost == 0) || hpCost < sourceStats->GetHP()) &&
            ((mpCost == 0) || mpCost <= sourceStats->GetMP());

        auto characterManager = server->GetCharacterManager();

        if(itemCosts.size() > 0)
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
                // Non-player entities cannot pay item costs
                canPay = false;
            }
        }

        // Handle costs that can't be paid as expected errors
        if(!canPay)
        {
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
            return ToggleSwitchSkill(client, activated);
        case 0:
            // Passive, shouldn't happen
        default:
            return false;
            break;
        }
    }

    // Only execute special function skills if the source was a player
    bool success = client && fIter->second(*this, activated, ctx, client);
    if(success)
    {
        FinalizeSkillExecution(client, activated, skillData);
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

        SendCompleteSkill(activated, 1);
        source->SetActivatedAbility(nullptr);
        return true;
    }
}

void SkillManager::SendFailure(const std::shared_ptr<ActiveEntityState> source,
    uint32_t skillID, const std::shared_ptr<ChannelClientConnection> client)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_FAILED);
    p.WriteS32Little(source ? source->GetEntityID() : -1);
    p.WriteU32Little(skillID);
    p.WriteS8(-1);  //Unknown
    p.WriteU8(0);  //Unknown
    p.WriteU8(0);  //Unknown
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
        FinalizeSkillExecution(client, activated, skillData);
        return ProcessSkillResult(activated, ctx);
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
        source->RefreshCurrentPosition(activated->GetExecutionTime());
        target->RefreshCurrentPosition(activated->GetExecutionTime());

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
    const std::shared_ptr<SkillExecutionContext>& ctx)
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
    skill.ExecutionContext = ctx.get();
    skill.EffectiveDependencyType = skillData->GetBasic()->GetDependencyType();
    skill.BaseAffinity = skill.EffectiveAffinity = skillData->GetCommon()->GetAffinity();
    skill.IsSuicide = skillData->GetDamage()->GetFunctionID() == SVR_CONST.SKILL_SUICIDE;

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
                switch(weaponDef->GetBasic()->GetWeaponType())
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
    uint16_t initialFlags1 = 0;
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

            initialFlags1 = target.Flags1;
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

                // Max and min radians of the arc's circle
                float maxRotL = sourceRot + maxRotOffset;
                float maxRotR = sourceRot - maxRotOffset;

                for(auto t : potentialTargets)
                {
                    float targetRot = (float)atan2((float)(sourceY - t->GetCurrentY()),
                        (float)(sourceX - t->GetCurrentX()));

                    if(maxRotL >= targetRot && maxRotR <= targetRot)
                    {
                        effectiveTargets.push_back(t);
                    }
                }
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
        if(isPrimaryTarget && (initialFlags1 & FLAG1_REFLECT) == 0)
        {
            target.Flags1 = initialFlags1;
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
    bool hasBattleDamage = damageData->GetBattleDamage()->GetFormula()
        != objects::MiBattleDamageData::Formula_t::NONE;

    // Calculate damage before determining if it hits in case a counter occurs
    if(hasBattleDamage)
    {
        CalculateOffenseValue(source, skill);
    }

    // Determine hit outcomes
    CheckSkillHits(source, activated, targetResults, skill);

    // Run calculations
    if(hasBattleDamage)
    {
        auto battleDamage = damageData->GetBattleDamage();
        if(!CalculateDamage(source, activated, targetResults, skill))
        {
            LOG_ERROR(libcomp::String("Damage failed to calculate: %1\n")
                .Arg(skillID));
            return false;
        }

        // Now that damage is calculated, apply drain
        uint8_t hpDrainPercent = battleDamage->GetHPDrainPercent();
        uint8_t mpDrainPercent = battleDamage->GetMPDrainPercent();
        if(hpDrainPercent > 0 || mpDrainPercent > 0)
        {
            auto selfTarget = GetSelfTarget(source, targetResults);

            int32_t hpDrain = 0, mpDrain = 0;
            for(SkillTargetResult& target : targetResults)
            {
                if(target.Damage1Type == DAMAGE_TYPE_GENERIC && hpDrainPercent > 0)
                {
                    hpDrain = (int32_t)(hpDrain +
                        (int32_t)floorl((float)target.Damage1 * (float)hpDrainPercent * 0.01f));
                }
                
                if(target.Damage2Type == DAMAGE_TYPE_GENERIC && mpDrainPercent > 0)
                {
                    mpDrain = (int32_t)(mpDrain +
                        (int32_t)floorl((float)target.Damage2 * (float)mpDrainPercent * 0.01f));
                }
            }

            // Always heal HP even if value is 0
            selfTarget->Damage1Type = DAMAGE_TYPE_HEALING;
            selfTarget->Damage1 = hpDrain;

            // Heal MP only if the value is over 0
            if(mpDrain > 0)
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

    uint64_t now = activated->GetExecutionTime();
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
        if(target.HitAvoided) continue;

        target.EntityState->RefreshCurrentPosition(now);
        cancellations[target.EntityState] = 0;

        int32_t hpDamage = target.TechnicalDamage;
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
        if(!(ctx && !ctx->ApplyStatusEffects) &&
            (target.Flags1 & (FLAG1_BLOCK | FLAG1_REFLECT | FLAG1_ABSORB)) == 0)
        {
            for(auto addStatus : addStatuses)
            {
                if(addStatus->GetOnKnockback() && !(target.Flags1 & FLAG1_KNOCKBACK)) continue;

                int32_t successRate = (int32_t)addStatus->GetSuccessRate();
                if(successRate >= 100 || RNG(int32_t, 1, 100) <= successRate)
                {
                    auto statusDef = definitionManager->GetStatusData(
                        addStatus->GetStatusID());

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
                            uint8_t affinity = statusDef->GetCommon()->GetAffinity();
                            CorrectTbl resistCorrectType = (CorrectTbl)(affinity + RES_OFFSET);
                            float resist = (float)target.EntityState->GetCorrectValue(resistCorrectType)
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
            
            if(hpAdjusted < 0)
            {
                if(kbMod)
                {
                    float kb = target.EntityState->UpdateKnockback(now, kbMod);
                    if(kb == 0.f)
                    {
                        target.Flags1 |= FLAG1_KNOCKBACK;
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
            bool success = RNG(uint16_t, 1, 100) <= 90;
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

    bool doRush = skillData->GetBasic()->GetActionType() ==
        objects::MiSkillBasicData::ActionType_t::RUSH && primaryTarget;
    if(doRush)
    {
        SkillTargetResult* selfTarget = GetSelfTarget(source, targetResults);
        selfTarget->Flags1 |= FLAG1_RUSH_MOVEMENT;
    }

    auto effectiveTarget = primaryTarget ? primaryTarget : effectiveSource;
    std::unordered_map<uint32_t, uint64_t> timeMap;
    uint64_t hitTimings[3];
    uint64_t completeTime = activated->GetExecutionTime() +
        (skillData->GetDischarge()->GetStiffness() * 1000);
    uint64_t hitStopTime = completeTime +
        (skillData->GetDamage()->GetHitStopTime() * 1000);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
    p.WriteS32Little(source->GetEntityID());
    p.WriteU32Little(skillID);
    p.WriteS8((int8_t)activated->GetActivationID());

    p.WriteU32Little((uint32_t)targetResults.size());
    for(SkillTargetResult& target : targetResults)
    {
        p.WriteS32Little(target.EntityState->GetEntityID());
        p.WriteS32Little(abs(target.Damage1));
        p.WriteU8(target.Damage1Type);
        p.WriteS32Little(abs(target.Damage2));
        p.WriteU8(target.Damage2Type);
        p.WriteU16Little(target.Flags1);

        p.WriteU8(target.AilmentDamageType);
        p.WriteS32Little(abs(target.AilmentDamage));

        bool rushing = false;
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
                        effectiveTarget->GetCurrentX(), effectiveTarget->GetCurrentY(), kbDistance,
                        true, now, hitStopTime);
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
                kbPoint = zoneManager->MoveRelative(target.EntityState, effectiveSource->GetCurrentX(),
                    effectiveSource->GetCurrentY(), kbDistance, true, now, hitStopTime);
                break;
            }

            target.EntityState->SetStatusTimes(STATUS_KNOCKBACK,
                target.EntityState->GetDestinationTicks());

            p.WriteFloat(kbPoint.x);
            p.WriteFloat(kbPoint.y);
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
        else if(target.Damage1Type == DAMAGE_TYPE_GENERIC)
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
                    hitTimings[2] = hitStopTime + target.AilmentDamageTime;
                }

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
            if(hitTimings[i])
            {
                timeMap[(uint32_t)(p.Size() + (4 * i))] = hitTimings[i];
            }
        }

        // Double back at the end and write client specific times
        p.WriteBlank(12);

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

    auto zConnections = zone->GetConnectionList();
    ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);

    if(revived.size() > 0)
    {
        for(auto entity : revived)
        {
            p.Clear();
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

uint16_t SkillManager::CalculateOffenseValue(const std::shared_ptr<ActiveEntityState>& source,
    ProcessingSkill& skill)
{
    auto damageData = skill.Definition->GetDamage()->GetBattleDamage();

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

    if(skill.ExecutionContext && skill.ExecutionContext->CounteredSkill)
    {
        // If countering, modify the offensive value with the offense value
        // of the original skill used
        off = (uint16_t)(off + (skill.ExecutionContext->CounteredSkill->OffenseValue * 2));
    }

    skill.OffenseValue = off;

    return off;
}

void SkillManager::CheckSkillHits(const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    std::list<SkillTargetResult>& targets, ProcessingSkill& skill)
{
    (void)activated;

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    for(SkillTargetResult& target : targets)
    {
        auto tActivated = target.EntityState->GetActivatedAbility();
        if(tActivated)
        {
            auto tSkillData = definitionManager->GetSkillData(tActivated->GetSkillID());
            switch(tSkillData->GetBasic()->GetActionType())
            {
            case objects::MiSkillBasicData::ActionType_t::GUARD:
                HandleGuard(source, target, skill);
                break;
            case objects::MiSkillBasicData::ActionType_t::COUNTER:
                HandleCounter(source, target, skill);
                break;
            case objects::MiSkillBasicData::ActionType_t::DODGE:
                HandleDodge(source, target, skill);
                break;
            default:
                /// @todo: break other cancellable skills
                break;
            }
        }
    }
}

void SkillManager::HandleGuard(const std::shared_ptr<ActiveEntityState>& source,
    SkillTargetResult& target, ProcessingSkill& skill)
{
    auto tActivated = target.EntityState->GetActivatedAbility();
    if(!tActivated)
    {
        return;
    }

    uint8_t activationID = tActivated->GetActivationID();
    bool cancel = !skill.Definition->GetBasic()->GetDefensible();
    if(!cancel)
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();
        auto tSkillData = definitionManager->GetSkillData(tActivated->GetSkillID());
        switch(skill.Definition->GetBasic()->GetActionType())
        {
        case objects::MiSkillBasicData::ActionType_t::ATTACK:
        case objects::MiSkillBasicData::ActionType_t::SPIN:
            target.Flags1 |= FLAG1_GUARDED;
            target.GuardModifier = tSkillData->GetDamage()->GetBattleDamage()->GetModifier1();
            ExecuteSkill(target.EntityState, activationID, (int64_t)source->GetEntityID());
            break;
        case objects::MiSkillBasicData::ActionType_t::RUSH:
            /// @todo: Same as not guarding but with special animation
            cancel = true;
            break;
        default:
            cancel = true;
            break;
        }
    }

    if(cancel)
    {
        CancelSkill(target.EntityState, tActivated->GetActivationID());
    }
}

void SkillManager::HandleCounter(const std::shared_ptr<ActiveEntityState>& source,
    SkillTargetResult& target, ProcessingSkill& skill)
{
    auto tActivated = target.EntityState->GetActivatedAbility();
    if(!tActivated)
    {
        return;
    }

    uint8_t activationID = tActivated->GetActivationID();
    if(skill.Definition->GetBasic()->GetDefensible())
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();
        auto tSkillData = definitionManager->GetSkillData(tActivated->GetSkillID());
        switch(skill.Definition->GetBasic()->GetActionType())
        {
        case objects::MiSkillBasicData::ActionType_t::ATTACK:
        case objects::MiSkillBasicData::ActionType_t::RUSH:
            {
                target.HitAvoided = true;
                auto counterCtx = std::make_shared<SkillExecutionContext>();
                counterCtx->CounteredSkill = &skill;
                ExecuteSkill(target.EntityState, activationID, (int64_t)source->GetEntityID(),
                    counterCtx);
            }
            return;
        default:
            break;
        }
    }

    CancelSkill(target.EntityState, activationID);
}

void SkillManager::HandleDodge(const std::shared_ptr<ActiveEntityState>& source,
    SkillTargetResult& target, ProcessingSkill& skill)
{
    auto tActivated = target.EntityState->GetActivatedAbility();
    if(!tActivated)
    {
        return;
    }

    uint8_t activationID = tActivated->GetActivationID();
    if(skill.Definition->GetBasic()->GetDefensible())
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();
        auto tSkillData = definitionManager->GetSkillData(tActivated->GetSkillID());
        switch(skill.Definition->GetBasic()->GetActionType())
        {
        case objects::MiSkillBasicData::ActionType_t::SHOT:
        case objects::MiSkillBasicData::ActionType_t::RAPID:
            target.Flags1 |= FLAG1_DODGED;
            target.Damage1Type = target.Damage2Type = DAMAGE_TYPE_MISS;
            target.HitAvoided = true;
            ExecuteSkill(target.EntityState, activationID, (int64_t)source->GetEntityID());
            return;
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
        std::set<uint32_t> spawnGroupIDs;
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

            spawnGroupIDs.insert(enemy->GetSpawnGroupID());
        }

        // For each loot body generate and send loot and show the body
        // After this schedule all of the bodies for cleanup after their
        // loot time passes
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
            auto damageSources = enemy->GetDamageSources();

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
        if(questKills.size() > 0)
        {
            server->GetEventManager()->UpdateQuestKillCount(sourceClient,
                questKills);
        }

        // Spawn group defeat actions for all empty groups
        spawnGroupIDs.erase(0);
        for(uint32_t spawnGroupID : spawnGroupIDs)
        {
            auto sg = zone->GetDefinition()->GetSpawnGroups(spawnGroupID);
            if(sg && sg->DefeatActionsCount() > 0 &&
                !zone->GroupHasSpawned(spawnGroupID, true))
            {
                server->GetActionManager()->PerformActions(sourceClient,
                    sg->GetDefeatActions(), 0);
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

            int64_t finalXP = (int64_t)ceil((double)xpPair.second * (
                (double)cState->GetCorrectValue(CorrectTbl::RATE_XP) * 0.01));

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

        characterManager->RecalculateStats(client, source->GetEntityID());

        client->FlushOutgoing();
    }
    else
    {
        source->RecalculateStats(definitionManager);
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

    uint16_t off = skill.OffenseValue;
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

    int16_t sourceLuck = source->GetLUCK();
    int16_t critValue = (int16_t)(source->GetCorrectValue(CorrectTbl::CRITICAL) +
        sourceLuck);
    int16_t critFinal = source->GetCorrectValue(CorrectTbl::FINAL_CRIT_CHANCE);
    int16_t lbChance = source->GetCorrectValue(CorrectTbl::LB_CHANCE);

    uint16_t mod1 = damageData->GetModifier1();
    uint16_t mod2 = damageData->GetModifier2();

    for(SkillTargetResult& target : targets)
    {
        if(target.HitAvoided) continue;

        switch(formula)
        {
        case objects::MiBattleDamageData::Formula_t::NONE:
            return true;
        case objects::MiBattleDamageData::Formula_t::DMG_NORMAL:
        case objects::MiBattleDamageData::Formula_t::DMG_COUNTER:
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

                def = (uint16_t)(def + target.GuardModifier);

                uint8_t critLevel = 0;
                if(critValue > 0)
                {
                    int16_t critDef1 = target.EntityState->GetCorrectValue(CorrectTbl::CRIT_DEF);
                    if(sourceLuck < 50)
                    {
                        critDef1 = (int16_t)(critDef1 + target.EntityState->GetLUCK());
                    }
                    else if(sourceLuck < 67)
                    {
                        critDef1 = (int16_t)(critDef1 + 50);
                    }
                    else
                    {
                        critDef1 = (int16_t)((float)critDef1 + floor(
                            (float)target.EntityState->GetLUCK() * 0.75f));
                    }

                    int16_t critDef2 = (int16_t)(10 + floor(
                        (float)target.EntityState->GetCorrectValue(CorrectTbl::CRIT_DEF) * 0.1f));

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

                float resist = (float)
                    (target.EntityState->GetCorrectValue(resistCorrectType) * 0.01);
                if(target.Flags1 & FLAG1_ABSORB)
                {
                    // Resistance is not applied during absorption
                    resist = 0;
                }

                target.Damage1 = CalculateDamage_Normal(source, mod1,
                    target.Damage1Type, off, def, resist, boost, critLevel);
                target.Damage2 = CalculateDamage_Normal(source, mod2,
                    target.Damage2Type, off, def, resist, boost, critLevel);

                // Crits, protect and weakpoint do not apply to healing
                if(!isHeal && (target.Flags1 & FLAG1_ABSORB) == 0)
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
                            else if(target.Damage1 > 9999 ||
                                target.Damage2 > 9999)
                            {
                                target.Flags2 |= FLAG2_LIMIT_BREAK;
                            }
                            else
                            {
                                target.Flags1 |= FLAG1_CRITICAL;
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
                        activated->GetHPCost()));
                target.Damage2 = CalculateDamage_Percent(
                    mod2, target.Damage2Type,
                    static_cast<int32_t>(source->GetCoreStats()->GetMP() +
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
        if(isHeal || (target.Flags1 & FLAG1_ABSORB))
        {
            target.Damage1 = target.Damage1 * -1;
            target.Damage2 = target.Damage2 * -1;
            target.Damage1Type = (target.Damage1Type == DAMAGE_TYPE_GENERIC) ?
                DAMAGE_TYPE_HEALING : target.Damage1Type;
            target.Damage2Type = (target.Damage2Type == DAMAGE_TYPE_GENERIC) ?
                DAMAGE_TYPE_HEALING : target.Damage2Type;
        }
    }

    if(skill.IsSuicide)
    {
        auto selfTarget = GetSelfTarget(source, targets);

        selfTarget->Damage1 = source->GetCoreStats()->GetHP();
        selfTarget->Damage1Type = DAMAGE_TYPE_GENERIC;
    }

    return true;
}

int32_t SkillManager::CalculateDamage_Normal(const std::shared_ptr<
    ActiveEntityState>& source, uint16_t mod, uint8_t& damageType,
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
                scale = 1.5f * (float)source->GetCorrectValue(
                    CorrectTbl::LB_DAMAGE) * 0.01f;
                break;
            default:	// Normal hit, 80%-99% damage
                scale = RNG_DEC(float, 0.8f, 0.99f, 2);
                break;
        }

        // Start with offense stat * modifier/100
        float calc = (float)off * ((float)mod * 0.01f);

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
            calc = calc * (1 + resist * -1.f);

            // Multiply by 100% + boost
            calc = calc * (1.f + boost);

            // Multiply by 1 + remaining power boosts/100
            calc = calc * (float)(1 - (0/* @todo: Action+Racial+Skill Power */ * 0.01f));

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
            int32_t maxLB = 30000;

            /// @todo: apply tokusei adjustments

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

    return amount;
}

SkillTargetResult* SkillManager::GetSelfTarget(const std::shared_ptr<ActiveEntityState>& source,
    std::list<SkillTargetResult>& targets)
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
    targets.push_back(target);
    return &targets.back();
}

bool SkillManager::SetNRA(SkillTargetResult& target, ProcessingSkill& skill)
{
    // Calculate affinity checks for physical vs magic and both base and effective
    // values if they differ
    std::list<CorrectTbl> affinities;
    if(skill.EffectiveAffinity != 11)
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

        if(skill.BaseAffinity != skill.EffectiveAffinity)
        {
            affinities.push_back((CorrectTbl)(skill.BaseAffinity + NRA_OFFSET));
        }
    }

    affinities.push_back((CorrectTbl)(skill.EffectiveAffinity + NRA_OFFSET));

    uint8_t resultIdx = 0;
    for(auto nraIdx : target.EntityState->PopNRAShields(affinities))
    {
        if(nraIdx > resultIdx)
        {
            resultIdx = nraIdx;
        }
    }

    if(resultIdx == 0)
    {
        // Check NRA chances
        for(CorrectTbl affinity : affinities)
        {
            for(auto nraIdx : { NRA_ABSORB, NRA_REFLECT, NRA_NULL })
            {
                int16_t chance = target.EntityState->GetNRAChance((uint8_t)nraIdx,
                    affinity);
                if(chance > 0 && RNG(int16_t, 1, 100) <= chance)
                {
                    resultIdx = (uint8_t)nraIdx;
                    break;
                }
            }
        }
    }

    switch(resultIdx)
    {
    case NRA_NULL:
        target.Flags1 |= FLAG1_BLOCK;
        target.HitAvoided = true;
        return false;
    case NRA_ABSORB:
        target.Flags1 |= FLAG1_ABSORB;
        return false;
    case NRA_REFLECT:
        target.Flags1 |= FLAG1_REFLECT;
        target.HitAvoided = true;
        return true;
    default:
        return false;
    }
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
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();

    // No pay the costs
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

    auto discharge = skillData->GetDischarge();
    uint64_t fixPosTime = activated->GetExecutionTime() +
        (uint64_t)(discharge->GetStiffness() * 1000);
    source->SetStatusTimes(STATUS_IMMOBILE, fixPosTime);

    if(source->IsMoving())
    {
        server->GetZoneManager()->FixCurrentPosition(source, fixPosTime,
            activated->GetExecutionTime());
    }

    activated->SetExecuteCount((uint8_t)(activated->GetExecuteCount() + 1));

    SendExecuteSkill(activated, skillData);

    if(client && source->GetEntityType() ==
        objects::EntityStateObject::EntityType_t::CHARACTER)
    {
        characterManager->UpdateExpertise(client, activated->GetSkillID());
    }

    // Update the execution count and remove and complete it from the entity
    // if its at max and not a guard
    if(activated->GetExecuteCount() >= skillData->GetCast()->GetBasic()->GetUseCount())
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

bool SkillManager::SpecialSkill(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    (void)activated;
    (void)ctx;
    (void)client;

    return true;
}

bool SkillManager::EquipItem(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    if(!client)
    {
        return false;
    }

    auto itemID = activated->GetTargetObjectID();
    if(itemID <= 0)
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->EquipItem(client, itemID);

    return ProcessSkillResult(activated, ctx);
}

bool SkillManager::FamiliarityUp(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    if(!client)
    {
        return false;
    }

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
    ctx->ApplyStatusEffects = false;
    return ProcessSkillResult(activated, ctx);
}

bool SkillManager::FamiliarityUpItem(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    if(!client)
    {
        return false;
    }

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

    return ProcessSkillResult(activated, ctx);
}

bool SkillManager::Mooch(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    (void)activated;

    if(!client)
    {
        return false;
    }

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

    if(!client)
    {
        return false;
    }

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

bool SkillManager::StoreDemon(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    (void)ctx;

    if(!client)
    {
        return false;
    }

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

bool SkillManager::Traesto(const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    if(!client)
    {
        return false;
    }

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

    ProcessSkillResult(activated, ctx);

    return mServer.lock()->GetZoneManager()->EnterZone(client, zoneID, xCoord, yCoord, 0, true);
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
        std::unordered_map<uint32_t, uint64_t> timeMap;

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_ACTIVATED);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(activated->GetSkillID());
        p.WriteS8((int8_t)activated->GetActivationID());

        timeMap[11] = activated->GetChargedTime();
        p.WriteFloat(0.f);

        uint8_t useCount = skillData->GetCast()->GetBasic()->GetUseCount();
        p.WriteU8(useCount);
        p.WriteU8(0);   //Unknown

        float chargeSpeed = 0.f, chargeCompleteSpeed = 0.f;

        // Send movement speed based off skill action type
        switch(skillData->GetBasic()->GetActionType())
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

        p.WriteFloat(chargeSpeed);
        p.WriteFloat(chargeCompleteSpeed);

        ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
    }
}

void SkillManager::SendExecuteSkill(std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<objects::MiSkillData> skillData)
{
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(activated->GetSourceEntity());
    auto zone = source ? source->GetZone() : nullptr;
    auto zConnections = zone
        ? zone->GetConnectionList() : std::list<std::shared_ptr<ChannelClientConnection>>();
    if(zConnections.size() > 0)
    {
        auto conditionData = skillData->GetCondition();
        auto dischargeData = skillData->GetDischarge();

        int32_t targetedEntityID = activated->GetEntityTargeted()
            ? (int32_t)activated->GetTargetObjectID()
            : source->GetEntityID();

        uint32_t cdTime = conditionData->GetCooldownTime();
        uint32_t stiffness = dischargeData->GetStiffness();
        uint64_t currentTime = activated->GetExecutionTime();

        bool moreUses = activated->GetExecuteCount() <
            skillData->GetCast()->GetBasic()->GetUseCount();
        uint64_t cooldownTime = cdTime && !moreUses
            ? (currentTime + (uint64_t)(cdTime * 1000)) : 0;
        uint64_t lockOutTime = currentTime + (uint64_t)(stiffness * 1000);

        std::unordered_map<uint32_t, uint64_t> timeMap;

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_EXECUTED);
        p.WriteS32Little(source->GetEntityID());
        p.WriteU32Little(activated->GetSkillID());
        p.WriteS8((int8_t)activated->GetActivationID());
        p.WriteS32Little(targetedEntityID);

        timeMap[15] = cooldownTime;
        p.WriteFloat(0.f);
        timeMap[19] = lockOutTime;
        p.WriteFloat(0.f);

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

        ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
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
        p.WriteFloat(0);   //Unknown
        p.WriteU8(1);   //Unknown
        p.WriteFloat(source->GetMovementSpeed());
        p.WriteU8(mode);

        ChannelClientConnection::BroadcastPacket(zConnections, p);
    }
}
