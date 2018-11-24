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
#include <ServerConstants.h>

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
#include <WorldSharedConfig.h>

// channel Includes
#include "AICommand.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"
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
                .Func("QueueMoveCommand", &AIManager::QueueMoveCommand)
                .Func("QueueScriptCommand", &AIManager::QueueScriptCommand)
                .Func("QueueUseSkillCommand", &AIManager::QueueUseSkillCommand)
                .Func("QueueWaitCommand", &AIManager::QueueWaitCommand)
                .Func("Chase", &AIManager::Chase)
                .Func("Circle", &AIManager::Circle)
                .Func("Retreat", &AIManager::Retreat);

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
    if(eBase)
    {
        if(eBase->GetSpawnLocation() || eBase->GetSpawnSpotID())
        {
            // Default to wandering first
            aiState->SetStatus(AIStatus_t::WANDERING, true);
        }

        if(eBase->GetEncounterID())
        {
            // Nothing with an encounter ID despawns when lost or we could
            // potentially break defeat actions by accident
            aiState->SetDespawnWhenLost(false);
        }
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
    aiState->ResetStatusChanged();

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

        if(aiState->ActionOverridesKeyExists("combatSkillHit"))
        {
            libcomp::String fOverride = aiState->GetActionOverrides(
                "combatSkillHit");

            Sqrat::Function f(Sqrat::RootTable(aiState->GetScript()->GetVM()),
                fOverride.IsEmpty() ? "combatSkillHit" : fOverride.C());

            /// @todo: bind MiSkillData and pass that
            auto scriptResult = !f.IsNull()
                ? f.Evaluate<int32_t>(eState, this, source,
                    skillData->GetCommon()->GetID()) : 0;
            if(!scriptResult || *scriptResult == 0)
            {
                // Do not continue
                return;
            }
        }

        if(!eState->SameFaction(source))
        {
            // If the entity's current target is not the source of this skill,
            // there is a chance they will target them now (20% chance by
            // default)
            if(aiState->GetTargetEntityID() != source->GetEntityID() &&
                RNG(int32_t, 1, 10) <= 2)
            {
                UpdateAggro(eState, source->GetEntityID());
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

    // Multiple triggers in combat cause normal AI to reset and reorient
    // itself so they're not spamming skills non-stop
    bool reset = false;
    bool wait = true;
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
            // or guard for example)
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
                auto cmd = std::make_shared<AIUseSkillCommand>(skillData,
                    target->GetEntityID());
                aiState->QueueCommand(cmd);
            }
            else
            {
                reset = true;
            }
        }
        else
        {
            // Check what kind of skill it was to decide how to handle
            switch(skillData->GetBasic()->GetActionType())
            {
            case objects::MiSkillBasicData::ActionType_t::GUARD:
            case objects::MiSkillBasicData::ActionType_t::COUNTER:
            case objects::MiSkillBasicData::ActionType_t::DODGE:
                // Reset actions but do not wait
                reset = true;
                wait = false;
                break;
            default:
                // Other skills should be staggered by thinkspeed unless
                // more executions exist
                reset = activated->GetExecuteCount() >= activated
                    ->GetMaxUseCount();
                break;
            }
        }
    }

    if(reset)
    {
        aiState->ClearCommands();
        if(wait)
        {
            QueueWaitCommand(aiState, (uint32_t)aiState->GetThinkSpeed());
        }
    }
}

bool AIManager::QueueMoveCommand(const std::shared_ptr<ActiveEntityState>& eState,
    float x, float y, bool interrupt)
{
    auto aiState = eState ? eState->GetAIState() : nullptr;
    if(!aiState)
    {
        return false;
    }

    auto cmdMove = GetMoveCommand(eState, Point(x, y));
    if(cmdMove)
    {
        aiState->QueueCommand(cmdMove, interrupt);

        return true;
    }

    return false;
}

bool AIManager::QueueUseSkillCommand(
    const std::shared_ptr<ActiveEntityState>& eState, uint32_t skillID,
    int32_t targetEntityID, bool advance)
{
    auto aiState = eState ? eState->GetAIState() : nullptr;
    if(!aiState)
    {
        return false;
    }

    auto definitionManager = mServer.lock()->GetDefinitionManager();
    auto skillData = definitionManager->GetSkillData(skillID);
    if(!skillData)
    {
        return false;
    }

    if(advance)
    {
        SkillAdvance(eState, skillData);
    }

    auto skillCmd = std::make_shared<AIUseSkillCommand>(skillData,
        targetEntityID);
    aiState->QueueCommand(skillCmd);

    return true;
}

void AIManager::QueueScriptCommand(const std::shared_ptr<AIState>& aiState,
    const libcomp::String& functionName, bool interrupt)
{
    if(aiState)
    {
        auto cmd = std::make_shared<AIScriptedCommand>(functionName);
        aiState->QueueCommand(cmd, interrupt);
    }
}

void AIManager::QueueWaitCommand(const std::shared_ptr<AIState>& aiState,
    uint32_t waitTime, bool interrupt)
{
    if(aiState)
    {
        auto cmd = GetWaitCommand(waitTime);
        aiState->QueueCommand(cmd, interrupt);
    }
}

bool AIManager::StartEvent(const std::shared_ptr<ActiveEntityState>& eState,
    const libcomp::String& eventID)
{
    if(eState)
    {
        auto eventManager = mServer.lock()->GetEventManager();

        EventOptions options;
        options.AutoOnly = true;

        return eventManager->HandleEvent(nullptr, eventID, eState
            ->GetEntityID(), eState->GetZone(), options);
    }

    return false;
}

void AIManager::UpdateAggro(const std::shared_ptr<ActiveEntityState>& eState,
    int32_t targetID)
{
    auto aiState = eState->GetAIState();
    auto zone = eState->GetZone();
    int32_t currentTargetID = aiState ? aiState->GetTargetEntityID() : -1;
    if(aiState && zone && currentTargetID != targetID)
    {
        if(currentTargetID > 0)
        {
            // Clear old aggro
            auto oldTarget = zone->GetActiveEntity(currentTargetID);
            if(oldTarget)
            {
                oldTarget->RemoveAggroIDs(eState->GetEntityID());
            }
        }

        if(targetID > 0)
        {
            // Set aggro
            if(aiState->GetStatus() == AIStatus_t::IDLE ||
                aiState->GetStatus() == AIStatus_t::WANDERING)
            {
                aiState->SetStatus(AIStatus_t::AGGRO);
            }

            auto newTarget = zone->GetActiveEntity(targetID);
            if(newTarget)
            {
                newTarget->InsertAggroIDs(eState->GetEntityID());
            }
        }
        else if(currentTargetID > 0)
        {
            // Remove all commands that targeted the old entity
            auto cmd = aiState->GetCurrentCommand();
            while(cmd && cmd->GetTargetEntityID() == currentTargetID)
            {
                aiState->PopCommand();
                cmd = aiState->GetCurrentCommand();
            }
        }

        aiState->SetTargetEntityID(targetID);

        if(eState->GetEnemyBase())
        {
            // Enemies and allies telegraph who they are targeting by facing them
            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENEMY_ACTIVATED);
            p.WriteS32Little(eState->GetEntityID());
            p.WriteS32Little(aiState->GetTargetEntityID());

            ChannelClientConnection::BroadcastPacket(zone->GetConnectionList(), p);
        }
    }
}

bool AIManager::Chase(const std::shared_ptr<ActiveEntityState>& eState,
    int32_t targetEntityID, float minDistance, float maxDistance,
    bool interrupt)
{
    auto zone = eState ? eState->GetZone() : nullptr;
    auto aiState = eState ? eState->GetAIState() : nullptr;
    if(!zone || !aiState)
    {
        return false;
    }

    auto targetEntity = zone->GetActiveEntity(targetEntityID);
    if(!targetEntity || eState == targetEntity)
    {
        return false;
    }

    Point src(eState->GetCurrentX(), eState->GetCurrentY());
    Point dest(targetEntity->GetCurrentX(), targetEntity->GetCurrentY());

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto point = zoneManager->GetLinearPoint(src.x, src.y,
        dest.x, dest.y, src.GetDistance(dest), false);

    auto cmd = GetMoveCommand(eState, point);
    if(cmd)
    {
        cmd->SetTargetEntityID(targetEntityID);
        cmd->SetTargetDistance(minDistance, true);
        cmd->SetTargetDistance(maxDistance, false);
        aiState->QueueCommand(cmd, interrupt);

        return true;
    }
    else
    {
        return false;
    }
}

bool AIManager::Retreat(const std::shared_ptr<ActiveEntityState>& eState,
    float x, float y, float distance, bool interrupt)
{
    auto zone = eState ? eState->GetZone() : nullptr;
    auto aiState = eState ? eState->GetAIState() : nullptr;
    if(!zone || !aiState || !eState->CanMove())
    {
        return false;
    }

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    Point src(eState->GetCurrentX(), eState->GetCurrentY());
    Point target(x, y);

    Point retreatPoint = zoneManager->GetLinearPoint(target.x, target.y,
        src.x, src.y, distance, false, zone);
    if(retreatPoint.GetDistance(target) > src.GetDistance(target))
    {
        std::list<Point> pathing;
        pathing.push_back(retreatPoint);

        auto cmd = std::make_shared<AIMoveCommand>();
        cmd->SetPathing(pathing);
        aiState->QueueCommand(cmd, interrupt);

        return true;
    }

    return false;
}

bool AIManager::Circle(const std::shared_ptr<ActiveEntityState>& eState,
    float x, float y, bool interrupt, float distance)
{
    auto zone = eState ? eState->GetZone() : nullptr;
    auto aiState = eState ? eState->GetAIState() : nullptr;
    if(!zone || !aiState || !eState->CanMove())
    {
        return false;
    }

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    Point src(eState->GetCurrentX(), eState->GetCurrentY());
    Point target(x, y);

    std::list<Point> pathing;

    // Distance from the target first
    Point start = zoneManager->GetLinearPoint(target.x, target.y,
        src.x, src.y, distance, false, zone);
    if(start != src)
    {
        // If we can't get to the start point in a straight line
        // quit now as the move should not be that complex
        Point collidePoint;
        if(zone->Collides(Line(start, src), collidePoint))
        {
            return false;
        }

        pathing.push_back(start);
    }

    // Rotate the starting point around the target for the second
    // (or third) point
    int32_t pointCount = RNG(int32_t, 1, 2);
    bool invert = RNG(int32_t, 1, 2) == 1;
    for(int32_t i = 0; i < pointCount; i++)
    {
        Point prev = pathing.size() > 0 ? pathing.back() : src;
        Point p = zoneManager->RotatePoint(prev, target,
            invert ? -0.52f : 0.52f);

        // Check for collision
        Point pFinal = zoneManager->GetLinearPoint(prev.x, prev.y,
            p.x, p.y, prev.GetDistance(p), false, zone);
        if(pFinal != p && i == 0)
        {
            // Hit something, try the other direction (only once)
            pointCount++;
            invert = !invert;
        }
        else if(pFinal == prev)
        {
            // Can't move further
            break;
        }
        else
        {
            // Add the point
            pathing.push_back(pFinal);
        }
    }

    if(pathing.size() > 0)
    {
        auto cmd = std::make_shared<AIMoveCommand>();
        cmd->SetPathing(pathing);
        aiState->QueueCommand(cmd, interrupt);

        return true;
    }

    return false;
}

bool AIManager::UpdateState(const std::shared_ptr<ActiveEntityState>& eState,
    uint64_t now, bool isNight)
{
    eState->RefreshCurrentPosition(now);

    auto aiState = eState->GetAIState();
    if(!aiState)
    {
        return false;
    }

    uint64_t despawnTimout = aiState->GetDespawnTimeout();
    if(despawnTimout && despawnTimout <= now)
    {
        // Despawn it and quit
        auto zone = eState->GetZone();
        if(zone)
        {
            zone->MarkDespawn(eState->GetEntityID());
        }

        aiState->SetDespawnTimeout(0);
        return false;
    }

    if(aiState->IsIdle() &&
        !aiState->ActionOverridesKeyExists("idle"))
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
            libcomp::String fOverride = aiState->GetActionOverrides(actionName);
            if(!fOverride.IsEmpty())
            {
                // Queue the overridden function
                QueueScriptCommand(aiState, fOverride);
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

        auto zone = eState->GetZone();
        if(!zone)
        {
            return false;
        }

        std::shared_ptr<ActiveEntityState> targetEntity;
        if(cmd->GetTargetEntityID() > 0)
        {
            targetEntity = zone->GetActiveEntity(cmd->GetTargetEntityID());
            if(targetEntity)
            {
                targetEntity->RefreshCurrentPosition(now);
            }
        }

        switch(cmd->GetType())
        {
        case AICommandType_t::MOVE:
            if(eState->CanMove())
            {
                Point src(eState->GetCurrentX(), eState->GetCurrentY());

                auto cmdMove = std::dynamic_pointer_cast<AIMoveCommand>(cmd);

                bool moving = eState->IsMoving();
                float minDistance = cmdMove->GetTargetDistance(true);
                if(moving || minDistance)
                {
                    float maxDistance = cmdMove->GetTargetDistance(false);
                    if(targetEntity && (minDistance || maxDistance))
                    {
                        // Make sure the destination is still close to the
                        // target
                        Point tPoint(targetEntity->GetCurrentX(),
                            targetEntity->GetCurrentY());
                        if(moving && maxDistance &&
                            tPoint.GetDistance(src) >= maxDistance)
                        {
                            // Quit movement
                            targetEntity->Stop(now);
                            aiState->PopCommand();
                        }
                        else if(moving && minDistance &&
                            tPoint.GetDistance(src) <= minDistance)
                        {
                            // Movement is done, stop now
                            targetEntity->Stop(now);
                            aiState->PopCommand();
                        }
                        else if(minDistance)
                        {
                            Point endPoint;
                            if(cmdMove->GetEndDestination(endPoint) &&
                                floor(tPoint.GetDistance(endPoint)) > minDistance)
                            {
                                // End point is no longer valid, repath
                                auto cmdNew = GetMoveCommand(eState, tPoint,
                                    minDistance);
                                if(cmdNew && cmdMove->GetEndDestination(endPoint))
                                {
                                    cmdMove->SetPathing(cmdNew->GetPathing());
                                }
                                else
                                {
                                    aiState->PopCommand();
                                }

                                return false;
                            }
                        }
                    }

                    if(moving)
                    {
                        return false;
                    }
                }

                // Move to the first point in the path that is not the
                // entity's current position
                Point dest;
                while(cmdMove->GetCurrentDestination(dest))
                {
                    if(dest != src)
                    {
                        Move(eState, dest, now);
                        return true;
                    }
                    else if(!cmdMove->SetNextDestination())
                    {
                        break;
                    }
                }

                aiState->PopCommand(cmdMove);
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

                if(cmdSkill->GetTargetEntityID() > 0)
                {
                    if(!targetEntity || !targetEntity->IsAlive() ||
                        targetEntity->GetAIIgnored())
                    {
                        // Target invalid or dead, cancel the skill and move on
                        if(activated)
                        {
                            skillManager->CancelSkill(eState, activated
                                ->GetActivationID());
                        }

                        // Not valid
                        aiState->PopCommand(cmdSkill);
                        return false;
                    }
                }

                if(targetEntity)
                {
                    targetEntity->ExpireStatusTimes(now);

                    if(AggroLimitEnabled() &&
                        targetEntity->GetStatusTimes(STATUS_KNOCKBACK))
                    {
                        // Delay execution or activation
                        QueueWaitCommand(aiState, (uint32_t)aiState
                            ->GetThinkSpeed(), true);
                        return false;
                    }
                }

                if(activated)
                {
                    // Execute the skill
                    if(!skillManager->ExecuteSkill(eState, activated
                        ->GetActivationID(), activated->GetTargetObjectID()) &&
                        eState->GetActivatedAbility() == activated)
                    {
                        if(!CanRetrySkill(eState, activated))
                        {
                            skillManager->CancelSkill(eState, activated
                                ->GetActivationID());
                        }
                    }
                }
                else
                {
                    // Activate the skill
                    if(skillManager->ActivateSkill(eState, cmdSkill
                        ->GetSkillID(), cmdSkill->GetTargetEntityID(),
                        cmdSkill->GetTargetEntityID(), ACTIVATION_TARGET))
                    {
                        auto skillData = cmdSkill->GetSkillData();
                        if(skillData->GetBasic()->GetActivationType() == 5)
                        {
                            aiState->SetSkillWaitStart(now);
                        }
                    }
                }

                aiState->PopCommand(cmdSkill);
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

    auto zone = eState->GetZone();
    int32_t targetEntityID = aiState->GetTargetEntityID();
    auto target = targetEntityID > 0
        ? zone->GetActiveEntity(targetEntityID) : nullptr;
    if(!target || !target->IsAlive() || !target->Ready() ||
        target->GetAIIgnored())
    {
        // Try to find another target
        target = Retarget(eState, now, isNight);

        if(!target)
        {
            // Reset to default state and quit
            UpdateAggro(eState, -1);
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
        if(eState->GetStatusTimes(STATUS_CHARGING))
        {
            // Let charging finish
            return false;
        }

        bool cancelAndReset = false;
        if(!CanRetrySkill(eState, activated))
        {
            // Somehow we have an error
            cancelAndReset = true;
        }
        else if(aiState->GetSkillWaitStart())
        {
            // If we have a skill waiting on being hit and we've been
            // waiting a decent amount of time, check if we should cancel
            int32_t thinkSpeedAdjust = aiState->GetThinkSpeed()
                ? aiState->GetThinkSpeed() : 1000;
            int32_t minWait = thinkSpeedAdjust * 2 * 1000;
            if(now > aiState->GetSkillWaitStart() + (uint64_t)minWait &&
                RNG(uint16_t, 1, 2) == 1)
            {
                cancelAndReset = true;
            }
            else
            {
                if(target && aiState->GetDefensiveDistance() > 0.f)
                {
                    // Circle the target
                    Circle(eState, targetX, targetY, true,
                        aiState->GetDefensiveDistance());
                }

                // Wait no matter what
                QueueWaitCommand(aiState, (uint32_t)thinkSpeedAdjust);
                return false;
            }
        }
        
        if(cancelAndReset)
        {
            aiState->SetSkillWaitStart(0);
            server->GetSkillManager()->CancelSkill(eState, activated
                ->GetActivationID());
            activated = nullptr;
        }
    }
    else if(aiState->GetSkillWaitStart())
    {
        // Shouldn't be set anymore
        aiState->SetSkillWaitStart(0);
    }

    if(activated)
    {
        auto skillManager = server->GetSkillManager();

        // Skill charged, cancel, execute or move within range
        int64_t activationTarget = activated->GetTargetObjectID();
        if(activationTarget > 0 &&
            targetEntityID != (int32_t)activationTarget &&
            eState->GetEntityID() != (int32_t)activationTarget)
        {
            // Target changed
            skillManager->TargetSkill(eState, targetEntityID);
            return false;
        }

        auto definitionManager = server->GetDefinitionManager();
        auto skillData = definitionManager->GetSkillData(activated
            ->GetSkillID());

        // Move forward if needed and execute when close enough
        uint8_t moveResponse = SkillAdvance(eState, skillData);
        if(moveResponse == 0)
        {
            // Moving forward, stop here
            return false;
        }
        else if(moveResponse == 1)
        {
            // Could not move forward
            skillManager->CancelSkill(eState, activated->GetActivationID());
        }
        else if(CanRetrySkill(eState, activated))
        {
            // Execute the skill
            auto cmd = std::make_shared<AIUseSkillCommand>(skillData,
                activated);
            aiState->QueueCommand(cmd);
        }
    }
    else
    {
        // 20% chance to just wait (lower for high aggression)
        int16_t waitChance = (int16_t)(100.f *
            (float)aiState->GetAggression() * 0.01f);
        if(RNG(int16_t, 1, waitChance > 25 ? waitChance : 25) <= 20)
        {
            QueueWaitCommand(aiState, (uint32_t)aiState->GetThinkSpeed());
        }
        else if(eState->CurrentSkillsCount() > 0)
        {
            // If aggro limit is enabled and the target is being knocked back
            // wait instead to stagger attacks
            if(target)
            {
                target->ExpireStatusTimes(now);
                if(AggroLimitEnabled() &&
                    target->GetStatusTimes(STATUS_KNOCKBACK))
                {
                    QueueWaitCommand(aiState,
                        (uint32_t)aiState->GetThinkSpeed());
                    return false;
                }
            }

            // All normal movement is based off skill usage, determine which
            // skill to use next
            if(!PrepareSkillUsage(eState))
            {
                // No skill can be used, drop aggro
                UpdateAggro(eState, -1);

                // Run away if defensive distance specified
                if(aiState->GetDefensiveDistance() > 0.f)
                {
                    Retreat(eState, targetX, targetY, aiState
                        ->GetDefensiveDistance(), true);
                }
            }
        }
        else
        {
            // No skills exist, drop aggro
            UpdateAggro(eState, -1);
        }
    }

    return false;
}

void AIManager::Move(const std::shared_ptr<ActiveEntityState>& eState,
    Point dest, uint64_t now)
{
    auto zone = eState->GetZone();
    if(!eState->CanMove() || !zone)
    {
        return;
    }

    Point collidePoint;
    if(zone->Collides(Line(Point(eState->GetCurrentX(), eState->GetCurrentY()),
        dest), collidePoint))
    {
        // Cannot reach the destination, clear commands and quit
        eState->GetAIState()->ClearCommands();
        return;
    }

    eState->Move(dest.x, dest.y, now);
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
        if(!zone)
        {
            return;
        }

        Point source(eState->GetCurrentX(), eState->GetCurrentY());

        // If the entity has a despawn timeout, they should attempt to wander
        // back to the spawn location
        bool wanderBack = aiState->GetDespawnTimeout() > 0;

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

            if(wanderBack)
            {
                std::list<Point> vertices;
                vertices.push_back(Point(spawnLocation->GetX(),
                    spawnLocation->GetY()));
                vertices.push_back(Point(spawnLocation->GetX() +
                    spawnLocation->GetWidth(), spawnLocation->GetY()));
                vertices.push_back(Point(
                    spawnLocation->GetX() + spawnLocation->GetWidth(),
                    spawnLocation->GetY() - spawnLocation->GetHeight()));
                vertices.push_back(Point(spawnLocation->GetX(),
                    spawnLocation->GetY() - spawnLocation->GetHeight()));

                wanderBack = !zoneManager->PointInPolygon(source, vertices);
            }
        }
        else
        {
            auto spot = zone->GetDynamicMap()->Spots[spotID];
            dest = zoneManager->GetRandomSpotPoint(spot->Definition);

            wanderBack &= !zoneManager->PointInPolygon(source, spot->Vertices);
        }

        // Use the destination as a direction to head and either
        // limit/extend to think speed distance (minimum 500ms)
        float moveDistance = (float)((float)eState->GetMovementSpeed() *
            (float)(thinkSpeed < 500 ? 500 : thinkSpeed) * 0.001f);

        Point finalDest = zoneManager->GetLinearPoint(source.x, source.y,
            dest.x, dest.y, moveDistance, false, zone);

        bool canReach = source.GetDistance(finalDest) >=
            source.GetDistance(dest);
        if(!canReach && wanderBack)
        {
            // Pull the shortest path and follow the first part of the path
            auto path = zoneManager->GetShortestPath(zone, source, dest);
            if(path.size() > 0)
            {
                finalDest = path.front();
                if(source.GetDistance(finalDest) > moveDistance)
                {
                    // Reduce to the maximum distance
                    finalDest = zoneManager->GetLinearPoint(source.x, source.y,
                        finalDest.x, finalDest.y, moveDistance, false, zone);
                }
            }
        }

        auto command = GetMoveCommand(eState, finalDest, 0.f, false);
        if(command)
        {
            aiState->QueueCommand(command);

            // If the entity has a respawn timeout, clear it if they can
            // reach the designated point which is in the spawn area or
            // they don't actually need to wander back
            if(aiState->GetDespawnTimeout() && (!wanderBack || canReach))
            {
                aiState->SetDespawnTimeout(0);
            }
        }
    }

    QueueWaitCommand(aiState, (uint32_t)(thinkSpeed * RNG(int32_t, 1, 3)));
}

uint8_t AIManager::SkillAdvance(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::MiSkillData>& skillData)
{
    auto zone = eState->GetZone();
    auto aiState = eState->GetAIState();

    int32_t targetEntityID = aiState->GetTargetEntityID();
    auto target = zone && targetEntityID > 0
        ? zone->GetActiveEntity(targetEntityID) : nullptr;
    if(!target)
    {
        return 1;
    }

    uint16_t normalRange = skillData->GetTarget()->GetRange();
    uint16_t maxTargetRange = (uint16_t)(400 + (normalRange * 10));

    Point src(eState->GetCurrentX(), eState->GetCurrentY());
    Point dest(target->GetCurrentX(), target->GetCurrentY());
    float targetDist = src.GetDistance(dest);

    // Move within range (keep a bit of a buffer for movement)
    if(targetDist > (maxTargetRange - 20.f))
    {
        // Either run up to max range if a ranged attack or to a close melee
        // distance
        float minDistance = normalRange ? (float)maxTargetRange - 20.f : 350.f;

        // Stop at de-aggro distance
        float maxDistance = aiState->GetAggroValue(0, false, 2000.f) * 1.5f;

        if(Chase(eState, targetEntityID, minDistance, maxDistance, false))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        // Nothing to do
        return 2;
    }
}

std::shared_ptr<ActiveEntityState> AIManager::Retarget(
    const std::shared_ptr<ActiveEntityState>& eState,
    uint64_t now, bool isNight)
{
    std::shared_ptr<ActiveEntityState> target;

    auto aiState = eState->GetAIState();

    auto zone = eState->GetZone();
    if(!zone)
    {
        return nullptr;
    }

    int32_t currentTarget = aiState->GetTargetEntityID();

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
            if(currentTarget > 0)
            {
                UpdateAggro(eState, -1);
            }

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
                        !entity->Ready() || entity->GetAIIgnored() ||
                        !entity->IsAlive();
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

            // If aggro limiting is enabled, remove targets based upon level
            // limit
            if(AggroLimitEnabled() && !aiState->GetIgnoreAggroLimit())
            {
                for(auto f : filtered)
                {
                    // Remove invalid pursuers first
                    for(int32_t aggroID : f->GetAggroIDs())
                    {
                        auto other = zone->GetActiveEntity(aggroID);
                        auto otherState = other
                            ? other->GetAIState() : nullptr;
                        if(!other || !other->Ready() || !otherState ||
                            otherState->GetTargetEntityID() != f->GetEntityID())
                        {
                            f->RemoveAggroIDs(aggroID);
                        }
                    }
                }

                // Do not pursue if they're already being pursued by too
                // many enemies (this is ignored for opponents)
                size_t max = 1;
                if(aggroLevelLimit >= 99)
                {
                    max = 2;
                }

                filtered.remove_if([max](
                    const std::shared_ptr<ActiveEntityState>& entity)
                    {
                        return entity->AggroIDsCount() >= max;
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

    int32_t newTarget = currentTarget;
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
            target = libcomp::Randomizer::GetEntry(possibleTargets);
        }

        newTarget = target ? target->GetEntityID() : -1;
    }
    else
    {
        newTarget = -1;
    }

    if(newTarget != currentTarget)
    {
        UpdateAggro(eState, newTarget);
    }

    return target;
}

void AIManager::RefreshSkillMap(const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<AIState>& aiState)
{
    if(aiState->GetSkillsMapped())
    {
        return;
    }

    bool isEnemy = eState->GetEnemyBase() != nullptr;

    AISkillMap_t skillMap;

    // AI entities have the concept of skills that can completely outclass
    // another. To be eligible to outclass another skill, the first attribute
    // it must have is that it must always be useable therefore it must have no
    // cost and no cooldown time. Beyond this the skill must be seen as one
    // that serves the same function but does so better than the other. This
    // adds both variance and predictability depending on the skill set and
    // can be disabled per entity at any time.
    std::set<uint32_t> outclassEligible;

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto skillManager = server->GetSkillManager();
    for(uint32_t skillID : eState->GetCurrentSkills())
    {
        auto skillData = definitionManager->GetSkillData(skillID);

        if(!SkillIsValid(skillData)) continue;

        int16_t targetType = -1;
        switch(skillData->GetRange()->GetValidType())
        {
        case objects::MiEffectiveRangeData::ValidType_t::ALLY:
        case objects::MiEffectiveRangeData::ValidType_t::SOURCE:
            targetType = (int16_t)AI_SKILL_TYPES_ALLY;
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
            // Determine if its a valid type
            int16_t skillType = -1;
            switch(skillData->GetBasic()->GetActionType())
            {
            case objects::MiSkillBasicData::ActionType_t::ATTACK:
            case objects::MiSkillBasicData::ActionType_t::RUSH:
            case objects::MiSkillBasicData::ActionType_t::SPIN:
                if(targetType == (int16_t)AI_SKILL_TYPES_ENEMY)
                {
                    skillType = (int16_t)AI_SKILL_TYPE_CLSR;
                }
                break;
            case objects::MiSkillBasicData::ActionType_t::SHOT:
            case objects::MiSkillBasicData::ActionType_t::RAPID:
                if(targetType == (int16_t)AI_SKILL_TYPES_ENEMY)
                {
                    skillType = (int16_t)AI_SKILL_TYPE_LNGR;
                }
                break;
            case objects::MiSkillBasicData::ActionType_t::SUPPORT:
                // Split based upon if the skill can heal or not
                switch(skillData->GetDamage()->GetBattleDamage()
                    ->GetFormula())
                {
                case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
                case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT:
                case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
                    if(skillData->GetDamage()->GetBattleDamage()
                        ->GetModifier1())
                    {
                        // MP only heal not supported
                        skillType = (int16_t)AI_SKILL_TYPE_HEAL;
                    }
                    break;
                default:
                    skillType = (int16_t)AI_SKILL_TYPE_SUPPORT;
                    break;
                }
                break;
            case objects::MiSkillBasicData::ActionType_t::GUARD:
            case objects::MiSkillBasicData::ActionType_t::COUNTER:
            case objects::MiSkillBasicData::ActionType_t::DODGE:
                skillType = (int16_t)AI_SKILL_TYPE_DEF;
                break;
            case objects::MiSkillBasicData::ActionType_t::TALK:
            case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
            case objects::MiSkillBasicData::ActionType_t::TAUNT:
            default:
                // Not supported
                break;
            }

            if(skillType != -1)
            {
                // Determine if costs are not valid for this entity or if any
                // cost exists at all. Even though the HP/MP cost will not
                // apply for the entire time the entity is active, since
                // percentage costs round up we should still have a clear
                // picture of if any costs exist.
                int32_t hpCost = 0, mpCost = 0;
                uint16_t bulletCost = 0;
                std::unordered_map<uint32_t, uint32_t> itemCosts;
                if(!skillManager->DetermineNormalCosts(eState, skillData,
                    hpCost, mpCost, bulletCost, itemCosts) || bulletCost ||
                    itemCosts.size() > 0)
                {
                    continue;
                }

                // Skill is valid. Calculate the weight (higher for more
                // preferable skills). Defense is hardly weighted and cannot
                // outclass anything else.
                int32_t weight = 2;
                if(skillType == AI_SKILL_TYPE_DEF)
                {
                    weight = 1;
                }
                else
                {
                    // Determine if the skill can outclass anything while
                    // calculating the weight
                    bool canOutclass = skillData->GetCondition()
                        ->GetCooldownTime() == 0;

                    // Having no charge time adds weight
                    if(!skillData->GetCast()->GetBasic()->GetChargeTime())
                    {
                        weight += aiState->GetSkillWeightCharge();
                    }

                    // Having no cost adds weight
                    if(hpCost == 0 && mpCost == 0)
                    {
                        weight += aiState->GetSkillWeightCost();
                    }
                    else
                    {
                        canOutclass = false;
                    }

                    // Heal skills are weighted more only when the heal threshold
                    // is active as they are not chosen otherwise
                    if(skillType == AI_SKILL_TYPE_HEAL)
                    {
                        weight += aiState->GetSkillWeightHeal();
                    }

                    // Ranged attacks add weight
                    if(skillData->GetTarget()->GetRange() > 0)
                    {
                        weight += aiState->GetSkillWeightRange();
                    }

                    if(canOutclass)
                    {
                        outclassEligible.insert(skillData->GetCommon()->GetID());
                    }
                }

                skillMap[(uint16_t)skillType].push_back(std::make_pair(
                    skillData, weight));
            }
        }
    }

    if(outclassEligible.size() > 0)
    {
        // Determine which skills are considered outclassed
        for(auto& pair : skillMap)
        {
            int16_t validType = -1;
            if(pair.first == AI_SKILL_TYPE_HEAL)
            {
                // Healing skills can only outclass healing skills,
                // regardless of if they can provide healing
                validType = AI_SKILL_TYPE_HEAL;
            }

            for(auto& weightPair : pair.second)
            {
                auto def1 = weightPair.first;
                if(outclassEligible.find(def1->GetCommon()->GetID()) ==
                    outclassEligible.end())
                {
                    continue;
                }

                // Compare against all other skills not already outclassed
                for(auto& pair2 : skillMap)
                {
                    std::list<std::shared_ptr<objects::MiSkillData>> others;
                    if(validType != -1 && (int16_t)pair2.first != validType)
                    {
                        continue;
                    }

                    for(auto& weightPair2 : pair2.second)
                    {
                        auto def2 = weightPair2.first;
                        if(def2 != def1 && !aiState->OutclassedSkillsContains(
                            def2->GetCommon()->GetID()))
                        {
                            others.push_back(def2);
                        }
                    }

                    for(auto def2 : others)
                    {
                        if(def2->GetRange()->GetAoeRange())
                        {
                            // Never outclass AoE
                            continue;
                        }
                        else if(def2->GetDamage()->AddStatusesCount())
                        {
                            // Never outclass status effect adding skills
                            continue;
                        }

                        auto damage1 = def1->GetDamage()->GetBattleDamage();
                        auto damage2 = def2->GetDamage()->GetBattleDamage();
                        if(damage1->GetFormula() != damage2->GetFormula())
                        {
                            // Damage formulas must match
                            continue;
                        }
                        else if(damage1->GetModifier1() &&
                            damage1->GetModifier1() < damage2->GetModifier1())
                        {
                            // Can't have lower damage potential (HP)
                            continue;
                        }
                        else if(!damage1->GetModifier1() &&
                            damage1->GetModifier2() &&
                            damage1->GetModifier2() < damage2->GetModifier2())
                        {
                            // If no HP damage, can't have lower damage
                            // potential (MP)
                            continue;
                        }

                        if((pair.first == AI_SKILL_TYPE_CLSR ||
                            pair.first == AI_SKILL_TYPE_LNGR) &&
                            def1->GetTarget()->GetRange() <=
                            def2->GetTarget()->GetRange())
                        {
                            // Can't be shorter or same range
                            continue;
                        }
                        else if(def1->GetCast()->GetBasic()->GetUseCount() <
                            def2->GetCast()->GetBasic()->GetUseCount())
                        {
                            // Don't outclass more uses
                            continue;
                        }

                        aiState->InsertOutclassedSkills(def2->GetCommon()
                            ->GetID());
                    }
                }
            }
        }
    }

    aiState->SetSkillMap(skillMap);
}

bool AIManager::SkillIsValid(
    const std::shared_ptr<objects::MiSkillData>& skillData)
{
    // Active skills only
    auto category = skillData->GetCommon()->GetCategory();
    if(category->GetMainCategory() != 1)
    {
        return false;
    }

    auto basic = skillData->GetBasic();

    // Ignore invalid family types (items, fusion)
    if(basic->GetFamily() == 2 || basic->GetFamily() == 5 ||
        basic->GetFamily() == 6)
    {
        return false;
    }

    uint16_t functionID = skillData->GetDamage()->GetFunctionID();
    if(functionID)
    {
        // Only certain function IDs are supported for general use
        const static std::set<uint16_t> supportedFIDs = {
                SVR_CONST.SKILL_ABS_DAMAGE,
                SVR_CONST.SKILL_DIGITALIZE_BREAK,
                SVR_CONST.SKILL_HP_DEPENDENT,
                SVR_CONST.SKILL_HP_MP_MIN,
                SVR_CONST.SKILL_LNC_DAMAGE,
                SVR_CONST.SKILL_PIERCE,
                SVR_CONST.SKILL_SLEEP_RESTRICTED,
                SVR_CONST.SKILL_STAT_SUM_DAMAGE,
                SVR_CONST.SKILL_STATUS_DIRECT,
                SVR_CONST.SKILL_STATUS_RANDOM,
                SVR_CONST.SKILL_STATUS_RANDOM2,
                SVR_CONST.SKILL_STATUS_SCALE,
                SVR_CONST.SKILL_SUICIDE,
            };

        if(supportedFIDs.find(functionID) == supportedFIDs.end())
        {
            // Not supported
            return false;
        }
    }

    return true;
}

bool AIManager::CanRetrySkill(const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::ActivatedAbility>& activated)
{
    if(!activated || !eState || eState->GetActivatedAbility() != activated)
    {
        return false;
    }

    switch(activated->GetErrorCode())
    {
    case (int8_t)SkillErrorCodes_t::ACTION_RETRY:
    case (int8_t)SkillErrorCodes_t::TOO_FAR:
        return true;
        break;
    case -1:
        // Can retry if no execution pending
        return !activated->GetExecutionRequestTime();
        break;
    default:
        break;
    }

    return false;
}

bool AIManager::PrepareSkillUsage(
    const std::shared_ptr<ActiveEntityState>& eState)
{
    auto zone = eState->GetZone();
    auto aiState = eState->GetAIState();
    auto cs = eState->GetCoreStats();
    if(!zone || !cs)
    {
        return false;
    }

    RefreshSkillMap(eState, aiState);

    int32_t targetID = aiState->GetTargetEntityID();
    auto target = targetID > 0 ? zone->GetActiveEntity(targetID) : nullptr;

    if(aiState->ActionOverridesKeyExists("prepareSkill"))
    {
        libcomp::String fOverride = aiState->GetActionOverrides(
            "prepareSkill");

        Sqrat::Function f(Sqrat::RootTable(aiState->GetScript()->GetVM()),
            fOverride.IsEmpty() ? "prepareSkill" : fOverride.C());

        auto scriptResult = !f.IsNull()
            ? f.Evaluate<int32_t>(eState, this, target) : 0;
        if(!scriptResult || *scriptResult == -1)
        {
            // Do not continue
            return false;
        }
        else if(*scriptResult == 0)
        {
            // Added by script
            return true;
        }
    }

    auto skillMap = aiState->GetSkillMap();

    bool canFight = target != nullptr;
    bool canHeal = (float)cs->GetHP() / (float)cs->GetMaxHP() <=
        (float)aiState->GetHealThreshold() * 0.01f;
    bool canSupport = skillMap[AI_SKILL_TYPE_SUPPORT].size() > 0;
    if(skillMap.size() > 0 && (canFight || canHeal || canSupport))
    {
        auto skillManager = mServer.lock()->GetSkillManager();

        uint16_t totalWeight = 0;
        std::list<AISkillWeight_t> weightedSkills;
        std::unordered_map<uint32_t, uint16_t> skillTypes;
        for(auto& pair : skillMap)
        {
            // Make sure the skill type is valid for the current state
            bool isHeal = pair.first == AI_SKILL_TYPE_HEAL;
            bool isSupport = pair.first == AI_SKILL_TYPE_SUPPORT;
            if((aiState->GetSkillSettings() & pair.first) != 0 &&
                ((isHeal && canHeal) ||
                (!isHeal && !isSupport && canFight) ||
                isSupport)) // No special requirement
            {
                for(auto& weight : pair.second)
                {
                    auto skillData = weight.first;
                    uint32_t skillID = skillData->GetCommon()->GetID();

                    // Remove outclassed skills
                    if(aiState->GetSkipOutclassedSkills() &&
                        aiState->OutclassedSkillsContains(skillID))
                    {
                        continue;
                    }

                    // Make sure its not cooling down or restricted
                    if(eState->SkillCooldownsKeyExists(skillData
                        ->GetBasic()->GetCooldownID()) ||
                        skillManager->SkillRestricted(eState, skillData))
                    {
                        continue;
                    }

                    // Make sure the target is valid
                    if((!isHeal && !isSupport && !skillManager
                        ->ValidateSkillTarget(eState, skillData, target)) ||
                        ((isHeal || isSupport) && !skillManager
                            ->ValidateSkillTarget(eState, skillData, eState)))
                    {
                        continue;
                    }

                    // Make sure costs can be paid (item/bullet costs won't
                    // be in the map at all)
                    int32_t hpCost = 0, mpCost = 0;
                    uint16_t bulletCost = 0;
                    std::unordered_map<uint32_t, uint32_t> itemCosts;
                    if(!skillManager->DetermineNormalCosts(eState, skillData,
                        hpCost, mpCost, bulletCost, itemCosts) ||
                        hpCost >= cs->GetHP() || mpCost > cs->GetMP())
                    {
                        continue;
                    }

                    weightedSkills.push_back(weight);
                    skillTypes[skillID] = pair.first;
                    totalWeight = (uint16_t)(totalWeight + weight.second);
                }
            }
        }

        if(weightedSkills.size() == 0)
        {
            // Can't use anything right now
            return false;
        }

        // Sort skills by weight (higher first)
        weightedSkills.sort([](const AISkillWeight_t& a,
            const AISkillWeight_t& b)
            {
                return a.second > b.second;
            });

        std::shared_ptr<objects::MiSkillData> skillData;

        // Pull a random number between 1 and the total weight and use the
        // first one that exceeds the value that we reduce by weight as we go
        uint16_t rVal = RNG(uint16_t, 1, totalWeight);
        for(auto& wSkill : weightedSkills)
        {
            if(wSkill.second >= rVal)
            {
                skillData = wSkill.first;
                break;
            }
            else
            {
                rVal = (uint16_t)(rVal - wSkill.second);
            }
        }

        if(skillData)
        {
            // The skill target is either the aggro target or the entity itself
            int32_t skillTargetID = targetID;
            switch(skillTypes[skillData->GetCommon()->GetID()])
            {
            case AI_SKILL_TYPE_HEAL:
            case AI_SKILL_TYPE_SUPPORT:
                skillTargetID = eState->GetEntityID();
                break;
            default:
                break;
            }

            switch(skillData->GetBasic()->GetActionType())
            {
            case objects::MiSkillBasicData::ActionType_t::SPIN:
            case objects::MiSkillBasicData::ActionType_t::RAPID:
            case objects::MiSkillBasicData::ActionType_t::COUNTER:
            case objects::MiSkillBasicData::ActionType_t::DODGE:
                // Move up to the target first
                SkillAdvance(eState, skillData);
                break;
            default:
                break;
            }

            auto cmd = std::make_shared<AIUseSkillCommand>(skillData,
                skillTargetID);
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

bool AIManager::AggroLimitEnabled()
{
    const static bool enabled = mServer.lock()->GetWorldSharedConfig()
        ->GetAggroLimit();
    return enabled;
}
