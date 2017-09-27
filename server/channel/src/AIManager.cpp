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

#include "AIManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>

// object Includes
#include <ActivatedAbility.h>
#include <MiBattleDamageData.h>
#include <MiCategoryData.h>
#include <MiDamageData.h>
#include <MiEffectiveRangeData.h>
#include <MiSkillBasicData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiTargetData.h>
#include <SpawnLocation.h>

// channel Includes
#include "AICommand.h"
#include "ChannelServer.h"

using namespace channel;

std::unordered_map<std::string,
    std::shared_ptr<libcomp::ScriptEngine>> AIManager::sPreparedScripts;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<AIManager>()
    {
        if(!BindingExists("AIManager"))
        {
            Using<ActiveEntityState>();

            Sqrat::Class<AIManager> binding(mVM, "AIManager");
            binding
                .Func<void (AIManager::*)(const std::shared_ptr<
                    ActiveEntityState>&, float, float, uint64_t)>(
                    "Move", &AIManager::Move)
                .Func<void (AIManager::*)(const std::shared_ptr<
                    AIState>, const libcomp::String&)>(
                    "QueueScriptCommand", &AIManager::QueueScriptCommand)
                .Func<void (AIManager::*)(const std::shared_ptr<
                    AIState>, uint32_t)>(
                    "QueueWaitCommand", &AIManager::QueueWaitCommand);

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

    if(!aiType.IsEmpty())
    {
        auto it = sPreparedScripts.find(aiType.C());
        if(it == sPreparedScripts.end())
        {
            auto script = mServer.lock()->GetServerDataManager()
                ->GetAIScript(aiType);
            if(!script)
            {
                return false;
            }

            auto engine = std::make_shared<libcomp::ScriptEngine>();
            engine->Using<AIManager>();

            if(!engine->Eval(script->Source))
            {
                return false;
            }

            sPreparedScripts[aiType.C()] = engine;
            aiState->SetScript(engine);
        }
        else
        {
            aiState->SetScript(it->second);
        }

        Sqrat::Function f(Sqrat::RootTable(aiState->GetScript()->GetVM()), "prepare");
        if(!f.IsNull())
        {
            auto result = !f.IsNull() ? f.Evaluate<int>(eState, this) : 0;
            return result && (*result == 0);
        }
    }

    return true;
}

void AIManager::UpdateActiveStates(const std::shared_ptr<Zone>& zone,
    uint64_t now)
{
    std::list<std::shared_ptr<EnemyState>> updated;
    for(auto enemy : zone->GetEnemies())
    {
        if(UpdateState(enemy, now))
        {
            updated.push_back(enemy);
        }
    }

    // Update enemy states first
    if(updated.size() > 0)
    {
        auto zConnections = zone->GetConnectionList();
        std::unordered_map<uint32_t, uint64_t> timeMap;
        for(auto enemy : updated)
        {
            // Update the clients with what the enemy is doing

            // Check if the enemy's position or rotation has updated
            if(now == enemy->GetOriginTicks())
            {
                if(enemy->IsMoving())
                {
                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MOVE);
                    p.WriteS32Little(enemy->GetEntityID());
                    p.WriteFloat(enemy->GetDestinationX());
                    p.WriteFloat(enemy->GetDestinationY());
                    p.WriteFloat(enemy->GetOriginX());
                    p.WriteFloat(enemy->GetOriginY());
                    p.WriteFloat(enemy->GetMovementSpeed());

                    timeMap[p.Size()] = now;
                    timeMap[p.Size() + 4] = enemy->GetDestinationTicks();
                    ChannelClientConnection::SendRelativeTimePacket(zConnections, p,
                        timeMap, true);
                }
                else if(enemy->IsRotating())
                {
                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ROTATE);
                    p.WriteS32Little(enemy->GetEntityID());
                    p.WriteFloat(enemy->GetDestinationRotation());

                    timeMap[p.Size()] = now;
                    timeMap[p.Size() + 4] = enemy->GetDestinationTicks();
                    ChannelClientConnection::SendRelativeTimePacket(zConnections, p,
                        timeMap, true);
                }
                else
                {
                    // The movement was actually a stop

                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_STOP_MOVEMENT);
                    p.WriteS32Little(enemy->GetEntityID());
                    p.WriteFloat(enemy->GetDestinationX());
                    p.WriteFloat(enemy->GetDestinationY());

                    timeMap[p.Size()] = enemy->GetDestinationTicks();
                    ChannelClientConnection::SendRelativeTimePacket(zConnections, p,
                        timeMap, true);
                }
            }
        }

        ChannelClientConnection::FlushAllOutgoing(zConnections);
    }
}

void AIManager::QueueScriptCommand(const std::shared_ptr<AIState> aiState,
    const libcomp::String& functionName)
{
    auto cmd = std::make_shared<AIScriptedCommand>(functionName);
    aiState->QueueCommand(cmd);
}

void AIManager::QueueWaitCommand(const std::shared_ptr<AIState> aiState,
    uint32_t waitTime)
{
    auto cmd = GetWaitCommand(waitTime);
    aiState->QueueCommand(cmd);
}

void AIManager::Move(const std::shared_ptr<ActiveEntityState>& eState,
    float xPos, float yPos, uint64_t now)
{
    bool canMove = eState->CanMove();

    /// @todo: check collision and correct

    if(canMove)
    {
        eState->Move(xPos, yPos, now);
    }
}

bool AIManager::UpdateState(const std::shared_ptr<ActiveEntityState>& eState,
    uint64_t now)
{
    eState->RefreshCurrentPosition(now);

    auto aiState = eState->GetAIState();
    if(!aiState || (aiState->IsIdle() && !aiState->IsOverriden("idle")))
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

        if(aiState->IsOverriden(actionName))
        {
            libcomp::String functionOverride = aiState->GetScriptFunction(actionName);
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
            case objects::EntityStateObject::EntityType_t::ENEMY:
                {
                    auto enemyState = std::dynamic_pointer_cast<EnemyState>(eState);
                    return UpdateEnemyState(enemyState, now);
                }
            case objects::EntityStateObject::EntityType_t::PARTNER_DEMON: // Maybe someday
                /// @todo: handle ally NPCs
            default:
                break;
            }
        }
    }

    auto cmd = aiState->GetCurrentCommand();
    if(cmd)
    {
        if(!cmd->Started())
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
                    float x = 0.f, y = 0.f;
                    while(cmdMove->GetCurrentDestination(x, y))
                    {
                        if(x != eState->GetCurrentX() ||
                            y != eState->GetCurrentY())
                        {
                            Move(eState, x, y, now);
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
                // Do nothing if hit stunned
                if(eState->GetStatusTimes(STATUS_HIT_STUN) ||
                    eState->GetStatusTimes(STATUS_KNOCKBACK))
                {
                    return false;
                }

                auto cmdSkill = std::dynamic_pointer_cast<AIUseSkillCommand>(cmd);

                auto server = mServer.lock();
                auto skillManager = server->GetSkillManager();

                bool valid = true;

                auto activated = cmdSkill->GetActivatedAbility();
                if(cmdSkill->GetTargetObjectID() > 0)
                {
                    auto targetEntity = eState->GetZone()->GetActiveEntity(
                        (int32_t)cmdSkill->GetTargetObjectID());
                    if(!targetEntity || !targetEntity->IsAlive())
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
                    auto ctx = std::make_shared<SkillExecutionContext>();
                    ctx->FreeCast = true;

                    if(activated)
                    {
                        // Execute the skill
                        if(skillManager->ExecuteSkill(eState, activated->GetActivationID(),
                            activated->GetTargetObjectID(), ctx))
                        {
                            // Queue a wait command
                            QueueWaitCommand(aiState, RNG(uint32_t, 4, 6));
                        }
                        else
                        {
                            /// @todo: if charged and the target has run away, pursue or cancel
                            skillManager->CancelSkill(eState, activated->GetActivationID());
                        }
                    }
                    else
                    {
                        // Activate the skill
                        skillManager->ActivateSkill(eState, cmdSkill->GetSkillID(),
                            cmdSkill->GetTargetObjectID(), ctx);
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

bool AIManager::UpdateEnemyState(const std::shared_ptr<EnemyState>& eState, uint64_t now)
{
    auto server = mServer.lock();

    auto aiState = eState->GetAIState();
    if(aiState->GetTarget() == 0 && eState->GetOpponentIDs().size() == 0)
    {
        Retarget(eState, now);
    }

    if(aiState->GetStatus() == AIStatus_t::WANDERING)
    {
        auto entity = eState->GetEntity();
        auto spawnLocation = entity->GetSpawnLocation();
        uint32_t spotID = entity->GetSpawnSpotID();

        if((spawnLocation || spotID > 0) && eState->CanMove())
        {
            Point dest;
            if(spawnLocation)
            {
                auto point = server->GetZoneManager()->GetRandomPoint(
                    spawnLocation->GetWidth(), spawnLocation->GetHeight());

                // Spawn location group bounding box points start in the top left
                // corner of the rectangle and extend towards +X/-Y
                dest.x = (float)(spawnLocation->GetX() + point.x);
                dest.y = (float)(spawnLocation->GetY() - point.y);
            }
            else
            {
                dest = server->GetZoneManager()->GetRandomSpotPoint(
                    eState->GetZone()->GetDynamicMap()->Spots[spotID]->Definition);
            }

            Point source(eState->GetCurrentX(), eState->GetCurrentY());

            auto command = GetMoveCommand(eState->GetZone(), source, dest);
            if(command)
            {
                aiState->QueueCommand(command);
            }
        }

        /// @todo: determine what this should be per demon type
        QueueWaitCommand(aiState, RNG(uint32_t, 3, 9));

        return true;
    }

    // If not wandering, handle aggro and combat somewhat similarly
    bool inCombat = aiState->GetStatus() == AIStatus_t::COMBAT;

    auto zone = eState->GetZone();
    int32_t targetEntityID = aiState->GetTarget();
    auto target = targetEntityID > 0
        ? zone->GetActiveEntity(targetEntityID) : nullptr;
    if(!target || !target->IsAlive() || !target->Ready())
    {
        if(inCombat)
        {
            // Try to find another opponent
            target = Retarget(eState, now);
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

    bool targetChanged = false;
    if(targetDist >= 5000.0)
    {
        // De-aggro on that one target and find a new one
        server->GetCharacterManager()->AddRemoveOpponent(false,
            eState, target);

        target = Retarget(eState, now);
        targetChanged = true;
    }

    auto activated = eState->GetActivatedAbility();
    if(!target)
    {
        // No target could be found, stop combat and quit
        if(activated)
        {
            server->GetSkillManager()->CancelSkill(eState, activated->GetActivationID());
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

    if(activated)
    {
        auto skillManager = server->GetSkillManager();

        // Skill charged, cancel, execute or move within range
        int64_t activationTarget = activated->GetActivationObjectID();
        if(activationTarget && targetEntityID != (int32_t)activationTarget)
        {
            // Target changed, cancel skill
            /// @todo: implement target switching
            skillManager->CancelSkill(eState, activated->GetActivationID());
            return false;
        }

        auto definitionManager = server->GetDefinitionManager();
        auto skillData = definitionManager->GetSkillData(activated->GetSkillID());
        uint16_t maxTargetRange = (uint16_t)(400 + (skillData->GetTarget()->GetRange() * 10));

        if(targetDist > maxTargetRange)
        {
            // Move within range
            Point source(eState->GetCurrentX(), eState->GetCurrentY());

            auto zoneManager = server->GetZoneManager();
            auto point = zoneManager->GetLinearPoint(source.x, source.y,
                targetX, targetY, (float)(targetDist - (float)maxTargetRange - 10.f), false);

            auto cmd = GetMoveCommand(zone, source, point);
            if(cmd)
            {
                aiState->QueueCommand(cmd);
            }
            else
            {
                skillManager->CancelSkill(eState, activated->GetActivationID());
            }
        }
        else
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
            QueueWaitCommand(aiState, RNG(uint32_t, 1, 3));
        }
        else
        {
            if(targetDist > 300.f)
            {
                // Run up to the target but don't do anything yet
                Point source(eState->GetCurrentX(), eState->GetCurrentY());

                auto zoneManager = server->GetZoneManager();
                auto point = zoneManager->GetLinearPoint(source.x, source.y,
                    targetX, targetY, targetDist, false);

                auto cmd = GetMoveCommand(zone, source, point, 250.f);
                if(cmd)
                {
                    aiState->QueueCommand(cmd);
                }
                else
                {
                    // If the enemy can't move to the target, retarget
                    // and wait
                    Retarget(eState, now);
                    QueueWaitCommand(aiState, 2);
                }
            }
            else if(eState->CurrentSkillsCount() > 0)
            {
                RefreshSkillMap(eState, aiState);

                auto skillMap = aiState->GetSkillMap();
                if(skillMap.size() > 0)
                {
                    /// @todo: change skill selection logic
                    uint16_t priorityType = RNG(uint16_t, AI_SKILL_TYPE_CLSR, AI_SKILL_TYPE_LNGR);
                    for(uint16_t skillType : { priorityType,
                        priorityType == AI_SKILL_TYPE_LNGR ? AI_SKILL_TYPE_CLSR : AI_SKILL_TYPE_LNGR })
                    {
                        if(skillMap[skillType].size() > 0)
                        {
                            size_t randIdx = (size_t)
                                RNG(uint16_t, 0, (uint16_t)(skillMap[skillType].size() - 1));

                            uint32_t skillID = skillMap[skillType][randIdx];
                            auto cmd = std::make_shared<AIUseSkillCommand>(skillID,
                                (int64_t)targetEntityID);
                            aiState->QueueCommand(cmd);
                        }
                    }
                }
            }
        }
    }

    return false;
}

std::shared_ptr<ActiveEntityState> AIManager::Retarget(const std::shared_ptr<EnemyState>& eState,
    uint64_t now)
{
    std::shared_ptr<ActiveEntityState> target;

    auto aiState = eState->GetAIState();
    aiState->SetTarget(0);

    auto zone = eState->GetZone();
    float sourceX = eState->GetCurrentX();
    float sourceY = eState->GetCurrentY();

    auto opponentIDs = eState->GetOpponentIDs();
    std::list<std::shared_ptr<ActiveEntityState>> possibleTargets;
    if(opponentIDs.size() > 0)
    {
        // Currently in combat, only pull from opponents
        auto inRange = zone->GetActiveEntitiesInRadius(
            sourceX, sourceY, 4000.0);

        for(auto entity : inRange)
        {
            if(opponentIDs.find(entity->GetEntityID()) != opponentIDs.end()
                && entity->IsAlive() && entity->Ready())
            {
                possibleTargets.push_back(entity);
            }
        }
    }
    else
    {
        // Not in combat, find a target to pursue

        // Default to 3000 units and 80 degree FoV angle (in radians)
        /// @todo: use different value settings for different enemies
        double maxAggroDistance = 3000.0;
        float fovAngle = 1.395f;

        auto geometry = zone->GetGeometry();

        // Get all active entities in range
        auto inRange = zone->GetActiveEntitiesInRadius(sourceX,
            sourceY, maxAggroDistance);

        // Remove allies and entities not ready yet
        inRange.remove_if([eState](
            const std::shared_ptr<ActiveEntityState>& entity)
            {
                return entity->GetFaction() == eState->GetFaction() ||
                    !entity->Ready();
            });
        
        if(inRange.size() > 0)
        {
            // Targets found, check if they're visible
            for(auto entity : inRange)
            {
                entity->RefreshCurrentPosition(now);
            }

            // Filter the set down to only entities in the FoV
            inRange = ZoneManager::GetEntitiesInFoV(inRange, sourceX,
                sourceY, eState->GetCurrentRotation(), fovAngle);

            for(auto entity : inRange)
            {
                // Possible target found, check line of sight
                bool add = true;
                if(geometry)
                {
                    Line path(Point(sourceX, sourceY),
                        Point(entity->GetCurrentX(), entity->GetCurrentY()));

                    Point collidePoint;
                    add = !geometry->Collides(path, collidePoint);
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
        /// @todo: add better target selection logic
        auto targetIter = possibleTargets.begin();
        size_t randomIdx = (size_t)RNG(int32_t, 0,
            (int32_t)(possibleTargets.size() - 1));
        std::advance(targetIter, randomIdx);

        target = *targetIter;

        aiState->SetTarget(target ? target->GetEntityID() : 0);
    }

    return target;
}

void AIManager::RefreshSkillMap(const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<AIState>& aiState)
{
    if(!aiState->SkillsMapped())
    {
        auto isEnemy = eState->GetEntityType() ==
            objects::EntityStateObject::EntityType_t::ENEMY;

        std::unordered_map<uint16_t, std::vector<uint32_t>> skillMap;
        auto definitionManager = mServer.lock()->GetDefinitionManager();

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
                            skillMap[AI_SKILL_TYPE_CLSR].push_back(skillID);
                        }
                        else
                        {
                            skillMap[AI_SKILL_TYPE_LNGR].push_back(skillID);
                        }
                    }
                    break;
                case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
                case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT:
                case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
                    {
                        skillMap[AI_SKILL_TYPE_HEAL].push_back(skillID);
                    }
                    break;
                default:
                    /// @todo: determine the difference between self buff and defense
                    if(targetType == (int16_t)AI_SKILL_TYPE_DEF)
                    {
                        skillMap[AI_SKILL_TYPE_DEF].push_back(skillID);
                    }
                    else
                    {
                        skillMap[AI_SKILL_TYPE_SUPPORT].push_back(skillID);
                    }
                    break;
                }
            }
        }

        aiState->SetSkillMap(skillMap);
    }
}

std::shared_ptr<AIMoveCommand> AIManager::GetMoveCommand(const std::shared_ptr<
    Zone>& zone, const Point& source, const Point& dest, float reduce) const
{
    if(source.GetDistance(dest) < reduce)
    {
        // Don't bother moving if we're trying to move away by accident
        return nullptr;
    }

    auto zoneManager = mServer.lock()->GetZoneManager();

    auto cmd = std::make_shared<AIMoveCommand>();

    std::list<std::pair<float, float>> pathing;

    bool collision = false;

    std::list<Point> shortestPath;
    auto geometry = zone->GetGeometry();
    if(geometry)
    {
        Line path(source, dest);

        Point collidePoint;
        Line outSurface;
        std::shared_ptr<ZoneShape> outShape;
        if(geometry->Collides(path, collidePoint, outSurface, outShape))
        {
            /*// Move off the collision point by 10
            collidePoint = zoneManager->GetLinearPoint(collidePoint.x,
                collidePoint.y, source.x, source.y, 10.f, false);

            /// @todo: handle pathing properly

            collision = true;*/
            return nullptr;
        }
    }

    if(!collision)
    {
        shortestPath.push_back(dest);
    }

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

    for(auto p : shortestPath)
    {
        pathing.push_back(std::pair<float, float>(p.x, p.y));
    }

    cmd->SetPathing(pathing);

    return cmd;
}

std::shared_ptr<AICommand> AIManager::GetWaitCommand(uint32_t waitTime) const
{
    auto cmd = std::make_shared<AICommand>();
    cmd->SetDelay((uint64_t)waitTime * 1000000);

    return cmd;
}
