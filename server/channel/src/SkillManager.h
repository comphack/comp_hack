/**
 * @file server/channel/src/SkillManager.h
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

#ifndef SERVER_CHANNEL_SRC_SKILLMANAGER_H
#define SERVER_CHANNEL_SRC_SKILLMANAGER_H

// channel Includes
#include "ChannelClientConnection.h"

// objgen Includes
#include <MiSkillBasicData.h>

namespace libcomp {
class ScriptEngine;
}

namespace objects {
class ActivatedAbility;
class Enemy;
class ItemDrop;
class MiSkillData;
class Spawn;
class TokuseiSkillCondition;
}  // namespace objects

namespace channel {

typedef objects::MiSkillBasicData::ActionType_t SkillActionType_t;
typedef objects::MiSkillBasicData::ActivationType_t SkillActivationType_t;
typedef objects::MiSkillBasicData::DependencyType_t SkillDependencyType_t;
typedef objects::MiSkillBasicData::Family_t SkillFamily_t;

class ProcessingSkill;
class SkillLogicSettings;
class SkillTargetResult;

class ActiveEntityState;
class AIState;
class ChannelServer;

/**
 * Container for skill execution contextual parameters.
 */
class SkillExecutionContext {
  friend class SkillManager;

 public:
  /// If specified, the availability of the requested skill will be ignored,
  /// including if a non-living source tries to use an instant activation
  /// (ex: death triggers).
  bool IgnoreAvailable = false;

 protected:
  /* The following options are for internal processing use only */

  /// true if status effects will be processed normally, false if they
  /// have been handled elsewhere
  bool ApplyStatusEffects = true;

  /// true if normal skill aggro should be applied, false if it should
  /// not be applied or has already been handled
  bool ApplyAggro = true;

  /// Indicates that the skill's has already been executed
  bool Executed = false;

  /// Indicates that the skill's has already been finalized
  bool Finalized = false;

  /// Indicates that no skill delays should be used when executing.
  /// Useful for "catching up" with something that should have already
  /// started earlier.
  bool FastTrack = false;

  /// Indicates that the skill should stop processing at the next check
  bool Fizzle = false;

  /// Designates the skill that is being processed
  std::shared_ptr<channel::ProcessingSkill> Skill;

  /// Designates a skill that is being countered (for any defensive skill)
  std::shared_ptr<channel::ProcessingSkill> CounteredSkill;

  /// List of skills that are countering the current skill context
  /// (can be any defensive skill, not just counters)
  std::list<std::shared_ptr<channel::ProcessingSkill>> CounteringSkills;

  /// List of skill contexts created as a result of this one, typically
  /// as a result of a defensive skill
  std::list<std::shared_ptr<channel::SkillExecutionContext>> SubContexts;
};

/**
 * Manager to handle skill focused actions.
 */
class SkillManager {
 public:
  /**
   * Create a new SkillManager.
   * @param server Pointer back to the channel server this
   *  belongs to
   */
  SkillManager(const std::weak_ptr<ChannelServer>& server);

  /**
   * Clean up the SkillManager.
   */
  ~SkillManager();

  /**
   * Activate the skill of an active entity. The client will have the
   * charge information sent to it if there is a charge time, otherwise
   * it will execute automatically.
   * @param source Pointer of the entity that activated the skill
   * @param skillID ID of the skill to activate
   * @param activationObjectID ID of the object used as the activation target
   * @param targetObjectID ID of the object being targeted by the skill,
   *  typically only set for instant activation skills at this phase. If
   *  a call to ExecuteSkill is performed after activation, the supplied
   *  targetObjectID will override this value.
   * @param targetType Type of entity/object being targeted by the
   *  activationObjectID (and sometimes targetObjectID as well)
   * @param ctx Special execution state for the skill (ex: free cast, counter)
   * @return true if the skill was activated successfully, false otherwise
   */
  bool ActivateSkill(const std::shared_ptr<ActiveEntityState> source,
                     uint32_t skillID, int64_t activationObjectID,
                     int64_t targetObjectID, uint8_t targetType,
                     std::shared_ptr<SkillExecutionContext> ctx = 0);

  /**
   * Target/retarget the skill of a character or demon. No response is sent
   * to the clients from this request.
   * @param source Pointer of the entity that activated the skill
   * @param targetObjectID ID of the object being targeted by the skill
   * @return true if the skill was targeted successfully, false otherwise
   */
  bool TargetSkill(const std::shared_ptr<ActiveEntityState> source,
                   int64_t targetObjectID);

  /**
   * Execute the skill of an active entity.
   * @param source Pointer of the entity that is executing the skill
   * @param activationID ID of the activated ability instance
   * @param targetObjectID ID of the object being targeted by the skill
   * @param ctx Special execution state for the skill (ex: free cast, counter)
   * @return true if the skill was executed successfully, false otherwise
   */
  bool ExecuteSkill(const std::shared_ptr<ActiveEntityState> source,
                    int8_t activationID, int64_t targetObjectID,
                    std::shared_ptr<SkillExecutionContext> ctx = 0);

  /**
   * Cancel the activated skill of an active entity.
   * @param source Pointer of the entity that is cancelling the skill
   * @param activationID ID of the activated ability instance
   * @param cancelType Cancellation type to send to the client
   * @return true if the skill was cancelled successfully, false otherwise
   */
  bool CancelSkill(const std::shared_ptr<ActiveEntityState> source,
                   int8_t activationID, uint8_t cancelType = 1);

  /**
   * Notify the client that a skill failed activation or execution.
   * @param source Pointer of the entity that the skill failed for
   * @param skillID ID of the skill to activate
   * @param client Pointer to the client connection that should be sent
   *  the failure instead of the entire zone
   * @param errorCode Error code to send (defaults to generic, no message)
   * @param activationID ID of the activated ability instance (or -1 for none)
   */
  void SendFailure(const std::shared_ptr<ActiveEntityState> source,
                   uint32_t skillID,
                   const std::shared_ptr<ChannelClientConnection> client,
                   uint8_t errorCode = 13, int8_t activationID = -1);

  /**
   * Determine if the specified skill is locked from use on the supplied
   * entity.
   * @param source Entity that is attempting to use the skill
   * @param skillData Pointer to the skill's definition
   * @param activationObjectID Optional ID of the object used as the
   *  activation target. If not specified, context specific checks will fail.
   * @param ctx Special execution state for the skill
   * @return true if the skill is restricted from being used
   */
  bool SkillRestricted(const std::shared_ptr<ActiveEntityState> source,
                       const std::shared_ptr<objects::MiSkillData>& skillData,
                       int64_t activationObjectID = 0,
                       std::shared_ptr<SkillExecutionContext> ctx = nullptr);

  /**
   * Determine if the specified skill is locked from use in the current zone.
   * @param skillID ID of the skill being checked
   * @param zone Pointer to the current zone
   * @return true if the skill is restricted from being used
   */
  bool SkillZoneRestricted(uint32_t skillID, const std::shared_ptr<Zone> zone);

  /**
   * Determine if the skill target supplied is in range for a specific skill
   * @param source Entity that is attempting to use the skill
   * @param skillData Pointer to the skill's definition
   * @param target Pointer to the targeted entity
   * @return true if the target entity is in range, false if it is not
   */
  bool TargetInRange(const std::shared_ptr<ActiveEntityState> source,
                     const std::shared_ptr<objects::MiSkillData>& skillData,
                     const std::shared_ptr<ActiveEntityState>& target);

  /**
   * Determine if the skill target supplied is valid for a specific skill
   * @param source Entity that is attempting to use the skill
   * @param skillData Pointer to the skill's definition
   * @param target Pointer to the targeted entity
   * @return Skill error code or -1 if no error
   */
  int8_t ValidateSkillTarget(
      const std::shared_ptr<ActiveEntityState> source,
      const std::shared_ptr<objects::MiSkillData>& skillData,
      const std::shared_ptr<ActiveEntityState>& target);

  /**
   * Determine if the skill activation item supplied is valid for a specific
   * source entity
   * @param source Entity that is attempting to use the skill
   * @param item Pointer to the item used for skill activation
   * @return true if it is valid, false if it is not
   */
  bool ValidateActivationItem(const std::shared_ptr<ActiveEntityState> source,
                              const std::shared_ptr<objects::Item>& item);

  /**
   * Determine if a skill has more uses pending
   * @param activated Pointer to the activated skill ability
   * @return true if the skill has more uses pending
   */
  static bool SkillHasMoreUses(
      const std::shared_ptr<objects::ActivatedAbility>& activated);

  /**
   * Get the charge and post charge movement speeds for a skill for the
   * supplied entity's current state
   * @param source Pointer to the state of the source entity
   * @param skillData Pointer to the skill's definition
   * @return Charge and post charage movement speeds
   */
  static std::pair<float, float> GetMovementSpeeds(
      const std::shared_ptr<ActiveEntityState>& source,
      const std::shared_ptr<objects::MiSkillData>& skillData);

  /**
   * Check fusion skill usage pre-skill validation and determine which one
   * should be used. Fusion skills are decided based upon the two demons
   * involved, defaulting to the COMP demon if no special skill exists.
   * @param client Pointer to the client connection belonging to the player
   *  activating the skill. Since non-player entities cannot use fusion
   *  skills, this is required unlike most other skill related processes.
   * @param skillID Current skill being used to execute the fusion skill
   *  which doubles as an output parameter for the context specific skill
   * @param targetEntityID ID of the entity being targeted when the skill
   *  is activated. Targeted fusion skills require a target is selected
   *  before charging starts.
   * @param mainDemonID Object ID of the demon that is currently summoned
   * @param compDemonID Object ID of the demon that is in the COMP
   * @return false if the fusion skill cannot be used for any reason
   */
  bool PrepareFusionSkill(const std::shared_ptr<ChannelClientConnection> client,
                          uint32_t& skillID, int32_t targetEntityID,
                          int64_t mainDemonID, int64_t compDemonID);

  /**
   * Begin executing a skill. If skill staggering is enabled and the target
   * is already being attacked or is stunned, the source will be sent a
   * retry request that will occur automatically for clients.
   * @param pSkill Current skill processing state
   * @param ctx Special execution state for the skill
   * @retult false if the skill failed execution immediately, true if it
   *  succeeded or was queued
   */
  bool BeginSkillExecution(std::shared_ptr<ProcessingSkill> pSkill,
                           std::shared_ptr<SkillExecutionContext> ctx);

  /**
   * Complete executing a skill. If the source has been killed or stunned
   * and it is not a skill that ignores these, it will not execute but
   * costs will still be paid as the skill has already been "shot" by now.
   * @param pSkill Current skill processing state
   * @param ctx Special execution state for the skill
   * @param syncTime Accurate skill completion time, useful for server
   *  processing delay correction
   * @retult false if the skill failed execution immediately, true if it
   *  succeeded or was queued
   */
  bool CompleteSkillExecution(std::shared_ptr<ProcessingSkill> pSkill,
                              std::shared_ptr<SkillExecutionContext> ctx,
                              uint64_t syncTime);

  /**
   * Complete executing a skill with a projectile, simulating the effect
   * of the projectile having just hit
   * @param pSkill Current skill processing state
   * @param ctx Special execution state for the skill
   */
  void ProjectileHit(std::shared_ptr<ProcessingSkill> pSkill,
                     std::shared_ptr<SkillExecutionContext> ctx);

  /**
   * Determine how much a skill would cost to execute at a base level
   * for any executing entity, not just a player entity. Useful for AI
   * determining skill usefullness. Determining if the costs CAN be paid
   * must be handled elsewhere.
   * @param source Pointer to the entity that would use the skill
   * @param skillData Pointer to the skill definition
   * @param hpCost Output parameter containing the HP cost, adjusted
   *  for tokusei
   * @param mpCost Output parameter containing the MP cost, adjusted
   *  for tokusei
   * @param bulletCost Output parameter containing how many bullets
   *  would be consumed by the skill
   * @param itemCosts Output parameter containing how many items would
   *  be consumed by the skill (type to count)
   * @return true if no errors were encountered in the costs, false if
   *  an error occurred
   */
  bool DetermineNormalCosts(
      const std::shared_ptr<ActiveEntityState>& source,
      const std::shared_ptr<objects::MiSkillData>& skillData, int32_t& hpCost,
      int32_t& mpCost, uint16_t& bulletCost,
      std::unordered_map<uint32_t, uint32_t>& itemCosts,
      std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr);

  /**
   * Schedule automatic skill cancellation after a set amount of time
   * as defined by the skill definition. If the skill is mutli-use,
   * this should be called after each non-final execution.
   * @param source Pointer to the entity using the skill
   * @param activated Pointer to the activated skill ability
   */
  void ScheduleAutoCancel(
      const std::shared_ptr<ActiveEntityState> source,
      const std::shared_ptr<objects::ActivatedAbility>& activated);

  /**
   * Check if the supplied function ID is mapped to a special function within
   * the skill manager. Skill function IDs that are not mapped will react like
   * no function ID skills when processed.
   * @param functionID Function ID to check
   * @return true if it is mapped, false if it is not
   */
  bool FunctionIDMapped(uint16_t functionID);

 private:
  /**
   * Load scripts bound to function IDs. Only used once during startup.
   */
  void LoadScripts();

  /**
   * Notify the client that a skill failed activation or execution.
   * @param activated Pointer to the activated ability instance
   * @param client Pointer to the client connection, can be null if not coming
   *  from a player entity
   * @param errorCode Error code to send (defaults to generic, no message)
   */
  void SendFailure(std::shared_ptr<objects::ActivatedAbility> activated,
                   const std::shared_ptr<ChannelClientConnection> client,
                   uint8_t errorCode = 0);

  /**
   * Get the current or past special skill activation by ID.
   * @param source Pointer to the skill source entity
   * @param activationID ID of the activation to retrieve
   * @return Pointer to the matching skill activation, null if it does not
   *  exist
   */
  std::shared_ptr<objects::ActivatedAbility> GetActivation(
      const std::shared_ptr<ActiveEntityState> source,
      int8_t activationID) const;

  /**
   * Calculate and set the costs required to execute a skill.
   * @param source Pointer to the state of the source entity
   * @param activated Pointer to the activated ability instance
   * @param client Pointer to the client connection, can be null if not coming
   *  from a player entity
   * @param ctx Special execution state for the skill
   * @return true if the skill can be paid, false if it cannot
   */
  bool DetermineCosts(std::shared_ptr<ActiveEntityState> source,
                      std::shared_ptr<objects::ActivatedAbility> activated,
                      const std::shared_ptr<ChannelClientConnection> client,
                      std::shared_ptr<SkillExecutionContext> ctx);

  /**
   * Pay any costs related to and execute the skill of a character or demon.
   * @param source Pointer to the state of the source entity
   * @param activated Pointer to the activated ability instance
   * @param client Pointer to the client connection, can be null if not coming
   *  from a player entity
   * @param ctx Special execution state for the skill
   * @param delayedAuto If true, the skill is being executed as the result
   *  of delayed auto-exection from an activation request
   * @return true if the skill was executed successfully, false otherwise
   */
  bool ExecuteSkill(std::shared_ptr<ActiveEntityState> source,
                    std::shared_ptr<objects::ActivatedAbility> activated,
                    const std::shared_ptr<ChannelClientConnection> client,
                    std::shared_ptr<SkillExecutionContext> ctx,
                    bool delayedAuto = false);

  /**
   * Execute a normal skill, not handled by a special handler.
   * @param client Pointer to the client connection that activated the skill
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   */
  bool ExecuteNormalSkill(const std::shared_ptr<ChannelClientConnection> client,
                          std::shared_ptr<objects::ActivatedAbility> activated,
                          const std::shared_ptr<SkillExecutionContext>& ctx);

  /**
   * Process the results of a skill. If the skill involves a projectile
   * this will be scheduled to execute after skill execution, otherwise it will
   * execute immediately.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @return true if the skill processed successfully, false otherwise
   */
  bool ProcessSkillResult(std::shared_ptr<objects::ActivatedAbility> activated,
                          std::shared_ptr<SkillExecutionContext> ctx);

  /**
   * Finalize skill processing and send the skill effect reports.
   * @param pSkill Current skill processing state
   * @param ctx Special execution state for the skill
   */
  void ProcessSkillResultFinal(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill,
      std::shared_ptr<SkillExecutionContext> ctx);

  /**
   * Prepare an executing fusion skill by setting the fusion demons that will
   * be used for damage calculation.
   * @param source Pointer to the state of the source entity
   * @param pSkill Current skill processing state
   * @return true if the skill was prepared successfully, false if it was not
   */
  bool ProcessFusionExecution(
      std::shared_ptr<ActiveEntityState> source,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Get a processing skill set with default values from the supplied
   * ability and context. If the context already has a processing skill
   * set, it will be returned instead.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @return Pointer to a processing skill contextual to the values supplied
   */
  std::shared_ptr<ProcessingSkill> GetProcessingSkill(
      std::shared_ptr<objects::ActivatedAbility> activated,
      std::shared_ptr<SkillExecutionContext> ctx);

  /**
   * Get a CalculatedEntityState based upon the skill being executed and
   * the state of the entity as either the source or a target of the skill.
   * @param eState Pointer to the entity to get a CalculatedEntityState for
   * @param pSkill Pointer to the current skill processing state
   * @param isTarget false if the entity is the source of the skill, false
   *  if the entity is a target
   * @param otherState Pointer to the source entity if a target is being
   *  calculated or the target in question if the source is being calculated.
   *  If the skill is still being checked validity, supplying no entity
   *  for this while calculating against the source is valid.
   * @return Pointer to the CalculatedEntityState to use relative to the
   *  supplied parameters. If the effects calculated for the entity do not
   *  differ from one already on the entity or used as the source execution
   *  context, the same one will be returned.
   */
  std::shared_ptr<objects::CalculatedEntityState> GetCalculatedState(
      const std::shared_ptr<ActiveEntityState>& eState,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill, bool isTarget,
      const std::shared_ptr<ActiveEntityState>& otherState);

  /**
   * Evaluate TokuseiSkillConditions relative to the current skill and
   * supplied entities.
   * @param eState Pointer to the entity the effect applies to
   * @param conditions List of points to the condition definitions
   * @param pSkill Pointer to the current skill processing state
   * @param otherState Pointer to the source entity if a target is being
   *  calculated or the target in question if the source is being calculated.
   *  If the skill is still being checked validity, supplying no entity
   *  for this while calculating against the source is valid.
   * @return 0 if the conditions evaluated to true, 1 if the conditions
   *  evaluated to false, -1 if the conditions cannot be evaluated yet
   */
  int8_t EvaluateTokuseiSkillConditions(
      const std::shared_ptr<ActiveEntityState>& eState,
      const std::list<std::shared_ptr<objects::TokuseiSkillCondition>>&
          conditions,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill,
      const std::shared_ptr<ActiveEntityState>& otherState);

  /**
   * Evaluate a TokuseiSkillCondition relative to the current skill and
   * supplied entities.
   * @param eState Pointer to the entity the effect applies to
   * @param condition Pointer to the condition definition
   * @param pSkill Pointer to the current skill processing state
   * @param otherState Pointer to the source entity if a target is being
   *  calculated or the target in question if the source is being calculated.
   *  If the skill is still being checked validity, supplying no entity
   *  for this while calculating against the source is valid.
   * @return 0 if the conditions evaluated to true, 1 if the conditions
   *  evaluated to false, -1 if the conditions cannot be evaluated yet
   */
  int8_t EvaluateTokuseiSkillCondition(
      const std::shared_ptr<ActiveEntityState>& eState,
      const std::shared_ptr<objects::TokuseiSkillCondition>& condition,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill,
      const std::shared_ptr<ActiveEntityState>& otherState);

  /**
   * Calculate and set the offense value of a damage dealing skill.
   * @param source Pointer to the state of the source entity
   * @param target Pointer to the state of the target entity
   * @param pSkill Pointer to the current skill processing state
   * @return Calculated offsense value of the skill
   */
  uint16_t CalculateOffenseValue(
      const std::shared_ptr<ActiveEntityState>& source,
      const std::shared_ptr<ActiveEntityState>& target,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Determine how the primary target reacts to being hit by a skill by
   * either guarding, dodging or countering. This assumes NRA has already
   * been applied. Upon success a target for the entity will be added to the
   * skill's targets list.
   * @param source Pointer to the state of the source entity
   * @param pSkill Current skill processing state
   * @param guard If true, guard skills will be checked too, if false only
   *  dodge and counter will be checked
   * @return true if the skill was countered, false if it was not
   */
  bool ApplyPrimaryCounter(
      const std::shared_ptr<ActiveEntityState>& source,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill, bool guard);

  /**
   * Determine how non-primary targets react to being hit by a skill by
   * either guarding or dodging. Countering cannot occur at this point.
   * This assumes NRA has already been applied.
   * @param source Pointer to the state of the source entity
   * @param target Skill targeted entity
   * @param pSkill Skill processing state of the skill
   */
  void ApplySecondaryCounter(
      const std::shared_ptr<ActiveEntityState>& source,
      SkillTargetResult& target,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Execute or cancel the guard skill currently being used by the
   * supplied target from being hit by the source entity
   * @param source Pointer to the state of the source entity
   * @param target Skill targeted entity
   * @param pSkill Skill processing state of the skill being guarded
   * @return true if the skill was guarded against, false if it was not
   */
  bool HandleGuard(const std::shared_ptr<ActiveEntityState>& source,
                   SkillTargetResult& target,
                   const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Execute or cancel the counter skill currently being used by the
   * supplied target from being hit by the source entity. Countering
   * skills activate manually from the source and are actually executed
   * in the middle of the skill being countered. The countering skill
   * will have its CounteredSkill property set on the context and the
   * skill being countered will have the countering skill added to its
   * CounteringSkills list, since technically multiple skills can
   * counter one.
   * @param source Pointer to the state of the source entity
   * @param target Skill targeted entity
   * @param pSkill Skill processing state of the skill being countered
   * @return true if the skill was countered, false if it was not
   */
  bool HandleCounter(const std::shared_ptr<ActiveEntityState>& source,
                     SkillTargetResult& target,
                     const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Execute or cancel the dodge skill currently being used by the
   * supplied target from being hit by the source entity
   * @param source Pointer to the state of the source entity
   * @param target Skill targeted entity
   * @param pSkill Skill processing state of the skill being dodged
   * @return true if the skill was dodged, false if it was not
   */
  bool HandleDodge(const std::shared_ptr<ActiveEntityState>& source,
                   SkillTargetResult& target,
                   const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Apply target skill interrupt based upon timing and state of the target
   * entity's skill being charged or executed
   * @param source Pointer to the state of the source entity
   * @param target Skill targeted entity
   * @param pSkill Skill processing state of the skill being guarded
   * @return true if the target had a skill that was interrupted, false if
   *  they did not
   */
  bool HandleSkillInterrupt(
      const std::shared_ptr<ActiveEntityState>& source,
      SkillTargetResult& target,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Handle all status effect application logic for a specific target
   * @param source Pointer to the state of the source entity
   * @param target Skill targeted entity
   * @param pSkill Skill processing state of the skill being executed
   * @return Set of status effects being applied that will be cancelled
   *  if the entity is killed as a result of the skill
   */
  std::set<uint32_t> HandleStatusEffects(
      const std::shared_ptr<ActiveEntityState>& source,
      SkillTargetResult& target,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Handle all logic related to kills that occurred from a skill's execution.
   * @param source Pointer to the source of the skill
   * @param zone Pointer to the zone where the skill was executed
   * @param killed Set of entities killed by the skill's execution
   */
  void HandleKills(const std::shared_ptr<ActiveEntityState> source,
                   const std::shared_ptr<Zone>& zone,
                   const std::set<std::shared_ptr<ActiveEntityState>> killed);

  /**
   * Distribute all XP from a defeated enemy to each player that caused damage
   * to it, adjusting for active party members and players still in the zone.
   * @param enemy Pointer to the enemy that was killed
   * @param zone Pointer ot the zone where the enemy was killed
   */
  void HandleKillXP(const std::shared_ptr<objects::Enemy>& enemy,
                    const std::shared_ptr<Zone>& zone);

  /**
   * Increase digitalize experience points for the supplied source's party
   * based upon defeated enemies. Points are granted at a flat rate and
   * unlike XP are not shared with other parties that dealt damage.
   * @param enemies List of pointers to enemies that have been killed
   * @param zone Pointer ot the zone where the enemies were killed
   */
  void HandleDigitalizeXP(
      const std::shared_ptr<ActiveEntityState> source,
      const std::list<std::shared_ptr<ActiveEntityState>>& enemies,
      const std::shared_ptr<Zone>& zone);

  /**
   * Handle all logic related to defeating an enemy encounter through combat
   * or talk skill outcome. Enemy despawn does not count as a defeat and should
   * be handled separately. If the encounter is not actually defeated, nothing
   * will be done.
   * @param source Pointer to the source of the skill
   * @param zone Pointer to the zone where the skill was executed
   * @param encounterGroups Map of encounter IDs to spawn group ID
   */
  void HandleEncounterDefeat(
      const std::shared_ptr<ActiveEntityState> source,
      const std::shared_ptr<Zone>& zone,
      const std::unordered_map<uint32_t, uint32_t>& encounterGroups);

  /**
   * Handle all logic related to revivals that occurred from a skill's
   * execution.
   * @param zone Pointer to the zone where the skill was executed
   * @param revived Set of entities revived by the skill's execution
   * @param pSkill Skill processing state of the skill being executed
   */
  void HandleRevives(const std::shared_ptr<Zone>& zone,
                     const std::set<std::shared_ptr<ActiveEntityState>> revived,
                     const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * After calculating the normal skill results during final processing
   * zone specific changes are applied here.
   * @param pSkill Skill processing state of the skill being executed
   * @return true if any changes were applied, false if they were not
   */
  bool ApplyZoneSpecificEffects(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Update all PvP instance recorded statistics if the match is active
   * @param pSkill Skill processing state of the skill being executed
   */
  void UpdatePvPStats(const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Determine if the supplied target should receive any negotation
   * related effects from the skill used and apply them if applicable.
   * @param source Pointer to the state of the source entity
   * @param target Skill targeted entity
   * @param pSkill Skill processing state of the skill being executed
   * @return true if the negotation has completed
   */
  bool ApplyNegotiationDamage(
      const std::shared_ptr<ActiveEntityState>& source,
      SkillTargetResult& target,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Handle the outcome of a negotation ending from a skill's execution
   * including things like creating demon eggs and gift boxes or running away.
   * @param source Pointer to the source of the skill
   * @param zone Pointer to the zone where the skill was executed
   * @param talkDone List of entities that have finished negotiation with
   *  the skill source and their respective outcomes related to respons flags
   */
  void HandleNegotiations(
      const std::shared_ptr<ActiveEntityState> source,
      const std::shared_ptr<Zone>& zone,
      const std::list<std::pair<std::shared_ptr<ActiveEntityState>, uint8_t>>
          talkDone);

  /**
   * Update InheritedSkills on the supplied entity if its entity type
   * is partner demon. This should be called once for each entity touched
   * by the processing skill, regardless of if anything happened to it.
   * @param entity Pointer to the entity to update
   * @param pSkill Pointer to the current skill processing state
   */
  void HandleSkillLearning(
      const std::shared_ptr<ActiveEntityState> entity,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Update item durability on the supplied entity if its entity is a
   * character. This should be called for all source entities and only targets
   * that have not avoided the hit.
   * @param entity Pointer to the entity to update
   * @param pSkill Pointer to the current skill processing state
   */
  void HandleDurabilityDamage(
      const std::shared_ptr<ActiveEntityState> entity,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * If the skill is not a fusion skill and also an offensive action type,
   * raise the fusion gauge.
   * @param pSkill Pointer to the current skill processing state
   */
  void HandleFusionGauge(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Interrupt any active events associated to the supplied world CIDs as
   * a result of being hit by a combat skill
   * @param worldCIDs List of player world CIDs
   */
  void InterruptEvents(const std::set<int32_t>& worldCIDs);

  /**
   * Toggle a switch skill, not handled by a special handler.
   * @param client Pointer to the client connection that activated the skill
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   */
  bool ToggleSwitchSkill(const std::shared_ptr<ChannelClientConnection> client,
                         std::shared_ptr<objects::ActivatedAbility> activated,
                         const std::shared_ptr<SkillExecutionContext>& ctx);

  /**
   * Calculate skill damage or healing using the correct formula
   * @param source Pointer to the entity that activated the skill
   * @param pSkill Pointer to the current skill processing state
   * @return true if the calculation succeeded, false otherwise
   */
  bool CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
                       const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Get the critical level of a skill that is being calculated for
   * damage and has already been determined as a hit. This should not
   * be called if the skill fundamentally will never crit such as if
   * it is a healing skill.
   * @param source Pointer to the entity that activated the skill
   * @param target Pointer to the entity that will receive damage
   * @param pSkill Pointer to the current skill processing state
   * @return 0 for no crit, 1 for normal crit, 2 for limit break
   */
  uint8_t GetCritLevel(const std::shared_ptr<ActiveEntityState>& source,
                       SkillTargetResult& target,
                       const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Get the entity rate scaling correct table value associated to the
   * the supplied entity from the other entity's calculated state
   * @param eState Pointer to the entity in question
   * @param calcState Pointer to the other entity's calculated state
   * @param taken true if the damage taken rate should be retrieved, false if
   *  the damage dealt rate should be retrieved
   * @return Correct table value for the rate scaling
   */
  int16_t GetEntityRate(
      const std::shared_ptr<ActiveEntityState> eState,
      std::shared_ptr<objects::CalculatedEntityState> calcState, bool taken);

  /**
   * Get the specific affinity boost associated to the supplied entity. Max
   * affinity cap is applied to the value.
   * @param eState Pointer to the entity
   * @param calcState Pointer to entity's calculated state
   * @param boostType CorrectTbl index of the boost type
   * @return Calculated boost level as a decimal
   */
  float GetAffinityBoost(
      const std::shared_ptr<ActiveEntityState> eState,
      std::shared_ptr<objects::CalculatedEntityState> calcState,
      CorrectTbl boostType);

  /**
   * Calculate skill damage or healing using the default formula
   * @param source Pointer to the entity that activated the skill
   * @param target Pointer to the entity that will receive damage
   * @param pSkill Pointer to the current skill processing state
   * @param mod Base modifier damage value
   * @param damageType Type of damage being dealt
   * @param resist Resistence to the skill affinity
   * @param critLevel Critical level adjustment. Valid values are:
   *  0) No critical adjustment
   *  1) Critical hit
   *  2) Limit break
   * @param isHeal true if healing "damage" should be applied instead
   * @return Raw calculated damage or healing (no cap applied)
   */
  int32_t CalculateDamage_Normal(
      const std::shared_ptr<ActiveEntityState>& source,
      SkillTargetResult& target,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill, uint16_t mod,
      uint8_t& damageType, float resist, uint8_t critLevel, bool isHeal);

  /**
   * Calculate skill damage or healing based on a static value
   * @param mod Base modifier damage value
   * @param damageType Type of damage being dealt
   * @return Raw calculated damage or healing (no cap applied)
   */
  int32_t CalculateDamage_Static(uint16_t mod, uint8_t& damageType);

  /**
   * Calculate skill damage or healing based on a percent value
   * @param mod Base modifier damage value
   * @param damageType Type of damage being dealt
   * @return Raw calculated damage or healing (no cap applied)
   */
  int32_t CalculateDamage_Percent(uint16_t mod, uint8_t& damageType,
                                  int32_t current);

  /**
   * Calculate skill damage or healing based on a max relative value
   * @param mod Base modifier damage value
   * @param damageType Type of damage being dealt
   * @param max Maximum value to calculate relative damage to
   * @return Raw calculated damage or healing (no cap applied)
   */
  int32_t CalculateDamage_MaxPercent(uint16_t mod, uint8_t& damageType,
                                     int32_t max);

  /**
   * Adjust skill damage or healing using skill rates from the source
   * and target entities
   * @param source Pointer to the entity that activated the skill
   * @param target Pointer to the entity that will receive damage
   * @param pSkill Pointer to the current skill processing state
   * @param isHeal true if healing "damage" should be applied instead
   * @param adjustPower If true, adjuste tokusei EFFECT_POWER as well
   * @return Raw calculated damage or healing (no cap applied)
   */
  int32_t AdjustDamageRates(
      int32_t damage, const std::shared_ptr<ActiveEntityState>& source,
      const std::shared_ptr<ActiveEntityState>& target,
      const std::shared_ptr<channel::ProcessingSkill>& pSkill, bool isHeal,
      bool adjustPower);

  /**
   * Get the skill target that hits the source entity or create it if it does
   * not already exist
   * @param source Pointer to the entity that activated the skill
   * @param targets List target results associated to the skill
   * @param indirectOnly If true and the target does not already exist, it will
   *  be created as an indirect target
   * @param autoCreate If true and the target does not exist, create it
   * @return Pointer to the skill target representing the source entity
   */
  SkillTargetResult* GetSelfTarget(
      const std::shared_ptr<ActiveEntityState>& source,
      std::list<SkillTargetResult>& targets, bool indirectDefault,
      bool autoCreate = true);

  /**
   * Set null, reflect or absorb on the skill target
   * @param target Target to update with NRA flags
   * @param skill Current skill processing state
   * @param reduceShields If true, any NRA shields will be consumed should
   *  they apply first
   * @return true if the damage was reflected, otherwise false
   */
  bool SetNRA(SkillTargetResult& target, ProcessingSkill& skill,
              bool reduceShields = true);

  /**
   * Get the result of the skill target's null, reflect or absorb checks
   * @param target Target to determine NRA for
   * @param skill Current skill processing state
   * @param effectiveAffinity Target specific affinity type for the skill
   * @param resultAffinity Output parameter containing the NRA affinity type
   *  if it occurs
   * @return NRA index value or 0 for none
   */
  uint8_t GetNRAResult(SkillTargetResult& target, ProcessingSkill& skill,
                       uint8_t effectiveAffinity);

  /**
   * Get the result of the skill target's null, reflect or absorb checks
   * @param target Target to determine NRA for
   * @param skill Current skill processing state
   * @param effectiveAffinity Target specific affinity type for the skill
   * @param resultAffinity Output parameter containing the NRA affinity type
   *  if it occurs
   * @param effectiveOnly If true, only the supplied affinity will be checked,
   *  otherwise the dependency type and base affinities will be checked as well
   * @param reduceShields If true, any NRA shields will be consumed should
   *  they apply first
   * @return NRA index value or 0 for none
   */
  uint8_t GetNRAResult(SkillTargetResult& target, ProcessingSkill& skill,
                       uint8_t effectiveAffinity, uint8_t& resultAffinity,
                       bool effectiveOnly, bool reduceShields);

  /**
   * Get a random stack size for a status effect being applied
   * @param minStack Minimum size of the stack
   * @param maxStack Maximum size of the stack
   * @return Random stack size between the range or 0 if the range
   *  is invalid
   */
  int8_t CalculateStatusEffectStack(int8_t minStack, int8_t maxStack) const;

  /**
   * Gather drops for a specific enemy spawn from its own drops, global drops
   * and demon family drops.
   * @param source Pointer to the entity that activated the skill
   * @param eState Pointer to enemy which may or may not have spawn
   *  information (ex: GM created enemy)
   * @param client Pointer to the client connection related to the source,
   *  can be null for non-player entity sources
   * @param zone Pointer to the zone the entities belong to
   * @param giftMode true if the enemy's negotation gifts should be gathered
   *  instead of their normal drops
   * @return Map of drop set types to list of item drops from each different
   *  source
   */
  std::unordered_map<uint8_t, std::list<std::shared_ptr<objects::ItemDrop>>>
  GetItemDrops(const std::shared_ptr<ActiveEntityState>& source,
               const std::shared_ptr<ActiveEntityState>& eState,
               const std::shared_ptr<ChannelClientConnection>& client,
               const std::shared_ptr<Zone>& zone, bool giftMode = false);

  /**
   * Schedule the adjustment of who is a valid looter for one or more loot
   * boxes.
   * @param time Server time to schedule the adjustment for
   * @param zone Pointer to the zone the entities belong to
   * @param lootEntityIDs List of loot box IDs to adjust
   * @param worldCIDs Set of character world CIDs to add as valid looters
   */
  void ScheduleFreeLoot(uint64_t time, const std::shared_ptr<Zone>& zone,
                        const std::list<int32_t>& lootEntityIDs,
                        const std::set<int32_t>& worldCIDs);

  /**
   * Execute post execution steps like notifying the client that the skill
   * has executed and updating any related expertises
   * @param client Pointer to the client connection that activated the skill
   * @param ctx Special execution state for the skill
   * @param activated Pointer to the activated ability instance
   */
  void FinalizeSkillExecution(
      const std::shared_ptr<ChannelClientConnection> client,
      const std::shared_ptr<SkillExecutionContext>& ctx,
      std::shared_ptr<objects::ActivatedAbility> activated);

  /**
   * Finalize a skill that has been activated and remove it from the entity
   * if it does not have additional uses. If the skill's execution
   * results in additional uses remaining, make a copy of the instance to
   * preserve execution values.
   * @param ctx Special execution state for the skill
   * @param activated Pointer to the activated ability instance
   * @return Pointer to the ability instance being executed
   */
  std::shared_ptr<objects::ActivatedAbility> FinalizeSkill(
      const std::shared_ptr<SkillExecutionContext>& ctx,
      std::shared_ptr<objects::ActivatedAbility> activated);

  /**
   * Mark all information pertaining to a skill completing either through
   * execution or cancellation with more uses available.
   * @param pSkill Pointer to the skill being processed
   * @param executed true if the skill was executed, false if it is being
   * cancelled
   * @return true if the skill can be used again, false if it cannot
   */
  bool SetSkillCompleteState(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill, bool executed);

  /**
   * Placeholder function for a skill actually handled outside of the
   * SkillManager.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  can be null for non-player entity sources
   */
  bool SpecialSkill(const std::shared_ptr<objects::ActivatedAbility>& activated,
                    const std::shared_ptr<SkillExecutionContext>& ctx,
                    const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the "cameo" transformation skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Cameo(const std::shared_ptr<objects::ActivatedAbility>& activated,
             const std::shared_ptr<SkillExecutionContext>& ctx,
             const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute an entity cloaking skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Cloak(const std::shared_ptr<objects::ActivatedAbility>& activated,
             const std::shared_ptr<SkillExecutionContext>& ctx,
             const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the "demonic compendium memory" skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool DCM(const std::shared_ptr<objects::ActivatedAbility>& activated,
           const std::shared_ptr<SkillExecutionContext>& ctx,
           const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute an enemy despawning skill. This mimics the behavior of the skill
   * in other games as it was not originally released.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Despawn(const std::shared_ptr<objects::ActivatedAbility>& activated,
               const std::shared_ptr<SkillExecutionContext>& ctx,
               const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a targeted spawn desummon skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Desummon(const std::shared_ptr<objects::ActivatedAbility>& activated,
                const std::shared_ptr<SkillExecutionContext>& ctx,
                const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a skill that results in digitalizing the player character.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Digitalize(const std::shared_ptr<objects::ActivatedAbility>& activated,
                  const std::shared_ptr<SkillExecutionContext>& ctx,
                  const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a skill that forces the cancellation of all targets hit.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool DigitalizeBreak(
      const std::shared_ptr<objects::ActivatedAbility>& activated,
      const std::shared_ptr<SkillExecutionContext>& ctx,
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a digitalize state cancellation skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool DigitalizeCancel(
      const std::shared_ptr<objects::ActivatedAbility>& activated,
      const std::shared_ptr<SkillExecutionContext>& ctx,
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a skill that always adds a specific status effect after the
   * skill's execution.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool DirectStatus(const std::shared_ptr<objects::ActivatedAbility>& activated,
                    const std::shared_ptr<SkillExecutionContext>& ctx,
                    const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the "equip item" skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool EquipItem(const std::shared_ptr<objects::ActivatedAbility>& activated,
                 const std::shared_ptr<SkillExecutionContext>& ctx,
                 const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the aggro clearing "Estoma" skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Estoma(const std::shared_ptr<objects::ActivatedAbility>& activated,
              const std::shared_ptr<SkillExecutionContext>& ctx,
              const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a familiarity boosting skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool FamiliarityUp(
      const std::shared_ptr<objects::ActivatedAbility>& activated,
      const std::shared_ptr<SkillExecutionContext>& ctx,
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a familiarity boosting skill from an item. These work slightly
   * different from the standard skill ones as they can adjust a delta percent
   * instead of a fixed value.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool FamiliarityUpItem(
      const std::shared_ptr<objects::ActivatedAbility>& activated,
      const std::shared_ptr<SkillExecutionContext>& ctx,
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a skill that causes all character expertise skills to be forgotten.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool ForgetAllExpertiseSkills(
      const std::shared_ptr<objects::ActivatedAbility>& activated,
      const std::shared_ptr<SkillExecutionContext>& ctx,
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the aggro drawing "liberama" skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Liberama(const std::shared_ptr<objects::ActivatedAbility>& activated,
                const std::shared_ptr<SkillExecutionContext>& ctx,
                const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a minion despawning skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool MinionDespawn(
      const std::shared_ptr<objects::ActivatedAbility>& activated,
      const std::shared_ptr<SkillExecutionContext>& ctx,
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a minion spawning skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool MinionSpawn(const std::shared_ptr<objects::ActivatedAbility>& activated,
                   const std::shared_ptr<SkillExecutionContext>& ctx,
                   const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the "Mooch" skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Mooch(const std::shared_ptr<objects::ActivatedAbility>& activated,
             const std::shared_ptr<SkillExecutionContext>& ctx,
             const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a demon riding skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Mount(const std::shared_ptr<objects::ActivatedAbility>& activated,
             const std::shared_ptr<SkillExecutionContext>& ctx,
             const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a random item granting skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool RandomItem(const std::shared_ptr<objects::ActivatedAbility>& activated,
                  const std::shared_ptr<SkillExecutionContext>& ctx,
                  const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a skill that is requesting a random number.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Randomize(const std::shared_ptr<objects::ActivatedAbility>& activated,
                 const std::shared_ptr<SkillExecutionContext>& ctx,
                 const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a skill that causes all character skill points to be reset.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Respec(const std::shared_ptr<objects::ActivatedAbility>& activated,
              const std::shared_ptr<SkillExecutionContext>& ctx,
              const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Handle the "Rest" skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Rest(const std::shared_ptr<objects::ActivatedAbility>& activated,
            const std::shared_ptr<SkillExecutionContext>& ctx,
            const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a (non-zone specific) enemy spawning skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Spawn(const std::shared_ptr<objects::ActivatedAbility>& activated,
             const std::shared_ptr<SkillExecutionContext>& ctx,
             const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a zone specific enemy spawning skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool SpawnZone(const std::shared_ptr<objects::ActivatedAbility>& activated,
                 const std::shared_ptr<SkillExecutionContext>& ctx,
                 const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the "summon demon" skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool SummonDemon(const std::shared_ptr<objects::ActivatedAbility>& activated,
                   const std::shared_ptr<SkillExecutionContext>& ctx,
                   const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the "store demon" skill.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool StoreDemon(const std::shared_ptr<objects::ActivatedAbility>& activated,
                  const std::shared_ptr<SkillExecutionContext>& ctx,
                  const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute the skill "Traesto" which returns the user to a preset zone.
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool Traesto(const std::shared_ptr<objects::ActivatedAbility>& activated,
               const std::shared_ptr<SkillExecutionContext>& ctx,
               const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Execute a skill that grants XP to the character or partner demon
   * @param activated Pointer to the activated ability instance
   * @param ctx Special execution state for the skill
   * @param client Pointer to the client connection that activated the skill,
   *  this will always fail if it is null
   */
  bool XPUp(const std::shared_ptr<objects::ActivatedAbility>& activated,
            const std::shared_ptr<SkillExecutionContext>& ctx,
            const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Run applicable script validations for an executing or activating skill
   * and stop execution if they do not pass.
   * @param pSkill Current skill processing state
   * @param execution If true, execution is being checked. If false,
   *  activation is being checked.
   * @return true if validation succeeded or does not apply
   */
  bool CheckScriptValidation(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill, bool execution);

  /**
   * Run applicable script cost adjustments.
   * @param pSkill Current skill processing state
   * @return false if an error occurred or the cost is unpayable
   */
  bool AdjustScriptCosts(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Run applicable script pre-actions during execution.
   * @param pSkill Current skill processing state
   * @param targets List of target states before skill NRA calculation, etc
   * @return false if an error occurred or the skill should fizzle
   */
  bool ExecuteScriptPreActions(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill,
      std::list<std::shared_ptr<channel::ActiveEntityState>> targets);

  /**
   * Run applicable script post-actions during execution.
   * @param pSkill Current skill processing state
   * @return false if an error occurred
   */
  bool ExecuteScriptPostActions(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Give a demon present to the player character as the result of a familiarity
   * adjusting skill.
   * @param client Pointer to the client connection
   * @param demonType Partner demon type
   * @param itemType Present item type
   * @param rarity Rarity level of the present being given
   * @param skillID ID of the skill that caused the present to be given
   */
  void GiveDemonPresent(const std::shared_ptr<ChannelClientConnection>& client,
                        uint32_t demonType, uint32_t itemType, int8_t rarity,
                        uint32_t skillID);

  /**
   * Fizzle a skill that has already started processing and perform any
   * necessary cleanup steps.
   * @param ctx Special execution state for the skill
   */
  void Fizzle(const std::shared_ptr<SkillExecutionContext>& ctx);

  /**
   * Notify the client that a skill has been activated.  The client will notify
   * the server when the specified charge time has elapsed for execution if
   * applicable.
   * @param pSkill Current skill processing state
   */
  void SendActivateSkill(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Notify the client that a skill is executing.
   * @param pSkill Current skill processing state
   */
  void SendExecuteSkill(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill);

  /**
   * Notify the client that a skill is immediately executing without
   * some normal prerequisite packets.
   * @param pSkill Current skill processing state
   * @param errorCode If the skill execution failed an error code will
   *  be supplied here.
   */
  void SendExecuteSkillInstant(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill,
      uint8_t errorCode);

  /**
   * Notify the client that a skill is complete.
   * @param activated Pointer to the activated ability instance
   * @param mode Preset complete mode
   *  0) Completed normally
   *  1) Cancelled
   */
  void SendCompleteSkill(std::shared_ptr<objects::ActivatedAbility> activated,
                         uint8_t mode);

  /**
   * Determine the summon speed for the supplied client and the targeted
   * demon in the COMP.
   * @param pSkill Current skill processing state
   * @param client Pointer to the client connection
   * @return Summon charge time in milliseconds, 0 on error
   */
  uint32_t GetSummonSpeed(
      const std::shared_ptr<channel::ProcessingSkill>& pSkill,
      const std::shared_ptr<ChannelClientConnection>& client);

  /**
   * Determine if the supplied skill data will involve talk results either
   * from a negotiation skill or a skill with negotiation damage
   * @param skillData Pointer to the skill's definition
   * @param primaryOnly If true, only skills that have a negotiation action
   *  type will return true. If false, skills that cause negotiation damage
   *  will also return true.
   * @return true if the skill is will involve talk results
   */
  bool IsTalkSkill(const std::shared_ptr<objects::MiSkillData>& skillData,
                   bool primaryOnly);

  /**
   * Determine if invincibility frames are enabled for the server to
   * adjust skill timing
   * @return true if I-frames are enabled, false if they are not
   */
  bool IFramesEnabled();

  /**
   * Check if an entity is responsible for this spawn and if the creation of the
   * spawn should be denied.
   * @param responsibleEntity Will be set to the UUID of the responsible entity
   * or NULL if there is none.
   * @param managedCountForEntity Output for the current number of spawns
   * managed by this entity.
   * @param activated The skill activation.
   * @param client Client connection.
   * @param zone Zone the skill was casted in.
   * @param source Entity that wishes to create the spawn.
   * @returns true if the spawn may be created; false otherwise
   * @note If this method returns false, an error has already been sent.
   */
  bool CheckResponsibility(
      libobjgen::UUID& responsibleEntity, int32_t& managedCountForEntity,
      const std::shared_ptr<objects::ActivatedAbility>& activated,
      const std::shared_ptr<ChannelClientConnection>& client,
      const std::shared_ptr<Zone>& zone,
      const std::shared_ptr<ActiveEntityState>& source);

  /// Pointer to the channel server
  std::weak_ptr<ChannelServer> mServer;

  /// Map of skill function IDs mapped to manager functions that execute
  /// in place of the normal skill handler.
  std::unordered_map<
      uint16_t,
      std::function<bool(SkillManager&,
                         const std::shared_ptr<objects::ActivatedAbility>&,
                         const std::shared_ptr<SkillExecutionContext>&,
                         const std::shared_ptr<ChannelClientConnection>&)>>
      mSkillFunctions;

  /// Map of skill function IDs mapped to manager functions that perform
  /// special actions after the normal skill processing completes.
  std::unordered_map<
      uint16_t,
      std::function<bool(SkillManager&,
                         const std::shared_ptr<objects::ActivatedAbility>&,
                         const std::shared_ptr<SkillExecutionContext>&,
                         const std::shared_ptr<ChannelClientConnection>&)>>
      mSkillEffectFunctions;

  /// Map of skill function IDs to prepared scripts to be executed during
  /// one of several points during skill processing.
  std::unordered_map<uint16_t, std::shared_ptr<libcomp::ScriptEngine>>
      mSkillLogicScripts;

  /// Map of skill function IDs to script specified settings signifying
  /// special logic to apply to the associated skills.
  std::unordered_map<uint16_t, std::shared_ptr<SkillLogicSettings>>
      mSkillLogicSettings;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_SKILLMANAGER_H
