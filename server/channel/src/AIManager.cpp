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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include <ServerConstants.h>
#include <ServerDataManager.h>

// object Includes
#include <AILogicGroup.h>
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
#include <MiSkillSpecialParams.h>
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
#include "TokuseiManager.h"
#include "ZoneManager.h"

#define FOLLOW_DISTANCE_MAX (MAX_ENTITY_DRAW_DISTANCE * 0.66f)
#define FOLLOW_DISTANCE_FAR (MAX_ENTITY_DRAW_DISTANCE * 0.25f)
#define FOLLOW_DISTANCE_CLOSE (300.f)

using namespace channel;

std::unordered_map<std::string, std::shared_ptr<libhack::ScriptEngine>>
    AIManager::sPreparedScripts;

namespace libcomp {
template <>
BaseScriptEngine& BaseScriptEngine::Using<AIManager>() {
  if (!BindingExists("AIManager", true)) {
    Using<ActiveEntityState>();
    Using<Zone>();
    Using<libcomp::Randomizer>();

    Sqrat::Class<AIManager, Sqrat::NoConstructor<AIManager>> binding(
        mVM, "AIManager");
    binding.Func("QueueMoveCommand", &AIManager::QueueMoveCommand)
        .Func("QueueScriptCommand", &AIManager::QueueScriptCommand)
        .Func("QueueUseSkillCommand", &AIManager::QueueUseSkillCommand)
        .Func("QueueWaitCommand", &AIManager::QueueWaitCommand)
        .Func("Prepare", &AIManager::Prepare)
        .Func("StartEvent", &AIManager::StartEvent)
        .Func("UseDiasporaQuake", &AIManager::UseDiasporaQuake)
        .Func("Chase", &AIManager::Chase)
        .Func("Circle", &AIManager::Circle)
        .Func("ClearCommands", &AIManager::ClearCommands)
        .Func("Retreat", &AIManager::Retreat)
        .Func("Warp", &AIManager::Warp);

    Bind<AIManager>("AIManager", binding);
  }

  return *this;
}
}  // namespace libcomp

AIManager::AIManager() {}

AIManager::AIManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server) {}

AIManager::~AIManager() {}

bool AIManager::Prepare(const std::shared_ptr<ActiveEntityState>& eState,
                        const libcomp::String& aiType, uint16_t baseAIType) {
  if (!eState) {
    return false;
  }

  if (eState->GetAIState()) {
    // AI state already set
    return true;
  }

  auto aiState = std::make_shared<AIState>();
  eState->SetAIState(aiState);

  auto eBase = eState->GetEnemyBase();
  if (eBase) {
    if (eBase->GetSpawnLocation() || eBase->GetSpawnSpotID()) {
      // Default to wandering first
      aiState->SetStatus(AIStatus_t::WANDERING, true);
    }

    if (eBase->GetEncounterID()) {
      // Nothing with an encounter ID despawns when lost or we could
      // potentially break defeat actions by accident
      aiState->SetDespawnWhenLost(false);
    }
  }

  auto server = mServer.lock();
  auto serverDataManager = server->GetServerDataManager();

  auto demonData = eState->GetDevilData();
  auto spawn = eBase ? eBase->GetSpawnSource() : nullptr;
  if (spawn && spawn->GetCategory() == objects::Spawn::Category_t::BOSS) {
    // Determine if we should default to ignoring estoma
    const static bool bossIgnore =
        server->GetWorldSharedConfig()->GetAIEstomaBossIgnore();
    aiState->SetIgnoreEstoma(bossIgnore);
  }

  // Logic group 1 corresponds to idle/wander logic, group 2 is used
  // for aggro and combat, 3 is unknown and potentially unused as all
  // known instances of it match group 1. For our purposes custom AI
  // and group 2 are all that are actually needed.
  uint16_t logicGroupID =
      demonData ? demonData->GetAI()->GetLogicGroupIDs(1) : 0;
  if (spawn && spawn->GetLogicGroupID()) {
    logicGroupID = spawn->GetLogicGroupID();
  }

  auto logicGroup =
      logicGroupID ? serverDataManager->GetAILogicGroup(logicGroupID) : nullptr;
  if (!logicGroup && logicGroupID) {
    // Default to the type 0 default group if one exists
    logicGroup = serverDataManager->GetAILogicGroup(0);
  }

  if (!baseAIType) {
    // Use spawn, else demon definition
    baseAIType = spawn ? spawn->GetBaseAIType() : 0;

    if (!baseAIType && demonData) {
      baseAIType = demonData->GetAI()->GetType();
    }
  }

  auto aiData = server->GetDefinitionManager()->GetAIData(baseAIType);
  if (!aiData) {
    LogAIManagerError([baseAIType]() {
      return libcomp::String(
                 "Active entity with invalid base AI data value specified: "
                 "%1\n")
          .Arg(baseAIType);
    });
  } else {
    // Set all default values now so any call to the script prepare
    // function can modify them
    aiState->SetBaseAI(aiData);
    aiState->SetLogicGroup(logicGroup);
    aiState->SetAggroLevelLimit(aiData->GetAggroLevelLimit());
    aiState->SetThinkSpeed(aiData->GetThinkSpeed());
    aiState->SetDeaggroScale((uint8_t)aiData->GetDeaggroScale());
    aiState->SetStrikeFirst(aiData->GetStrikeFirst());
    aiState->SetNormalSkillUse(aiData->GetNormalSkillUse());
    aiState->SetAggroLimit((uint8_t)aiData->GetAggroLimit());
  }

  libcomp::String finalAIType = aiType;
  if (aiType.IsEmpty() && logicGroup) {
    finalAIType = logicGroup->GetDefaultScriptID();
  }

  std::shared_ptr<libhack::ScriptEngine> aiEngine;
  if (!finalAIType.IsEmpty()) {
    auto it = sPreparedScripts.find(finalAIType.C());
    if (it == sPreparedScripts.end()) {
      auto script = serverDataManager->GetAIScript(finalAIType);
      if (!script) {
        LogAIManagerError([finalAIType]() {
          return libcomp::String("AI type '%1' does not exist\n")
              .Arg(finalAIType);
        });

        return false;
      }

      aiEngine = std::make_shared<libhack::ScriptEngine>();
      aiEngine->Using<AIManager>();

      if (!aiEngine->Eval(script->Source)) {
        LogAIManagerError([finalAIType]() {
          return libcomp::String("AI type '%1' is not a valid AI script\n")
              .Arg(finalAIType);
        });

        return false;
      }

      if (!script->Instantiated) {
        sPreparedScripts[finalAIType.C()] = aiEngine;
      }
    } else {
      aiEngine = it->second;
    }

    Sqrat::Function f(Sqrat::RootTable(aiEngine->GetVM()), "prepare");
    if (!f.IsNull()) {
      auto result = !f.IsNull() ? f.Evaluate<int>(eState, this) : 0;
      if (!result || (*result != 0)) {
        LogAIManagerError([finalAIType]() {
          return libcomp::String("Failed to prepare AI type '%1'\n")
              .Arg(finalAIType);
        });

        return false;
      }
    }
  }

  aiState->SetScript(aiEngine);

  // The first command all AI perform is a wait command for a set time
  auto wait = GetWaitCommand(3000);
  wait->SetIgnoredDelay(true);
  aiState->QueueCommand(wait);

  aiState->ResetStatusChanged();

  return true;
}

void AIManager::UpdateActiveStates(const std::shared_ptr<Zone>& zone,
                                   uint64_t now, bool isNight) {
  std::list<std::shared_ptr<ActiveEntityState>> updated;
  for (auto eState : zone->GetEnemiesAndAllies()) {
    if (UpdateState(eState, now, isNight)) {
      updated.push_back(eState);
    }
  }

  // Update enemy states first
  if (updated.size() > 0) {
    auto zConnections = zone->GetConnectionList();
    RelativeTimeMap timeMap;
    for (auto entity : updated) {
      // Update the clients with what the entity is doing

      // Check if the entity's position or rotation has updated
      if (now == entity->GetOriginTicks()) {
        if (entity->IsMoving()) {
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
        } else if (entity->IsRotating()) {
          libcomp::Packet p;
          p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ROTATE);
          p.WriteS32Little(entity->GetEntityID());
          p.WriteFloat(entity->GetDestinationRotation());

          timeMap[p.Size()] = now;
          timeMap[p.Size() + 4] = entity->GetDestinationTicks();
          ChannelClientConnection::SendRelativeTimePacket(zConnections, p,
                                                          timeMap, true);
        } else {
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
    const std::shared_ptr<objects::MiSkillData>& skillData) {
  for (auto eState : entities) {
    auto aiState = eState->GetAIState();
    if (!aiState) continue;

    // If the current command is a skill command and it was cancelled
    // by the hit, remove it now so they can react faster later
    auto skillCmd = std::dynamic_pointer_cast<AIUseSkillCommand>(
        aiState->GetCurrentCommand());
    auto activated = skillCmd ? skillCmd->GetActivatedAbility() : nullptr;
    if (activated && activated->GetCancelled()) {
      aiState->PopCommand();
    }

    // If currently not acting, cancel now
    eState->RemoveStatusTimes(STATUS_RESTING);

    if (aiState->ActionOverridesKeyExists("combatSkillHit")) {
      libcomp::String fOverride = aiState->GetActionOverrides("combatSkillHit");

      LogAIManagerDebug([eState, fOverride]() {
        return libcomp::String("Executing combatSkillHit override for %1: %2\n")
            .Arg(eState->GetEntityLabel())
            .Arg(fOverride);
      });

      Sqrat::Function f(Sqrat::RootTable(aiState->GetScript()->GetVM()),
                        fOverride.IsEmpty() ? "combatSkillHit" : fOverride.C());

      auto scriptResult =
          !f.IsNull() ? f.Evaluate<int32_t>(eState, this, source, skillData)
                      : 0;
      if (!scriptResult || *scriptResult == 0) {
        // Do not continue
        return;
      }
    }

    if (eState->IsAlive() && !eState->SameFaction(source)) {
      // If the entity's current target is not the source of this skill,
      // there is a chance they will target them now (20% chance by
      // default)
      if (aiState->GetTargetEntityID() != source->GetEntityID() &&
          RNG(int32_t, 1, 10) <= 2) {
        UpdateAggro(eState, source->GetEntityID());
      }

      // If the entity is not aggro'd, clear all pending commands
      // and let them figure out if they need to resume later
      if (!aiState->IsAggro()) {
        aiState->ClearCommands();
      }
    }
  }
}

void AIManager::CombatSkillComplete(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<objects::MiSkillData>& skillData,
    const std::shared_ptr<ActiveEntityState>& target, bool hit) {
  auto aiState = eState->GetAIState();
  if (!aiState) {
    return;
  }

  // Multiple triggers in combat cause normal AI to reset and reorient
  // itself so they're not spamming skills non-stop
  bool reset = false;
  bool wait = true;

  bool normalProcesing = true;
  if (aiState->ActionOverridesKeyExists("combatSkillComplete")) {
    libcomp::String fOverride =
        aiState->GetActionOverrides("combatSkillComplete");

    LogAIManagerDebug([eState, fOverride]() {
      return libcomp::String(
                 "Executing combatSkillComplete override for %1: %2\n")
          .Arg(eState->GetEntityLabel())
          .Arg(fOverride);
    });

    Sqrat::Function f(
        Sqrat::RootTable(aiState->GetScript()->GetVM()),
        fOverride.IsEmpty() ? "combatSkillComplete" : fOverride.C());

    auto scriptResult =
        !f.IsNull() ? f.Evaluate<int32_t>(eState, this, activated, target, hit)
                    : 0;
    if (!scriptResult || *scriptResult == -1) {
      // Do not continue
      return;
    } else if (*scriptResult & 0x01) {
      // Skip normal processing
      normalProcesing = false;
      reset = (*scriptResult & 0x02) != 0;
      wait = (*scriptResult & 0x04) == 0;
    }
  }

  if (normalProcesing && target) {
    if (target->GetStatusTimes(STATUS_KNOCKBACK)) {
      // If the target is currently being knocked back (from this
      // skill or some other one), reset
      reset = true;
    } else if (eState->GetStatusTimes(STATUS_HIT_STUN)) {
      // If the source is hitstunned for whatever reason (counter
      // or guard for example)
      reset = true;
    } else if (!skillData->GetTarget()->GetRange() &&
               !skillData->GetCast()->GetBasic()->GetChargeTime() &&
               !skillData->GetCondition()->GetCooldownTime()) {
      // No charge, no cooldown, no range combat skills are typically
      // used in succession until knockback occurs (delayed by lockout
      // animation time)
      bool combo = false;

      if (target->GetStatusTimes(STATUS_HIT_STUN)) {
        // If the target is hitstunned, use again to attempt to combo
        // into knockback most of the time. Even if no combo occurs,
        // do not wait to use the next skill.
        combo = RNG(int32_t, 1, 10) <= 9;
        wait = false;
      } else {
        // If the target was still hit, repeat attack 30% of the
        // time, 10% if they were not hit
        combo = (hit && RNG(int32_t, 1, 10) <= 3) ||
                (!hit && RNG(int32_t, 1, 10) == 1);
      }

      if (combo && !aiState->GetCurrentCommand()) {
        auto cmd = std::make_shared<AIUseSkillCommand>(skillData,
                                                       target->GetEntityID());
        aiState->QueueCommand(cmd);
      } else {
        reset = true;
      }
    } else {
      // Check what kind of skill it was to decide how to handle
      switch (skillData->GetBasic()->GetActionType()) {
        case objects::MiSkillBasicData::ActionType_t::GUARD:
        case objects::MiSkillBasicData::ActionType_t::COUNTER:
        case objects::MiSkillBasicData::ActionType_t::DODGE:
          // Reset actions but do not wait
          reset = true;
          wait = false;
          break;
        default:
          // Other skills should be staggered unless more
          // executions exist
          reset = activated->GetExecuteCount() >= activated->GetMaxUseCount();
          break;
      }
    }
  }

  if (reset) {
    aiState->ClearCommands();
    if (wait) {
      auto logicGroup = aiState->GetLogicGroup();
      if (logicGroup && logicGroup->GetSkillResetStagger()) {
        QueueWaitCommand(aiState, logicGroup->GetSkillResetStagger());
      }
    }
  }
}

bool AIManager::QueueMoveCommand(
    const std::shared_ptr<ActiveEntityState>& eState, float x, float y,
    bool interrupt) {
  auto aiState = eState ? eState->GetAIState() : nullptr;
  if (!aiState) {
    return false;
  }

  auto cmdMove = GetMoveCommand(eState, Point(x, y));
  if (cmdMove) {
    aiState->QueueCommand(cmdMove, interrupt);

    return true;
  }

  return false;
}

bool AIManager::QueueUseSkillCommand(
    const std::shared_ptr<ActiveEntityState>& eState, uint32_t skillID,
    int32_t targetEntityID, bool advance) {
  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto skillData = definitionManager->GetSkillData(skillID);
  if (!skillData) {
    return false;
  }

  bool instantUse = skillData->GetBasic()->GetActivationType() ==
                    SkillActivationType_t::INSTANT;

  // Allow non-AI controlled entity if instantly using skill
  auto aiState = eState ? eState->GetAIState() : nullptr;
  if (!aiState && (advance || !instantUse)) {
    return false;
  }

  if (advance) {
    SkillAdvance(eState, skillData);
  } else if (instantUse) {
    // Do not actually queue since its an instant activation, use it now
    auto ctx = std::make_shared<SkillExecutionContext>();
    ctx->IgnoreAvailable = true;

    return server->GetSkillManager()->ActivateSkill(
        eState, skillID, targetEntityID, targetEntityID, ACTIVATION_TARGET,
        ctx);
  }

  auto skillCmd =
      std::make_shared<AIUseSkillCommand>(skillData, targetEntityID);
  aiState->QueueCommand(skillCmd);

  return true;
}

void AIManager::QueueScriptCommand(const std::shared_ptr<AIState>& aiState,
                                   const libcomp::String& functionName,
                                   bool interrupt) {
  if (aiState) {
    auto cmd = std::make_shared<AIScriptedCommand>(functionName);
    aiState->QueueCommand(cmd, interrupt);
  }
}

void AIManager::QueueWaitCommand(const std::shared_ptr<AIState>& aiState,
                                 uint32_t waitTime, bool interrupt) {
  if (aiState) {
    auto cmd = GetWaitCommand(waitTime);
    aiState->QueueCommand(cmd, interrupt);
  }
}

bool AIManager::StartEvent(const std::shared_ptr<ActiveEntityState>& eState,
                           const libcomp::String& eventID) {
  if (eState) {
    LogAIManagerDebug([eState, eventID]() {
      return libcomp::String("%1 is starting event: %2\n")
          .Arg(eState->GetEntityLabel())
          .Arg(eventID);
    });

    auto eventManager = mServer.lock()->GetEventManager();

    EventOptions options;
    options.AutoOnly = true;

    return eventManager->HandleEvent(nullptr, eventID, eState->GetEntityID(),
                                     eState->GetZone(), options);
  }

  return false;
}

void AIManager::UpdateAggro(const std::shared_ptr<ActiveEntityState>& eState,
                            int32_t targetID) {
  auto aiState = eState->GetAIState();
  auto zone = eState->GetZone();
  int32_t currentTargetID = aiState ? aiState->GetTargetEntityID() : -1;
  if (aiState && zone && currentTargetID != targetID) {
    if (currentTargetID > 0) {
      // Clear old aggro
      auto oldTarget = zone->GetActiveEntity(currentTargetID);
      if (oldTarget) {
        LogAIManagerDebug([eState, oldTarget]() {
          return libcomp::String("%1 loses aggro on %2.\n")
              .Arg(eState->GetEntityLabel())
              .Arg(oldTarget->GetEntityLabel());
        });

        AddRemoveAggro(oldTarget, eState->GetEntityID(), true);
      }
    }

    if (targetID > 0) {
      // Set aggro
      if (!aiState->IsAggro()) {
        aiState->SetStatus(AIStatus_t::AGGRO);
      }

      auto newTarget = zone->GetActiveEntity(targetID);
      if (newTarget) {
        LogAIManagerDebug([eState, newTarget]() {
          return libcomp::String("%1 aggros on %2.\n")
              .Arg(eState->GetEntityLabel())
              .Arg(newTarget->GetEntityLabel());
        });

        AddRemoveAggro(newTarget, eState->GetEntityID(), false);
      }

      // If current command targets current target, switch to new
      auto cmd = aiState->GetCurrentCommand();
      if (cmd && cmd->GetTargetEntityID() == currentTargetID) {
        cmd->SetTargetEntityID(targetID);
      }
    } else if (currentTargetID > 0) {
      // Remove all commands that targeted the old entity
      auto cmd = aiState->GetCurrentCommand();
      while (cmd && cmd->GetTargetEntityID() == currentTargetID) {
        aiState->PopCommand();
        cmd = aiState->GetCurrentCommand();
      }
    }

    aiState->SetTargetEntityID(targetID);

    if (eState->GetEnemyBase()) {
      // Enemies and allies telegraph who they are targeting by facing them
      libcomp::Packet p;
      p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENEMY_ACTIVATED);
      p.WriteS32Little(eState->GetEntityID());
      p.WriteS32Little(aiState->GetTargetEntityID());

      ChannelClientConnection::BroadcastPacket(zone->GetConnectionList(), p);
    }
  }
}

bool AIManager::ClearCommands(
    const std::shared_ptr<ActiveEntityState>& eState) {
  auto aiState = eState ? eState->GetAIState() : nullptr;
  if (!aiState) {
    return false;
  }

  aiState->ClearCommands();

  return true;
}

bool AIManager::UseDiasporaQuake(
    const std::shared_ptr<ActiveEntityState>& source, uint32_t skillID,
    float delay) {
  auto zone = source ? source->GetZone() : nullptr;
  if (!zone || !source->GetAIState()) {
    LogAIManagerError([skillID]() {
      return libcomp::String(
                 "Attempted to use a Diaspora quake skill from an invalid "
                 "entity or zone: %1\n")
          .Arg(skillID);
    });

    return false;
  } else if (zone->GetInstanceType() != InstanceType_t::DIASPORA) {
    LogAIManagerError([skillID]() {
      return libcomp::String(
                 "Attempted to use a Diaspora quake skill outside of a "
                 "Diaspora instance: %1\n")
          .Arg(skillID);
    });

    return false;
  }

  auto server = mServer.lock();

  auto validSkillIDs = server->GetDefinitionManager()->GetFunctionIDSkills(
      SVR_CONST.SKILL_DIASPORA_QUAKE);
  if (validSkillIDs.find(skillID) == validSkillIDs.end()) {
    LogAIManagerError([skillID]() {
      return libcomp::String(
                 "Attempted to use invalid Diaspora quake skill: %1\n")
          .Arg(skillID);
    });

    return false;
  }

  libcomp::Packet start;
  start.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DIASPORA_QUAKE);
  start.WriteS32Little(0);  // Not ending
  start.WriteFloat(delay);
  start.WriteS32Little(source->GetEntityID());

  server->GetZoneManager()->BroadcastPacket(zone, start);

  // Calculate use time to the half second
  uint64_t useTime =
      ChannelServer::GetServerTime() + ((uint64_t)(delay * 10.f) * 100000ULL);

  // Schedule end signifier and skill usage
  server->ScheduleWork(
      useTime,
      [](const std::shared_ptr<ChannelServer> pServer,
         const std::shared_ptr<ActiveEntityState> pSource, uint32_t pSkillID) {
        auto pZone = pSource->GetZone();
        if (!pZone) {
          return;
        }

        libcomp::Packet stop;
        stop.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_DIASPORA_QUAKE);
        stop.WriteS32Little(1);  // Ending
        stop.WriteFloat(0.f);
        stop.WriteS32Little(pSource->GetEntityID());

        pServer->GetZoneManager()->BroadcastPacket(pZone, stop);

        // Perform instant activation usage if the source is still alive
        auto pSkillManager = pServer->GetSkillManager();
        if (pSkillManager && pSource->IsAlive()) {
          pSkillManager->ActivateSkill(pSource, pSkillID, 0, 0,
                                       ACTIVATION_NOTARGET);
        }
      },
      server, source, skillID);

  return true;
}

bool AIManager::Warp(const std::shared_ptr<ActiveEntityState>& eState,
                     uint32_t spotID, float x, float y, float rot) {
  auto zone = eState ? eState->GetZone() : nullptr;
  auto aiState = eState ? eState->GetAIState() : nullptr;
  if (!zone || !aiState) {
    return false;
  }

  auto zoneManager = mServer.lock()->GetZoneManager();
  if (spotID && !zoneManager->GetSpotPosition(zone->GetDynamicMapID(), spotID,
                                              x, y, rot)) {
    return false;
  }

  zoneManager->Warp(nullptr, eState, x, y, rot);

  return true;
}

bool AIManager::Chase(const std::shared_ptr<ActiveEntityState>& eState,
                      int32_t targetEntityID, float minDistance,
                      float maxDistance, bool interrupt, bool allowLazy) {
  auto zone = eState ? eState->GetZone() : nullptr;
  auto aiState = eState ? eState->GetAIState() : nullptr;
  if (!zone || !aiState) {
    return false;
  }

  auto targetEntity = zone->GetActiveEntity(targetEntityID);
  if (!targetEntity || eState == targetEntity) {
    return false;
  }

  Point src(eState->GetCurrentX(), eState->GetCurrentY());
  Point dest(targetEntity->GetCurrentX(), targetEntity->GetCurrentY());

  auto server = mServer.lock();
  auto zoneManager = server->GetZoneManager();

  auto point = zoneManager->GetLinearPoint(src.x, src.y, dest.x, dest.y,
                                           src.GetDistance(dest), false);

  auto cmd = GetMoveCommand(eState, point, minDistance, true, allowLazy);
  if (cmd) {
    cmd->SetTargetEntityID(targetEntityID);
    cmd->SetTargetDistance(minDistance, true);
    cmd->SetTargetDistance(maxDistance, false);
    aiState->QueueCommand(cmd, interrupt);

    return true;
  } else {
    return false;
  }
}

bool AIManager::Retreat(const std::shared_ptr<ActiveEntityState>& eState,
                        float x, float y, float distance, bool interrupt) {
  auto zone = eState ? eState->GetZone() : nullptr;
  auto aiState = eState ? eState->GetAIState() : nullptr;
  if (!zone || !aiState || !eState->CanMove()) {
    return false;
  }

  auto server = mServer.lock();
  auto zoneManager = server->GetZoneManager();

  Point src(eState->GetCurrentX(), eState->GetCurrentY());
  Point target(x, y);

  Point retreatPoint = zoneManager->GetLinearPoint(
      target.x, target.y, src.x, src.y, distance, false, zone);
  if (retreatPoint.GetDistance(target) > src.GetDistance(target)) {
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
                       float x, float y, bool interrupt, float distance) {
  auto zone = eState ? eState->GetZone() : nullptr;
  auto aiState = eState ? eState->GetAIState() : nullptr;
  if (!zone || !aiState || !eState->CanMove()) {
    return false;
  }

  auto server = mServer.lock();
  auto zoneManager = server->GetZoneManager();

  Point src(eState->GetCurrentX(), eState->GetCurrentY());
  Point target(x, y);

  std::list<Point> pathing;

  // Distance from the target first
  Point start = zoneManager->GetLinearPoint(target.x, target.y, src.x, src.y,
                                            distance, false, zone);
  if (start != src) {
    // If we can't get to the start point in a straight line
    // quit now as the move should not be that complex
    Point collidePoint;
    if (zone->Collides(Line(start, src), collidePoint)) {
      return false;
    }

    pathing.push_back(start);
  }

  // Rotate the starting point around the target for the second
  // (or third) point
  int32_t pointCount = RNG(int32_t, 1, 2);
  bool invert = RNG(int32_t, 1, 2) == 1;
  for (int32_t i = 0; i < pointCount; i++) {
    Point prev = pathing.size() > 0 ? pathing.back() : src;
    Point p = zoneManager->RotatePoint(prev, target, invert ? -0.52f : 0.52f);

    // Check for collision
    Point pFinal = zoneManager->GetLinearPoint(
        prev.x, prev.y, p.x, p.y, prev.GetDistance(p), false, zone);
    if (pFinal != p && i == 0) {
      // Hit something, try the other direction (only once)
      pointCount++;
      invert = !invert;
    } else if (pFinal == prev) {
      // Can't move further
      break;
    } else {
      // Add the point
      pathing.push_back(pFinal);
    }
  }

  if (pathing.size() > 0) {
    auto cmd = std::make_shared<AIMoveCommand>();
    cmd->SetPathing(pathing);
    aiState->QueueCommand(cmd, interrupt);

    return true;
  }

  return false;
}

bool AIManager::UpdateState(const std::shared_ptr<ActiveEntityState>& eState,
                            uint64_t now, bool isNight) {
  eState->RefreshCurrentPosition(now);

  auto aiState = eState->GetAIState();
  if (!aiState) {
    return false;
  }

  uint64_t despawnTimout = aiState->GetDespawnTimeout();
  if (despawnTimout && despawnTimout <= now) {
    // Despawn it and quit
    auto zone = eState->GetZone();
    if (zone) {
      zone->MarkDespawn(eState->GetEntityID());
    }

    aiState->SetDespawnTimeout(0);
    return false;
  }

  if (aiState->IsIdle() && !aiState->ActionOverridesKeyExists("idle") &&
      !aiState->HasFollowTarget() && !aiState->GetCurrentCommand()) {
    // Nothing to do
    return false;
  }

  eState->ExpireStatusTimes(now);

  bool canAct =
      !eState->StatusTimesKeyExists(STATUS_RESTING) && eState->CanAct();
  bool noTargetState = aiState->IsIdle() || aiState->IsFollowing();

  // If no target exists and the next target time has passed, search now
  if (canAct && !noTargetState && aiState->GetTargetEntityID() <= 0 &&
      eState->GetOpponentIDs().size() == 0 &&
      (!aiState->GetNextTargetTime() || aiState->GetNextTargetTime() <= now)) {
    // If still in the ignore state, fail to target until it expires
    auto newTarget = !eState->StatusTimesKeyExists(STATUS_IGNORE)
                         ? Retarget(eState, now, isNight)
                         : nullptr;
    if (newTarget) {
      // Stop movement and clear existing commands but continue the current
      // wait command if active
      eState->Stop(now);

      auto current = aiState->GetCurrentCommand();
      aiState->ClearCommands();

      if (current && current->GetType() == AICommandType_t::NONE &&
          eState->StatusTimesKeyExists(STATUS_WAITING)) {
        aiState->QueueCommand(current);
      }

      return true;
    } else {
      // Push the next target time based on think speed and move on
      aiState->SetNextTargetTime(now +
                                 (uint64_t)(aiState->GetThinkSpeed() * 1000));
    }
  }

  // If the entity cannot act or is waiting, stop if moving and quit here
  if (!canAct || eState->StatusTimesKeyExists(STATUS_WAITING)) {
    if (eState->IsMoving() && !eState->GetStatusTimes(STATUS_KNOCKBACK)) {
      eState->Stop(now);
      return true;
    } else if (!canAct) {
      return false;
    } else if (!aiState->HasFollowTarget() || !aiState->IsWandering()) {
      // Do not actually wait (here) if wandering with an entity to
      // follow
      return false;
    }
  }

  // Entity cannot do anything if still affected by skill lockout
  if (eState->GetStatusTimes(STATUS_LOCKOUT)) {
    return false;
  }

  if (aiState->StatusChanged()) {
    // Do not clear actions if going from aggro to a combat state
    if (!(aiState->InCombat() &&
          aiState->GetPreviousStatus() == AIStatus_t::AGGRO)) {
      auto cmd = aiState->GetCurrentCommand();
      aiState->ClearCommands();

      // If the current command was a use skill, let it complete and
      // fail if it needs to
      if (cmd && cmd->GetType() == AICommandType_t::USE_SKILL) {
        aiState->QueueCommand(cmd);
      }
    }

    auto activated = eState->GetActivatedAbility();
    if (activated) {
      bool cancelActivatedSkill = false;

      switch (aiState->GetStatus()) {
        case AIStatus_t::IDLE:
        case AIStatus_t::WANDERING:
        case AIStatus_t::FOLLOWING:
          if (aiState->GetPreviousStatus() == AIStatus_t::AGGRO ||
              aiState->GetPreviousStatus() == AIStatus_t::COMBAT ||
              aiState->GetPreviousStatus() == AIStatus_t::ENRAGED) {
            // Leftover combat skill, cancel it
            cancelActivatedSkill = true;
          }
          break;
        case AIStatus_t::ENRAGED: {
          auto skillActionType =
              activated->GetSkillData()->GetBasic()->GetActionType();
              
          if (skillActionType == SkillActionType_t::COUNTER ||
              skillActionType == SkillActionType_t::DODGE ||
              skillActionType == SkillActionType_t::GUARD) {
            // Target has been taunted, defensive skills will be canceled
            cancelActivatedSkill = true;
          }
          break;
        }
        default:
          break;
      }

      if (cancelActivatedSkill) {
        mServer.lock()->GetSkillManager()->CancelSkill(
            eState, activated->GetActivationID());
      }
    }

    // If the status has been changed to or from ENRAGED, refresh the skill map so that
    // appropriate skills are selectable.
    if (aiState->IsEnraged() ||
        aiState->GetPreviousStatus() == AIStatus_t::ENRAGED) {
      aiState->ResetSkillsMapped();
      RefreshSkillMap(eState, aiState);
    }

    aiState->ResetStatusChanged();
  }

  if (!aiState->GetCurrentCommand()) {
    // Check for overrides first
    libcomp::String actionName;
    switch (aiState->GetStatus()) {
      case AIStatus_t::IDLE:
        actionName = "idle";
        break;
      case AIStatus_t::WANDERING:
        actionName = "wander";
        break;
      case AIStatus_t::FOLLOWING:
        actionName = "follow";
        break;
      case AIStatus_t::AGGRO:
        actionName = "aggro";
        break;
      case AIStatus_t::COMBAT:
        actionName = "combat";
        break;
      case AIStatus_t::ENRAGED:
        actionName = "enraged";
        break;
      default:
        break;
    }

    if (aiState->ActionOverridesKeyExists(actionName)) {
      libcomp::String fOverride = aiState->GetActionOverrides(actionName);
      if (!fOverride.IsEmpty()) {
        // Queue the overridden function
        QueueScriptCommand(aiState, fOverride);
      } else {
        // Run the function with the action name
        int32_t result = 0;
        if (ExecuteScriptFunction(eState, actionName.C(), now, result)) {
          if (result == -1) {
            // Erroring or skipping the action
            return false;
          } else if (result == 1) {
            // Direct entity update, communicate the results
            return true;
          }
        }
      }
    }

    // If no commands were added by the script, use the normal logic
    if (!aiState->GetCurrentCommand()) {
      switch (eState->GetEntityType()) {
        case EntityType_t::ENEMY:
        case EntityType_t::ALLY:
          return UpdateEnemyState(eState, eState->GetEnemyBase(), now, isNight);
        default:
          break;
      }
    }
  }

  auto cmd = aiState->GetCurrentCommand();
  if (cmd) {
    if (!cmd->GetStartTime()) {
      cmd->Start();
      if (cmd->GetDelay() > 0) {
        uint64_t statusTime = now + cmd->GetDelay();
        eState->SetStatusTimes(STATUS_WAITING, statusTime);

        if (cmd->GetIgnoredDelay() && eState->GetStatusTimes(STATUS_IGNORE) &&
            eState->GetStatusTimes(STATUS_IGNORE) < statusTime) {
          eState->SetStatusTimes(STATUS_IGNORE, statusTime);
        }

        return false;
      }
    }

    auto zone = eState->GetZone();
    if (!zone) {
      return false;
    }

    std::shared_ptr<ActiveEntityState> targetEntity;
    if (cmd->GetTargetEntityID() > 0) {
      targetEntity = zone->GetActiveEntity(cmd->GetTargetEntityID());
      if (targetEntity) {
        targetEntity->RefreshCurrentPosition(now);
      }
    }

    switch (cmd->GetType()) {
      case AICommandType_t::MOVE:
        if (eState->CanMove()) {
          Point src(eState->GetCurrentX(), eState->GetCurrentY());

          auto cmdMove = std::dynamic_pointer_cast<AIMoveCommand>(cmd);

          bool moving = eState->IsMoving();
          float minDistance = cmdMove->GetTargetDistance(true);
          if (moving || minDistance) {
            float maxDistance = cmdMove->GetTargetDistance(false);
            if (targetEntity && (minDistance || maxDistance)) {
              // Make sure the destination is still close to the
              // target
              Point tPoint(targetEntity->GetCurrentX(),
                           targetEntity->GetCurrentY());
              if (moving && maxDistance &&
                  tPoint.GetDistance(src) >= maxDistance) {
                // Quit movement
                targetEntity->Stop(now);
                aiState->PopCommand();
              } else if (moving && minDistance &&
                         tPoint.GetDistance(src) <= minDistance) {
                // Movement is done, stop now
                targetEntity->Stop(now);
                aiState->PopCommand();
              } else if (minDistance) {
                Point endPoint;
                if (cmdMove->GetEndDestination(endPoint) &&
                    floor(tPoint.GetDistance(endPoint)) > minDistance) {
                  // End point is no longer valid, repath
                  auto cmdNew =
                      GetMoveCommand(eState, tPoint, minDistance, true, true);
                  if (cmdNew && cmdMove->GetEndDestination(endPoint)) {
                    cmdMove->SetPathing(cmdNew->GetPathing());
                  } else {
                    aiState->PopCommand();
                  }

                  return false;
                }
              }
            }

            if (moving) {
              return false;
            }
          }

          // Move to the first point in the path that is not the
          // entity's current position
          Point dest;
          while (cmdMove->GetCurrentDestination(dest)) {
            if (dest != src) {
              Move(eState, dest, now);
              return true;
            } else if (!cmdMove->SetNextDestination()) {
              break;
            }
          }

          aiState->PopCommand(cmdMove);
        } else {
          // If the entity can't move, clear the queued commands and let
          // it figure out what to do instead
          aiState->ClearCommands();
        }
        break;
      case AICommandType_t::USE_SKILL: {
        // Do nothing if hit stunned or still charging
        if (eState->GetStatusTimes(STATUS_HIT_STUN) ||
            eState->GetStatusTimes(STATUS_KNOCKBACK) ||
            eState->GetStatusTimes(STATUS_CHARGING)) {
          return false;
        }

        auto cmdSkill = std::dynamic_pointer_cast<AIUseSkillCommand>(cmd);

        auto activated = cmdSkill->GetActivatedAbility();
        if (activated && eState->GetActivatedAbility() == activated &&
            activated->GetErrorCode() == -1) {
          // Check the state of the current activated skill
          if (activated->GetExecutionRequestTime() &&
              !activated->GetExecutionTime()) {
            // Waiting on skill to start
            return false;
          }

          if (activated->GetHitTime() && activated->GetHitTime() < now) {
            // Waiting on skill hit
            return false;
          }
        }

        auto server = mServer.lock();
        auto skillManager = server->GetSkillManager();

        if (cmdSkill->GetTargetEntityID() > 0) {
          if (!targetEntity || !targetEntity->IsAlive() ||
              targetEntity->GetAIIgnored()) {
            // Target invalid or dead, cancel the skill and move on
            if (activated) {
              LogAIManagerDebug([eState, activated, cmdSkill]() {
                return libcomp::String(
                           "%1 canceling skill %2 on no longer valid target: "
                           "%3\n")
                    .Arg(eState->GetEntityLabel())
                    .Arg(activated->GetSkillData()->GetCommon()->GetID())
                    .Arg(cmdSkill->GetTargetEntityID());
              });

              skillManager->CancelSkill(eState, activated->GetActivationID());
            }

            // Not valid
            aiState->PopCommand(cmdSkill);
            return false;
          }
        }

        if (targetEntity) {
          targetEntity->ExpireStatusTimes(now);

          uint64_t kbTime = targetEntity->GetStatusTimes(STATUS_KNOCKBACK);
          if (CombatStaggerEnabled() && kbTime) {
            // Delay execution or activation
            QueueWaitCommand(aiState, (uint32_t)((kbTime - now) / 1000 + 500),
                             true);
            return false;
          }
        }

        if (activated) {
          // Execute the skill
          if (!skillManager->ExecuteSkill(eState, activated->GetActivationID(),
                                          activated->GetTargetObjectID()) &&
              eState->GetActivatedAbility() == activated) {
            if (!CanRetrySkill(eState, activated)) {
              skillManager->CancelSkill(eState, activated->GetActivationID());
            }
          }
        } else {
          // Activate the skill
          skillManager->ActivateSkill(
              eState, cmdSkill->GetSkillID(), cmdSkill->GetTargetEntityID(),
              cmdSkill->GetTargetEntityID(), ACTIVATION_TARGET);
        }

        aiState->PopCommand(cmdSkill);
      } break;
      case AICommandType_t::SCRIPTED: {
        // Execute a custom scripted command
        auto cmdScript = std::dynamic_pointer_cast<AIScriptedCommand>(cmd);

        LogAIManagerDebug([eState, cmdScript]() {
          return libcomp::String("%1 executing custom script: %2\n")
              .Arg(eState->GetEntityLabel())
              .Arg(cmdScript->GetFunctionName());
        });

        int32_t result = 0;
        if (!ExecuteScriptFunction(eState, cmdScript->GetFunctionName(), now,
                                   result)) {
          // Pop the command and move on
          aiState->PopCommand();
        } else if (result == 0) {
          return false;
        } else {
          aiState->PopCommand();
          if (result == 1) {
            return true;
          }
        }
      } break;
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
    bool isNight) {
  auto aiState = eState->GetAIState();

  // Check if we need to pursue the follow target
  if (aiState->HasFollowTarget() && Follow(eState, now)) {
    return false;
  }

  // If we get here and are still idle, stop now
  if (aiState->IsIdle()) {
    return false;
  }

  if (aiState->IsWandering() && eBase) {
    // If we're wandering but have opponents (typically from being hit)
    // try to target one of them and stop here if we do
    if (eState->GetOpponentIDs().size() > 0 && Retarget(eState, now, isNight)) {
      return false;
    }

    Wander(eState, eBase);
    return true;
  }

  auto zone = eState->GetZone();
  if (!zone) {
    return false;
  }

  int32_t targetEntityID = aiState->GetTargetEntityID();
  auto target =
      targetEntityID > 0 ? zone->GetActiveEntity(targetEntityID) : nullptr;
  if (!target || !target->IsAlive() || !target->Ready() ||
      target->GetAIIgnored()) {
    // Try to find another target
    target = Retarget(eState, now, isNight);

    if (!target) {
      // Reset to default state and quit
      UpdateAggro(eState, -1);
      aiState->SetStatus(aiState->GetDefaultStatus());
      return false;
    }
  }

  float targetDist = 0.f, targetX = 0.f, targetY = 0.f;
  if (target) {
    target->RefreshCurrentPosition(now);
    targetX = target->GetCurrentX();
    targetY = target->GetCurrentY();
    targetDist = eState->GetDistance(targetX, targetY);
  }

  auto server = mServer.lock();

  bool targetChanged = false;
  float deaggroDist = aiState->GetDeaggroDistance(isNight);
  if (deaggroDist && targetDist >= deaggroDist) {
    // De-aggro on that one target and find a new one
    server->GetCharacterManager()->AddRemoveOpponent(false, eState, target);

    target = Retarget(eState, now, isNight);
    targetChanged = true;
  }

  auto activated = eState->GetActivatedAbility();
  if (!target) {
    // No target could be found, stop combat and quit
    if (activated) {
      server->GetSkillManager()->CancelSkill(eState,
                                             activated->GetActivationID());
    }

    return false;
  } else if (targetChanged) {
    target->RefreshCurrentPosition(now);
    targetX = target->GetCurrentX();
    targetY = target->GetCurrentY();
    targetDist = eState->GetDistance(targetX, targetY);
  }

  targetEntityID = target->GetEntityID();

  bool skillActivationWait = false;
  if (activated) {
    if (eState->GetStatusTimes(STATUS_CHARGING)) {
      // Let charging finish
      return false;
    }

    auto logicGroup = aiState->GetLogicGroup();
    uint64_t minCharge =
        activated->GetChargedTime() +
        (uint64_t)(logicGroup ? logicGroup->GetPostChargeStagger() : 0) *
            (uint64_t)1000ULL;
    if (now < minCharge) {
      // Minimum charge stagger has not passed
      return false;
    }

    if (activated->GetExecutionRequestTime() && activated->GetExecutionTime() &&
        activated->GetErrorCode() == -1) {
      // Skill mid execution
      return false;
    }

    // If a skill has been charged but for less than the max wait time, do
    // not decide to do something else by default (ex: rapid/counter)
    skillActivationWait =
        now < (activated->GetChargedTime() + (uint64_t)AI_MAX_CHARGE_WAIT);

    bool cancelAndReset = false;
    if (!CanRetrySkill(eState, activated)) {
      // Somehow we have an error
      cancelAndReset = true;
    } else if (!skillActivationWait && RNG(uint16_t, 1, 2) == 1) {
      // Chance to cancel and reset if we've waited for a while
      cancelAndReset = true;
    } else if (aiState->GetFollowEntityID() > 0 &&
               eState->GetDistance(zone->GetActiveEntity(
                   aiState->GetFollowEntityID())) > FOLLOW_DISTANCE_MAX) {
      // Cancel to pursue follow target
      server->GetSkillManager()->CancelSkill(eState,
                                             activated->GetActivationID());
      return false;
    } else if (activated->GetSkillData()->GetBasic()->GetActivationType() ==
                   SkillActivationType_t::ON_HIT &&
               target && aiState->GetDefensiveDistance() > 0.f) {
      // If we have a skill waiting on being hit, either circle the
      // target or just wait
      if (target && aiState->GetDefensiveDistance() > 0.f) {
        // Circle the target
        Circle(eState, targetX, targetY, true, aiState->GetDefensiveDistance());
      }

      // Wait no matter what
      int32_t thinkSpeedAdjust =
          aiState->GetThinkSpeed() && aiState->GetThinkSpeed() > 2000
              ? aiState->GetThinkSpeed()
              : 2000;
      QueueWaitCommand(aiState, (uint32_t)thinkSpeedAdjust);
      return false;
    }

    if (cancelAndReset) {
      server->GetSkillManager()->CancelSkill(eState,
                                             activated->GetActivationID());
      activated = nullptr;
    }
  }

  if (activated) {
    auto skillManager = server->GetSkillManager();

    // Skill charged, cancel, execute or move within range
    int64_t activationTarget = activated->GetTargetObjectID();
    if (activationTarget > 0 && targetEntityID != (int32_t)activationTarget &&
        eState->GetEntityID() != (int32_t)activationTarget) {
      // Target changed
      skillManager->TargetSkill(eState, targetEntityID);
      return false;
    }

    auto skillData = activated->GetSkillData();

    // Move forward if needed and execute when close enough
    uint8_t moveResponse = SkillAdvance(eState, skillData);
    if (moveResponse == 0) {
      // Moving forward, stop here
      return false;
    } else if (moveResponse == 1) {
      // Could not move forward, either continue waiting or cancel
      if (skillActivationWait) {
        return false;
      } else {
        skillManager->CancelSkill(eState, activated->GetActivationID());
      }
    } else if (CanRetrySkill(eState, activated)) {
      auto logicGroup = aiState->GetLogicGroup();
      uint32_t retryStagger =
          logicGroup ? logicGroup->GetSkillRetryStagger() : 0;
      if (retryStagger &&
          activated->GetErrorCode() ==
              (int8_t)SkillErrorCodes_t::ACTION_RETRY &&
          activated->GetActivationTime()) {
        // Clear the activation time and wait
        activated->SetActivationTime(0);
        QueueWaitCommand(aiState, retryStagger, true);
      } else {
        // Execute the skill
        auto cmd = std::make_shared<AIUseSkillCommand>(activated);
        aiState->QueueCommand(cmd);
      }
    }
  } else {
    uint32_t waitTime = 0;

    int16_t waitChance = (int16_t)(100.f * aiState->GetAggression());
    if (RNG(int16_t, 1, waitChance > 25 ? waitChance : 25) <= 20) {
      // 20% chance to just wait (lower for high aggression)
      waitTime = 1000;
    } else if (eState->CurrentSkillsCount() > 0) {
      // If aggro limit is enabled and the target is being knocked back
      // wait instead to stagger attacks
      if (target) {
        target->ExpireStatusTimes(now);
        uint64_t kbTime = target->GetStatusTimes(STATUS_KNOCKBACK);
        if (CombatStaggerEnabled() && kbTime) {
          waitTime = (uint32_t)((kbTime - now) / 1000 + 500);
        }
      }

      if (!waitTime) {
        // All normal movement is based off skill usage, determine
        // which skill to use next
        if (!PrepareSkillUsage(eState)) {
          // No skill can be used, drop aggro
          UpdateAggro(eState, -1);

          // Run away if defensive distance specified
          if (aiState->GetDefensiveDistance() > 0.f) {
            Retreat(eState, targetX, targetY, aiState->GetDefensiveDistance(),
                    true);
          }
        } else if (!aiState->GetCurrentCommand()) {
          // Nothing was queued, wait instead
          waitTime = (uint32_t)aiState->GetThinkSpeed();
        }
      }
    } else {
      // No skills exist, drop aggro
      UpdateAggro(eState, -1);
    }

    if (waitTime) {
      QueueWaitCommand(aiState, waitTime);
    }
  }

  return false;
}

bool AIManager::Follow(const std::shared_ptr<ActiveEntityState>& eState,
                       uint64_t now) {
  auto aiState = eState->GetAIState();
  auto zone = eState->GetZone();
  if (!zone || eState->GetActivatedAbility()) {
    return false;
  }

  auto followEntity = zone->GetActiveEntity(aiState->GetFollowEntityID());
  if (followEntity) {
    if (aiState->GetDespawnTimeout()) {
      // Do not despawn even if nowhere near entity
      aiState->SetDespawnTimeout(0);
    }

    followEntity->RefreshCurrentPosition(now);

    float maxDistance =
        aiState->IsAggro() ? FOLLOW_DISTANCE_MAX : FOLLOW_DISTANCE_FAR;
    if (eState->GetDistance(followEntity) > maxDistance) {
      // Pursue follow target
      if (Chase(eState, followEntity->GetEntityID(), FOLLOW_DISTANCE_CLOSE)) {
        // Start following
        if (!aiState->IsFollowing()) {
          UpdateAggro(eState, -1);
          eState->RemoveStatusTimes(STATUS_WAITING);
          aiState->SetStatus(AIStatus_t::FOLLOWING);
        }
      }

      return true;
    }
  } else if (aiState->GetDespawnWhenLost() && !aiState->GetDespawnTimeout()) {
    aiState->SetDespawnTimeout(now + (uint64_t)AI_DESPAWN_TIMEOUT);
  }

  if (aiState->IsFollowing()) {
    // Reset default state and walk to a point near the target
    aiState->SetStatus(aiState->GetDefaultStatus());

    if (followEntity) {
      Point src(followEntity->GetCurrentX(), followEntity->GetCurrentY());
      Point target(src.x, src.y + FOLLOW_DISTANCE_CLOSE);

      target = ZoneManager::RotatePoint(target, src,
                                        ZoneManager::GetRandomRotation());

      if (QueueMoveCommand(eState, target.x, target.y)) {
        QueueWaitCommand(aiState, 3000);

        // Undo status change so the commands remain
        aiState->ResetStatusChanged();
      }
    }

    return true;
  } else if (eState->StatusTimesKeyExists(STATUS_WAITING)) {
    // Keep waiting
    return true;
  }

  return false;
}

void AIManager::Move(const std::shared_ptr<ActiveEntityState>& eState,
                     Point dest, uint64_t now) {
  auto zone = eState->GetZone();
  if (!eState->CanMove() || !zone) {
    return;
  }

  Point collidePoint;
  if (zone->Collides(
          Line(Point(eState->GetCurrentX(), eState->GetCurrentY()), dest),
          collidePoint)) {
    // Cannot reach the destination, clear commands and quit
    eState->GetAIState()->ClearCommands();
    return;
  }

  eState->Move(dest.x, dest.y, now);
}

void AIManager::Wander(const std::shared_ptr<ActiveEntityState>& eState,
                       const std::shared_ptr<objects::EnemyBase>& eBase) {
  auto aiState = eState->GetAIState();

  auto spawnLocation = eBase->GetSpawnLocation();
  uint32_t spotID = eBase->GetSpawnSpotID();

  if (eState->CanMove()) {
    auto zone = eState->GetZone();
    if (!zone) {
      return;
    }

    Point source(eState->GetCurrentX(), eState->GetCurrentY());

    // If the entity has a despawn timeout, they should attempt to wander
    // back to the spawn location
    bool wanderBack = aiState->GetDespawnTimeout() > 0;

    // Move for 2s max
    float moveDistance = (float)((float)eState->GetMovementSpeed() * 2.f);

    Point dest;
    auto zoneManager = mServer.lock()->GetZoneManager();
    if (spawnLocation) {
      // Wander using spawn location
      auto point = zoneManager->GetRandomPoint(spawnLocation->GetWidth(),
                                               spawnLocation->GetHeight());

      // Spawn location group bounding box points start in the top
      // left corner of the rectangle and extend towards +X/-Y
      dest.x = (float)(spawnLocation->GetX() + point.x);
      dest.y = (float)(spawnLocation->GetY() - point.y);

      if (wanderBack) {
        std::list<Point> vertices;
        vertices.push_back(Point(spawnLocation->GetX(), spawnLocation->GetY()));
        vertices.push_back(
            Point(spawnLocation->GetX() + spawnLocation->GetWidth(),
                  spawnLocation->GetY()));
        vertices.push_back(
            Point(spawnLocation->GetX() + spawnLocation->GetWidth(),
                  spawnLocation->GetY() - spawnLocation->GetHeight()));
        vertices.push_back(
            Point(spawnLocation->GetX(),
                  spawnLocation->GetY() - spawnLocation->GetHeight()));

        wanderBack = !zoneManager->PointInPolygon(source, vertices);
      }
    } else if (spotID) {
      // Wander using spot, clear if somehow set invalid (via script etc)
      bool error = false;

      auto dynamicMap = zone->GetDynamicMap();
      if (dynamicMap) {
        auto spotIter = dynamicMap->Spots.find(spotID);
        if (spotIter != dynamicMap->Spots.end()) {
          auto spot = spotIter->second;
          dest = zoneManager->GetRandomSpotPoint(spot->Definition);

          wanderBack &= !zoneManager->PointInPolygon(source, spot->Vertices);
        } else {
          error = true;
        }
      } else {
        error = true;
      }

      if (error) {
        LogAIManagerDebug([eState, zone]() {
          return libcomp::String(
                     "Clearing invalid spot in zone %1 on AI controlled "
                     "entity: %2\n")
              .Arg(zone->GetDefinitionID())
              .Arg(eState->GetEntityLabel());
        });

        eBase->SetSpawnSpotID(0);
      }
    } else {
      // Wander aimlessly by just picking a direction to go
      dest = Point(source.x, source.y + moveDistance);
      dest = zoneManager->RotatePoint(dest, source,
                                      ZoneManager::GetRandomRotation());

      // Nothing to wander back to
      wanderBack = false;
    }

    // Use the destination as the direction to head
    Point finalDest = zoneManager->GetLinearPoint(
        source.x, source.y, dest.x, dest.y, moveDistance, false, zone);

    bool canReach = source.GetDistance(finalDest) >= source.GetDistance(dest);
    if (!canReach && wanderBack) {
      // Pull the shortest path and follow the first part of the path
      auto path = zoneManager->GetShortestPath(zone, source, dest);
      if (path.size() > 0) {
        finalDest = path.front();
        if (source.GetDistance(finalDest) > moveDistance) {
          // Reduce to the maximum distance
          finalDest = zoneManager->GetLinearPoint(source.x, source.y,
                                                  finalDest.x, finalDest.y,
                                                  moveDistance, false, zone);
        }
      }
    }

    auto command = GetMoveCommand(eState, finalDest, 0.f, false);
    if (command) {
      aiState->QueueCommand(command);

      // If the entity has a respawn timeout, clear it if they can
      // reach the designated point which is in the spawn area or
      // they don't actually need to wander back (ignore for following)
      if (aiState->GetDespawnTimeout() && !aiState->HasFollowTarget() &&
          (!wanderBack || canReach)) {
        aiState->SetDespawnTimeout(0);
      }
    }
  }

  // Wait between min/max times (check in case of custom AI errors)
  uint16_t minWait = aiState->GetWanderWaitMin();
  uint16_t maxWait = aiState->GetWanderWaitMax();
  QueueWaitCommand(
      aiState,
      (uint32_t)(RNG(int32_t, (int32_t)minWait,
                     (int32_t)(maxWait > minWait ? maxWait : minWait)) *
                 1000));
}

uint8_t AIManager::SkillAdvance(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::MiSkillData>& skillData,
    float distOverride) {
  auto zone = eState->GetZone();
  auto aiState = eState->GetAIState();

  if (skillData->GetRange()->GetValidType() !=
      objects::MiEffectiveRangeData::ValidType_t::ENEMY) {
    // No need to advance
    return 2;
  }

  int32_t targetEntityID = aiState->GetTargetEntityID();
  auto target = zone && targetEntityID > 0
                    ? zone->GetActiveEntity(targetEntityID)
                    : nullptr;
  if (!target) {
    return 1;
  }

  Point src(eState->GetCurrentX(), eState->GetCurrentY());
  Point dest(target->GetCurrentX(), target->GetCurrentY());

  // Convert distance to a whole number to simplify movement
  float targetDist = (float)floor(src.GetDistance(dest));

  float minDistance = distOverride;
  if (distOverride == 0.f) {
    // Move within range (keep a bit of a buffer for movement)
    uint16_t normalRange = skillData->GetTarget()->GetRange();
    uint32_t maxTargetRange =
        (uint32_t)(SKILL_DISTANCE_OFFSET + (target->GetHitboxSize() * 10) +
                   (uint32_t)(normalRange * 10));
    minDistance = (float)maxTargetRange - 20.f;
  }

  if (targetDist > minDistance) {
    // Stop at de-aggro distance
    float maxDistance = aiState->GetDeaggroDistance(false);

    if (Chase(eState, targetEntityID, minDistance, maxDistance, false, true)) {
      return 0;
    } else {
      return 1;
    }
  } else {
    // Nothing to do
    return 2;
  }
}

std::shared_ptr<ActiveEntityState> AIManager::Retarget(
    const std::shared_ptr<ActiveEntityState>& eState, uint64_t now,
    bool isNight) {
  std::shared_ptr<ActiveEntityState> target;

  auto aiState = eState->GetAIState();

  auto zone = eState->GetZone();
  if (!zone) {
    return nullptr;
  }

  int32_t currentTarget = aiState->GetTargetEntityID();

  float sourceX = eState->GetCurrentX();
  float sourceY = eState->GetCurrentY();

  auto opponentIDs = eState->GetOpponentIDs();
  std::list<std::shared_ptr<ActiveEntityState>> possibleTargets;
  if (opponentIDs.size() > 0) {
    // Currently in combat, only pull from opponents. Use deaggro
    // distance instead of the normal aggro distance since the AI
    // should technically be aggro until no opponents are still around
    auto inRange = zone->GetActiveEntitiesInRadius(
        sourceX, sourceY, aiState->GetDeaggroDistance(isNight));

    for (auto entity : inRange) {
      if (opponentIDs.find(entity->GetEntityID()) != opponentIDs.end() &&
          entity->IsAlive() && entity->Ready() && !entity->GetAIIgnored()) {
        possibleTargets.push_back(entity);
      }
    }
  } else {
    // Not in combat, find a target to pursue

    // If the entity has a low aggression level, check if targetting should
    // occur
    uint8_t aggroChance = (uint8_t)(aiState->GetAggression() * 100.f);
    if (aggroChance < 100 && RNG(int32_t, 1, 100) > aggroChance) {
      if (currentTarget > 0) {
        UpdateAggro(eState, -1);
      }

      return nullptr;
    }

    int32_t aggroLevelLimit =
        eState->GetLevel() + aiState->GetAggroLevelLimit();

    // Get aggro values, default to 2000 units and 80 degree FoV angle (in
    // radians)
    std::pair<float, float> aggroNormal(
        aiState->GetAggroValue(isNight ? 1 : 0, false, AI_DEFAULT_AGGRO_RANGE),
        aiState->GetAggroValue(isNight ? 1 : 0, true, 1.395f));
    std::pair<float, float> aggroCast(
        aiState->GetAggroValue(2, false, AI_DEFAULT_AGGRO_RANGE),
        aiState->GetAggroValue(2, true, 1.395f));

    // Get all active entities in range and FoV (cast aggro first,
    // leaving in doubles for higher chances when closer)
    bool castingOnly = true;

    std::list<std::shared_ptr<ActiveEntityState>> inFoV;
    for (auto aggro : {aggroCast, aggroNormal}) {
      auto filtered = zone->GetActiveEntitiesInRadius(sourceX, sourceY,
                                                      (double)aggro.first);

      // Remove allies, entities not ready yet or in an invalid state
      filtered.remove_if([eState, castingOnly, now](
                             const std::shared_ptr<ActiveEntityState>& entity) {
        entity->ExpireStatusTimes(now);
        return eState->SameFaction(entity) ||
               (castingOnly && !entity->GetStatusTimes(STATUS_CHARGING)) ||
               entity->StatusTimesKeyExists(STATUS_IGNORE) ||
               !entity->Ready() || entity->GetAIIgnored() || !entity->IsAlive();
      });

      // If the aggro level limit could potentially exclude a target
      // filter them out now
      if (aggroLevelLimit < 99) {
        filtered.remove_if(
            [aggroLevelLimit](
                const std::shared_ptr<ActiveEntityState>& entity) {
              return entity->GetLevel() > aggroLevelLimit;
            });
      }

      // If aggro limiting is enabled, remove targets based upon level
      // limit
      const static auto aggroLimit =
          mServer.lock()->GetWorldSharedConfig()->GetAIAggroLimit();
      if (aggroLimit != objects::WorldSharedConfig::AIAggroLimit_t::NONE) {
        for (auto f : filtered) {
          // Remove invalid pursuers first
          for (int32_t aggroID : f->GetAggroIDs()) {
            auto other = zone->GetActiveEntity(aggroID);
            auto otherState = other ? other->GetAIState() : nullptr;

            bool remove = false;
            if (otherState &&
                otherState->GetTargetEntityID() != f->GetEntityID()) {
              // If aggro is shared, check to see if they're
              // actually aggroed on the other entity and let
              // them handle this logic
              if (aggroLimit ==
                  objects::WorldSharedConfig::AIAggroLimit_t::PLAYER_SHARED) {
                auto sEntity = GetSharedAggroEntity(f);
                remove = !sEntity || sEntity->GetEntityID() !=
                                         otherState->GetTargetEntityID();
              } else {
                remove = true;
              }
            } else if (!other || !other->Ready() || !otherState) {
              remove = true;
            }

            if (remove) {
              LogAIManagerDebug([aggroID, f]() {
                return libcomp::String(
                           "Removing invalid aggro entity ID for %1: %2\n")
                    .Arg(f->GetEntityLabel())
                    .Arg(aggroID);
              });

              AddRemoveAggro(f, aggroID, true);
            }
          }
        }

        // Do not pursue if they're already being pursued by too
        // many enemies (this is ignored for opponents)
        uint8_t limit = aiState->GetAggroLimit();
        size_t max = 0;
        if (limit) {
          // Aggro limits dictate both max count and priority. The
          // count is determined by the sum of bit shift values and
          // the priority is determined by the numeric value. For
          // example both 0x03 (byte position 1 and 2) and 0x04 (byte
          // position 3) designate an aggro limit of 3 but 0x04 takes
          // priority over 0x03. Only the count designation is
          // currently supported.
          for (uint8_t i = 0; i < 8; i++) {
            if ((limit >> i) & 0x01) {
              max = (size_t)(max + i + 1);
            }
          }
        } else {
          // Default to 1
          max = 1;
        }

        filtered.remove_if(
            [max](const std::shared_ptr<ActiveEntityState>& entity) {
              return entity->AggroIDsCount() >= max;
            });
      }

      if (filtered.size() > 0) {
        // Targets found, check if they're visible
        for (auto entity : filtered) {
          entity->RefreshCurrentPosition(now);
        }

        // Filter the set down to only entities in the FoV
        for (auto fovEntity : ZoneManager::GetEntitiesInFoV(
                 filtered, sourceX, sourceY, eState->GetCurrentRotation(),
                 aggro.second)) {
          inFoV.push_back(fovEntity);
        }
      }

      castingOnly = false;
    }

    if (inFoV.size() > 0) {
      auto geometry = zone->GetGeometry();
      for (auto entity : inFoV) {
        // Possible target found, check line of sight
        bool add = true;
        if (geometry) {
          Line path(Point(sourceX, sourceY),
                    Point(entity->GetCurrentX(), entity->GetCurrentY()));

          Point collidePoint;
          add = !zone->Collides(path, collidePoint);
        }

        if (add) {
          possibleTargets.push_back(entity);

          if (aiState->IsFollowing() || aiState->IsWandering()) {
            aiState->SetStatus(AIStatus_t::AGGRO);
          }
        }
      }
    }
  }

  int32_t newTarget = currentTarget;
  if (possibleTargets.size() > 0) {
    if (aiState->ActionOverridesKeyExists("target") && aiState->GetScript()) {
      Sqrat::Function f(Sqrat::RootTable(aiState->GetScript()->GetVM()),
                        aiState->GetActionOverrides("target").C());

      auto scriptResult =
          !f.IsNull() ? f.Evaluate<int32_t>(eState, possibleTargets, this, now)
                      : 0;
      if (scriptResult) {
        target = zone->GetActiveEntity(*scriptResult);
      }
    } else {
      target = libcomp::Randomizer::GetEntry(possibleTargets);
    }

    newTarget = target ? target->GetEntityID() : -1;
  } else {
    newTarget = -1;
  }

  if (newTarget != currentTarget) {
    UpdateAggro(eState, newTarget);
  }

  return target;
}

void AIManager::RefreshSkillMap(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<AIState>& aiState) {
  if (aiState->GetSkillsMapped()) {
    return;
  }

  auto zone = eState->GetZone();
  bool isEnemy = eState->GetEnemyBase() != nullptr;
  auto logicGroup = aiState->GetLogicGroup();

  AISkillMap_t skillMap;

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto skillManager = server->GetSkillManager();
  for (uint32_t skillID : eState->GetCurrentSkills()) {
    auto skillData = definitionManager->GetSkillData(skillID);

    if (!SkillIsValid(skillData) ||
        skillManager->SkillZoneRestricted(skillID, zone)) {
      continue;
    }

    int16_t targetType = -1;
    switch (skillData->GetRange()->GetValidType()) {
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
        if (!isEnemy) {
          // Skills that affect parties or dead entities are not
          // usable by enemies
          targetType = (int16_t)AI_SKILL_TYPES_ALLY;
        }
        break;
      default:
        break;
    }

    if (targetType != -1) {
      // Determine if its a valid type
      int16_t skillType = -1;
      switch (skillData->GetBasic()->GetActionType()) {
        case objects::MiSkillBasicData::ActionType_t::ATTACK:
        case objects::MiSkillBasicData::ActionType_t::RUSH:
        case objects::MiSkillBasicData::ActionType_t::SPIN:
          if (targetType == (int16_t)AI_SKILL_TYPES_ENEMY) {
            skillType = (int16_t)AI_SKILL_TYPE_CLSR;
          }
          break;
        case objects::MiSkillBasicData::ActionType_t::SHOT:
        case objects::MiSkillBasicData::ActionType_t::RAPID:
          if (targetType == (int16_t)AI_SKILL_TYPES_ENEMY) {
            skillType = (int16_t)AI_SKILL_TYPE_LNGR;
          }
          break;
        case objects::MiSkillBasicData::ActionType_t::SUPPORT:
          // Split based upon if the skill can heal or not
          switch (skillData->GetDamage()->GetBattleDamage()->GetFormula()) {
            case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
            case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT:
            case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
              if (skillData->GetDamage()->GetBattleDamage()->GetModifier1()) {
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
          if (aiState->GetStatus() != AIStatus_t::ENRAGED) {
            skillType = (int16_t)AI_SKILL_TYPE_DEF;
          }
          break;
        case objects::MiSkillBasicData::ActionType_t::TALK:
        case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
        case objects::MiSkillBasicData::ActionType_t::TAUNT:
        default:
          // Not supported
          break;
      }

      if (skillType != -1) {
        // Determine if costs are not valid for this entity or if any
        // cost exists at all. Even though the HP/MP cost will not
        // apply for the entire time the entity is active, since
        // percentage costs round up we should still have a clear
        // picture of if any costs exist.
        int32_t hpCost = 0, mpCost = 0;
        uint16_t bulletCost = 0;
        std::unordered_map<uint32_t, uint32_t> itemCosts;
        if (!skillManager->DetermineNormalCosts(
                eState, skillData, hpCost, mpCost, bulletCost, itemCosts) ||
            bulletCost || itemCosts.size() > 0) {
          continue;
        }

        // Skill is valid. Calculate the weight (higher for more
        // preferable skills).
        int32_t weight = skillType == AI_SKILL_TYPE_DEF ? 1 : 2;
        if (logicGroup) {
          // Calculate the weight generic skill weight

          // Having no charge time adds weight
          if (!skillData->GetCast()->GetBasic()->GetChargeTime()) {
            weight += logicGroup->GetSkillWeightCharge();
          }

          // Having no cost adds weight
          if (hpCost == 0 && mpCost == 0) {
            weight += logicGroup->GetSkillWeightCost();
          }

          // Heal skills are weighted more only when the heal threshold
          // is active as they are not chosen otherwise
          if (skillType == AI_SKILL_TYPE_HEAL) {
            weight += logicGroup->GetSkillWeightHeal();
          }

          // Ranged attacks add weight
          if (skillData->GetTarget()->GetRange() > 0) {
            weight += logicGroup->GetSkillWeightRange();
          }
        }

        skillMap[(uint16_t)skillType].push_back(
            std::make_pair(skillData, weight));
      }
    }
  }

  aiState->SetSkillMap(skillMap);
}

bool AIManager::SkillIsValid(
    const std::shared_ptr<objects::MiSkillData>& skillData) {
  // Active skills only
  auto category = skillData->GetCommon()->GetCategory();
  if (category->GetMainCategory() != SKILL_CATEGORY_ACTIVE) {
    return false;
  }

  auto basic = skillData->GetBasic();

  // Ignore invalid family types (items, fusion)
  if (basic->GetFamily() == SkillFamily_t::DEMON_SOLO ||
      basic->GetFamily() == SkillFamily_t::FUSION ||
      basic->GetFamily() == SkillFamily_t::ITEM) {
    return false;
  }

  uint16_t functionID = skillData->GetDamage()->GetFunctionID();
  if (functionID) {
    // Only certain function IDs are supported for general use
    const static std::set<uint16_t> supportedFIDs = {
        SVR_CONST.SKILL_ABS_DAMAGE,       SVR_CONST.SKILL_DESPAWN,
        SVR_CONST.SKILL_DIGITALIZE_BREAK, SVR_CONST.SKILL_HP_DEPENDENT,
        SVR_CONST.SKILL_HP_MP_MIN,        SVR_CONST.SKILL_LNC_DAMAGE,
        SVR_CONST.SKILL_MINION_SPAWN,     SVR_CONST.SKILL_PIERCE,
        SVR_CONST.SKILL_SLEEP_RESTRICTED, SVR_CONST.SKILL_STAT_SUM_DAMAGE,
        SVR_CONST.SKILL_STATUS_DIRECT,    SVR_CONST.SKILL_STATUS_RANDOM,
        SVR_CONST.SKILL_STATUS_RANDOM2,   SVR_CONST.SKILL_STATUS_SCALE,
        SVR_CONST.SKILL_SUICIDE,
    };

    if (supportedFIDs.find(functionID) == supportedFIDs.end() &&
        mServer.lock()->GetSkillManager()->FunctionIDMapped(functionID)) {
      // Mapped and not whitelisted, therefore not supported
      return false;
    }
  }

  return true;
}

bool AIManager::CanRetrySkill(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::ActivatedAbility>& activated) {
  if (!activated || !eState || eState->GetActivatedAbility() != activated) {
    return false;
  }

  switch (activated->GetErrorCode()) {
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
    const std::shared_ptr<ActiveEntityState>& eState) {
  auto zone = eState->GetZone();
  auto aiState = eState->GetAIState();
  auto logicGroup = aiState->GetLogicGroup();
  auto cs = eState->GetCoreStats();
  if (!zone || !cs) {
    return false;
  }

  RefreshSkillMap(eState, aiState);

  int32_t targetID = aiState->GetTargetEntityID();
  auto target = targetID > 0 ? zone->GetActiveEntity(targetID) : nullptr;

  if (aiState->ActionOverridesKeyExists("prepareSkill")) {
    libcomp::String fOverride = aiState->GetActionOverrides("prepareSkill");

    Sqrat::Function f(Sqrat::RootTable(aiState->GetScript()->GetVM()),
                      fOverride.IsEmpty() ? "prepareSkill" : fOverride.C());

    auto scriptResult =
        !f.IsNull() ? f.Evaluate<int32_t>(eState, this, target) : 0;
    if (!scriptResult || *scriptResult == -1) {
      // Do not continue
      return false;
    } else if (*scriptResult == 0) {
      // Added by script
      return true;
    }
  }

  if (!aiState->GetNormalSkillUse()) {
    // Do not select a skill via normal logic
    return false;
  }

  auto skillMap = aiState->GetSkillMap();

  // Non-defensive/support combat skills are only accessible to first
  // strike entities unless combat has already started
  bool canFight = target && (aiState->GetStrikeFirst() || aiState->InCombat());

  bool canHeal =
      logicGroup && (float)cs->GetHP() / (float)eState->GetMaxHP() <=
                        (float)logicGroup->GetHealThreshold() * 0.01f;
  bool canSupport = skillMap[AI_SKILL_TYPE_SUPPORT].size() > 0;
  bool canDefend = skillMap[AI_SKILL_TYPE_DEF].size() > 0;
  if (skillMap.size() > 0 && (canFight || canHeal || canSupport || canDefend)) {
    auto server = mServer.lock();
    auto skillManager = server->GetSkillManager();

    std::set<uint32_t> lockedSkills;
    for (double val : server->GetTokuseiManager()->GetAspectValueList(
             eState, TokuseiAspectType::SKILL_LOCK)) {
      lockedSkills.insert((uint32_t)val);
    }

    std::list<AISkillWeight_t> weightedSkills;
    std::unordered_map<uint32_t, uint16_t> skillTypes;
    for (auto& pair : skillMap) {
      // Make sure the skill type is valid for the current state
      bool isHeal = pair.first == AI_SKILL_TYPE_HEAL;
      bool isDefense = pair.first == AI_SKILL_TYPE_DEF;
      bool isSupport = pair.first == AI_SKILL_TYPE_SUPPORT;

      if ((aiState->GetSkillSettings() & pair.first) != 0 &&
          (isSupport || isHeal ||  // No special requirement (yet)
           (isDefense || canFight))) {
        for (auto& weight : pair.second) {
          auto skillData = weight.first;
          uint32_t skillID = skillData->GetCommon()->GetID();

          // Make sure its not cooling down or restricted
          if (eState->SkillCooldownsKeyExists(
                  skillData->GetBasic()->GetCooldownID()) ||
              skillManager->SkillRestricted(eState, skillData) ||
              lockedSkills.find(skillID) != lockedSkills.end()) {
            continue;
          }

          auto skillTarget = target;
          if (skillData->GetTarget()->GetType() !=
              objects::MiTargetData::Type_t::ENEMY) {
            if (isHeal && !canHeal) {
              // Can't currently heal self
              continue;
            }

            skillTarget = eState;
          }

          // If it is a minion spawning skill, make sure the SLG is
          // not currently spawned (restricted for auto-use only)
          const static auto minionSpawnFID = SVR_CONST.SKILL_MINION_SPAWN;
          if (skillData->GetDamage()->GetFunctionID() == minionSpawnFID) {
            auto params = skillData->GetSpecial()->GetSpecialParams();
            if ((uint32_t)params[0] != zone->GetDefinitionID() ||
                zone->MinionSpawned(eState, (uint32_t)params[1])) {
              continue;
            }
          }

          // Make sure the target is valid
          if (!skillManager->ValidateSkillTarget(eState, skillData,
                                                 skillTarget)) {
            continue;
          }

          // Make sure costs can be paid (item/bullet costs won't
          // be in the map at all)
          int32_t hpCost = 0, mpCost = 0;
          uint16_t bulletCost = 0;
          std::unordered_map<uint32_t, uint32_t> itemCosts;
          if (!skillManager->DetermineNormalCosts(
                  eState, skillData, hpCost, mpCost, bulletCost, itemCosts) ||
              hpCost >= cs->GetHP() || mpCost > cs->GetMP()) {
            continue;
          }

          weightedSkills.push_back(weight);
          skillTypes[skillID] = pair.first;
        }
      }
    }

    if (logicGroup && logicGroup->GetActionTypeWeighted()) {
      // Action type weights exist. Choose which action type we will
      // use and filter to just that type. If no skill exists that
      // matches any type configured, no skill will be selected but
      // the AI will NO deaggro/retreat.
      std::set<uint8_t> actionTypes;
      for (auto& pair : weightedSkills) {
        actionTypes.insert((uint8_t)pair.first->GetBasic()->GetActionType());
      }

      uint16_t totalWeight = 0;
      std::list<std::pair<uint8_t, uint16_t>> actionTypeWeights;
      for (uint8_t i = 0; i < 12; i++) {
        if (actionTypes.find(i) != actionTypes.end()) {
          uint16_t val = (uint16_t)logicGroup->GetActionTypeWeights(i);
          actionTypeWeights.push_back(std::make_pair(i, val));
          totalWeight = (uint16_t)(totalWeight + val);
        }
      }

      uint8_t selectedActionType = 0;
      if (totalWeight > 0) {
        uint16_t rVal = RNG(uint16_t, 1, totalWeight);
        for (auto& pair : actionTypeWeights) {
          if (pair.second >= rVal) {
            selectedActionType = pair.first;
            break;
          } else {
            rVal = (uint16_t)(rVal - pair.second);
          }
        }
      }

      if (selectedActionType) {
        weightedSkills.remove_if(
            [selectedActionType](const AISkillWeight_t& wSkill) {
              return (uint8_t)wSkill.first->GetBasic()->GetActionType() !=
                     selectedActionType;
            });
      } else {
        // Do not act but do not deaggro
        return true;
      }
    }

    if (weightedSkills.size() == 0) {
      // Can't use anything right now
      return false;
    }

    // Sort skills by weight (higher first)
    weightedSkills.sort([](const AISkillWeight_t& a, const AISkillWeight_t& b) {
      return a.second > b.second;
    });

    std::shared_ptr<objects::MiSkillData> skillData;
    if (weightedSkills.size() == 1) {
      // Only one valid skill left
      skillData = weightedSkills.begin()->first;
    } else {
      // Pull a random number between 1 and the total weight and use
      // the first one that exceeds the value that we reduce by weight
      // as we go
      uint16_t totalWeight = 0;
      for (auto& wSkill : weightedSkills) {
        totalWeight = (uint16_t)(totalWeight + wSkill.second);
      }

      uint16_t rVal = RNG(uint16_t, 1, totalWeight);
      for (auto& wSkill : weightedSkills) {
        if (wSkill.second >= rVal) {
          skillData = wSkill.first;
          break;
        } else {
          rVal = (uint16_t)(rVal - wSkill.second);
        }
      }
    }

    if (skillData) {
      // The skill target is either the aggro target or the entity itself
      int32_t skillTargetID = targetID;
      if (skillData->GetTarget()->GetType() !=
          objects::MiTargetData::Type_t::ENEMY) {
        skillTargetID = eState->GetEntityID();
      }

      switch (skillData->GetBasic()->GetActionType()) {
        case objects::MiSkillBasicData::ActionType_t::SPIN:
          if (skillTargetID != eState->GetEntityID()) {
            // Move up to the target first
            SkillAdvance(eState, skillData);
          }
          break;
        case objects::MiSkillBasicData::ActionType_t::GUARD:
        case objects::MiSkillBasicData::ActionType_t::COUNTER:
        case objects::MiSkillBasicData::ActionType_t::DODGE:
          if (targetID && aiState->GetDefensiveDistance() > 0.f) {
            // Move up to defensive distance
            SkillAdvance(eState, skillData, aiState->GetDefensiveDistance());
          }
          break;
        default:
          break;
      }

      auto cmd = std::make_shared<AIUseSkillCommand>(skillData, skillTargetID);
      aiState->QueueCommand(cmd);

      return true;
    }
  }

  return false;
}

std::shared_ptr<AIMoveCommand> AIManager::GetMoveCommand(
    const std::shared_ptr<ActiveEntityState>& eState, const Point& dest,
    float reduce, bool split, bool allowLazy) {
  auto zone = eState->GetZone();
  if (!zone || !eState->CanMove()) {
    return nullptr;
  }

  Point source(eState->GetCurrentX(), eState->GetCurrentY());
  if (source.GetDistance(dest) < reduce) {
    // Don't bother moving if we're trying to move away by accident
    return nullptr;
  }

  auto zoneManager = mServer.lock()->GetZoneManager();

  std::list<Point> pathing;
  if (allowLazy && LazyPathingEnabled()) {
    // Set path only if there is no linear collision
    Point collidePoint;
    if (!zone->Collides(Line(source, dest), collidePoint)) {
      pathing.push_back(dest);
    }
  } else {
    pathing = zoneManager->GetShortestPath(zone, source, dest);
  }

  if (pathing.size() == 0) {
    // No valid path
    return nullptr;
  }

  auto cmd = std::make_shared<AIMoveCommand>();
  if (reduce > 0.f) {
    auto it = pathing.rbegin();
    Point& last = *it;

    Point secondLast;
    if (pathing.size() > 1) {
      it++;
      secondLast = *it;
    } else {
      secondLast = source;
    }

    float dist = secondLast.GetDistance(last);
    auto adjusted = zoneManager->GetLinearPoint(
        secondLast.x, secondLast.y, last.x, last.y, dist - reduce, false);
    last.x = adjusted.x;
    last.y = adjusted.y;
  }

  float moveSpeed = eState->GetMovementSpeed();
  if (split && moveSpeed > 0.f) {
    // Move in 0.5s increments so it looks less robotic
    // (maximum distance in 0.5s is = speed * 0.5);
    float maxMoveDistance = moveSpeed * 0.5f;

    Point prev = source;

    std::list<Point> splitPath;
    for (auto& p : pathing) {
      if (prev.GetDistance(p) > maxMoveDistance) {
        // Break down into parts
        bool splitMore = true;
        while (splitMore) {
          Point sub = zoneManager->GetLinearPoint(prev.x, prev.y, p.x, p.y,
                                                  maxMoveDistance, false);
          splitPath.push_back(sub);
          prev = sub;

          if (prev.GetDistance(p) <= maxMoveDistance) {
            splitPath.push_back(p);
            splitMore = false;
            prev = p;
          }
        }
      } else {
        splitPath.push_back(p);
        prev = p;
      }
    }

    cmd->SetPathing(splitPath);
  } else {
    cmd->SetPathing(pathing);
  }

  return cmd;
}

std::shared_ptr<AICommand> AIManager::GetWaitCommand(uint32_t waitTime) const {
  auto cmd = std::make_shared<AICommand>();
  cmd->SetDelay((uint64_t)waitTime * 1000);

  return cmd;
}

void AIManager::AddRemoveAggro(const std::shared_ptr<ActiveEntityState>& eState,
                               int32_t targetID, bool remove) {
  if (!eState) {
    return;
  }

  const static bool sharePlayerAggro =
      mServer.lock()->GetWorldSharedConfig()->GetAIAggroLimit() ==
      objects::WorldSharedConfig::AIAggroLimit_t::PLAYER_SHARED;

  if (sharePlayerAggro) {
    auto sharedEntity = GetSharedAggroEntity(eState);
    if (sharedEntity) {
      // Update both entities
      std::list<std::shared_ptr<ActiveEntityState>> eStates;
      eStates.push_back(eState);
      eStates.push_back(sharedEntity);
      for (auto e : eStates) {
        if (e->Ready(true)) {
          if (remove) {
            e->RemoveAggroIDs(targetID);

            LogAIManagerDebug([e]() {
              return libcomp::String("%1 shared aggro count lowered to %2.\n")
                  .Arg(e->GetEntityLabel())
                  .Arg(e->AggroIDsCount());
            });
          } else {
            e->InsertAggroIDs(targetID);

            LogAIManagerDebug([e]() {
              return libcomp::String("%1 shared aggro count raised to %2.\n")
                  .Arg(e->GetEntityLabel())
                  .Arg(e->AggroIDsCount());
            });
          }
        }
      }

      return;
    }
  }

  if (remove) {
    eState->RemoveAggroIDs(targetID);

    LogAIManagerDebug([eState]() {
      return libcomp::String("%1 aggro count lowered to %2.\n")
          .Arg(eState->GetEntityLabel())
          .Arg(eState->AggroIDsCount());
    });
  } else {
    eState->InsertAggroIDs(targetID);

    LogAIManagerDebug([eState]() {
      return libcomp::String("%1 aggro count raised to %2.\n")
          .Arg(eState->GetEntityLabel())
          .Arg(eState->AggroIDsCount());
    });
  }
}

std::shared_ptr<ActiveEntityState> AIManager::GetSharedAggroEntity(
    const std::shared_ptr<ActiveEntityState>& eState) {
  int32_t worldCID = eState ? eState->GetWorldCID() : 0;
  auto state = worldCID > 0 ? ClientState::GetEntityClientState(worldCID, true)
                            : nullptr;
  if (state) {
    auto cState = state->GetCharacterState();
    if (eState == cState) {
      return state->GetDemonState();
    } else {
      return cState;
    }
  }

  return nullptr;
}

bool AIManager::CombatStaggerEnabled() {
  const static bool enabled =
      mServer.lock()->GetWorldSharedConfig()->GetAICombatStagger();
  return enabled;
}

bool AIManager::LazyPathingEnabled() {
  const static bool enabled =
      mServer.lock()->GetWorldSharedConfig()->GetAILazyPathing();
  return enabled;
}
