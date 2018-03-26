/**
 * @file server/channel/src/TokuseiManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages tokusei specific logic for the server and validates
 *  the definitions read at run time.
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

#ifndef SERVER_CHANNEL_SRC_TOKUSEIMANAGER_H
#define SERVER_CHANNEL_SRC_TOKUSEIMANAGER_H

// object Includes
#include <TokuseiAspect.h>
#include <TokuseiCondition.h>
#include <TokuseiSkillCondition.h>

// channel Includes
#include "ActiveEntityState.h"

namespace objects
{
class ClientCostAdjustment;
class Party;
class Tokusei;
class TokuseiAttributes;
}

typedef objects::TokuseiAspect::Type_t TokuseiAspectType;
typedef objects::TokuseiCondition::Type_t TokuseiConditionType;
typedef objects::TokuseiSkillCondition
    ::SkillConditionType_t TokuseiSkillConditionType;

namespace channel
{

class ChannelClientConnection;
class ChannelServer;
class WorldClock;
class WorldClockTime;

/**
 * Manages tokusei specific logic for the server and validates
 * the definitions read at run time.
 */
class TokuseiManager
{
public:
    /**
     * Create a new TokuseiManager
     * @param server Pointer back to the channel server this belongs to.
     */
    TokuseiManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the TokuseiManager
     */
    virtual ~TokuseiManager();

    /**
     * Initialize the manager and validate the tokusei definitions loaded.
     * @return false if any errors were encountered
     */
    bool Initialize();

    /**
     * Recalculate the tokusei effects on the supplied entity and any related entities
     * if any of the specified changes are triggers on the entity.
     * @param eState Pointer to the entity that has changed
     * @param changes Changes that could trigger a tokusei recalculation
     * @return Map of entity IDs to a true value if they have had their stats recalculated
     *  or false if only their tokusei sets and triggers were updated
     */
    std::unordered_map<int32_t, bool> Recalculate(const std::shared_ptr<ActiveEntityState>& eState,
        std::set<TokuseiConditionType> changes);

    /**
     * Recalculate the tokusei effects on the supplied entity and any related entities.
     * @param eState Pointer to the primary entity to recalculate
     * @param recalcStats false if the effect tokusei should be determined but the entities
     *  should not have their stats recalculated, true if both should occur
     * @param ignoreStateRecalc Set of entity IDs to ignore when recalculating stats
     * @return Map of entity IDs to a true value if they have had their stats recalculated
     *  or false if only their tokusei sets and triggers were updated
     */
    std::unordered_map<int32_t, bool> Recalculate(const std::shared_ptr<ActiveEntityState>& eState,
        bool recalcStats = false, std::set<int32_t> ignoreStatRecalc = {});

    /**
     * Recalculate the tokusei effects on the supplied entities.
     * @param entities List of pointers to the entities to recalculate
     * @param recalcStats false if the effect tokusei should be determined but the entities
     *  should not have their stats recalculated, true if both should occur
     * @param ignoreStateRecalc Set of entity IDs to ignore when recalculating stats
     * @return Map of entity IDs to a true value if they have had their stats recalculated
     *  or false if only their tokusei sets and triggers were updated
     */
    std::unordered_map<int32_t, bool> Recalculate(const std::list<std::shared_ptr<
        ActiveEntityState>>& entities, bool recalcStats = false, std::set<int32_t> ignoreStatRecalc = {});

    /**
     * Recalculate the tokusei effects for all entities in a party on the channel.
     * @param party Pointer to the party which will have its entities recalculated
     * @return Map of entity IDs to a true value if they have had their stats recalculated
     *  or false if only their tokusei sets and triggers were updated
     */
    std::unordered_map<int32_t, bool> RecalculateParty(const std::shared_ptr<objects::Party>& party);

    /**
     * Get all entities that could be affected by any other tokusei effect from another
     * entity in the list, starting with the supplied entity.
     * @param eState Pointer to the primary entity to add to the list
     * @return List of entities that could affect one another via a tokusei
     */
    std::list<std::shared_ptr<ActiveEntityState>> GetAllTokuseiEntities(
        const std::shared_ptr<ActiveEntityState>& eState);

    /**
     * Get all tokusei originating from the supplied entity, active or not.
     * @param eState Pointer to the entity
     * @return List of tokusei originating from the supplied entity, can contain
     *  duplicates
     */
    std::list<std::shared_ptr<objects::Tokusei>> GetDirectTokusei(
        const std::shared_ptr<ActiveEntityState>& eState);

    /**
     * Evaluate all conditions on a tokusei to determine if it should be active.
     * @param eState Pointer to the tokusei source to evaluate the conditions for
     * @param tokusei Pointer to the tokusei to evaluate the conditions for
     * @return true if the condition set evaluates to true
     */
    bool EvaluateTokuseiConditions(const std::shared_ptr<ActiveEntityState>& eState,
        const std::shared_ptr<objects::Tokusei>& tokusei);

    /**
     * Evaluate a condition from a tokusei.
     * @param eState Pointer to the tokusei source to evaluate the conditions for
     * @param tokuseiID ID of the source tokusei effect
     * @param condition Pointer to the condition to evaluate
     * @return true if the condition evaluates to true
     */
    bool EvaluateTokuseiCondition(const std::shared_ptr<ActiveEntityState>& eState,
        int32_t tokuseiID, const std::shared_ptr<objects::TokuseiCondition>& condition);

    /**
     * Calculate the value of an attribute driven tokusei value.
     * @param eState Pointer to the tokusei source
     * @param value Default value to use for the calculation
     * @param base Base value used for specific calculation types
     * @param attributes Pointer to the attribute definitions to use when calculating
     *  the value
     * @param calcState Override CalculatedEntityState to use instead of the entity's default
     * @return Calculated attribute value
     */
    static double CalculateAttributeValue(ActiveEntityState* eState, int32_t value, int32_t base,
        const std::shared_ptr<objects::TokuseiAttributes>& attributes,
        std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr);

    /**
     * Calculate the sum of all instances of a specific aspect value on the supplied entity.
     * @param eState Pointer to the tokusei source
     * @param type Aspect type to calculate the sum of
     * @param calcState Override CalculatedEntityState to use instead of the entity's default
     * @return Calculated aspect value sum
     */
    double GetAspectSum(const std::shared_ptr<ActiveEntityState>& eState,
        TokuseiAspectType type, std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr);

    /**
     * Calculate the sum of all instances of a specific aspect's modifier values keyed on
     * shared aspect value on the supplied entity.
     * @param eState Pointer to the tokusei source
     * @param type Aspect type to gather values from
     * @param calcState Override CalculatedEntityState to use instead of the entity's default
     * @return Map of calculated aspect modifier value sums by aspect value
     */
    std::unordered_map<int32_t, double> GetAspectMap(const std::shared_ptr<ActiveEntityState>& eState,
        TokuseiAspectType type, std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr);

    /**
     * Calculate the sum of all instances of a specific aspect's modifier values keyed on
     * shared aspect value on the supplied entity.
     * @param eState Pointer to the tokusei source
     * @param type Aspect type to gather values from
     * @param validKeys Set of keys to keep from the overall set
     * @param calcState Override CalculatedEntityState to use instead of the entity's default
     * @return Map of calculated aspect modifier value sums by aspect value with a default value
     *  of zero
     */
    std::unordered_map<int32_t, double> GetAspectMap(const std::shared_ptr<ActiveEntityState>& eState,
        TokuseiAspectType type, std::set<int32_t> validKeys, std::shared_ptr<
        objects::CalculatedEntityState> calcState = nullptr);

    /**
     * Get the list of all aspect values on the supplied entity and type.
     * @param eState Pointer to the tokusei source
     * @param type Aspect type to gather values from
     * @param calcState Override CalculatedEntityState to use instead of the entity's default
     * @return List of all aspect values of the specified type
     */
    std::list<double> GetAspectValueList(const std::shared_ptr<ActiveEntityState>& eState,
        TokuseiAspectType type, std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr);

    /**
     * Recalculate all time restricted tokusei based on the current world time
     * @param clock World clock set to the current time
     */
    void RecalcTimedTokusei(WorldClock& clock);

    /**
     * Unregister the world CID of a character that may have had time restricted
     * tokusei associated to one or more entity. Call this any time a player
     * logs off just in case.
     * @param worldCID World CID associated to a character
     */
    void RemoveTrackingEntities(int32_t worldCID);

    /**
     * Send skill cost adjustments from tokusei for the specified entity to the
     * client
     * @param entityID ID of the entity with cost adjustments
     * @param client Pointer to the client connection
     */
    void SendCostAdjustments(int32_t entityID, const std::shared_ptr<
        ChannelClientConnection>& client);

private:
    /**
     * Recalculate skill cost adjustments from tokusei for the specified
     * entity. If the entity's data has already been sent to the client,
     * the new costs will be sent too.
     * @param eState Pointer to the entity to recalculate
     */
    void RecalcCostAdjustments(const std::shared_ptr<ActiveEntityState>& eState);

    /**
     * Send skill cost adjustments from tokusei for the specified entity to the
     * client
     * @param entityID ID of the entity with cost adjustments
     * @param adjustments List of adjustments to send to the client
     * @param client Pointer to the client connection
     */
    void SendCostAdjustments(int32_t entityID,
        std::list<std::shared_ptr<objects::ClientCostAdjustment>>& adjustments,
        const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Gather all timed tokusei conditions and register their time representations
     * with the manager and server
     * @param tokusei Pointer to the tokusei definition
     * @return true if the tokusei was registered successfully, false if an error
     *  occurred
     */
    bool GatherTimedTokusei(const std::shared_ptr<objects::Tokusei>& tokusei);

    /**
     * Convert a tokusei condition to a world clock time representation. Calling
     * this function for multiple conditions will combine the times into a complex
     * time representation.
     * @param condition Pointer to the tokusei condition
     * @param time Output parameter to store the time in
     * @return true the time was adjusted correctly, false if an error occurred
     */
    bool BuildWorldClockTime(std::shared_ptr<objects::TokuseiCondition> condition,
        WorldClockTime& time);

    /**
     * Compare the supplied value and condition value.
     * @param value LHS value to compare
     * @param condition Tokusei condition to use the comparator and RHS value from
     * @param numericCompare If false and a numeric comparator type is on the condition
     *  this will always return false
     * @return false if the condition does not evaluate to true
     */
    bool Compare(int32_t value, std::shared_ptr<objects::TokuseiCondition> condition,
        bool numericCompare) const;

    /**
     * Compare the supplied two values.
     * @param value1 LHS value to compare
     * @param value2 RHS value to compare
     * @param condition Tokusei condition to use the comparator from
     * @param numericCompare If false and a numeric comparator type is on the condition
     *  this will always return false
     * @return false if the condition does not evaluate to true
     */
    bool Compare(int32_t value1, int32_t value2, std::shared_ptr<
        objects::TokuseiCondition> condition, bool numericCompare) const;

    /// Quick access mapping of constant status effect IDs to their source tokusei IDs
    std::unordered_map<uint32_t, std::set<int32_t>> mStatusEffectTokusei;

    /// Map of all tokusei effect IDs that have a time restriction to a boolean
    /// "active" indicator
    std::unordered_map<int32_t, bool> mTimedTokusei;

    /// Map of world CIDs to the set of related time restricted tokusei.
    /// This set is updated entity direct tokusei only and does not require
    /// that the effect is ultimately marked as effective.
    std::unordered_map<int32_t, std::set<int32_t>> mTimedTokuseiEntities;

    /// Set of all tokusei with at least one cost adjustment aspect
    std::set<int32_t> mCostAdjustmentTokusei;

    /// Server lock for time calculation
    std::mutex mTimeLock;

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_TOKUSEIMANAGER_H
