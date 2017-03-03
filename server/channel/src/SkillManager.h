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
class EntityStats;
class EntityStateObject;
class MiSkillData;
}

namespace channel
{

class ChannelServer;

/**
 * Manager to handle Character focused actions.
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
     * @param sourceStats Pointer to the stats of the source entity
     * @param sourceState Pointer to the state of the source entity
     * @param activated Pointer to the activated ability instance
     * @param targetObjectID ID of the object being targeted by the skill
     * @return true if the skill was executed successfully, false otherwise
     */
    bool ExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
        std::shared_ptr<objects::EntityStats> sourceStats,
        std::shared_ptr<objects::EntityStateObject> sourceState,
        std::shared_ptr<objects::ActivatedAbility> activated, int64_t targetObjectID);

    /**
     * Execute a normal ability, not handled by a special handler.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     * @param hpCost Amount of HP spent on the skill's execution
     * @param mpCost Amount of MP spent on the skill's execution
     */
    bool ExecuteNormalSkill(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
        uint32_t hpCost, uint32_t mpCost);

    /**
     * Execute post execution steps like notifying the client that the skill
     * has executed and updating any related expertises.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     * @param skillData Pointer to the skill data
     * @param hpCost Amount of HP spent on the skill's execution
     * @param mpCost Amount of MP spent on the skill's execution
     */
    void FinalizeSkillExecution(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
        std::shared_ptr<objects::MiSkillData> skillData, uint32_t hpCost, uint32_t mpCost);

    /**
     * Execute the "equip item" ability.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool EquipItem(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the "summon demon" ability.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool SummonDemon(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Execute the "store demon" ability.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    bool StoreDemon(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Notify the client that a skill needs to charge.  The client will notify
     * the server when the specified charge time has elapsed for execution.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    void SendChargeSkill(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Notify the client that a skill is executing.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     * @param skillData Pointer to the skill data
     * @param hpCost Amount of HP spent on the skill's execution
     * @param mpCost Amount of MP spent on the skill's execution
     */
    void SendExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated,
        std::shared_ptr<objects::MiSkillData> skillData, uint32_t hpCost, uint32_t mpCost);

    /**
     * Notify the client that a skill is complete.
     * @param client Pointer to the client connection that activated the skill
     * @param sourceEntityID ID of the entity that activated the skill
     * @param activated Pointer to the activated ability instance
     */
    void SendCompleteSkill(const std::shared_ptr<ChannelClientConnection> client,
        int32_t sourceEntityID, std::shared_ptr<objects::ActivatedAbility> activated);

    /**
     * Get the activated ability source stats and state pointers.
     * @param state Pointer to the client state containing the entities to check
     * @param sourceEntityID ID of the entity that activated the skill
     * @param sourceStats Reference to the stats pointer to set on success
     * @param sourceState Reference to the state pointer to set on success
     * @return true if the entity was found, false otherwise
     */
    bool GetSkillSource(channel::ClientState *state,
        int32_t sourceEntityID, std::shared_ptr<objects::EntityStats>& sourceStats,
        std::shared_ptr<objects::EntityStateObject>& sourceState);

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_SKILLMANAGER_H
