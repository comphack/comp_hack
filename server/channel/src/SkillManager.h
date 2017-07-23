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
class MiSkillData;
}

namespace channel
{

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
     * Execute a normal ability, not handled by a special handler.
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
     * @return true if the skill processed successfully, false otherwise
     */
    bool ProcessSkillResult(std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Calculate skill damage or healing using the correct formula
     * @param source Pointer to the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     * @param targets List target results associated to the skill
     * @param skillData Pointer to skill definition
     * @return true if the calculation succeeded, false otherwise
     */
    bool CalculateDamage(const std::shared_ptr<ActiveEntityState>& source,
        const std::shared_ptr<objects::ActivatedAbility>& activated,
        std::list<SkillTargetResult>& targets,
        const std::shared_ptr<objects::MiSkillData>& skillData);

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
     * Execute the "equip item" ability.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool EquipItem(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the "summon demon" ability.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool SummonDemon(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the "store demon" ability.
     * @param client Pointer to the client connection that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool StoreDemon(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the ability "Traesto" which returns the user to their homepoint.
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
