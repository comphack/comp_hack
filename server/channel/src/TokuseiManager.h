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

// channel Includes
#include "ActiveEntityState.h"

namespace objects
{
class Party;
class Tokusei;
class TokuseiAttributes;
}

typedef objects::TokuseiAspect::Type_t TokuseiAspectType;
typedef objects::TokuseiCondition::Type_t TokuseiConditionType;

namespace channel
{

class ChannelServer;

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
     * @param condition Pointer to the condition to evaluate
     * @return true if the condition evaluates to true
     */
    bool EvaluateTokuseiCondition(const std::shared_ptr<ActiveEntityState>& eState,
        const std::shared_ptr<objects::TokuseiCondition>& condition);

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

private:
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

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_TOKUSEIMANAGER_H
