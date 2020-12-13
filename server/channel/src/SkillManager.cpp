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
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <ActionSpawn.h>
#include <ActivatedAbility.h>
#include <CalculatedEntityState.h>
#include <ChannelConfig.h>
#include <CharacterProgress.h>
#include <DemonFamiliarityType.h>
#include <DigitalizeState.h>
#include <DropSet.h>
#include <Expertise.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <ItemDrop.h>
#include <Loot.h>
#include <LootBox.h>
#include <MiAcquisitionData.h>
#include <MiAddStatusTbl.h>
#include <MiBattleDamageData.h>
#include <MiBreakData.h>
#include <MiCancelData.h>
#include <MiCastBasicData.h>
#include <MiCastCancelData.h>
#include <MiCastData.h>
#include <MiCategoryData.h>
#include <MiConditionData.h>
#include <MiCostTbl.h>
#include <MiDCategoryData.h>
#include <MiDamageData.h>
#include <MiDevilBattleData.h>
#include <MiDevilBookData.h>
#include <MiDevilData.h>
#include <MiDevilFamiliarityData.h>
#include <MiDevilFusionData.h>
#include <MiDischargeData.h>
#include <MiDoTDamageData.h>
#include <MiEffectData.h>
#include <MiEffectiveRangeData.h>
#include <MiExpertClassData.h>
#include <MiExpertData.h>
#include <MiExpertGrowthTbl.h>
#include <MiExpertRankData.h>
#include <MiGrowthData.h>
#include <MiGuardianLevelData.h>
#include <MiGuardianLevelDataEntry.h>
#include <MiGuardianSpecialData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiItemPvPData.h>
#include <MiKnockBackData.h>
#include <MiNPCBasicData.h>
#include <MiNegotiationDamageData.h>
#include <MiNegotiationData.h>
#include <MiPossessionData.h>
#include <MiRentalData.h>
#include <MiRestrictionData.h>
#include <MiSItemData.h>
#include <MiSkillBasicData.h>
#include <MiSkillCharasticData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSkillPvPData.h>
#include <MiSkillSpecialParams.h>
#include <MiStatusBasicData.h>
#include <MiStatusData.h>
#include <MiSummonData.h>
#include <MiTargetData.h>
#include <MiUnionData.h>
#include <MiUseRestrictionsData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <Party.h>
#include <PvPData.h>
#include <PvPInstanceStats.h>
#include <PvPPlayerStats.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <ServerZonePartial.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocation.h>
#include <SpawnLocationGroup.h>
#include <Team.h>
#include <Tokusei.h>
#include <TokuseiCondition.h>
#include <TokuseiSkillCondition.h>
#include <UBMatch.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "AIManager.h"
#include "ActionManager.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ChatManager.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "MatchManager.h"
#include "TokuseiManager.h"
#include "Zone.h"
#include "ZoneInstance.h"
#include "ZoneManager.h"

using namespace channel;

const uint8_t DAMAGE_TYPE_GENERIC = 0;
const uint8_t DAMAGE_TYPE_HEALING = 1;
const uint8_t DAMAGE_TYPE_NONE = 2;
const uint8_t DAMAGE_TYPE_MISS = 3;
const uint8_t DAMAGE_TYPE_DRAIN = 5;
const uint8_t DAMAGE_EXPLICIT_SET = 6;

const uint16_t FLAG1_LETHAL = 1;
const uint16_t FLAG1_CLENCH = 1 << 2;
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
// const uint16_t FLAG1_REFLECT_UNUSED = 1 << 13;

const uint16_t FLAG2_CLENCH = 1 << 4;
const uint16_t FLAG2_LIMIT_BREAK = 1 << 5;
const uint16_t FLAG2_IMPOSSIBLE = 1 << 6;
const uint16_t FLAG2_BARRIER = 1 << 7;
const uint16_t FLAG2_INTENSIVE_BREAK = 1 << 8;
const uint16_t FLAG2_INSTANT_DEATH = 1 << 9;

const uint8_t TALK_SUCCESS_LVL1 = 0;
const uint8_t TALK_SUCCESS_LVL2 = 1;
const uint8_t TALK_SUCCESS_LVL3 = 2;
const uint8_t TALK_SUCCESS_LVL4 = 3;
const uint8_t TALK_FAIL = 4;
const uint8_t TALK_JOIN_1 = 5;
const uint8_t TALK_GIVE_ITEM_1 = 6;
const uint8_t TALK_DONE_1 = 7;
const uint8_t TALK_LEAVE_1 = 8;
const uint8_t TALK_JOIN_2 = 9;
const uint8_t TALK_GIVE_ITEM_2 = 10;
const uint8_t TALK_DONE_2 = 11;
const uint8_t TALK_LEAVE_2 = 12;
const uint8_t TALK_FOFF_1 = 13;
const uint8_t TALK_FOFF_2 = 14;
// Unused
// const uint8_t TALK_LOW_LEVEL_1 = 15;
// const uint8_t TALK_LOW_LEVEL_2 = 16;

const uint8_t RES_OFFSET = (uint8_t)CorrectTbl::RES_DEFAULT;
const uint8_t BOOST_OFFSET = (uint8_t)CorrectTbl::BOOST_DEFAULT;
const uint8_t NRA_OFFSET = (uint8_t)CorrectTbl::NRA_DEFAULT;
const uint8_t AIL_OFFSET = (uint8_t)((uint8_t)CorrectTbl::RES_FIRE -
                                     (uint8_t)CorrectTbl::RES_DEFAULT - 1);

class channel::ProcessingSkill {
 public:
  uint32_t SkillID = 0;
  uint16_t FunctionID = 0;
  std::shared_ptr<objects::MiSkillData> Definition;
  std::shared_ptr<objects::ActivatedAbility> Activated;
  SkillExecutionContext* ExecutionContext = 0;
  uint16_t Modifier1 = 0;
  uint16_t Modifier2 = 0;
  int8_t TalkAffSuccess = 0;
  int8_t TalkAffFailure = 0;
  int8_t TalkFearSuccess = 0;
  int8_t TalkFearFailure = 0;
  uint8_t BaseAffinity = 0;
  uint8_t EffectiveAffinity = 0;
  uint8_t WeaponAffinity = 0;
  SkillDependencyType_t EffectiveDependencyType = (SkillDependencyType_t)0;
  uint8_t ExpertiseType = 0;
  uint8_t ExpertiseRankBoost = 0;
  uint8_t KnowledgeRank = 0;
  int32_t AbsoluteDamage = 0;
  int16_t ChargeReduce = 0;
  bool IsItemSkill = false;
  bool IsProjectile = false;
  bool CanNRA = true;
  uint8_t Nulled = 0;
  uint8_t Reflected = 0;
  bool Absorbed = false;
  uint8_t NRAAffinity = 0;
  bool HardStrike = false;
  bool InPvP = false;

  // Only used for rushes
  uint64_t RushStartTime = 0;
  std::shared_ptr<Point> RushStartPoint;

  std::shared_ptr<Zone> CurrentZone;
  std::shared_ptr<ActiveEntityState> EffectiveSource;
  std::list<std::shared_ptr<DemonState>> FusionDemons;
  std::list<channel::SkillTargetResult> Targets;
  std::shared_ptr<ActiveEntityState> PrimaryTarget;
  std::shared_ptr<objects::CalculatedEntityState> SourceExecutionState;

  // Keyed on entity IDs (source or fusion demons) then target entity IDs
  std::unordered_map<int32_t, std::unordered_map<int32_t, uint16_t>>
      OffenseValues;
  std::unordered_map<
      int32_t, std::unordered_map<
                   int32_t, std::shared_ptr<objects::CalculatedEntityState>>>
      SourceCalcStates;

  // Keyed on target entity IDs
  std::unordered_map<int32_t, std::shared_ptr<objects::CalculatedEntityState>>
      TargetCalcStates;
};

class channel::SkillLogicSettings {
 public:
  uint16_t FunctionID = 0;
  bool HasActivationValidation = false;
  bool HasExecutionValidation = false;
  bool HasCostAdjustment = false;
  bool HasPreAction = false;
  bool HasPostAction = false;
};

class channel::SkillTargetResult {
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
  StatusEffectChanges AddedStatuses;
  std::set<uint32_t> CancelledStatuses;
  bool HitAvoided = false;
  uint8_t HitNull = 0;     // 0: None, 1: Physical, 2: Magic, 3: Barrier
  uint8_t HitReflect = 0;  // 0: None, 1: Physical, 2: Magic
  bool HitAbsorb = false;
  uint8_t NRAAffinity = 0;
  bool CanHitstun = false;
  bool ApplyAddedKnockbackEffects = false;
  bool AutoProtect = false;
  bool ClenchOverflow = false;
  uint16_t GuardModifier = 0;

  uint8_t EffectCancellations = 0;
  std::set<TokuseiConditionType> RecalcTriggers;
  bool TalkDone = false;
};

SkillManager::SkillManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server) {
  // Map unique function skills
  mSkillFunctions[SVR_CONST.SKILL_CAMEO] = &SkillManager::Cameo;
  mSkillFunctions[SVR_CONST.SKILL_CLOAK] = &SkillManager::Cloak;
  mSkillFunctions[SVR_CONST.SKILL_DCM] = &SkillManager::DCM;
  mSkillFunctions[SVR_CONST.SKILL_DESPAWN] = &SkillManager::Despawn;
  mSkillFunctions[SVR_CONST.SKILL_DESUMMON] = &SkillManager::Desummon;
  mSkillFunctions[SVR_CONST.SKILL_DIGITALIZE] = &SkillManager::Digitalize;
  mSkillFunctions[SVR_CONST.SKILL_DIGITALIZE_CANCEL] =
      &SkillManager::DigitalizeCancel;
  mSkillFunctions[SVR_CONST.SKILL_EQUIP_ITEM] = &SkillManager::EquipItem;
  mSkillFunctions[SVR_CONST.SKILL_EXPERT_FORGET_ALL] =
      &SkillManager::ForgetAllExpertiseSkills;
  mSkillFunctions[SVR_CONST.SKILL_FAM_UP] = &SkillManager::FamiliarityUp;
  mSkillFunctions[SVR_CONST.SKILL_ITEM_FAM_UP] =
      &SkillManager::FamiliarityUpItem;
  mSkillFunctions[SVR_CONST.SKILL_MINION_DESPAWN] =
      &SkillManager::MinionDespawn;
  mSkillFunctions[SVR_CONST.SKILL_MINION_SPAWN] = &SkillManager::MinionSpawn;
  mSkillFunctions[SVR_CONST.SKILL_MOOCH] = &SkillManager::Mooch;
  mSkillFunctions[SVR_CONST.SKILL_MOUNT] = &SkillManager::Mount;
  mSkillFunctions[SVR_CONST.SKILL_RANDOM_ITEM] = &SkillManager::RandomItem;
  mSkillFunctions[SVR_CONST.SKILL_RANDOMIZE] = &SkillManager::Randomize;
  mSkillFunctions[SVR_CONST.SKILL_RESPEC] = &SkillManager::Respec;
  mSkillFunctions[SVR_CONST.SKILL_REST] = &SkillManager::Rest;
  mSkillFunctions[SVR_CONST.SKILL_SPAWN] = &SkillManager::Spawn;
  mSkillFunctions[SVR_CONST.SKILL_SPAWN_ZONE] = &SkillManager::SpawnZone;
  mSkillFunctions[SVR_CONST.SKILL_SUMMON_DEMON] = &SkillManager::SummonDemon;
  mSkillFunctions[SVR_CONST.SKILL_STORE_DEMON] = &SkillManager::StoreDemon;
  mSkillFunctions[SVR_CONST.SKILL_TRAESTO] = &SkillManager::Traesto;
  mSkillFunctions[(uint16_t)SVR_CONST.SKILL_TRAESTO_ARCADIA[0]] =
      &SkillManager::Traesto;
  mSkillFunctions[(uint16_t)SVR_CONST.SKILL_TRAESTO_DSHINJUKU[0]] =
      &SkillManager::Traesto;
  mSkillFunctions[(uint16_t)SVR_CONST.SKILL_TRAESTO_KAKYOJO[0]] =
      &SkillManager::Traesto;
  mSkillFunctions[(uint16_t)SVR_CONST.SKILL_TRAESTO_NAKANO_BDOMAIN[0]] =
      &SkillManager::Traesto;
  mSkillFunctions[(uint16_t)SVR_CONST.SKILL_TRAESTO_SOUHONZAN[0]] =
      &SkillManager::Traesto;
  mSkillFunctions[SVR_CONST.SKILL_XP_PARTNER] = &SkillManager::XPUp;
  mSkillFunctions[SVR_CONST.SKILL_XP_SELF] = &SkillManager::XPUp;

  // Map skills that will send a follow up packet after processing
  mSkillFunctions[SVR_CONST.SKILL_CLAN_FORM] = &SkillManager::SpecialSkill;
  mSkillFunctions[SVR_CONST.SKILL_EQUIP_MOD_EDIT] = &SkillManager::SpecialSkill;
  mSkillFunctions[SVR_CONST.SKILL_EXPERT_CLASS_DOWN] =
      &SkillManager::SpecialSkill;
  mSkillFunctions[SVR_CONST.SKILL_EXPERT_FORGET] = &SkillManager::SpecialSkill;
  mSkillFunctions[SVR_CONST.SKILL_EXPERT_RANK_DOWN] =
      &SkillManager::SpecialSkill;
  mSkillFunctions[SVR_CONST.SKILL_MAX_DURABILITY_FIXED] =
      &SkillManager::SpecialSkill;
  mSkillFunctions[SVR_CONST.SKILL_MAX_DURABILITY_RANDOM] =
      &SkillManager::SpecialSkill;
  mSkillFunctions[SVR_CONST.SKILL_SPECIAL_REQUEST] =
      &SkillManager::SpecialSkill;
  mSkillFunctions[SVR_CONST.SKILL_WARP] = &SkillManager::SpecialSkill;

  // Map of skills that have special effects after normal processing
  mSkillEffectFunctions[SVR_CONST.SKILL_DIGITALIZE_BREAK] =
      &SkillManager::DigitalizeBreak;
  mSkillEffectFunctions[SVR_CONST.SKILL_ESTOMA] = &SkillManager::Estoma;
  mSkillEffectFunctions[SVR_CONST.SKILL_LIBERAMA] = &SkillManager::Liberama;
  mSkillEffectFunctions[SVR_CONST.SKILL_STATUS_DIRECT] =
      &SkillManager::DirectStatus;
  mSkillEffectFunctions[SVR_CONST.SKILL_STATUS_LIMITED] =
      &SkillManager::DirectStatus;

  // Make sure anything not set is not pulled in to the mapping
  mSkillFunctions.erase(0);

  LoadScripts();
}

SkillManager::~SkillManager() {}

void SkillManager::LoadScripts() {
  // Load all skill logic scripts
  auto server = mServer.lock();
  auto serverDataManager = server->GetServerDataManager();
  bool scriptsLoaded = false;

  LogSkillManagerDebugMsg("Loading custom skill logic...\n");

  std::list<std::shared_ptr<libcomp::ServerScript>> scriptDefs;
  for (auto serverScript : serverDataManager->LoadScripts(
           server->GetDataStore(), "/skills", scriptsLoaded, false)) {
    if (serverScript->Type.ToLower() == "skilllogic") {
      scriptDefs.push_back(serverScript);
    }
  }

  if (!scriptsLoaded) {
    LogSkillManagerError([scriptDefs]() {
      return libcomp::String(
                 "API handler failed after loading %1 custom skill logic "
                 "script(s)\n")
          .Arg(scriptDefs.size());
    });
  } else if (scriptDefs.size() == 0) {
    LogSkillManagerDebugMsg("No custom skill logic found\n");
    return;
  } else {
    LogSkillManagerDebug([scriptDefs]() {
      return libcomp::String(
                 "API handler successfully loaded %1 custom skill logic "
                 "script(s)\n")
          .Arg(scriptDefs.size());
    });
  }

  // Prepare scripts and load settings
  for (auto def : scriptDefs) {
    auto script = std::make_shared<libcomp::ScriptEngine>();

    if (!script->Eval(def->Source)) {
      LogSkillManagerError([def]() {
        return libcomp::String("Failed to prepare skill logic script: %1\n")
            .Arg(def->Name);
      });

      continue;
    }

    auto settings = std::make_shared<SkillLogicSettings>();

    auto vm = script->GetVM();
    {
      Sqrat::Class<SkillLogicSettings, Sqrat::NoConstructor<SkillLogicSettings>>
          sBinding(vm, "SkillLogicSettings");
      sBinding.Var("FunctionID", &SkillLogicSettings::FunctionID)
          .Var("HasActivationValidation",
               &SkillLogicSettings::HasActivationValidation)
          .Var("HasExecutionValidation",
               &SkillLogicSettings::HasExecutionValidation)
          .Var("HasCostAdjustment", &SkillLogicSettings::HasCostAdjustment)
          .Var("HasPreAction", &SkillLogicSettings::HasPreAction)
          .Var("HasPostAction", &SkillLogicSettings::HasPostAction);
      Sqrat::RootTable(vm).Bind("SkillLogicSettings", sBinding);
    }

    bool invalid = false;

    Sqrat::Function f(Sqrat::RootTable(vm), "prepare");
    if (!f.IsNull()) {
      auto result = f.Evaluate<int>(settings);
      if (!result || (*result != 0) || !settings->FunctionID) {
        // Unknownw error or no function ID specified
        invalid = true;
      } else if (settings->HasActivationValidation &&
                 (Sqrat::Function(Sqrat::RootTable(vm), "validateActivation"))
                     .IsNull()) {
        // No activation validate function
        invalid = true;
      } else if (settings->HasExecutionValidation &&
                 (Sqrat::Function(Sqrat::RootTable(vm), "validateExecution"))
                     .IsNull()) {
        // No execution validate function
        invalid = true;
      } else if (settings->HasCostAdjustment &&
                 (Sqrat::Function(Sqrat::RootTable(vm), "adjustCost"))
                     .IsNull()) {
        // No cost adjustment function
        invalid = true;
      } else if (settings->HasPreAction &&
                 (Sqrat::Function(Sqrat::RootTable(vm), "preAction"))
                     .IsNull()) {
        // No pre-action function
        invalid = true;
      } else if (settings->HasPostAction &&
                 (Sqrat::Function(Sqrat::RootTable(vm), "postAction"))
                     .IsNull()) {
        // No post-action function
        invalid = true;
      }
    } else {
      // No prepare function, should be caught by ServerDataManager
      invalid = true;
    }

    if (invalid) {
      LogSkillManagerError([def]() {
        return libcomp::String(
                   "Failed to load skill logic settings from script: %1\n")
            .Arg(def->Name);
      });
    } else {
      // Bind more and store script
      Sqrat::Class<ProcessingSkill, Sqrat::NoConstructor<ProcessingSkill>>
          sBinding(vm, "ProcessingSkill");
      sBinding
          // Modifiable vars should be prepared at the very latest
          // during preaction
          .Var("Modifier1", &ProcessingSkill::Modifier1)
          .Var("Modifier2", &ProcessingSkill::Modifier2)
          .Var("TalkAffSuccess", &ProcessingSkill::TalkAffSuccess)
          .Var("TalkAffFailure", &ProcessingSkill::TalkAffFailure)
          .Var("TalkFearSuccess", &ProcessingSkill::TalkFearSuccess)
          .Var("TalkFearFailure", &ProcessingSkill::TalkFearFailure)
          // Remaining vars are not modifiable and should be used for
          // logic only
          .ConstVar("Activated", &ProcessingSkill::Activated)
          .ConstVar("Definition", &ProcessingSkill::Definition)
          .ConstVar("EffectiveSource", &ProcessingSkill::EffectiveSource)
          .ConstVar("PrimaryTarget", &ProcessingSkill::PrimaryTarget)
          .ConstVar("SourceExecutionState",
                    &ProcessingSkill::SourceExecutionState)
          .ConstVar("FusionDemons", &ProcessingSkill::FusionDemons)
          .ConstVar("Nulled", &ProcessingSkill::Nulled)
          .ConstVar("Reflected", &ProcessingSkill::Reflected)
          .ConstVar("Absorbed", &ProcessingSkill::Absorbed)
          .ConstVar("InPvP", &ProcessingSkill::InPvP);
      Sqrat::RootTable(vm).Bind("ProcessingSkill", sBinding);

      Sqrat::Class<SkillTargetResult, Sqrat::NoConstructor<SkillTargetResult>>
          tBinding(vm, "SkillTargetResult");
      tBinding.ConstVar("EntityState", &SkillTargetResult::EntityState)
          .ConstVar("Damage1", &SkillTargetResult::Damage1)
          .ConstVar("Damage1Type", &SkillTargetResult::Damage1Type)
          .ConstVar("Damage2", &SkillTargetResult::Damage2)
          .ConstVar("Damage2Type", &SkillTargetResult::Damage2Type)
          .ConstVar("Flags1", &SkillTargetResult::Flags1)
          .ConstVar("Flags2", &SkillTargetResult::Flags2)
          .ConstVar("TalkFlags", &SkillTargetResult::TalkFlags)
          .ConstVar("PursuitDamage", &SkillTargetResult::PursuitDamage)
          .ConstVar("TechnicalDamage", &SkillTargetResult::TechnicalDamage)
          .ConstVar("HitNull", &SkillTargetResult::HitNull)
          .ConstVar("HitReflect", &SkillTargetResult::HitReflect)
          .ConstVar("HitAbsorb", &SkillTargetResult::HitAbsorb);
      Sqrat::RootTable(vm).Bind("SkillTargetResult", tBinding);

      script->Using<AllyState>();
      script->Using<objects::CalculatedEntityState>();
      script->Using<ChannelServer>();
      script->Using<CharacterState>();
      script->Using<DemonState>();
      script->Using<EnemyState>();
      script->Using<Zone>();
      script->Using<libcomp::Randomizer>();

      mSkillLogicScripts[settings->FunctionID] = script;
      mSkillLogicSettings[settings->FunctionID] = settings;
    }
  }
}

bool SkillManager::ActivateSkill(
    const std::shared_ptr<ActiveEntityState> source, uint32_t skillID,
    int64_t activationObjectID, int64_t targetObjectID, uint8_t targetType,
    std::shared_ptr<SkillExecutionContext> ctx) {
  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto tokuseiManager = server->GetTokuseiManager();
  auto def = definitionManager->GetSkillData(skillID);
  if (nullptr == def) {
    return false;
  }

  auto now = ChannelServer::GetServerTime();
  auto client =
      server->GetManagerConnection()->GetEntityClient(source->GetEntityID());
  auto skillBasic = def->GetBasic();
  SkillActivationType_t activationType = skillBasic->GetActivationType();

  // Check for cooldown first
  source->ExpireStatusTimes(now);
  if (source->GetSkillCooldowns(skillBasic->GetCooldownID())) {
    SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::COOLING_DOWN);
    return false;
  } else if ((source->StatusTimesKeyExists(STATUS_LOCKOUT) ||
              source->StatusTimesKeyExists(STATUS_KNOCKBACK)) &&
             (activationType != SkillActivationType_t::INSTANT || !ctx ||
              !ctx->IgnoreAvailable)) {
    SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::SILENT_FAIL);
    return false;
  }

  // Check additional restrictions
  if (SkillRestricted(source, def, activationObjectID, ctx)) {
    SendFailure(source, skillID, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  } else if (SkillZoneRestricted(skillID, source->GetZone())) {
    if (def->GetDamage()->GetFunctionID() == SVR_CONST.SKILL_SPAWN) {
      // Special error message
      SendFailure(source, skillID, client,
                  (uint8_t)SkillErrorCodes_t::NOTHING_HAPPENED_HERE);
    } else {
      SendFailure(source, skillID, client,
                  (uint8_t)SkillErrorCodes_t::RESTRICED_USE);
    }

    return false;
  } else if (tokuseiManager->AspectValueExists(
                 source, TokuseiAspectType::SKILL_LOCK, (double)skillID)) {
    SendFailure(source, skillID, client,
                (uint8_t)SkillErrorCodes_t::RESTRICED_USE);
    return false;
  }

  auto cast = def->GetCast();
  auto castBasic = cast->GetBasic();
  uint32_t defaultChargeTime = castBasic->GetChargeTime();

  // Auto-use activation skills ALWAYS ignore charge time for AI
  // despite this making them look weird from player entities when
  // a charge time is still on the data. Having a charge time on
  // these is incredibly rare and probably not intentional.
  bool autoUse = activationType == SkillActivationType_t::INSTANT;

  auto activated = source->GetActivatedAbility();
  if (activated && !autoUse) {
    // Cancel existing first unless it's still pending execution
    if (activated->GetErrorCode() == -1 &&
        activated->GetExecutionRequestTime()) {
      SendFailure(source, skillID, client,
                  (uint8_t)SkillErrorCodes_t::SILENT_FAIL);
      return false;
    }

    CancelSkill(source, activated->GetActivationID());
  }

  if (autoUse) {
    // Reset default target types as they are typically not provided
    switch (def->GetTarget()->GetType()) {
      case objects::MiTargetData::Type_t::ALLY:
        if (targetObjectID <= 0) {
          targetObjectID = source->GetEntityID();
        }
        break;
      case objects::MiTargetData::Type_t::PARTNER: {
        targetObjectID = -1;
        if (client) {
          auto dState = client->GetClientState()->GetDemonState();
          if (dState->Ready()) {
            targetObjectID = (int64_t)dState->GetEntityID();
          }
        }
      } break;
      default:
        break;
    }
  }

  activated = std::make_shared<objects::ActivatedAbility>();
  activated->SetSkillData(def);
  activated->SetSourceEntity(source);
  activated->SetActivationObjectID(activationObjectID);
  activated->SetTargetObjectID(targetObjectID);
  activated->SetActivationTargetType(targetType);
  activated->SetActivationTime(now);

  auto pSkill = GetProcessingSkill(activated, nullptr);
  if (!CheckScriptValidation(pSkill, false)) {
    SendFailure(source, skillID, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  if (autoUse) {
    // Instant activations are technically not activated
    activated->SetActivationID(-1);
  } else {
    activated->SetActivationID(source->GetNextActivatedAbilityID());
  }

  auto calcState = GetCalculatedState(source, pSkill, false, nullptr);

  // Fusion skills have special adjustment restrictions
  bool fusionSkill = pSkill->FunctionID == SVR_CONST.SKILL_DEMON_FUSION;

  // Stack adjust is affected by 2 sources if not an item skill or just
  // explicit item including adjustments if it is an item skill
  // (Ignore activation type special and toggle)
  uint8_t maxStacks = castBasic->GetUseCount();
  if ((castBasic->GetAdjustRestrictions() & SKILL_FIXED_STACK) == 0 &&
      !fusionSkill && activationType != SkillActivationType_t::SPECIAL &&
      activationType != SkillActivationType_t::ON_TOGGLE) {
    maxStacks = (uint8_t)(
        maxStacks +
        tokuseiManager->GetAspectSum(
            source, TokuseiAspectType::SKILL_ITEM_STACK_ADJUST, calcState) +
        (!pSkill->IsItemSkill
             ? tokuseiManager->GetAspectSum(
                   source, TokuseiAspectType::SKILL_STACK_ADJUST, calcState)
             : 0));
  }

  activated->SetMaxUseCount(maxStacks);

  uint64_t chargedTime = 0;

  bool executeNow =
      autoUse || (defaultChargeTime == 0 &&
                  (activationType == SkillActivationType_t::SPECIAL ||
                   activationType == SkillActivationType_t::ON_TOGGLE));

  // If the skill is not an autoUse, activate it and calculate
  // movement speed
  if (!autoUse) {
    // If the skill needs to charge, see if any time adjustments exist
    uint32_t chargeTime = defaultChargeTime;
    if (pSkill->FunctionID == SVR_CONST.SKILL_SUMMON_DEMON) {
      // Summon charge time is unique from all other skills
      chargeTime = GetSummonSpeed(pSkill, client);
      if (!chargeTime) {
        SendFailure(source, skillID, client,
                    (uint8_t)SkillErrorCodes_t::GENERIC);
        return false;
      }

      executeNow = false;
    } else if (chargeTime > 0 && !fusionSkill &&
               (castBasic->GetAdjustRestrictions() & SKILL_FIXED_CHARGE) == 0) {
      int16_t chargeAdjust =
          (int16_t)(source->GetCorrectValue(CorrectTbl::CHANT_TIME, calcState) -
                    (pSkill->ChargeReduce / 10));
      if (chargeAdjust < 0) {
        chargeAdjust = 0;
      }

      if (chargeAdjust != 100) {
        chargeTime = (uint32_t)ceill(chargeTime * (chargeAdjust * 0.01));
      }
    }

    // Charge time is in milliseconds, convert to microseconds
    chargedTime = now + (chargeTime * 1000);

    activated->SetChargedTime(chargedTime);

    auto speeds = GetMovementSpeeds(source, def);
    activated->SetChargeMoveSpeed(speeds.first);
    activated->SetChargeCompleteMoveSpeed(speeds.second);

    source->SetActivatedAbility(activated);

    if (pSkill->FunctionID) {
      if (mSkillFunctions.find(pSkill->FunctionID) != mSkillFunctions.end()) {
        // Set special activation and let the respective skill handle it
        source->SetSpecialActivations(activated->GetActivationID(), activated);
      }
    }

    SendActivateSkill(pSkill);

    if (!executeNow && def->GetCondition()->GetActiveMPDrain() > 0) {
      // Start pre-cast upkeep
      activated->SetUpkeepCost(def->GetCondition()->GetActiveMPDrain());
      source->ResetUpkeep();
    }
  }

  if (executeNow) {
    LogSkillManagerDebug([source, skillID]() {
      return libcomp::String("%1 instantly executes skill %2.\n")
          .Arg(source->GetEntityLabel())
          .Arg(skillID);
    });

    if (!ExecuteSkill(source, activated, client, ctx)) {
      return false;
    }
  } else {
    LogSkillManagerDebug([source, skillID, activated]() {
      return libcomp::String("%1 activates skill %2[%3].\n")
          .Arg(source->GetEntityLabel())
          .Arg(skillID)
          .Arg(activated->GetActivationID());
    });

    source->SetStatusTimes(STATUS_CHARGING, chargedTime);

    if (activationType == SkillActivationType_t::SPECIAL ||
        activationType == SkillActivationType_t::ON_TOGGLE) {
      // Special/toggle activation skills with a charge time execute
      // automatically when the charge time completes
      server->ScheduleWork(
          chargedTime,
          [](const std::shared_ptr<ChannelServer> pServer,
             const std::shared_ptr<ActiveEntityState> pSource,
             const std::shared_ptr<objects::ActivatedAbility> pActivated,
             const std::shared_ptr<ChannelClientConnection> pClient) {
            auto pSkillManager = pServer->GetSkillManager();
            if (pSkillManager) {
              pSkillManager->ExecuteSkill(pSource, pActivated, pClient, nullptr,
                                          true);
            }
          },
          server, source, activated, client);
    } else if (cast->GetCancel()->GetAutoCancelTime()) {
      ScheduleAutoCancel(source, activated);
    }
  }

  return true;
}

bool SkillManager::TargetSkill(const std::shared_ptr<ActiveEntityState> source,
                               int64_t targetObjectID) {
  bool success = true;

  auto activated = source->GetActivatedAbility();
  if (!activated) {
    success = false;
  } else if (activated->GetExecutionTime() && activated->GetErrorCode() == -1) {
    success = false;
  } else {
    activated->SetTargetObjectID(targetObjectID);
  }

  // No packet response here

  return success;
}

bool SkillManager::ExecuteSkill(const std::shared_ptr<ActiveEntityState> source,
                                int8_t activationID, int64_t targetObjectID,
                                std::shared_ptr<SkillExecutionContext> ctx) {
  auto client = mServer.lock()->GetManagerConnection()->GetEntityClient(
      source->GetEntityID());

  bool success = true;

  auto activated = GetActivation(source, activationID);
  if (!activated) {
    success = false;
  } else {
    // Check if its currently being used or not ready to execute again
    bool notReady = activated->GetExecutionRequestTime() &&
                    !activated->GetHitTime() && activated->GetErrorCode() == -1;
    if (!notReady) {
      source->ExpireStatusTimes(ChannelServer::GetServerTime());
      notReady = source->StatusTimesKeyExists(STATUS_LOCKOUT) ||
                 source->StatusTimesKeyExists(STATUS_KNOCKBACK);
    }

    if (notReady) {
      SendFailure(source, activated->GetSkillData()->GetCommon()->GetID(),
                  client, (uint8_t)SkillErrorCodes_t::SILENT_FAIL);
      return false;
    }

    activated->SetTargetObjectID(targetObjectID);
  }

  if (success && !ExecuteSkill(source, activated, client, ctx)) {
    success = false;
  }

  return success;
}

bool SkillManager::ExecuteSkill(
    std::shared_ptr<ActiveEntityState> source,
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<SkillExecutionContext> ctx, bool delayedAuto) {
  auto skillData = activated->GetSkillData();
  auto zone = source ? source->GetZone() : nullptr;
  if (nullptr == zone) {
    LogSkillManagerErrorMsg("Skill activation attempted outside of a zone.\n");

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
    return false;
  } else if (activated->GetCancelled()) {
    if (!delayedAuto) {
      SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    }

    return false;
  }

  bool invalidSource = !source;
  if (!invalidSource) {
    // The source must be ready and in the active state
    invalidSource = !source->Ready(true) ||
                    source->GetDisplayState() != ActiveDisplayState_t::ACTIVE;
  }

  if (invalidSource) {
    auto state = client ? client->GetClientState() : nullptr;
    if (state) {
      LogSkillManagerError([&]() {
        return libcomp::String(
                   "Invalid source player entity attempted to use skill %1: "
                   "%2\n")
            .Arg(skillData->GetCommon()->GetID())
            .Arg(state->GetAccountUID().ToString());
      });
    }

    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::CONDITION_RESTRICT);
    return false;
  } else if (!source->IsAlive() &&
             (!ctx || !ctx->IgnoreAvailable ||
              skillData->GetBasic()->GetActivationType() !=
                  SkillActivationType_t::INSTANT)) {
    // Do not actually execute from dead entities unless it's a controlled
    // instant activation
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  uint16_t functionID = skillData->GetDamage()->GetFunctionID();
  uint8_t skillCategory =
      skillData->GetCommon()->GetCategory()->GetMainCategory();

  if (skillCategory == SKILL_CATEGORY_PASSIVE ||
      SkillRestricted(source, skillData, activated->GetActivationObjectID(),
                      ctx)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  } else if (mServer.lock()->GetTokuseiManager()->AspectValueExists(
                 source, TokuseiAspectType::SKILL_LOCK,
                 (double)skillData->GetCommon()->GetID())) {
    // Skill may have been locked between activation and execution
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::RESTRICED_USE);
    return false;
  }

  if (functionID != SVR_CONST.SKILL_MOUNT && source->IsMounted()) {
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::MOUNT_OTHER_SKILL_RESTRICT);
    return false;
  }

  // Check FID skill restrictions
  if (functionID) {
    if (functionID == SVR_CONST.SKILL_ZONE_RESTRICTED ||
        functionID == SVR_CONST.SKILL_ZONE_RESTRICTED_ITEM) {
      // Restricted to certain instances which are stored in the
      // group format meaning the smallest two digits are irrelavent
      bool valid = false;

      auto instance = zone->GetInstance();
      uint32_t instGroup =
          (uint32_t)((instance ? instance->GetDefinition()->GetID() : 0) / 100);

      for (int32_t param : skillData->GetSpecial()->GetSpecialParams()) {
        if (param > 0 && (uint32_t)param == instGroup) {
          valid = true;
          break;
        }
      }

      if (!valid) {
        SendFailure(activated, client,
                    (uint8_t)SkillErrorCodes_t::LOCATION_RESTRICT);
        return false;
      }
    } else if (functionID == SVR_CONST.SKILL_STATUS_RESTRICTED ||
               functionID == SVR_CONST.SKILL_STATUS_LIMITED) {
      // Source cannot have the specified status effect(s)
      for (int32_t param : skillData->GetSpecial()->GetSpecialParams()) {
        if (param > 0 && source->StatusEffectActive((uint32_t)param)) {
          SendFailure(activated, client,
                      (uint8_t)SkillErrorCodes_t::GENERIC_USE);
          return false;
        }
      }
    }
  }

  // Stop skills that are demon only instance restricted when not in one
  // as well as non-restricted skills used by an invalid player entity
  bool demonOnlyInst = zone->GetInstanceType() == InstanceType_t::DEMON_ONLY;
  bool instRestrict =
      skillData->GetBasic()->GetFamily() == SkillFamily_t::DEMON_SOLO;
  if ((instRestrict && !demonOnlyInst) ||
      (!instRestrict && demonOnlyInst && client &&
       source->GetEntityType() != EntityType_t::PARTNER_DEMON)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::ZONE_INVALID);
    return false;
  }

  // Check targets
  auto targetType = skillData->GetTarget()->GetType();
  bool checkTargets = targetType != objects::MiTargetData::Type_t::NONE &&
                      functionID != SVR_CONST.SKILL_ZONE_TARGET_ALL;

  // Verify the target now
  if (checkTargets && targetType != objects::MiTargetData::Type_t::OBJECT) {
    // Normal target invalidation reasons do not print an error
    auto targetEntityID = (int32_t)activated->GetTargetObjectID();
    if (targetEntityID <= 0) {
      SendFailure(activated, client);
      return false;
    }

    auto targetEntity = zone->GetActiveEntity(targetEntityID);

    int8_t errCode = ValidateSkillTarget(source, skillData, targetEntity);
    if (errCode != -1) {
      SendFailure(activated, client, (uint8_t)errCode);
      return false;
    }

    // Line of sight required for primary target
    if (!source->HasLineOfSight(targetEntity)) {
      SendFailure(activated, client);
      return false;
    }

    activated->SetEntityTargeted(true);

    LogSkillManagerDebug([source, activated, targetEntity]() {
      return libcomp::String("%1 executes skill %2[%3] targeting %4.\n")
          .Arg(source->GetEntityLabel())
          .Arg(activated->GetSkillData()->GetCommon()->GetID())
          .Arg(activated->GetActivationID())
          .Arg(targetEntity->GetEntityLabel());
    });
  } else {
    LogSkillManagerDebug([source, activated]() {
      return libcomp::String("%1 executes skill %2[%3].\n")
          .Arg(source->GetEntityLabel())
          .Arg(activated->GetSkillData()->GetCommon()->GetID())
          .Arg(activated->GetActivationID());
    });
  }

  // Make sure we have an execution context
  if (!ctx) {
    ctx = std::make_shared<SkillExecutionContext>();
  }

  auto pSkill = GetProcessingSkill(activated, ctx);
  pSkill->SourceExecutionState =
      GetCalculatedState(source, pSkill, false, nullptr);

  if (!DetermineCosts(source, activated, client, ctx)) {
    return false;
  }

  // Reset anything that may have happened from previous attempt
  activated->SetErrorCode(-1);

  activated->SetExecutionRequestTime(ChannelServer::GetServerTime());
  source->RefreshCurrentPosition(activated->GetExecutionTime());

  // Execute the skill
  auto fIter = mSkillFunctions.find(pSkill->FunctionID);
  if (fIter == mSkillFunctions.end()) {
    switch (skillCategory) {
      case SKILL_CATEGORY_ACTIVE:
        return ExecuteNormalSkill(client, activated, ctx);
      case SKILL_CATEGORY_SWITCH:
        return ToggleSwitchSkill(client, activated, ctx);
      case SKILL_CATEGORY_PASSIVE:
      default:
        SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
        return false;
    }
  }

  bool success = fIter->second(*this, activated, ctx, client);
  if (success) {
    FinalizeSkillExecution(client, ctx, activated);
    FinalizeSkill(ctx, activated);
  } else {
    // Skip finalization if performing an instant activation
    if (skillData->GetBasic()->GetActivationType() !=
        SkillActivationType_t::INSTANT) {
      // Clear skill first as it can affect movement speed
      source->SetActivatedAbility(nullptr);
      source->ResetUpkeep();

      SendCompleteSkill(activated, 1);
    }
  }

  return success;
}

bool SkillManager::CancelSkill(const std::shared_ptr<ActiveEntityState> source,
                               int8_t activationID, uint8_t cancelType) {
  auto activated = GetActivation(source, activationID);
  if (!activated) {
    return false;
  } else {
    // If the skill is a special toggle, fire its function again
    auto skillData = activated->GetSkillData();
    auto functionID = skillData->GetDamage()->GetFunctionID();
    auto fIter = mSkillFunctions.find(functionID);
    if (fIter != mSkillFunctions.end() &&
        skillData->GetBasic()->GetActivationType() ==
            SkillActivationType_t::ON_TOGGLE) {
      auto ctx = std::make_shared<SkillExecutionContext>();
      auto client = mServer.lock()->GetManagerConnection()->GetEntityClient(
          source->GetEntityID());
      fIter->second(*this, activated, ctx, client);
    }

    // A skill is considered hit cancelled if its pending a hit
    bool hitCancel = !activated->GetCancelled() && activated->GetHitTime();

    // If any executions have occurred, the cooldown needs to be activated
    if (activated->GetExecuteCount() > 0) {
      auto pSkill = GetProcessingSkill(activated, nullptr);
      SetSkillCompleteState(pSkill, false);
    } else {
      // Make sure to cancel the skill even if it didn't execute
      activated->SetCancelled(true);
    }

    if (hitCancel) {
      // Hit cancellations need to send an empty skill report or the
      // player who used the skill will display a visual effect bug
      // the next time the activation ID rolls around
      libcomp::Packet notify;
      notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
      notify.WriteS32Little(source->GetEntityID());
      notify.WriteU32Little(activated->GetSkillData()->GetCommon()->GetID());
      notify.WriteS8(activated->GetActivationID());
      notify.WriteU32Little(0);  // Nothing hit

      mServer.lock()->GetZoneManager()->BroadcastPacket(source->GetZone(),
                                                        notify);

      LogSkillManagerDebug([source, activated]() {
        return libcomp::String("%1 skill %2[%3] has been hit cancelled.\n")
            .Arg(source->GetEntityLabel())
            .Arg(activated->GetSkillData()->GetCommon()->GetID())
            .Arg(activated->GetActivationID());
      });
    } else {
      LogSkillManagerDebug([source, activated]() {
        return libcomp::String("%1 cancels skill %2[%3].\n")
            .Arg(source->GetEntityLabel())
            .Arg(activated->GetSkillData()->GetCommon()->GetID())
            .Arg(activated->GetActivationID());
      });
    }

    if (source->GetSpecialActivations(activationID) == activated) {
      source->RemoveSpecialActivations(activationID);
    }

    if (source->GetActivatedAbility() == activated) {
      source->SetActivatedAbility(nullptr);
      source->ResetUpkeep();
    }

    SendCompleteSkill(activated, cancelType);
    return true;
  }
}

void SkillManager::SendFailure(
    const std::shared_ptr<ActiveEntityState> source, uint32_t skillID,
    const std::shared_ptr<ChannelClientConnection> client, uint8_t errorCode,
    int8_t activationID) {
  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_FAILED);
  p.WriteS32Little(source ? source->GetEntityID() : -1);
  p.WriteU32Little(skillID);
  p.WriteS8(activationID);
  p.WriteU8(0);  // Unknown (values seen: 0, 1, 2)
  p.WriteU8(errorCode);
  p.WriteS32Little(-1);  // Target entity ID, doesn't seem to be used

  if (client) {
    client->SendPacket(p);
  } else if (source) {
    auto zone = source->GetZone();
    if (zone) {
      auto zConnections = zone->GetConnectionList();
      ChannelClientConnection::BroadcastPacket(zConnections, p);
    }
  }
}

bool SkillManager::SkillRestricted(
    const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<objects::MiSkillData>& skillData,
    int64_t activationObjectID, std::shared_ptr<SkillExecutionContext> ctx) {
  // Digitize skills have a different weapon restricton
  bool isDigiSkill = false;

  bool playerEntity = source->GetEntityType() == EntityType_t::CHARACTER ||
                      source->GetEntityType() == EntityType_t::PARTNER_DEMON;
  if (playerEntity && (!ctx || !ctx->IgnoreAvailable)) {
    // Player entities need to have proper availability to the skill
    bool available = true;
    switch (skillData->GetBasic()->GetFamily()) {
      case SkillFamily_t::NORMAL:
      case SkillFamily_t::MAGIC:
      case SkillFamily_t::SPECIAL:
        // Normal availability required
        available =
            source->CurrentSkillsContains(skillData->GetCommon()->GetID());
        break;
      case SkillFamily_t::FUSION:  // Should be prepared elsewhere
      default:                     // Handle item skills etc via their cost
        break;
    }

    if (!available && source->GetEntityType() == EntityType_t::CHARACTER) {
      // Allow if its a digitalized demon skill
      auto cState = std::dynamic_pointer_cast<CharacterState>(source);
      auto dgState = cState ? cState->GetDigitalizeState() : nullptr;
      isDigiSkill = available = dgState && dgState->ActiveSkillsContains(
                                               skillData->GetCommon()->GetID());
    }

    if (!available) {
      // If not otherwise available, check to see if it is the use skill
      // from an item that is the activation target
      bool isUseSkill = false;
      if (activationObjectID > 0) {
        auto state = ClientState::GetEntityClientState(source->GetEntityID());
        if (state) {
          auto item = std::dynamic_pointer_cast<objects::Item>(
              libcomp::PersistentObject::GetObjectByUUID(
                  state->GetObjectUUID(activationObjectID)));
          if (item) {
            auto itemDef = mServer.lock()->GetDefinitionManager()->GetItemData(
                item->GetType());

            isUseSkill = itemDef && itemDef->GetPossession()->GetUseSkill() ==
                                        skillData->GetCommon()->GetID();
          }
        }
      }

      if (!isUseSkill) {
        return true;
      }
    }
  }

  if (skillData->GetDamage()->GetFunctionID() ==
      SVR_CONST.SKILL_DIASPORA_QUAKE) {
    // The current state of the source is not checked for quakes, they
    // just need to be alive when processed
    return false;
  }

  if (source->StatusRestrictActCount() > 0) {
    return true;
  }

  if (IsTalkSkill(skillData, true) && source->StatusRestrictTalkCount() > 0) {
    return true;
  }

  auto zone = source->GetZone();
  if (zone) {
    // Player entities can by restricted by bases in the zone
    if (playerEntity) {
      auto restricted = zone->GetBaseRestrictedActionTypes();
      if (restricted.size() > 0) {
        int8_t actionType = (int8_t)skillData->GetBasic()->GetActionType();
        if (restricted.find(actionType) != restricted.end()) {
          return true;
        }

        // Check if an item skill is being used
        auto family = skillData->GetBasic()->GetFamily();
        if (restricted.find(-1) != restricted.end() &&
            (family == SkillFamily_t::ITEM ||
             family == SkillFamily_t::DEMON_SOLO)) {
          return true;
        }
      }
    }

    // Make sure we're not PvP restricted
    switch (skillData->GetPvp()->GetPVPRestriction()) {
      case objects::MiSkillPvPData::PVPRestriction_t::PVP_ONLY:
        if (zone && zone->GetInstanceType() != InstanceType_t::PVP) {
          return true;
        }
        break;
      case objects::MiSkillPvPData::PVPRestriction_t::PVP_RESTRICTED:
        if (zone && zone->GetInstanceType() == InstanceType_t::PVP) {
          return true;
        }
        break;
      default:
        break;
    }
  }

  auto restr = skillData->GetCondition()->GetRestriction();
  if (source->GetEntityType() == EntityType_t::CHARACTER) {
    // Check character specific restrictions
    auto cState = std::dynamic_pointer_cast<CharacterState>(source);

    // Verify if the weapon type is valid
    auto weapon = cState->GetEntity()->GetEquippedItems(
        (size_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON);
    auto weaponDef = weapon
                         ? mServer.lock()->GetDefinitionManager()->GetItemData(
                               weapon->GetType())
                         : nullptr;
    if (weapon && !weaponDef) {
      // Sanity check, not a problem to solve here
      return true;
    }

    // "No weapon" counts as close range
    bool longRange =
        weaponDef && weaponDef->GetBasic()->GetWeaponType() ==
                         objects::MiItemBasicData::WeaponType_t::LONG_RANGE;

    // Check digi or normal restriction depending on if the skill
    // was obtained through digi or not. Digi skills consider both
    // digi and normal restrictions.
    if (isDigiSkill &&
        restr->GetDigitizeWeaponType() !=
            objects::MiRestrictionData::DigitizeWeaponType_t::NONE &&
        (longRange !=
         (restr->GetDigitizeWeaponType() ==
          objects::MiRestrictionData::DigitizeWeaponType_t::LONG_RANGE))) {
      return true;
    } else if (restr->GetWeaponType() !=
                   objects::MiRestrictionData::WeaponType_t::NONE &&
               (longRange !=
                (restr->GetWeaponType() ==
                 objects::MiRestrictionData::WeaponType_t::LONG_RANGE))) {
      return true;
    }

    // Check LNC restrictions
    if (restr->GetLNC() != objects::MiRestrictionData::LNC_t::ALL) {
      auto lType = restr->GetLNC();
      switch (cState->GetLNCType()) {
        case LNC_LAW:
          if (lType != objects::MiRestrictionData::LNC_t::LAW &&
              lType != objects::MiRestrictionData::LNC_t::NEUTRAL_LAW &&
              lType != objects::MiRestrictionData::LNC_t::CHAOS_LAW) {
            return true;
          }
          break;
        case LNC_NEUTRAL:
          if (lType != objects::MiRestrictionData::LNC_t::NEUTRAL &&
              lType != objects::MiRestrictionData::LNC_t::NEUTRAL_LAW &&
              lType != objects::MiRestrictionData::LNC_t::CHAOS_NEUTRAL) {
            return true;
          }
          break;
        case LNC_CHAOS:
          if (lType != objects::MiRestrictionData::LNC_t::CHAOS &&
              lType != objects::MiRestrictionData::LNC_t::CHAOS_LAW &&
              lType != objects::MiRestrictionData::LNC_t::CHAOS_NEUTRAL) {
            return true;
          }
          break;
        default:
          break;
      }
    }
  }

  switch (skillData->GetBasic()->GetFamily()) {
    case SkillFamily_t::NORMAL:
      return source->StatusRestrictSpecialCount() > 0;
    case SkillFamily_t::MAGIC:
      return source->StatusRestrictMagicCount() > 0;
    default:
      return false;
  }
}

bool SkillManager::SkillZoneRestricted(uint32_t skillID,
                                       const std::shared_ptr<Zone> zone) {
  if (!zone) {
    return true;
  }

  auto instance = zone->GetInstance();
  auto variant = instance ? instance->GetVariant() : nullptr;
  bool whitelistOnly = variant && variant->GetWhitelistSkillsOnly();

  bool blacklisted = zone->GetDefinition()->SkillBlacklistContains(skillID);
  bool whitelisted = zone->GetDefinition()->SkillWhitelistContains(skillID);

  auto globalDef =
      mServer.lock()->GetServerDataManager()->GetZonePartialData(0);
  if (globalDef) {
    blacklisted |= globalDef->SkillBlacklistContains(skillID);
    whitelisted |= globalDef->SkillWhitelistContains(skillID);
  }

  return whitelistOnly ? !whitelisted : (blacklisted && !whitelisted);
}

bool SkillManager::TargetInRange(
    const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<objects::MiSkillData>& skillData,
    const std::shared_ptr<ActiveEntityState>& target) {
  if (!target) {
    return false;
  } else if (target == source) {
    // Sanity check
    return true;
  }

  target->RefreshCurrentPosition(ChannelServer::GetServerTime());

  float distance =
      source->GetDistance(target->GetCurrentX(), target->GetCurrentY());

  // Occasionally the client will send requests from distances SLIGHTLY off
  // from the allowed range but seemingly only from the partner demon. Allow
  // it up to the source hitbox size.
  uint32_t maxTargetRange =
      (uint32_t)(SKILL_DISTANCE_OFFSET + (target->GetHitboxSize() * 10) +
                 (source->GetHitboxSize() * 10) +
                 (uint32_t)(skillData->GetTarget()->GetRange() * 10));

  return (float)maxTargetRange >= distance;
}

int8_t SkillManager::ValidateSkillTarget(
    const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<objects::MiSkillData>& skillData,
    const std::shared_ptr<ActiveEntityState>& target) {
  // Target must be ready (ignore display state for skills targeting
  // hidden sources)
  if (!target ||
      !target->Ready(target == source && source->GetDisplayState() ==
                                             ActiveDisplayState_t::ACTIVE)) {
    return (int8_t)SkillErrorCodes_t::SILENT_FAIL;
  }

  uint16_t functionID = skillData->GetDamage()->GetFunctionID();
  if (functionID) {
    // Check FID target state restrictions
    bool valid = true;
    if (functionID == SVR_CONST.SKILL_GENDER_RESTRICTED) {
      valid = (int32_t)target->GetGender() ==
              skillData->GetSpecial()->GetSpecialParams(0);
    } else if (functionID == SVR_CONST.SKILL_SLEEP_RESTRICTED) {
      valid = target->StatusEffectActive(SVR_CONST.STATUS_SLEEP);
    }

    if (!valid) {
      return (int8_t)SkillErrorCodes_t::TARGET_INVALID;
    }
  }

  bool targetAlive = target->IsAlive();
  bool allies = source->SameFaction(target);
  auto targetEntityType = target->GetEntityType();
  if (IsTalkSkill(skillData, true)) {
    if (targetEntityType != EntityType_t::ENEMY) {
      return (int8_t)SkillErrorCodes_t::TALK_INVALID;
    }

    auto enemyState = std::dynamic_pointer_cast<EnemyState>(target);
    auto enemy = enemyState ? enemyState->GetEntity() : nullptr;
    auto spawn = enemy ? enemy->GetSpawnSource() : nullptr;

    // Non-spawn and 100% talk resist enemies cannot be
    // negotiated with
    if (!spawn || spawn->GetTalkResist() >= 100) {
      return (int8_t)SkillErrorCodes_t::TALK_INVALID;
    }

    // Talk restrictions apply to source and target
    if (target->StatusRestrictTalkCount() > 0) {
      return (int8_t)SkillErrorCodes_t::TALK_INVALID_STATE;
    }

    if ((spawn->GetTalkResults() & SPAWN_TALK_RESULT_JOIN) == 0) {
      // If an enemy can't join, fail if auto-join skill
      auto talkDamage = skillData->GetDamage()->GetNegotiationDamage();
      if (!talkDamage->GetSuccessAffability() &&
          !talkDamage->GetFailureAffability() &&
          !talkDamage->GetSuccessFear() && !talkDamage->GetFailureFear()) {
        return (int8_t)SkillErrorCodes_t::TARGET_INVALID;
      }
    }

    int8_t targetLvl = target->GetLevel();
    if (targetLvl > source->GetLevel()) {
      return (int8_t)SkillErrorCodes_t::TALK_LEVEL;
    }

    if (!functionID) {
      // No FID, talk skills use level requirements in the params
      auto params = skillData->GetSpecial()->GetSpecialParams();
      if ((params[0] && params[0] > (int32_t)targetLvl) ||
          (params[1] && params[1] < (int32_t)targetLvl)) {
        return (int8_t)SkillErrorCodes_t::TARGET_INVALID;
      }
    }
  }

  auto sourceState = ClientState::GetEntityClientState(source->GetEntityID());
  auto targetState = ClientState::GetEntityClientState(target->GetEntityID());

  bool targetInvalid = false;
  bool targetLivingStateInvalid = !targetAlive;
  switch (skillData->GetTarget()->GetType()) {
    case objects::MiTargetData::Type_t::ALLY:
      targetInvalid = !allies;
      break;
    case objects::MiTargetData::Type_t::DEAD_ALLY:
      targetInvalid = !allies;
      targetLivingStateInvalid = targetAlive;
      if (!targetInvalid) {
        // If reviving and the target is a character (or demon in a
        // demon only instance) and they have not accepted revival,
        // stop here
        bool isRevive = false;
        switch (skillData->GetDamage()->GetBattleDamage()->GetFormula()) {
          case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
          case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
          case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT:
            isRevive = true;
            break;
          default:
            break;
        }

        auto targetClientState =
            ClientState::GetEntityClientState(target->GetEntityID());
        auto zone = target->GetZone();
        if (isRevive && targetClientState && zone) {
          targetInvalid =
              !targetClientState->GetAcceptRevival() &&
              (targetClientState->GetCharacterState() == target ||
               (targetClientState->GetDemonState() == target &&
                zone->GetInstanceType() == InstanceType_t::DEMON_ONLY));
        }
      }
      break;
    case objects::MiTargetData::Type_t::PARTNER:
      targetInvalid = !sourceState ||
                      sourceState->GetCharacterState() != source ||
                      sourceState->GetDemonState() != target;
      break;
    case objects::MiTargetData::Type_t::PARTY:
      targetInvalid =
          !sourceState || !targetState ||
          (sourceState->GetPartyID() &&
           sourceState->GetPartyID() != targetState->GetPartyID()) ||
          (!sourceState->GetPartyID() && sourceState != targetState);
      break;
    case objects::MiTargetData::Type_t::ENEMY:
      targetInvalid = allies || !targetAlive;
      break;
    case objects::MiTargetData::Type_t::DEAD_PARTNER:
      targetInvalid = !sourceState ||
                      sourceState->GetCharacterState() != source ||
                      sourceState->GetDemonState() != target;
      targetLivingStateInvalid = targetAlive;
      break;
    case objects::MiTargetData::Type_t::OTHER_PLAYER:
      targetInvalid = targetEntityType != EntityType_t::CHARACTER ||
                      sourceState == targetState || !allies;
      break;
    case objects::MiTargetData::Type_t::OTHER_DEMON:
      targetInvalid = targetEntityType != EntityType_t::PARTNER_DEMON ||
                      (sourceState && sourceState->GetDemonState() != target) ||
                      !allies;
      break;
    case objects::MiTargetData::Type_t::ALLY_PLAYER:
      targetInvalid = targetEntityType != EntityType_t::CHARACTER || !allies;
      break;
    case objects::MiTargetData::Type_t::ALLY_DEMON:
      targetInvalid =
          targetEntityType != EntityType_t::PARTNER_DEMON || !allies;
      break;
    case objects::MiTargetData::Type_t::PLAYER:
      targetInvalid =
          !sourceState || (sourceState->GetCharacterState() != target &&
                           sourceState->GetDemonState() != target);
      break;
    default:
      break;
  }

  if (targetInvalid) {
    if (target == source) {
      // The client has a very strange habit of setting the target
      // as yourself for skills that should never hit you if no
      // valid target exists (maybe a side effect of ally buff skills
      // that don't NEED a target)
      return (int8_t)SkillErrorCodes_t::SILENT_FAIL;
    } else {
      return (int8_t)SkillErrorCodes_t::TARGET_INVALID;
    }
  } else if (targetLivingStateInvalid) {
    // No message here or skill spammers would be spammed in return
    return (int8_t)SkillErrorCodes_t::SILENT_FAIL;
  }

  return -1;
}

bool SkillManager::ValidateActivationItem(
    const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<objects::Item>& item) {
  if (!item || (item->GetRentalExpiration() > 0 &&
                item->GetRentalExpiration() < (uint32_t)std::time(0))) {
    // Check if the item is invalid or it is an expired rental
    return false;
  } else {
    // Check if its use restricted (applies to equipping too)
    bool valid = true;

    auto definitionManager = mServer.lock()->GetDefinitionManager();
    auto itemDef = definitionManager->GetItemData(item->GetType());
    if (!itemDef) {
      return false;
    }

    // Id item is already equipped, allow it to be unequipped.
    bool equipped = false;
    auto equipType = itemDef->GetBasic()->GetEquipType();
    if (equipType != objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE) {
      // Equippable; check if the item is equipped.
      auto state = ClientState::GetEntityClientState(source->GetEntityID());
      if (state) {
        equipped = state->GetCharacterState()
                       ->GetEntity()
                       ->GetEquippedItems((size_t)equipType)
                       .Get() == item;
      } else {
        valid = false;
      }
    }

    // The item is not currently equipped, so check restrictions.
    if (valid && !equipped) {
      auto restr = itemDef->GetRestriction();
      if (restr->GetLevel()) {
        if (restr->GetLevel() > 100) {
          // Level must be less than or equal to limit - 100
          valid &= source->GetLevel() <= (int8_t)(restr->GetLevel() - 100);
        } else {
          // Level must be greater than or equal to limit
          valid &= source->GetLevel() >= (int8_t)restr->GetLevel();
        }
      }

      switch (restr->GetAlignment()) {
        case objects::MiUseRestrictionsData::Alignment_t::LAW:
          valid &= source->GetLNCType() == LNC_LAW;
          break;
        case objects::MiUseRestrictionsData::Alignment_t::NEUTRAL:
          valid &= source->GetLNCType() == LNC_NEUTRAL;
          break;
        case objects::MiUseRestrictionsData::Alignment_t::CHAOS:
          valid &= source->GetLNCType() == LNC_CHAOS;
          break;
        default:
          break;
      }

      if (restr->GetGender() != GENDER_NA) {
        valid &= source->GetGender() == restr->GetGender();
      }

      auto pvp = itemDef->GetPvp();
      if (pvp->GetGPRequirement() > 0) {
        auto state = ClientState::GetEntityClientState(source->GetEntityID());
        if (state) {
          auto pvpData =
              state->GetCharacterState()->GetEntity()->GetPvPData().Get();
          valid &= pvpData && pvpData->GetGP() >= pvp->GetGPRequirement();
        } else {
          valid = false;
        }
      }
    }

    return valid;
  }
}

bool SkillManager::SkillHasMoreUses(
    const std::shared_ptr<objects::ActivatedAbility>& activated) {
  return activated &&
         activated->GetExecuteCount() < activated->GetMaxUseCount();
}

std::pair<float, float> SkillManager::GetMovementSpeeds(
    const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<objects::MiSkillData>& skillData) {
  float chargeSpeed = 0.f, chargeCompleteSpeed = 0.f;

  // Send movement speed based off skill action type
  switch (skillData->GetBasic()->GetActionType()) {
    case objects::MiSkillBasicData::ActionType_t::SPIN:
    case objects::MiSkillBasicData::ActionType_t::RAPID:
    case objects::MiSkillBasicData::ActionType_t::COUNTER:
    case objects::MiSkillBasicData::ActionType_t::DODGE:
      // No movement during or after
      break;
    case objects::MiSkillBasicData::ActionType_t::SHOT:
    case objects::MiSkillBasicData::ActionType_t::TALK:
    case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
    case objects::MiSkillBasicData::ActionType_t::TAUNT:
    case objects::MiSkillBasicData::ActionType_t::SUPPORT:
      // Move after only
      chargeCompleteSpeed = source->GetMovementSpeed(true);
      break;
    case objects::MiSkillBasicData::ActionType_t::GUARD:
      // Move during and after charge (1/2 normal speed)
      chargeSpeed = chargeCompleteSpeed =
          (source->GetMovementSpeed(true) * 0.5f);
      break;
    case objects::MiSkillBasicData::ActionType_t::ATTACK:
    case objects::MiSkillBasicData::ActionType_t::RUSH:
    default:
      // Move during and after charge (normal speed)
      chargeSpeed = chargeCompleteSpeed = source->GetMovementSpeed(true);
      break;
  }

  if (skillData->GetDamage()->GetFunctionID() == SVR_CONST.SKILL_REST) {
    // Rest has a special no movement rule after charging
    chargeCompleteSpeed = 0;
  }

  return std::make_pair(chargeSpeed, chargeCompleteSpeed);
}

bool SkillManager::PrepareFusionSkill(
    const std::shared_ptr<ChannelClientConnection> client, uint32_t& skillID,
    int32_t targetEntityID, int64_t mainDemonID, int64_t compDemonID) {
  if (!client) {
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto zone = state->GetZone();

  if (!zone) {
    return false;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  // If the executing skill is not the expected type, fail now
  auto skillData = definitionManager->GetSkillData(skillID);
  if (!skillData || skillData->GetDamage()->GetFunctionID() !=
                        SVR_CONST.SKILL_DEMON_FUSION_EXECUTE) {
    SendFailure(cState, skillID, client,
                (uint8_t)SkillErrorCodes_t::ACTIVATION_FAILURE);
    return false;
  }

  auto demon1 = std::dynamic_pointer_cast<objects::Demon>(
      libcomp::PersistentObject::GetObjectByUUID(
          state->GetObjectUUID(mainDemonID)));
  auto demon2 = std::dynamic_pointer_cast<objects::Demon>(
      libcomp::PersistentObject::GetObjectByUUID(
          state->GetObjectUUID(compDemonID)));

  // Both demons needed, first summoned, alive, nearby and not using
  // a skill, second in COMP
  auto comp = state->GetCharacterState()->GetEntity()->GetCOMP();
  if (!demon1 || !demon2 || dState->GetEntity() != demon1 ||
      comp.GetUUID() != demon2->GetDemonBox() ||
      dState->GetActivatedAbility()) {
    SendFailure(cState, skillID, client,
                (uint8_t)SkillErrorCodes_t::ACTIVATION_FAILURE);
    return false;
  } else if (!dState->IsAlive()) {
    SendFailure(cState, skillID, client,
                (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
    return false;
  }

  // Demons in valid state, determine skill type
  uint32_t demonType1 = demon1->GetType();
  uint32_t demonType2 = demon2->GetType();

  auto demon1Data = definitionManager->GetDevilData(demonType1);
  auto demon2Data = definitionManager->GetDevilData(demonType2);

  uint32_t baseDemonType1 = demon1Data->GetUnionData()->GetBaseDemonID();
  uint32_t baseDemonType2 = demon2Data->GetUnionData()->GetBaseDemonID();

  // If any special pairings exist for the two demons, use that skill
  bool specialSkill = false;
  for (uint32_t fSkillID :
       definitionManager->GetDevilFusionIDsByDemonID(demonType1)) {
    bool valid = true;

    auto fusionData = definitionManager->GetDevilFusionData(fSkillID);
    for (uint32_t demonType : fusionData->GetRequiredDemons()) {
      auto demonDef = definitionManager->GetDevilData(demonType);
      if (demonDef) {
        uint32_t baseDemonType = demonDef->GetUnionData()->GetBaseDemonID();
        if (baseDemonType != baseDemonType1 &&
            baseDemonType != baseDemonType2) {
          valid = false;
          break;
        }
      }
    }

    if (valid) {
      skillID = fSkillID;
      specialSkill = true;
      break;
    }
  }

  if (!specialSkill) {
    // No special skill found, calculate normal fusion skill
    uint8_t iType = demon2Data->GetGrowth()->GetInheritanceType();
    if (iType > SVR_CONST.DEMON_FUSION_SKILLS.size()) {
      SendFailure(cState, skillID, client,
                  (uint8_t)SkillErrorCodes_t::ACTIVATION_FAILURE);
      return false;
    }

    auto& levels = SVR_CONST.DEMON_FUSION_SKILLS[iType];

    uint8_t magAverage =
        (uint8_t)floor((float)(demon1Data->GetSummonData()->GetMagModifier() +
                               demon2Data->GetSummonData()->GetMagModifier()) /
                       2.f);

    uint8_t magLevel = 4;
    if (magAverage <= 10) {
      magLevel = 0;
    } else if (magAverage <= 15) {
      magLevel = 1;
    } else if (magAverage <= 19) {
      magLevel = 2;
    } else if (magAverage <= 24) {
      magLevel = 3;
    }

    uint8_t fusionAverage =
        (uint8_t)floor((float)(demon1Data->GetBasic()->GetFusionModifier() +
                               demon2Data->GetBasic()->GetFusionModifier()) /
                       2.f);

    uint16_t rankSum = (uint16_t)(magLevel + fusionAverage);
    if (rankSum <= 2) {
      // Level 1
      skillID = levels[0];
    } else if (rankSum <= 5) {
      // Level 2
      skillID = levels[1];
    } else {
      // Level 3
      skillID = levels[2];
    }
  }

  bool targeted =
      skillData->GetTarget()->GetType() != objects::MiTargetData::Type_t::NONE;
  auto target = zone && targetEntityID > 0 && targeted
                    ? zone->GetActiveEntity(targetEntityID)
                    : nullptr;

  // Skill converted, check target as fusion skills cannot have their
  // target set after activation
  skillData = definitionManager->GetSkillData(skillID);
  if (skillData && (target || !targeted)) {
    cState->RefreshCurrentPosition(ChannelServer::GetServerTime());

    // Ranges are checked at activation time instead of execution time
    if (target && !TargetInRange(cState, skillData, target)) {
      SendFailure(cState, skillID, client, (uint8_t)SkillErrorCodes_t::TOO_FAR);
      return false;
    }

    auto zoneManager = server->GetZoneManager();

    // Hide the partner demon now then calculate the demon's position
    // that will be warped to
    dState->SetAIIgnored(true);

    Point cPoint(cState->GetCurrentX(), cState->GetCurrentY());
    Point dPoint(cPoint.x + 150.f, cPoint.y + 100.f);
    float rot = cState->GetCurrentRotation();

    dPoint = zoneManager->RotatePoint(dPoint, cPoint, rot);

    // Make sure its not out of bounds
    if (zone->Collides(Line(cPoint, dPoint), dPoint)) {
      // Correct to character position
      dPoint = cPoint;
    }

    zoneManager->Warp(client, dState, dPoint.x, dPoint.y, rot);
    return true;
  } else {
    SendFailure(cState, skillID, client,
                (uint8_t)SkillErrorCodes_t::ACTIVATION_FAILURE);
    return false;
  }
}

bool SkillManager::BeginSkillExecution(
    std::shared_ptr<ProcessingSkill> pSkill,
    std::shared_ptr<SkillExecutionContext> ctx) {
  auto zone = pSkill->CurrentZone;
  auto activated = pSkill->Activated;
  auto source = activated ? std::dynamic_pointer_cast<ActiveEntityState>(
                                activated->GetSourceEntity())
                          : nullptr;
  if (!source || !zone || source->GetZone() != zone) {
    LogSkillManagerDebug([&]() {
      return libcomp::String(
                 "Fizzling starting skill with no source or a source not in "
                 "the skill's zone: %1\n")
          .Arg(pSkill->SkillID);
    });

    return false;
  }

  auto server = mServer.lock();
  auto client =
      server->GetManagerConnection()->GetEntityClient(source->GetEntityID());

  // Complete delay does not appear to adjust the actual hit timing just
  // if you can counter it before it completes. If its not specified
  // no delay applies at all unless it is also a projectile.
  uint32_t completeDelay =
      pSkill->Definition->GetDischarge()->GetCompleteDelay();
  bool skipDelay = ctx->FastTrack || (!completeDelay && !pSkill->IsProjectile);

  uint64_t now = ChannelServer::GetServerTime();
  source->RefreshCurrentPosition(now);

  uint64_t processTime = 0;
  if (!skipDelay) {
    // If hitstunned, don't start the skill
    source->ExpireStatusTimes(now);
    if (source->StatusTimesKeyExists(STATUS_HIT_STUN)) {
      SendFailure(activated, client);
      return false;
    }

    // Execute the skill now; finalize, calculate damage and effects when
    // it hits
    processTime = activated->GetExecutionRequestTime() + 500000ULL;
  } else {
    processTime = activated->GetExecutionRequestTime();
  }

  if (!ctx->Fizzle) {
    // NRA for the primary target determines how the rest of the skill
    // behaves, causing it to fizzle or be eligible to defend
    switch (pSkill->Definition->GetTarget()->GetType()) {
      case objects::MiTargetData::Type_t::NONE:
        // Source is technically the primary target (though most of
        // these types of skills will filter it out)
        pSkill->PrimaryTarget = source;
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
      case objects::MiTargetData::Type_t::PLAYER: {
        auto targetEntityID = (int32_t)activated->GetTargetObjectID();
        auto targetEntity = zone->GetActiveEntity(targetEntityID);
        if (!targetEntity) {
          // Target is not valid anymore, fizzle it
          ctx->Fizzle = true;
          break;
        }

        if (pSkill->FunctionID != SVR_CONST.SKILL_DEMON_FUSION &&
            targetEntity != source && !ctx->CounteredSkill &&
            !TargetInRange(source, pSkill->Definition, targetEntity)) {
          // Out of range, fail execution (checked at activation time
          // for fusion skills)
          SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::TOO_FAR);
          return false;
        }

        SkillTargetResult target;
        target.EntityState = targetEntity;
        target.CalcState =
            GetCalculatedState(targetEntity, pSkill, true, source);
        GetCalculatedState(source, pSkill, false, targetEntity);

        if (SetNRA(target, *pSkill, false)) {
          // The skill is reflected and the source becomes
          // the primary target (except for specific AoE types)
          switch (pSkill->Definition->GetRange()->GetAreaType()) {
            case objects::MiEffectiveRangeData::AreaType_t::SOURCE_RADIUS:
            case objects::MiEffectiveRangeData::AreaType_t::FRONT_1:
            case objects::MiEffectiveRangeData::AreaType_t::FRONT_2:
            case objects::MiEffectiveRangeData::AreaType_t::SOURCE:
              pSkill->PrimaryTarget = targetEntity;
              break;
            default:
              pSkill->PrimaryTarget = source;
              pSkill->EffectiveSource = targetEntity;
              pSkill->Targets.push_back(target);
              break;
          }

          pSkill->Reflected = target.HitReflect;
          pSkill->NRAAffinity = target.NRAAffinity;
        } else {
          pSkill->PrimaryTarget = targetEntity;

          pSkill->Nulled = target.HitNull;
          pSkill->Absorbed = target.HitAbsorb;
          pSkill->NRAAffinity = target.NRAAffinity;

          // If it had been reflected we wouldn't be here!
          pSkill->Reflected = 0;
        }
      } break;
      case objects::MiTargetData::Type_t::OBJECT:
        // Nothing special to do (for now)
        break;
      default:
        LogSkillManagerError([&]() {
          return libcomp::String("Unknown target type encountered: %1\n")
              .Arg((uint8_t)pSkill->Definition->GetTarget()->GetType());
        });

        ctx->Fizzle = true;
    }
  }

  if (ctx->Fizzle) {
    // Fast track skills fizzle at the end
    if (!ctx->FastTrack) {
      return false;
    }
  }

  // Set again later for projectiles and delayed hits
  activated->SetHitTime(processTime);

  uint32_t hitDelay = pSkill->Definition->GetDischarge()->GetHitDelay();
  if (!pSkill->IsProjectile && !hitDelay) {
    // If the skill can be defended against and it was not nulled or
    // absorbed, check for counter, dodge or guard on the primary target
    // as these kick off immediately. This happens at projectile hit for
    // anything with a projectile.
    if (pSkill->PrimaryTarget && pSkill->PrimaryTarget != source &&
        pSkill->Definition->GetBasic()->GetCombatSkill() && !pSkill->Nulled &&
        !pSkill->Reflected && !pSkill->Absorbed) {
      ApplyPrimaryCounter(source, pSkill, true);
    }
  }

  FinalizeSkillExecution(client, ctx, activated);

  // If the target is rushing back at the source and this skill is not also
  // a rush, interrupt the rush (projectiles cannot interrupt at this point)
  // Skip if using a defense skill, the hit is nulled, absorbed or the target
  // has hitstun null
  if (!ctx->CounteredSkill && !pSkill->IsProjectile && pSkill->PrimaryTarget &&
      pSkill->PrimaryTarget != source && !pSkill->RushStartPoint &&
      !pSkill->Nulled && !pSkill->Absorbed &&
      !pSkill->PrimaryTarget->GetCalculatedState()
           ->ExistingTokuseiAspectsContains(
               (int8_t)TokuseiAspectType::HITSTUN_NULL)) {
    auto tActivated = pSkill->PrimaryTarget->GetActivatedAbility();
    auto tSkillData = tActivated ? tActivated->GetSkillData() : nullptr;
    auto tDischarge = tSkillData ? tSkillData->GetDischarge() : nullptr;
    if (tSkillData &&
        tSkillData->GetBasic()->GetActionType() ==
            objects::MiSkillBasicData::ActionType_t::RUSH &&
        tDischarge->GetShotInterruptible() &&
        source->GetEntityID() == (int32_t)tActivated->GetTargetObjectID()) {
      // The last X% of the rush is not interruptible
      uint64_t hitWindowAdjust =
          (uint64_t)(500000.0 * (double)tDischarge->GetCompleteDelay() * 0.01);
      uint64_t hitTime = (uint64_t)(tActivated->GetHitTime() - hitWindowAdjust);
      if (now < hitTime) {
        CancelSkill(pSkill->PrimaryTarget, tActivated->GetActivationID());
      }
    }
  }

  if (!skipDelay) {
    // Re-pull the process time to handle an updated delay
    processTime = activated->GetHitTime();
    server->ScheduleWork(
        processTime,
        [](std::shared_ptr<ChannelServer> pServer,
           std::shared_ptr<ProcessingSkill> prSkill,
           std::shared_ptr<SkillExecutionContext> pCtx, uint64_t pSyncTime) {
          pServer->GetSkillManager()->CompleteSkillExecution(prSkill, pCtx,
                                                             pSyncTime);
        },
        server, pSkill, ctx, processTime);
  } else {
    return CompleteSkillExecution(pSkill, ctx, processTime);
  }

  return true;
}

bool SkillManager::CompleteSkillExecution(
    std::shared_ptr<ProcessingSkill> pSkill,
    std::shared_ptr<SkillExecutionContext> ctx, uint64_t syncTime) {
  auto zone = pSkill->CurrentZone;
  auto activated = pSkill->Activated;
  auto source = activated ? std::dynamic_pointer_cast<ActiveEntityState>(
                                activated->GetSourceEntity())
                          : nullptr;
  if (!source || !zone || source->GetZone() != zone) {
    LogSkillManagerDebug([&]() {
      return libcomp::String(
                 "Fizzling skill with no source or a source not in the skill's "
                 "zone: %1\n")
          .Arg(pSkill->SkillID);
    });

    Fizzle(ctx);
    return false;
  }

  if (activated->GetCancelled() ||
      (!pSkill->IsProjectile && activated->GetActivationID() != -1 &&
       source->GetActivatedAbility() != activated)) {
    // Skill cancelled or otherwise detached already
    Fizzle(ctx);
    return false;
  }

  // No cancelling past this point, it can only fizzle
  if (!ctx->Fizzle) {
    if (ctx->FastTrack &&
        pSkill->Definition->GetBasic()->GetActionType() !=
            objects::MiSkillBasicData::ActionType_t::COUNTER) {
      // Fizzle fast track skills without line of sight now (excluding
      // counter which always hits at this point)
      auto target =
          zone->GetActiveEntity((int32_t)activated->GetTargetObjectID());
      if (target && !source->HasLineOfSight(target)) {
        ctx->Fizzle = true;
      }
    }
  }

  if (!ctx->FastTrack || ctx->Fizzle) {
    // Continue on with a copy if more uses exist
    pSkill->Activated = activated = FinalizeSkill(ctx, activated);
  }

  if (ctx->Fizzle) {
    Fizzle(ctx);
    return false;
  } else if (ctx->CounteredSkill) {
    // If this is a counter, dodge or guard, defer final processing to
    // the skill being countered
    ctx->CounteredSkill->ExecutionContext->CounteringSkills.push_back(pSkill);
  } else {
    // Determine if we delay the hit or hit right away
    uint64_t delay = 0;
    if (!ctx->FastTrack) {
      uint32_t hitDelay = pSkill->Definition->GetDischarge()->GetHitDelay();
      delay = (uint64_t)(hitDelay * 1000ULL);

      if (pSkill->IsProjectile) {
        auto targetEntityID = (int32_t)activated->GetTargetObjectID();
        auto target = zone->GetActiveEntity(targetEntityID);
        if (!target) {
          // Target is not valid anymore, let it fizzle
          Fizzle(ctx);
          return false;
        }

        uint64_t projectileTime = 0;
        if (!pSkill->Definition->GetTarget()->GetRange()) {
          // Special "skill wasn't cleaned up" condition for a
          // previously ranged skill with no range set. This is
          // still a projectile but it should effectively hit as
          // soon as the minimum time set below passes.
        } else {
          // Determine time from projectile speed and distance
          // (cannot miss from range after execution starts)
          auto now = ChannelServer::GetServerTime();
          target->RefreshCurrentPosition(now);

          double distance = (double)source->GetDistance(target->GetCurrentX(),
                                                        target->GetCurrentY());

          // Projectile speed is measured in how many 10ths of
          // a unit the projectile will traverse per millisecond
          double distAdjust = distance >= SKILL_DISTANCE_OFFSET
                                  ? distance - (double)SKILL_DISTANCE_OFFSET
                                  : 0.0;

          auto discharge = pSkill->Definition->GetDischarge();
          uint32_t projectileSpeed = discharge->GetProjectileSpeed();
          projectileTime = (uint64_t)(
              distAdjust / (double)(projectileSpeed * 10) * 1000000.0);
        }

        if (projectileTime < 100000) {
          // Projectiles require a delay, even if its miniscule. If
          // the projectile will take less than a server tick to
          // hit, let it hit as fast as possible to make timing look
          // more accurate.
          projectileTime = 1;
        }

        delay = (uint64_t)(delay + projectileTime);
      }
    }

    if (delay) {
      delay = syncTime + delay;
      activated->SetHitTime(delay);

      auto server = mServer.lock();
      server->ScheduleWork(
          delay,
          [](std::shared_ptr<ChannelServer> pServer,
             std::shared_ptr<ProcessingSkill> prSkill,
             std::shared_ptr<SkillExecutionContext> pCtx) {
            pServer->GetSkillManager()->ProjectileHit(prSkill, pCtx);
          },
          server, pSkill, ctx);
    } else {
      activated->SetHitTime(syncTime);

      return ProcessSkillResult(activated, ctx);
    }
  }

  return true;
}

void SkillManager::ProjectileHit(std::shared_ptr<ProcessingSkill> pSkill,
                                 std::shared_ptr<SkillExecutionContext> ctx) {
  // If the skill can be defended against and it was not nulled or absorbed,
  // check for counter, dodge or guard on the primary target now that the
  // projectile will hit. Under normal circumstances this will only result
  // in a dodge.
  if (!pSkill->Nulled && !pSkill->Reflected && !pSkill->Absorbed &&
      pSkill->Definition->GetBasic()->GetCombatSkill()) {
    ApplyPrimaryCounter(pSkill->EffectiveSource, pSkill, true);
  }

  ProcessSkillResult(pSkill->Activated, ctx);
}

void SkillManager::SendFailure(
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<ChannelClientConnection> client, uint8_t errorCode) {
  activated->SetErrorCode((int8_t)errorCode);

  if (activated->GetActivationID() == -1) {
    auto pSkill = GetProcessingSkill(activated, nullptr);
    SendExecuteSkillInstant(pSkill, errorCode);
  } else {
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(
        activated->GetSourceEntity());
    SendFailure(source, activated->GetSkillData()->GetCommon()->GetID(), client,
                errorCode, activated->GetActivationID());
  }

  if (activated->GetActivationTargetType() == ACTIVATION_FUSION) {
    // All failures for fusion skills sent once we have an activated
    // ability need to be cancelled or the client will get stuck
    auto source = std::dynamic_pointer_cast<ActiveEntityState>(
        activated->GetSourceEntity());
    CancelSkill(source, activated->GetActivationID());
  }
}

std::shared_ptr<objects::ActivatedAbility> SkillManager::GetActivation(
    const std::shared_ptr<ActiveEntityState> source,
    int8_t activationID) const {
  auto activated = source->GetSpecialActivations(activationID);
  if (activated) {
    return activated;
  }

  activated = source ? source->GetActivatedAbility() : nullptr;
  if (nullptr == activated || activationID != activated->GetActivationID()) {
    LogSkillManagerDebug([source, activationID]() {
      return libcomp::String("Unknown activation ID encountered from %1: %2\n")
          .Arg(source->GetEntityLabel())
          .Arg(activationID);
    });

    return nullptr;
  }

  return activated;
}

bool SkillManager::DetermineCosts(
    std::shared_ptr<ActiveEntityState> source,
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<SkillExecutionContext> ctx) {
  auto pSkill = GetProcessingSkill(activated, ctx);
  uint8_t skillCategory =
      pSkill->Definition->GetCommon()->GetCategory()->GetMainCategory();

  // Skip invalid skill category or deactivating a switch skill
  if (skillCategory != SKILL_CATEGORY_ACTIVE &&
      (skillCategory != SKILL_CATEGORY_SWITCH ||
       source->ActiveSwitchSkillsContains(pSkill->SkillID))) {
    return true;
  }

  // Gather some client specific data if applicable
  ClientState* state = 0;
  std::shared_ptr<objects::Character> character;
  if (client) {
    state = client->GetClientState();
    character = state->GetCharacterState()->GetEntity();
  }

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();

  int32_t hpCost = 0, mpCost = 0;
  uint32_t fGaugeCost = 0;
  uint16_t bulletCost = 0;
  std::unordered_map<uint32_t, uint32_t> itemCosts;

  // Gather the normal costs first
  if (!DetermineNormalCosts(source, pSkill->Definition, hpCost, mpCost,
                            bulletCost, itemCosts,
                            pSkill->SourceExecutionState)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  // Gather special function costs (only applies to client)
  if (pSkill->FunctionID && client) {
    if (pSkill->FunctionID == SVR_CONST.SKILL_SUMMON_DEMON) {
      auto demon = std::dynamic_pointer_cast<objects::Demon>(
          libcomp::PersistentObject::GetObjectByUUID(
              state->GetObjectUUID(activated->GetActivationObjectID())));
      if (demon == nullptr) {
        LogSkillManagerErrorMsg(
            "Attempted to summon a demon that does not exist.\n");

        SendFailure(activated, client,
                    (uint8_t)SkillErrorCodes_t::SUMMON_INVALID);
        return false;
      }

      // Calculate MAG cost (Diaspora cost is always zero)
      if (pSkill->CurrentZone->GetInstanceType() != InstanceType_t::DIASPORA) {
        uint32_t demonType = demon->GetType();
        auto demonData =
            server->GetDefinitionManager()->GetDevilData(demonType);

        int16_t characterLNC = character->GetLNC();
        int16_t demonLNC = demonData->GetBasic()->GetLNC();
        int8_t level = demon->GetCoreStats()->GetLevel();
        uint8_t magMod = demonData->GetSummonData()->GetMagModifier();

        double lncAdjust = characterLNC == 0
                               ? pow(demonLNC, 2.0f)
                               : (pow(abs(characterLNC), -0.06f) *
                                  pow(characterLNC - demonLNC, 2.0f));
        double magAdjust = (double)(level * magMod);

        double mag = (magAdjust * lncAdjust / 18000000.0) + (magAdjust * 0.25);

        int8_t magReduction =
            characterManager->CalculateMagReduction(client, demon);
        if (magReduction > 0) {
          mag = mag * (double)(100 - magReduction) * 0.01;
        }

        uint32_t cost = (uint32_t)round(mag);
        if (cost) {
          itemCosts[SVR_CONST.ITEM_MAGNETITE] = cost;
        }
      }
    } else if (pSkill->FunctionID == SVR_CONST.SKILL_DEMON_FUSION) {
      // Pay MAG and fusion gauge stocks
      auto fusionData =
          server->GetDefinitionManager()->GetDevilFusionData(pSkill->SkillID);
      if (fusionData) {
        int8_t stockCount = fusionData->GetStockCost();
        fGaugeCost = (uint32_t)(stockCount * 10000);

        itemCosts[SVR_CONST.ITEM_MAGNETITE] = fusionData->GetMagCost();
      }
    } else if (pSkill->FunctionID == SVR_CONST.SKILL_DIGITALIZE) {
      auto demon = std::dynamic_pointer_cast<objects::Demon>(
          libcomp::PersistentObject::GetObjectByUUID(
              state->GetObjectUUID(activated->GetActivationObjectID())));
      if (demon == nullptr) {
        LogSkillManagerErrorMsg(
            "Attempted to digitalize with a demon that does not exist.\n");

        SendFailure(activated, client,
                    (uint8_t)SkillErrorCodes_t::SUMMON_INVALID);
        return false;
      }

      // Calculate MAG cost
      uint32_t demonType = demon->GetType();
      auto demonData = server->GetDefinitionManager()->GetDevilData(demonType);

      int16_t characterLNC = character->GetLNC();
      int16_t demonLNC = demonData->GetBasic()->GetLNC();
      int8_t level = demon->GetCoreStats()->GetLevel();
      int8_t dLevel = character->GetProgress()->GetDigitalizeLevels(
          (uint8_t)demonData->GetCategory()->GetRace());
      int8_t mRank = (int8_t)(demon->GetMitamaRank() + 1);

      double lncCost =
          (double)dLevel * pow(characterLNC - demonLNC, 2) * 0.000001;
      double levelCost = (double)level * pow(mRank, 2) * 0.02;
      double dLevelCost = pow(dLevel, 2) * (double)mRank * 1.25;

      uint32_t dgCost = (uint32_t)floor(lncCost + levelCost + dLevelCost);
      if (dgCost) {
        itemCosts[SVR_CONST.ITEM_MAGNETITE] = dgCost;
      }
    } else if (pSkill->FunctionID == SVR_CONST.SKILL_GEM_COST) {
      // Add one crystal matching target race
      int32_t targetEntityID = (int32_t)activated->GetTargetObjectID();
      auto zone = state->GetZone();
      auto target = zone ? zone->GetEnemy(targetEntityID) : nullptr;
      auto demonData = target ? target->GetDevilData() : nullptr;
      if (!demonData) {
        SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
        return false;
      }

      uint8_t raceID = (uint8_t)demonData->GetCategory()->GetRace();
      for (auto& pair : SVR_CONST.DEMON_CRYSTALS) {
        if (pair.second.find(raceID) != pair.second.end()) {
          itemCosts[pair.first] = 1;
        }
      }
    }
  }

  if (pSkill->IsItemSkill) {
    // If using an item skill and the item is a specific type and
    // non-rental but the skill does not specify a cost for it, it is
    // still consumed.
    int64_t targetObjectID = activated->GetActivationObjectID();
    auto item = targetObjectID ? std::dynamic_pointer_cast<objects::Item>(
                                     libcomp::PersistentObject::GetObjectByUUID(
                                         state->GetObjectUUID(targetObjectID)))
                               : nullptr;
    if (item && itemCosts.find(item->GetType()) == itemCosts.end()) {
      auto itemData =
          server->GetDefinitionManager()->GetItemData(item->GetType());
      auto category = itemData->GetCommon()->GetCategory();

      bool isRental = itemData->GetRental()->GetRental() != 0;
      bool isActive = category->GetMainCategory() == ITEM_CATEGORY_ACTIVE;
      bool isGeneric =
          isActive && category->GetSubCategory() == ITEM_SUBCATEGORY_GENERIC;
      bool isDemonInstItem =
          isActive && category->GetSubCategory() == ITEM_SUBCATEGORY_DEMON_SOLO;
      if (!isRental && (isGeneric || isDemonInstItem)) {
        itemCosts[item->GetType()] = 1;
      }
    }
  }

  // Set costs now in case the script uses them
  activated->SetHPCost(hpCost);
  activated->SetMPCost(mpCost);
  activated->SetBulletCost(bulletCost);
  activated->SetItemCosts(itemCosts);

  if (!AdjustScriptCosts(pSkill)) {
    // Clear costs
    activated->SetHPCost(0);
    activated->SetMPCost(0);
    activated->SetBulletCost(0);
    activated->ClearItemCosts();

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  hpCost = activated->GetHPCost();
  mpCost = activated->GetMPCost();

  // Determine if the payment is possible
  auto sourceStats = source->GetCoreStats();
  bool canPay = sourceStats &&
                ((hpCost == 0) || hpCost < sourceStats->GetHP()) &&
                ((mpCost == 0) || mpCost <= sourceStats->GetMP());
  if (activated->ItemCostsCount() > 0 || activated->GetBulletCost() > 0) {
    if (client && character) {
      for (auto itemCost : activated->GetItemCosts()) {
        uint32_t itemCount =
            characterManager->GetExistingItemCount(character, itemCost.first);
        if (itemCount < itemCost.second) {
          canPay = false;
          break;
        }
      }

      if (activated->GetBulletCost() > 0) {
        auto bullets = character->GetEquippedItems(
            (size_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BULLETS);
        if (bullets && bullets->GetRentalExpiration()) {
          // If the bullets are time limited and active, cost
          // becomes 0. If they are not active, the cost cannot
          // be paid.
          if (bullets->GetRentalExpiration() > (uint32_t)std::time(0)) {
            activated->SetBulletCost(0);
          } else {
            canPay = false;
          }
        } else if (!bullets ||
                   bullets->GetStackSize() < activated->GetBulletCost()) {
          canPay = false;
        }
      }
    } else {
      // Non-player entities cannot pay item-based costs
      canPay = false;
    }
  }

  if (fGaugeCost && (!character || character->GetFusionGauge() < fGaugeCost)) {
    canPay = false;
  }

  // Handle costs that can't be paid as expected errors
  if (!canPay) {
    // Clear costs
    activated->SetHPCost(0);
    activated->SetMPCost(0);
    activated->SetBulletCost(0);
    activated->ClearItemCosts();

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_COST);
    return false;
  }

  return true;
}

bool SkillManager::DetermineNormalCosts(
    const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<objects::MiSkillData>& skillData, int32_t& hpCost,
    int32_t& mpCost, uint16_t& bulletCost,
    std::unordered_map<uint32_t, uint32_t>& itemCosts,
    std::shared_ptr<objects::CalculatedEntityState> calcState) {
  hpCost = 0;
  mpCost = 0;
  bulletCost = 0;
  itemCosts.clear();

  // Only characters and partner demons have to pay item costs
  bool noItem = source->GetEntityType() != EntityType_t::CHARACTER &&
                source->GetEntityType() != EntityType_t::PARTNER_DEMON;
  uint32_t hpCostPercent = 0, mpCostPercent = 0;
  for (auto cost : skillData->GetCondition()->GetCosts()) {
    auto num = cost->GetCost();
    bool percentCost =
        cost->GetNumType() == objects::MiCostTbl::NumType_t::PERCENT;
    switch (cost->GetType()) {
      case objects::MiCostTbl::Type_t::HP:
        if (percentCost) {
          hpCostPercent = (uint32_t)(hpCostPercent + num);
        } else {
          hpCost = (int32_t)(hpCost + num);
        }
        break;
      case objects::MiCostTbl::Type_t::MP:
        if (percentCost) {
          mpCostPercent = (uint32_t)(mpCostPercent + num);
        } else {
          mpCost = (int32_t)(mpCost + num);
        }
        break;
      case objects::MiCostTbl::Type_t::ITEM:
        // Ignore explicit zero item costs
        if (!noItem && num) {
          if (percentCost) {
            LogSkillManagerErrorMsg("Item percent cost encountered.\n");

            return false;
          } else {
            auto itemID = cost->GetItem();
            if (itemCosts.find(itemID) == itemCosts.end()) {
              itemCosts[itemID] = 0;
            }
            itemCosts[itemID] = (uint32_t)(itemCosts[itemID] + num);
          }
        }
        break;
      case objects::MiCostTbl::Type_t::BULLET:
        if (!noItem) {
          if (percentCost) {
            LogSkillManagerErrorMsg("Bullet percent cost encountered.\n");

            return false;
          } else {
            bulletCost = (uint16_t)(bulletCost + num);
          }
        }
        break;
      default:
        LogSkillManagerError([&]() {
          return libcomp::String("Unsupported cost type encountered: %1\n")
              .Arg((uint8_t)cost->GetType());
        });

        return false;
    }
  }

  // Get final HP cost
  if (hpCost != 0 || hpCostPercent != 0) {
    hpCost = (int32_t)(hpCost + (int32_t)ceil(((float)hpCostPercent * 0.01f) *
                                              (float)source->GetMaxHP()));

    double multiplier = 1.0;
    if ((skillData->GetCast()->GetBasic()->GetAdjustRestrictions() &
         SKILL_FIXED_HP_COST) == 0) {
      for (double adjust :
           mServer.lock()->GetTokuseiManager()->GetAspectValueList(
               source, TokuseiAspectType::HP_COST_ADJUST, calcState)) {
        multiplier =
            adjust <= -100.0 ? 0.0 : (multiplier * (1.0 + adjust * 0.01));
      }
    }

    hpCost = (int32_t)ceil((double)hpCost * multiplier);

    if (hpCost < 0) {
      hpCost = 0;
    }
  }

  // Get final MP cost
  if (mpCost != 0 || mpCostPercent != 0) {
    mpCost = (int32_t)(mpCost + (int32_t)ceil(((float)mpCostPercent * 0.01f) *
                                              (float)source->GetMaxMP()));

    double multiplier = 1.0;
    if ((skillData->GetCast()->GetBasic()->GetAdjustRestrictions() &
         SKILL_FIXED_MP_COST) == 0) {
      for (double adjust :
           mServer.lock()->GetTokuseiManager()->GetAspectValueList(
               source, TokuseiAspectType::MP_COST_ADJUST, calcState)) {
        multiplier =
            adjust <= -100.0 ? 0.0 : (multiplier * (1.0 + adjust * 0.01));
      }
    }

    mpCost = (int32_t)ceil((double)mpCost * multiplier);

    if (mpCost < 0) {
      mpCost = 0;
    }
  }

  return true;
}

void SkillManager::ScheduleAutoCancel(
    const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<objects::ActivatedAbility>& activated) {
  uint32_t cancelTime =
      activated->GetSkillData()->GetCast()->GetCancel()->GetAutoCancelTime();
  auto zone = source ? source->GetZone() : nullptr;
  if (cancelTime && zone) {
    uint64_t time = ChannelServer::GetServerTime();
    if (time < activated->GetChargedTime()) {
      // If not already charged, start after charge is complete
      time = activated->GetChargedTime();
    }

    time = (uint64_t)(time + (uint64_t)(cancelTime * 1000));

    auto server = mServer.lock();
    server->ScheduleWork(
        time,
        [](SkillManager* skillManager, uint8_t execCount,
           const std::shared_ptr<Zone> pZone,
           const std::shared_ptr<ActiveEntityState> pSource,
           const std::shared_ptr<objects::ActivatedAbility> pActivated) {
          // If the source is still in the zone with the same skill usage
          // pending and its not executing now, cancel it automatically
          if (pSource->GetZone() == pZone &&
              pSource->GetActivatedAbility() == pActivated &&
              pActivated->GetExecuteCount() == execCount &&
              !pActivated->GetExecutionRequestTime()) {
            skillManager->CancelSkill(pSource, pActivated->GetActivationID());
          }
        },
        this, activated->GetExecuteCount(), zone, source, activated);
  }
}

bool SkillManager::FunctionIDMapped(uint16_t functionID) {
  return mSkillFunctions.find(functionID) != mSkillFunctions.end() ||
         mSkillEffectFunctions.find(functionID) != mSkillEffectFunctions.end();
}

bool SkillManager::ExecuteNormalSkill(
    const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<SkillExecutionContext>& ctx) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  if (!source) {
    return false;
  }

  auto zone = source->GetZone();
  if (!zone) {
    return false;
  }

  auto skillData = activated->GetSkillData();

  bool retry = false;
  if (!ctx->FastTrack && IFramesEnabled() &&
      skillData->GetBasic()->GetCombatSkill() &&
      skillData->GetTarget()->GetType() !=
          objects::MiTargetData::Type_t::NONE) {
    // Check if the hit is valid or if another one is already pending
    auto targetEntityID = (int32_t)activated->GetTargetObjectID();
    auto target = zone->GetActiveEntity(targetEntityID);
    if (!target) {
      // Not a retry and not valid
      return false;
    }

    target->ExpireStatusTimes(ChannelServer::GetServerTime());
    if (target->StatusTimesKeyExists(STATUS_KNOCKBACK)) {
      retry = true;
    } else {
      // Hitstun null removes the stagger requirement
      bool targetHitstunNull =
          target->GetCalculatedState()->ExistingTokuseiAspectsContains(
              (int8_t)TokuseiAspectType::HITSTUN_NULL);
      if (!targetHitstunNull &&
          !target->UpdatePendingCombatants(
              source->GetEntityID(), activated->GetExecutionRequestTime())) {
        retry = true;
      }
    }
  }

  if (retry) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::ACTION_RETRY);

    return true;
  }

  auto pSkill = GetProcessingSkill(activated, ctx);
  BeginSkillExecution(pSkill, ctx);

  return true;
}

bool SkillManager::ProcessSkillResult(
    std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<SkillExecutionContext> ctx) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());

  auto server = mServer.lock();
  auto skillData = activated->GetSkillData();

  auto pSkill = GetProcessingSkill(activated, ctx);
  auto zone = pSkill->CurrentZone;
  if (!zone || activated->GetCancelled()) {
    Fizzle(ctx);
    return false;
  }

  if (pSkill->FunctionID == SVR_CONST.SKILL_DEMON_FUSION &&
      !ProcessFusionExecution(source, pSkill)) {
    Fizzle(ctx);
    return false;
  }

  if (pSkill->PrimaryTarget &&
      (pSkill->PrimaryTarget->GetEntityType() == EntityType_t::CHARACTER ||
       pSkill->PrimaryTarget->GetEntityType() == EntityType_t::PARTNER_DEMON)) {
    // If the primary target is a player entity and the player has changed
    // zones, fizzle the skill
    bool targetZoneInvalid = false;
    if (pSkill->PrimaryTarget->GetZone() != zone) {
      targetZoneInvalid = true;
    } else {
      auto targetClient = server->GetManagerConnection()->GetEntityClient(
          pSkill->PrimaryTarget->GetWorldCID(), true);
      auto targetState =
          targetClient ? targetClient->GetClientState() : nullptr;
      targetZoneInvalid = targetState && (!targetState->GetZoneInTime() ||
                                          targetState->GetZoneInTime() >
                                              activated->GetExecutionTime());
    }

    if (targetZoneInvalid) {
      Fizzle(ctx);
      return false;
    }
  }

  ProcessingSkill& skill = *pSkill.get();

  bool initialHitNull = pSkill->Nulled != 0;
  bool initialHitReflect = pSkill->Reflected != 0;
  if (pSkill->Nulled || pSkill->Reflected || pSkill->Absorbed) {
    // Apply original target NRA
    std::shared_ptr<ActiveEntityState> nraTarget;
    if (activated->GetEntityTargeted()) {
      nraTarget =
          zone->GetActiveEntity((int32_t)activated->GetTargetObjectID());
    } else {
      nraTarget = source;
    }

    // Let it fail later if the target doesn't exist
    if (nraTarget) {
      uint8_t nraIdx = 0;
      if (pSkill->Nulled) {
        nraIdx = NRA_NULL;
      } else if (pSkill->Reflected) {
        nraIdx = NRA_REFLECT;
      } else {
        nraIdx = NRA_ABSORB;
      }

      // It's possible at this point for all NRA to have been consumed
      // or lost. In this instance a delayed counter skill can occur.
      bool delayedCounter = false;

      // Attempt to consume a shield first
      CorrectTbl nraAffinity = (CorrectTbl)(pSkill->NRAAffinity + NRA_OFFSET);
      if (!nraTarget->GetNRAShield(nraIdx, nraAffinity, true)) {
        // If a natural chance exists, use that
        auto calcState = GetCalculatedState(nraTarget, pSkill, true, source);
        delayedCounter = nraTarget->GetNRAChance((uint8_t)nraIdx, nraAffinity,
                                                 calcState) == 0;
      }

      if (delayedCounter) {
        // Rest the NRA, and the targets and check if the original
        // primary target defends against it
        pSkill->Nulled = pSkill->Reflected = 0;
        pSkill->Absorbed = false;
        pSkill->Targets.clear();
        pSkill->PrimaryTarget = nraTarget;

        ApplyPrimaryCounter(source, pSkill, false);
      }
    }
  }

  if (pSkill->PrimaryTarget) {
    if (pSkill->PrimaryTarget != source) {
      // Rotate the source to face the target
      float destRot = (float)atan2(
          source->GetCurrentY() - pSkill->PrimaryTarget->GetCurrentY(),
          source->GetCurrentX() - pSkill->PrimaryTarget->GetCurrentX());
      source->SetCurrentRotation(destRot);
      source->SetOriginRotation(destRot);
      source->SetDestinationRotation(destRot);
    }

    if (pSkill->EffectiveSource != source && pSkill->PrimaryTarget == source) {
      // Determine NRA for new primary target and update skill NRA
      SkillTargetResult selfTarget;
      selfTarget.EntityState = source;
      selfTarget.CalcState = GetCalculatedState(source, pSkill, true, source);
      GetCalculatedState(source, pSkill, false, source);

      SetNRA(selfTarget, *pSkill, true);

      pSkill->Nulled = selfTarget.HitNull;
      pSkill->Reflected = selfTarget.HitReflect;
      pSkill->Absorbed = selfTarget.HitAbsorb;
      pSkill->NRAAffinity = selfTarget.NRAAffinity;

      // Check guard/dodge and add to target list now so NRA is
      // not calculated twice below
      ApplySecondaryCounter(source, selfTarget, pSkill);

      skill.Targets.push_back(selfTarget);
    }
  }

  // Determine if the AoE targets should be gathered based on the state
  // of the hit against the primary target
  bool gatherTargets = true;
  if (initialHitNull || initialHitReflect) {
    // AoE targeting from passive avoidance via null or reflect depends
    // on the AoE type
    switch (skillData->GetRange()->GetAreaType()) {
      case objects::MiEffectiveRangeData::AreaType_t::SOURCE_RADIUS:
      case objects::MiEffectiveRangeData::AreaType_t::FRONT_1:
      case objects::MiEffectiveRangeData::AreaType_t::FRONT_2:
      case objects::MiEffectiveRangeData::AreaType_t::SOURCE:
        // Ignore what happened to the primary target completely
        break;
      case objects::MiEffectiveRangeData::AreaType_t::TARGET_RADIUS:
      case objects::MiEffectiveRangeData::AreaType_t::FRONT_3:
        // Double reflect and reflect to null stops all AoE. Otherwise
        // reflect activates AoE on source from target.
        gatherTargets =
            !initialHitNull && !pSkill->Reflected && !pSkill->Nulled;
        break;
      case objects::MiEffectiveRangeData::AreaType_t::NONE:
      case objects::MiEffectiveRangeData::AreaType_t::STRAIGHT_LINE:
      case objects::MiEffectiveRangeData::AreaType_t::SOURCE_RADIUS2:
      default:
        // AoE does nothing upon initial null or reflect
        gatherTargets = false;
        break;
    }
  } else if (pSkill->PrimaryTarget && !pSkill->Reflected && !pSkill->Nulled) {
    // Counter/dodge stop all AoE types
    for (auto& target : pSkill->Targets) {
      if (target.EntityState == pSkill->PrimaryTarget) {
        if (target.HitAvoided) {
          gatherTargets = false;
        }

        break;
      }
    }
  }

  auto effectiveSource = skill.EffectiveSource;
  auto primaryTarget = skill.PrimaryTarget;

  auto skillRange = skillData->GetRange();
  std::list<std::shared_ptr<ActiveEntityState>> effectiveTargets;
  if (pSkill->FunctionID == SVR_CONST.SKILL_ZONE_TARGET_ALL) {
    effectiveTargets = zone->GetActiveEntities();
  } else if (gatherTargets &&
             skillRange->GetAreaType() !=
                 objects::MiEffectiveRangeData::AreaType_t::NONE) {
    // Determine area effects
    // Unlike damage calculations, this will use effectiveSource instead
    // of source since reflects may have changed the context of the skill

    double aoeRange = (double)(skillRange->GetAoeRange() * 10);

    Point srcPoint(effectiveSource->GetCurrentX(),
                   effectiveSource->GetCurrentY());
    if (pSkill->RushStartPoint) {
      srcPoint = *pSkill->RushStartPoint;
    }

    switch (skillRange->GetAreaType()) {
      case objects::MiEffectiveRangeData::AreaType_t::SOURCE:
        // Not exactly an area but skills targetting the source only should
        // pass both this check and area target type filtering for "Ally"
        // or "Source"
        effectiveTargets.push_back(effectiveSource);
        break;
      case objects::MiEffectiveRangeData::AreaType_t::SOURCE_RADIUS:
      case objects::MiEffectiveRangeData::AreaType_t::SOURCE_RADIUS2:
        // AoE range is extended by the hitbox size of the source
        aoeRange = aoeRange + (double)(effectiveSource->GetHitboxSize() * 10.0);

        effectiveTargets = zone->GetActiveEntitiesInRadius(
            srcPoint.x, srcPoint.y, aoeRange, true);
        break;
      case objects::MiEffectiveRangeData::AreaType_t::TARGET_RADIUS:
        // If the primary target is set and the hit was not absorbed,
        // gather other targets
        if (primaryTarget && !skill.Absorbed) {
          // AoE range is not extended
          effectiveTargets = zone->GetActiveEntitiesInRadius(
              primaryTarget->GetCurrentX(), primaryTarget->GetCurrentY(),
              aoeRange, true);
        }
        break;
      case objects::MiEffectiveRangeData::AreaType_t::FRONT_1:
      case objects::MiEffectiveRangeData::AreaType_t::FRONT_2:
      case objects::MiEffectiveRangeData::AreaType_t::FRONT_3:
        // NRA behavior differs between the three but is already filtered
        // at this point so use effective source to calculate. Type 3 is
        // prevented by being absorbed.
        if (!skill.Absorbed ||
            skillRange->GetAreaType() !=
                objects::MiEffectiveRangeData::AreaType_t::FRONT_3) {
          double maxTargetRange =
              (double)(skillData->GetTarget()->GetRange() * 10);

          // Max target range is extended by the hitbox size of the source
          maxTargetRange = maxTargetRange +
                           (double)(effectiveSource->GetHitboxSize() * 10.0);

          // Get entities in range using the target distance
          auto potentialTargets = zone->GetActiveEntitiesInRadius(
              srcPoint.x, srcPoint.y, maxTargetRange, true);

          // Center pointer of the arc
          float sourceRot = ActiveEntityState::CorrectRotation(
              effectiveSource->GetCurrentRotation());

          // AoE range for this is the percentage of a half circle
          // included on either side (ex: 20 would mean 20% of a full
          // radian on both sides is included and 100 would behave like
          // a source radius AoE)
          float maxRotOffset = (float)(aoeRange * 0.001 * libcomp::PI);

          effectiveTargets = ZoneManager::GetEntitiesInFoV(
              potentialTargets, srcPoint.x, srcPoint.y, sourceRot, maxRotOffset,
              true);
        }
        break;
      case objects::MiEffectiveRangeData::AreaType_t::STRAIGHT_LINE:
        if (primaryTarget && skillRange->GetAoeLineWidth() >= 0) {
          // Create a rotated rectangle to represent the line with
          // a designated width equal to the AoE range
          Point dest(primaryTarget->GetCurrentX(),
                     primaryTarget->GetCurrentY());

          // Half width on each side
          float lineWidth = (float)(skillRange->GetAoeLineWidth() * 10) * 0.5f;

          // If not rushing, max length can go beyond the target
          if (skill.Definition->GetBasic()->GetActionType() !=
              objects::MiSkillBasicData::ActionType_t::RUSH) {
            // AoE range is extended by the hitbox size of the source
            aoeRange =
                aoeRange + (double)(effectiveSource->GetHitboxSize() * 10.0);

            dest = server->GetZoneManager()->GetLinearPoint(
                srcPoint.x, srcPoint.y, dest.x, dest.y, (float)aoeRange, false);
          }

          std::list<Point> rect;
          if (dest.y != srcPoint.y) {
            // Set the line rectangle corner points from the source,
            // destination and perpendicular slope

            float pSlope =
                ((dest.x - srcPoint.x) / (dest.y - srcPoint.y)) * -1.f;
            float denom = (float)std::sqrt(1.0f + std::pow(pSlope, 2));

            float xOffset = (float)(lineWidth / denom);
            float yOffset = (float)fabs((pSlope * lineWidth) / denom);

            if (pSlope > 0) {
              rect.push_back(Point(srcPoint.x + xOffset, srcPoint.y + yOffset));
              rect.push_back(Point(srcPoint.x - xOffset, srcPoint.y - yOffset));
              rect.push_back(Point(dest.x - xOffset, dest.y - yOffset));
              rect.push_back(Point(dest.x + xOffset, dest.y + yOffset));
            } else {
              rect.push_back(Point(srcPoint.x - xOffset, srcPoint.y + yOffset));
              rect.push_back(Point(srcPoint.x + xOffset, srcPoint.y - yOffset));
              rect.push_back(Point(dest.x - xOffset, dest.y + yOffset));
              rect.push_back(Point(dest.x + xOffset, dest.y - yOffset));
            }
          } else if (dest.x != srcPoint.x) {
            // Horizontal line, add points directly to +Y/-Y
            rect.push_back(Point(srcPoint.x, srcPoint.y + lineWidth));
            rect.push_back(Point(srcPoint.x, srcPoint.y - lineWidth));
            rect.push_back(Point(dest.x, dest.y - lineWidth));
            rect.push_back(Point(dest.x, dest.y + lineWidth));
          } else {
            // Same point, only add the target
            effectiveTargets.push_back(primaryTarget);
            break;
          }

          // Gather entities in the polygon as well as ones bisected
          // by the boundaries on their hitbox
          uint64_t now = ChannelServer::GetServerTime();
          for (auto t : zone->GetActiveEntities()) {
            if (t == effectiveSource) {
              // Do not check, just add
              effectiveTargets.push_back(t);
              continue;
            }

            t->RefreshCurrentPosition(now);

            Point p(t->GetCurrentX(), t->GetCurrentY());
            if (ZoneManager::PointInPolygon(p, rect,
                                            (float)t->GetHitboxSize() * 10.f)) {
              effectiveTargets.push_back(t);
            }
          }
        }
        break;
      default:
        LogSkillManagerError([&]() {
          return libcomp::String(
                     "Unsupported skill area type encountered: %1\n")
              .Arg((uint8_t)skillRange->GetAreaType());
        });

        Fizzle(ctx);
        return false;
    }
  }

  // Remove all targets that are not ready
  effectiveTargets.remove_if(
      [](const std::shared_ptr<ActiveEntityState>& target) {
        return !target->Ready();
      });

  // Make sure the primary target isn't in here twice and it is also
  // at the front of the list
  if (primaryTarget) {
    effectiveTargets.remove_if(
        [primaryTarget](const std::shared_ptr<ActiveEntityState>& target) {
          return target == primaryTarget;
        });
    effectiveTargets.push_front(primaryTarget);
  }

  // Filter out invalid effective targets
  auto validType = skillRange->GetValidType();
  switch (validType) {
    case objects::MiEffectiveRangeData::ValidType_t::ENEMY:
      effectiveTargets.remove_if(
          [effectiveSource](const std::shared_ptr<ActiveEntityState>& target) {
            return effectiveSource->SameFaction(target) || !target->IsAlive();
          });
      break;
    case objects::MiEffectiveRangeData::ValidType_t::ALLY:
    case objects::MiEffectiveRangeData::ValidType_t::PARTY:
    case objects::MiEffectiveRangeData::ValidType_t::DEAD_ALLY:
    case objects::MiEffectiveRangeData::ValidType_t::DEAD_PARTY: {
      bool deadOnly =
          validType == objects::MiEffectiveRangeData::ValidType_t::DEAD_ALLY ||
          validType == objects::MiEffectiveRangeData::ValidType_t::DEAD_PARTY;
      effectiveTargets.remove_if(
          [effectiveSource,
           deadOnly](const std::shared_ptr<ActiveEntityState>& target) {
            return !effectiveSource->SameFaction(target) ||
                   (deadOnly == target->IsAlive());
          });

      // Work around CAVE setting a validtype of PARTY while setting a
      // targetype of ALLY by skipping further target removal in that case
      if (skillData->GetTarget()->GetType() !=
              objects::MiTargetData::Type_t::ALLY &&
          (validType == objects::MiEffectiveRangeData::ValidType_t::PARTY ||
           validType ==
               objects::MiEffectiveRangeData::ValidType_t::DEAD_PARTY)) {
        // This will result in an empty list if cast by an enemy, though
        // technically it should in that instance
        auto sourceState =
            ClientState::GetEntityClientState(effectiveSource->GetEntityID());
        uint32_t sourcePartyID = sourceState ? sourceState->GetPartyID() : 0;

        effectiveTargets.remove_if(
            [sourceState,
             sourcePartyID](const std::shared_ptr<ActiveEntityState>& target) {
              auto state =
                  ClientState::GetEntityClientState(target->GetEntityID());
              return !state || (!sourcePartyID && state != sourceState) ||
                     (sourcePartyID && state->GetPartyID() != sourcePartyID);
            });
      }
    } break;
    case objects::MiEffectiveRangeData::ValidType_t::SOURCE:
      // Only affect source and partner or summoner
      {
        auto sourceState =
            ClientState::GetEntityClientState(effectiveSource->GetEntityID());
        std::shared_ptr<ActiveEntityState> otherValid;
        if (sourceState) {
          switch (effectiveSource->GetEntityType()) {
            case EntityType_t::CHARACTER:
              otherValid = sourceState->GetDemonState();
              break;
            case EntityType_t::PARTNER_DEMON:
              otherValid = sourceState->GetCharacterState();
              break;
            default:
              // Shouldn't happen
              break;
          }
        }

        effectiveTargets.remove_if(
            [effectiveSource,
             otherValid](const std::shared_ptr<ActiveEntityState>& target) {
              return target != effectiveSource && target != otherValid;
            });
      }
      break;
    default:
      LogSkillManagerError([&]() {
        return libcomp::String(
                   "Unsupported skill valid target type encountered: %1\n")
            .Arg((uint8_t)validType);
      });

      Fizzle(ctx);
      return false;
  }

  // Filter out special target restrictions
  if (pSkill->FunctionID) {
    if (pSkill->FunctionID == SVR_CONST.SKILL_GENDER_RESTRICTED) {
      // Specific gender targets only
      uint8_t gender = (uint8_t)skillData->GetSpecial()->GetSpecialParams(0);
      effectiveTargets.remove_if(
          [gender](const std::shared_ptr<ActiveEntityState>& target) {
            return target->GetGender() != gender;
          });
    } else if (pSkill->FunctionID == SVR_CONST.SKILL_SLEEP_RESTRICTED) {
      // Sleeping targets only
      effectiveTargets.remove_if(
          [](const std::shared_ptr<ActiveEntityState>& target) {
            return !target->StatusEffectActive(SVR_CONST.STATUS_SLEEP);
          });
    }
  }

  // Make sure nothing would be added twice (should only be the initial
  // target under very strange conditions)
  effectiveTargets.remove_if(
      [pSkill](const std::shared_ptr<ActiveEntityState>& eTarget) {
        for (auto& target : pSkill->Targets) {
          if (target.EntityState == eTarget) {
            return true;
          }
        }

        return false;
      });

  if (!ExecuteScriptPreActions(pSkill, effectiveTargets)) {
    Fizzle(ctx);
    return false;
  }

  // Filter down to all valid targets
  uint16_t aoeReflect = 0;
  for (auto effectiveTarget : effectiveTargets) {
    SkillTargetResult target;
    target.PrimaryTarget = effectiveTarget == primaryTarget;
    target.EntityState = effectiveTarget;
    target.CalcState =
        GetCalculatedState(effectiveTarget, pSkill, true, source);
    GetCalculatedState(source, pSkill, false, effectiveTarget);

    // Set NRA
    // If the primary target is still in the set and a reflect did not
    // occur on the original target, apply the initially calculated flags
    // If an AOE target that is not the source is in the set, increase
    // the number of AOE reflections as needed
    bool isSource = effectiveTarget == source;
    if (target.PrimaryTarget && !initialHitReflect) {
      target.HitNull = skill.Nulled;
      target.HitReflect = skill.Reflected;
      target.HitAbsorb = skill.Absorbed;
      target.HitAvoided = skill.Nulled != 0;
      target.NRAAffinity = skill.NRAAffinity;
    } else {
      if (SetNRA(target, skill) && !isSource) {
        aoeReflect++;
      }

      ApplySecondaryCounter(source, target, pSkill);
    }

    skill.Targets.push_back(target);
  }

  // For each time the skill was reflected by an AOE target, target the
  // source again as each can potentially have NRA and damage calculated
  for (uint16_t i = 0; i < aoeReflect; i++) {
    SkillTargetResult target;
    target.EntityState = source;

    // Calculate the effects done to and from the source itself
    target.CalcState = GetCalculatedState(source, pSkill, true, source);
    GetCalculatedState(source, pSkill, false, source);
    SetNRA(target, skill);

    skill.Targets.push_back(target);
  }

  // Apply skill effect functions now that all normal handling is complete
  auto fIter = mSkillEffectFunctions.find(pSkill->FunctionID);
  if (fIter != mSkillEffectFunctions.end()) {
    auto client =
        server->GetManagerConnection()->GetEntityClient(source->GetEntityID());
    fIter->second(*this, activated, ctx, client);
  }

  if (skillData->GetBasic()->GetCombatSkill() && ctx->ApplyAggro) {
    // Update all opponents
    auto characterManager = server->GetCharacterManager();
    for (auto& target : skill.Targets) {
      if (!source->SameFaction(target.EntityState)) {
        characterManager->AddRemoveOpponent(true, source, target.EntityState);
      }
    }
  }

  // Finalize the skill processing
  ProcessSkillResultFinal(pSkill, ctx);

  // Lastly if the skill was countered, finalize those too
  if (ctx->CounteringSkills.size() > 0) {
    for (auto counteringSkill : ctx->CounteringSkills) {
      auto copyCtx = std::make_shared<SkillExecutionContext>(
          *counteringSkill->ExecutionContext);
      ProcessSkillResult(counteringSkill->Activated, copyCtx);

      // Now that we're done make sure we clean up the context pointer
      counteringSkill->ExecutionContext = nullptr;
    }
  }

  // Clean up the related contexts as they are no longer needed
  ctx->CounteringSkills.clear();
  ctx->SubContexts.clear();

  return true;
}

void SkillManager::ProcessSkillResultFinal(
    const std::shared_ptr<ProcessingSkill>& pSkill,
    std::shared_ptr<SkillExecutionContext> ctx) {
  ProcessingSkill& skill = *pSkill.get();

  auto activated = skill.Activated;
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto zone = skill.CurrentZone;

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();
  auto tokuseiManager = server->GetTokuseiManager();
  auto zoneManager = server->GetZoneManager();

  auto damageData = skill.Definition->GetDamage();
  bool hasBattleDamage = damageData->GetBattleDamage()->GetFormula() !=
                         objects::MiBattleDamageData::Formula_t::NONE;
  if (hasBattleDamage) {
    auto battleDamage = damageData->GetBattleDamage();
    if (!CalculateDamage(source, pSkill)) {
      LogSkillManagerError([&]() {
        return libcomp::String("Damage failed to calculate: %1\n")
            .Arg(skill.SkillID);
      });

      return;
    }

    // Now that damage has been calculated, merge final NRA flags in
    for (SkillTargetResult& target : skill.Targets) {
      switch (target.HitNull) {
        case 1:
          target.Flags1 |= FLAG1_BLOCK_PHYS;
          break;
        case 2:
          target.Flags1 |= FLAG1_BLOCK_MAGIC;
          break;
        case 3:
          target.Flags2 |= FLAG2_BARRIER;
          target.Damage1Type = DAMAGE_TYPE_GENERIC;
          break;
        default:
          break;
      }

      switch (target.HitReflect) {
        case 1:
          target.Flags1 |= FLAG1_REFLECT_PHYS;
          break;
        case 2:
          target.Flags1 |= FLAG1_REFLECT_MAGIC;
          break;
        default:
          break;
      }

      if (target.HitAbsorb) {
        target.Flags1 |= FLAG1_ABSORB;
      }
    }

    // Now that damage is calculated, apply drain
    uint8_t hpDrainPercent = battleDamage->GetHPDrainPercent();
    uint8_t mpDrainPercent = battleDamage->GetMPDrainPercent();
    if (skill.Targets.size() > 0 &&
        (hpDrainPercent > 0 || mpDrainPercent > 0)) {
      auto selfTarget = GetSelfTarget(source, skill.Targets, true);
      auto sourceCalc = GetCalculatedState(source, pSkill, false, nullptr);

      // Apply the heal rate taken for the drain amount. Originally
      // this rate was taken from the target but we're just going to
      // assume that was not intentional.
      float healRate = (float)sourceCalc->GetCorrectTbl(
                           (size_t)CorrectTbl::RATE_HEAL_TAKEN) *
                       0.01f;

      int32_t hpDrain = 0, mpDrain = 0;
      for (SkillTargetResult& target : skill.Targets) {
        if (target.Damage1Type == DAMAGE_TYPE_GENERIC && hpDrainPercent > 0) {
          hpDrain = (int32_t)(hpDrain - (int32_t)floorl((float)target.Damage1 *
                                                        (float)hpDrainPercent *
                                                        0.01f * healRate));

          // Check negated limit
          if (hpDrain < -9999) {
            hpDrain = -9999;
          }
        }

        if (target.Damage2Type == DAMAGE_TYPE_GENERIC && mpDrainPercent > 0) {
          mpDrain = (int32_t)(mpDrain - (int32_t)floorl((float)target.Damage2 *
                                                        (float)mpDrainPercent *
                                                        0.01f * healRate));

          // Check negated limit
          if (mpDrain < -9999) {
            mpDrain = -9999;
          }
        }
      }

      // Heal HP/MP if part of the skill even if value is 0
      if (hpDrainPercent) {
        selfTarget->Damage1Type = DAMAGE_TYPE_HEALING;
        selfTarget->Damage1 = hpDrain < 0 ? hpDrain : 0;
      }

      if (mpDrainPercent) {
        selfTarget->Damage2Type = DAMAGE_TYPE_HEALING;
        selfTarget->Damage2 = mpDrain < 0 ? mpDrain : 0;
      }
    }
  }

  // Get knockback info
  auto skillKnockback = damageData->GetKnockBack();
  int8_t kbMod = skillKnockback->GetModifier();
  uint8_t kbType = skillKnockback->GetKnockBackType();
  float kbDistance = (float)(skillKnockback->GetDistance() * 10);
  bool knockbackExists = false;

  bool doTalk = IsTalkSkill(skill.Definition, false) &&
                source->StatusRestrictTalkCount() == 0;
  bool primaryTalk = doTalk && IsTalkSkill(skill.Definition, true);
  uint64_t now = ChannelServer::GetServerTime();
  source->RefreshCurrentPosition(now);

  // Apply calculation results
  std::list<std::pair<std::shared_ptr<ActiveEntityState>, uint8_t>> talkDone;
  for (SkillTargetResult& target : skill.Targets) {
    if (target.HitAvoided) continue;

    auto targetCalc =
        GetCalculatedState(target.EntityState, pSkill, true, source);
    auto calcState =
        GetCalculatedState(source, pSkill, false, target.EntityState);

    target.EntityState->RefreshCurrentPosition(now);
    target.EntityState->ExpireStatusTimes(now);

    bool hpMpSet = false;
    int32_t hpDamage = target.TechnicalDamage + target.PursuitDamage;
    int32_t mpDamage = 0;
    if (hasBattleDamage) {
      for (int i = 0; i < 2; i++) {
        bool hpMode = i == 0;
        int32_t val = i == 0 ? target.Damage1 : target.Damage2;
        uint8_t type = i == 0 ? target.Damage1Type : target.Damage2Type;

        switch (type) {
          case DAMAGE_EXPLICIT_SET:
            if (hpMode) {
              hpDamage = val;
              target.Damage1Type = DAMAGE_TYPE_GENERIC;
              hpMpSet = true;
            } else {
              mpDamage = val;
              target.Damage2Type = DAMAGE_TYPE_GENERIC;
              hpMpSet = true;
            }
            break;
          case DAMAGE_TYPE_HEALING:
          case DAMAGE_TYPE_DRAIN:
            if (hpMode) {
              hpDamage = (int32_t)(hpDamage + val);
            } else {
              mpDamage = (int32_t)(mpDamage + val);
            }
            break;
          default:
            if (hpMode) {
              hpDamage = (int32_t)(hpDamage + val);
            }
            break;
        }
      }
    }

    auto battleDamage = damageData->GetBattleDamage();
    bool applyKnockback = skill.HardStrike && !target.IndirectTarget &&
                          !target.HitAvoided && !target.HitAbsorb;
    if (!applyKnockback && kbMod && kbType != 2) {
      if (battleDamage->GetFormula() ==
          objects::MiBattleDamageData::Formula_t::NONE) {
        // Perform knockback if configured for knockback with no
        // damage possible
        applyKnockback = true;
      } else if ((battleDamage->GetFormula() ==
                      objects::MiBattleDamageData::Formula_t::DMG_NORMAL &&
                  skill.Modifier1 == 0 && skill.Modifier2 == 0) ||
                 (!hpMpSet && hpDamage > 0) || (hpMpSet && hpDamage != -1)) {
        // Also perform knockback if there is normal damage but no
        // damage potential or if damage will be dealt (since effective
        // damage cannot be set to zero past this point)
        applyKnockback = true;
      }
    }

    if (applyKnockback) {
      if (target.EntityState->GetStatusTimes(STATUS_KNOCKBACK)) {
        // Cannot knockback during knockback (or active rush)
        applyKnockback = false;
      } else if (target.GuardModifier) {
        // Guarding prevents all knockback increases
        applyKnockback = false;
      }
    }

    if (applyKnockback) {
      // Check if the source removes knockback
      int32_t kbRemove =
          (int32_t)tokuseiManager->GetAspectSum(
              source, TokuseiAspectType::KNOCKBACK_REMOVE, calcState) *
          100;

      if (!kbRemove ||
          !(kbRemove >= 10000 || RNG(int32_t, 1, 10000) <= kbRemove)) {
        // Source does not remove knockback, so continue
        float kbRecoverBoost =
            (float)(tokuseiManager->GetAspectSum(
                        target.EntityState,
                        TokuseiAspectType::KNOCKBACK_RECOVERY, targetCalc) *
                    0.01);

        // Hard strikes are instant knockbacks if we get here, provided the
        // target does not null knockback
        float kb = target.EntityState->UpdateKnockback(
            now, skill.HardStrike ? -1.f : kbMod, kbRecoverBoost);
        if (kb == 0.f) {
          // STATUS_ADD_KNOCKBACK effects will apply, regardless if target nulls
          // knockback
          target.ApplyAddedKnockbackEffects = true;

          // Check if the target nulls knockback
          int32_t kbNull = (int32_t)tokuseiManager->GetAspectSum(
                               target.EntityState,
                               TokuseiAspectType::KNOCKBACK_NULL, targetCalc) *
                           100;

          if (!target.EntityState->StatusRestrictKnockbackCount() &&
              (!kbNull ||
               !(kbNull >= 10000 || RNG(int32_t, 1, 10000) <= kbNull))) {
            // Knockback not restricted by target's status or nullified by the
            // target, apply knockback itself
            target.Flags1 |= FLAG1_KNOCKBACK;
            target.EffectCancellations |= EFFECT_CANCEL_KNOCKBACK;
            target.CanHitstun = true;
            knockbackExists = true;
          }
        }
      }
    }

    // Now that knockback has been calculated, determine which status
    // effects to apply
    std::set<uint32_t> cancelOnKill;
    if (!(skill.ExecutionContext &&
          !skill.ExecutionContext->ApplyStatusEffects) &&
        !target.IndirectTarget && !target.HitAbsorb) {
      cancelOnKill = HandleStatusEffects(source, target, pSkill);

      if (hpMpSet) {
        // Explicitly setting HP/MP stops all ailment damage
        target.AilmentDamage = 0;
      } else {
        hpDamage += target.AilmentDamage;
      }
    }

    // Now that damage, knockback, and status effects have been calculated for
    // the target, cancel any status effects on the source (which were not just
    // added) that expire on skill execution
    auto selfTarget = GetSelfTarget(source, pSkill->Targets, true, false);
    std::set<uint32_t> ignore;
    if (selfTarget) {
      for (auto& added : selfTarget->AddedStatuses) {
        ignore.insert(added.first);
      }
    }
    source->CancelStatusEffects(EFFECT_CANCEL_SKILL, ignore);

    // If death is applied, kill the target and stop HP damage
    bool targetKilled = false;
    int32_t hpAdjustedSum = 0, mpAdjusted = 0;
    if (target.AddedStatuses.find(SVR_CONST.STATUS_DEATH) !=
        target.AddedStatuses.end()) {
      targetKilled = target.EntityState->SetHPMP(0, -1, false, true, 0,
                                                 hpAdjustedSum, mpAdjusted);
      target.Flags2 |= FLAG2_INSTANT_DEATH;
    }

    // Now apply damage
    if (hpMpSet || hpDamage != 0 || mpDamage != 0) {
      bool targetAlive = target.EntityState->IsAlive();

      // If the target can be killed by the hit, get clench chance
      // but only if ailment damage has not occurred. Suicide skills
      // cannot allow the source to clench and zone target all skills
      // cannot be clenched by anyone.
      int32_t clenchChance = 0;
      if (hpDamage > 0 && targetAlive && !target.AilmentDamage &&
          skill.FunctionID != SVR_CONST.SKILL_ZONE_TARGET_ALL &&
          (skill.FunctionID != SVR_CONST.SKILL_SUICIDE ||
           target.EntityState != source)) {
        // If reflect occurred, a special clench type must be active
        auto clenchType = skill.Reflected
                              ? TokuseiAspectType::CLENCH_REFLECT_CHANCE
                              : TokuseiAspectType::CLENCH_CHANCE;

        clenchChance =
            (int32_t)floor(tokuseiManager->GetAspectSum(
                               target.EntityState, clenchType, targetCalc) *
                           100.0);
      }

      if (!hpMpSet) {
        hpDamage = -hpDamage;
        mpDamage = -mpDamage;
      }

      int32_t hpAdjusted;
      if (target.EntityState->SetHPMP(hpDamage, mpDamage, !hpMpSet, true,
                                      clenchChance, hpAdjusted, mpAdjusted)) {
        // Changed from alive to dead or vice versa
        if (target.EntityState->GetEntityType() == EntityType_t::CHARACTER) {
          // Reset accept revival
          auto targetClientState = ClientState::GetEntityClientState(
              target.EntityState->GetEntityID());
          if (targetClientState) {
            targetClientState->SetAcceptRevival(false);
          }
        }

        if (targetAlive) {
          targetKilled = true;
        } else {
          target.Flags1 |= FLAG1_REVIVAL;
        }
      }

      hpAdjustedSum += hpAdjusted;

      if (hpMpSet) {
        // Correct explicit damage
        target.Damage1 = -hpAdjusted;
        target.Damage2 = -mpAdjusted;
      } else if (hpAdjusted != hpDamage) {
        // HP damage can only change when clench occurs (HP update
        // prevented from being lethal when it should have been).
        if (!target.TechnicalDamage && !target.PursuitDamage) {
          // If no technical or pursuit damage occurs, the number
          // just reduces to the actual damage dealt.
          target.Damage1 = -hpAdjusted;
        } else {
          // Otherwise the overflow numbers display and the entity
          // is left with 1 HP if they're still alive (see followup
          // packet for how we force this).
          target.ClenchOverflow = !targetKilled;
        }

        // I'm going to assume the second flag was SUPPOSED to leave
        // you with 1 HP but the client doesn't do this :(
        target.Flags1 |= FLAG1_CLENCH;
        target.Flags2 |= FLAG2_CLENCH;
      }

      if (mpAdjusted != 0) {
        target.RecalcTriggers.insert(TokuseiConditionType::CURRENT_MP);
      }
    }

    if (hpAdjustedSum != 0) {
      target.RecalcTriggers.insert(TokuseiConditionType::CURRENT_HP);

      if (hpAdjustedSum < 0) {
        target.EffectCancellations |= EFFECT_CANCEL_DAMAGE;
      }
    }

    // If we haven't already set hitstun, check if we can now
    if (!target.CanHitstun) {
      bool nonDamaging = battleDamage->GetFormula() ==
                             objects::MiBattleDamageData::Formula_t::NONE ||
                         skill.Modifier1 == 0;
      bool calcHitstun = false;
      if (hpAdjustedSum < 0) {
        calcHitstun = true;
      } else if (!target.IndirectTarget && nonDamaging && !target.HitAvoided &&
                 !target.HitAbsorb &&
                 skill.Definition->GetDamage()->GetHitStopTime()) {
        // If the skill is non-damaging and the target is
        // "hit", hitstun applies when set
        calcHitstun = true;
      }

      if (calcHitstun) {
        int32_t hitstunNull = (int32_t)tokuseiManager->GetAspectSum(
                                  target.EntityState,
                                  TokuseiAspectType::HITSTUN_NULL, targetCalc) *
                              100;
        target.CanHitstun =
            hitstunNull != 10000 && (target.Flags1 & FLAG1_GUARDED) == 0 &&
            !target.HitAbsorb &&
            (hitstunNull < 0 || RNG(int32_t, 1, 10000) > hitstunNull);
      }
    }

    // Hitstun or damage counts as a hit cancellation
    if ((target.CanHitstun || hpAdjustedSum < 0) && !target.HitAvoided) {
      target.EffectCancellations |= EFFECT_CANCEL_HIT;
    }

    switch (target.EntityState->GetEntityType()) {
      case EntityType_t::ENEMY:
        if (hpAdjustedSum < 0) {
          // If an enemy is damaged by a player character or their
          // partner demon, keep track of the damage for the damage
          // race drop rule
          auto sourceState =
              ClientState::GetEntityClientState(source->GetEntityID());
          if (sourceState) {
            auto worldCID = sourceState->GetWorldCID();

            auto eState =
                std::dynamic_pointer_cast<EnemyState>(target.EntityState);
            auto enemy = eState->GetEntity();
            if (!enemy->DamageSourcesKeyExists(worldCID)) {
              enemy->SetDamageSources(worldCID, (uint64_t)-hpAdjustedSum);
            } else {
              uint64_t damage = enemy->GetDamageSources(worldCID);
              damage = (uint64_t)(damage + (uint64_t)-hpAdjustedSum);
              enemy->SetDamageSources(worldCID, damage);
            }
          }
        }
        break;
      default:
        break;
    }

    // Set the killed state
    if (targetKilled) {
      target.Flags1 |= FLAG1_LETHAL;
      target.EffectCancellations |= EFFECT_CANCEL_DEATH;

      for (uint32_t effectID : cancelOnKill) {
        target.AddedStatuses.erase(effectID);
      }
    }

    if (doTalk && !targetKilled &&
        target.EntityState->GetEntityType() == EntityType_t::ENEMY) {
      bool entityTalkDone = ApplyNegotiationDamage(source, target, pSkill);
      if (!primaryTalk) {
        // Results are adjusted but conversation can't "end" and
        // nothing displays to the player
        target.TalkFlags = 0;
      } else if (entityTalkDone) {
        talkDone.push_back(
            std::make_pair(target.EntityState, target.TalkFlags));
      }
    }
  }

  ApplyZoneSpecificEffects(pSkill);

  std::set<uint32_t> keepEffects;
  if (skill.FunctionID &&
      skill.FunctionID == SVR_CONST.SKILL_SLEEP_RESTRICTED) {
    // Sleep effects are not cancelled by these skills
    keepEffects.insert(SVR_CONST.STATUS_SLEEP);
  }

  // Handle status and skill interruptions
  for (SkillTargetResult& target : skill.Targets) {
    if (target.EffectCancellations) {
      auto eState = target.EntityState;
      uint8_t cancelFlags = target.EffectCancellations;

      bool cancelled = false;

      auto keep = eState->IsAlive() ? keepEffects : std::set<uint32_t>();
      eState->CancelStatusEffects(cancelFlags, cancelled, keep);

      HandleSkillInterrupt(source, target, pSkill);

      if (cancelled) {
        target.RecalcTriggers.insert(TokuseiConditionType::STATUS_ACTIVE);
      }
    }
  }

  // Now that previous effects have been cancelled, add the new ones
  uint32_t effectTime = (uint32_t)std::time(0);
  bool canAddEffects = skill.Definition->GetDamage()->AddStatusesCount() > 0;
  if (!ctx || ctx->ApplyStatusEffects) {
    for (SkillTargetResult& target : skill.Targets) {
      bool ailmentDamage = target.AilmentDamage != 0;
      if (ailmentDamage && target.Damage1Type == DAMAGE_TYPE_NONE) {
        // This will display zero normal damage but it appears to be
        // the only way to get ailment damage to show when its the only
        // damage dealt using the late-game damage timing
        target.Damage1Type = DAMAGE_TYPE_GENERIC;
      }

      if (target.AddedStatuses.size() > 0) {
        auto removed = target.EntityState->AddStatusEffects(
            target.AddedStatuses, definitionManager, effectTime, false);
        for (auto r : removed) {
          target.CancelledStatuses.insert(r);
        }

        target.RecalcTriggers.insert(TokuseiConditionType::STATUS_ACTIVE);
      } else if (canAddEffects && target.Damage1Type == DAMAGE_TYPE_NONE &&
                 target.Damage2Type == DAMAGE_TYPE_NONE && !target.HitAvoided) {
        // If status effects could be added but weren't and the hit was
        // not avoided but no damage was dealt, the target was missed
        target.Damage1Type = target.Damage2Type = DAMAGE_TYPE_MISS;
        target.HitAvoided = true;
      }
    }
  }

  // Recalculate any effects that trigger from the skill effects
  std::unordered_map<int32_t, bool> effectRecalc;
  for (SkillTargetResult& target : skill.Targets) {
    if (target.RecalcTriggers.size() == 0) continue;

    std::unordered_map<int32_t, bool> result;

    auto eState = target.EntityState;
    auto& triggers = target.RecalcTriggers;

    // Anything with a status effect modified needs a full tokusei
    // and stat recalc
    bool statusChanged =
        triggers.find(TokuseiConditionType::STATUS_ACTIVE) != triggers.end();
    if (effectRecalc.find(eState->GetEntityID()) == effectRecalc.end()) {
      if (statusChanged) {
        result = tokuseiManager->Recalculate(eState, true);
      } else {
        result = tokuseiManager->Recalculate(eState, triggers);
      }
    }

    for (auto resultPair : result) {
      if (effectRecalc.find(resultPair.first) == effectRecalc.end()) {
        effectRecalc[resultPair.first] = resultPair.second;
      } else {
        effectRecalc[resultPair.first] |= resultPair.second;
      }
    }

    if (statusChanged && !effectRecalc[eState->GetEntityID()]) {
      characterManager->RecalculateStats(eState);
    }
  }

  // Send negotiation results first since some are dependent upon the
  // skill hit
  if (talkDone.size() > 0) {
    HandleNegotiations(source, zone, talkDone);
  }

  if (!ctx || !ctx->Executed || !ctx->Finalized) {
    // Send right before finishing execution if we haven't already
    auto client =
        server->GetManagerConnection()->GetEntityClient(source->GetEntityID());
    FinalizeSkillExecution(client, ctx, activated);
    FinalizeSkill(ctx, activated);
  }

  bool isDefense = (skill.Definition->GetBasic()->GetActionType() ==
                        objects::MiSkillBasicData::ActionType_t::GUARD ||
                    skill.Definition->GetBasic()->GetActionType() ==
                        objects::MiSkillBasicData::ActionType_t::DODGE) &&
                   skill.PrimaryTarget;

  bool doRush = skill.Definition->GetBasic()->GetActionType() ==
                    objects::MiSkillBasicData::ActionType_t::RUSH &&
                skill.PrimaryTarget;
  if (doRush) {
    // If a rush is countered, do not actually rush
    bool countered = false;
    for (auto& target : skill.Targets) {
      if (target.PrimaryTarget && (target.Flags1 & FLAG1_GUARDED) != 0 &&
          target.HitAvoided) {
        countered = true;
        break;
      }
    }

    if (!countered) {
      SkillTargetResult* selfTarget =
          GetSelfTarget(source, skill.Targets, true);
      selfTarget->Flags1 |= FLAG1_RUSH_MOVEMENT;
    } else {
      doRush = false;
    }
  }

  auto effectiveSource = skill.EffectiveSource;
  auto primaryTarget = skill.PrimaryTarget;
  auto effectiveTarget = primaryTarget ? primaryTarget : effectiveSource;

  uint64_t hitTimings[3];
  uint64_t completeTime =
      activated->GetExecutionTime() +
      (skill.Definition->GetDischarge()->GetStiffness() * 1000);
  uint64_t hitStopTime =
      activated->GetExecutionTime() +
      (skill.Definition->GetDamage()->GetHitStopTime() * 1000);
  uint64_t selfDelay = 0;

  // Knockback time is a fixed 2s (hitstun can extend delay)
  uint64_t kbTime = activated->GetExecutionTime() + 2000000;

  // Make sure the hit stop times aren't somehow before now
  if (hitStopTime < now) {
    hitStopTime = now;
  }

  if (kbTime < now) {
    kbTime = now;
  }

  if (knockbackExists && !doRush && activated->GetLockOutTime() &&
      activated->GetLockOutTime() > now) {
    // Causing knockback results in a longer immobolization period for
    // the source entity but only if they would still be stopped by
    // the lockout time
    GetSelfTarget(source, skill.Targets, true);
    selfDelay = kbTime;
  }

  auto zConnections = zone->GetConnectionList();
  RelativeTimeMap timeMap;

  // The skill report packet can easily go over the max packet size so
  // the targets in the results need to be batched
  std::list<std::list<SkillTargetResult*>> targetBatches;
  std::list<SkillTargetResult*> currentBatch;
  int32_t currentBatchSize = 0;
  for (SkillTargetResult& target : skill.Targets) {
    int32_t currentTargetSize =
        (int32_t)(64 + (target.AddedStatuses.size() * 9) +
                  (target.CancelledStatuses.size() * 4));

    // If the new list size + the header size is larger than the max
    // packet size, move on to the next batch
    if ((uint32_t)(currentBatchSize + currentTargetSize + 15) >
        MAX_CHANNEL_PACKET_SIZE) {
      targetBatches.push_back(currentBatch);
      currentBatch.clear();
      currentBatchSize = currentTargetSize;
    } else {
      currentBatchSize += currentTargetSize;
    }

    currentBatch.push_back(&target);
  }

  // If we get here with an empty target list, send the empty list
  targetBatches.push_back(currentBatch);

  for (auto it = targetBatches.begin(); it != targetBatches.end(); it++) {
    if (it != targetBatches.begin()) {
      timeMap.clear();

      // An execute packet must be sent once per report (even if its
      // identical) or the client starts ignoring the reports
      SendExecuteSkill(pSkill);
    }

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
    p.WriteS32Little(source->GetEntityID());
    p.WriteU32Little(skill.SkillID);
    p.WriteS8(activated->GetActivationID());

    p.WriteU32Little((uint32_t)it->size());
    for (SkillTargetResult* skillTarget : *it) {
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
      bool defended = isDefense && target.EntityState == skill.PrimaryTarget;
      hitTimings[0] = hitTimings[1] = hitTimings[2] = 0;
      if (target.Flags1 & FLAG1_KNOCKBACK) {
        uint8_t kbEffectiveType = kbType;
        if (kbType == 1 && target.PrimaryTarget) {
          // Targets of AOE knockback are treated like default knockback
          kbEffectiveType = 0;
        }

        // Ignore knockback type 2 which is "None"
        Point kbPoint(target.EntityState->GetCurrentX(),
                      target.EntityState->GetCurrentY());
        switch (kbEffectiveType) {
          case 1: {
            // Away from the effective target (ex: AOE explosion)
            kbPoint = zoneManager->MoveRelative(
                target.EntityState, effectiveTarget->GetCurrentX(),
                effectiveTarget->GetCurrentY(), kbDistance, true, now, kbTime);
          } break;
          case 4:
            if (target.EntityState != effectiveTarget) {
              // Move to the same spot as the target entity
              target.EntityState->SetOriginX(target.EntityState->GetCurrentX());
              target.EntityState->SetOriginY(target.EntityState->GetCurrentY());
              target.EntityState->SetOriginTicks(now);

              target.EntityState->SetDestinationX(
                  effectiveTarget->GetCurrentX());
              target.EntityState->SetDestinationY(
                  effectiveTarget->GetCurrentY());
              target.EntityState->SetDestinationTicks(kbTime);
            }
            break;
          case 5: {
            // Position becomes source position
            target.EntityState->SetOriginX(target.EntityState->GetCurrentX());
            target.EntityState->SetOriginY(target.EntityState->GetCurrentY());
            target.EntityState->SetOriginTicks(now);

            target.EntityState->SetDestinationX(source->GetCurrentX());
            target.EntityState->SetDestinationY(source->GetCurrentY());
            target.EntityState->SetDestinationTicks(kbTime);
          } break;
          case 0:
          case 3:  /// @todo: technically this has more spread than 0
          default:
            // Default if not specified, directly away from source
            kbPoint = zoneManager->MoveRelative(
                target.EntityState, effectiveSource->GetCurrentX(),
                effectiveSource->GetCurrentY(), kbDistance, true, now, kbTime);
            break;
        }

        target.EntityState->SetStatusTimes(STATUS_KNOCKBACK, kbTime);

        p.WriteFloat(kbPoint.x);
        p.WriteFloat(kbPoint.y);

        knockedBack = true;
      } else if (target.EntityState == source && doRush) {
        // Set the new location of the rush user
        float dist = source->GetDistance(primaryTarget->GetCurrentX(),
                                         primaryTarget->GetCurrentY());

        hitTimings[0] = now;
        hitTimings[1] = now + 200000ULL;

        // Count rushing as knockback because functionally the same
        // AI and skill rules apply
        target.EntityState->SetStatusTimes(STATUS_KNOCKBACK, hitTimings[1]);

        Point rushPoint = zoneManager->MoveRelative(
            source, primaryTarget->GetCurrentX(), primaryTarget->GetCurrentY(),
            dist + 250.f, false, now, hitTimings[1]);

        p.WriteFloat(rushPoint.x);
        p.WriteFloat(rushPoint.y);

        rushing = true;
      } else {
        p.WriteBlank(8);
      }

      p.WriteFloat(0);  // Unused additional timing value

      // Calculate hit timing
      if (rushing) {
        // Timing calculated above
      } else if (target.CanHitstun) {
        uint64_t effectiveHitStop =
            knockedBack && kbTime > hitStopTime ? kbTime : hitStopTime;
        if (target.Damage1 || defended) {
          // Damage dealt (or defended), determine stun time
          bool extendHitStun = target.AilmentDamageType != 0 || knockedBack;
          if (extendHitStun) {
            // Apply extended hit stop and determine what else may be needed
            hitTimings[0] = knockedBack ? now : completeTime;
            hitTimings[1] = effectiveHitStop;

            if (!target.AilmentDamageType) {
              // End after hit stop
              hitTimings[2] = effectiveHitStop;
            } else {
              // Apply ailment damage after hit stop
              hitTimings[2] = effectiveHitStop + target.AilmentDamageTime;
            }
          } else {
            // Normal hit stop
            hitTimings[2] = effectiveHitStop;
          }

          target.EntityState->SetStatusTimes(STATUS_HIT_STUN, hitTimings[2]);
        } else if (knockedBack) {
          // Normal hit stop time to finish knockback
          hitTimings[0] = now;
          hitTimings[2] = hitTimings[1] = effectiveHitStop;

          target.EntityState->SetStatusTimes(STATUS_HIT_STUN, hitTimings[2]);
        } else if (target.AilmentDamageType != 0) {
          // Only apply ailment stun time
          hitTimings[2] = effectiveHitStop + target.AilmentDamageTime;

          target.EntityState->SetStatusTimes(STATUS_HIT_STUN, hitTimings[2]);
        } else {
          // No damage, just result displays
          hitTimings[2] = completeTime;
        }
      } else if (target.EntityState == source && selfDelay) {
        // Source is quasi-hitstunned until the knockback ends
        source->SetStatusTimes(STATUS_IMMOBILE, selfDelay);
        hitTimings[2] = selfDelay;
      }

      for (size_t i = 0; i < 3; i++) {
        if (hitTimings[i]) {
          timeMap[(uint32_t)(p.Size() + (4 * i))] = hitTimings[i];
        }
      }

      // Double back at the end and write client specific times
      p.WriteBlank(12);

      p.WriteU8(target.TalkFlags);

      std::list<std::shared_ptr<objects::StatusEffect>> addedStatuses;
      std::set<uint32_t> cancelledStatuses;
      if (target.AddedStatuses.size() > 0) {
        // Make sure the added statuses didn't get removed/re-added
        // already for some reason
        auto effects = target.EntityState->GetStatusEffects();
        for (auto added : target.AddedStatuses) {
          if (effects.find(added.first) != effects.end()) {
            addedStatuses.push_back(effects[added.first]);
          }
        }

        for (auto cancelled : target.CancelledStatuses) {
          if (effects.find(cancelled) == effects.end()) {
            cancelledStatuses.insert(cancelled);
          }
        }
      }

      p.WriteU32Little((uint32_t)addedStatuses.size());
      p.WriteU32Little((uint32_t)cancelledStatuses.size());

      for (auto effect : addedStatuses) {
        p.WriteU32Little(effect->GetEffect());
        p.WriteS32Little((int32_t)effect->GetExpiration());
        p.WriteU8(effect->GetStack());
      }

      for (auto cancelled : cancelledStatuses) {
        p.WriteU32Little(cancelled);
      }

      p.WriteU16Little(target.Flags2);
      p.WriteS32Little(target.TechnicalDamage);
      p.WriteS32Little(target.PursuitDamage);
    }

    ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
  }

  // Handle all skill side effects
  std::set<std::shared_ptr<ActiveEntityState>> durabilityHit;
  std::set<std::shared_ptr<ActiveEntityState>> inheritSkill;
  std::set<std::shared_ptr<ActiveEntityState>> revived;
  std::set<std::shared_ptr<ActiveEntityState>> killed;
  std::set<std::shared_ptr<ActiveEntityState>> cOverflow;
  std::list<std::shared_ptr<ActiveEntityState>> aiHit;
  std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
  std::set<int32_t> interruptEvent;

  // Determine if the zone or specific teams need to be tracked
  bool trackZone = false;
  bool diaspora = zone->GetInstanceType() == InstanceType_t::DIASPORA;
  std::set<std::shared_ptr<objects::Team>> trackedTeams;

  bool playerSkill = false;
  switch (source->GetEntityType()) {
    case EntityType_t::CHARACTER:
      durabilityHit.insert(source);
      playerSkill = true;
      break;
    case EntityType_t::PARTNER_DEMON:
      // If any (direct) target didn't auto avoid, raise inheritance
      for (SkillTargetResult& target : skill.Targets) {
        if (!target.IndirectTarget &&
            (!target.HitAvoided || !target.HitNull || !target.HitReflect)) {
          inheritSkill.insert(source);
          break;
        }
      }
      playerSkill = true;
      break;
    default:
      break;
  }

  for (SkillTargetResult& target : skill.Targets) {
    auto eState = target.EntityState;
    auto& triggers = target.RecalcTriggers;

    bool playerEntity = false;
    switch (eState->GetEntityType()) {
      case EntityType_t::CHARACTER:
        if (!target.IndirectTarget && !target.HitAvoided && !target.HitAbsorb) {
          durabilityHit.insert(eState);
        }
        playerEntity = true;
        break;
      case EntityType_t::PARTNER_DEMON:
        // Manual avoids do not raise inheritance, auto avoids do
        if (!target.IndirectTarget &&
            (!target.HitAvoided || target.HitNull || target.HitReflect)) {
          inheritSkill.insert(eState);
        }
        playerEntity = true;
        break;
      default:
        break;
    }

    if (target.ClenchOverflow) {
      cOverflow.insert(eState);
    }

    bool targetRevived = false;
    bool targetKilled = false;
    if (target.Damage1Type == DAMAGE_TYPE_HEALING &&
        (target.Flags1 & FLAG1_REVIVAL) != 0) {
      revived.insert(eState);

      // Set AI ignore
      target.EntityState->SetStatusTimes(STATUS_IGNORE,
                                         now + (uint64_t)AI_REVIVE_IGNORE);
    } else if ((target.Flags1 & FLAG1_LETHAL) != 0) {
      killed.insert(eState);
      targetKilled = true;
    }

    if (playerEntity) {
      // If a player entity is hit by a combat skill while in an event,
      // whether it did damage or not, interrupt the event
      if (skill.Definition->GetBasic()->GetCombatSkill() &&
          eState->HasActiveEvent()) {
        interruptEvent.insert(eState->GetWorldCID());
      }

      // If alive state changed for a character and they are in a tracked
      // zone, notify the rest of the players/teammates
      if ((targetRevived || targetKilled) &&
          eState->GetEntityType() == EntityType_t::CHARACTER &&
          zone->GetDefinition()->GetTrackTeam()) {
        if (diaspora) {
          // Track entire zone
          trackZone = true;
        } else {
          // Track just the teams
          auto state = ClientState::GetEntityClientState(eState->GetEntityID());
          auto team = state ? state->GetTeam() : nullptr;
          if (team) {
            trackedTeams.insert(team);
          }
        }
      }

      // Be sure to update the party display state
      if (targetRevived || targetKilled ||
          triggers.find(TokuseiConditionType::CURRENT_HP) != triggers.end() ||
          triggers.find(TokuseiConditionType::CURRENT_MP) != triggers.end()) {
        displayStateModified.insert(eState);
      }
    }

    if (eState != source && eState->GetAIState() &&
        skill.Definition->GetBasic()->GetCombatSkill()) {
      aiHit.push_back(eState);
    }
  }

  // Process all additional effects
  for (auto entity : cOverflow) {
    // Entities can survive a lethal blow via clench without damage
    // reduction (when pursuit or technical damage occurs) but will
    // sit at 0 HP. The problem arises when the entity regens back to
    // max HP - 1 which is easily noticeable on player entities. The
    // sitting at 0 HP behavior has been observed but to more properly
    // correct this, increase each entity's HP back to 1 via regen
    // client side since it still is server side. Only send this to
    // the same connections sent the skill packets.
    libcomp::Packet p;
    CharacterManager::GetTDamagePacket(p, entity->GetEntityID(), 1, 0);
    ChannelClientConnection::BroadcastPacket(zConnections, p);
  }

  if (interruptEvent.size() > 0) {
    InterruptEvents(interruptEvent);
  }

  if (playerSkill) {
    HandleFusionGauge(pSkill);
  }

  // Update durability (ignore for PvP)
  if (!skill.InPvP) {
    for (auto entity : durabilityHit) {
      HandleDurabilityDamage(entity, pSkill);
    }
  }

  // Update inherited skills
  for (auto entity : inheritSkill) {
    // Even if the hit is avoided, anything that touches the entity will
    // update inheriting skills
    HandleSkillLearning(entity, pSkill);
  }

  // Report each revived entity
  if (revived.size() > 0) {
    HandleRevives(zone, revived, pSkill);
  }

  // Set all killed entities
  if (killed.size() > 0) {
    HandleKills(source, zone, killed);
  }

  // Make sure all AI entities that got attacked are notified
  if (aiHit.size() > 0) {
    server->GetAIManager()->CombatSkillHit(aiHit, source, skill.Definition);
  }

  if (source->GetAIState() && skill.Definition->GetBasic()->GetCombatSkill()) {
    // The skill hit if it wasn't nulled, absorbed or countered
    bool hit =
        !skill.Nulled && !skill.Absorbed && ctx->CounteringSkills.size() == 0;
    server->GetAIManager()->CombatSkillComplete(
        source, activated, skill.Definition, skill.PrimaryTarget, hit);
  }

  // Report all updates to the world
  if (displayStateModified.size() > 0) {
    characterManager->UpdateWorldDisplayState(displayStateModified);
  }

  // Report tracking updates
  if (trackZone) {
    zoneManager->UpdateTrackedZone(zone);
  } else {
    for (auto team : trackedTeams) {
      zoneManager->UpdateTrackedTeam(team);
    }
  }

  // Nothing to fail at this point, just execute post actions
  ExecuteScriptPostActions(pSkill);
}

bool SkillManager::ProcessFusionExecution(
    std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  auto activated = pSkill->Activated;
  auto state = ClientState::GetEntityClientState(source->GetEntityID());
  if (!activated || !state) {
    return false;
  }

  auto dState = state->GetDemonState();
  auto otherDemon = std::dynamic_pointer_cast<objects::Demon>(
      libcomp::PersistentObject::GetObjectByUUID(
          state->GetObjectUUID(activated->GetActivationObjectID())));
  if (!dState || !otherDemon) {
    LogSkillManagerError([source, pSkill]() {
      return libcomp::String(
                 "Fusion skill from %1 attempted with one or more invalid "
                 "demon(s): %2\n")
          .Arg(source->GetEntityLabel())
          .Arg(pSkill->SkillID);
    });

    return false;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  // Stage the secondary fusion demon and store it on the skill. This version
  // has just its own stats and effects since it is not "summoned". This is
  // necessary however to gain benefits from skills and the like.
  auto dState2 = std::make_shared<DemonState>();
  dState2->SetEntity(otherDemon, definitionManager);

  server->GetTokuseiManager()->Recalculate(dState2, false);
  dState2->RecalculateStats(definitionManager);

  pSkill->FusionDemons.push_back(dState);
  pSkill->FusionDemons.push_back(dState2);

  return true;
}

std::shared_ptr<ProcessingSkill> SkillManager::GetProcessingSkill(
    std::shared_ptr<objects::ActivatedAbility> activated,
    std::shared_ptr<SkillExecutionContext> ctx) {
  if (ctx && ctx->Skill) {
    return ctx->Skill;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto skillData = activated->GetSkillData();
  auto talkDamage = skillData->GetDamage()->GetNegotiationDamage();
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto cSource = std::dynamic_pointer_cast<CharacterState>(source);

  auto skill = std::make_shared<ProcessingSkill>();
  skill->SkillID = skillData->GetCommon()->GetID();
  skill->Definition = skillData;
  skill->Activated = activated;
  skill->Modifier1 = skillData->GetDamage()->GetBattleDamage()->GetModifier1();
  skill->Modifier2 = skillData->GetDamage()->GetBattleDamage()->GetModifier2();
  skill->TalkAffSuccess = talkDamage->GetSuccessAffability();
  skill->TalkAffFailure = talkDamage->GetFailureAffability();
  skill->TalkFearSuccess = talkDamage->GetSuccessFear();
  skill->TalkFearFailure = talkDamage->GetFailureFear();
  skill->BaseAffinity = skill->EffectiveAffinity =
      skillData->GetCommon()->GetAffinity();
  skill->EffectiveDependencyType = skillData->GetBasic()->GetDependencyType();
  skill->EffectiveSource = source;
  skill->CurrentZone = source->GetZone();
  skill->InPvP = skill->CurrentZone &&
                 skill->CurrentZone->GetInstanceType() == InstanceType_t::PVP;
  skill->IsItemSkill =
      skillData->GetBasic()->GetFamily() == SkillFamily_t::ITEM ||
      skillData->GetBasic()->GetFamily() == SkillFamily_t::DEMON_SOLO;
  skill->IsProjectile =
      skillData->GetDischarge()->GetProjectileSpeed() &&
      skillData->GetTarget()->GetType() != objects::MiTargetData::Type_t::NONE;
  skill->FunctionID = skillData->GetDamage()->GetFunctionID();

  // Non-combat skills cannot be NRA'd meaning NRA_HEAL was (apparently)
  // never implemented originally
  skill->CanNRA = skillData->GetBasic()->GetCombatSkill() &&
                  (!skill->FunctionID ||
                   skill->FunctionID != SVR_CONST.SKILL_ZONE_TARGET_ALL);

  if (skill->FunctionID &&
      (skill->FunctionID == SVR_CONST.SKILL_ABS_DAMAGE ||
       skill->FunctionID == SVR_CONST.SKILL_ZONE_TARGET_ALL)) {
    skill->AbsoluteDamage = skillData->GetSpecial()->GetSpecialParams(0);
  }

  // Set the expertise and any boosts gained from ranks
  // The expertise type of a skill is determined by the first
  // type listed in the expertise growth list (defaults to attack)
  auto expGrowth = skillData->GetExpertGrowth();
  if (expGrowth.size() > 0) {
    skill->ExpertiseType = expGrowth.front()->GetExpertiseID();
    if (cSource) {
      skill->ExpertiseRankBoost =
          cSource->GetExpertiseRank(skill->ExpertiseType, definitionManager);

      // Calculate charge reduction before any boost extensions
      skill->ChargeReduce = (int16_t)(skill->ExpertiseRankBoost * 2);

      if (skill->ExpertiseType == EXPERTISE_ATTACK) {
        // Attack expertise gains an extra bonus from regal presence
        uint8_t boost2 = cSource->GetExpertiseRank(EXPERTISE_CHAIN_R_PRESENCE,
                                                   definitionManager);
        skill->ExpertiseRankBoost =
            (uint8_t)(skill->ExpertiseRankBoost + boost2);
      }
    }
  }

  // Calculate effective dependency and affinity types if "weapon" is specified
  if (skill->EffectiveDependencyType == SkillDependencyType_t::WEAPON ||
      skill->BaseAffinity == 1) {
    auto weapon =
        cSource ? cSource->GetEntity()
                      ->GetEquippedItems((size_t)objects::MiItemBasicData::
                                             EquipType_t::EQUIP_TYPE_WEAPON)
                      .Get()
                : nullptr;
    auto weaponDef =
        weapon ? definitionManager->GetItemData(weapon->GetType()) : nullptr;

    if (weaponDef) {
      if (skill->EffectiveDependencyType == SkillDependencyType_t::WEAPON) {
        switch (weaponDef->GetBasic()->GetWeaponType()) {
          case objects::MiItemBasicData::WeaponType_t::LONG_RANGE:
            skill->EffectiveDependencyType = SkillDependencyType_t::LNGR;
            break;
          case objects::MiItemBasicData::WeaponType_t::CLOSE_RANGE:
          default:
            // Use default below
            break;
        }
      }

      if (skill->EffectiveAffinity == 1) {
        if (weaponDef->GetBasic()->GetWeaponType() ==
            objects::MiItemBasicData::WeaponType_t::LONG_RANGE) {
          // If the bullet has an affinity, use that instead
          auto bullet = cSource ? cSource->GetEntity()
                                      ->GetEquippedItems(
                                          (size_t)objects::MiItemBasicData::
                                              EquipType_t::EQUIP_TYPE_BULLETS)
                                      .Get()
                                : nullptr;
          auto bulletDef =
              bullet ? definitionManager->GetItemData(bullet->GetType())
                     : nullptr;
          if (bulletDef && bulletDef->GetCommon()->GetAffinity() != 0) {
            skill->EffectiveAffinity = bulletDef->GetCommon()->GetAffinity();
          }
        }

        if (skill->EffectiveAffinity == 1) {
          // Weapon affinity comes from the basic effect (if one is set)
          uint32_t basicEffect = weapon->GetBasicEffect();
          auto bWeaponDef = definitionManager->GetItemData(
              basicEffect ? basicEffect : weapon->GetType());
          if (bWeaponDef) {
            skill->EffectiveAffinity = bWeaponDef->GetCommon()->GetAffinity();
          }
        }

        // Take the lowest value applied tokusei affinity override if one exists
        auto tokuseiOverrides = server->GetTokuseiManager()->GetAspectValueList(
            source, TokuseiAspectType::WEAPON_AFFINITY_OVERRIDE);
        if (tokuseiOverrides.size() > 0) {
          tokuseiOverrides.sort();
          skill->EffectiveAffinity = (uint8_t)tokuseiOverrides.front();
        }

        skill->WeaponAffinity = skill->EffectiveAffinity;
      }
    }

    // If at any point the type cannot be determined,
    // default to strike, close range (ex: no weapon/non-character source)
    if (skill->EffectiveAffinity == 1) {
      skill->EffectiveAffinity = (uint8_t)CorrectTbl::RES_STRIKE - RES_OFFSET;
    }

    if (skill->EffectiveDependencyType == SkillDependencyType_t::WEAPON) {
      skill->EffectiveDependencyType = SkillDependencyType_t::CLSR;
    }
  }

  if (cSource) {
    // Set the knowledge rank for critical and durability adjustment
    switch (skill->EffectiveDependencyType) {
      case SkillDependencyType_t::CLSR:
      case SkillDependencyType_t::CLSR_LNGR_SPELL:
      case SkillDependencyType_t::CLSR_SPELL:
        skill->KnowledgeRank =
            cSource->GetExpertiseRank(EXPERTISE_WEAPON_KNOWLEDGE);
        break;
      case SkillDependencyType_t::LNGR:
      case SkillDependencyType_t::LNGR_CLSR_SPELL:
      case SkillDependencyType_t::LNGR_SPELL:
        skill->KnowledgeRank =
            cSource->GetExpertiseRank(EXPERTISE_GUN_KNOWLEDGE);
        break;
      default:
        break;
    }

    // Magic control lowers charge time
    uint8_t mcRank = cSource->GetExpertiseRank(EXPERTISE_MAGIC_CONTROL);
    skill->ChargeReduce =
        (int16_t)(skill->ChargeReduce + (uint8_t)(mcRank / 10) * 4);
  }

  if (ctx) {
    skill->ExecutionContext = ctx.get();
    ctx->Skill = skill;
  }

  // Lastly set skill definition overrides
  auto sourceState = GetCalculatedState(source, skill, false, nullptr);

  auto affinityOverride = server->GetTokuseiManager()->GetAspectValueList(
      source, TokuseiAspectType::AFFINITY_OVERRIDE, sourceState);
  for (double ovr : affinityOverride) {
    // Skip weapon affinity overrides (includes base affinity because the
    // skill is being redefined, not added to)
    if (ovr > (double)((uint8_t)CorrectTbl::RES_WEAPON - RES_OFFSET) &&
        ovr <= (double)((uint8_t)CorrectTbl::RES_SUICIDE - RES_OFFSET)) {
      skill->BaseAffinity = skill->EffectiveAffinity = (uint8_t)ovr;
      break;
    }
  }

  return skill;
}

std::shared_ptr<objects::CalculatedEntityState>
SkillManager::GetCalculatedState(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<ProcessingSkill>& pSkill, bool isTarget,
    const std::shared_ptr<ActiveEntityState>& otherState) {
  ProcessingSkill& skill = *pSkill.get();

  std::shared_ptr<objects::CalculatedEntityState> calcState;
  if (isTarget) {
    calcState = skill.TargetCalcStates[eState->GetEntityID()];
  } else if (otherState) {
    calcState =
        skill
            .SourceCalcStates[eState->GetEntityID()][otherState->GetEntityID()];
  }

  if (!calcState) {
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    // Determine which tokusei are active and don't need to be calculated again
    if (!isTarget && otherState && skill.SourceExecutionState &&
        eState == pSkill->Activated->GetSourceEntity()) {
      // If we're calculating for a skill target, start with the execution state
      calcState = skill.SourceExecutionState;
    } else {
      // Otherwise start with the base calculated state
      calcState = eState->GetCalculatedState();
    }

    // Keep track of tokusei that are not valid for the skill conditions but
    // CAN become active given the correct target (only valid for source)
    std::unordered_map<int32_t, uint16_t> stillPendingSkillTokusei;

    auto effectiveTokusei = calcState->GetEffectiveTokusei();
    auto pendingSkillTokusei = calcState->GetPendingSkillTokusei();
    auto aspects = calcState->GetExistingTokuseiAspects();

    // Determine if a skill context will change the state calculation
    std::shared_ptr<objects::MiSkillData> contextSkill;
    if (!isTarget) {
      // Always use current skill
      contextSkill = skill.Definition;
    } else {
      // Only apply guard stats if targeted (adjustments apply while
      // guarding too)
      auto otherSkill = eState->GetActivatedAbility();
      auto otherDef = otherSkill ? otherSkill->GetSkillData() : nullptr;
      if (otherDef && otherDef->GetBasic()->GetActionType() ==
                          objects::MiSkillBasicData::ActionType_t::GUARD) {
        contextSkill = otherDef;
      }
    }

    bool useSkillContext = contextSkill != nullptr;
    if (useSkillContext) {
      // Filter invalid skills out
      if (contextSkill->GetCommon()->GetCategory()->GetMainCategory() !=
          SKILL_CATEGORY_ACTIVE) {
        // Active only
        contextSkill = nullptr;
        useSkillContext = false;
      } else if (contextSkill->GetCommon()->CorrectTblCount() == 0) {
        // No adjustments, nothing to do
        contextSkill = nullptr;
        useSkillContext = false;
      } else if (contextSkill->GetDamage()->GetBattleDamage()->GetFormula() ==
                 objects::MiBattleDamageData::Formula_t::DMG_NORMAL_SIMPLE) {
        // Used post recalc only
        useSkillContext = false;
      }
    }

    bool modified = contextSkill != nullptr;
    for (auto pair : pendingSkillTokusei) {
      auto tokusei = definitionManager->GetTokuseiData(pair.first);
      if (tokusei) {
        auto sourceConditions = tokusei->GetSkillConditions();
        auto targetConditions = tokusei->GetSkillTargetConditions();
        if ((sourceConditions.size() > 0 && isTarget) ||
            (targetConditions.size() > 0 && !isTarget)) {
          stillPendingSkillTokusei[tokusei->GetID()] = pair.second;
          continue;
        }

        auto& conditions = isTarget ? targetConditions : sourceConditions;
        int8_t eval = EvaluateTokuseiSkillConditions(eState, conditions, pSkill,
                                                     otherState);
        if (eval == 1) {
          effectiveTokusei[tokusei->GetID()] = pair.second;
          modified = true;

          for (auto aspect : tokusei->GetAspects()) {
            aspects.insert((int8_t)aspect->GetType());
          }
        } else if (eval == -1) {
          stillPendingSkillTokusei[tokusei->GetID()] = pair.second;
        }
      }
    }

    if (modified) {
      // If the tokusei set was modified, calculate skill specific stats
      calcState = std::make_shared<objects::CalculatedEntityState>();
      calcState->SetExistingTokuseiAspects(aspects);
      calcState->SetEffectiveTokusei(effectiveTokusei);
      calcState->SetPendingSkillTokusei(stillPendingSkillTokusei);

      eState->RecalculateStats(definitionManager, calcState,
                               useSkillContext ? contextSkill : nullptr);

      if (contextSkill &&
          contextSkill->GetDamage()->GetBattleDamage()->GetFormula() ==
              objects::MiBattleDamageData::Formula_t::DMG_NORMAL_SIMPLE) {
        // Stats on skill override entity stats
        for (auto ct : contextSkill->GetCommon()->GetCorrectTbl()) {
          calcState->SetCorrectTbl(ct->GetType(), ct->GetValue());
        }
      }
    }

    if (isTarget) {
      skill.TargetCalcStates[eState->GetEntityID()] = calcState;
    } else if (otherState) {
      skill.SourceCalcStates[eState->GetEntityID()][otherState->GetEntityID()] =
          calcState;
    }
  }

  return calcState;
}

int8_t SkillManager::EvaluateTokuseiSkillConditions(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::list<std::shared_ptr<objects::TokuseiSkillCondition>>&
        conditions,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill,
    const std::shared_ptr<ActiveEntityState>& otherState) {
  // Just like non-skill conditions, compare singular (and) and option group
  // (or) conditions and only return 0 if the entire clause evaluates to
  // true. If at any point an invalid target condition is encountered, the
  // conditions cannot be evaluated until this changes.
  std::unordered_map<uint8_t, bool> optionGroups;
  for (auto condition : conditions) {
    // If the option group has already had a condition pass, skip it
    uint8_t optionGroupID = condition->GetOptionGroupID();
    if (optionGroupID != 0) {
      if (optionGroups.find(optionGroupID) == optionGroups.end()) {
        optionGroups[optionGroupID] = false;
      } else if (optionGroups[optionGroupID]) {
        continue;
      }
    }

    int8_t eval =
        EvaluateTokuseiSkillCondition(eState, condition, pSkill, otherState);
    if (eval == -1) {
      // If a single condition requires re-evaluation, stop here
      return -1;
    }

    if (optionGroupID != 0) {
      optionGroups[optionGroupID] |= (eval == 1);
    } else if (eval == 0) {
      // Standalone did not pass
      return 0;
    }
  }

  for (auto pair : optionGroups) {
    if (!pair.second) {
      // Option group did not pass
      return 0;
    }
  }

  return 1;
}

int8_t SkillManager::EvaluateTokuseiSkillCondition(
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<objects::TokuseiSkillCondition>& condition,
    const std::shared_ptr<ProcessingSkill>& pSkill,
    const std::shared_ptr<ActiveEntityState>& otherState) {
  ProcessingSkill& skill = *pSkill.get();

  // TokuseiSkillCondition comparators can only be equals or not equal
  bool negate = condition->GetComparator() ==
                objects::TokuseiCondition::Comparator_t::NOT_EQUAL;

  switch (condition->GetSkillConditionType()) {
    case TokuseiSkillConditionType::ANY_SKILL:
      // Used to bind conditions to skill processing time only
      return 1;
    case TokuseiSkillConditionType::EXPLICIT_SKILL:
      // Current skill is the specified skill
      return (skill.SkillID == (uint32_t)condition->GetValue()) == !negate ? 1
                                                                           : 0;
    case TokuseiSkillConditionType::ACTION_TYPE:
      // Current skill is the specified action type
      return ((int32_t)skill.Definition->GetBasic()->GetActionType() ==
              condition->GetValue()) == !negate
                 ? 1
                 : 0;
    case TokuseiSkillConditionType::AFFINITY:
      // Current skill is the specified affinity type
      return ((int32_t)skill.BaseAffinity == condition->GetValue() ||
              (int32_t)skill.EffectiveAffinity == condition->GetValue()) ==
                     !negate
                 ? 1
                 : 0;
    case TokuseiSkillConditionType::SKILL_CLASS:
      // Current skill is magic, physical or misc
      switch (skill.EffectiveDependencyType) {
        case SkillDependencyType_t::SPELL:
        case SkillDependencyType_t::SPELL_CLSR:
        case SkillDependencyType_t::SPELL_CLSR_LNGR:
        case SkillDependencyType_t::SPELL_LNGR:
        case SkillDependencyType_t::SUPPORT:
          // Magic
          return (1 == condition->GetValue()) == !negate ? 1 : 0;
        case SkillDependencyType_t::CLSR:
        case SkillDependencyType_t::CLSR_LNGR_SPELL:
        case SkillDependencyType_t::CLSR_SPELL:
        case SkillDependencyType_t::LNGR:
        case SkillDependencyType_t::LNGR_CLSR_SPELL:
        case SkillDependencyType_t::LNGR_SPELL:
          // Physical
          return (2 == condition->GetValue()) == !negate ? 1 : 0;
        case SkillDependencyType_t::NONE:
        default:
          // Misc
          return (3 == condition->GetValue()) == !negate ? 1 : 0;
      }
    case TokuseiSkillConditionType::SKILL_EXPERTISE:
      // Current skill is the specified expertise type
      return ((int32_t)skill.ExpertiseType == condition->GetValue()) == !negate
                 ? 1
                 : 0;
    case TokuseiSkillConditionType::ENEMY_DIGITALIZED:
      // Enemy is digitalized (must be a player entity)
      if (!otherState) {
        // Target required
        return -1;
      } else {
        auto state =
            ClientState::GetEntityClientState(otherState->GetEntityID(), false);
        auto cState = state ? state->GetCharacterState() : nullptr;
        return (cState && cState->GetDigitalizeState()) == !negate ? 1 : 0;
      }
      break;
    case TokuseiSkillConditionType::ENEMY_EQUIPPED:
      // Enemy has the specified item equipped (must be a character)
      if (!otherState) {
        // Target required
        return -1;
      } else {
        auto cState = std::dynamic_pointer_cast<CharacterState>(otherState);

        bool equipped = false;
        if (cState) {
          for (auto equip : cState->GetEntity()->GetEquippedItems()) {
            if (!equip.IsNull() &&
                equip->GetType() == (uint32_t)condition->GetValue()) {
              equipped = true;
              break;
            }
          }
        }

        return equipped == !negate ? 1 : 0;
      }
      break;
    case TokuseiSkillConditionType::ENEMY_FACTION:
      // Enemy is in a different faction (0) or the same faction (1)
      if (!otherState) {
        // Target required
        return -1;
      } else {
        return eState->SameFaction(otherState) == !negate ? 1 : 0;
      }
      break;
    case TokuseiSkillConditionType::ENEMY_GENDER:
      // Enemy's gender matches the specified type (can be any target type)
      if (!otherState) {
        // Target required
        return -1;
      } else {
        int32_t gender = (int32_t)objects::MiNPCBasicData::Gender_t::NONE;

        auto demonData = otherState->GetDevilData();
        if (demonData) {
          gender =
              demonData ? (int32_t)demonData->GetBasic()->GetGender() : gender;
        } else if (otherState->GetEntityType() == EntityType_t::CHARACTER) {
          auto character = std::dynamic_pointer_cast<CharacterState>(otherState)
                               ->GetEntity();
          gender = character ? (int32_t)character->GetGender() : gender;
        }

        return (gender == condition->GetValue()) == !negate ? 1 : 0;
      }
    case TokuseiSkillConditionType::ENEMY_LNC:
      // Enemy's LNC matches the specified type (can be any target type)
      if (!otherState) {
        // Target required
        return -1;
      } else {
        return otherState->IsLNCType((uint8_t)condition->GetValue(), false) ==
                       !negate
                   ? 1
                   : 0;
      }
    case TokuseiSkillConditionType::ENEMY_STATUS_ACTIVE:
      // Enemy's has status of the specified type
      if (!otherState) {
        // Target required
        return -1;
      } else {
        return otherState->StatusEffectActive(
                   (uint32_t)condition->GetValue()) == !negate
                   ? 1
                   : 0;
      }
    case TokuseiSkillConditionType::ENEMY_TOKUSEI:
      // Enemy has a tokusei matching the specified type (tokusei cannot be
      // skill granted like the one being checked)
      if (!otherState) {
        // Target required
        return -1;
      } else {
        return otherState->GetCalculatedState()->EffectiveTokuseiKeyExists(
                   condition->GetValue()) == !negate
                   ? 1
                   : 0;
      }
    default:
      break;
  }

  // The remaining conditions depend on the other entity being a demon
  if (!otherState) {
    // Target required
    return -1;
  }

  auto demonData = otherState ? otherState->GetDevilData() : nullptr;
  if (!demonData) {
    // Target is not a demon
    return 0;
  }

  switch (condition->GetSkillConditionType()) {
    case TokuseiSkillConditionType::DEMON_TYPE:
      // Demon is the specified type
      return ((int32_t)demonData->GetBasic()->GetID() ==
              condition->GetValue()) == !negate
                 ? 1
                 : 0;
    case TokuseiSkillConditionType::DEMON_FAMILY:
      // Demon is the specified family
      return ((int32_t)demonData->GetCategory()->GetFamily() ==
              condition->GetValue()) == !negate
                 ? 1
                 : 0;
    case TokuseiSkillConditionType::DEMON_RACE:
      // Demon is the specified race
      return ((int32_t)demonData->GetCategory()->GetRace() ==
              condition->GetValue()) == !negate
                 ? 1
                 : 0;
    case TokuseiSkillConditionType::DEMON_TITLE:
      // Demon has the specified title
      return ((int32_t)demonData->GetBasic()->GetTitle() ==
              condition->GetValue()) == !negate
                 ? 1
                 : 0;
    case TokuseiSkillConditionType::DEMON_PARTNER_MATCH:
      // Demon is the same family, race or type as the entity's partner demon
      {
        std::shared_ptr<objects::MiDevilData> partnerData;
        auto state =
            ClientState::GetEntityClientState(eState->GetEntityID(), false);
        if (state && state->GetCharacterState() == eState &&
            state->GetDemonState()->Ready()) {
          partnerData = state->GetDemonState()->GetDevilData();
        }

        if (!partnerData) {
          // Unlike the target not existing, the partner not existing is
          // not a condition to evaluate later
          return negate ? 1 : 0;
        }

        switch (condition->GetValue()) {
          case 0:
            // Same family
            return (partnerData->GetCategory()->GetFamily() ==
                    demonData->GetCategory()->GetFamily()) == !negate
                       ? 1
                       : 0;
          case 1:
            // Same race
            return (partnerData->GetCategory()->GetRace() ==
                    demonData->GetCategory()->GetRace()) == !negate
                       ? 1
                       : 0;
          case 2:
            // Same type
            return (partnerData->GetBasic()->GetID() ==
                    demonData->GetBasic()->GetID()) == !negate
                       ? 1
                       : 0;
          default:
            return 0;
        }
      }
      break;
    default:
      break;
  }

  return 0;
}

uint16_t SkillManager::CalculateOffenseValue(
    const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<ActiveEntityState>& target,
    const std::shared_ptr<ProcessingSkill>& pSkill) {
  ProcessingSkill& skill = *pSkill.get();
  auto& offenseValues = skill.OffenseValues[source->GetEntityID()];
  if (offenseValues.find(target->GetEntityID()) != offenseValues.end()) {
    return offenseValues[target->GetEntityID()];
  }

  auto calcState = GetCalculatedState(source, pSkill, false, target);

  int16_t clsr = calcState->GetCorrectTbl((size_t)CorrectTbl::CLSR);
  int16_t lngr = calcState->GetCorrectTbl((size_t)CorrectTbl::LNGR);
  int16_t spell = calcState->GetCorrectTbl((size_t)CorrectTbl::SPELL);
  int16_t support = calcState->GetCorrectTbl((size_t)CorrectTbl::SUPPORT);

  uint16_t off = 0;
  switch (skill.EffectiveDependencyType) {
    case SkillDependencyType_t::CLSR:
      off = (uint16_t)clsr;
      break;
    case SkillDependencyType_t::LNGR:
      off = (uint16_t)lngr;
      break;
    case SkillDependencyType_t::SPELL:
      off = (uint16_t)spell;
      break;
    case SkillDependencyType_t::SUPPORT:
      off = (uint16_t)support;
      break;
    case SkillDependencyType_t::LNGR_SPELL:
      off = (uint16_t)(lngr + spell / 2);
      break;
    case SkillDependencyType_t::SPELL_CLSR:
      off = (uint16_t)(spell + clsr / 2);
      break;
    case SkillDependencyType_t::SPELL_LNGR:
      off = (uint16_t)(spell + lngr / 2);
      break;
    case SkillDependencyType_t::CLSR_LNGR_SPELL:
      off = (uint16_t)(clsr + lngr + spell);
      break;
    case SkillDependencyType_t::LNGR_CLSR_SPELL:
      off = (uint16_t)(lngr + clsr + spell);
      break;
    case SkillDependencyType_t::SPELL_CLSR_LNGR:
      off = (uint16_t)(spell + clsr + lngr);
      break;
    case SkillDependencyType_t::CLSR_SPELL:
      off = (uint16_t)(clsr + spell / 2);
      break;
    case SkillDependencyType_t::NONE:
    default:
      LogSkillManagerError([skill]() {
        return libcomp::String(
                   "Invalid dependency type for damage calculation "
                   "encountered: %1\n")
            .Arg((uint8_t)skill.EffectiveDependencyType);
      });

      return false;
  }

  if (skill.ExecutionContext->CounteredSkill) {
    // If countering, modify the offensive value with the offense value
    // of the original skill used, min for invalid dependency type
    uint16_t counterOff = 0;
    if (skill.ExecutionContext->CounteredSkill->EffectiveDependencyType ==
        SkillDependencyType_t::NONE) {
      counterOff = 1;
    } else {
      counterOff = CalculateOffenseValue(
          target, source, skill.ExecutionContext->CounteredSkill);
    }

    off = (uint16_t)(off + (counterOff * 2));
  }

  offenseValues[target->GetEntityID()] = off;

  return off;
}

bool SkillManager::ApplyPrimaryCounter(
    const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<ProcessingSkill>& pSkill, bool guard) {
  auto tActivated = pSkill->PrimaryTarget->GetActivatedAbility();
  if (!tActivated || !pSkill->PrimaryTarget) {
    return false;
  }

  if (pSkill->ExecutionContext->CounteredSkill) {
    // Cannot double counter
    return false;
  }

  SkillTargetResult target;
  target.PrimaryTarget = true;
  target.EntityState = pSkill->PrimaryTarget;
  target.CalcState =
      GetCalculatedState(pSkill->PrimaryTarget, pSkill, true, source);

  auto tSkillData = tActivated->GetSkillData();
  switch (tSkillData->GetBasic()->GetActionType()) {
    case objects::MiSkillBasicData::ActionType_t::COUNTER:
      if (HandleCounter(source, target, pSkill)) {
        pSkill->Targets.push_back(target);
        return true;
      }
      break;
    case objects::MiSkillBasicData::ActionType_t::DODGE:
      if (HandleDodge(source, target, pSkill)) {
        pSkill->Targets.push_back(target);
        return true;
      }
      break;
    case objects::MiSkillBasicData::ActionType_t::GUARD:
      if (guard && HandleGuard(source, target, pSkill)) {
        pSkill->Targets.push_back(target);
        return true;
      }
      break;
    default:
      // Cancellations occur based on knockback or damage later
      break;
  }

  return false;
}

void SkillManager::ApplySecondaryCounter(
    const std::shared_ptr<ActiveEntityState>& source, SkillTargetResult& target,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  if (!target.HitNull && !target.HitAbsorb && !target.HitReflect &&
      pSkill->Definition->GetBasic()->GetCombatSkill()) {
    // Check if the target dodges or guards the skill. Counters
    // only apply for the primary target and are handled earlier.
    auto tActivated = target.EntityState->GetActivatedAbility();
    if (tActivated && target.EntityState != pSkill->EffectiveSource) {
      auto tSkillData = tActivated->GetSkillData();
      switch (tSkillData->GetBasic()->GetActionType()) {
        case objects::MiSkillBasicData::ActionType_t::GUARD:
          HandleGuard(source, target, pSkill);
          break;
        case objects::MiSkillBasicData::ActionType_t::DODGE:
          HandleDodge(source, target, pSkill);
          break;
        default:
          // Cancellations occur based on knockback or damage
          // later
          break;
      }
    }
  }
}

bool SkillManager::HandleGuard(const std::shared_ptr<ActiveEntityState>& source,
                               SkillTargetResult& target,
                               const std::shared_ptr<ProcessingSkill>& pSkill) {
  auto tActivated = target.EntityState->GetActivatedAbility();
  if (!tActivated) {
    return false;
  }

  bool guardValid = false;
  uint8_t cancelType = 1;
  int8_t activationID = tActivated->GetActivationID();
  if (pSkill->Definition->GetBasic()->GetDefensible()) {
    switch (pSkill->Definition->GetBasic()->GetActionType()) {
      case objects::MiSkillBasicData::ActionType_t::ATTACK:
      case objects::MiSkillBasicData::ActionType_t::SPIN:
      case objects::MiSkillBasicData::ActionType_t::TAUNT:
        guardValid = true;
        break;
      case objects::MiSkillBasicData::ActionType_t::RUSH:
      case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
        cancelType = 3;  // Display guard break animation
        break;
      case objects::MiSkillBasicData::ActionType_t::TALK:
        // Nothing happens, skill stays active
        return true;
      default:
        break;
    }
  }

  bool quake = false;
  if (!guardValid && pSkill->FunctionID == SVR_CONST.SKILL_DIASPORA_QUAKE) {
    guardValid = true;
    quake = true;
  }

  if (guardValid &&
      tActivated->GetChargedTime() <= pSkill->Activated->GetHitTime()) {
    auto tSkillData = tActivated->GetSkillData();

    target.Flags1 |= FLAG1_GUARDED;
    target.GuardModifier =
        tSkillData->GetDamage()->GetBattleDamage()->GetModifier1();

    // Fast track execute now but fizzle if not the primary target,
    // defense bonus still applies
    auto guardCtx = std::make_shared<SkillExecutionContext>();
    guardCtx->CounteredSkill = pSkill;
    guardCtx->FastTrack = true;
    guardCtx->Fizzle = quake || target.EntityState != pSkill->PrimaryTarget;

    if (ExecuteSkill(target.EntityState, activationID, source->GetEntityID(),
                     guardCtx)) {
      LogSkillManagerDebug([target, pSkill]() {
        return libcomp::String("%1 guards against skill %2[%3].\n")
            .Arg(target.EntityState->GetEntityLabel())
            .Arg(pSkill->SkillID)
            .Arg(pSkill->Activated->GetActivationID());
      });

      if (quake) {
        // The Diaspora Quake skill is fully cancelled when guarding
        target.HitNull = 2;
        target.HitAvoided = true;
      }

      pSkill->ExecutionContext->SubContexts.push_back(guardCtx);
      return true;
    }
  }

  CancelSkill(target.EntityState, tActivated->GetActivationID(), cancelType);
  return false;
}

bool SkillManager::HandleCounter(
    const std::shared_ptr<ActiveEntityState>& source, SkillTargetResult& target,
    const std::shared_ptr<ProcessingSkill>& pSkill) {
  auto tActivated = target.EntityState->GetActivatedAbility();
  if (!tActivated) {
    return false;
  }

  uint8_t cancelType = 1;
  int8_t activationID = tActivated->GetActivationID();
  if (pSkill->Definition->GetBasic()->GetDefensible()) {
    auto tSkillData = tActivated->GetSkillData();
    switch (pSkill->Definition->GetBasic()->GetActionType()) {
      case objects::MiSkillBasicData::ActionType_t::ATTACK:
      case objects::MiSkillBasicData::ActionType_t::RUSH:
      case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
        if (tActivated->GetChargedTime() <= pSkill->Activated->GetHitTime()) {
          target.Flags1 |= FLAG1_GUARDED;
          target.HitAvoided = true;

          auto counterCtx = std::make_shared<SkillExecutionContext>();
          counterCtx->CounteredSkill = pSkill;
          counterCtx->FastTrack = true;

          if (ExecuteSkill(target.EntityState, activationID,
                           source->GetEntityID(), counterCtx)) {
            LogSkillManagerDebug([target, pSkill]() {
              return libcomp::String("%1 counters skill %2[%3].\n")
                  .Arg(target.EntityState->GetEntityLabel())
                  .Arg(pSkill->SkillID)
                  .Arg(pSkill->Activated->GetActivationID());
            });

            pSkill->ExecutionContext->SubContexts.push_back(counterCtx);
            return true;
          }
        }
        break;
      case objects::MiSkillBasicData::ActionType_t::SPIN:
      case objects::MiSkillBasicData::ActionType_t::TAUNT:
        cancelType = 3;  // Display counter break animation
        break;
      case objects::MiSkillBasicData::ActionType_t::TALK:
        // Nothing happens, skill stays active
        return true;
      default:
        break;
    }
  }

  CancelSkill(target.EntityState, activationID, cancelType);
  return false;
}

bool SkillManager::HandleDodge(const std::shared_ptr<ActiveEntityState>& source,
                               SkillTargetResult& target,
                               const std::shared_ptr<ProcessingSkill>& pSkill) {
  auto tActivated = target.EntityState->GetActivatedAbility();
  if (!tActivated) {
    return false;
  }

  int8_t activationID = tActivated->GetActivationID();
  if (pSkill->Definition->GetBasic()->GetDefensible()) {
    auto tSkillData = tActivated->GetSkillData();
    switch (pSkill->Definition->GetBasic()->GetActionType()) {
      case objects::MiSkillBasicData::ActionType_t::SHOT:
      case objects::MiSkillBasicData::ActionType_t::RAPID:
      case objects::MiSkillBasicData::ActionType_t::TALK:
        if (tActivated->GetChargedTime() <= pSkill->Activated->GetHitTime()) {
          LogSkillManagerDebug([target, pSkill]() {
            return libcomp::String("%1 dodges skill %2[%3].\n")
                .Arg(target.EntityState->GetEntityLabel())
                .Arg(pSkill->SkillID)
                .Arg(pSkill->Activated->GetActivationID());
          });

          target.Flags1 |= FLAG1_DODGED;
          target.Damage1Type = target.Damage2Type = DAMAGE_TYPE_MISS;
          target.HitAvoided = true;

          // Fast track execute now but fizzle if not the primary target
          auto dodgeCtx = std::make_shared<SkillExecutionContext>();
          dodgeCtx->CounteredSkill = pSkill;
          dodgeCtx->FastTrack = true;
          dodgeCtx->Fizzle = target.EntityState != pSkill->PrimaryTarget;

          if (ExecuteSkill(target.EntityState, activationID,
                           source->GetEntityID(), dodgeCtx)) {
            pSkill->ExecutionContext->SubContexts.push_back(dodgeCtx);
            return true;
          }
        }
        break;
      case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
      case objects::MiSkillBasicData::ActionType_t::TAUNT:
        // Nothing happens, skill stays active
        return true;
      default:
        break;
    }
  }

  CancelSkill(target.EntityState, activationID);
  return false;
}

bool SkillManager::HandleSkillInterrupt(
    const std::shared_ptr<ActiveEntityState>& source, SkillTargetResult& target,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  auto eState = target.EntityState;
  uint8_t cancelFlags = target.EffectCancellations;

  // Check for skills that need to be cancelled
  if (cancelFlags & (EFFECT_CANCEL_DAMAGE | EFFECT_CANCEL_KNOCKBACK)) {
    auto tActivated = eState->GetActivatedAbility();
    auto tSkillData = tActivated ? tActivated->GetSkillData() : nullptr;
    bool applyInterrupt = false;
    if (tSkillData) {
      auto tDischarge = tSkillData->GetDischarge();
      if (!tActivated->GetExecutionRequestTime()) {
        // Not executed yet, apply charge cancellations
        auto tCancel = tSkillData->GetCast()->GetCancel();

        if ((cancelFlags & EFFECT_CANCEL_DAMAGE) != 0 &&
            tCancel->GetDamageCancel()) {
          // Interrupted from damage
          applyInterrupt = true;
        } else if ((cancelFlags & EFFECT_CANCEL_KNOCKBACK) != 0 &&
                   tCancel->GetKnockbackCancel()) {
          // Interrupted from knockback
          applyInterrupt = true;
        }

        if (applyInterrupt) {
          // Cast interrupt must exist regardless of hitstun null
          auto tokuseiManager = mServer.lock()->GetTokuseiManager();

          int32_t interruptNull =
              (int32_t)tokuseiManager->GetAspectSum(
                  source, TokuseiAspectType::CAST_INTERRUPT_NULL,
                  GetCalculatedState(eState, pSkill, true, source)) *
              100;

          applyInterrupt =
              interruptNull < 10000 &&
              (interruptNull < 0 || RNG(int32_t, 1, 10000) > interruptNull);
        }
      } else if (target.CanHitstun && tDischarge->GetShotInterruptible() &&
                 !pSkill->IsProjectile) {
        // Determine which part of the skill can be interrupted (not
        // possible for projectile sources past charge)
        uint64_t hit = pSkill->Activated->GetHitTime();
        if (tActivated->GetHitTime() == 0) {
          // Interrupted before shot
          applyInterrupt = true;
        } else if (hit < tActivated->GetHitTime()) {
          uint64_t hitWindowAdjust = (uint64_t)(
              500000.0 * (double)tDischarge->GetCompleteDelay() * 0.01);
          uint64_t hitTime = 0;
          if (tSkillData->GetBasic()->GetActionType() ==
              objects::MiSkillBasicData::ActionType_t::RUSH) {
            // The last X% of rush skills is not interruptible
            hitTime = (uint64_t)(tActivated->GetHitTime() - hitWindowAdjust);
          } else {
            // The first X% of non-rush skills is interruptible
            hitTime = (uint64_t)((tActivated->GetHitTime() - 500000ULL) +
                                 hitWindowAdjust);
          }

          if (hit < hitTime) {
            // Interrupted during shot
            applyInterrupt = true;
          }
        }
      }
    }

    // If an interrupt would happen but the skill is a countering
    // skill, do not cancel
    if (applyInterrupt) {
      auto ctx = pSkill->ExecutionContext;
      for (auto counteringSkill : ctx->CounteringSkills) {
        if (counteringSkill->Activated == tActivated) {
          applyInterrupt = false;
          break;
        }
      }
    }

    if (applyInterrupt) {
      CancelSkill(eState, tActivated->GetActivationID());
      return true;
    }
  }

  return false;
}

std::set<uint32_t> SkillManager::HandleStatusEffects(
    const std::shared_ptr<ActiveEntityState>& source, SkillTargetResult& target,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  std::set<uint32_t> cancelOnKill;
  if ((target.Flags2 & FLAG2_IMPOSSIBLE) != 0) {
    // The target cannot be affected by the skill in any way,
    // return an empty set
    return cancelOnKill;
  }

  // Gather status effects from the skill
  auto directStatuses = pSkill->Definition->GetDamage()->GetAddStatuses();

  int16_t stackScale = 1;
  if (pSkill->FunctionID) {
    // Apply FID transformations
    if (pSkill->FunctionID == SVR_CONST.SKILL_STATUS_RANDOM ||
        pSkill->FunctionID == SVR_CONST.SKILL_STATUS_RANDOM2) {
      // Randomly pick one
      auto entry = libcomp::Randomizer::GetEntry(directStatuses);
      directStatuses.clear();
      directStatuses.push_back(entry);
    } else if (pSkill->FunctionID == SVR_CONST.SKILL_STATUS_SCALE) {
      // Multiply stacks from stat
      auto params = pSkill->Definition->GetSpecial()->GetSpecialParams();
      int16_t stat = source->GetCorrectValue((CorrectTbl)params[0]);

      stackScale =
          (int16_t)floor((float)stat * ((float)(100 - params[1]) / 100.f));

      if (stackScale < 1) {
        stackScale = 1;
      }
    }
  }

  std::unordered_map<uint32_t, double> addStatusMap;
  std::unordered_map<uint32_t, std::shared_ptr<objects::MiAddStatusTbl>>
      addStatusDefs;
  std::set<uint32_t> maxRates;
  for (auto addStatus : directStatuses) {
    uint32_t effectID = addStatus->GetStatusID();
    if (!addStatus->GetOnKnockback() || target.ApplyAddedKnockbackEffects) {
      addStatusMap[effectID] = (double)addStatus->GetSuccessRate();
      addStatusDefs[effectID] = addStatus;
      if (addStatus->GetSuccessRate() >= 100) {
        maxRates.insert(effectID);
      }
    }
  }

  auto eState = target.EntityState;
  auto sourceCalc = GetCalculatedState(source, pSkill, false, eState);

  // Add tokusei status effects
  for (auto addStatus : mServer.lock()->GetTokuseiManager()->GetAspectMap(
           source, TokuseiAspectType::STATUS_ADD, sourceCalc)) {
    uint32_t effectID = (uint32_t)addStatus.first;
    if (addStatus.second >= 100) {
      maxRates.insert(effectID);
    }

    if (addStatusMap.find(effectID) != addStatusMap.end()) {
      addStatusMap[effectID] += addStatus.second;
    } else {
      addStatusMap[effectID] = addStatus.second;
    }
  }

  // If a knockback occurred, add bonus knockback status effects from tokusei
  if (target.ApplyAddedKnockbackEffects) {
    for (auto addStatus : mServer.lock()->GetTokuseiManager()->GetAspectMap(
             source, TokuseiAspectType::KNOCKBACK_STATUS_ADD, sourceCalc)) {
      uint32_t effectID = (uint32_t)addStatus.first;
      if (addStatus.second >= 100) {
        maxRates.insert(effectID);
      }

      if (addStatusMap.find(effectID) != addStatusMap.end()) {
        addStatusMap[effectID] += addStatus.second;
      } else {
        addStatusMap[effectID] = addStatus.second;
      }
    }
  }

  if (addStatusMap.size() == 0) {
    return cancelOnKill;
  }

  auto targetCalc = GetCalculatedState(eState, pSkill, true, source);

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto tokuseiManager = server->GetTokuseiManager();

  const static bool nraStatusNull =
      server->GetWorldSharedConfig()->GetNRAStatusNull();

  auto statusAdjusts = tokuseiManager->GetAspectMap(
      source, TokuseiAspectType::STATUS_INFLICT_ADJUST, sourceCalc);
  auto statusNulls = tokuseiManager->GetAspectMap(
      eState, TokuseiAspectType::STATUS_NULL, targetCalc);

  for (auto statusPair : addStatusMap) {
    uint32_t effectID = statusPair.first;

    auto addStatus = addStatusDefs[effectID];

    bool isRemove = addStatus && addStatus->GetMinStack() == 0 &&
                    addStatus->GetMaxStack() == 0;
    bool isReplace = addStatus && addStatus->GetIsReplace();

    auto statusDef = definitionManager->GetStatusData(effectID);
    if (!statusDef) continue;

    uint8_t affinity = statusDef->GetCommon()->GetAffinity();
    uint8_t statusCategory =
        statusDef->GetCommon()->GetCategory()->GetMainCategory();

    // Adjusted category (-category - 1) can be used to null or alter rate
    int32_t adjustCategory = (int32_t)(statusCategory * -1) - 1;

    // Determine if the effect can be added
    if (!isRemove) {
      // If its application logic type 1, it cannot be applied if
      // it is already active unless we're replacing (ex: sleep)
      if (statusDef->GetBasic()->GetApplicationLogic() == 1 && !isReplace &&
          eState->StatusEffectActive(effectID)) {
        continue;
      }

      // Determine if the effect should be nullified by direct ID,
      // then adjusted category
      if (statusNulls.find((int32_t)effectID) != statusNulls.end()) {
        continue;
      } else if (statusNulls.find(adjustCategory) != statusNulls.end()) {
        continue;
      }

      if (affinity) {
        // Roll NRA chances; if they succeed don't even bother doing the
        // status infliction math to see if it'd hit. If the server
        // setting NRAStatusNull is true, rolls to nullify status effects
        // with an affinity type that the target could potentially NRA
        // automatically succeed (this does not take NRA shields into account
        // since nothing is "consumed" by this)
        bool nraSuccess = false;
        CorrectTbl nraType = (CorrectTbl)(affinity + NRA_OFFSET);
        for (auto nraIdx : {NRA_ABSORB, NRA_REFLECT, NRA_NULL}) {
          int16_t chance =
              eState->GetNRAChance((uint8_t)nraIdx, nraType, targetCalc);
          if (chance >= 100 ||
              (chance > 0 &&
               (nraStatusNull || RNG(int16_t, 1, 100) <= chance))) {
            nraSuccess = true;
            break;
          }
        }

        if (nraSuccess) {
          continue;
        }
      }
    }

    uint8_t statusSubCategory =
        statusDef->GetCommon()->GetCategory()->GetSubCategory();

    // Only certain types of status effects can be resisted
    bool canResist = !isRemove && statusCategory != STATUS_CATEGORY_GOOD &&
                     statusSubCategory != STATUS_SUBCATEGORY_BUFF &&
                     statusSubCategory != STATUS_SUBCATEGORY_DEBUFF;

    // Effect can be added (or removed), determine success rate
    double successRate = statusPair.second;

    // Hard 100% success rates cannot be adjusted, only avoided entirely
    if (maxRates.find(effectID) == maxRates.end()) {
      // Boost for certain epertise
      if ((pSkill->ExpertiseType == EXPERTISE_CHAIN_COTW ||
           pSkill->ExpertiseType == EXPERTISE_CHAIN_M_BULLET) &&
          pSkill->ExpertiseRankBoost) {
        // Raise by 1% per rank
        successRate += (double)(pSkill->ExpertiseRankBoost / 2);
      }

      // Apply affinity based adjustments
      if (affinity) {
        // Add affinity boost/2
        successRate +=
            (double)GetAffinityBoost(source, sourceCalc,
                                     (CorrectTbl)(affinity + BOOST_OFFSET)) /
            2.0;

        if (successRate > 0.f && canResist) {
          // Multiply by 100% + -resistance
          CorrectTbl resistCorrectType = (CorrectTbl)(affinity + RES_OFFSET);

          double resist =
              (double)(eState->GetCorrectValue(resistCorrectType, targetCalc) *
                       0.01);
          successRate *= (1.0 + resist * -1.0);
        }
      }

      if (successRate < 0.f) {
        // Minimum value for this point is 0%
        successRate = 0.f;
      }

      if (statusAdjusts.size() > 0) {
        double rateBoost = 0.0;
        // A STATUS_INFLICT_ADJUST value of 0 boosts all inflicts
        auto it = statusAdjusts.find(0);
        if (it != statusAdjusts.end()) {
          rateBoost += it->second;
        }

        // Boost success by direct inflict adjust
        it = statusAdjusts.find((int32_t)effectID);
        if (it != statusAdjusts.end()) {
          rateBoost += it->second;
        }

        // Boost success by adjusted category inflict adjust
        it = statusAdjusts.find(adjustCategory);
        if (it != statusAdjusts.end()) {
          rateBoost += it->second;
        }

        // Add rate boost directly
        if (rateBoost > 0.0) {
          successRate += rateBoost;
        }
      }

      // Add bad status resistance from target
      if (successRate > 0.f && canResist) {
        double resist = ((double)eState->GetCorrectValue(CorrectTbl::RES_STATUS,
                                                         targetCalc) -
                         100.0) /
                        10.0;
        successRate += resist;
      }
    }

    if (effectID == SVR_CONST.STATUS_DEATH && successRate > 50.0) {
      // Instant death has a hard cap at 50%
      successRate = 50.0;
    }

    // Check if the status effect hits
    if (successRate >= 100.0 ||
        (successRate > 0.0 &&
         RNG(int32_t, 1, 10000) <= (int32_t)(successRate * 100.0))) {
      // If the status was added by the skill itself, use that for
      // application logic, otherwise default to 1 non-replace
      int8_t minStack = addStatus ? addStatus->GetMinStack() : 1;
      int8_t maxStack = addStatus ? addStatus->GetMaxStack() : 1;

      // Scale stacks
      if (stackScale > 1) {
        minStack = (int8_t)(minStack * stackScale);
        maxStack = (int8_t)(maxStack * stackScale);

        // Adjust for overflow
        if (minStack < 0) {
          minStack = 127;
        }

        if (maxStack < 0) {
          maxStack = 127;
        }
      }

      int8_t stack = CalculateStatusEffectStack(minStack, maxStack);
      if (stack == 0 && !isReplace) continue;

      // Check for status damage to apply at the end of the skill
      if (statusCategory == 2) {
        // Apply ailment damage only if HP damage exists and the target
        // does not min normal damage (ignore crit level)
        auto tDamage = statusDef->GetEffect()->GetDamage();
        bool minDamage = targetCalc->ExistingTokuseiAspectsContains(
            (int8_t)TokuseiAspectType::DAMAGE_MIN);
        if (tDamage->GetHPDamage() > 0 && !minDamage && stack > 0) {
          uint8_t ailmentDamageType = (uint8_t)(affinity - AIL_OFFSET);

          // If the ailment damage type is not set yet or the type is
          // lower than the one assigned, set the type
          if (target.AilmentDamage == 0 ||
              ailmentDamageType < target.AilmentDamageType) {
            target.AilmentDamageType = ailmentDamageType;
          }

          target.AilmentDamage += tDamage->GetHPDamage() + stack;

          uint64_t ailmentTime =
              (uint64_t)((uint32_t)stack *
                         statusDef->GetCancel()->GetDuration()) *
              1000;
          if (ailmentTime > target.AilmentDamageTime) {
            target.AilmentDamageTime = ailmentTime;
          }
        }
      } else {
        target.AddedStatuses[effectID] =
            StatusEffectChange(effectID, stack, isReplace);

        auto cancelDef = statusDef->GetCancel();
        if (cancelDef->GetCancelTypes() & EFFECT_CANCEL_DEATH) {
          cancelOnKill.insert(effectID);
        }
      }
    }
  }

  return cancelOnKill;
}

void SkillManager::HandleKills(
    std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<Zone>& zone,
    std::set<std::shared_ptr<ActiveEntityState>> killed) {
  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto managerConnection = server->GetManagerConnection();
  auto zoneManager = server->GetZoneManager();

  auto zConnections = zone->GetConnectionList();

  auto sourceDevilData = source->GetDevilData();
  uint32_t sourceDemonBaseType =
      sourceDevilData ? sourceDevilData->GetUnionData()->GetBaseDemonID() : 0;

  // Familiarity is reduced from death or same demon kills and is dependent
  // upon familiarity type
  auto sourceDemonFType =
      sourceDevilData
          ? server->GetServerDataManager()->GetDemonFamiliarityTypeData(
                sourceDevilData->GetFamiliarity()->GetFamiliarityType())
          : nullptr;

  bool playerSource = source->GetEntityType() == EntityType_t::CHARACTER ||
                      source->GetEntityType() == EntityType_t::PARTNER_DEMON;
  auto instance = zone->GetInstance();

  auto sourceClient =
      playerSource ? managerConnection->GetEntityClient(source->GetEntityID())
                   : nullptr;
  auto sourceState = sourceClient ? sourceClient->GetClientState() : nullptr;

  // Source cooldowns can affect restricted drops as well as invoke points
  std::set<int32_t> sourceCooldowns;
  if (sourceState) {
    // Only pull character level cooldowns
    auto cState = sourceState->GetCharacterState();
    auto character = cState->GetEntity();
    if (character) {
      cState->RefreshActionCooldowns(false);
      for (auto& pair : character->GetActionCooldowns()) {
        sourceCooldowns.insert(pair.first);
      }
    }
  }

  auto deathTriggers =
      zoneManager->GetZoneTriggers(zone, ZoneTrigger_t::ON_DEATH);

  std::unordered_map<int32_t, int32_t> adjustments;
  std::list<std::shared_ptr<ActiveEntityState>> enemiesKilled;
  std::list<std::shared_ptr<ActiveEntityState>> partnerDemonsKilled;
  std::list<std::shared_ptr<ActiveEntityState>> playersKilled;
  libcomp::EnumMap<objects::Spawn::KillValueType_t,
                   std::list<std::shared_ptr<ActiveEntityState>>>
      killValues;
  for (auto entity : killed) {
    // Remove all opponents
    characterManager->AddRemoveOpponent(false, entity, nullptr);

    // Cancel any pending skill
    auto activated = entity->GetActivatedAbility();
    if (activated) {
      CancelSkill(entity, activated->GetActivationID());
    }

    // Determine familiarity adjustments
    bool partnerDeath = false;
    auto demonData = entity->GetDevilData();
    switch (entity->GetEntityType()) {
      case EntityType_t::CHARACTER:
        playersKilled.push_back(entity);
        characterManager->CancelMount(
            ClientState::GetEntityClientState(entity->GetEntityID()));
        break;
      case EntityType_t::PARTNER_DEMON:
        partnerDemonsKilled.push_back(entity);
        partnerDeath = true;
        break;
      case EntityType_t::ENEMY:
      case EntityType_t::ALLY:
        enemiesKilled.push_back(entity);
        break;
      default:
        break;
    }

    int32_t killVal = entity->GetKillValue();
    if (killVal) {
      auto type = objects::Spawn::KillValueType_t::INHERITED;

      auto eBase = entity->GetEnemyBase();
      auto spawn = eBase ? eBase->GetSpawnSource() : nullptr;
      if (spawn) {
        type = spawn->GetKillValueType();
      }

      killValues[type].push_back(entity);
    }

    // Trigger death actions (before zone removal)
    if (deathTriggers.size() > 0) {
      auto client = managerConnection->GetEntityClient(entity->GetEntityID());
      zoneManager->HandleZoneTriggers(zone, deathTriggers, entity, client);
    }

    if (demonData && sourceDemonFType) {
      std::list<std::pair<int32_t, int32_t>> adjusts;
      if (partnerDeath) {
        // Partner demon has died
        int32_t adjust = (int32_t)sourceDemonFType->GetDeath();
        adjusts.push_back(
            std::pair<int32_t, int32_t>(entity->GetEntityID(), adjust));
      }

      if (entity != source &&
          sourceDemonBaseType == demonData->GetUnionData()->GetBaseDemonID()) {
        // Same (base) demon type killed
        int32_t adjust = (int32_t)sourceDemonFType->GetKillTypeMatch();
        adjusts.push_back(
            std::pair<int32_t, int32_t>(source->GetEntityID(), adjust));
      }

      for (auto aPair : adjusts) {
        if (adjustments.find(aPair.first) == adjustments.end()) {
          adjustments[aPair.first] = aPair.second;
        } else {
          adjustments[aPair.first] =
              (int32_t)(adjustments[aPair.first] + aPair.second);
        }
      }
    }

    LogSkillManagerDebug([entity]() {
      return libcomp::String("%1 has been killed.\n")
          .Arg(entity->GetEntityLabel());
    });
  }

  // Apply familiarity adjustments
  for (auto aPair : adjustments) {
    auto demonClient = managerConnection->GetEntityClient(aPair.first);
    if (demonClient) {
      characterManager->UpdateFamiliarity(demonClient, aPair.second, true);
    }
  }

  switch (zone->GetInstanceType()) {
    case InstanceType_t::PVP:
      // Increase by PvP values and set auto-revive time-out
      {
        auto matchManager = server->GetMatchManager();

        if (playerSource) {
          // Reward points to player entities that get a kill
          // (including self)
          for (auto entity :
               killValues[objects::Spawn::KillValueType_t::INHERITED]) {
            matchManager->UpdatePvPPoints(instance->GetID(), source, entity,
                                          entity->GetKillValue());
          }
        }

        for (auto entity : playersKilled) {
          matchManager->PlayerKilled(entity, instance);
        }
      }
      break;
    case InstanceType_t::DEMON_ONLY:
      // Start demon only instance death time-outs
      for (auto dState : partnerDemonsKilled) {
        auto demonCState =
            ClientState::GetEntityClientState(dState->GetEntityID());
        zoneManager->UpdateDeathTimeOut(demonCState, 60);
      }

      // Convert inherited kill values to SP
      for (auto entity :
           killValues[objects::Spawn::KillValueType_t::INHERITED]) {
        killValues[objects::Spawn::KillValueType_t::SOUL_POINTS].push_back(
            entity);
      }
      break;
    case InstanceType_t::PENTALPHA:
      // Convert inherited kill values to bethel
      for (auto entity :
           killValues[objects::Spawn::KillValueType_t::INHERITED]) {
        killValues[objects::Spawn::KillValueType_t::BETHEL].push_back(entity);
      }
      break;
    default:
      break;
  }

  // Inherited kill values must be handled by variant types above
  killValues.erase(objects::Spawn::KillValueType_t::INHERITED);

  auto ubMatch = zone->GetUBMatch();

  if (enemiesKilled.size() > 0) {
    // Gather all enemy entity IDs and levels
    std::list<int32_t> removeIDs;
    std::list<int8_t> levels;
    std::set<int32_t> canRevive;

    auto aiManager = server->GetAIManager();
    for (auto eState : enemiesKilled) {
      aiManager->UpdateAggro(eState, -1);

      zone->RemoveEntity(eState->GetEntityID(), 1);
      levels.push_back(eState->GetLevel());

      if (eState->GetEnemyBase()->GetCanRevive()) {
        canRevive.insert(eState->GetEntityID());
      } else {
        removeIDs.push_back(eState->GetEntityID());
      }
    }

    // Update status effects one last time for each entity so none stick
    // on the bodies
    zoneManager->UpdateStatusEffectStates(zone, (uint32_t)std::time(0),
                                          enemiesKilled);

    zoneManager->RemoveEntitiesFromZone(zone, removeIDs, 4, true);

    // Transform enemies into loot bodies and gather quest kills
    std::unordered_map<std::shared_ptr<LootBoxState>,
                       std::shared_ptr<ActiveEntityState>>
        lStates;
    std::unordered_map<uint32_t, int32_t> questKills;
    std::unordered_map<uint32_t, uint32_t> encounterGroups;
    std::list<std::shared_ptr<ActiveEntityState>> dgEnemies;
    std::list<uint32_t> multiZoneBosses;
    for (auto eState : enemiesKilled) {
      auto eBase = eState->GetEnemyBase();
      auto enemyData = eState->GetDevilData();

      if (enemyData->GetBattleData()->GetDigitalizeXP()) {
        dgEnemies.push_back(eState);
      }

      auto spawn = eBase->GetSpawnSource();
      if (spawn) {
        if (spawn->GetBossGroup()) {
          multiZoneBosses.push_back(eBase->GetType());
        }

        // Add recently killed here as any source counts as a kill
        if (ubMatch && spawn->GetKillValueType() ==
                           objects::Spawn::KillValueType_t::UB_POINTS) {
          ubMatch->AppendRecentlyKilled(spawn);
        }
      }

      if (canRevive.find(eState->GetEntityID()) != canRevive.end()) {
        continue;
      }

      auto lootBody = std::make_shared<objects::LootBox>();
      lootBody->SetType(objects::LootBox::Type_t::BODY);
      lootBody->SetEnemy(eBase);

      auto lState = std::make_shared<LootBoxState>(lootBody);
      lState->SetCurrentX(eState->GetDestinationX());
      lState->SetCurrentY(eState->GetDestinationY());
      lState->SetCurrentRotation(eState->GetDestinationRotation());
      lState->SetEntityID(server->GetNextEntityID());
      lStates[lState] = eState;

      zone->AddLootBox(lState);

      uint32_t dType = enemyData->GetBasic()->GetID();
      if (sourceState && sourceState->QuestTargetEnemiesContains(dType)) {
        if (questKills.find(dType) == questKills.end()) {
          questKills[dType] = 1;
        } else {
          questKills[dType] = (questKills[dType] + 1);
        }
      }

      encounterGroups[eBase->GetEncounterID()] = eBase->GetSpawnGroupID();
    }

    // For each loot body generate and send loot and show the body
    // After this schedule all of the bodies for cleanup after their
    // loot time passes
    uint64_t now = ChannelServer::GetServerTime();
    int16_t luck = 0;
    float maccaRate = 1.f;
    float magRate = 1.f;

    if (sourceState) {
      auto cState = sourceState->GetCharacterState();

      luck = source->GetLUCK();
      maccaRate =
          (float)cState->GetCorrectValue(CorrectTbl::RATE_MACCA) / 100.f;
      magRate = (float)cState->GetCorrectValue(CorrectTbl::RATE_MAG) / 100.f;
    }

    auto firstClient = zConnections.size() > 0 ? zConnections.front() : nullptr;
    auto sourceParty = sourceState ? sourceState->GetParty() : nullptr;

    std::set<int32_t> sourcePartyMembers;
    if (sourceParty) {
      // Filter out party members not in the zone
      for (int32_t memberID : sourceParty->GetMemberIDs()) {
        auto state = ClientState::GetEntityClientState(memberID, true);
        if (state && state->GetZone() == zone) {
          sourcePartyMembers.insert(memberID);
        }
      }
    }

    std::unordered_map<uint64_t, std::list<int32_t>> lootTimeEntityIDs;
    std::unordered_map<uint64_t, std::list<int32_t>> delayedLootEntityIDs;
    for (auto lPair : lStates) {
      auto lState = lPair.first;
      auto eState = lPair.second;

      int32_t lootEntityID = lState->GetEntityID();

      auto lootBody = lState->GetEntity();
      auto eBase = lootBody->GetEnemy();

      auto enemy = std::dynamic_pointer_cast<objects::Enemy>(eBase);

      // Create loot based off drops and send if any was added
      std::set<int32_t> validLooterIDs;
      bool timedAdjust = false;

      // Anyone can loot non-enemy bodies or the bodies of enemies
      // not damage by a player
      if (enemy && enemy->DamageSourcesCount() > 0) {
        // Only certain players can loot enemy bodies
        if (sourceState) {
          // Include skill source if a player entity
          validLooterIDs.insert(sourceState->GetWorldCID());
        } else {
          // Include anyone who damaged the entity that is in
          // the zone (ignore party rules)
          for (auto pair : enemy->GetDamageSources()) {
            auto state = ClientState::GetEntityClientState(pair.first, true);
            if (state && state->GetZone() == zone) {
              validLooterIDs.insert(pair.first);
            }
          }
        }

        timedAdjust = true;
        if (sourceParty) {
          switch (sourceParty->GetDropRule()) {
            case objects::Party::DropRule_t::DAMAGE_RACE: {
              // Highest damage dealer member wins
              std::map<uint64_t, int32_t> damageMap;
              for (auto pair : enemy->GetDamageSources()) {
                if (sourcePartyMembers.find(pair.first) !=
                    sourcePartyMembers.end()) {
                  damageMap[pair.second] = pair.first;
                }
              }

              if (damageMap.size() > 0) {
                validLooterIDs = {damageMap.rbegin()->second};
              }
            } break;
            case objects::Party::DropRule_t::RANDOM_LOOT: {
              // Randomly pick a member
              auto it = sourcePartyMembers.begin();
              size_t offset = (size_t)RNG(
                  uint16_t, 0, (uint16_t)(sourcePartyMembers.size() - 1));
              std::advance(it, offset);

              validLooterIDs = {*it};
            } break;
            case objects::Party::DropRule_t::FREE_LOOT: {
              // Every member is valid
              validLooterIDs = sourcePartyMembers;
              timedAdjust = false;
            } break;
            default:
              break;
          }
        }
      }

      auto drops = GetItemDrops(source, eState, sourceClient, zone);

      // Remove cooldown restricted drops
      std::set<int32_t> invalid;
      for (auto& pair : drops) {
        for (auto loot : pair.second) {
          int32_t cd = loot->GetCooldownRestrict();
          if (cd) {
            if (sourceCooldowns.find(cd) == sourceCooldowns.end()) {
              invalid.insert(cd);
            }
          }
        }

        pair.second.remove_if(
            [invalid](const std::shared_ptr<objects::ItemDrop>& drop) {
              return invalid.find(drop->GetCooldownRestrict()) != invalid.end();
            });
      }

      if (validLooterIDs.size() > 0) {
        lootBody->SetValidLooterIDs(validLooterIDs);

        if (timedAdjust) {
          // The last 60 seconds are fair game for everyone
          uint64_t delayedlootTime = (uint64_t)(now + 60000000);
          delayedLootEntityIDs[delayedlootTime].push_back(lootEntityID);
        }
      }

      auto nDrops = drops[(uint8_t)objects::DropSet::Type_t::NORMAL];
      auto dDrops = drops[(uint8_t)objects::DropSet::Type_t::DESTINY];

      uint64_t lootTime = 0;
      if (characterManager->CreateLootFromDrops(lootBody, nDrops, luck, false,
                                                maccaRate, magRate)) {
        // Bodies remain lootable for 120 seconds with loot
        lootTime = (uint64_t)(now + 120000000);
      } else {
        // Bodies remain visible for 10 seconds without loot
        lootTime = (uint64_t)(now + 10000000);
      }

      lootBody->SetLootTime(lootTime);
      lootTimeEntityIDs[lootTime].push_back(lootEntityID);

      if (firstClient) {
        zoneManager->SendLootBoxData(firstClient, lState, eState, true, true);
      }

      if (dDrops.size() > 0 && instance && sourceState) {
        // Always add at least one item
        auto filtered = characterManager->DetermineDrops(dDrops, 0, false);
        if (filtered.size() == 0) {
          filtered = {libcomp::Randomizer::GetEntry(dDrops)};
        }

        if (filtered.size() > 0) {
          // Create loot one drop at a time so we don't combine
          // two non-max stacks into one
          std::list<std::shared_ptr<objects::Loot>> loot;
          for (auto f : filtered) {
            auto dList = {f};
            for (auto l : characterManager->CreateLootFromDrops(dList)) {
              loot.push_back(l);
            }
          }

          zoneManager->UpdateDestinyBox(instance, sourceState->GetWorldCID(),
                                        loot);
        }
      }
    }

    for (auto pair : lootTimeEntityIDs) {
      zoneManager->ScheduleEntityRemoval(pair.first, zone, pair.second, 13);
    }

    for (auto pair : delayedLootEntityIDs) {
      ScheduleFreeLoot(pair.first, zone, pair.second, sourcePartyMembers);
    }

    if (multiZoneBosses.size() > 0) {
      zoneManager->MultiZoneBossKilled(zone, sourceState, multiZoneBosses);
    }

    // Update quest kill counts (ignore for demon only zones)
    if (sourceClient && questKills.size() > 0 &&
        zone->GetInstanceType() != InstanceType_t::DEMON_ONLY) {
      server->GetEventManager()->UpdateQuestKillCount(sourceClient, questKills);
    }

    if (zone->DiasporaMiniBossUpdated()) {
      // Update mini-boss count before firing defeat actions
      server->GetTokuseiManager()->UpdateDiasporaMinibossCount(zone);
    }

    // Perform defeat actions for all empty encounters
    HandleEncounterDefeat(source, zone, encounterGroups);

    ChannelClientConnection::FlushAllOutgoing(zConnections);

    // Loop through one last time and send all XP gained
    for (auto state : enemiesKilled) {
      auto eState = std::dynamic_pointer_cast<EnemyState>(state);
      if (eState) {
        auto enemy = eState->GetEntity();
        HandleKillXP(enemy, zone);
      }
    }

    if (dgEnemies.size() > 0) {
      HandleDigitalizeXP(source, dgEnemies, zone);
    }

    // Update crushing technique
    if (source->GetEntityType() == EntityType_t::CHARACTER && sourceClient) {
      auto cState = std::dynamic_pointer_cast<CharacterState>(source);
      auto character = cState ? cState->GetEntity() : nullptr;
      auto expertise =
          character ? character->GetExpertises(EXPERTISE_CRUSH_TECHNIQUE).Get()
                    : nullptr;
      if (expertise && !expertise->GetDisabled()) {
        int8_t lvl = cState->GetLevel();
        double rate =
            (double)cState->GetCorrectValue(CorrectTbl::RATE_EXPERTISE) * 0.01;

        int32_t points = 0;
        for (int8_t dLvl : levels) {
          int32_t up =
              (int32_t)(3.0 * (double)(5 + (dLvl / 10 - lvl / 10)) * rate);
          points += (5 + (up > 0 ? up : 0));
        }

        std::list<std::pair<uint8_t, int32_t>> expPoints;
        expPoints.push_back(
            std::pair<uint8_t, int32_t>(EXPERTISE_CRUSH_TECHNIQUE, points));

        characterManager->UpdateExpertisePoints(sourceClient, expPoints);
      }
    }

    // Update invoke values for active cooldowns
    if (sourceClient) {
      // Should only be one at a time but account for more just in case
      for (int32_t invokeID : {COOLDOWN_INVOKE_LAW, COOLDOWN_INVOKE_NEUTRAL,
                               COOLDOWN_INVOKE_CHAOS}) {
        if (sourceCooldowns.find(invokeID) != sourceCooldowns.end()) {
          characterManager->UpdateEventCounter(sourceClient, invokeID,
                                               (int32_t)enemiesKilled.size());
        }
      }
    }
  }

  // Handle additional kill values
  if (sourceClient) {
    for (auto& pair : killValues) {
      int32_t valSum = 0;
      for (auto entity : pair.second) {
        if (entity->GetKillValue() > 0) {
          valSum += entity->GetKillValue();
        }
      }

      if (valSum == 0) continue;

      switch (pair.first) {
        case objects::Spawn::KillValueType_t::SOUL_POINTS:
          characterManager->UpdateSoulPoints(sourceClient, valSum, true, true);
          break;
        case objects::Spawn::KillValueType_t::BETHEL:
          // If in an active Pentalpha instance, bethel is "held" until
          // the timer expires and is also divided amongst everyone who
          // has access to the instance. Otherwise it is given right
          // away. Both require active Pentalpha entries to actually do
          // anything.
          {
            float globalBonus =
                server->GetWorldSharedConfig()->GetBethelBonus();
            if (globalBonus != 0.f) {
              valSum = (int32_t)((double)valSum * (double)(1.f + globalBonus));
            }

            if (zone->GetInstanceType() == InstanceType_t::PENTALPHA &&
                instance->GetTimerStart() && !instance->GetTimerStop()) {
              // Distribute bethel by access CID, currently connected
              // or not and divide evenly so being in a group is not
              // the clear better way to run these instances
              valSum = (int32_t)ceil(
                  (double)valSum / (double)instance->OriginalAccessCIDsCount());
              for (auto c : managerConnection->GetEntityClients(
                       instance->GetOriginalAccessCIDs(), true)) {
                auto s = c->GetClientState();
                s->SetInstanceBethel(valSum + s->GetInstanceBethel());
              }
            } else {
              characterManager->UpdateBethel(sourceClient, valSum, true);
            }
          }
          break;
        case objects::Spawn::KillValueType_t::UB_POINTS:
          server->GetMatchManager()->UpdateUBPoints(sourceClient, valSum);
          break;
        case objects::Spawn::KillValueType_t::ZIOTITE: {
          // Ziotite can only be granted to a team and is increased
          // by 15% per team member over 1
          auto team = sourceState->GetTeam();
          if (team) {
            valSum =
                (int32_t)((float)valSum *
                          (1.f + (float)(team->MemberIDsCount() - 1) * 0.15f));
            server->GetMatchManager()->UpdateZiotite(
                team, valSum, 0, sourceState->GetWorldCID());
          }
        } break;
        case objects::Spawn::KillValueType_t::INHERITED:
        default:
          // Should have been handled above by the instance variant
          break;
      }
    }
  }

  if (playersKilled.size() > 0 || partnerDemonsKilled.size() > 0) {
    // If dead tokusei are disabled, recalculate player entities now
    auto tokuseiManager = server->GetTokuseiManager();
    if (tokuseiManager->DeadTokuseiDisabled()) {
      auto all = playersKilled;
      for (auto demon : partnerDemonsKilled) {
        all.push_back(demon);
      }

      tokuseiManager->Recalculate(all, true);
    }
  }
}

void SkillManager::HandleKillXP(const std::shared_ptr<objects::Enemy>& enemy,
                                const std::shared_ptr<Zone>& zone) {
  auto spawn = enemy->GetSpawnSource();

  int64_t totalXP = 0;
  if (spawn && spawn->GetXP() >= 0) {
    totalXP = spawn->GetXP();
  } else {
    // All non-spawn enemies have a calculated value
    totalXP = (int64_t)(enemy->GetCoreStats()->GetLevel() * 20);
  }

  if (totalXP <= 0) {
    return;
  }

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto managerConnection = server->GetManagerConnection();

  // Apply global XP bonus
  float globalXPBonus = server->GetWorldSharedConfig()->GetXPBonus();

  totalXP = (int64_t)((double)totalXP * (double)(1.f + globalXPBonus));

  // Apply zone XP multiplier
  totalXP = (int64_t)((double)totalXP * (double)zone->GetXPMultiplier());

  // Determine XP distribution
  // -Individuals/single parties gain max XP
  // -Multiple individuals/parties have XP distributed by damage dealt
  // -Party members gain alloted XP - ((number of members in the zone - 1) *
  // 10%)
  std::unordered_map<int32_t, uint64_t> playerDamage;
  std::unordered_map<uint32_t, uint64_t> partyDamage;
  std::unordered_map<uint32_t, std::shared_ptr<objects::Party>> parties;

  uint64_t totalDamage = 0;
  auto damageSources = enemy->GetDamageSources();
  for (auto damagePair : damageSources) {
    totalDamage = (uint64_t)(totalDamage + damagePair.second);
  }

  std::unordered_map<int32_t, std::shared_ptr<ChannelClientConnection>>
      clientMap;
  for (auto damagePair : damageSources) {
    auto c = managerConnection->GetEntityClient(damagePair.first, true);
    if (c) {
      clientMap[damagePair.first] = c;

      uint64_t dmg = damagePair.second;
      auto s = c->GetClientState();
      auto party = s->GetParty();
      if (party) {
        uint32_t partyID = party->GetID();
        if (partyDamage.find(partyID) == partyDamage.end()) {
          parties[partyID] = party;
          partyDamage[partyID] = dmg;
        } else {
          partyDamage[partyID] = partyDamage[partyID] + dmg;
        }
      } else {
        if (s->GetCharacterState()->GetZone() == zone) {
          playerDamage[s->GetWorldCID()] = dmg;
        } else {
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
  for (auto pPair : partyDamage) {
    for (int32_t memberID : parties[pPair.first]->GetMemberIDs()) {
      auto c = clientMap[memberID];
      if (c == nullptr) {
        c = server->GetManagerConnection()->GetEntityClient(memberID, true);
        clientMap[memberID] = c;
      }

      if (c) {
        auto s = c->GetClientState();
        if (s->GetCharacterState()->GetZone() == zone) {
          membersInZone[pPair.first].insert(memberID);
        }
      }
    }

    // No party members are in the zone
    if (membersInZone[pPair.first].size() == 0) {
      // Since no one in the party is still in the zone,
      // reduce the total damage since no member will
      // receive any XP
      totalDamage = (totalDamage - pPair.second);
    }
  }

  // Calculate the XP gains based on damage dealt by players
  // and parties still in the zone
  std::unordered_map<int32_t, int64_t> xpMap;
  for (auto pair : playerDamage) {
    xpMap[pair.first] = (int64_t)ceil((double)totalXP * (double)pair.second /
                                      (double)totalDamage);
  }

  for (auto pair : membersInZone) {
    double xp =
        (double)totalXP * (double)partyDamage[pair.first] / (double)totalDamage;

    int64_t partyXP =
        (int64_t)ceil(xp * 1.0 - ((double)(membersInZone.size() - 1) * 0.1));

    for (auto memberID : pair.second) {
      xpMap[memberID] = partyXP;
    }
  }

  // Apply the adjusted XP values to each player
  for (auto xpPair : xpMap) {
    auto c = clientMap[xpPair.first];
    if (c == nullptr) continue;

    auto s = c->GetClientState();
    std::list<std::shared_ptr<ActiveEntityState>> clientStates = {
        s->GetCharacterState()};
    clientStates.push_back(s->GetDemonState());
    for (auto cState : clientStates) {
      // Demons only get XP if they are alive, characters get
      // it regardless
      if (cState->Ready() &&
          (cState == s->GetCharacterState() || cState->IsAlive())) {
        int64_t finalXP = (int64_t)ceil(
            (double)xpPair.second *
            ((double)cState->GetCorrectValue(CorrectTbl::RATE_XP) * 0.01));
        if (finalXP > 0) {
          characterManager->UpdateExperience(c, finalXP, cState->GetEntityID());
        }
      }
    }
  }
}

void SkillManager::HandleDigitalizeXP(
    const std::shared_ptr<ActiveEntityState> source,
    const std::list<std::shared_ptr<ActiveEntityState>>& enemies,
    const std::shared_ptr<Zone>& zone) {
  // Grant digitalize XP to all players in the source's party that
  // have a digitalized character
  auto server = mServer.lock();
  auto managerConnection = server->GetManagerConnection();

  auto client = managerConnection->GetEntityClient(source->GetEntityID());
  if (!client) {
    // Not a player entity/not connected
    return;
  }

  auto characterManager = server->GetCharacterManager();
  float globalDXPBonus =
      server->GetWorldSharedConfig()->GetDigitalizePointBonus();

  // Sum points gained from all enemies
  int32_t dxp = 0;
  for (auto enemy : enemies) {
    dxp = (int32_t)(
        dxp +
        (int32_t)enemy->GetDevilData()->GetBattleData()->GetDigitalizeXP());
  }

  // Apply global XP bonus
  dxp = (int32_t)((double)dxp * (double)(1.f + globalDXPBonus));

  for (auto c : managerConnection->GetPartyConnections(client, true, false)) {
    auto state = c->GetClientState();

    // Only party members in the same zone get points
    if (state != client->GetClientState() && state->GetZone() != zone) {
      continue;
    }

    auto dgState = state->GetCharacterState()->GetDigitalizeState();
    uint8_t raceID = dgState ? dgState->GetRaceID() : 0;
    if (raceID) {
      std::unordered_map<uint8_t, int32_t> points;
      points[raceID] = dxp;

      characterManager->UpdateDigitalizePoints(c, points, true);
    }
  }
}

void SkillManager::HandleEncounterDefeat(
    const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<Zone>& zone,
    const std::unordered_map<uint32_t, uint32_t>& encounterGroups) {
  if (encounterGroups.size() == 0 ||
      (encounterGroups.size() == 1 &&
       encounterGroups.find(0) != encounterGroups.end())) {
    // Nothing to do
    return;
  }

  auto server = mServer.lock();
  auto actionManager = server->GetActionManager();
  auto sourceClient =
      server->GetManagerConnection()->GetEntityClient(source->GetEntityID());
  std::list<std::shared_ptr<objects::Action>> defeatActions;
  for (auto ePair : encounterGroups) {
    if (!ePair.first) continue;

    if (zone->EncounterDefeated(ePair.first, defeatActions)) {
      ActionOptions options;
      options.GroupID = ePair.first;
      options.NoEventInterrupt = true;

      // If the defeatActionSource has actions, those override the
      // group's default
      if (defeatActions.size() > 0) {
        actionManager->PerformActions(sourceClient, defeatActions,
                                      source->GetEntityID(), zone, options);
      } else {
        auto group = zone->GetDefinition()->GetSpawnGroups(ePair.second);
        if (group && group->DefeatActionsCount() > 0) {
          actionManager->PerformActions(sourceClient, group->GetDefeatActions(),
                                        source->GetEntityID(), zone, options);
        }
      }
    }
  }
}

void SkillManager::HandleRevives(
    const std::shared_ptr<Zone>& zone,
    const std::set<std::shared_ptr<ActiveEntityState>> revived,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto zoneManager = server->GetZoneManager();

  double maxLoss = 0.01;
  double lossDrop = 0.00005;

  auto rateIter = SVR_CONST.ADJUSTMENT_SKILLS.find(pSkill->SkillID);
  if (rateIter != SVR_CONST.ADJUSTMENT_SKILLS.end() &&
      rateIter->second[0] == 5) {
    maxLoss = (double)rateIter->second[2] / 100000;
    lossDrop = (double)rateIter->second[1] / 100000;
  }

  for (auto entity : revived) {
    libcomp::Packet p;
    if (characterManager->GetEntityRevivalPacket(p, entity, 6)) {
      zoneManager->BroadcastPacket(zone, p);
    }

    // Clear the death time-out if one exists
    if (entity->GetDeathTimeOut()) {
      auto entityCState =
          ClientState::GetEntityClientState(entity->GetEntityID());
      zoneManager->UpdateDeathTimeOut(entityCState, -1);
    }

    auto cState = std::dynamic_pointer_cast<CharacterState>(entity);
    if (cState && maxLoss > 0.f) {
      float xpLossPercent =
          (float)(maxLoss - (lossDrop * cState->GetCoreStats()->GetLevel()));
      if (xpLossPercent > 0.f) {
        characterManager->UpdateRevivalXP(cState, xpLossPercent);
      }
    }

    LogSkillManagerDebug([entity]() {
      return libcomp::String("%1 has been revived.\n")
          .Arg(entity->GetEntityLabel());
    });
  }

  // Trigger revival actions (but not respawn)
  auto reviveTriggers =
      zoneManager->GetZoneTriggers(zone, ZoneTrigger_t::ON_REVIVAL);
  if (reviveTriggers.size() > 0) {
    auto managerConnection = server->GetManagerConnection();
    for (auto entity : revived) {
      auto client = managerConnection->GetEntityClient(entity->GetEntityID());
      zoneManager->HandleZoneTriggers(zone, reviveTriggers, entity, client);
    }
  }

  // Check if we need to recalculate player tokusei
  auto tokuseiManager = server->GetTokuseiManager();
  if (tokuseiManager->DeadTokuseiDisabled()) {
    std::list<std::shared_ptr<ActiveEntityState>> playerEntities;
    for (auto e : revived) {
      switch (e->GetEntityType()) {
        case EntityType_t::CHARACTER:
        case EntityType_t::PARTNER_DEMON:
          playerEntities.push_back(e);
          break;
        default:
          break;
      }
    }

    if (playerEntities.size() > 0) {
      tokuseiManager->Recalculate(playerEntities, true);
    }
  }
}

bool SkillManager::ApplyZoneSpecificEffects(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  bool changed = false;
  switch (pSkill->CurrentZone->GetInstanceType()) {
    case InstanceType_t::PVP:
      // Update end of match stats
      UpdatePvPStats(pSkill);
      break;
    case InstanceType_t::DEMON_ONLY: {
      // If a partner demon was killed or revived, mirror the effect on
      // the associated character
      std::list<std::shared_ptr<ActiveEntityState>> revive;
      std::list<std::shared_ptr<ActiveEntityState>> kill;
      for (auto& target : pSkill->Targets) {
        bool revived = (target.Flags1 & FLAG1_REVIVAL) != 0;
        bool killed = (target.Flags1 & FLAG1_LETHAL) != 0;

        auto eState = target.EntityState;
        if (eState->GetEntityType() == EntityType_t::PARTNER_DEMON &&
            (revived || killed)) {
          auto demonCState =
              ClientState::GetEntityClientState(eState->GetEntityID());
          if (demonCState) {
            auto cState = demonCState->GetCharacterState();
            if (revived && !cState->IsAlive()) {
              revive.push_back(cState);
            } else if (killed && cState->IsAlive()) {
              kill.push_back(cState);
            }
          }
        }
      }

      for (auto cState : revive) {
        // Revive the character with 1 HP
        if (cState->SetHPMP(1, -1, true, true)) {
          SkillTargetResult target;
          target.EntityState = cState;
          target.Damage1 = 1;
          target.Damage1Type = DAMAGE_TYPE_HEALING;

          target.Flags1 |= FLAG1_REVIVAL;
          target.RecalcTriggers.insert(TokuseiConditionType::CURRENT_HP);

          pSkill->Targets.push_back(target);
          changed = true;
        }
      }

      for (auto cState : kill) {
        // Kill the character
        if (cState->SetHPMP(0, -1, false, true)) {
          SkillTargetResult target;
          target.EntityState = cState;
          target.Damage1 = MAX_PLAYER_HP_MP;
          target.Damage1Type = DAMAGE_TYPE_GENERIC;

          target.Flags1 |= FLAG1_LETHAL;
          target.RecalcTriggers.insert(TokuseiConditionType::CURRENT_HP);
          target.EffectCancellations |=
              EFFECT_CANCEL_HIT | EFFECT_CANCEL_DEATH | EFFECT_CANCEL_DAMAGE;

          pSkill->Targets.push_back(target);
          changed = true;
        }
      }
    } break;
    default:
      break;
  }

  return changed;
}

void SkillManager::UpdatePvPStats(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  auto instance = pSkill->CurrentZone->GetInstance();
  auto pvpStats = instance ? instance->GetPvPStats() : nullptr;
  if (!MatchManager::PvPActive(instance)) {
    return;
  }

  bool sourceIsDemon = false;
  int32_t sourceID = pSkill->EffectiveSource->GetEntityID();
  if (pSkill->EffectiveSource->GetEntityType() == EntityType_t::PARTNER_DEMON) {
    auto state = ClientState::GetEntityClientState(sourceID);
    if (state) {
      sourceID = state->GetCharacterState()->GetEntityID();
      sourceIsDemon = true;
    }
  }

  auto definitionManager = mServer.lock()->GetDefinitionManager();

  bool firstDamageSet = pvpStats->FirstDamageCount() != 0;

  std::unordered_map<int32_t, int32_t> damageDealt;
  std::unordered_map<int32_t, int32_t> damageDealtMax;
  std::set<int32_t> killed;
  std::set<int32_t> demonsKilled;
  std::set<int32_t> othersKilled;
  int32_t gStatus = 0;
  std::unordered_map<int32_t, int32_t> bStatus;
  for (auto& target : pSkill->Targets) {
    if (target.IndirectTarget) continue;

    bool targetIsDemon = false;
    int32_t entityID = target.EntityState->GetEntityID();
    if (target.EntityState->GetEntityType() == EntityType_t::PARTNER_DEMON) {
      auto state = ClientState::GetEntityClientState(entityID);
      if (state) {
        entityID = state->GetCharacterState()->GetEntityID();
        targetIsDemon = true;
      }
    }

    if (target.Flags1 & FLAG1_LETHAL) {
      if (targetIsDemon) {
        demonsKilled.insert(entityID);
      } else {
        killed.insert(entityID);
      }

      // Killing your own entities count as deaths, not kills
      if (entityID != sourceID) {
        othersKilled.insert(entityID);
      }
    }

    for (auto& sPair : target.AddedStatuses) {
      auto& change = sPair.second;
      if (change.Stack) {
        auto effect = definitionManager->GetStatusData(change.Type);
        switch (effect->GetCommon()->GetCategory()->GetMainCategory()) {
          case STATUS_CATEGORY_BAD:
            if (bStatus.find(entityID) != bStatus.end()) {
              bStatus[entityID]++;
            } else {
              bStatus[entityID] = 1;
            }
            break;
          case STATUS_CATEGORY_GOOD:
            gStatus++;
            break;
          default:
            break;
        }
      }
    }

    if (target.EntityState != pSkill->EffectiveSource &&
        (target.Damage1Type == DAMAGE_TYPE_GENERIC ||
         target.Damage2Type == DAMAGE_TYPE_GENERIC)) {
      int32_t damage = target.Damage1 + target.Damage2;
      if (!firstDamageSet) {
        pvpStats->InsertFirstDamage(sourceID);
        pvpStats->InsertFirstDamageTaken(entityID);
      }

      auto it = damageDealtMax.find(entityID);
      if (it == damageDealtMax.end()) {
        damageDealtMax[entityID] = 0;
        it = damageDealtMax.find(entityID);
      }

      if (it->second < damage) {
        it->second = damage;
      }

      if (damageDealt.find(entityID) != damageDealt.end()) {
        damageDealt[entityID] += damage;
      } else {
        damageDealt[entityID] = damage;
      }
    }
  }

  // Update source stats
  auto stats = pvpStats->GetPlayerStats(sourceID);
  if (stats) {
    if (sourceIsDemon) {
      stats->SetDemonKills(
          (uint16_t)(stats->GetDemonKills() + othersKilled.size()));
    } else {
      stats->SetKills((uint16_t)(stats->GetKills() + othersKilled.size()));
    }

    stats->SetGoodStatus((uint16_t)(stats->GetGoodStatus() + gStatus));

    int32_t maxDamage = stats->GetDamageMax();
    for (auto& dPair : damageDealtMax) {
      if (maxDamage < dPair.second) {
        maxDamage = dPair.second;
      }
    }
    stats->SetDamageMax(maxDamage);

    int32_t damageSum = stats->GetDamageSum();
    for (auto& dPair : damageDealt) {
      damageSum += dPair.second;
    }
    stats->SetDamageSum(damageSum);

    for (auto& sPair : bStatus) {
      stats->SetBadStatus((uint16_t)(stats->GetBadStatus() + sPair.second));
    }
  }

  // Update target deaths
  for (int32_t kill : killed) {
    stats = pvpStats->GetPlayerStats(kill);
    if (stats) {
      stats->SetDeaths((uint16_t)(stats->GetDeaths() + 1));
    }
  }

  // Update target demon deaths
  for (int32_t kill : demonsKilled) {
    stats = pvpStats->GetPlayerStats(kill);
    if (stats) {
      stats->SetDemonDeaths((uint16_t)(stats->GetDemonDeaths() + 1));
    }
  }

  // Update target damage max
  for (auto& pair : damageDealtMax) {
    stats = pvpStats->GetPlayerStats(pair.first);
    if (stats) {
      if (stats->GetDamageMaxTaken() < pair.second) {
        stats->SetDamageMaxTaken(pair.second);
      }
    }
  }

  // Update target damage sum
  for (auto& pair : damageDealt) {
    stats = pvpStats->GetPlayerStats(pair.first);
    if (stats) {
      stats->SetDamageSumTaken(stats->GetDamageSum() + pair.second);
    }
  }

  // Update target bad status taken
  for (auto& sPair : bStatus) {
    stats = pvpStats->GetPlayerStats(sPair.first);
    if (stats) {
      stats->SetBadStatusTaken(
          (uint16_t)(stats->GetBadStatusTaken() + sPair.second));
    }
  }
}

bool SkillManager::ApplyNegotiationDamage(
    const std::shared_ptr<ActiveEntityState>& source, SkillTargetResult& target,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  auto eState = std::dynamic_pointer_cast<EnemyState>(target.EntityState);
  auto enemy = eState ? eState->GetEntity() : nullptr;
  if (!enemy) {
    return false;
  }

  auto spawn = enemy->GetSpawnSource();
  if (enemy->GetCoreStats()->GetLevel() > source->GetLevel()) {
    // Enemies that are a higher level cannot be negotiated with
    return false;
  }

  bool exists = false;
  auto talkPoints = eState->GetTalkPoints(source->GetEntityID(), exists);
  auto demonData = eState->GetDevilData();
  auto negData = demonData->GetNegotiation();
  int8_t affThreshold = (int8_t)(100 - negData->GetAffabilityThreshold());
  int8_t fearThreshold = (int8_t)(100 - negData->GetFearThreshold());

  if (talkPoints.first >= affThreshold || talkPoints.second >= fearThreshold) {
    // Done but not at zero, print the message and move on
    target.TalkFlags =
        talkPoints.first >= affThreshold ? TALK_DONE_1 : TALK_DONE_2;
    return false;
  }

  int32_t talkType = 0;
  int8_t expID = 0;
  switch (pSkill->Definition->GetBasic()->GetActionType()) {
    case objects::MiSkillBasicData::ActionType_t::TALK:
      talkType = 1;
      expID = EXPERTISE_TALK;
      break;
    case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
      talkType = 2;
      expID = EXPERTISE_INTIMIDATE;
      break;
    case objects::MiSkillBasicData::ActionType_t::TAUNT:
      talkType = 3;
      expID = EXPERTISE_TAUNT;
      break;
    default:
      // Can continue but no rate adjustments will apply
      break;
  }

  if (exists &&
      (talkPoints.first + (int8_t)negData->GetAffabilityThreshold()) == 0 &&
      (talkPoints.second + (int8_t)negData->GetFearThreshold()) == 0) {
    // Locked in a negative result state
    target.TalkFlags = talkType != 2 ? TALK_FOFF_1 : TALK_FOFF_2;
    return false;
  }

  // No points in anything but still primary talk skill means
  // the skill will always result in a join
  bool isTalkAction = IsTalkSkill(pSkill->Definition, true);
  bool avoided =
      (target.Flags1 & FLAG1_GUARDED) != 0 || (target.Flags1 & FLAG1_DODGED);
  bool autoJoin = isTalkAction && !pSkill->TalkAffSuccess && !avoided &&
                  !pSkill->TalkAffFailure && !pSkill->TalkFearSuccess &&
                  !pSkill->TalkFearFailure;

  bool success = false;
  if (autoJoin) {
    success = true;
    talkPoints.first = affThreshold;
    talkPoints.second = fearThreshold;
  } else {
    double talkSuccess =
        spawn && !avoided ? (double)(100 - spawn->GetTalkResist()) : 0.0;

    auto calcState = GetCalculatedState(source, pSkill, false, eState);
    if (talkSuccess != 0.0 && talkType) {
      auto adjust = mServer.lock()->GetTokuseiManager()->GetAspectMap(
          source, TokuseiAspectType::TALK_RATE, std::set<int32_t>{0, talkType},
          calcState);

      for (auto pair : adjust) {
        talkSuccess += pair.second;
      }

      auto cSource = std::dynamic_pointer_cast<CharacterState>(source);
      if (cSource && talkSuccess < 100.0) {
        // Boost success based on expertise
        talkSuccess +=
            (double)(cSource->GetExpertiseRank(EXPERTISE_DEMONOLOGY) / 10) *
            2.0;
        talkSuccess +=
            (double)(cSource->GetExpertiseRank((uint8_t)expID) / 10) * 3.0;
      }
    }

    success =
        talkSuccess > 0.0 && RNG(uint16_t, 1, 100) <= (uint16_t)talkSuccess;
    int16_t aff =
        (int16_t)(talkPoints.first +
                  (success ? pSkill->TalkAffSuccess : pSkill->TalkAffFailure));
    int16_t fear =
        (int16_t)(talkPoints.second + (success ? pSkill->TalkFearSuccess
                                               : pSkill->TalkFearFailure));

    // Don't let the sums drop below 0 or go over the threshold
    if (aff > affThreshold) {
      talkPoints.first = affThreshold;
    } else if (aff < (int16_t)(-negData->GetAffabilityThreshold())) {
      talkPoints.first = (int8_t)(-negData->GetAffabilityThreshold());
    } else {
      talkPoints.first = (int8_t)aff;
    }

    if (fear > fearThreshold) {
      talkPoints.second = fearThreshold;
    } else if (fear < (int16_t)(-negData->GetFearThreshold())) {
      talkPoints.second = (int8_t)(-negData->GetFearThreshold());
    } else {
      talkPoints.second = (int8_t)fear;
    }

    if (!isTalkAction) {
      // Non-talk skills can never hit the threshold
      if (talkPoints.first >= affThreshold) {
        talkPoints.first = (int8_t)(affThreshold - 1);
      }

      if (talkPoints.second >= fearThreshold) {
        talkPoints.second = (int8_t)(fearThreshold - 1);
      }
    }

    LogSkillManagerDebug([source, target, success, talkPoints, affThreshold,
                          fearThreshold, pSkill]() {
      return libcomp::String(
                 "%1 talk points became %2/%3 (max %4/%5) from skill %6 %7 "
                 "when used by %8.\n")
          .Arg(target.EntityState->GetEntityLabel())
          .Arg(talkPoints.first)
          .Arg(talkPoints.second)
          .Arg(affThreshold)
          .Arg(fearThreshold)
          .Arg(pSkill->SkillID)
          .Arg(success ? "success" : "failure")
          .Arg(source->GetEntityLabel());
    });
  }

  eState->SetTalkPoints(source->GetEntityID(), talkPoints);

  if ((talkPoints.first + (int8_t)negData->GetAffabilityThreshold()) == 0 &&
      (talkPoints.second + (int8_t)negData->GetFearThreshold()) == 0) {
    // Points hit 0/0, the enemy is now locked in a negative state
    target.TalkFlags =
        expID == EXPERTISE_INTIMIDATE ? TALK_FOFF_2 : TALK_FOFF_1;

    return false;
  }

  bool affPass = talkPoints.first >= affThreshold;
  bool fearPass = talkPoints.second >= fearThreshold;
  if (affPass || fearPass) {
    // Determine which outcomes are valid and randomly
    // select one
    int32_t minVal = 1;
    int32_t maxVal = 6;

    bool canJoin = true;
    bool canGift = true;
    if (autoJoin) {
      minVal = 1;
      maxVal = 1;
    } else {
      uint8_t talkResults = spawn ? spawn->GetTalkResults() : 3;
      if ((talkResults & SPAWN_TALK_RESULT_JOIN) == 0) {
        canJoin = false;
        maxVal -= 2;
      }

      if ((talkResults & SPAWN_TALK_RESULT_GIFT) == 0) {
        canGift = false;
        maxVal -= 2;
      }
    }

    int32_t outcome = RNG(int32_t, minVal, maxVal);

    if (!autoJoin) {
      // Shift the outcome to the proper position if some
      // results are not available
      if (!canJoin) {
        outcome += 2;
      }

      if (!canGift && outcome >= 3 && outcome <= 4) {
        outcome += 2;
      }
    }

    switch (outcome) {
      case 1:
      case 2:
        target.TalkFlags = affPass ? TALK_JOIN_1 : TALK_JOIN_2;
        break;
      case 3:
      case 4:
        target.TalkFlags = affPass ? TALK_GIVE_ITEM_1 : TALK_GIVE_ITEM_2;
        break;
      case 5:
        target.TalkFlags = affPass ? TALK_DONE_1 : TALK_DONE_2;
        break;
      case 6:
      default:
        break;
    }

    if ((target.TalkFlags == TALK_GIVE_ITEM_1 ||
         target.TalkFlags == TALK_GIVE_ITEM_2) &&
        (!spawn ||
         (spawn->GiftsCount() == 0 && spawn->GiftSetIDsCount() == 0))) {
      // No gifts mapped, default instead
      target.TalkFlags = 0;
    }

    if (target.TalkFlags == 0) {
      // If all else fails, leave
      target.TalkFlags = affPass ? TALK_LEAVE_1 : TALK_LEAVE_2;
    }

    target.TalkDone = true;
  } else {
    if (success) {
      // Reseponse is based on the max value between the two point
      // types
      int32_t currentAff = (int32_t)negData->GetAffabilityThreshold() +
                           (int32_t)talkPoints.first;
      int32_t currentFear =
          (int32_t)negData->GetFearThreshold() + (int32_t)talkPoints.second;

      int32_t currentVal = currentAff >= currentFear ? currentAff : currentFear;
      if (currentVal < 30) {
        target.TalkFlags = TALK_SUCCESS_LVL1;
      } else if (currentVal < 60) {
        target.TalkFlags = TALK_SUCCESS_LVL2;
      } else if (currentVal < 90) {
        target.TalkFlags = TALK_SUCCESS_LVL3;
      } else {
        target.TalkFlags = TALK_SUCCESS_LVL4;
      }
    } else {
      target.TalkFlags = TALK_FAIL;
    }
  }

  // If the target is AI controlled, update aggro
  auto aiState = target.EntityState->GetAIState();
  if (aiState) {
    bool isTaunt = pSkill->FunctionID == SVR_CONST.SKILL_TAUNT;
    if ((success && isTaunt) ||
        (!success && !isTaunt && !aiState->GetTargetEntityID())) {
      // Taunt skills aggro on success
      // Non-taunt skills aggro on failure (if no target set)
      mServer.lock()->GetAIManager()->UpdateAggro(target.EntityState,
                                                  source->GetEntityID());
    }
  }

  return target.TalkDone;
}

void SkillManager::HandleNegotiations(
    const std::shared_ptr<ActiveEntityState> source,
    const std::shared_ptr<Zone>& zone,
    const std::list<std::pair<std::shared_ptr<ActiveEntityState>, uint8_t>>
        talkDone) {
  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto zoneManager = server->GetZoneManager();
  auto zConnections = zone->GetConnectionList();

  // Gather all enemy IDs that will be removed
  std::unordered_map<uint32_t, uint32_t> encounterGroups;
  std::unordered_map<int32_t, std::list<int32_t>> removedEnemies;
  for (auto pair : talkDone) {
    if (pair.second != TALK_DONE_1 && pair.second != TALK_DONE_2) {
      int32_t removeMode = 0;
      switch (pair.second) {
        case TALK_JOIN_1:
        case TALK_JOIN_2:
          removeMode = 5;
          break;
        case TALK_GIVE_ITEM_1:
        case TALK_GIVE_ITEM_2:
          removeMode = 6;
          break;
        case TALK_LEAVE_1:
        case TALK_LEAVE_2:
          removeMode = 8;
          break;
        default:
          break;
      }

      // Get encounter information
      auto eState = std::dynamic_pointer_cast<EnemyState>(pair.first);
      auto enemy = eState->GetEntity();
      if (enemy && enemy->GetEncounterID()) {
        encounterGroups[enemy->GetEncounterID()] = enemy->GetSpawnGroupID();
      }

      // Remove all opponents
      characterManager->AddRemoveOpponent(false, pair.first, nullptr);
      zone->RemoveEntity(pair.first->GetEntityID(), 1);
      removedEnemies[removeMode].push_back(pair.first->GetEntityID());
    }
  }

  for (auto pair : removedEnemies) {
    zoneManager->RemoveEntitiesFromZone(zone, pair.second, pair.first, true);
  }

  auto sourceClient =
      server->GetManagerConnection()->GetEntityClient(source->GetEntityID());
  auto sourceState = sourceClient ? sourceClient->GetClientState() : nullptr;
  if (!sourceState) {
    // Not a player, flush connections and stop
    ChannelClientConnection::FlushAllOutgoing(zConnections);
    return;
  }

  // Partner demon can gain familiarity from successful negotiations
  int32_t fGain = 0;
  auto partnerDef = sourceState->GetDemonState()->GetDevilData();
  auto fType =
      partnerDef ? server->GetServerDataManager()->GetDemonFamiliarityTypeData(
                       partnerDef->GetFamiliarity()->GetFamiliarityType())
                 : nullptr;

  // Keep track of demons that have "joined" for demon quests
  std::unordered_map<uint32_t, int32_t> joined;

  // Handle the results of negotiations that result in an enemy being removed
  std::unordered_map<std::shared_ptr<LootBoxState>, std::shared_ptr<EnemyState>>
      lStates;
  for (auto pair : talkDone) {
    auto eState = std::dynamic_pointer_cast<EnemyState>(pair.first);
    if (pair.second != TALK_DONE_1 && pair.second != TALK_DONE_2) {
      auto enemy = eState->GetEntity();

      std::shared_ptr<objects::LootBox> lBox;
      switch (pair.second) {
        case TALK_JOIN_1:
        case TALK_JOIN_2: {
          lBox = std::make_shared<objects::LootBox>();
          lBox->SetType(objects::LootBox::Type_t::EGG);
          lBox->SetEnemy(enemy);

          auto demonLoot = std::make_shared<objects::Loot>();
          demonLoot->SetType(enemy->GetType());
          demonLoot->SetCount(1);
          lBox->SetLoot(0, demonLoot);

          if (joined.find(enemy->GetType()) == joined.end()) {
            joined[enemy->GetType()] = 1;
          } else {
            joined[enemy->GetType()]++;
          }
        } break;
        case TALK_GIVE_ITEM_1:
        case TALK_GIVE_ITEM_2: {
          lBox = std::make_shared<objects::LootBox>();
          lBox->SetType(objects::LootBox::Type_t::GIFT_BOX);
          lBox->SetEnemy(enemy);

          auto drops = GetItemDrops(source, eState, sourceClient, zone, true);
          auto gifts = drops[(uint8_t)objects::DropSet::Type_t::NORMAL];
          characterManager->CreateLootFromDrops(lBox, gifts, source->GetLUCK(),
                                                true);
        } break;
        default:
          break;
      }

      if (lBox) {
        auto lState = std::make_shared<LootBoxState>(lBox);
        lState->SetCurrentX(eState->GetDestinationX());
        lState->SetCurrentY(eState->GetDestinationY());
        lState->SetCurrentRotation(eState->GetDestinationRotation());
        lState->SetEntityID(server->GetNextEntityID());
        lStates[lState] = eState;

        zone->AddLootBox(lState);
      }
    }

    if (fType) {
      fGain = fGain + (int32_t)fType->GetTalkSuccess();
    }
  }

  // Show each look box and schedule them for cleanup after their
  // loot time passes
  if (lStates.size() > 0) {
    // Spawned boxes remain lootable for 120 seconds
    uint64_t now = ChannelServer::GetServerTime();

    auto firstClient = zConnections.size() > 0 ? zConnections.front() : nullptr;
    auto sourceParty = sourceState->GetParty();
    std::set<int32_t> sourcePartyMembers =
        sourceParty ? sourceParty->GetMemberIDs() : std::set<int32_t>();

    std::unordered_map<uint64_t, std::list<int32_t>> lootTimeEntityIDs;
    std::unordered_map<uint64_t, std::list<int32_t>> delayedLootEntityIDs;
    for (auto lPair : lStates) {
      auto lState = lPair.first;
      auto eState = lPair.second;

      auto lootBox = lState->GetEntity();
      lootBox->InsertValidLooterIDs(sourceState->GetWorldCID());

      uint64_t lootTime = 0;
      uint64_t delayedLootTime = 0;
      if (lootBox->GetType() == objects::LootBox::Type_t::EGG) {
        // Demon eggs remain lootable for 300 seconds
        lootTime = (uint64_t)(now + 300000000);

        // Free loot starts 120 seconds in
        delayedLootTime = (uint64_t)(now + 120000000);
      } else {
        // Gift boxes remain lootable for 120 seconds
        lootTime = (uint64_t)(now + 120000000);

        if (sourceParty) {
          if (sourceParty->GetDropRule() ==
              objects::Party::DropRule_t::FREE_LOOT) {
            lootBox->SetValidLooterIDs(sourcePartyMembers);
          } else {
            // Free loot starts 60 seconds in
            delayedLootTime = (uint64_t)(now + 60000000);
          }
        }
      }
      lootBox->SetLootTime(lootTime);

      if (firstClient) {
        zoneManager->SendLootBoxData(firstClient, lState, eState, true, true);
      }

      int32_t lootEntityID = lState->GetEntityID();
      lootTimeEntityIDs[lootTime].push_back(lootEntityID);

      if (sourceParty && delayedLootTime) {
        delayedLootEntityIDs[delayedLootTime].push_back(lootEntityID);
      }
    }

    for (auto pair : lootTimeEntityIDs) {
      zoneManager->ScheduleEntityRemoval(pair.first, zone, pair.second, 13);
    }

    for (auto pair : delayedLootEntityIDs) {
      ScheduleFreeLoot(pair.first, zone, pair.second, sourcePartyMembers);
    }
  }

  if (zone->DiasporaMiniBossUpdated()) {
    server->GetTokuseiManager()->UpdateDiasporaMinibossCount(zone);
  }

  if (encounterGroups.size() > 0) {
    HandleEncounterDefeat(source, zone, encounterGroups);
  }

  if (joined.size() > 0 && sourceClient) {
    // Update demon quest if active
    auto eventManager = server->GetEventManager();
    for (auto& pair : joined) {
      eventManager->UpdateDemonQuestCount(sourceClient,
                                          objects::DemonQuest::Type_t::CONTRACT,
                                          pair.first, pair.second);
    }
  }

  ChannelClientConnection::FlushAllOutgoing(zConnections);

  // Lastly update familiarity
  if (fGain) {
    characterManager->UpdateFamiliarity(sourceClient, fGain, true, true);
  }
}

void SkillManager::HandleSkillLearning(
    const std::shared_ptr<ActiveEntityState> entity,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  double iMod1 =
      (double)pSkill->Definition->GetAcquisition()->GetInheritanceModifier();

  auto dState = std::dynamic_pointer_cast<DemonState>(entity);
  if (!dState || !dState->Ready() || iMod1 <= 0) {
    return;
  }

  bool isSource = pSkill->Activated->GetSourceEntity() == entity;
  auto learningSkills = dState->GetLearningSkills(pSkill->EffectiveAffinity);
  if (learningSkills.size() == 0) {
    return;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto managerConnection = server->GetManagerConnection();

  auto dbChanges = libcomp::DatabaseChangeSet::Create();

  std::list<std::pair<uint32_t, int16_t>> updateMap;
  for (auto iSkill : learningSkills) {
    auto iSkillData = definitionManager->GetSkillData(iSkill->GetSkill());
    auto iMod2 =
        iSkillData
            ? (double)iSkillData->GetAcquisition()->GetInheritanceModifier()
            : 0.0;
    if (iMod2 > 0.0) {
      uint16_t updateProgress = 0;
      if (isSource) {
        updateProgress = (uint16_t)floor(pow((iMod1 * 40.0) / iMod2, 2) * 0.25);
      } else {
        updateProgress = (uint16_t)floor(pow((iMod1 * 40.0) / iMod2, 2));
      }

      // Apply rate from demon
      int16_t learnRate = dState->GetCorrectValue(CorrectTbl::RATE_EXPERTISE);
      if (learnRate <= 0) {
        updateProgress = 0;
      } else if (updateProgress > 0 && learnRate != 100) {
        float calc = (float)updateProgress * (float)learnRate * 0.01f;
        if (calc > (float)std::numeric_limits<uint16_t>::max()) {
          updateProgress = std::numeric_limits<uint16_t>::max();
        } else {
          updateProgress = (uint16_t)calc;
        }
      }

      if (updateProgress > 0) {
        int16_t progress = dState->UpdateLearningSkill(iSkill, updateProgress);
        updateMap.push_back(
            std::pair<uint32_t, int16_t>(iSkill->GetSkill(), progress));

        dbChanges->Update(iSkill);
      }
    }
  }

  if (updateMap.size() > 0) {
    auto dClient =
        managerConnection->GetEntityClient(dState->GetEntityID(), false);
    if (dClient) {
      libcomp::Packet p;
      p.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_INHERIT_SKILL_UPDATED);
      p.WriteS32Little(dState->GetEntityID());
      p.WriteS32Little((int32_t)updateMap.size());
      for (auto pair : updateMap) {
        p.WriteU32Little(pair.first);
        p.WriteS32Little((int32_t)pair.second);
      }

      dClient->SendPacket(p);
    }

    dState->RefreshLearningSkills(pSkill->EffectiveAffinity, definitionManager);

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
  }
}

void SkillManager::HandleDurabilityDamage(
    const std::shared_ptr<ActiveEntityState> entity,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  auto cState = std::dynamic_pointer_cast<CharacterState>(entity);
  auto character = cState ? cState->GetEntity() : nullptr;
  if (!cState || !character || !cState->Ready()) {
    return;
  }

  const size_t WEAPON_IDX =
      (size_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON;

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();

  auto client =
      server->GetManagerConnection()->GetEntityClient(cState->GetEntityID());
  if (!client) {
    return;
  }

  bool isSource = pSkill->Activated->GetSourceEntity() == entity;
  if (isSource) {
    if (pSkill->FunctionID == SVR_CONST.SKILL_DURABILITY_DOWN) {
      // Explicit set to visible durability
      auto params = pSkill->Definition->GetSpecial()->GetSpecialParams();
      auto equip = character->GetEquippedItems((size_t)params[0]).Get();
      if (equip) {
        characterManager->UpdateDurability(client, equip, params[1] * -1000);
      }
    }

    // Decrease weapon durability by value * 2
    // (do not scale for target count hit)
    auto weapon = character->GetEquippedItems(WEAPON_IDX).Get();
    if (!weapon) {
      return;
    }

    uint16_t weaponDamage =
        pSkill->Definition->GetDamage()->GetBreakData()->GetWeapon();
    if (!weaponDamage) {
      return;
    }

    double knowledgeRank = (double)pSkill->KnowledgeRank;

    int32_t durabilityLoss = weaponDamage * 2;
    if (knowledgeRank) {
      // Decrease damage more for lower values
      durabilityLoss =
          (int32_t)floor(pow(knowledgeRank, 2) / 450.0 -
                         (0.4275 * knowledgeRank) + (double)durabilityLoss);
    }

    if (durabilityLoss <= 0) {
      // Floor at 1 point
      durabilityLoss = 1;
    }

    characterManager->UpdateDurability(client, weapon, -durabilityLoss);
  } else {
    // Decrease armor durability on everything equipped but the weapon by value
    std::list<std::shared_ptr<objects::Item>> otherEquipment;
    for (size_t i = 0; i < 15; i++) {
      if (i != WEAPON_IDX) {
        auto equip = character->GetEquippedItems(i).Get();
        if (equip) {
          otherEquipment.push_back(equip);
        }
      }
    }

    if (otherEquipment.size() == 0) {
      return;
    }

    uint16_t armorDamage =
        pSkill->Definition->GetDamage()->GetBreakData()->GetArmor();
    if (!armorDamage) {
      return;
    }

    double defRank = (double)cState->GetExpertiseRank(EXPERTISE_SURVIVAL) +
                     cState->GetExpertiseRank(EXPERTISE_CHAIN_R_PRESENCE,
                                              server->GetDefinitionManager());

    int32_t durabilityLoss = (int32_t)armorDamage;
    if (defRank) {
      // Decrease damage more for lower values
      double adjust = defRank / 25000.0;
      durabilityLoss = (int32_t)ceil(
          (double)durabilityLoss * (1.0 - (adjust + (0.12 * defRank)) / 10.0));
    }

    if (durabilityLoss <= 0) {
      // Floor at 1 point
      durabilityLoss = 1;
    }

    std::unordered_map<std::shared_ptr<objects::Item>, int32_t> equipMap;
    for (auto equip : otherEquipment) {
      equipMap[equip] = -durabilityLoss;
    }

    characterManager->UpdateDurability(client, equipMap);
  }
}

void SkillManager::HandleFusionGauge(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  auto def = pSkill->Definition;
  bool isFusionSkill = pSkill->FunctionID == SVR_CONST.SKILL_DEMON_FUSION;
  auto actionType = def->GetBasic()->GetActionType();
  if (isFusionSkill ||
      actionType >
          objects::MiSkillBasicData::MiSkillBasicData::ActionType_t::DODGE) {
    return;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      pSkill->Activated->GetSourceEntity());

  auto server = mServer.lock();
  auto client = server->GetManagerConnection()->GetEntityClient(
      source->GetEntityID(), false);
  if (client) {
    // Raise the fusion gauge
    bool isDemon = std::dynamic_pointer_cast<DemonState>(source) != nullptr;
    bool higherLevel = false;
    bool skillHit = false;

    int8_t lvl = source->GetLevel();
    for (auto& target : pSkill->Targets) {
      if (target.EntityState != source && !target.GuardModifier &&
          !target.HitAvoided && !target.HitAbsorb) {
        skillHit = true;
        if (target.EntityState->GetLevel() > lvl) {
          higherLevel = true;
          break;
        }
      }
    }

    if (skillHit) {
      int32_t points =
          (int32_t)libcomp::FUSION_GAUGE_GROWTH[(size_t)actionType][(size_t)(
              (isDemon ? 2 : 0) + (higherLevel ? 1 : 0))];

      float fgBonus = server->GetWorldSharedConfig()->GetFusionGaugeBonus();
      if (fgBonus > 0.f) {
        points = (int32_t)ceil((double)points * (double)(1.f + fgBonus));
      }

      server->GetCharacterManager()->UpdateFusionGauge(client, points, true);
    }
  }
}

void SkillManager::InterruptEvents(const std::set<int32_t>& worldCIDs) {
  auto server = mServer.lock();
  auto eventManager = server->GetEventManager();
  auto managerConnection = server->GetManagerConnection();
  for (int32_t worldCID : worldCIDs) {
    int32_t sourceEntityID = 0;

    auto client = managerConnection->GetEntityClient(worldCID, true);
    auto zone = client ? client->GetClientState()->GetZone() : nullptr;
    if (client) {
      sourceEntityID = eventManager->InterruptEvent(client);
    }

    auto eState =
        (sourceEntityID && zone) ? zone->GetEntity(sourceEntityID) : nullptr;
    if (eState) {
      switch (eState->GetEntityType()) {
        case EntityType_t::PLASMA:
          // Fail the plasma event
          server->GetZoneManager()->FailPlasma(client, sourceEntityID);
          break;
        case EntityType_t::PVP_BASE:
          // End occupy attempt
          server->GetMatchManager()->LeaveBase(client, sourceEntityID);
          break;
        default:
          // Nothing more needs to be done
          break;
      }
    }
  }
}

bool SkillManager::ToggleSwitchSkill(
    const std::shared_ptr<ChannelClientConnection> client,
    std::shared_ptr<objects::ActivatedAbility> activated,
    const std::shared_ptr<SkillExecutionContext>& ctx) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());

  auto skillData = activated->GetSkillData();
  uint32_t skillID = skillData->GetCommon()->GetID();

  bool toggleOn = false;
  if (source->ActiveSwitchSkillsContains(skillID)) {
    source->RemoveActiveSwitchSkills(skillID);
  } else {
    source->InsertActiveSwitchSkills(skillID);
    toggleOn = true;
  }

  FinalizeSkillExecution(client, ctx, activated);
  FinalizeSkill(ctx, activated);

  if (client) {
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_SWITCH);
    p.WriteS32Little(source->GetEntityID());
    p.WriteU32Little(skillID);
    p.WriteS8(toggleOn ? 1 : 0);

    client->QueuePacket(p);

    mServer.lock()->GetCharacterManager()->RecalculateTokuseiAndStats(source,
                                                                      client);

    client->FlushOutgoing();
  } else {
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    server->GetTokuseiManager()->Recalculate(source, false);
    source->RecalculateStats(definitionManager);
  }

  return true;
}

bool SkillManager::CalculateDamage(
    const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<ProcessingSkill>& pSkill) {
  ProcessingSkill& skill = *pSkill.get();

  auto tokuseiManager = mServer.lock()->GetTokuseiManager();

  auto damageData = skill.Definition->GetDamage()->GetBattleDamage();
  auto formula = damageData->GetFormula();

  bool isHeal =
      formula == objects::MiBattleDamageData::Formula_t::HEAL_NORMAL ||
      formula == objects::MiBattleDamageData::Formula_t::HEAL_STATIC ||
      formula == objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT;
  bool isSimpleDamage =
      formula == objects::MiBattleDamageData::Formula_t::DMG_NORMAL_SIMPLE;

  uint16_t baseMod1 = skill.Modifier1;
  uint16_t baseMod2 = skill.Modifier2;

  float mod1Multiplier = 1.f;
  float mod2Multiplier = 1.f;
  if (formula == objects::MiBattleDamageData::Formula_t::DMG_SOURCE_PERCENT) {
    // Modifiers adjust based upon current remaining HP
    std::vector<std::pair<int32_t, int32_t>> hpMpCurrent;

    auto cs = source->GetCoreStats();
    if (cs) {
      // Use pre-cost values
      hpMpCurrent.push_back(std::pair<int32_t, int32_t>(
          cs->GetHP() + skill.Activated->GetHPCost(), source->GetMaxHP()));
      hpMpCurrent.push_back(std::pair<int32_t, int32_t>(
          cs->GetMP() + skill.Activated->GetMPCost(), source->GetMaxMP()));
    } else {
      hpMpCurrent.push_back(std::pair<int32_t, int32_t>(0, 1));
      hpMpCurrent.push_back(std::pair<int32_t, int32_t>(0, 1));
    }

    for (size_t i = 0; i < 2; i++) {
      auto& pair = hpMpCurrent[i];
      float mod = (float)pair.first / (float)pair.second;

      if (i == 0) {
        mod1Multiplier = mod;
      } else {
        mod2Multiplier = mod;
      }
    }
  }

  if (skill.FunctionID) {
    // Apply source specific FID modifiers
    auto calcState = source->GetCalculatedState();
    if (skill.FunctionID == SVR_CONST.SKILL_STAT_SUM_DAMAGE) {
      // Sum core stats together for modifiers
      auto ct = calcState->GetCorrectTbl();
      int32_t statSum = (int32_t)ct[(size_t)CorrectTbl::STR] +
                        (int32_t)ct[(size_t)CorrectTbl::MAGIC] +
                        (int32_t)ct[(size_t)CorrectTbl::VIT] +
                        (int32_t)ct[(size_t)CorrectTbl::INT] +
                        (int32_t)ct[(size_t)CorrectTbl::SPEED] +
                        (int32_t)ct[(size_t)CorrectTbl::LUCK];

      double levelMod = (double)source->GetLevel() / 100.0;

      int32_t mod =
          (int32_t)(levelMod * (double)statSum * ((double)baseMod1 / 20.0));
      baseMod1 = (uint16_t)(mod > 1000 ? 1000 : mod);

      mod = (int32_t)(levelMod * (double)statSum * ((double)baseMod2 / 20.0));
      baseMod2 = (uint16_t)(mod > 1000 ? 1000 : mod);
    } else if (skill.FunctionID == SVR_CONST.SKILL_HP_DEPENDENT) {
      // Multiplier changes at higher/lower HP
      auto params = skill.Definition->GetSpecial()->GetSpecialParams();

      bool lt = params[0] == 1;
      float split = (float)(lt ? (100 + params[2]) : (params[2])) * 0.01f;

      auto cs = source->GetCoreStats();
      float percentLeft = 0.f;
      if (cs) {
        percentLeft = (float)cs->GetHP() / (float)source->GetMaxHP();
      }

      if ((lt && percentLeft <= split) || (!lt && percentLeft >= split)) {
        float adjust = (float)params[1] * 0.01f;
        mod1Multiplier = mod1Multiplier * adjust;
        mod2Multiplier = mod2Multiplier * adjust;
      }
    } else if (skill.FunctionID == SVR_CONST.SKILL_SUICIDE) {
      // Apply a flat x4 multiplier
      mod1Multiplier = mod1Multiplier * 4.f;
      mod2Multiplier = mod2Multiplier * 4.f;
    }
  }

  bool fidTargetAdjusted =
      skill.FunctionID && (skill.FunctionID == SVR_CONST.SKILL_HP_MP_MIN ||
                           skill.FunctionID == SVR_CONST.SKILL_LNC_DAMAGE);
  for (SkillTargetResult& target : skill.Targets) {
    if (target.HitAvoided) continue;

    uint16_t mod1 = baseMod1;
    uint16_t mod2 = baseMod2;

    auto targetState =
        GetCalculatedState(target.EntityState, pSkill, true, source);
    if (skill.Definition->GetBasic()->GetCombatSkill() && (mod1 || mod2) &&
        GetEntityRate(source, targetState, true) == 0) {
      // Combat skills that deal damage display "impossible" if a 0%
      // entity rate taken exists
      target.Flags2 |= FLAG2_IMPOSSIBLE;
      target.Damage1Type = DAMAGE_TYPE_GENERIC;
      continue;
    }

    float targetModMultiplier = 1.f;
    if (fidTargetAdjusted) {
      // Apply target specific FID modifiers
      if (skill.FunctionID == SVR_CONST.SKILL_HP_MP_MIN) {
        // Immutable reduction to 1 HP/MP
        target.Damage1Type = DAMAGE_EXPLICIT_SET;
        target.Damage2Type = DAMAGE_EXPLICIT_SET;

        auto params = skill.Definition->GetSpecial()->GetSpecialParams();
        if (params[0]) {
          // HP to 1
          target.Damage1 = 1;
        } else {
          // Do not change
          target.Damage1 = -1;
        }

        if (params[1]) {
          // MP to 1
          target.Damage2 = 1;
        } else {
          // Do not change
          target.Damage2 = -1;
        }

        // Nothing left to do
        continue;
      } else if (skill.FunctionID == SVR_CONST.SKILL_LNC_DAMAGE) {
        // Modifier dependent on LNC difference
        size_t diff = (size_t)abs(((int8_t)source->GetLNCType() -
                                   (int8_t)target.EntityState->GetLNCType()) /
                                  2);
        int32_t mod = skill.Definition->GetSpecial()->GetSpecialParams(diff);
        if (mod) {
          targetModMultiplier = 1.f + ((float)mod / 100.f);
        }
      }
    }

    // Apply multipliers
    if (mod1Multiplier != 1.f || targetModMultiplier != 1.f) {
      mod1 =
          (uint16_t)floor((float)mod1 * mod1Multiplier * targetModMultiplier);
    }

    if (mod2Multiplier != 1.f || targetModMultiplier != 1.f) {
      mod2 =
          (uint16_t)floor((float)mod2 * mod2Multiplier * targetModMultiplier);
    }

    // Floor modifiers at 1
    if (!mod1 && skill.Modifier1) {
      mod1 = 1;
    }

    if (!mod2 && skill.Modifier2) {
      mod2 = 1;
    }

    bool effectiveHeal = isHeal || target.HitAbsorb;

    int8_t minDamageLevel = -1;
    if (!effectiveHeal) {
      // If not healing, determine if the calculated critical level will
      // result in minimum damage
      for (double damageMin : tokuseiManager->GetAspectValueList(
               target.EntityState, TokuseiAspectType::DAMAGE_MIN,
               targetState)) {
        if (minDamageLevel < (int8_t)damageMin) {
          minDamageLevel = (int8_t)damageMin;
        }
      }
    }

    uint8_t critLevel = 0;
    bool calcTechPursuit = false;
    bool adjustRate = true;
    bool minAdjust = minDamageLevel > -1;
    switch (formula) {
      case objects::MiBattleDamageData::Formula_t::NONE:
        return true;
      case objects::MiBattleDamageData::Formula_t::DMG_NORMAL:
      case objects::MiBattleDamageData::Formula_t::DMG_NORMAL_SIMPLE:
      case objects::MiBattleDamageData::Formula_t::DMG_COUNTER:
      case objects::MiBattleDamageData::Formula_t::HEAL_NORMAL:
      case objects::MiBattleDamageData::Formula_t::DMG_SOURCE_PERCENT: {
        auto calcState =
            GetCalculatedState(source, pSkill, false, target.EntityState);

        critLevel = !effectiveHeal ? GetCritLevel(source, target, pSkill) : 0;

        CorrectTbl resistCorrectType =
            (CorrectTbl)(skill.EffectiveAffinity + RES_OFFSET);

        float resist =
            (float)(targetState->GetCorrectTbl((size_t)resistCorrectType) *
                    0.01);
        if (target.AutoProtect) {
          // Always resist with min damage
          minDamageLevel = 3;
          resist = 99.9f;
        } else if (target.HitAbsorb) {
          // Resistance is not applied during absorption
          resist = 0;
        }

        // Calculate both damage types
        target.Damage1 = CalculateDamage_Normal(source, target, pSkill, mod1,
                                                target.Damage1Type, resist,
                                                critLevel, isHeal);
        target.Damage2 = CalculateDamage_Normal(source, target, pSkill, mod2,
                                                target.Damage2Type, resist,
                                                critLevel, isHeal);

        // Rates adjusted in calculation as this has special min logic
        adjustRate = false;

        // Always disable min adjust as it will be done here
        minAdjust = false;

        if (minDamageLevel >= (int8_t)critLevel) {
          // If the min damage level is equal to or greater than the
          // critical level, adjust to minimum damage
          target.Damage1 = target.Damage1 ? 1 : 0;
          target.Damage2 = target.Damage2 ? 1 : 0;
        }

        // Set resistence flags, if not healing
        if (!effectiveHeal) {
          if (resist >= 0.5f) {
            target.Flags1 |= FLAG1_PROTECT;
          } else if (resist <= -0.5f) {
            target.Flags1 |= FLAG1_WEAKPOINT;
          }
        }

        calcTechPursuit =
            !effectiveHeal && !isSimpleDamage && target.Damage1 > 0;
      } break;
      case objects::MiBattleDamageData::Formula_t::DMG_STATIC:
      case objects::MiBattleDamageData::Formula_t::HEAL_STATIC:
        target.Damage1 = CalculateDamage_Static(mod1, target.Damage1Type);
        target.Damage2 = CalculateDamage_Static(mod2, target.Damage2Type);
        break;
      case objects::MiBattleDamageData::Formula_t::DMG_PERCENT: {
        auto cs = target.EntityState->GetCoreStats();
        target.Damage1 = CalculateDamage_Percent(mod1, target.Damage1Type,
                                                 cs ? cs->GetHP() : 0);
        target.Damage2 = CalculateDamage_Percent(mod2, target.Damage2Type,
                                                 cs ? cs->GetMP() : 0);
      } break;
      case objects::MiBattleDamageData::Formula_t::DMG_MAX_PERCENT:
      case objects::MiBattleDamageData::Formula_t::HEAL_MAX_PERCENT: {
        target.Damage1 = CalculateDamage_MaxPercent(
            mod1, target.Damage1Type, target.EntityState->GetMaxHP());
        target.Damage2 = CalculateDamage_MaxPercent(
            mod2, target.Damage2Type, target.EntityState->GetMaxMP());
      } break;
      default:
        LogSkillManagerError([&]() {
          return libcomp::String(
                     "Unknown damage formula type encountered: %1\n")
              .Arg((uint8_t)formula);
        });

        return false;
    }

    if (pSkill->AbsoluteDamage) {
      // Hits calculated so adjust any damage parameters to match
      // absolute damage
      if (target.Damage1) {
        target.Damage1 = pSkill->AbsoluteDamage;
      }

      if (target.Damage2) {
        target.Damage2 = pSkill->AbsoluteDamage;
      }
    } else {
      if (adjustRate) {
        // Apply rate adjustments to normal calculation results
        if (target.Damage1 > 0) {
          target.Damage1 =
              AdjustDamageRates(target.Damage1, source, target.EntityState,
                                pSkill, isHeal, false);
        }

        if (target.Damage2 > 0) {
          target.Damage2 =
              AdjustDamageRates(target.Damage2, source, target.EntityState,
                                pSkill, isHeal, false);
        }
      }

      // Apply minimum adjustment for anything that hasn't already
      if (minAdjust) {
        target.Damage1 = target.Damage1 ? 1 : 0;
        target.Damage2 = target.Damage2 ? 1 : 0;
      }

      // Reduce for AOE and make sure at least 1 damage was dealt to each
      // specified type (uses raw, uncapped damage as base)
      float aoeReduction = (float)damageData->GetAoeReduction();
      if (mod1) {
        if (!target.PrimaryTarget && aoeReduction > 0) {
          target.Damage1 = (int32_t)((float)target.Damage1 *
                                     (float)(1.f - (0.01f * aoeReduction)));
        }

        if (target.Damage1 == 0) {
          target.Damage1 = 1;
        }
      }

      if (mod2) {
        if (!target.PrimaryTarget && aoeReduction > 0) {
          target.Damage2 = (int32_t)((float)target.Damage2 *
                                     (float)(1.f - (0.01f * aoeReduction)));
        }

        if (target.Damage2 == 0) {
          target.Damage2 = 1;
        }
      }

      // Apply limits and set crit flags
      if (critLevel == 2) {
        // Apply LB upper limit
        auto calcState =
            GetCalculatedState(source, pSkill, false, target.EntityState);

        int32_t maxLB =
            (int32_t)(30000 + floor(tokuseiManager->GetAspectSum(
                                  source, TokuseiAspectType::LIMIT_BREAK_MAX,
                                  calcState)));

        if (target.Damage1 > maxLB) {
          target.Damage1 = maxLB;
        }

        if (target.Damage2 > maxLB) {
          target.Damage2 = maxLB;
        }

        if (target.Damage1 > 30000 || target.Damage2 > 30000) {
          target.Flags2 |= FLAG2_INTENSIVE_BREAK;
        } else {
          target.Flags2 |= FLAG2_LIMIT_BREAK;
        }
      } else {
        if (target.Damage1 > 9999) {
          target.Damage1 = 9999;
        }

        if (target.Damage2 > 9999) {
          target.Damage2 = 9999;
        }

        if (critLevel == 1) {
          target.Flags1 |= FLAG1_CRITICAL;
        }
      }

      // Determine pursuit/tech damage
      if (calcTechPursuit) {
        auto calcState =
            GetCalculatedState(source, pSkill, false, target.EntityState);

        int32_t pursuitRate = (int32_t)floor(tokuseiManager->GetAspectSum(
            source, TokuseiAspectType::PURSUIT_RATE, calcState));
        int32_t pursuitPow = (int32_t)floor(tokuseiManager->GetAspectSum(
            source, TokuseiAspectType::PURSUIT_POWER, calcState));
        if (pursuitRate > 0 &&
            (pursuitRate >= 100 || RNG(int32_t, 1, 100) <= pursuitRate)) {
          // Take the lowest value applied tokusei affinity override if one
          // exists
          auto affinityOverrides = tokuseiManager->GetAspectValueList(
              source, TokuseiAspectType::PURSUIT_AFFINITY_OVERRIDE);
          if (affinityOverrides.size() > 0) {
            affinityOverrides.sort();
            target.PursuitAffinity = (uint8_t)affinityOverrides.front();

            // If the result is weapon affinity, match it
            if (target.PursuitAffinity == 1) {
              target.PursuitAffinity = skill.WeaponAffinity;
            }
          }

          if (target.PursuitAffinity) {
            // Even if it matches the skill, check NRA for
            // pursuit affinity and stop if it is prevented
            if (!GetNRAResult(target, skill, target.PursuitAffinity)) {
              // Calculate the new enemy resistence and determine damage
              float pResist =
                  (float)(targetState->GetCorrectTbl(
                              (size_t)target.PursuitAffinity + RES_OFFSET) *
                          0.01);

              // Damage is always dealt at this point even with full
              // resistance, floor at 1
              float calc = (float)target.Damage1 * (1.f + pResist * -1.f);
              target.PursuitDamage = (int32_t)floor(calc < 1.f ? 1.f : calc);
            }
          } else {
            // Pursuit damage is a straight adjustment from normal damage
            // when there is no affinity (this is the default state)
            target.PursuitDamage = target.Damage1;
          }

          if (target.PursuitDamage > 0) {
            // Pursuit power floors at 1% if it hits
            if (pursuitPow < 1) {
              pursuitPow = 1;
            }

            // Apply the rate adjustment
            target.PursuitDamage = (int32_t)floor((double)target.PursuitDamage *
                                                  pursuitPow * 0.01);

            // Adjust for 100% limit
            if (target.PursuitDamage > target.Damage1) {
              target.PursuitDamage = target.Damage1;
            } else if (!target.PursuitDamage) {
              target.PursuitDamage = 1;
            }
          }
        }

        int32_t techRate = (int32_t)floor(tokuseiManager->GetAspectSum(
            source, TokuseiAspectType::TECH_ATTACK_RATE, calcState));
        double techPow = floor(tokuseiManager->GetAspectSum(
            source, TokuseiAspectType::TECH_ATTACK_POWER, calcState));
        if (techPow > 0.0 && techRate > 0 &&
            (techRate >= 100 || RNG(int32_t, 1, 100) <= techRate)) {
          // Calculate relative damage
          target.TechnicalDamage =
              (int32_t)floor((double)target.Damage1 * techPow * 0.01);

          // Damage is supposed to hit, floor at 1
          if (!target.TechnicalDamage) {
            target.TechnicalDamage = 1;
          }

          // Apply limits
          if (critLevel == 2) {
            // Cap at LB limit
            int32_t maxLB = (int32_t)(
                30000 +
                floor(tokuseiManager->GetAspectSum(
                    source, TokuseiAspectType::LIMIT_BREAK_MAX, calcState)));

            if (target.TechnicalDamage > maxLB) {
              target.TechnicalDamage = maxLB;
            }
          } else if (target.TechnicalDamage > 9999) {
            target.TechnicalDamage = 9999;
          }
        }
      }
    }

    // If the damage was actually a heal, invert the amount and change the type
    if (effectiveHeal) {
      target.Damage1 = target.Damage1 * -1;
      target.Damage2 = target.Damage2 * -1;
      target.Damage1Type =
          (target.Damage1Type == DAMAGE_TYPE_GENERIC)
              ? (isHeal ? DAMAGE_TYPE_HEALING : DAMAGE_TYPE_DRAIN)
              : target.Damage1Type;
      target.Damage2Type =
          (target.Damage2Type == DAMAGE_TYPE_GENERIC)
              ? (isHeal ? DAMAGE_TYPE_HEALING : DAMAGE_TYPE_DRAIN)
              : target.Damage2Type;
    }
  }

  if (skill.FunctionID == SVR_CONST.SKILL_SUICIDE) {
    auto selfTarget = GetSelfTarget(source, skill.Targets, true);

    selfTarget->Damage1 = source->GetCoreStats()->GetHP();
    selfTarget->Damage1Type = DAMAGE_TYPE_GENERIC;
  }

  return true;
}

uint8_t SkillManager::GetCritLevel(
    const std::shared_ptr<ActiveEntityState>& source, SkillTargetResult& target,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  uint8_t critLevel = 0;
  if (target.GuardModifier > 0) {
    // Cannot receive crit or LB while guarding
    return critLevel;
  }

  auto calcState =
      GetCalculatedState(source, pSkill, false, target.EntityState);
  auto targetState =
      GetCalculatedState(target.EntityState, pSkill, true, source);

  int16_t sourceLuck = source->GetCorrectValue(CorrectTbl::LUCK, calcState);
  int16_t knowledgeCritBoost = (int16_t)(pSkill->KnowledgeRank * 0.5f);
  int16_t critValue = source->GetCorrectValue(CorrectTbl::CRITICAL, calcState);
  critValue = (int16_t)(critValue + sourceLuck + knowledgeCritBoost);

  int16_t critFinal =
      source->GetCorrectValue(CorrectTbl::FINAL_CRIT_CHANCE, calcState);
  int16_t lbChance = source->GetCorrectValue(CorrectTbl::LB_CHANCE, calcState);

  float critRate = 0.f;
  if (critValue > 0) {
    int16_t critDef1 = targetState->GetCorrectTbl((size_t)CorrectTbl::CRIT_DEF);
    if (sourceLuck < 50) {
      critDef1 = (int16_t)(
          critDef1 + targetState->GetCorrectTbl((size_t)CorrectTbl::LUCK));
    } else if (sourceLuck < 67) {
      critDef1 = (int16_t)(critDef1 + 50);
    } else {
      critDef1 = (int16_t)(
          (float)critDef1 +
          floor((float)targetState->GetCorrectTbl((size_t)CorrectTbl::LUCK) *
                0.75f));
    }

    int16_t critDef2 = (int16_t)(10 + floor((float)targetState->GetCorrectTbl(
                                                (size_t)CorrectTbl::CRIT_DEF) *
                                            0.1f));

    critRate = (float)((floor((float)critValue * 0.2f) *
                        (1.f + ((float)critValue * 0.01f)) /
                        (float)(std::max((critDef1 * critDef2), 1))) *
                           100.f +
                       (float)critFinal);
  } else {
    critRate = (float)critFinal;
  }

  if (critRate > 0.f &&
      (critRate >= 100.f ||
       RNG(int16_t, 1, 10000) <= (int16_t)(critRate * 100.f))) {
    critLevel = 1;

    if (lbChance > 0 && RNG(int16_t, 1, 100) <= lbChance) {
      critLevel = 2;
    }
  }

  return critLevel;
}

int16_t SkillManager::GetEntityRate(
    const std::shared_ptr<ActiveEntityState> eState,
    std::shared_ptr<objects::CalculatedEntityState> calcState, bool taken) {
  if (eState->GetEntityType() == EntityType_t::CHARACTER) {
    return calcState->GetCorrectTbl(
        (size_t)(taken ? CorrectTbl::RATE_PC_TAKEN : CorrectTbl::RATE_PC));
  } else {
    return calcState->GetCorrectTbl((size_t)(
        taken ? CorrectTbl::RATE_DEMON_TAKEN : CorrectTbl::RATE_DEMON));
  }
}

float SkillManager::GetAffinityBoost(
    const std::shared_ptr<ActiveEntityState> eState,
    std::shared_ptr<objects::CalculatedEntityState> calcState,
    CorrectTbl boostType) {
  float aBoost = (float)eState->GetCorrectValue(boostType, calcState);
  if (aBoost != 0.f) {
    // Limit boost based on tokusei or 100% by default
    auto tokuseiManager = mServer.lock()->GetTokuseiManager();
    double affinityMax = tokuseiManager->GetAspectSum(
        eState, TokuseiAspectType::AFFINITY_CAP_MAX, calcState);
    if ((double)(aBoost - 100.f) > affinityMax) {
      aBoost = (float)(100.0 + affinityMax);
    }
  }

  return aBoost;
}

int32_t SkillManager::CalculateDamage_Normal(
    const std::shared_ptr<ActiveEntityState>& source, SkillTargetResult& target,
    const std::shared_ptr<ProcessingSkill>& pSkill, uint16_t mod,
    uint8_t& damageType, float resist, uint8_t critLevel, bool isHeal) {
  int32_t amount = 0;

  if (mod != 0) {
    ProcessingSkill& skill = *pSkill.get();
    auto damageData = skill.Definition->GetDamage()->GetBattleDamage();

    auto calcState =
        GetCalculatedState(source, pSkill, false, target.EntityState);
    auto targetState =
        GetCalculatedState(target.EntityState, pSkill, true, source);

    // Determine boost(s)
    std::set<CorrectTbl> boostTypes;
    boostTypes.insert((CorrectTbl)(skill.EffectiveAffinity + BOOST_OFFSET));
    if (skill.BaseAffinity == 1) {
      // Include weapon boost too
      boostTypes.insert(CorrectTbl::BOOST_WEAPON);
    }

    // Get the offense value and boost
    uint16_t off = 0;
    float boost = 0.f;
    if (skill.FusionDemons.size() > 0) {
      // Combine offense value and boost from fusion demons
      int32_t combinedVal = 0;
      for (auto dState : skill.FusionDemons) {
        auto dCalcState =
            GetCalculatedState(dState, pSkill, false, target.EntityState);

        combinedVal +=
            CalculateOffenseValue(dState, target.EntityState, pSkill);

        for (auto boostType : boostTypes) {
          boost += GetAffinityBoost(dState, dCalcState, boostType) * 0.01f;
        }
      }

      if (combinedVal > (int32_t)std::numeric_limits<uint16_t>::max()) {
        // Prevent overflow
        off = std::numeric_limits<uint16_t>::max();
      } else {
        off = (uint16_t)combinedVal;
      }
    } else {
      // Offense value and boost come from normal source
      off = CalculateOffenseValue(source, target.EntityState, pSkill);

      for (auto boostType : boostTypes) {
        boost += GetAffinityBoost(source, calcState, boostType) * 0.01f;
      }
    }

    // -100% boost is the minimum amount allowed
    if (boost < -1.f) {
      boost = -1.f;
    }

    uint16_t def = 0;
    switch (skill.EffectiveDependencyType) {
      case SkillDependencyType_t::CLSR:
      case SkillDependencyType_t::CLSR_LNGR_SPELL:
      case SkillDependencyType_t::CLSR_SPELL:
        def = (uint16_t)targetState->GetCorrectTbl((size_t)CorrectTbl::PDEF);
        break;
      case SkillDependencyType_t::LNGR:
      case SkillDependencyType_t::LNGR_CLSR_SPELL:
      case SkillDependencyType_t::LNGR_SPELL:
        def = (uint16_t)targetState->GetCorrectTbl((size_t)CorrectTbl::PDEF);
        break;
      case SkillDependencyType_t::SPELL:
      case SkillDependencyType_t::SPELL_CLSR:
      case SkillDependencyType_t::SPELL_CLSR_LNGR:
      case SkillDependencyType_t::SPELL_LNGR:
        def = (uint16_t)targetState->GetCorrectTbl((size_t)CorrectTbl::MDEF);
        break;
      case SkillDependencyType_t::SUPPORT:
        def = (uint16_t)targetState->GetCorrectTbl((size_t)CorrectTbl::MDEF);
        break;
      case SkillDependencyType_t::NONE:
      default:
        break;
    }

    // Do not defend against non-combat skills
    if (!skill.Definition->GetBasic()->GetCombatSkill()) {
      def = 0;
    }

    def = (uint16_t)(def + target.GuardModifier);

    float scale = 0.f;
    switch (critLevel) {
      case 1:  // Critical hit
        scale = 1.2f;
        break;
      case 2:  // Limit Break
        scale =
            1.5f *
            (float)source->GetCorrectValue(CorrectTbl::LB_DAMAGE, calcState) *
            0.01f;
        break;
      default:  // Normal hit, 80%-99% damage
        scale = RNG_DEC(float, 0.8f, 0.99f, 2);
        break;
    }

    // Damage starts with offense stat * modifier/100
    float calc = (float)off * ((float)mod * 0.01f);

    // Add the expertise modifier
    calc = calc + (float)skill.ExpertiseRankBoost * 0.5f;

    // Subtract the enemy defense, unless its a critical or limit break
    if (critLevel > 0) {
      const static float reduction =
          mServer.lock()->GetWorldSharedConfig()->GetCritDefenseReduction();
      if (reduction != 1.f) {
        // Non-full reduction on crit
        calc = calc - (float)def * (1.f - reduction);
      }
    } else {
      calc = calc - (float)def;
    }

    if (calc > 0.f) {
      // Scale the current value by the critical, limit break or min to
      // max damage factor
      calc = calc * scale;

      // Multiply by 100% + -resistance
      calc = calc * (1.f + resist * -1.f);

      // Multiply by 100% + boost
      calc = calc * (1.f + boost);

      // Floor and adjust rates
      amount = AdjustDamageRates((int32_t)floor(calc), source,
                                 target.EntityState, pSkill, isHeal, true);
    }

    if (amount < 1) {
      // Apply minimum value of 1
      amount = 1;
    }

    damageType = DAMAGE_TYPE_GENERIC;
  }

  return amount;
}

int32_t SkillManager::CalculateDamage_Static(uint16_t mod,
                                             uint8_t& damageType) {
  int32_t amount = 0;

  if (mod != 0) {
    amount = (int32_t)mod;
    damageType = DAMAGE_TYPE_GENERIC;
  }

  return amount;
}

int32_t SkillManager::CalculateDamage_Percent(uint16_t mod, uint8_t& damageType,
                                              int32_t current) {
  int32_t amount = 0;

  if (mod != 0) {
    amount = (int32_t)ceil((float)current * ((float)mod * 0.01f));
    damageType = DAMAGE_TYPE_GENERIC;
  }

  return amount;
}

int32_t SkillManager::CalculateDamage_MaxPercent(uint16_t mod,
                                                 uint8_t& damageType,
                                                 int32_t max) {
  int32_t amount = 0;

  if (mod != 0) {
    amount = (int32_t)ceil((float)max * ((float)mod * 0.01f));
    damageType = DAMAGE_TYPE_GENERIC;
  }

  return amount;
}

int32_t SkillManager::AdjustDamageRates(
    int32_t damage, const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<ActiveEntityState>& target,
    const std::shared_ptr<channel::ProcessingSkill>& pSkill, bool isHeal,
    bool adjustPower) {
  auto calcState = GetCalculatedState(source, pSkill, false, target);
  auto targetState = GetCalculatedState(target, pSkill, true, source);

  auto tokuseiManager = mServer.lock()->GetTokuseiManager();

  // Get source rate boost and target rate defense boost
  int32_t dependencyDealt = 100;
  int32_t dependencyTaken = 100;
  if (isHeal) {
    // Heal uses its own rates when it applies
    dependencyDealt =
        (int32_t)calcState->GetCorrectTbl((size_t)CorrectTbl::RATE_HEAL);

    dependencyTaken = (int32_t)targetState->GetCorrectTbl(
        (size_t)CorrectTbl::RATE_HEAL_TAKEN);
  } else {
    uint8_t rateBoostIdx = 0;
    switch (pSkill->EffectiveDependencyType) {
      case SkillDependencyType_t::CLSR:
      case SkillDependencyType_t::CLSR_LNGR_SPELL:
      case SkillDependencyType_t::CLSR_SPELL:
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_CLSR;
        break;
      case SkillDependencyType_t::LNGR:
      case SkillDependencyType_t::LNGR_CLSR_SPELL:
      case SkillDependencyType_t::LNGR_SPELL:
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_LNGR;
        break;
      case SkillDependencyType_t::SPELL:
      case SkillDependencyType_t::SPELL_CLSR:
      case SkillDependencyType_t::SPELL_CLSR_LNGR:
      case SkillDependencyType_t::SPELL_LNGR:
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_SPELL;
        break;
      case SkillDependencyType_t::SUPPORT:
        rateBoostIdx = (uint8_t)CorrectTbl::RATE_SUPPORT;
        break;
      case SkillDependencyType_t::NONE:
      default:
        break;
    }

    if (rateBoostIdx != 0) {
      if (source != target) {
        dependencyDealt =
            (int32_t)calcState->GetCorrectTbl((size_t)rateBoostIdx);
      }

      // Apply offset to get defensive value
      dependencyTaken = (int32_t)targetState->GetCorrectTbl(
          (size_t)(rateBoostIdx + ((uint8_t)CorrectTbl::RATE_CLSR_TAKEN -
                                   (uint8_t)CorrectTbl::RATE_CLSR)));
    }
  }

  // Adjust dependency limits
  if (dependencyDealt < 0) {
    dependencyDealt = 0;
  }

  if (dependencyTaken < 0) {
    dependencyTaken = 0;
  }

  // Get tokusei adjustments
  double tokuseiBoost =
      adjustPower ? (tokuseiManager->GetAspectSum(
                         source, TokuseiAspectType::EFFECT_POWER, calcState) *
                     0.01)
                  : 0.0;
  double tokuseiReduction = 0.0;
  if (!isHeal) {
    // Only apply damage adjustments if not healing
    if (source != target) {
      tokuseiBoost += tokuseiManager->GetAspectSum(
                          source, TokuseiAspectType::DAMAGE_DEALT, calcState) *
                      0.01;
    }

    tokuseiReduction -=
        tokuseiManager->GetAspectSum(target, TokuseiAspectType::DAMAGE_TAKEN,
                                     targetState) *
        0.01;

    if (tokuseiBoost < 0.0) {
      tokuseiBoost = 0.0;
    }

    if (tokuseiReduction < 0.0) {
      tokuseiReduction = 0.0;
    }
  }

  float calc = (float)damage;
  std::list<float> rateTaken;

  // If the source is not hitting itself, apply entity rates
  if (source != target) {
    // Multiply by entity rate dealt
    calc = calc * (float)(GetEntityRate(target, calcState, false) * 0.01);

    // Multiply by entity rate taken
    rateTaken.push_back(
        (float)(GetEntityRate(source, targetState, true) * 0.01));
  }

  // Multiply by dependency rate dealt even to source if it is a heal
  if (isHeal || source != target) {
    calc = calc * (float)(dependencyDealt * 0.01);
  }

  if (tokuseiBoost != 0.0) {
    // Multiply by 1 + remaining power boosts/100
    calc = calc * (float)(1.0 + tokuseiBoost);
  }

  // Multiply by dependency rate taken
  rateTaken.push_back((float)(dependencyTaken * 0.01));

  // Multiply by 100% + -general rate taken
  rateTaken.push_back((float)(1.0 + tokuseiReduction));

  for (float taken : rateTaken) {
    // Apply rate taken if not piercing or rate is not a reduction
    if (!pSkill->FunctionID || pSkill->FunctionID != SVR_CONST.SKILL_PIERCE ||
        taken > 1.f) {
      calc = calc * taken;
    }
  }

  // Apply floor
  if (calc < 0.f) {
    calc = 0.f;
  }

  return (int32_t)floor(calc);
}

SkillTargetResult* SkillManager::GetSelfTarget(
    const std::shared_ptr<ActiveEntityState>& source,
    std::list<SkillTargetResult>& targets, bool indirectDefault,
    bool autoCreate) {
  for (SkillTargetResult& target : targets) {
    if (target.EntityState == source) {
      return &target;
    }
  }

  if (autoCreate) {
    SkillTargetResult target;
    target.EntityState = source;
    target.IndirectTarget = indirectDefault;
    targets.push_back(target);
    return &targets.back();
  } else {
    return nullptr;
  }
}

bool SkillManager::SetNRA(SkillTargetResult& target, ProcessingSkill& skill,
                          bool reduceShields) {
  uint8_t resultIdx = GetNRAResult(target, skill, skill.EffectiveAffinity,
                                   target.NRAAffinity, false, reduceShields);
  if (resultIdx && skill.InPvP) {
    target.AutoProtect = true;
    return false;
  }

  switch (resultIdx) {
    case NRA_NULL: {
      if (target.CalcState->ExistingTokuseiAspectsContains(
              (int8_t)TokuseiAspectType::BARRIER)) {
        target.HitNull = 3;  // Barrier
      } else {
        switch (skill.EffectiveDependencyType) {
          case SkillDependencyType_t::CLSR:
          case SkillDependencyType_t::CLSR_LNGR_SPELL:
          case SkillDependencyType_t::CLSR_SPELL:
          case SkillDependencyType_t::LNGR:
          case SkillDependencyType_t::LNGR_CLSR_SPELL:
          case SkillDependencyType_t::LNGR_SPELL:
            target.HitNull = 1;  // Physical null
            break;
          default:
            target.HitNull = 2;  // Magic null
            break;
        }
      }

      target.HitAvoided = true;

      LogSkillManagerDebug([target, skill]() {
        return libcomp::String("%1 nullifies skill %2[%3].\n")
            .Arg(target.EntityState->GetEntityLabel())
            .Arg(skill.SkillID)
            .Arg(skill.Activated->GetActivationID());
      });
    }
      return false;
    case NRA_REFLECT:
      switch (skill.EffectiveDependencyType) {
        case SkillDependencyType_t::CLSR:
        case SkillDependencyType_t::CLSR_LNGR_SPELL:
        case SkillDependencyType_t::CLSR_SPELL:
        case SkillDependencyType_t::LNGR:
        case SkillDependencyType_t::LNGR_CLSR_SPELL:
        case SkillDependencyType_t::LNGR_SPELL:
          target.HitReflect = 1;  // Physical reflect
          break;
        default:
          target.HitReflect = 2;  // Magic reflect
          break;
      }
      target.HitAvoided = true;

      LogSkillManagerDebug([target, skill]() {
        return libcomp::String("%1 reflects skill %2[%3].\n")
            .Arg(target.EntityState->GetEntityLabel())
            .Arg(skill.SkillID)
            .Arg(skill.Activated->GetActivationID());
      });
      return true;
    case NRA_ABSORB:
      target.HitAbsorb = true;

      LogSkillManagerDebug([target, skill]() {
        return libcomp::String("%1 absorbs skill %2[%3].\n")
            .Arg(target.EntityState->GetEntityLabel())
            .Arg(skill.SkillID)
            .Arg(skill.Activated->GetActivationID());
      });
      return false;
    default:
      return false;
  }
}

uint8_t SkillManager::GetNRAResult(SkillTargetResult& target,
                                   ProcessingSkill& skill,
                                   uint8_t effectiveAffinity) {
  return GetNRAResult(target, skill, effectiveAffinity, target.NRAAffinity,
                      true, true);
}

uint8_t SkillManager::GetNRAResult(SkillTargetResult& target,
                                   ProcessingSkill& skill,
                                   uint8_t effectiveAffinity,
                                   uint8_t& resultAffinity, bool effectiveOnly,
                                   bool reduceShields) {
  resultAffinity = 0;
  if (!skill.CanNRA) {
    return 0;
  }

  std::list<CorrectTbl> affinities;
  if (!effectiveOnly) {
    // Calculate affinity checks for physical vs magic and both base and
    // effective values if they differ
    if (effectiveAffinity != 11) {
      // Gather based on dependency type and base affinity if not almighty
      switch (skill.EffectiveDependencyType) {
        case SkillDependencyType_t::CLSR:
        case SkillDependencyType_t::CLSR_LNGR_SPELL:
        case SkillDependencyType_t::CLSR_SPELL:
        case SkillDependencyType_t::LNGR:
        case SkillDependencyType_t::LNGR_CLSR_SPELL:
        case SkillDependencyType_t::LNGR_SPELL:
          affinities.push_back(CorrectTbl::NRA_PHYS);
          break;
        case SkillDependencyType_t::SPELL:
        case SkillDependencyType_t::SPELL_CLSR:
        case SkillDependencyType_t::SPELL_CLSR_LNGR:
        case SkillDependencyType_t::SPELL_LNGR:
        case SkillDependencyType_t::SUPPORT:
          affinities.push_back(CorrectTbl::NRA_MAGIC);
          break;
        case SkillDependencyType_t::NONE:
        default:
          break;
      }

      if (skill.BaseAffinity != effectiveAffinity) {
        affinities.push_back((CorrectTbl)(skill.BaseAffinity + NRA_OFFSET));
      }
    }
  }

  affinities.push_back((CorrectTbl)(effectiveAffinity + NRA_OFFSET));

  // Check NRA chances (absorb in affinity order, reflect in affinity
  // order, then null in affinity order)
  for (auto nraIdx : {NRA_ABSORB, NRA_REFLECT, NRA_NULL}) {
    for (CorrectTbl affinity : affinities) {
      // Consume shields first
      if (target.EntityState->GetNRAShield((uint8_t)nraIdx, affinity,
                                           reduceShields)) {
        resultAffinity = (uint8_t)((uint8_t)affinity - NRA_OFFSET);
        return (uint8_t)nraIdx;
      }

      // If no shield exists, check natural chances
      int16_t chance = target.EntityState->GetNRAChance(
          (uint8_t)nraIdx, affinity, target.CalcState);
      if (chance >= 100 || (chance > 0 && RNG(int16_t, 1, 100) <= chance)) {
        resultAffinity = (uint8_t)((uint8_t)affinity - NRA_OFFSET);
        return (uint8_t)nraIdx;
      }
    }
  }

  return 0;
}

int8_t SkillManager::CalculateStatusEffectStack(int8_t minStack,
                                                int8_t maxStack) const {
  // Sanity check
  if (minStack > maxStack) {
    return 0;
  }

  return minStack == maxStack
             ? maxStack
             : (int8_t)RNG(int16_t, (int16_t)minStack, (int16_t)maxStack);
}

std::unordered_map<uint8_t, std::list<std::shared_ptr<objects::ItemDrop>>>
SkillManager::GetItemDrops(
    const std::shared_ptr<ActiveEntityState>& source,
    const std::shared_ptr<ActiveEntityState>& eState,
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<Zone>& zone, bool giftMode) {
  std::unordered_map<uint8_t, std::list<std::shared_ptr<objects::ItemDrop>>>
      drops;

  auto eBase = eState ? eState->GetEnemyBase() : nullptr;
  auto spawn = eBase ? eBase->GetSpawnSource() : nullptr;
  if (!spawn) {
    return drops;
  }

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto serverDataManager = server->GetServerDataManager();

  // Add specific spawn drops, then drop sets
  std::list<uint32_t> dropSetIDs;
  std::unordered_map<uint8_t, std::list<std::shared_ptr<objects::ItemDrop>>>
      dropsTemp;
  if (giftMode) {
    for (auto gift : spawn->GetGifts()) {
      dropsTemp[(uint8_t)objects::DropSet::Type_t::NORMAL].push_back(gift);
    }

    for (uint32_t giftSetID : spawn->GetGiftSetIDs()) {
      dropSetIDs.push_back(giftSetID);
    }

    if (spawn->GetInheritDrops()) {
      // Add global gifts
      auto globalDef = serverDataManager->GetZonePartialData(0);
      if (globalDef) {
        for (uint32_t giftSetID : globalDef->GetGiftSetIDs()) {
          dropSetIDs.push_back(giftSetID);
        }
      }

      // Add zone gifts
      for (uint32_t giftSetID : zone->GetDefinition()->GetGiftSetIDs()) {
        dropSetIDs.push_back(giftSetID);
      }
    }
  } else {
    for (auto drop : spawn->GetDrops()) {
      dropsTemp[(uint8_t)objects::DropSet::Type_t::NORMAL].push_back(drop);
    }

    for (uint32_t dropSetID : spawn->GetDropSetIDs()) {
      dropSetIDs.push_back(dropSetID);
    }

    if (spawn->GetInheritDrops()) {
      // Add global drops
      auto globalDef = serverDataManager->GetZonePartialData(0);
      if (globalDef) {
        for (uint32_t dropSetID : globalDef->GetDropSetIDs()) {
          dropSetIDs.push_back(dropSetID);
        }
      }

      // Add zone drops
      for (uint32_t dropSetID : zone->GetDefinition()->GetDropSetIDs()) {
        dropSetIDs.push_back(dropSetID);
      }
    }
  }

  // Get drops from drop sets
  for (auto dropSet :
       characterManager->DetermineDropSets(dropSetIDs, zone, client)) {
    uint8_t type = (uint8_t)dropSet->GetType();
    for (auto drop : dropSet->GetDrops()) {
      dropsTemp[type].push_back(drop);
    }
  }

  // Now apply special drop definitions
  for (auto& pair : dropsTemp) {
    auto type = pair.first;
    for (auto drop : pair.second) {
      switch (drop->GetType()) {
        case objects::ItemDrop::Type_t::LEVEL_MULTIPLY: {
          // Copy the drop and scale stacks
          auto copy = std::make_shared<objects::ItemDrop>(*drop);

          uint16_t min = copy->GetMinStack();
          uint16_t max = copy->GetMaxStack();
          float multiplier = (float)eState->GetLevel() * copy->GetModifier();

          copy->SetMinStack((uint16_t)((float)min * multiplier));
          copy->SetMaxStack((uint16_t)((float)max * multiplier));

          drops[type].push_back(copy);
        } break;
        case objects::ItemDrop::Type_t::RELATIVE_LEVEL_MIN:
          // Only add if the (non-source) relative entity's level is
          // at least the same as the source's level + the modifier
          if (eState != source &&
              (int32_t)eState->GetLevel() >=
                  (int32_t)(source->GetLevel() + drop->GetModifier())) {
            drops[type].push_back(drop);
          }
          break;
        case objects::ItemDrop::Type_t::NORMAL:
          drops[type].push_back(drop);
          break;
      }
    }
  }

  return drops;
}

void SkillManager::ScheduleFreeLoot(uint64_t time,
                                    const std::shared_ptr<Zone>& zone,
                                    const std::list<int32_t>& lootEntityIDs,
                                    const std::set<int32_t>& worldCIDs) {
  auto server = mServer.lock();
  server->ScheduleWork(
      time,
      [](CharacterManager* characterManager, const std::shared_ptr<Zone> pZone,
         const std::list<int32_t> pLootEntityIDs,
         const std::set<int32_t> pWorldCIDs) {
        auto clients = pZone->GetConnectionList();
        for (int32_t lootEntityID : pLootEntityIDs) {
          auto lState = pZone->GetLootBox(lootEntityID);
          if (lState) {
            lState->GetEntity()->SetValidLooterIDs(pWorldCIDs);
            characterManager->SendLootItemData(clients, lState, true);
          }
        }

        ChannelClientConnection::FlushAllOutgoing(clients);
      },
      server->GetCharacterManager(), zone, lootEntityIDs, worldCIDs);
}

void SkillManager::FinalizeSkillExecution(
    const std::shared_ptr<ChannelClientConnection> client,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    std::shared_ptr<objects::ActivatedAbility> activated) {
  if (ctx) {
    if (ctx->Executed) {
      // Already executed
      return;
    }

    ctx->Executed = true;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto zone = source->GetZone();
  auto pSkill = GetProcessingSkill(activated, ctx);
  auto skillData = pSkill->Definition;

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto tokuseiManager = server->GetTokuseiManager();

  // Now pay the costs
  int32_t hpCost = activated->GetHPCost();
  int32_t mpCost = activated->GetMPCost();
  bool hpMpCost = hpCost > 0 || mpCost > 0;
  if (hpMpCost) {
    source->SetHPMP((int32_t)-hpCost, (int32_t)-mpCost, true);
  }

  if (client) {
    auto state = client->GetClientState();
    if (hpMpCost) {
      std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
      displayStateModified.insert(source);
      characterManager->UpdateWorldDisplayState(displayStateModified);

      tokuseiManager->Recalculate(
          source,
          std::set<TokuseiConditionType>{TokuseiConditionType::CURRENT_HP,
                                         TokuseiConditionType::CURRENT_MP});
    }

    auto itemCosts = activated->GetItemCosts();
    uint16_t bulletCost = activated->GetBulletCost();

    int64_t targetItem = activated->GetActivationObjectID();
    if (bulletCost > 0) {
      auto character = state->GetCharacterState()->GetEntity();
      auto bullets = character->GetEquippedItems(
          (size_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BULLETS);
      if (bullets) {
        itemCosts[bullets->GetType()] = (uint32_t)bulletCost;
        targetItem = state->GetObjectID(bullets.GetUUID());
      }
    }

    if (itemCosts.size() > 0) {
      characterManager->AddRemoveItems(client, itemCosts, false, targetItem);
    }

    if (pSkill->FunctionID &&
        pSkill->FunctionID == SVR_CONST.SKILL_DEMON_FUSION) {
      // Lower the fusion gauge
      auto definitionManager = server->GetDefinitionManager();
      auto fusionData = definitionManager->GetDevilFusionData(pSkill->SkillID);
      if (fusionData) {
        int8_t stockCount = fusionData->GetStockCost();
        characterManager->UpdateFusionGauge(
            client, (int32_t)(stockCount * -10000), true);
      }

      // Unhide the demon
      client->GetClientState()->GetDemonState()->SetAIIgnored(false);
    }
  }

  uint64_t now = ChannelServer::GetServerTime();
  if (pSkill->Definition->GetBasic()->GetActionType() ==
      objects::MiSkillBasicData::ActionType_t::RUSH) {
    // Move the source to the rush point and bump the execution and hit
    // times forward
    uint64_t rushExecTime = now;
    uint64_t hitTime = activated->GetHitTime();

    Point sourcePoint(source->GetCurrentX(), source->GetCurrentY());
    pSkill->RushStartTime = now;
    pSkill->RushStartPoint = std::make_shared<Point>(sourcePoint);

    if (pSkill->PrimaryTarget && pSkill->PrimaryTarget != source) {
      // Rush forward to the melee attack distance for 500ms. Use
      // destination coordinates instead of current in case they
      // are also rushing right now.
      Point targetPoint(pSkill->PrimaryTarget->GetDestinationX(),
                        pSkill->PrimaryTarget->GetDestinationY());
      float dist = sourcePoint.GetDistance(targetPoint);

      // Max distance for no rush period is essentially a zero distance
      // skill
      float maxDist = (float)(SKILL_DISTANCE_OFFSET +
                              (pSkill->PrimaryTarget->GetHitboxSize() * 10) +
                              (source->GetHitboxSize() * 10));
      if (dist > maxDist) {
        Point rushStart = server->GetZoneManager()->GetLinearPoint(
            targetPoint.x, targetPoint.y, sourcePoint.x, sourcePoint.y, maxDist,
            false, zone);
        pSkill->RushStartPoint->x = rushStart.x;
        pSkill->RushStartPoint->y = rushStart.y;

        // Originally the rush times recorded fluctated between 0.1s
        // and 0.5s with 0.4~0.5s being the most common. For the sake
        // of skill consistency, use the high end of normal.
        uint64_t offset = 500000ULL;
        hitTime = (uint64_t)(hitTime + offset);
        rushExecTime = (uint64_t)(rushExecTime + offset);
      } else {
        // Synch the times
        hitTime = rushExecTime;
      }
    }

    activated->SetHitTime(hitTime);
    activated->SetExecutionTime(rushExecTime);

    // Lock out so we can't act before the rush starts
    auto dischargeData = pSkill->Definition->GetDischarge();
    uint32_t stiffness = dischargeData->GetStiffness();

    uint64_t lockOutTime = hitTime + (uint64_t)(stiffness * 1000);
    source->SetStatusTimes(STATUS_LOCKOUT, lockOutTime);
  } else {
    activated->SetExecutionTime(now);
  }

  if (skillData->GetBasic()->GetCombatSkill() &&
      activated->GetEntityTargeted() && zone) {
    // Start combat if the target exists
    auto targetEntityID = (int32_t)activated->GetTargetObjectID();
    auto target = zone->GetActiveEntity(targetEntityID);
    if (target) {
      auto kbData = skillData->GetDamage()->GetKnockBack();
      uint8_t kbType = kbData->GetKnockBackType();
      if (activated->GetHitTime() && kbType != 2) {
        // Estimate if the attack will cause knockback to indicate
        // a "hard strike" animation. Does not guarantee knockback.
        float kbRecoverBoost =
            (float)(tokuseiManager->GetAspectSum(
                        target, TokuseiAspectType::KNOCKBACK_RECOVERY,
                        target->GetCalculatedState()) *
                    0.01);
        if (target->RefreshKnockback(activated->GetHitTime(), kbRecoverBoost,
                                     false) -
                kbData->GetModifier() <=
            0.f) {
          pSkill->HardStrike = true;
        }
      }
    }
  }

  SetSkillCompleteState(pSkill, true);

  // Do not ACTUALLY execute when using Rest
  if (pSkill->FunctionID != SVR_CONST.SKILL_REST) {
    SendExecuteSkill(pSkill);
  }

  if (client && source->GetEntityType() == EntityType_t::CHARACTER) {
    auto calcState = GetCalculatedState(source, pSkill, false, nullptr);
    float multiplier =
        (float)(source->GetCorrectValue(CorrectTbl::RATE_EXPERTISE, calcState) *
                0.01);

    float globalExpertiseBonus =
        mServer.lock()->GetWorldSharedConfig()->GetExpertiseBonus();

    multiplier = multiplier * (float)(1.f + globalExpertiseBonus);

    characterManager->UpdateExpertise(
        client, pSkill->SkillID, activated->GetExpertiseBoost(), multiplier);
  }
}

std::shared_ptr<objects::ActivatedAbility> SkillManager::FinalizeSkill(
    const std::shared_ptr<SkillExecutionContext>& ctx,
    std::shared_ptr<objects::ActivatedAbility> activated) {
  if (ctx) {
    if (ctx->Finalized) {
      // Already finalized
      return activated;
    }

    ctx->Finalized = true;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto zone = source->GetZone();
  auto pSkill = GetProcessingSkill(activated, ctx);
  auto skillData = pSkill->Definition;

  if (activated->GetExecuteCount() < activated->GetMaxUseCount()) {
    // More uses, make a copy and reset values on original
    auto copy = std::make_shared<objects::ActivatedAbility>(*activated);

    activated->SetHPCost(0);
    activated->SetMPCost(0);
    activated->SetBulletCost(0);
    activated->ClearItemCosts();

    // Reset times
    activated->SetExecutionTime(0);
    activated->SetExecutionRequestTime(0);
    activated->SetHitTime(0);

    // Proceed with the copy
    activated = copy;

    // Reset the upkeep counter
    source->ResetUpkeep();
  } else if (pSkill->FunctionID != SVR_CONST.SKILL_REST) {
    // Update the execution count and remove and complete it from the
    // entity
    if (source->GetActivatedAbility() == activated) {
      source->SetActivatedAbility(nullptr);
      source->ResetUpkeep();
    }

    SendCompleteSkill(activated, 0);
  }

  return activated;
}

bool SkillManager::SetSkillCompleteState(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill, bool executed) {
  auto activated = pSkill->Activated;
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());

  uint64_t currentTime = activated->GetExecutionRequestTime();
  if (!currentTime) {
    currentTime = ChannelServer::GetServerTime();
  }

  uint8_t execCount = activated->GetExecuteCount();
  if (executed) {
    execCount = (uint8_t)(execCount + 1);
    activated->SetExecuteCount(execCount);
  } else {
    // Skill was cancelled
    activated->SetCancelled(true);
  }

  bool moreUses = SkillHasMoreUses(activated);

  // If the skill was executed, set lockout time and increase
  // the execution count
  if (executed) {
    auto dischargeData = pSkill->Definition->GetDischarge();
    uint32_t stiffness = dischargeData->GetStiffness();

    uint64_t lockOutTime = currentTime + (uint64_t)(stiffness * 1000);
    if (stiffness) {
      if (source->IsMoving()) {
        mServer.lock()->GetZoneManager()->FixCurrentPosition(
            source, lockOutTime, currentTime);
      }

      // Use the longer lockout if one exists (only fix position for
      // normal amount)
      uint64_t lastLockout = source->GetStatusTimes(STATUS_LOCKOUT);
      if (lastLockout > lockOutTime) {
        lockOutTime = lastLockout;
      }

      source->SetStatusTimes(STATUS_LOCKOUT, lockOutTime);
    }

    activated->SetLockOutTime(lockOutTime);
  }

  // Set the cooldown if no remaining uses are available
  uint32_t cdTime = pSkill->Definition->GetCondition()->GetCooldownTime();

  uint64_t cooldownTime = 0;
  if (!moreUses || (execCount > 0 && !executed)) {
    if (cdTime) {
      // Adjust cooldown time if supported by the skill
      if ((pSkill->Definition->GetCast()->GetBasic()->GetAdjustRestrictions() &
           SKILL_FIXED_COOLDOWN) == 0) {
        auto calcState = GetCalculatedState(source, pSkill, false, nullptr);

        cdTime = (uint32_t)ceil(
            (double)cdTime *
            (source->GetCorrectValue(CorrectTbl::COOLDOWN_TIME, calcState) *
             0.01));
      }

      cooldownTime = currentTime + (uint64_t)((double)cdTime * 1000);
    } else {
      cooldownTime = currentTime;
    }
  }

  activated->SetCooldownTime(cooldownTime);

  if (cooldownTime) {
    source->SetSkillCooldowns(pSkill->Definition->GetBasic()->GetCooldownID(),
                              cooldownTime);
  } else {
    source->RemoveSkillCooldowns(
        pSkill->Definition->GetBasic()->GetCooldownID());
  }

  // Auto-cancel skills reset each execution
  if (moreUses && execCount > 0 &&
      pSkill->Definition->GetCast()->GetCancel()->GetAutoCancelTime()) {
    ScheduleAutoCancel(source, activated);
  }

  return !executed || !moreUses;
}

bool SkillManager::SpecialSkill(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  (void)ctx;
  (void)client;

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  if (source->GetSpecialActivations(activated->GetActivationID()) ==
      activated) {
    // Clean up the special activation
    source->RemoveSpecialActivations(activated->GetActivationID());
  }

  activated->SetExecutionTime(ChannelServer::GetServerTime());

  return true;
}

bool SkillManager::Cameo(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  if (!cState->Ready() || !cState->IsAlive()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
    return false;
  }

  // Drop the durability of the equipped ring by 1000 points,
  // fail if we can't
  auto item =
      character
          ->GetEquippedItems(
              (size_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_RING)
          .Get();

  auto transformIter = item ? SVR_CONST.CAMEO_MAP.find(item->GetType())
                            : SVR_CONST.CAMEO_MAP.end();
  if (!item || transformIter == SVR_CONST.CAMEO_MAP.end() ||
      transformIter->second.size() == 0 || item->GetDurability() < 1000) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::ITEM_USE);
    return false;
  }

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();

  uint32_t effectID = 0;
  if (transformIter->second.size() > 1) {
    effectID = libcomp::Randomizer::GetEntry(transformIter->second);
  } else {
    effectID = transformIter->second.front();
  }

  StatusEffectChanges effects;
  effects[effectID] = StatusEffectChange(effectID, 1, true);

  if (ProcessSkillResult(activated, ctx)) {
    cState->AddStatusEffects(effects, server->GetDefinitionManager());
    server->GetTokuseiManager()->Recalculate(
        cState,
        std::set<TokuseiConditionType>{TokuseiConditionType::STATUS_ACTIVE});

    characterManager->UpdateDurability(client, item, -1000);
  } else {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  return true;
}

bool SkillManager::Cloak(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!source->Ready() || !source->IsAlive()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  // Check game time
  auto skillData = activated->GetSkillData();
  auto worldClock = mServer.lock()->GetWorldClockTime();
  int32_t gameTime = (int32_t)((worldClock.Hour * 100) + worldClock.Min);

  auto special = skillData->GetSpecial();
  int32_t after = special->GetSpecialParams(0);
  int32_t before = special->GetSpecialParams(1);

  bool rollover = before < after;
  if ((!rollover && (gameTime < after || gameTime > before)) ||
      (rollover && (gameTime < after && gameTime > before))) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::TIME_RESTRICT);
    return false;
  }

  if (ProcessSkillResult(activated, ctx)) {
    return true;
  } else {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }
}

bool SkillManager::DCM(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto dState = state->GetDemonState();
  auto demon = dState->GetEntity();
  auto demonData = dState->GetDevilData();

  if (!demon || !demonData) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
    return false;
  }

  if (!dState->IsAlive()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
    return false;
  }

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  auto bookData = definitionManager->GetDevilBookData(demon->GetType());
  if (!bookData || (!CharacterManager::HasValuable(
                        character, SVR_CONST.VALUABLE_DEVIL_BOOK_V1) &&
                    !CharacterManager::HasValuable(
                        character, SVR_CONST.VALUABLE_DEVIL_BOOK_V2))) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  if (ProcessSkillResult(activated, ctx)) {
    auto worldData = state->GetAccountWorldData().Get();

    size_t index;
    uint8_t shiftVal;
    CharacterManager::ConvertIDToMaskValues((uint16_t)bookData->GetShiftValue(),
                                            index, shiftVal);

    uint8_t currentVal = worldData->GetDevilBook(index);
    uint8_t newVal = (uint8_t)(currentVal | shiftVal);

    if (newVal != currentVal) {
      worldData->SetDevilBook(index, newVal);

      libcomp::Packet reply;
      reply.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_DEMON_COMPENDIUM_ADD);
      reply.WriteS32Little(0);  // Success
      reply.WriteU32Little(bookData->GetShiftValue());

      client->QueuePacket(reply);

      if (dState->UpdateSharedState(character, definitionManager)) {
        // If this resulted in an update, recalculate tokusei
        server->GetTokuseiManager()->Recalculate(
            cState, true, std::set<int32_t>{dState->GetEntityID()});
      }

      // Always recalculate stats
      characterManager->RecalculateStats(dState, client);

      client->FlushOutgoing();

      server->GetWorldDatabase()->QueueUpdate(worldData,
                                              state->GetAccountUID());
    }

    return true;
  } else {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }
}

bool SkillManager::Despawn(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  auto zone = source->GetZone();
  if (ProcessSkillResult(activated, ctx) && zone) {
    switch (source->GetEntityType()) {
      case EntityType_t::ALLY:
      case EntityType_t::ENEMY:
        zone->MarkDespawn(source->GetEntityID());
        break;
      default:
        break;
    }
  }

  return true;
}

bool SkillManager::Desummon(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  auto zone = source->GetZone();
  if (ProcessSkillResult(activated, ctx) && zone) {
    auto target =
        zone->GetActiveEntity((int32_t)activated->GetTargetObjectID());
    if (target) {
      std::unordered_map<uint32_t, uint32_t> encounterGroups;

      auto eBase = target->GetEnemyBase();
      if (eBase && eBase->GetEncounterID()) {
        encounterGroups[eBase->GetEncounterID()] = eBase->GetSpawnGroupID();
      }

      LogSkillManagerDebug([source, target]() {
        return libcomp::String("%1 desummons %2.\n")
            .Arg(source->GetEntityLabel())
            .Arg(target->GetEntityLabel());
      });

      auto state = client ? client->GetClientState() : nullptr;
      switch (target->GetEntityType()) {
        case EntityType_t::ALLY:
        case EntityType_t::ENEMY:
          // Non-players and GMs can despawn any enemy/ally
          if (!state || state->GetUserLevel() > 0) {
            zone->MarkDespawn(target->GetEntityID());
          }
          break;
        case EntityType_t::PARTNER_DEMON:
          // Desummon's partner demons for valid players
          {
            auto server = mServer.lock();
            auto targetClient = server->GetManagerConnection()->GetEntityClient(
                target->GetEntityID());
            if (targetClient) {
              server->GetCharacterManager()->StoreDemon(targetClient);
            }
          }
          break;
        default:
          break;
      }
    }
  }

  return true;
}

bool SkillManager::Digitalize(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto demonID = activated->GetActivationObjectID();
  auto demon = demonID > 0 ? std::dynamic_pointer_cast<objects::Demon>(
                                 libcomp::PersistentObject::GetObjectByUUID(
                                     state->GetObjectUUID(demonID)))
                           : nullptr;
  if (!demon) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
    return false;
  }

  uint8_t dgAbility = cState->GetDigitalizeAbilityLevel();
  if (dgAbility == 0) {
    // Digitalize not enabled
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  auto demonData = definitionManager->GetDevilData(demon->GetType());
  if (CharacterManager::IsMitamaDemon(demonData) && dgAbility < 2) {
    // Mitama demon not valid
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  // If the demon ID or base ID are enabled, the
  std::set<uint32_t> demonIDs;
  demonIDs.insert(demonData->GetBasic()->GetID());
  demonIDs.insert(demonData->GetUnionData()->GetBaseDemonID());

  uint8_t raceID = (uint8_t)demonData->GetCategory()->GetRace();

  bool valid = false;
  auto levelData = definitionManager->GetGuardianLevelData(raceID);
  auto progress = cState->GetEntity()->GetProgress().Get();
  if (levelData) {
    uint8_t lvl =
        (uint8_t)(progress ? progress->GetDigitalizeLevels(raceID) : 0);
    for (uint8_t i = 1; i <= lvl; i++) {
      for (uint32_t dID : levelData->GetLevels(i)->GetDemonIDs()) {
        if (demonIDs.find(dID) != demonIDs.end()) {
          valid = true;
          break;
        }
      }
    }
  }

  if (!valid) {
    // Not found yet, check special unlocks
    for (uint32_t dID : demonIDs) {
      auto specialData = definitionManager->GetGuardianSpecialData(dID);
      if (specialData) {
        auto reqs = specialData->GetRequirements();
        for (size_t i = 0; i < reqs.size();) {
          uint8_t rID = reqs[i];
          if (rID > 0) {
            uint8_t val = reqs[(size_t)(i + 1)];
            uint8_t lvl =
                (uint8_t)(progress ? progress->GetDigitalizeLevels(rID) : 0);
            if (val <= lvl) {
              valid = true;
              break;
            }

            i += 2;
          }
        }
      }
    }
  }

  if (!valid) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
    return false;
  }

  if (!ProcessSkillResult(activated, ctx) ||
      !server->GetCharacterManager()->DigitalizeStart(client, demon)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  return true;
}

bool SkillManager::DigitalizeBreak(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  (void)client;

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto pSkill = GetProcessingSkill(activated, ctx);

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto managerConnection = server->GetManagerConnection();
  for (auto& target : pSkill->Targets) {
    if (target.EntityState != source && !target.HitAbsorb &&
        !target.HitAvoided &&
        target.EntityState->GetEntityType() == EntityType_t::CHARACTER) {
      auto targetClient =
          managerConnection->GetEntityClient(target.EntityState->GetEntityID());
      characterManager->DigitalizeEnd(targetClient);
    }
  }

  return true;
}

bool SkillManager::DigitalizeCancel(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  if (cState->GetDigitalizeState() && ProcessSkillResult(activated, ctx)) {
    if (!mServer.lock()->GetCharacterManager()->DigitalizeEnd(client)) {
      LogSkillManagerError([&]() {
        return libcomp::String("Digitalize cancellation failed: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });
    }
  } else {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  return true;
}

bool SkillManager::DirectStatus(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  (void)client;

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto pSkill = GetProcessingSkill(activated, ctx);

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  auto skillData = pSkill->Definition;
  uint16_t functionID = pSkill->FunctionID;

  bool limited = functionID == SVR_CONST.SKILL_STATUS_LIMITED;
  StatusEffectChanges effects;

  for (int32_t param : skillData->GetSpecial()->GetSpecialParams()) {
    if (param > 0) {
      uint32_t effectID = (uint32_t)param;

      int8_t stackSize = 1;
      if (!limited) {
        // Add 30% of max stack
        auto statusData = definitionManager->GetStatusData(effectID);
        uint8_t maxStack = statusData->GetBasic()->GetMaxStack();
        stackSize = (int8_t)ceil((float)maxStack / 30.f);
      }

      effects[effectID] = StatusEffectChange(effectID, stackSize, false);
    }
  }

  std::list<std::shared_ptr<ActiveEntityState>> entities;
  if (limited) {
    // Source gains status effects
    entities.push_back(source);
  } else {
    // All living targets gain the status effects
    for (auto& target : pSkill->Targets) {
      if (target.EntityState != source && target.EntityState->IsAlive()) {
        entities.push_back(target.EntityState);
      }
    }
  }

  for (auto entity : entities) {
    entity->AddStatusEffects(effects, definitionManager);
  }

  return true;
}

bool SkillManager::EquipItem(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto itemID = activated->GetActivationObjectID();
  if (itemID <= 0) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  // Skip finalization for now so post equip effects are communicated
  // in packets
  ctx->Finalized = true;

  if (!ProcessSkillResult(activated, ctx)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  mServer.lock()->GetCharacterManager()->EquipItem(client, itemID);

  // Finalize now that it all succeeded
  ctx->Finalized = false;
  FinalizeSkill(ctx, activated);

  return true;
}

bool SkillManager::Estoma(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  (void)client;

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto pSkill = GetProcessingSkill(activated, ctx);

  auto server = mServer.lock();
  auto aiManager = server->GetAIManager();
  auto characterManager = server->GetCharacterManager();

  const static bool chargeIgnore =
      server->GetWorldSharedConfig()->GetAIEstomaChargeIgnore();
  const static auto duration =
      server->GetWorldSharedConfig()->GetAIEstomaDuration();

  uint64_t waitTime =
      (uint64_t)(server->GetServerTime() + ((uint64_t)duration * 1000000ULL));

  for (SkillTargetResult& target : pSkill->Targets) {
    auto eState = target.EntityState;
    auto aiState = eState->GetAIState();
    auto targetSkill = eState->GetActivatedAbility();
    if (aiState && !aiState->GetIgnoreEstoma() &&
        (!chargeIgnore || !targetSkill ||
         targetSkill->GetActivationObjectID() != source->GetEntityID())) {
      eState->SetStatusTimes(STATUS_RESTING, waitTime);

      for (int32_t opponentID : eState->GetOpponentIDs()) {
        auto other = pSkill->CurrentZone->GetActiveEntity(opponentID);
        if (other) {
          characterManager->AddRemoveOpponent(false, eState, other);
        }
      }

      aiManager->UpdateAggro(target.EntityState, -1);

      // Cancel skill if one is active and not currently executing
      if (targetSkill && !targetSkill->GetExecutionRequestTime()) {
        CancelSkill(eState, targetSkill->GetActivationID());
      }
    }
  }

  ctx->ApplyAggro = false;

  return true;
}

bool SkillManager::FamiliarityUp(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto demon = dState->GetEntity();
  auto demonData = dState->GetDevilData();

  if (!demon || !demonData) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
    return false;
  }

  if (!dState->IsAlive()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
    return false;
  }

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();
  auto skillData = activated->GetSkillData();

  // Present is retrieved after updating the familiarity for an update but
  // the skill errors if any present will be given based on the starting
  // familiarity level and there is no inventory space open
  int8_t rarity;
  uint16_t currentVal = demon->GetFamiliarity();
  if (characterManager->GetFamiliarityRank(currentVal) >= 3 &&
      characterManager->GetDemonPresent(demon->GetType(),
                                        demon->GetCoreStats()->GetLevel(),
                                        MAX_FAMILIARITY, rarity) != 0 &&
      characterManager->GetFreeSlots(client).size() == 0) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::INVENTORY_SPACE);
    return false;
  }

  // Skills of this type add a "cooldown status effect". If the player
  // character already has it, do not allow the skill's usage
  auto statusEffects = cState->GetStatusEffects();
  for (auto addStatus : skillData->GetDamage()->GetAddStatuses()) {
    if (statusEffects.find(addStatus->GetStatusID()) != statusEffects.end()) {
      SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
      return false;
    }
  }

  auto fType = server->GetServerDataManager()->GetDemonFamiliarityTypeData(
      demonData->GetFamiliarity()->GetFamiliarityType());
  if (!fType) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  // Process the skill without status effects
  ctx->ApplyStatusEffects = false;
  if (!ProcessSkillResult(activated, ctx)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  // Familiarity is adjusted based on the demon's familiarity type
  // and if it shares the same alignment with the character
  bool sameLNC = cState->GetLNCType() == dState->GetLNCType();

  int32_t fPoints = (int32_t)(sameLNC ? fType->GetBoostSkillLNCMatch()
                                      : fType->GetBoostSkill());
  characterManager->UpdateFamiliarity(client, fPoints, true);

  // Apply the status effects
  StatusEffectChanges effects;
  for (auto addStatus : skillData->GetDamage()->GetAddStatuses()) {
    int8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
                                              addStatus->GetMaxStack());
    if (stack == 0 && !addStatus->GetIsReplace()) continue;

    effects[addStatus->GetStatusID()] = StatusEffectChange(
        addStatus->GetStatusID(), stack, addStatus->GetIsReplace());
  }

  if (effects.size() > 0) {
    cState->AddStatusEffects(effects, definitionManager);
    server->GetTokuseiManager()->Recalculate(
        cState,
        std::set<TokuseiConditionType>{TokuseiConditionType::STATUS_ACTIVE});
  }

  // Re-pull the present type and give it to the character
  if (characterManager->GetFamiliarityRank(demon->GetFamiliarity()) >= 3) {
    uint32_t presentType = characterManager->GetDemonPresent(
        demon->GetType(), demon->GetCoreStats()->GetLevel(),
        demon->GetFamiliarity(), rarity);
    GiveDemonPresent(client, demon->GetType(), presentType, rarity,
                     skillData->GetCommon()->GetID());
  }

  return true;
}

bool SkillManager::FamiliarityUpItem(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto dState = state->GetDemonState();
  auto demon = dState->GetEntity();
  auto demonData = dState->GetDevilData();

  if (!demon || !demonData) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
    return false;
  }

  if (!dState->IsAlive()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
    return false;
  }

  auto skillData = activated->GetSkillData();
  auto special = skillData->GetSpecial();

  int32_t maxFamiliarity = special->GetSpecialParams(0);
  float deltaPercent = (float)special->GetSpecialParams(1);
  int32_t minIncrease = special->GetSpecialParams(2);
  int32_t raceRestrict = special->GetSpecialParams(3);

  if (raceRestrict &&
      (int32_t)demonData->GetCategory()->GetRace() != raceRestrict) {
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::PARTNER_INCOMPATIBLE);
    return false;
  }

  if (!ProcessSkillResult(activated, ctx)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  uint16_t currentVal = demon->GetFamiliarity();
  if (maxFamiliarity > (int32_t)currentVal) {
    int32_t fPoints = 0;
    if (maxFamiliarity && deltaPercent) {
      fPoints = (int32_t)ceill(
          floorl((float)(maxFamiliarity - currentVal) * deltaPercent * 0.01f) -
          1);
      if (fPoints < 0) {
        fPoints = 0;
      }
    }

    if (minIncrease && fPoints < minIncrease) {
      fPoints = minIncrease;
    }

    mServer.lock()->GetCharacterManager()->UpdateFamiliarity(client, fPoints,
                                                             true);
  }

  return true;
}

bool SkillManager::ForgetAllExpertiseSkills(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  if (!ProcessSkillResult(activated, ctx)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  std::set<uint32_t> learnedSkills = character->GetLearnedSkills();

  auto defaultObjs = server->GetDefaultCharacterObjectMap();

  std::set<uint32_t> keepSkills;
  for (auto defaultCharObj : defaultObjs["Character"]) {
    auto defaultChar =
        std::dynamic_pointer_cast<objects::Character>(defaultCharObj);
    for (uint32_t keepSkill : defaultChar->GetLearnedSkills()) {
      keepSkills.insert(keepSkill);
    }
  }

  uint32_t maxExpertise = (uint32_t)(EXPERTISE_COUNT + CHAIN_EXPERTISE_COUNT);
  for (uint32_t i = 0; i < maxExpertise; i++) {
    auto expertData = definitionManager->GetExpertClassData(i);
    if (expertData) {
      for (auto classData : expertData->GetClassData()) {
        for (auto rankData : classData->GetRankData()) {
          for (uint32_t skillID : rankData->GetSkill()) {
            if (skillID && keepSkills.find(skillID) == keepSkills.end()) {
              learnedSkills.erase(skillID);
            }
          }
        }
      }
    }
  }

  character->SetLearnedSkills(learnedSkills);

  cState->RecalcDisabledSkills(definitionManager);
  state->GetDemonState()->UpdateDemonState(definitionManager);
  server->GetCharacterManager()->RecalculateTokuseiAndStats(cState, client);

  server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

  return true;
}

bool SkillManager::Liberama(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  (void)client;

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto pSkill = GetProcessingSkill(activated, ctx);

  auto aiManager = mServer.lock()->GetAIManager();
  for (SkillTargetResult& target : pSkill->Targets) {
    auto aiState = target.EntityState->GetAIState();
    if (aiState) {
      aiManager->UpdateAggro(target.EntityState, source->GetEntityID());
    }
  }

  ctx->ApplyAggro = false;

  return true;
}

bool SkillManager::MinionDespawn(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  auto zone = source->GetZone();
  if (!zone) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  auto skillData = activated->GetSkillData();
  auto params = skillData->GetSpecial()->GetSpecialParams();

  auto zoneDef = zone->GetDefinition();
  if (zoneDef->GetID() != (uint32_t)params[0]) {
    // Restricted to specific zone
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::NOTHING_HAPPENED_HERE);
    return false;
  }

  if (ProcessSkillResult(activated, ctx)) {
    for (auto eState : zone->GetEnemiesAndAllies(true)) {
      auto eBase = eState->GetEnemyBase();
      if (eBase->GetSummonerID() == source->GetEntityID() &&
          eBase->GetSpawnLocationGroupID() == (uint32_t)params[1]) {
        zone->MarkDespawn(eState->GetEntityID());
      }
    }
  }

  return true;
}

bool SkillManager::MinionSpawn(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  auto zone = source->GetZone();
  if (!zone) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  auto skillData = activated->GetSkillData();
  auto params = skillData->GetSpecial()->GetSpecialParams();

  // Pull the spawn location group information from the current zone
  auto zoneDef = zone->GetDefinition();
  if (zoneDef->GetID() != (uint32_t)params[0]) {
    // Restricted to specific zone
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::NOTHING_HAPPENED_HERE);
    return false;
  }

  auto slg = zoneDef->GetSpawnLocationGroups((uint32_t)params[1]);
  if (!slg) {
    LogSkillManagerError([&]() {
      return libcomp::String(
                 "Failed to use MinionSpawn skill from invalid "
                 "SpawnLocationGroup: %1\n")
          .Arg(params[1]);
    });

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  if (ProcessSkillResult(activated, ctx)) {
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    uint32_t sgID = libcomp::Randomizer::GetEntry(slg->GetGroupIDs());

    auto spawnGroup = zoneDef->GetSpawnGroups(sgID);
    if (!spawnGroup) {
      LogSkillManagerError([&]() {
        return libcomp::String(
                   "Invalid spawn group ID for MinionSpawn skill: %1\n")
            .Arg(sgID);
      });

      return false;
    }

    // Pick one spot ID (default to source spot if its one of them)
    uint32_t sourceSpotID = source && source->GetEnemyBase()
                                ? source->GetEnemyBase()->GetSpawnSpotID()
                                : 0;
    uint32_t spotID = sourceSpotID && slg->SpotIDsContains(sourceSpotID)
                          ? sourceSpotID
                          : libcomp::Randomizer::GetEntry(slg->GetSpotIDs());

    // If no spot currently selected, default to the summoner's spot
    // regardless
    if (!spotID) {
      spotID = sourceSpotID;
    }

    std::list<std::shared_ptr<ActiveEntityState>> enemies;
    for (auto& spawnPair : spawnGroup->GetSpawns()) {
      auto spawn = zoneDef->GetSpawns(spawnPair.first);
      if (!spawn) {
        LogSkillManagerError([&]() {
          return libcomp::String("Invalid spawn ID for MinionSpawn skill: %1\n")
              .Arg(spawnPair.first);
        });

        continue;
      }

      for (uint16_t i = 0; i < spawnPair.second; i++) {
        auto enemy = zoneManager->CreateEnemy(
            zone, spawn->GetEnemyType(), spawn->GetID(), spotID, 0.f, 0.f, 0.f);
        if (enemy) {
          auto eBase = enemy->GetEnemyBase();
          eBase->SetSpawnGroupID(sgID);
          eBase->SetSpawnLocationGroupID(slg->GetID());
          eBase->SetSummonerID(source->GetEntityID());
          enemies.push_back(enemy);

          auto sourceBase = source->GetEnemyBase();
          if (sourceBase) {
            sourceBase->InsertMinionIDs(enemy->GetEntityID());
          }
        } else {
          LogSkillManagerError([&]() {
            return libcomp::String(
                       "Failed to create enemy for MinionSpawn skill: %1\n")
                .Arg(spawnPair.first);
          });
        }
      }
    }

    auto defeatActions = spawnGroup->GetDefeatActions();
    zoneManager->AddEnemiesToZone(enemies, zone, true, true, defeatActions);

    if (spawnGroup->SpawnActionsCount() > 0) {
      ActionOptions options;
      options.GroupID = spawnGroup->GetID();

      server->GetActionManager()->PerformActions(
          nullptr, spawnGroup->GetSpawnActions(), 0, zone, options);
    }
  }

  return true;
}

bool SkillManager::Mooch(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto demon = dState->GetEntity();

  if (!demon) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
    return false;
  }

  if (!dState->IsAlive()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
    return false;
  }

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();
  auto skillData = activated->GetSkillData();

  if (characterManager->GetFamiliarityRank(demon->GetFamiliarity()) < 3) {
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::MOOCH_PARTNER_FAMILIARITY);
    return false;
  }

  // Present is retrieved prior to updating the familiarity for a drop
  int8_t rarity;
  uint16_t familiarity = demon->GetFamiliarity();
  uint32_t presentType = characterManager->GetDemonPresent(
      demon->GetType(), demon->GetCoreStats()->GetLevel(), familiarity, rarity);

  // If a present will be given and there are no free slots, error the skill
  if (presentType == 0) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  } else if (characterManager->GetFreeSlots(client).size() == 0) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::INVENTORY_SPACE);
    return false;
  }

  // Skills of this type add a "cooldown status effect". If the player
  // character already has it, do not allow the skill's usage
  auto statusEffects = cState->GetStatusEffects();
  for (auto addStatus : skillData->GetDamage()->GetAddStatuses()) {
    if (statusEffects.find(addStatus->GetStatusID()) != statusEffects.end()) {
      SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
      return false;
    }
  }

  // Process the skill without status effects
  ctx->ApplyStatusEffects = false;
  if (!ProcessSkillResult(activated, ctx)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  server->GetCharacterManager()->UpdateFamiliarity(client, -2000, true);

  // Apply the status effects
  StatusEffectChanges effects;
  for (auto addStatus : skillData->GetDamage()->GetAddStatuses()) {
    int8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
                                              addStatus->GetMaxStack());
    if (stack == 0 && !addStatus->GetIsReplace()) continue;

    effects[addStatus->GetStatusID()] = StatusEffectChange(
        addStatus->GetStatusID(), stack, addStatus->GetIsReplace());
  }

  if (effects.size() > 0) {
    cState->AddStatusEffects(effects, definitionManager);
    server->GetTokuseiManager()->Recalculate(
        cState,
        std::set<TokuseiConditionType>{TokuseiConditionType::STATUS_ACTIVE});
  }

  GiveDemonPresent(client, demon->GetType(), presentType, rarity,
                   skillData->GetCommon()->GetID());

  return true;
}

bool SkillManager::Mount(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto character = cState->GetEntity();

  if (cState != source || !cState->Ready() || !cState->IsAlive()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  // The mount status effects are bound to tokusei with no expiration.
  // If either status effect exists on the character, this is actually
  // a request to end the mount state.

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  auto skillData = activated->GetSkillData();
  uint32_t skillID = skillData->GetCommon()->GetID();

  bool end = false;
  if (cState->StatusEffectActive(SVR_CONST.STATUS_MOUNT) ||
      cState->StatusEffectActive(SVR_CONST.STATUS_MOUNT_SUPER)) {
    // Ending mount
    // Very lax validations here so the player can't get stuck in
    // the mounted state
    end = true;
  } else {
    // Starting mount
    // Check the demon's basic state
    auto demon = dState->GetEntity();
    if (!demon) {
      SendFailure(activated, client,
                  (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
      return false;
    } else if (!dState->IsAlive()) {
      SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_DEAD);
      return false;
    }

    // Make sure mounts are allowed in the zone
    auto zone = cState->GetZone();
    if (!zone || zone->GetDefinition()->GetMountDisabled()) {
      SendFailure(activated, client,
                  (uint8_t)SkillErrorCodes_t::LOCATION_RESTRICT);
      return false;
    }

    // Check action restrictions
    uint64_t now = ChannelServer::GetServerTime();
    cState->ExpireStatusTimes(now);
    dState->ExpireStatusTimes(now);

    cState->RefreshCurrentPosition(now);
    dState->RefreshCurrentPosition(now);

    if (!cState->CanMove(true)) {
      SendFailure(activated, client,
                  (uint8_t)SkillErrorCodes_t::MOUNT_MOVE_RESTRICT);
      return false;
    } else if (!dState->CanMove()) {
      SendFailure(activated, client,
                  (uint8_t)SkillErrorCodes_t::MOUNT_DEMON_CONDITION);
      return false;
    } else if (cState->GetDistance(dState->GetCurrentX(), dState->GetCurrentY(),
                                   true) > 250000.f) {
      // Distance is greater than 500 units (squared)
      SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::MOUNT_TOO_FAR);
      return false;
    }

    // Match the demon to the mount skill and item
    std::set<uint32_t> validDemons;
    for (int32_t demonType : skillData->GetSpecial()->GetSpecialParams()) {
      validDemons.insert((uint32_t)demonType);
    }

    auto ring =
        character
            ->GetEquippedItems(
                (size_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_RING)
            .Get();
    bool ringValid = false;
    if (ring) {
      // Make sure the tokusei adds the skill. These items should not
      // have SI available but check properly just in case
      uint32_t specialEffect = ring->GetSpecialEffect();
      for (int32_t tokuseiID : definitionManager->GetSItemTokusei(
               specialEffect ? specialEffect : ring->GetType())) {
        auto tokusei = definitionManager->GetTokuseiData(tokuseiID);
        for (auto aspect : tokusei->GetAspects()) {
          if (aspect->GetType() == TokuseiAspectType::SKILL_ADD &&
              (uint32_t)aspect->GetValue() == skillID) {
            ringValid = true;
            break;
          }
        }
      }
    }

    if (!ringValid) {
      SendFailure(activated, client,
                  (uint8_t)SkillErrorCodes_t::MOUNT_ITEM_MISSING);
      return false;
    } else if (ring->GetDurability() == 0) {
      SendFailure(activated, client,
                  (uint8_t)SkillErrorCodes_t::MOUNT_ITEM_DURABILITY);
      return false;
    } else if (validDemons.find(demon->GetType()) == validDemons.end()) {
      SendFailure(activated, client,
                  (uint8_t)SkillErrorCodes_t::MOUNT_DEMON_INVALID);
      return false;
    }
  }

  // Mount/unmount is valid

  if (ProcessSkillResult(activated, ctx)) {
    if (end) {
      server->GetCharacterManager()->CancelMount(state);
    } else {
      // Toggle the skill on character and demon
      cState->InsertActiveSwitchSkills(skillID);
      dState->InsertActiveSwitchSkills(skillID);

      // Update the demon's display state and warp it
      dState->SetDisplayState(ActiveDisplayState_t::MOUNT);
      server->GetZoneManager()->Warp(client, dState, cState->GetCurrentX(),
                                     cState->GetCurrentY(),
                                     cState->GetCurrentRotation());

      // Recalc tokusei to apply the effects
      server->GetTokuseiManager()->Recalculate(cState, true);
    }

    return true;
  } else {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }
}

bool SkillManager::RandomItem(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  if (!cState->Ready() || !cState->IsAlive()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto zoneManager = server->GetZoneManager();

  if (characterManager->GetFreeSlots(client).size() == 0) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::INVENTORY_SPACE);
    return false;
  }

  // Get drop set from gift box ID
  auto pSkill = GetProcessingSkill(activated, ctx);
  int32_t giftBoxID = pSkill->Definition->GetSpecial()->GetSpecialParams(0);
  auto dropSet =
      server->GetServerDataManager()->GetGiftDropSetData((uint32_t)giftBoxID);
  if (!dropSet) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::ITEM_USE);
    return false;
  }

  // Get one drop from the set
  auto drops = characterManager->DetermineDrops(dropSet->GetDrops(), 0, true);
  auto drop = libcomp::Randomizer::GetEntry(drops);
  if (!drop) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::ITEM_USE);
    return false;
  }

  // Item valid

  uint16_t count = RNG(uint16_t, drop->GetMinStack(), drop->GetMaxStack());

  // Should only be one
  for (auto& pair : activated->GetItemCosts()) {
    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_RANDOM_BOX);
    notify.WriteS32Little(cState->GetEntityID());
    notify.WriteU32Little(pair.first);
    notify.WriteU32Little(drop->GetItemType());
    notify.WriteU16Little(count);
    notify.WriteS8(0);

    zoneManager->BroadcastPacket(client, notify);
  }

  std::unordered_map<uint32_t, uint32_t> items;
  items[drop->GetItemType()] = count;

  characterManager->AddRemoveItems(client, items, true);

  ProcessSkillResult(activated, ctx);

  return true;
}

bool SkillManager::Randomize(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  // No validation needed for this one

  auto skillData = activated->GetSkillData();

  ProcessSkillResult(activated, ctx);

  auto params = skillData->GetSpecial()->GetSpecialParams();

  libcomp::Packet notify;
  notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_RANDOM_NUMBERS);
  notify.WriteS32Little(source->GetEntityID());

  // Distinction between the two versions seems to be hardcoded
  if (params[0] == 0 && params[1] == 1) {
    // Coin flip
    notify.WriteS8(1);
    notify.WriteU32Little(RNG(uint32_t, 0, 1));
  } else {
    // Dice roll
    notify.WriteS8(0);
    notify.WriteU32Little(
        RNG(uint32_t, (uint32_t)params[0], (uint32_t)params[1]));
  }

  mServer.lock()->GetZoneManager()->BroadcastPacket(client, notify);

  return true;
}

bool SkillManager::Respec(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto cs = character->GetCoreStats().Get();

  std::list<int16_t> statVals;
  statVals.push_back(cs->GetSTR());
  statVals.push_back(cs->GetMAGIC());
  statVals.push_back(cs->GetVIT());
  statVals.push_back(cs->GetINTEL());
  statVals.push_back(cs->GetSPEED());
  statVals.push_back(cs->GetLUCK());

  // Loop through each stat and "de-allocate" them
  int32_t respecPoints = 0;
  for (int16_t stat : statVals) {
    if (stat > 1) {
      int32_t delta = (stat % 10) + 1;
      if (stat < 10) {
        delta -= 2;
      }

      int32_t sum = (int32_t)(floor(stat / 10) + 1) * delta;
      for (int32_t i = (int32_t)floor(stat / 10) - 1; i >= 0; i--) {
        if (i == 0) {
          // Skip the first point
          sum += 8;
        } else {
          sum += (i + 1) * 10;
        }
      }

      respecPoints += sum;
    }
  }

  if (ProcessSkillResult(activated, ctx)) {
    // Reset all stats back to 1 and set the new point value
    cs->SetSTR(1);
    cs->SetMAGIC(1);
    cs->SetVIT(1);
    cs->SetINTEL(1);
    cs->SetSPEED(1);
    cs->SetLUCK(1);

    character->SetPoints(respecPoints + character->GetPoints());

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();

    // Recalculate stored dependent stats
    characterManager->CalculateCharacterBaseStats(cs);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_RESET_SKILL_POINTS);
    p.WriteS32Little(cState->GetEntityID());
    characterManager->GetEntityStatsPacketData(p, cs, cState, 1);
    p.WriteS32Little(respecPoints);

    client->QueuePacket(p);

    characterManager->RecalculateTokuseiAndStats(cState, client);

    client->FlushOutgoing();

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
    dbChanges->Update(character);
    dbChanges->Update(cs);

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);

    return true;
  } else {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }
}

bool SkillManager::Rest(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  (void)ctx;

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  // Do not call SpecialSkill as this needs to persist as a special activation

  auto skillData = activated->GetSkillData();

  source->ExpireStatusTimes(ChannelServer::GetServerTime());
  if (source->StatusTimesKeyExists(STATUS_RESTING)) {
    // Expire the status
    std::set<uint32_t> expire;
    for (auto addStatus : skillData->GetDamage()->GetAddStatuses()) {
      expire.insert(addStatus->GetStatusID());
    }

    source->ExpireStatusEffects(expire);
    source->RemoveStatusTimes(STATUS_RESTING);
  } else {
    // Add the status
    StatusEffectChanges effects;
    for (auto addStatus : skillData->GetDamage()->GetAddStatuses()) {
      int8_t stack = CalculateStatusEffectStack(addStatus->GetMinStack(),
                                                addStatus->GetMaxStack());
      if (stack == 0 && !addStatus->GetIsReplace()) continue;

      effects[addStatus->GetStatusID()] = StatusEffectChange(
          addStatus->GetStatusID(), stack, addStatus->GetIsReplace());
    }

    auto definitionManager = mServer.lock()->GetDefinitionManager();
    source->AddStatusEffects(effects, definitionManager);

    source->SetStatusTimes(STATUS_RESTING, 0);
  }

  mServer.lock()->GetCharacterManager()->RecalculateTokuseiAndStats(source,
                                                                    client);

  // Active toggle skill "Rest" only activates and cancels, it never executes
  return true;
}

bool SkillManager::Spawn(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  auto zone = source->GetZone();
  if (!zone) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  libobjgen::UUID responsibleEntity;
  int32_t managedCountForEntity;

  if (!CheckResponsibility(responsibleEntity, managedCountForEntity, activated,
                           client, zone, source)) {
    return false;
  }

  int32_t managedCountForEntityAfter = managedCountForEntity;

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto serverDataManager = server->GetServerDataManager();

  // Zone independent spawns are restricted to fields and dungeons
  auto zoneData = definitionManager->GetZoneData(zone->GetDefinitionID());
  if (!zoneData || (zoneData->GetBasic()->GetType() !=
                        objects::MiZoneBasicData::Type_t::FIELD &&
                    zoneData->GetBasic()->GetType() !=
                        objects::MiZoneBasicData::Type_t::DUNGEON)) {
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::NOTHING_HAPPENED_HERE);
    return false;
  }

  auto skillData = activated->GetSkillData();
  auto params = skillData->GetSpecial()->GetSpecialParams();

  // Pull the spawn group information from the global partial
  auto globalDef = serverDataManager->GetZonePartialData(0);

  // If it matches a spawn location group, pull a random group from
  // that instead
  uint32_t sgID = (uint32_t)params[0];
  auto slg = globalDef ? globalDef->GetSpawnLocationGroups(sgID) : nullptr;
  if (slg) {
    sgID = libcomp::Randomizer::GetEntry(slg->GetGroupIDs());
  }

  auto spawnGroup = globalDef ? globalDef->GetSpawnGroups(sgID) : nullptr;
  if (!spawnGroup) {
    LogSkillManagerError([&]() {
      return libcomp::String(
                 "Failed to use Spawn skill from invalid global SpawnGroup: "
                 "%1\n")
          .Arg(sgID);
    });

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  if (ProcessSkillResult(activated, ctx)) {
    auto zoneManager = server->GetZoneManager();

    Point center(source->GetCurrentX(), source->GetCurrentY());

    auto spawnLoc = std::make_shared<objects::SpawnLocation>();
    spawnLoc->SetX(center.x - 1000.f);
    spawnLoc->SetY(center.y + 1000.f);
    spawnLoc->SetWidth(2000.f);
    spawnLoc->SetHeight(2000.f);

    std::list<std::shared_ptr<ActiveEntityState>> enemies;
    for (auto& spawnPair : spawnGroup->GetSpawns()) {
      auto spawn = globalDef->GetSpawns(spawnPair.first);
      if (!spawn) {
        LogSkillManagerError([&]() {
          return libcomp::String("Invalid spawn ID for Spawn skill: %1\n")
              .Arg(spawnPair.first);
        });

        continue;
      }

      for (uint16_t i = 0; i < spawnPair.second; i++) {
        auto sp = zoneManager->GetRandomPoint(2000.f, 2000.f);

        sp.x += spawnLoc->GetX();
        sp.y = spawnLoc->GetY() - sp.y;

        // Make sure we don't spawn out of bounds
        sp = zoneManager->GetLinearPoint(center.x, center.y, sp.x, sp.y,
                                         center.GetDistance(sp), false, zone);

        float rot = ZoneManager::GetRandomRotation();
        auto enemy =
            zoneManager->CreateEnemy(zone, spawn->GetEnemyType(), 0, 0, sp.x,
                                     sp.y, rot, responsibleEntity);
        if (enemy) {
          auto eBase = enemy->GetEnemyBase();
          eBase->SetSpawnSource(spawn);
          eBase->SetSpawnLocation(spawnLoc);

          // Unlike zone specific spawns, global spawns do not
          // have their group ID set on them as they should never
          // be considered part of that group for that zone.
          // eBase->SetSpawnGroupID(spawnGroup->GetID());

          managedCountForEntityAfter++;
          enemies.push_back(enemy);
        } else {
          LogSkillManagerError([&]() {
            return libcomp::String(
                       "Failed to create enemy for Spawn skill: %1\n")
                .Arg(spawnPair.first);
          });
        }
      }
    }

    if (client) {
      LogSkillManagerDebug([&]() {
        return libcomp::String(
                   "Global spawn group %1 created by player in zone %2: %3\n")
            .Arg(spawnGroup->GetID())
            .Arg(zone->GetDefinitionID())
            .Arg(client->GetClientState()->GetAccountUID().ToString());
      });
    }

    auto defeatActions = spawnGroup->GetDefeatActions();
    zoneManager->AddEnemiesToZone(enemies, zone, true, true, defeatActions);

    if (spawnGroup->SpawnActionsCount() > 0) {
      ActionOptions options;
      options.GroupID = spawnGroup->GetID();

      server->GetActionManager()->PerformActions(
          nullptr, spawnGroup->GetSpawnActions(), 0, zone, options);
    }
  }

  if ((managedCountForEntityAfter / 10) != (managedCountForEntity / 10)) {
    auto username = client->GetClientState()
                        ->GetAccountLogin()
                        ->GetAccount()
                        ->GetUsername();

    LogSkillManagerWarning([&]() {
      return libcomp::String("Account %1 now has %2 managed spawns.")
          .Arg(username)
          .Arg(managedCountForEntityAfter);
    });
  }

  return true;
}

bool SkillManager::SpawnZone(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  auto zone = source->GetZone();
  if (!zone) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  libobjgen::UUID responsibleEntity;
  int32_t managedCountForEntity;

  if (!CheckResponsibility(responsibleEntity, managedCountForEntity, activated,
                           client, zone, source)) {
    return false;
  }

  int32_t managedCountForEntityAfter = managedCountForEntity;

  auto skillData = activated->GetSkillData();
  auto params = skillData->GetSpecial()->GetSpecialParams();

  // Pull the spawn group information from the current zone
  auto zoneDef = zone->GetDefinition();
  if (zoneDef->GetID() != (uint32_t)params[0]) {
    // Restricted to specific zone
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::NOTHING_HAPPENED_HERE);
    return false;
  } else if (zone->GroupHasSpawned((uint32_t)params[1], false, true)) {
    // Cannot be alive already
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::NOTHING_HAPPENED_NOW);
    return false;
  }

  auto spawnGroup = zoneDef->GetSpawnGroups((uint32_t)params[1]);
  if (!spawnGroup) {
    LogSkillManagerError([&]() {
      return libcomp::String(
                 "Failed to use SpawnZone skill from invalid global "
                 "SpawnGroup: %1\n")
          .Arg(params[1]);
    });

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC);
    return false;
  }

  if (ProcessSkillResult(activated, ctx)) {
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    Point center(source->GetCurrentX(), source->GetCurrentY());

    float hw = (float)(params[2] * 2);

    auto spawnLoc = std::make_shared<objects::SpawnLocation>();
    spawnLoc->SetX(center.x - (float)params[2]);
    spawnLoc->SetY(center.y + (float)params[2]);
    spawnLoc->SetWidth(hw);
    spawnLoc->SetHeight(hw);

    std::list<std::shared_ptr<ActiveEntityState>> enemies;
    for (auto& spawnPair : spawnGroup->GetSpawns()) {
      auto spawn = zoneDef->GetSpawns(spawnPair.first);
      if (!spawn) {
        LogSkillManagerError([&]() {
          return libcomp::String("Invalid spawn ID for SpawnZone skill: %1\n")
              .Arg(spawnPair.first);
        });

        continue;
      }

      for (uint16_t i = 0; i < spawnPair.second; i++) {
        auto sp = zoneManager->GetRandomPoint(hw, hw);

        sp.x += spawnLoc->GetX();
        sp.y = spawnLoc->GetY() - sp.y;

        // Make sure we don't spawn out of bounds
        sp = zoneManager->GetLinearPoint(center.x, center.y, sp.x, sp.y,
                                         center.GetDistance(sp), false, zone);

        float rot = ZoneManager::GetRandomRotation();
        auto enemy = zoneManager->CreateEnemy(zone, spawn->GetEnemyType(),
                                              spawn->GetID(), 0, sp.x, sp.y,
                                              rot, responsibleEntity);
        if (enemy) {
          auto eBase = enemy->GetEnemyBase();
          eBase->SetSpawnSource(spawn);
          eBase->SetSpawnGroupID(spawnGroup->GetID());
          eBase->SetSpawnLocation(spawnLoc);
          managedCountForEntityAfter++;
          enemies.push_back(enemy);
        } else {
          LogSkillManagerError([&]() {
            return libcomp::String(
                       "Failed to create enemy for SpawnZone skill: %1\n")
                .Arg(spawnPair.first);
          });
        }
      }
    }

    if (client) {
      LogSkillManagerDebug([&]() {
        return libcomp::String(
                   "Zone spawn group %1 created by player in zone %2: %3\n")
            .Arg(spawnGroup->GetID())
            .Arg(zone->GetDefinitionID())
            .Arg(client->GetClientState()->GetAccountUID().ToString());
      });
    }

    auto defeatActions = spawnGroup->GetDefeatActions();
    zoneManager->AddEnemiesToZone(enemies, zone, true, true, defeatActions);

    if (spawnGroup->SpawnActionsCount() > 0) {
      ActionOptions options;
      options.GroupID = spawnGroup->GetID();

      server->GetActionManager()->PerformActions(
          nullptr, spawnGroup->GetSpawnActions(), 0, zone, options);
    }
  }

  if ((managedCountForEntityAfter / 10) != (managedCountForEntity / 10)) {
    auto username = client->GetClientState()
                        ->GetAccountLogin()
                        ->GetAccount()
                        ->GetUsername();

    LogSkillManagerWarning([&]() {
      return libcomp::String("Account %1 now has %2 managed spawns.")
          .Arg(username)
          .Arg(managedCountForEntityAfter);
    });
  }

  return true;
}

bool SkillManager::SummonDemon(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto dgState = cState->GetDigitalizeState();
  auto demonID = activated->GetActivationObjectID();
  auto demon = demonID > 0 ? std::dynamic_pointer_cast<objects::Demon>(
                                 libcomp::PersistentObject::GetObjectByUUID(
                                     state->GetObjectUUID(demonID)))
                           : nullptr;
  if (!demon) {
    LogSkillManagerError([&]() {
      return libcomp::String(
                 "Invalid demon specified to summon on account: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::SUMMON_INVALID);
    return false;
  } else if (dgState && dgState->GetDemon().Get() == demon) {
    // Cannot summon the digitalized demon
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::SUMMON_INVALID);
    return false;
  } else if (demon->GetCoreStats()->GetLevel() > cState->GetLevel()) {
    // Allow if special status effects exist
    bool allow = false;
    for (uint32_t effectID : SVR_CONST.STATUS_COMP_TUNING) {
      if (cState->StatusEffectActive(effectID)) {
        allow = true;
        break;
      }
    }

    if (!allow) {
      SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::SUMMON_LEVEL);
      return false;
    }
  }

  if (cState->IsMounted()) {
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::MOUNT_SUMMON_RESTRICT);
    return false;
  }

  ProcessSkillResult(activated, ctx);

  mServer.lock()->GetCharacterManager()->SummonDemon(client, demonID);

  LogSkillManagerDebug([cState, dState]() {
    return libcomp::String("%1 summons %2.\n")
        .Arg(cState->GetEntityLabel())
        .Arg(dState->GetEntityLabel());
  });

  return true;
}

bool SkillManager::StoreDemon(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto demonID = activated->GetActivationObjectID();
  if (demonID <= 0) {
    LogSkillManagerError([&]() {
      return libcomp::String("Invalid demon specified to store: %1\n")
          .Arg(demonID);
    });

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::PARTNER_MISSING);
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  if (cState->IsMounted()) {
    SendFailure(activated, client,
                (uint8_t)SkillErrorCodes_t::MOUNT_SUMMON_RESTRICT);
    return false;
  } else if (state->GetObjectID(dState->GetEntityUUID()) != demonID) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
    return false;
  }

  ProcessSkillResult(activated, ctx);

  LogSkillManagerDebug([cState, dState]() {
    return libcomp::String("%1 stores %2.\n")
        .Arg(cState->GetEntityLabel())
        .Arg(dState->GetEntityLabel());
  });

  mServer.lock()->GetCharacterManager()->StoreDemon(client);

  return true;
}

bool SkillManager::Traesto(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto server = mServer.lock();
  auto zoneManager = server->GetZoneManager();

  auto pSkill = GetProcessingSkill(activated, ctx);
  auto skillData = pSkill->Definition;
  uint16_t functionID = pSkill->FunctionID;

  uint32_t zoneID = 0;
  uint32_t spotID = 0;
  if (functionID == SVR_CONST.SKILL_TRAESTO) {
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    zoneID = character->GetHomepointZone();
    spotID = character->GetHomepointSpotID();
  } else if (functionID == (uint16_t)SVR_CONST.SKILL_TRAESTO_ARCADIA[0]) {
    zoneID = SVR_CONST.SKILL_TRAESTO_ARCADIA[1];
    spotID = SVR_CONST.SKILL_TRAESTO_ARCADIA[2];
  } else if (functionID == (uint16_t)SVR_CONST.SKILL_TRAESTO_DSHINJUKU[0]) {
    zoneID = SVR_CONST.SKILL_TRAESTO_DSHINJUKU[1];
    spotID = SVR_CONST.SKILL_TRAESTO_DSHINJUKU[2];
  } else if (functionID == (uint16_t)SVR_CONST.SKILL_TRAESTO_KAKYOJO[0]) {
    zoneID = SVR_CONST.SKILL_TRAESTO_KAKYOJO[1];
    spotID = SVR_CONST.SKILL_TRAESTO_KAKYOJO[2];
  } else if (functionID ==
             (uint16_t)SVR_CONST.SKILL_TRAESTO_NAKANO_BDOMAIN[0]) {
    zoneID = SVR_CONST.SKILL_TRAESTO_NAKANO_BDOMAIN[1];
    spotID = SVR_CONST.SKILL_TRAESTO_NAKANO_BDOMAIN[2];
  } else if (functionID == (uint16_t)SVR_CONST.SKILL_TRAESTO_SOUHONZAN[0]) {
    zoneID = SVR_CONST.SKILL_TRAESTO_SOUHONZAN[1];
    spotID = SVR_CONST.SKILL_TRAESTO_SOUHONZAN[2];
  }

  if (zoneID == 0 || spotID == 0) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::ZONE_INVALID);
    return false;
  }

  float xCoord = 0.f;
  float yCoord = 0.f;
  float rot = 0.f;

  auto zoneDef = server->GetServerDataManager()->GetZoneData(zoneID, 0);
  uint32_t dynamicMapID = zoneDef ? zoneDef->GetDynamicMapID() : 0;

  if (!zoneDef || !zoneManager->GetSpotPosition(dynamicMapID, spotID, xCoord,
                                                yCoord, rot)) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::ZONE_INVALID);
    return false;
  }

  if (ProcessSkillResult(activated, ctx) &&
      zoneManager->EnterZone(client, zoneID, zoneDef->GetDynamicMapID(), xCoord,
                             yCoord, rot, true)) {
    return true;
  } else {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }
}

bool SkillManager::XPUp(
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<SkillExecutionContext>& ctx,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  SpecialSkill(activated, ctx, client);

  if (!client) {
    SendFailure(activated, nullptr);
    return false;
  }

  auto state = client->GetClientState();

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();

  auto pSkill = GetProcessingSkill(activated, ctx);
  auto skillData = pSkill->Definition;

  std::shared_ptr<ActiveEntityState> eState;
  if (pSkill->FunctionID == SVR_CONST.SKILL_XP_SELF) {
    eState = state->GetCharacterState();
  } else if (pSkill->FunctionID == SVR_CONST.SKILL_XP_PARTNER) {
    eState = state->GetDemonState();
  }

  if (!eState || !eState->Ready()) {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::TARGET_INVALID);
    return false;
  }

  if (ProcessSkillResult(activated, ctx)) {
    characterManager->UpdateExperience(
        client, skillData->GetSpecial()->GetSpecialParams(0),
        eState->GetEntityID());
    return true;
  } else {
    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::GENERIC_USE);
    return false;
  }
}

bool SkillManager::CheckScriptValidation(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill, bool execution) {
  if (!pSkill->FunctionID) {
    // Nothing to do
    return true;
  }

  auto settingsIter = mSkillLogicSettings.find(pSkill->FunctionID);
  if (settingsIter == mSkillLogicSettings.end() ||
      (execution && !settingsIter->second->HasExecutionValidation) ||
      (!execution && !settingsIter->second->HasActivationValidation)) {
    return true;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      pSkill->Activated->GetSourceEntity());
  auto state = source ? ClientState::GetEntityClientState(source->GetEntityID())
                      : nullptr;

  Sqrat::Function f(
      Sqrat::RootTable(mSkillLogicScripts[pSkill->FunctionID]->GetVM()),
      execution ? "validateExecution" : "validateActivation");
  auto result = !f.IsNull()
                    ? f.Evaluate<int32_t>(
                          source, state ? state->GetCharacterState() : nullptr,
                          state ? state->GetDemonState() : nullptr, pSkill,
                          pSkill->CurrentZone)
                    : 0;

  // Print error if validation was not successful (0) and not a normal
  // error (1)
  if (!result || (*result != 0 && *result != 1)) {
    LogSkillManagerError([source, pSkill, execution]() {
      return libcomp::String(
                 "Script validation failed for %1 when %2 skill: %3.\n")
          .Arg(source->GetEntityLabel())
          .Arg(execution ? "executing" : "activating")
          .Arg(pSkill->SkillID);
    });
  }

  return result && *result == 0;
}

bool SkillManager::AdjustScriptCosts(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  if (!pSkill->FunctionID) {
    // Nothing to do
    return true;
  }

  auto settingsIter = mSkillLogicSettings.find(pSkill->FunctionID);
  if (settingsIter == mSkillLogicSettings.end() ||
      !settingsIter->second->HasCostAdjustment) {
    return true;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      pSkill->Activated->GetSourceEntity());
  auto state = source ? ClientState::GetEntityClientState(source->GetEntityID())
                      : nullptr;

  Sqrat::Function f(
      Sqrat::RootTable(mSkillLogicScripts[pSkill->FunctionID]->GetVM()),
      "adjustCost");
  auto result = !f.IsNull()
                    ? f.Evaluate<int32_t>(
                          source, state ? state->GetCharacterState() : nullptr,
                          state ? state->GetDemonState() : nullptr, pSkill,
                          pSkill->CurrentZone)
                    : 0;

  // Print error if cost calculation was not successful (0) and not deemed
  // unpayable (1)
  if (!result || (*result != 0 && *result != 1)) {
    LogSkillManagerError([source, pSkill]() {
      return libcomp::String(
                 "Script cost adjustment failed for %1 when using skill: "
                 "%2.\n")
          .Arg(source->GetEntityLabel())
          .Arg(pSkill->SkillID);
    });
  }

  return result && *result == 0;
}

bool SkillManager::ExecuteScriptPreActions(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill,
    std::list<std::shared_ptr<channel::ActiveEntityState>> targets) {
  if (!pSkill->FunctionID) {
    // Nothing to do
    return true;
  }

  auto settingsIter = mSkillLogicSettings.find(pSkill->FunctionID);
  if (settingsIter == mSkillLogicSettings.end() ||
      !settingsIter->second->HasPreAction) {
    return true;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      pSkill->Activated->GetSourceEntity());
  auto state = source ? ClientState::GetEntityClientState(source->GetEntityID())
                      : nullptr;

  auto vm = mSkillLogicScripts[pSkill->FunctionID]->GetVM();
  Sqrat::Array targetStates(vm);
  for (auto& targetState : targets) {
    targetStates.Append(targetState);
  }

  Sqrat::Function f(Sqrat::RootTable(vm), "preAction");
  auto result = !f.IsNull()
                    ? f.Evaluate<int32_t>(
                          source, state ? state->GetCharacterState() : nullptr,
                          state ? state->GetDemonState() : nullptr, pSkill,
                          pSkill->CurrentZone, targetStates, mServer.lock())
                    : 0;

  // Print error if cost calculation was not successful (0) and not requested
  // to fizzle (1)
  if (!result || (*result != 0 && *result != 1)) {
    LogSkillManagerError([source, pSkill]() {
      return libcomp::String(
                 "Script pre-action failed for %1 when using skill: %2.\n")
          .Arg(source->GetEntityLabel())
          .Arg(pSkill->SkillID);
    });
  }

  return result && *result == 0;
}

bool SkillManager::ExecuteScriptPostActions(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  if (!pSkill->FunctionID) {
    // Nothing to do
    return true;
  }

  auto settingsIter = mSkillLogicSettings.find(pSkill->FunctionID);
  if (settingsIter == mSkillLogicSettings.end() ||
      !settingsIter->second->HasPostAction) {
    return true;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      pSkill->Activated->GetSourceEntity());
  auto state = source ? ClientState::GetEntityClientState(source->GetEntityID())
                      : nullptr;

  auto vm = mSkillLogicScripts[pSkill->FunctionID]->GetVM();
  Sqrat::Array directTargets(vm);
  // std::list<SkillTargetResult*> directTargets;
  for (auto& target : pSkill->Targets) {
    if (!target.IndirectTarget) {
      directTargets.Append(&target);
    }
  }

  Sqrat::Function f(Sqrat::RootTable(vm), "postAction");
  auto result = !f.IsNull()
                    ? f.Evaluate<int32_t>(
                          source, state ? state->GetCharacterState() : nullptr,
                          state ? state->GetDemonState() : nullptr, pSkill,
                          pSkill->CurrentZone, directTargets, mServer.lock())
                    : 0;

  // Print error if post-action was not successful (0)
  if (!result || *result != 0) {
    LogSkillManagerError([source, pSkill]() {
      return libcomp::String(
                 "Script post-action failed for %1 when using skill: %2.\n")
          .Arg(source->GetEntityLabel())
          .Arg(pSkill->SkillID);
    });
  }

  return result && *result == 0;
}

void SkillManager::GiveDemonPresent(
    const std::shared_ptr<ChannelClientConnection>& client, uint32_t demonType,
    uint32_t itemType, int8_t rarity, uint32_t skillID) {
  if (!client || itemType == 0) {
    return;
  }

  auto characterManager = mServer.lock()->GetCharacterManager();

  std::unordered_map<uint32_t, uint32_t> items;
  items[itemType] = 1;

  if (characterManager->AddRemoveItems(client, items, true)) {
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_PRESENT);
    p.WriteU32Little(itemType);
    p.WriteS32Little((int32_t)rarity);
    p.WriteU32Little(skillID);
    p.WriteU32Little(demonType);

    client->SendPacket(p);
  }
}

void SkillManager::Fizzle(const std::shared_ptr<SkillExecutionContext>& ctx) {
  if (ctx) {
    auto managerConnection = mServer.lock()->GetManagerConnection();

    // Make sure all countering skills are completed in full
    for (auto counteringSkill : ctx->CounteringSkills) {
      auto copyCtx = std::make_shared<SkillExecutionContext>(
          *counteringSkill->ExecutionContext);
      auto activated = copyCtx->Skill->Activated;
      auto client = managerConnection->GetEntityClient(
          activated->GetSourceEntity()->GetEntityID());

      FinalizeSkillExecution(client, copyCtx, activated);
      FinalizeSkill(copyCtx, activated);

      counteringSkill->ExecutionContext = nullptr;
    }

    LogSkillManagerDebug([ctx]() {
      return libcomp::String("Skill %1[%2] from %3 fizzles.\n")
          .Arg(ctx->Skill->SkillID)
          .Arg(ctx->Skill->Activated->GetActivationID())
          .Arg(ctx->Skill->EffectiveSource->GetEntityLabel());
    });
  }
}

void SkillManager::SendActivateSkill(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  // Instant executions are not technically activated
  auto activated = pSkill->Activated;
  if (activated->GetActivationID() == -1) {
    return;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto zone = source ? source->GetZone() : nullptr;
  auto zConnections =
      zone ? zone->GetConnectionList()
           : std::list<std::shared_ptr<ChannelClientConnection>>();
  if (zConnections.size() > 0) {
    RelativeTimeMap timeMap;

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_ACTIVATED);
    p.WriteS32Little(source->GetEntityID());
    p.WriteU32Little(pSkill->SkillID);
    p.WriteS8(activated->GetActivationID());

    timeMap[11] = activated->GetChargedTime();
    p.WriteFloat(0.f);

    p.WriteU8(activated->GetMaxUseCount());

    // Set some sort of category flag. Initially I thought this was a
    // notification that the attacker needed to get closer or not but
    // the client handles this just fine.
    switch (pSkill->Definition->GetBasic()->GetActionType()) {
      case objects::MiSkillBasicData::ActionType_t::ATTACK:
      case objects::MiSkillBasicData::ActionType_t::RUSH:
        p.WriteU8(1);
        break;
      case objects::MiSkillBasicData::ActionType_t::GUARD:
        p.WriteU8(0);
        break;
      default:
        p.WriteU8(2);
        break;
    }

    p.WriteFloat(activated->GetChargeMoveSpeed());
    p.WriteFloat(activated->GetChargeCompleteMoveSpeed());

    ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
  }
}

void SkillManager::SendExecuteSkill(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill) {
  // Instant executions use a special packet to execute
  auto activated = pSkill->Activated;
  if (activated->GetActivationID() == -1) {
    SendExecuteSkillInstant(pSkill, 0);
    return;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto zone = source ? source->GetZone() : nullptr;
  auto zConnections =
      zone ? zone->GetConnectionList()
           : std::list<std::shared_ptr<ChannelClientConnection>>();
  if (zConnections.size() > 0) {
    int32_t targetedEntityID = activated->GetEntityTargeted()
                                   ? (int32_t)activated->GetTargetObjectID()
                                   : source->GetEntityID();

    RelativeTimeMap timeMap;

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_EXECUTED);
    p.WriteS32Little(source->GetEntityID());
    p.WriteU32Little(pSkill->SkillID);
    p.WriteS8(activated->GetActivationID());
    p.WriteS32Little(targetedEntityID);

    timeMap[15] = activated->GetCooldownTime();
    p.WriteFloat(0.f);
    timeMap[19] = activated->GetLockOutTime();
    p.WriteFloat(0.f);

    p.WriteU32Little((uint32_t)activated->GetHPCost());
    p.WriteU32Little((uint32_t)activated->GetMPCost());

    // Rush skills have additional execution point values as well as
    // an offset to their normal hit timing
    if (pSkill->RushStartPoint) {
      p.WriteU8(1);  // Rush flag

      // Source will "speed up" to this point if not already there
      p.WriteFloat(pSkill->RushStartPoint->x);
      p.WriteFloat(pSkill->RushStartPoint->y);

      p.WriteFloat(0);  // Always 0

      // Usage timing
      timeMap[44] = pSkill->RushStartTime;
      timeMap[48] = activated->GetExecutionTime();
      p.WriteFloat(0.f);
      p.WriteFloat(0.f);
    } else {
      p.WriteBlank(21);
    }

    p.WriteU8(pSkill->HardStrike ? 1 : 0);
    p.WriteU8(0xFF);  // Always the same value

    ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
  }
}

void SkillManager::SendExecuteSkillInstant(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill,
    uint8_t errorCode) {
  auto activated = pSkill->Activated;
  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto zone = source ? source->GetZone() : nullptr;
  auto zConnections =
      zone ? zone->GetConnectionList()
           : std::list<std::shared_ptr<ChannelClientConnection>>();
  if (zConnections.size() > 0) {
    int32_t targetedEntityID = activated->GetEntityTargeted()
                                   ? (int32_t)activated->GetTargetObjectID()
                                   : source->GetEntityID();

    RelativeTimeMap timeMap;

    libcomp::Packet p;
    p.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_SKILL_EXECUTED_INSTANT);
    p.WriteU8(errorCode);
    p.WriteS32Little(source->GetEntityID());
    p.WriteU32Little(pSkill->SkillID);
    p.WriteS32Little(targetedEntityID);

    uint64_t cooldown = errorCode == 0 ? activated->GetCooldownTime() : 0;
    timeMap[p.Size()] = cooldown;
    p.WriteFloat(0.f);

    p.WriteU32Little((uint32_t)activated->GetHPCost());
    p.WriteU32Little((uint32_t)activated->GetMPCost());

    if (cooldown) {
      // Relative times are only needed if a cooldown is set
      ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
    } else {
      ChannelClientConnection::BroadcastPacket(zConnections, p);
    }
  }
}

void SkillManager::SendCompleteSkill(
    std::shared_ptr<objects::ActivatedAbility> activated, uint8_t mode) {
  // Instant executions are not completed as they are not technically
  // activated
  if (activated->GetActivationID() == -1) {
    return;
  }

  auto source = std::dynamic_pointer_cast<ActiveEntityState>(
      activated->GetSourceEntity());
  auto zone = source ? source->GetZone() : nullptr;
  auto zConnections =
      zone ? zone->GetConnectionList()
           : std::list<std::shared_ptr<ChannelClientConnection>>();
  if (zConnections.size() > 0) {
    RelativeTimeMap timeMap;

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_COMPLETED);
    p.WriteS32Little(source->GetEntityID());
    p.WriteU32Little(activated->GetSkillData()->GetCommon()->GetID());
    p.WriteS8(activated->GetActivationID());

    // Write the cooldown time if cancelling in case its set
    // (mostly for multi-use skills)
    uint64_t cooldown = mode == 1 ? activated->GetCooldownTime() : 0;
    timeMap[p.Size()] = cooldown;
    p.WriteFloat(0.f);

    p.WriteU8(1);  // Unknown, always the same
    p.WriteFloat(source->GetMovementSpeed());
    p.WriteU8(mode);

    if (cooldown) {
      // Relative times are only needed if a cooldown is set
      ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
    } else {
      ChannelClientConnection::BroadcastPacket(zConnections, p);
    }
  }
}

uint32_t SkillManager::GetSummonSpeed(
    const std::shared_ptr<channel::ProcessingSkill>& pSkill,
    const std::shared_ptr<ChannelClientConnection>& client) {
  auto state = client ? client->GetClientState() : nullptr;
  auto cState = state ? state->GetCharacterState() : nullptr;
  if (!state || pSkill->EffectiveSource != cState) {
    return 0;
  }

  auto calcState = GetCalculatedState(cState, pSkill, false, nullptr);
  auto activated = pSkill->Activated;
  auto demonID = activated->GetActivationObjectID();
  auto demon = demonID > 0 ? std::dynamic_pointer_cast<objects::Demon>(
                                 libcomp::PersistentObject::GetObjectByUUID(
                                     state->GetObjectUUID(demonID)))
                           : nullptr;
  if (!demon) {
    return 0;
  }

  auto demonData =
      mServer.lock()->GetDefinitionManager()->GetDevilData(demon->GetType());
  if (!demonData) {
    return 0;
  }

  uint32_t demonSpeed = demonData->GetSummonData()->GetSummonSpeed();
  int16_t correctSpeed =
      cState->GetCorrectValue(CorrectTbl::SUMMON_SPEED, calcState);
  double speed = (((double)demonSpeed - (double)correctSpeed) / 100.0 * 2000.0);

  // Minimum 1ms
  return (uint32_t)(speed > 0.0 ? speed : 1.0);
}

bool SkillManager::IsTalkSkill(
    const std::shared_ptr<objects::MiSkillData>& skillData, bool primaryOnly) {
  switch (skillData->GetBasic()->GetActionType()) {
    case objects::MiSkillBasicData::ActionType_t::TALK:
    case objects::MiSkillBasicData::ActionType_t::INTIMIDATE:
    case objects::MiSkillBasicData::ActionType_t::TAUNT:
      return true;
    default:
      if (!primaryOnly) {
        // If the action type doesn't match but there is talk
        // damage it is still a talk skill
        auto talkDamage = skillData->GetDamage()->GetNegotiationDamage();
        return talkDamage->GetSuccessAffability() != 0 ||
               talkDamage->GetFailureAffability() != 0 ||
               talkDamage->GetSuccessFear() != 0 ||
               talkDamage->GetFailureFear() != 0;
      }
      break;
  }

  return false;
}

bool SkillManager::IFramesEnabled() {
  const static bool enabled =
      mServer.lock()->GetWorldSharedConfig()->GetIFramesEnabled();
  return enabled;
}

bool SkillManager::CheckResponsibility(
    libobjgen::UUID& responsibleEntity, int32_t& managedCountForEntity,
    const std::shared_ptr<objects::ActivatedAbility>& activated,
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<ActiveEntityState>& source) {
  managedCountForEntity = 0;

  bool playerEntity = source->GetEntityType() == EntityType_t::CHARACTER;
  int32_t managedZoneEntityCap = 0;
  int32_t managedEntityCap = 0;

  if (playerEntity) {
    responsibleEntity = client->GetClientState()->GetAccountUID();
  }

  LogSkillManagerDebug([&]() {
    return libcomp::String("Responsible entity: %1\n")
        .Arg(responsibleEntity.ToString());
  });

  auto server = mServer.lock();
  auto worldSharedConfig = server->GetWorldSharedConfig();

  if (playerEntity) {
    LogSkillManagerDebug([&]() {
      return libcomp::String("Player user level: %1\n")
          .Arg(client->GetClientState()->GetUserLevel());
    });

    if (worldSharedConfig->GetSpawnSpamUserLevel() <
        client->GetClientState()->GetUserLevel()) {
      responsibleEntity = {};
    } else {
      if (zone->GetInstance()) {
        managedZoneEntityCap = worldSharedConfig->GetSpawnSpamInstanceZoneMax();
      } else {
        managedZoneEntityCap = worldSharedConfig->GetSpawnSpamGlobalZoneMax();
      }

      managedEntityCap = worldSharedConfig->GetSpawnSpamUserMax();
    }
  }

  if (managedZoneEntityCap &&
      zone->GetManagedEntities() >= managedZoneEntityCap) {
    LogSkillManagerError([&]() {
      auto username = client->GetClientState()
                          ->GetAccountLogin()
                          ->GetAccount()
                          ->GetUsername();

      return libcomp::String(
                 "Account %1 tried to spawn more enemies but there is already "
                 "%2 in zone %3 with a cap of %4.\n")
          .Arg(username)
          .Arg(zone->GetManagedEntities())
          .Arg(zone->GetDefinitionID())
          .Arg(managedZoneEntityCap);
    });

    server->GetChatManager()->SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "Zone has reached the maximum number of player spawns.");

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::SILENT_FAIL);
    return false;
  }

  if (managedEntityCap) {
    managedCountForEntity = zone->GetEntitiesManagedBy(responsibleEntity);
  }

  if (managedEntityCap && managedCountForEntity >= managedEntityCap) {
    LogSkillManagerError([&]() {
      auto username = client->GetClientState()
                          ->GetAccountLogin()
                          ->GetAccount()
                          ->GetUsername();

      return libcomp::String(
                 "Account %1 tried to spawn more enemies but they already "
                 "spawned %2 in zone %3 with a cap of %4.\n")
          .Arg(username)
          .Arg(managedCountForEntity)
          .Arg(zone->GetDefinitionID())
          .Arg(managedEntityCap);
    });

    if (worldSharedConfig->GetAutobanSpawnSpammers()) {
      auto targetAccount =
          client->GetClientState()->GetAccountLogin()->GetAccount();
      targetAccount->SetEnabled(false);
      targetAccount->SetBanReason(
          libcomp::String(
              "Account tried to spawn more than %1 enemies into a zone.")
              .Arg(managedEntityCap));
      targetAccount->SetBanInitiator("<channel server>");
      targetAccount->Update(server->GetLobbyDatabase());
      client->Close();

      return false;
    }

    server->GetChatManager()->SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "You may not spawn more enemies. Kill the enemies you spawned.");

    SendFailure(activated, client, (uint8_t)SkillErrorCodes_t::SILENT_FAIL);
    return false;
  }

  return true;
}
