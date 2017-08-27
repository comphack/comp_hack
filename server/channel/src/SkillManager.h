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
class ItemDrop;
class MiSkillData;
class Spawn;
}

namespace channel
{

class ProcessingSkill;
class SkillTargetResult;
class ActiveEntityState;
class ChannelServer;

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
     * @param client Pointer to the client connection that activated the skill
     * @param skillID ID of the skill to activate
     * @param sourceEntityID ID of the entity that activated the skill
     * @param targetObjectID ID of the object being targeted by the skill
     * @return true if the skill was activated successfully, false otherwise
     */
    bool ActivateSkill(const std::shared_ptr<ChannelClientConnection> client,
        uint32_t skillID, int32_t sourceEntityID, int64_t targetObjectID);

    /**
     * Execute the skill of a character or demon.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activationID ID of the activated ability instance
     * @param targetObjectID ID of the object being targeted by the skill
     * @return true if the skill was executed successfully, false otherwise
     */
    bool ExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, uint8_t activationID, int64_t targetObjectID);

    /**
     * Cancel the activated skill of a character or demon.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activationID ID of the activated ability instance
     * @return true if the skill was cancelled successfully, false otherwise
     */
    bool CancelSkill(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, uint8_t activationID);

    /**
     * Notify the client that a skill failed activation or execution.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param skillID ID of the skill to activate
     */
    void SendFailure(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, uint32_t skillID);

private:
    /**
     * Pay any costs related to and execute the skill of a character or demon.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceState Pointer to the state of the source entity
     * @param activated Pointer to the activated ability instance
     * @return true if the skill was executed successfully, false otherwise
     */
    bool ExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<ActiveEntityState> sourceState,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute a normal skill, not handled by a special handler.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool ExecuteNormalSkill(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Process the results of a normal skill. If the skill involves a projectile
     * this will be scheduled to execute after skill execution, otherwise it will
     * execute immediately.
     * @param activated Pointer to the activated ability instance
     * @param applyStatusEffects false if the status effects are handled
     *  in a special way outside of normal method
     * @return true if the skill processed successfully, false otherwise
     */
    bool ProcessSkillResult(std::shared_ptr<objects::ActivatedAbility> activated,
        bool applyStatusEffects = true);

    /**
     * Handle all logic related to kills that occurred from a skill's execution.
     * @param source Pointer to the source of the skill
     * @param zone Pointer to the zone where the skill was executed
     * @param killed Set of entities killed by the skill's execution
     */
    void HandleKills(const std::shared_ptr<ActiveEntityState> source,
        const std::shared_ptr<Zone> zone,
        const std::set<std::shared_ptr<ActiveEntityState>> killed);

    /**
     * Handle the outcome of a negotation ending from a skill's execution
     * including things like creating demon eggs and gift boxes or running away.
     * @param source Pointer to the source of the skill
     * @param zone Pointer to the zone where the skill was executed
     * @param talkDone List of entities that have finished negotiation with
     *  the skill source and their respective outcomes related to respons flags
     */
    void HandleNegotiations(const std::shared_ptr<ActiveEntityState> source,
        const std::shared_ptr<Zone> zone,
        const std::list<std::pair<std::shared_ptr<ActiveEntityState>,
        uint8_t>> talkDone);

    /**
     * Toggle a switch skill, not handled by a special handler.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool ToggleSwitchSkill(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Calculate skill damage or healing using the correct formula
     * @param source Pointer to the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     * @param targets List target results associated to the skill
     * @param skill Current skill processing state
     * @return true if the calculation succeeded, false otherwise
     */
    bool CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
        const std::shared_ptr<objects::ActivatedAbility>& activated,
        std::list<SkillTargetResult>& targets, ProcessingSkill& skill);

    /**
     * Calculate skill damage or healing using the default formula
     * @param mod Base modifier damage value
     * @param damageType Type of damage being dealt
     * @param off Offensive value of the source
     * @param def Defensive value of the target
     * @param resist Resistence to the skill affinity
     * @param boost Boost level to apply to the skill affinity
     * @param critLevel Critical level adjustment. Valid values are:
     *  0) No critical adjustment
     *  1) Critical hit
     *  2) Limit break
     * @return Calculated damage or healing
     */
    int32_t CalculateDamage_Normal(uint16_t mod, uint8_t& damageType,
        uint16_t off, uint16_t def, float resist, float boost, uint8_t critLevel);

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
        int16_t current);

    /**
     * Calculate skill damage or healing based on a max relative value
     * @param mod Base modifier damage value
     * @param damageType Type of damage being dealt
     * @param max Maximum value to calculate relative damage to
     * @return Calculated damage or healing
     */
    int32_t CalculateDamage_MaxPercent(uint16_t mod, uint8_t& damageType,
        int16_t max);

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
     * @param activated Pointer to the activated ability instance
     * @param skillData Pointer to the skill data
     */
    void FinalizeSkillExecution(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated,
        std::shared_ptr<objects::MiSkillData> skillData);

    /**
     * Placeholder function for a skill actually handled outside of the
     * SkillManager.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool SpecialSkill(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the "equip item" skill.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool EquipItem(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute a familiarity boosting skill.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool FamiliarityUp(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute a familiarity boosting skill from an item. These work slightly
     * different from the standard skill ones as they can adjust a delta percent
     * instead of a fixed value.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool FamiliarityUpItem(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the "Mooch" skill.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool Mooch(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the "summon demon" skill.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool SummonDemon(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the "store demon" skill.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool StoreDemon(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the skill "Traesto" which returns the user to their homepoint.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool Traesto(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Notify the client that a skill needs to charge.  The client will notify
     * the server when the specified charge time has elapsed for execution.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    void SendChargeSkill(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Notify the client that a skill is executing.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     * @param skillData Pointer to the skill data
     */
    void SendExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated,
        std::shared_ptr<objects::MiSkillData> skillData);

    /**
     * Notify the client that a skill is complete.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     * @param cancelled true if the skill was cancelled
     */
    void SendCompleteSkill(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated,
        bool cancelled);

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;

    /// List of skill function IDs mapped to manager functions.
    std::unordered_map<uint16_t, std::function<bool(SkillManager&,
        const std::shared_ptr<channel::ChannelClientConnection>&,
        std::shared_ptr<objects::ActivatedAbility>&)>> mSkillFunctions;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_SKILLMANAGER_H
