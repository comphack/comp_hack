/**
 * @file server/channel/src/AIManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Class to manage all server side AI related actions.
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

#include "AIManager.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <ErrorCodes.h>
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ScriptEngine.h>
#include <ServerDataManager.h>

// object Includes
#include <ActivatedAbility.h>
#include <Ally.h>
#include <MiAIData.h>
#include <MiAIRelationData.h>
#include <MiBattleDamageData.h>
#include <MiCastBasicData.h>
#include <MiCastData.h>
#include <MiCategoryData.h>
#include <MiConditionData.h>
#include <MiCostTbl.h>
#include <MiDamageData.h>
#include <MiDevilData.h>
#include <MiEffectiveRangeData.h>
#include <MiFindInfo.h>
#include <MiSkillBasicData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiTargetData.h>
#include <Spawn.h>
#include <SpawnLocation.h>

// channel Includes
#include "AICommand.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "SkillManager.h"
#include "ZoneManager.h"

using namespace channel;

std::unordered_map<std::string,
    std::shared_ptr<libcomp::ScriptEngine>> AIManager::sPreparedScripts;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<AIManager>()
    {
        if(!BindingExists("AIManager", true))
        {
            Using<ActiveEntityState>();

            Sqrat::Class<AIManager,
                Sqrat::NoConstructor<AIManager>> binding(mVM, "AIManager");
            binding
                .Func("Move", &AIManager::Move)
                .Func("QueueScriptCommand", &AIManager::QueueScriptCommand)
                .Func("QueueWaitCommand", &AIManager::QueueWaitCommand);

            Bind<AIManager>("AIManager", binding);
        }

        return *this;
    }
}

AIManager::AIManager()
{
}

AIManager::AIManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

AIManager::~AIManager()
{
}

bool AIManager::Prepare(const std::shared_ptr<ActiveEntityState>& eState,
    const libcomp::String& aiType)
{
    auto aiState = std::make_shared<AIState>();
    eState->SetAIState(aiState);

    auto eBase = eState->GetEnemyBase();
    if(eBase && (eBase->GetSpawnLocation() || eBase->GetSpawnSpotID()))
    {
        // Default to wandering first
        aiState->SetStatus(AIStatus_t::WANDERING, true);
    }

    auto spawn = eBase ? eBase->GetSpawnSource() : nullptr;
    uint16_t baseAIType = spawn ? spawn->GetBaseAIType() : 0;

    auto demonData = eState->GetDevilData();
    auto aiData = demonData ? mServer.lock()->GetDefinitionManager()->GetAIData(
        baseAIType ? baseAIType : demonData->GetAI()->GetType()) : nullptr;
    if(!aiData)
    {
        LOG_ERROR("Active entity with invalid base AI data value specified\n");
        return false;
    }

    // Set all default values now so any call to the script prepare function
    // can modify them
    aiState->SetBaseAI(aiData);
    aiState->SetAggroLevelLimit(aiData->GetAggroLevelLimit());
    aiState->SetThinkSpeed(aiData->GetThinkSpeed());

    if(spawn)
    {
        aiState->SetAggression(spawn->GetAggression());
    }

    std::shared_ptr<libcomp::ScriptEngine> aiEngine;
    if(!aiType.IsEmpty())
    {
        auto it = sPreparedScripts.find(aiType.C());
        if(it == sPreparedScripts.end())
        {
            auto script = mServer.lock()->GetServerDataManager()
                ->GetAIScript(aiType);
            if(!script)
            {
                LOG_ERROR(libcomp::String("AI type '%1' does not exist\n")
                    .Arg(aiType));
                return false;
            }

            aiEngine = std::make_shared<libcomp::ScriptEngine>();
            aiEngine->Using<AIManager>();

            if(!aiEngine->Eval(script->Source))
            {
                LOG_ERROR(libcomp::String("AI type '%1' is not a valid AI script\n")
                    .Arg(aiType));
                return false;
            }

            sPreparedScripts[aiType.C()] = aiEngine;
        }
        else
        {
            aiEngine = it->second;
        }

        Sqrat::Function f(Sqrat::RootTable(aiEngine->GetVM()), "prepare");
        if(!f.IsNull())
        {
            auto result = !f.IsNull() ? f.Evaluate<int>(eState, this) : 0;
            if(!result || (*result != 0))
            {
                LOG_ERROR(libcomp::String("Failed to prepare AI type '%1'\n")
                    .Arg(aiType));
                return false;
            }
        }
    }

    aiState->SetScript(aiEngine);

    // The first command all AI perform is a wait command for a set time
    QueueWaitCommand(aiState, 3000);

    return true;
}

void AIManager::UpdateActiveStates(const std::shared_ptr<Zone>& zone,
    uint64_t now, bool isNight)
{
    std::list<std::shared_ptr<ActiveEntityState>> updated;
    for(auto enemy : zone->GetEnemies())
    {
        if(UpdateState(enemy, now, isNight))
        {
            updated.push_back(enemy);
        }
    }

    for(auto ally : zone->GetAllies())
    {
        if(UpdateState(ally, now, isNight))
        {
            updated.push_back(ally);
        }
    }

    // Update enemy states first
    if(updated.size() > 0)
    {
        auto zConnections = zone->GetConnectionList();
        RelativeTimeMap timeMap;
        for(auto entity : updated)
        {
            // Update the clients with what the entity is doing

            // Check if the entity's position or rotation has updated
            if(now == entity->GetOriginTicks())
            {
                if(entity->IsMoving())
                {
                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MOVE);
                    p.WriteS32Little(entity->GetEntityID());
                    p.WriteFloat(entity->GetDestinationX());
                    p.WriteFloat(entity->GetDestinationY());
                    p.WriteFloat(entity->GetOriginX());
                    p.WriteFloat(entity->GetOriginY());
                    p.WriteFloat(entity->GetMovementSpeed());

                    timeMap[p.Size()] = now;
                    timeMap[p.Size() + 4] = entity->GetDestinationTicks();
                    ChannelClientConnection::SendRelativeTimePacket(zConnections, p,
                        timeMap, true);
                }
                else if(entity->IsRotating())
                {
                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ROTATE);
                    p.WriteS32Little(entity->GetEntityID());
                    p.WriteFloat(entity->GetDestinationRotation());

                    timeMap[p.Size()] = now;
                    timeMap[p.Size() + 4] = entity->GetDestinationTicks();
                    ChannelClientConnection::SendRelativeTimePacket(zConnections, p,
                        timeMap, true);
                }
                else
                {
                    // The movement was actually a stop

                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_STOP_MOVEMENT);
                    p.WriteS32Little(entity->GetEntityID());
                    p.WriteFloat(entity->GetDestinationX());
                    p.WriteFloat(entity->GetDestinationY());

                    timeMap[p.Size()] = entity->GetDestinationTicks();
                    ChannelClientConnection::SendRelativeTimePacket(zConnections, p,
                        timeMap, true);
                }
            }
        }

        ChannelClientConnection::FlushAllOutgoing(zConnections);
    }
}

void AIManager::CombatSkillHit(
    const std::list<std::shared_ptr<ActiveEntityState>>& entities,
    const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<objects::MiSkillData>& skillData)
{
    (void)skillData;

    for(auto eState : entities)
    {
        auto aiState = eState->GetAIState();
        if(!aiState) continue;

        // If the current command is a skill command and it was cancelled
        // by the hit, remove it now so they can react faster later
        auto skillCmd = std::dynamic_pointer_cast<AIUseSkillCommand>(aiState
            ->GetCurrentCommand());
        auto activated = skillCmd ? skillCmd->GetActivatedAbility() : nullptr;
        if(activated && activated->GetCancelled())
        {
            aiState->PopCommand();
        }

        /// @todo: add override
        if(!eState->SameFaction(source))
        {
            // If the entity's current target is not the source of this skill,
            // there is a chance they will target them now (20% chance by
            // default)
            if(aiState->GetTargetEntityID() != source->GetEntityID() &&
                RNG(int32_t, 1, 10) <= 2)
            {
                aiState->SetTargetEntityID(source->GetEntityID());
            }

            // If the entity is not active, clear all pending commands
            // and let them figure out if they need to resume later
            if(aiState->GetStatus() == AIStatus_t::IDLE &&
                aiState->GetStatus() == AIStatus_t::WANDERING)
            {
                aiState->ClearCommands();
            }
        }
    }
}

void AIManager::CombatSkillComplete(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<objects::MiSkillData>& skillData,
    const std::shared_ptr<ActiveEntityState>& target, bool hit)
{
    auto aiState = eState->GetAIState();
    if(!aiState)
    {
        return;
    }

    uint32_t skillID = skillData->GetCommon()->GetID();

    // Multiple triggers in combat cause normal AI to reset and reorient
    // itself so they're not spamming skills non-stop
    bool reset = false;
    if(target)
    {
        if(target->GetStatusTimes(STATUS_KNOCKBACK))
        {
            // If the target is currently being knocked back (from this
            // skill or some other one), reset
            reset = true;
        }
        else if(eState->GetStatusTimes(STATUS_HIT_STUN))
        {
            // If the source is hitstunned for whatever reason (counter
            // or guard for example), reset
            reset = true;
        }
        else if(!skillData->GetTarget()->GetRange() &&
            !skillData->GetCast()->GetBasic()->GetChargeTime() &&
            !skillData->GetCondition()->GetCooldownTime())
        {
            // No charge, no cooldown, no range combat skills are typically
            // used in succession until knockback occurs (delayed by lockout
            // animation time)
            bool combo = false;

            if(target->GetStatusTimes(STATUS_HIT_STUN))
            {
                // If the target is hitstunned, always use again to attempt
                // to combo into knockback
                combo = true;
            }
            else
            {
                // If the target was still hit, repeat attack 30% of the
                // time, 10% if they were not hit
                combo = (hit && RNG(int32_t, 1, 10) <= 3) ||
                    (!hit && RNG(int32_t, 1, 10) == 1);
            }

            if(combo && !aiState->GetCurrentCommand())
            {
                auto cmd = std::make_shared<AIUseSkillCommand>(skillID,
                    target->GetEntityID());
                aiState->QueueCommand(cmd);
            }
            else
            {
                reset = true;
            }
        }
        else if(activated->GetExecuteCount() >= activated->GetMaxUseCount())
        {
            // Other skills should be staggered by thinkspeed unless
            // more executions exist
            reset = true;
        }
    }

    if(reset)
    {
        aiState->ClearCommands();
        QueueWaitCommand(aiState, (uint32_t)aiState->GetThinkSpeed());
    }
}

void AIManager::QueueScriptCommand(const std::shared_ptr<AIState>& aiState,
    const libcomp::String& functionName)
{
    auto cmd = std::make_shared<AIScriptedCommand>(functionName);
    aiState->QueueCommand(cmd);
}

void AIManager::QueueWaitCommand(const std::shared_ptr<AIState>& aiState,
    uint32_t waitTime)
{
    auto cmd = GetWaitCommand(waitTime);
    aiState->QueueCommand(cmd);
}

void AIManager::UpdateAggro(const std::shared_ptr<ActiveEntityState>& eState,
    int32_t targetID)
{
    auto aiState = eState->GetAIState();
    if(aiState)
    {
        if(targetID > 0 &&
            (aiState->GetStatus() == AIStatus_t::IDLE ||
             aiState->GetStatus() == AIStatus_t::WANDERING))
        {
            aiState->SetStatus(AIStatus_t::AGGRO);
        }

        aiState->SetTargetEntityID(targetID);
    }
}

void AIManager::Move(const std::shared_ptr<ActiveEntityState>& eState,
    float xPos, float yPos, uint64_t now)
{
    if(!eState->CanMove())
    {
        return;
    }

    /// @todo: check collision and correct
    eState->Move(xPos, yPos, now);
}

bool AIManager::UpdateState(const std::shared_ptr<ActiveEntityState>& eState,
    uint64_t now, bool isNight)
{
    eState->RefreshCurrentPosition(now);

    auto aiState = eState->GetAIState();
    if(!aiState || (aiState->IsIdle() &&
        !aiState->ActionOverridesKeyExists("idle")))
    {
        return false;
    }

    eState->ExpireStatusTimes(now);

    // If the entity cannot act or is waiting, stop if moving and quit here
    if(!eState->CanAct() || eState->GetStatusTimes(STATUS_WAITING))
    {
        if(eState->IsMoving() && !eState->GetStatusTimes(STATUS_KNOCKBACK))
        {
            eState->Stop(now);
            return true;
        }

        return false;
    }

    // Entity cannot do anything if still affected by skill lockout
    if(eState->GetStatusTimes(STATUS_LOCKOUT))
    {
        return false;
    }

    if(aiState->StatusChanged())
    {
        // Do not clear actions if going from aggro to combat
        if(!(aiState->GetStatus() == AIStatus_t::COMBAT &&
            aiState->GetPreviousStatus() == AIStatus_t::AGGRO))
        {
            auto cmd = aiState->GetCurrentCommand();
            aiState->ClearCommands();

            // If the current command was a use skill, let it complete and
            // fail if it needs to
            if(cmd && cmd->GetType() == AICommandType_t::USE_SKILL)
            {
                aiState->QueueCommand(cmd);
            }
        }

        aiState->ResetStatusChanged();
    }

    if(!aiState->GetCurrentCommand())
    {
        // Check for overrides first
        libcomp::String actionName;
        switch(aiState->GetStatus())
        {
        case AIStatus_t::IDLE:
            actionName = "idle";
            break;
        case AIStatus_t::WANDERING:
            actionName = "wander";
            break;
        case AIStatus_t::AGGRO:
            actionName = "aggro";
            break;
        case AIStatus_t::COMBAT:
            actionName = "combat";
            break;
        default:
            break;
        }

        if(aiState->ActionOverridesKeyExists(actionName))
        {
            libcomp::String functionOverride = aiState->GetActionOverrides(actionName);
            if(!functionOverride.IsEmpty())
            {
                // Queue the overridden function
                QueueScriptCommand(aiState, functionOverride);
            }
            else
            {
                // Run the function with the action name
                int32_t result = 0;
                if(ExecuteScriptFunction(eState, actionName.C(), now, result))
                {
                    if(result == -1)
                    {
                        // Erroring or skipping the action
                        return false;
                    }
                    else if(result == 1)
                    {
                        // Direct entity update, communicate the results
                        return true;
                    }
                }
            }
        }

        // If no commands were added by the script, use the normal logic
        if(!aiState->GetCurrentCommand())
        {
            switch(eState->GetEntityType())
            {
            case EntityType_t::ENEMY:
            case EntityType_t::ALLY:
                return UpdateEnemyState(eState, eState->GetEnemyBase(),
                    now, isNight);
            default:
                break;
            }
        }
    }

    auto cmd = aiState->GetCurrentCommand();
    if(cmd)
    {
        if(!cmd->GetStartTime())
        {
            cmd->Start();
            if(cmd->GetDelay() > 0)
            {
                eState->SetStatusTimes(STATUS_WAITING, now + cmd->GetDelay());

                return false;
            }
        }

        switch(cmd->GetType())
        {
        case AICommandType_t::MOVE:
            if(eState->CanMove())
            {
                if(eState->IsMoving())
                {
                    return false;
                }
                else
                {
                    auto cmdMove = std::dynamic_pointer_cast<AIMoveCommand>(cmd);

                    // Move to the first point in the path that is not the entity's
                    // current position
                    Point dest;
                    while(cmdMove->GetCurrentDestination(dest))
                    {
                        if(dest.x != eState->GetCurrentX() ||
                            dest.y != eState->GetCurrentY())
                        {
                            Move(eState, dest.x, dest.y, now);
                            return true;
                        }
                        else if(!cmdMove->SetNextDestination())
                        {
                            break;
                        }
                    }
                }
                aiState->PopCommand();
            }
            else
            {
                // If the entity can't move, clear the queued commands and let
                // it figure out what to do instead
                aiState->ClearCommands();
            }
            break;
        case AICommandType_t::USE_SKILL:
            {
                // Do nothing if hit stunned or still charging
                if(eState->GetStatusTimes(STATUS_HIT_STUN) ||
                    eState->GetStatusTimes(STATUS_KNOCKBACK) ||
                    eState->GetStatusTimes(STATUS_CHARGING))
                {
                    return false;
                }

                auto cmdSkill = std::dynamic_pointer_cast<AIUseSkillCommand>(cmd);

                auto activated = cmdSkill->GetActivatedAbility();
                if(activated && eState->GetActivatedAbility() == activated &&
                   activated->GetErrorCode() == -1)
                {
                    // Check the state of the current activated skill
                    if(activated->GetExecutionRequestTime() &&
                        !activated->GetExecutionTime())
                    {
                        // Waiting on skill to start
                        return false;
                    }

                    if(activated->GetHitTime() &&
                        activated->GetHitTime() < now)
                    {
                        // Waiting on skill hit
                        return false;
                    }
                }

                auto server = mServer.lock();
                auto skillManager = server->GetSkillManager();

                bool valid = true;

                if(cmdSkill->GetTargetEntityID() > 0)
                {
                    auto targetEntity = eState->GetZone()->GetActiveEntity(
                        cmdSkill->GetTargetEntityID());
                    if(!targetEntity || !targetEntity->IsAlive() ||
                        targetEntity->GetAIIgnored())
                    {
                        // Target invalid or dead, cancel the skill and move on
                        if(activated)
                        {
                            skillManager->CancelSkill(eState, activated->GetActivationID());
                        }
                        valid = false;
                    }
                }

                if(valid)
                {
                    if(activated)
                    {
                        // Execute the skill
                        if(!skillManager->ExecuteSkill(eState, activated->GetActivationID(),
                            activated->GetTargetObjectID()) &&
                            eState->GetActivatedAbility() == activated)
                        {
                            if(activated->GetErrorCode() != (int8_t)SkillErrorCodes_t::ACTION_RETRY)
                            {
                                skillManager->CancelSkill(eState, activated->GetActivationID());
                            }
                        }
                    }
                    else
                    {
                        // Activate the skill
                        skillManager->ActivateSkill(eState, cmdSkill->GetSkillID(),
                            cmdSkill->GetTargetEntityID(), cmdSkill->GetTargetEntityID());
                    }
                }

                aiState->PopCommand();
            }
            break;
        case AICommandType_t::SCRIPTED:
            {
                // Execute a custom scripted command
                auto cmdScript = std::dynamic_pointer_cast<AIScriptedCommand>(cmd);

                int32_t result = 0;
                if(!ExecuteScriptFunction(eState, cmdScript->GetFunctionName(), now,
                    result))
                {
                    // Pop the command and move on
                    aiState->PopCommand();
                }
                else if(result == 0)
                {
                    return false;
                }
                else
                {
                    aiState->PopCommand();
                    if(result == 1)
                    {
                        return true;
                    }
                }
            }
            break;
        case AICommandType_t::NONE:
        default:
            aiState->PopCommand();
            break;
        }
    }

    return false;
}

bool AIManager::UpdateEnemyState(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::EnemyBase>& eBase, uint64_t now,
    bool isNight)
{
    auto aiState = eState->GetAIState();
    if(aiState->GetTargetEntityID() <= 0 &&
        eState->GetOpponentIDs().size() == 0)
    {
        auto newTarget = Retarget(eState, now, isNight);
        if(newTarget)
        {
            // First target always results in a 3s wait
            QueueWaitCommand(aiState, 3000);
            return false;
        }
    }

    if(aiState->GetStatus() == AIStatus_t::WANDERING && eBase)
    {
        Wander(eState, eBase);
        return true;
    }

    // If not wandering, handle aggro and combat somewhat similarly
    bool inCombat = aiState->GetStatus() == AIStatus_t::COMBAT;

    auto zone = eState->GetZone();
    int32_t targetEntityID = aiState->GetTargetEntityID();
    auto target = targetEntityID > 0
        ? zone->GetActiveEntity(targetEntityID) : nullptr;
    if(!target || !target->IsAlive() || !target->Ready() ||
        target->GetAIIgnored())
    {
        if(inCombat)
        {
            // Try to find another opponent
            target = Retarget(eState, now, isNight);
        }
        else
        {
            // Reset to default state and quit
            aiState->SetStatus(aiState->GetDefaultStatus());
            return false;
        }
    }
            
    float targetDist = 0.f, targetX = 0.f, targetY = 0.f;
    if(target)
    {
        target->RefreshCurrentPosition(now);
        targetX = target->GetCurrentX();
        targetY = target->GetCurrentY();
        targetDist = eState->GetDistance(targetX, targetY);
    }

    auto server = mServer.lock();

    // If the target is 1.5x the aggro distance, de-aggro
    bool targetChanged = false;
    if(targetDist >= aiState->GetAggroValue(isNight ? 1 : 0, false, 2000.f) * 1.5f)
    {
        // De-aggro on that one target and find a new one
        server->GetCharacterManager()->AddRemoveOpponent(false,
            eState, target);

        target = Retarget(eState, now, isNight);
        targetChanged = true;
    }

    auto activated = eState->GetActivatedAbility();
    if(!target)
    {
        // No target could be found, stop combat and quit
        if(activated)
        {
            server->GetSkillManager()->CancelSkill(eState,
                activated->GetActivationID());
        }

        server->GetCharacterManager()->AddRemoveOpponent(false,
            eState, nullptr);

        return false;
    }
    else if(targetChanged)
    {
        target->RefreshCurrentPosition(now);
        targetX = target->GetCurrentX();
        targetY = target->GetCurrentY();
        targetDist = eState->GetDistance(targetX, targetY);
    }

    targetEntityID = target->GetEntityID();

    if(activated && activated->GetErrorCode() >= 0)
    {
        // Somehow we have an error, cancel and choose something else
        server->GetSkillManager()->CancelSkill(eState, activated
            ->GetActivationID());
        activated = nullptr;
    }

    if(activated)
    {
        auto skillManager = server->GetSkillManager();

        // Skill charged, cancel, execute or move within range
        int64_t activationTarget = activated->GetTargetObjectID();
        if(activationTarget && targetEntityID != (int32_t)activationTarget)
        {
            // Target changed
            skillManager->TargetSkill(eState, targetEntityID);
            return false;
        }

        auto definitionManager = server->GetDefinitionManager();
        auto skillData = definitionManager->GetSkillData(activated
            ->GetSkillID());
        uint16_t maxTargetRange = (uint16_t)(400 +
            (skillData->GetTarget()->GetRange() * 10));

        if(targetDist > (maxTargetRange + 20.f))
        {
            // Move within range (keep a bit of a buffer for movement)
            auto zoneManager = server->GetZoneManager();
            auto point = zoneManager->GetLinearPoint(eState->GetCurrentX(),
                eState->GetCurrentY(), targetX, targetY,
                (float)(targetDist - (float)maxTargetRange + 10.f), false);

            auto cmd = GetMoveCommand(eState, point);
            if(cmd)
            {
                aiState->QueueCommand(cmd);
            }
            else
            {
                skillManager->CancelSkill(eState, activated->GetActivationID());
            }
        }
        else if(activated->GetExecutionRequestTime() == 0)
        {
            // Execute the skill
            auto cmd = std::make_shared<AIUseSkillCommand>(activated);
            aiState->QueueCommand(cmd);
        }
    }
    else
    {
        int16_t rCommand = RNG(int16_t, 1, 10);

        if(rCommand == 1)
        {
            // 10% chance to just wait
            QueueWaitCommand(aiState, (uint32_t)aiState->GetThinkSpeed());
        }
        else
        {
            if(targetDist > 400.f)
            {
                // Run up to the target but don't do anything yet
                auto zoneManager = server->GetZoneManager();
                auto point = zoneManager->GetLinearPoint(eState->GetCurrentX(),
                    eState->GetCurrentY(), targetX, targetY, targetDist, false);

                auto cmd = GetMoveCommand(eState, point, 200.f);
                if(cmd)
                {
                    aiState->QueueCommand(cmd);
                }
                else
                {
                    // If the enemy can't move to the target, retarget and quit
                    Retarget(eState, now, isNight);
                }
            }
            else if(eState->CurrentSkillsCount() > 0)
            {
                PrepareSkillUsage(eState);
            }
        }
    }

    return false;
}

void AIManager::Wander(const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::EnemyBase>& eBase)
{
    auto aiState = eState->GetAIState();

    auto spawnLocation = eBase->GetSpawnLocation();
    uint32_t spotID = eBase->GetSpawnSpotID();

    int32_t thinkSpeed = aiState->GetThinkSpeed();
    if((spawnLocation || spotID > 0) && eState->CanMove())
    {
        auto zone = eState->GetZone();

        Point dest;
        auto zoneManager = mServer.lock()->GetZoneManager();
        if(spawnLocation)
        {
            auto point = zoneManager->GetRandomPoint(spawnLocation
                ->GetWidth(), spawnLocation->GetHeight());

            // Spawn location group bounding box points start in the top
            // left corner of the rectangle and extend towards +X/-Y
            dest.x = (float)(spawnLocation->GetX() + point.x);
            dest.y = (float)(spawnLocation->GetY() - point.y);
        }
        else
        {
            dest = zoneManager->GetRandomSpotPoint(zone
                ->GetDynamicMap()->Spots[spotID]->Definition);
        }

        // Use the destination as a direction to head and either
        // limit/extend to think speed distance (minimum 500ms)
        float moveDistance = (float)((float)eState->GetMovementSpeed() *
            (float)(thinkSpeed < 500 ? 500 : thinkSpeed) * 0.001f);

        Point source(eState->GetCurrentX(), eState->GetCurrentY());
        dest = zoneManager->GetLinearPoint(source.x, source.y,
            dest.x, dest.y, moveDistance, false, zone);

        auto command = GetMoveCommand(eState, dest, 0.f, false);
        if(command)
        {
            aiState->QueueCommand(command);
        }
    }

    QueueWaitCommand(aiState, (uint32_t)(thinkSpeed * RNG(int32_t, 1, 3)));
}

std::shared_ptr<ActiveEntityState> AIManager::Retarget(
    const std::shared_ptr<ActiveEntityState>& eState,
    uint64_t now, bool isNight)
{
    std::shared_ptr<ActiveEntityState> target;

    auto aiState = eState->GetAIState();

    int32_t currentTarget = aiState->GetTargetEntityID();
    aiState->SetTargetEntityID(-1);

    auto zone = eState->GetZone();
    if(!zone)
    {
        return nullptr;
    }

    float sourceX = eState->GetCurrentX();
    float sourceY = eState->GetCurrentY();

    auto opponentIDs = eState->GetOpponentIDs();
    std::list<std::shared_ptr<ActiveEntityState>> possibleTargets;
    if(opponentIDs.size() > 0)
    {
        float aggroNormal = aiState->GetAggroValue(isNight ? 1 : 0, false, 2000.f);
        float aggroCast = aiState->GetAggroValue(2, false, 2000.f);
        float aggroMax = aggroNormal > aggroCast ? aggroNormal : aggroCast;

        // Currently in combat, only pull from opponents
        auto inRange = zone->GetActiveEntitiesInRadius(
            sourceX, sourceY, aggroMax);

        for(auto entity : inRange)
        {
            if(opponentIDs.find(entity->GetEntityID()) != opponentIDs.end()
                && entity->IsAlive() && entity->Ready() &&
                !entity->GetAIIgnored())
            {
                possibleTargets.push_back(entity);
            }
        }
    }
    else
    {
        // Not in combat, find a target to pursue

        // If the entity has a low aggression level, check if targetting should occur
        uint8_t aggression = aiState->GetAggression();
        if(aggression < 100 && RNG(int32_t, 1, 100) > aggression)
        {
            return nullptr;
        }

        int32_t aggroLevelLimit = eState->GetLevel() +
            aiState->GetAggroLevelLimit();

        // Get aggro values, default to 2000 units and 80 degree FoV angle (in radians)
        std::pair<float, float> aggroNormal(aiState->GetAggroValue(isNight ? 1 : 0, false,
            2000.f), aiState->GetAggroValue(isNight ? 1 : 0, true, 1.395f));
        std::pair<float, float> aggroCast(aiState->GetAggroValue(2, false, 2000.f),
            aiState->GetAggroValue(2, true, 1.395f));

        // Get all active entities in range and FoV (cast aggro first,
        // leaving in doubles for higher chances when closer)
        bool castingOnly = true;

        std::list<std::shared_ptr<ActiveEntityState>> inFoV;
        for(auto aggro : { aggroCast, aggroNormal })
        {
            auto filtered = zone->GetActiveEntitiesInRadius(sourceX,
                sourceY, (double)aggro.first);

            // Remove allies, entities not ready yet or in an invalid state
            filtered.remove_if([eState, castingOnly](
                const std::shared_ptr<ActiveEntityState>& entity)
                {
                    return eState->SameFaction(entity) ||
                        (castingOnly && !entity->GetStatusTimes(STATUS_CHARGING)) ||
                        !entity->Ready() || entity->GetAIIgnored();
                });

            // If the aggro level limit could potentially exclude a target
            // filter them out now
            if(aggroLevelLimit < 99)
            {
                filtered.remove_if([aggroLevelLimit](
                    const std::shared_ptr<ActiveEntityState>& entity)
                    {
                        return entity->GetLevel() > aggroLevelLimit;
                    });
            }

            if(filtered.size() > 0)
            {
                // Targets found, check if they're visible
                for(auto entity : filtered)
                {
                    entity->RefreshCurrentPosition(now);
                }

                // Filter the set down to only entities in the FoV
                for(auto fovEntity : ZoneManager::GetEntitiesInFoV(filtered, sourceX,
                    sourceY, eState->GetCurrentRotation(), aggro.second))
                {
                    inFoV.push_back(fovEntity);
                }
            }

            castingOnly = false;
        }

        if(inFoV.size() > 0)
        {
            auto geometry = zone->GetGeometry();
            for(auto entity : inFoV)
            {
                // Possible target found, check line of sight
                bool add = true;
                if(geometry)
                {
                    Line path(Point(sourceX, sourceY),
                        Point(entity->GetCurrentX(), entity->GetCurrentY()));

                    Point collidePoint;
                    add = !zone->Collides(path, collidePoint);
                }

                if(add)
                {
                    possibleTargets.push_back(entity);

                    if(aiState->GetStatus() == AIStatus_t::WANDERING)
                    {
                        aiState->SetStatus(AIStatus_t::AGGRO);
                    }
                }
            }
        }
    }

    if(possibleTargets.size() > 0)
    {
        if(aiState->ActionOverridesKeyExists("target") && aiState->GetScript())
        {
            Sqrat::Function f(Sqrat::RootTable(aiState->GetScript()->GetVM()),
                aiState->GetActionOverrides("target").C());

            auto scriptResult = !f.IsNull()
                ? f.Evaluate<int32_t>(eState, possibleTargets, this, now) : 0;
            if(scriptResult)
            {
                target = zone->GetActiveEntity(*scriptResult);
            }
        }
        else
        {
            /// @todo: add better default target selection logic
            target = libcomp::Randomizer::GetEntry(possibleTargets);
        }

        aiState->SetTargetEntityID(target ? target->GetEntityID() : -1);
    }

    if(eState->GetEnemyBase() && aiState->GetTargetEntityID() != currentTarget)
    {
        // Enemies and allies telegraph who they are targeting by facing them
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENEMY_ACTIVATED);
        p.WriteS32Little(eState->GetEntityID());
        p.WriteS32Little(aiState->GetTargetEntityID());

        ChannelClientConnection::BroadcastPacket(zone->GetConnectionList(), p);
    }

    return target;
}

void AIManager::RefreshSkillMap(const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<AIState>& aiState)
{
    if(!aiState->GetSkillsMapped())
    {
        auto cs = eState->GetCoreStats();
        auto isEnemy = eState->GetEntityType() == EntityType_t::ENEMY;

        std::unordered_map<uint16_t,
            std::list<std::shared_ptr<objects::MiSkillData>>> skillMap;

        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();

        auto currentSkills = eState->GetCurrentSkills();
        for(uint32_t skillID : currentSkills)
        {
            auto skillData = definitionManager->GetSkillData(skillID);

            // Active skills only
            auto category = skillData->GetCommon()->GetCategory();
            if(category->GetMainCategory() != 1) continue;

            auto range = skillData->GetRange();

            int16_t targetType = -1;
            switch(range->GetValidType())
            {
            case objects::MiEffectiveRangeData::ValidType_t::ALLY:
                targetType = (int16_t)AI_SKILL_TYPES_ALLY;
                break;
            case objects::MiEffectiveRangeData::ValidType_t::SOURCE:
                targetType = (int16_t)AI_SKILL_TYPE_DEF;
                break;
            case objects::MiEffectiveRangeData::ValidType_t::ENEMY:
                targetType = (int16_t)AI_SKILL_TYPES_ENEMY;
                break;
            case objects::MiEffectiveRangeData::ValidType_t::PARTY:
            case objects::MiEffectiveRangeData::ValidType_t::DEAD_ALLY:
            case objects::MiEffectiveRangeData::ValidType_t::DEAD_PARTY:
                if(!isEnemy)
                {
                    // Skills that affect parties or dead entities are not
                    // usable by enemies
                    targetType = (int16_t)AI_SKILL_TYPES_ALLY;
                }
                break;
            default:
                break;
            }

            if(targetType != -1)
            {
                auto damage = skillData->GetDamage()->GetBattleDamage();
                switch(damage->GetFormula())
                {
                case objects::MiBattleDamageData::Formula_t::DMG_NORMAL:
                case objects::MiBattleDamageData::Formula_t::DMG_PERCENT:
                case objects::MiBattleDamageData::Formula_t::DMG_SOURCE_PERCENT:
                case objects::MiBattleDamageData::Formula_t::DMG_MAX_PERCENT:
                case objects::MiBattleDamageData::Formula_t::DMG_STATIC:
                    // Do not add skills that damage allies by default
                    if(targetType == (int16_t)AI_SKILL_TYPES_ENEMY)
                    {
                        auto target = skillData->GetTarget();
                        if(target->GetRange() > 0)
                        {
                            skillMap[AI_SKILL_TYPE_CLSR].push_back(skillData);
                        }
                        else
                        {
                            skillMap[AI_SKILL_TYPE_LNGR].push_back(skillData);
                        }
                    }
                    break;
                case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
                case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT:
                case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
                    {
                        skillMap[AI_SKILL_TYPE_HEAL].push_back(skillData);
                    }
                    break;
                default:
                    /// @todo: determine the difference between self buff and defense
                    if(targetType == (int16_t)AI_SKILL_TYPE_DEF)
                    {
                        skillMap[AI_SKILL_TYPE_DEF].push_back(skillData);
                    }
                    else
                    {
                        skillMap[AI_SKILL_TYPE_SUPPORT].push_back(skillData);
                    }
                    break;
                }
            }
        }

        aiState->SetSkillMap(skillMap);
    }
}

bool AIManager::PrepareSkillUsage(
    const std::shared_ptr<ActiveEntityState>& eState)
{
    auto aiState = eState->GetAIState();
    auto cs = eState->GetCoreStats();

    RefreshSkillMap(eState, aiState);

    int32_t targetID = aiState->GetTargetEntityID();

    auto skillMap = aiState->GetSkillMap();
    if(skillMap.size() > 0 && targetID > 0)
    {
        auto skillManager = mServer.lock()->GetSkillManager();

        /// @todo: add skill selection weights and non-combat skills
        uint32_t skillID = 0;

        uint16_t priorityType = RNG(uint16_t, AI_SKILL_TYPE_CLSR,
            AI_SKILL_TYPE_LNGR);
        for(uint16_t skillType : { priorityType,
            priorityType == AI_SKILL_TYPE_LNGR ? AI_SKILL_TYPE_CLSR : AI_SKILL_TYPE_LNGR })
        {
            std::set<std::shared_ptr<objects::MiSkillData>> skillSet;
            for(auto skillData : skillMap[skillType])
            {
                // Make sure its not cooling down or restricted
                if(!eState->SkillCooldownsKeyExists(skillData->GetBasic()
                    ->GetCooldownID()) &&
                    !skillManager->SkillRestricted(eState, skillData))
                {
                    skillSet.insert(skillData);
                }
            }

            while(!skillID && skillSet.size() > 0)
            {
                auto skillData = libcomp::Randomizer::GetEntry(skillSet);
                skillSet.erase(skillData);

                // Make sure costs can be paid
                int32_t hpCost = 0, mpCost = 0;
                uint16_t bulletCost = 0;
                std::unordered_map<uint32_t, uint32_t> itemCosts;
                if(!skillManager->DetermineNormalCosts(eState, skillData,
                    hpCost, mpCost, bulletCost, itemCosts) || bulletCost ||
                    itemCosts.size() > 0)
                {
                    continue;
                }

                if(hpCost || mpCost)
                {
                    if(!cs) continue;

                    if(hpCost >= cs->GetHP() || mpCost > cs->GetMP())
                    {
                        continue;
                    }
                }

                skillID = skillData->GetCommon()->GetID();
            }

            if(skillID)
            {
                break;
            }
        }

        if(skillID)
        {
            auto cmd = std::make_shared<AIUseSkillCommand>(skillID, targetID);
            aiState->QueueCommand(cmd);

            return true;
        }
    }

    return false;
}

std::shared_ptr<AIMoveCommand> AIManager::GetMoveCommand(
    const std::shared_ptr<ActiveEntityState>& eState, const Point& dest,
    float reduce, bool split) const
{
    auto zone = eState->GetZone();
    if(!zone || !eState->CanMove())
    {
        return nullptr;
    }

    Point source(eState->GetCurrentX(), eState->GetCurrentY());
    if(source.GetDistance(dest) < reduce)
    {
        // Don't bother moving if we're trying to move away by accident
        return nullptr;
    }

    auto zoneManager = mServer.lock()->GetZoneManager();

    auto shortestPath = zoneManager->GetShortestPath(zone, source, dest);
    if(shortestPath.size() == 0)
    {
        // No valid path
        return nullptr;
    }

    auto cmd = std::make_shared<AIMoveCommand>();
    if(reduce > 0.f)
    {
        auto it = shortestPath.rbegin();
        Point& last = *it;

        Point secondLast;
        if(shortestPath.size() > 1)
        {
            it++;
            secondLast = *it;
        }
        else
        {
            secondLast = source;
        }

        float dist = secondLast.GetDistance(last);
        auto adjusted = zoneManager->GetLinearPoint(secondLast.x, secondLast.y,
            last.x, last.y, dist - reduce, false);
        last.x = adjusted.x;
        last.y = adjusted.y;
    }

    float moveSpeed = eState->GetMovementSpeed();
    if(split && moveSpeed > 0.f)
    {
        // Move in 0.5s increments so it looks less robotic
        // (maximum distance in 0.5s is = speed * 0.5);
        float maxMoveDistance = moveSpeed * 0.5f;

        Point prev = source;

        std::list<Point> pathing;
        for(auto& p : shortestPath)
        {
            if(prev.GetDistance(p) > maxMoveDistance)
            {
                // Break down into parts
                bool splitMore = true;
                while(splitMore)
                {
                    Point sub = zoneManager->GetLinearPoint(prev.x, prev.y,
                        p.x, p.y, maxMoveDistance, false);
                    pathing.push_back(sub);
                    prev = sub;

                    if(prev.GetDistance(p) <= maxMoveDistance)
                    {
                        pathing.push_back(p);
                        splitMore = false;
                        prev = p;
                    }
                }
            }
            else
            {
                pathing.push_back(p);
                prev = p;
            }
        }

        cmd->SetPathing(pathing);
    }
    else
    {
        cmd->SetPathing(shortestPath);
    }

    return cmd;
}

std::shared_ptr<AICommand> AIManager::GetWaitCommand(uint32_t waitTime) const
{
    auto cmd = std::make_shared<AICommand>();
    cmd->SetDelay((uint64_t)waitTime * 1000);

    return cmd;
}
