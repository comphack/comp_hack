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

#ifndef SERVER_CHANNEL_SRC_SKILLMANAGER_H
#define SERVER_CHANNEL_SRC_SKILLMANAGER_H

// channel Includes
#include "ChannelClientConnection.h"

namespace objects
{
class ActivatedAbility;
class Enemy;
class ItemDrop;
class MiSkillData;
class Spawn;
class TokuseiSkillCondition;
}

namespace channel
{

class ProcessingSkill;
class SkillTargetResult;

class ActiveEntityState;
class AIState;
class ChannelServer;

/**
 * Container for skill execution contextual parameters.
 */
class SkillExecutionContext
{
friend class SkillManager;

public:
    /// true if the skill should not cost anything, false if normal costs
    /// apply
    bool FreeCast = false;

protected:
    /* The following options are for internal processing use only */

    /// true if status effects will be processed normally, false if they
    /// have been handled elsewhere
    bool ApplyStatusEffects = true;

    /// Designates the skill that is being processed
    std::shared_ptr<channel::ProcessingSkill> Skill;

    /// Designates a skill that is being countered
    std::shared_ptr<channel::ProcessingSkill> CounteredSkill;

    /// List of skills that are countering the current skill context
    std::list<std::shared_ptr<channel::ProcessingSkill>> CounteringSkills;
};

/**
 * Manager to handle skill focused actions.
 */
class SkillManager
{
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
     * Activate the skill of a character or demon. The client will have the
     * charge information sent to it if there is a charge time, otherwise
     * it will execute automatically.
     * @param source Pointer of the entity that activated the skill
     * @param skillID ID of the skill to activate
     * @param targetObjectID ID of the object being targeted by the skill
     * @param ctx Special execution state for the skill (ex: free cast, counter)
     * @return true if the skill was activated successfully, false otherwise
     */
    bool ActivateSkill(const std::shared_ptr<ActiveEntityState> source,
        uint32_t skillID, int64_t targetObjectID,
        std::shared_ptr<SkillExecutionContext> ctx = 0);

    /**
     * Execute the skill of a character or demon.
     * @param source Pointer of the entity that is executing the skill
     * @param activationID ID of the activated ability instance
     * @param targetObjectID ID of the object being targeted by the skill
     * @param ctx Special execution state for the skill (ex: free cast, counter)
     * @return true if the skill was executed successfully, false otherwise
     */
    bool ExecuteSkill(const std::shared_ptr<ActiveEntityState> source,
        uint8_t activationID, int64_t targetObjectID,
        std::shared_ptr<SkillExecutionContext> ctx = 0);

    /**
     * Cancel the activated skill of a character or demon.
     * @param source Pointer of the entity that is cancelling the skill
     * @param activationID ID of the activated ability instance
     * @return true if the skill was cancelled successfully, false otherwise
     */
    bool CancelSkill(const std::shared_ptr<ActiveEntityState> source,
        uint8_t activationID);

    /**
     * Notify the client that a skill failed activation or execution.
     * @param source Pointer of the entity that the skill failed for
     * @param skillID ID of the skill to activate
     * @param client Optional parameter, pointer to the client connection
     *  that should be sent the failure instead of the entire zone
     */
    void SendFailure(const std::shared_ptr<ActiveEntityState> source, uint32_t skillID,
        const std::shared_ptr<ChannelClientConnection> client = nullptr);

private:
    /**
     * Pay any costs related to and execute the skill of a character or demon.
     * @param source Pointer to the state of the source entity
     * @param activated Pointer to the activated ability instance
     * @param client Pointer to the client connection, can be null if not coming
     *  from a player entity
     * @param ctx Special execution state for the skill
     * @return true if the skill was executed successfully, false otherwise
     */
    bool ExecuteSkill(std::shared_ptr<ActiveEntityState> source,
        std::shared_ptr<objects::ActivatedAbility> activated,
        const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<SkillExecutionContext> ctx);

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
     */
    void ProcessSkillResultFinal(const std::shared_ptr<channel::ProcessingSkill>& pSkill);

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
     * @param pSkill Current skill processing state
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
        const std::shared_ptr<ProcessingSkill>& pSkill, bool isTarget,
        const std::shared_ptr<ActiveEntityState>& otherState);

    /**
     * Evaluate a TokuseiSkillCondition relative to the current skill and
     * supplied entities.
     * @param eState Pointer to the entity the effect applies to
     * @param condition Pointer to the condition definition
     * @param pSkill Current skill processing state
     * @param otherState Pointer to the source entity if a target is being
     *  calculated or the target in question if the source is being calculated.
     *  If the skill is still being checked validity, supplying no entity
     *  for this while calculating against the source is valid.
     * @return true if the condition evaluates to true
     */
    bool EvaluateTokuseiSkillCondition(const std::shared_ptr<ActiveEntityState>& eState,
        const std::shared_ptr<objects::TokuseiSkillCondition>& condition,
        const std::shared_ptr<ProcessingSkill>& pSkill,
        const std::shared_ptr<ActiveEntityState>& otherState);

    /**
     * Calculate and set the offense value of a damage dealing skill.
     * @param source Pointer to the state of the source entity
     * @param target Pointer to the state of the target entity
     * @param skill Current skill processing state
     * @return Calculated offsense value of the skill
     */
    uint16_t CalculateOffenseValue(const std::shared_ptr<ActiveEntityState>& source,
        const std::shared_ptr<ActiveEntityState>& target, ProcessingSkill& skill);

    /**
     * Determine how each targeted entity reacts to being hit by a skill. This
     * assumes NRA has already adjusted who will is a target.
     * @param source Pointer to the state of the source entity
     * @param pSkill Current skill processing state
     */
    void CheckSkillHits(const std::shared_ptr<ActiveEntityState>& source,
        const std::shared_ptr<channel::ProcessingSkill>& pSkill);

    /**
     * Execute or cancel the guard skill currently being used by the
     * supplied target from being hit by the source entity
     * @param source Pointer to the state of the source entity
     * @param target Pointer ot the stare of the target entity
     * @param pSkill Skill processing state of the skill being guarded
     */
    void HandleGuard(const std::shared_ptr<ActiveEntityState>& source,
        SkillTargetResult& target, const std::shared_ptr<
        channel::ProcessingSkill>& pSkill);

    /**
     * Execute or cancel the counter skill currently being used by the
     * supplied target from being hit by the source entity
     * @param source Pointer to the state of the source entity
     * @param target Pointer ot the stare of the target entity
     * @param pSkill Skill processing state of the skill being counterd
     */
    void HandleCounter(const std::shared_ptr<ActiveEntityState>& source,
        SkillTargetResult& target, const std::shared_ptr<
        channel::ProcessingSkill>& pSkill);

    /**
     * Execute or cancel the dodge skill currently being used by the
     * supplied target from being hit by the source entity
     * @param source Pointer to the state of the source entity
     * @param target Pointer ot the stare of the target entity
     * @param pSkill Skill processing state of the skill being dodged
     */
    void HandleDodge(const std::shared_ptr<ActiveEntityState>& source,
        SkillTargetResult& target, const std::shared_ptr<
        channel::ProcessingSkill>& pSkill);

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
     * Handle the outcome of a negotation ending from a skill's execution
     * including things like creating demon eggs and gift boxes or running away.
     * @param source Pointer to the source of the skill
     * @param zone Pointer to the zone where the skill was executed
     * @param talkDone List of entities that have finished negotiation with
     *  the skill source and their respective outcomes related to respons flags
     */
    void HandleNegotiations(const std::shared_ptr<ActiveEntityState> source,
        const std::shared_ptr<Zone>& zone,
        const std::list<std::pair<std::shared_ptr<ActiveEntityState>,
        uint8_t>> talkDone);

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
     * @param skill Current skill processing state
     * @return true if the calculation succeeded, false otherwise
     */
    bool CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
        ProcessingSkill& skill);

    /**
     * Calculate skill damage or healing using the default formula
     * @param source Pointer to the entity that activated the skill
     * @param target Pointer to the entity that will receive damage
     * @param skill Current skill processing state
     * @param mod Base modifier damage value
     * @param damageType Type of damage being dealt
     * @param resist Resistence to the skill affinity
     * @param critLevel Critical level adjustment. Valid values are:
     *  0) No critical adjustment
     *  1) Critical hit
     *  2) Limit break
     * @param isHeal true if healing "damage" should be applied instead
     * @return Calculated damage or healing
     */
    int32_t CalculateDamage_Normal(const std::shared_ptr<ActiveEntityState>& source,
        SkillTargetResult& target, ProcessingSkill& skill, uint16_t mod,
        uint8_t& damageType, float resist, uint8_t critLevel, bool isHeal);

    /**
     * Calculate skill damage or healing based on a static value
     * @param mod Base modifier damage value
     * @param damageType Type of damage being dealt
     * @return Calculated damage or healing
     */
    int32_t CalculateDamage_Static(uint16_t mod, uint8_t& damageType);

    /**
     * Calculate skill damage or healing based on a percent value
     * @param mod Base modifier damage value
     * @param damageType Type of damage being dealt
     * @return Calculated damage or healing
     */
    int32_t CalculateDamage_Percent(uint16_t mod, uint8_t& damageType,
        int32_t current);

    /**
     * Calculate skill damage or healing based on a max relative value
     * @param mod Base modifier damage value
     * @param damageType Type of damage being dealt
     * @param max Maximum value to calculate relative damage to
     * @return Calculated damage or healing
     */
    int32_t CalculateDamage_MaxPercent(uint16_t mod, uint8_t& damageType,
        int32_t max);

    /**
     * Get the skill target that hits the source entity or create it if it does
     * not already exist
     * @param source Pointer to the entity that activated the skill
     * @param targets List target results associated to the skill
     * @return Pointer to the skill target representing the source entity
     */
    SkillTargetResult* GetSelfTarget(const std::shared_ptr<ActiveEntityState>& source,
        std::list<SkillTargetResult>& targets);

    /**
     * Set null, reflect or absorb on the skill target
     * @param target Target to update with NRA flags
     * @param skill Current skill processing state
     * @return true if the damage was reflected, otherwise false
     */
    bool SetNRA(SkillTargetResult& target, ProcessingSkill& skill);

    /**
     * Get a random stack size for a status effect being applied
     * @param minStack Minimum size of the stack
     * @param maxStack Maximum size of the stack
     * @return Random stack size between the range or 0 if the range
     *  is invalid
     */
    uint8_t CalculateStatusEffectStack(int8_t minStack, int8_t maxStack) const;

    /**
     * Gather drops for a specific enemy spawn from its own drops, global drops
     * and demon family drops.
     * @param enemyType Type of demon to gather drops from, separated in case
     *  the enemy did not originate from a spawn
     * @param spawn Pointer to the spawn information for the enemy which may
     *  or may not exist (ex: GM created enemy)
     * @param giftMode true if the enemy's negotation gifts should be gathered
     *  instead of their normal drops
     * @return List of item drops from each different source
     */
    std::list<std::shared_ptr<objects::ItemDrop>> GetItemDrops(uint32_t enemyType,
        const std::shared_ptr<objects::Spawn>& spawn, bool giftMode = false) const;

    /**
     * Schedule the adjustment of who is a valid looter for one or more loot
     * boxes.
     * @param time Server time to schedule the adjustment for
     * @param zone Pointer to the zone the entities belong to
     * @param lootEntityIDs List of loot box IDs to adjust
     * @param worldCIDs Set of character world CIDs to add as valid looters
     */
    void ScheduleFreeLoot(uint64_t time, const std::shared_ptr<Zone>& zone,
        const std::list<int32_t>& lootEntityIDs, const std::set<int32_t>& worldCIDs);

    /**
     * Execute post execution steps like notifying the client that the skill
     * has executed and updating any related expertises.
     * @param client Pointer to the client connection that activated the skill
     * @param ctx Special execution state for the skill
     * @param activated Pointer to the activated ability instance
     * @param skillData Pointer to the skill data
     */
    void FinalizeSkillExecution(const std::shared_ptr<ChannelClientConnection> client,
        const std::shared_ptr<SkillExecutionContext>& ctx,
        std::shared_ptr<objects::ActivatedAbility> activated,
        std::shared_ptr<objects::MiSkillData> skillData);

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
     * Execute a familiarity boosting skill.
     * @param activated Pointer to the activated ability instance
     * @param ctx Special execution state for the skill
     * @param client Pointer to the client connection that activated the skill,
     *  this will always fail if it is null
     */
    bool FamiliarityUp(const std::shared_ptr<objects::ActivatedAbility>& activated,
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
    bool FamiliarityUpItem(const std::shared_ptr<objects::ActivatedAbility>& activated,
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
     * Execute the skill "Traesto" which returns the user to their homepoint.
     * @param activated Pointer to the activated ability instance
     * @param ctx Special execution state for the skill
     * @param client Pointer to the client connection that activated the skill,
     *  this will always fail if it is null
     */
    bool Traesto(const std::shared_ptr<objects::ActivatedAbility>& activated,
        const std::shared_ptr<SkillExecutionContext>& ctx,
        const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Notify the client that a skill has been activated.  The client will notify
     * the server when the specified charge time has elapsed for execution if
     * applicable.
     * @param activated Pointer to the activated ability instance
     * @param skillData Pointer to the skill data
     */
    void SendActivateSkill(std::shared_ptr<objects::ActivatedAbility> activated,
        std::shared_ptr<objects::MiSkillData> skillData);

    /**
     * Notify the client that a skill is executing.
     * @param activated Pointer to the activated ability instance
     * @param cooldownTime Server time when the skill cooldown will end
     * @param lockOutTime Server time when the skill lockout will end
     */
    void SendExecuteSkill(std::shared_ptr<objects::ActivatedAbility> activated,
        uint64_t cooldownTime, uint64_t lockOutTime);

    /**
     * Notify the client that a skill is complete.
     * @param activated Pointer to the activated ability instance
     * @param mode Preset complete mode
     *  0) Completed normally
     *  1) Cancelled
     */
    void SendCompleteSkill(std::shared_ptr<objects::ActivatedAbility> activated,
        uint8_t mode);

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;

    /// Map of skill function IDs mapped to manager functions.
    std::unordered_map<uint16_t, std::function<bool(SkillManager&,
        const std::shared_ptr<objects::ActivatedAbility>&,
        const std::shared_ptr<SkillExecutionContext>&,
        const std::shared_ptr<ChannelClientConnection>&)>> mSkillFunctions;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_SKILLMANAGER_H
