/**
 * @file server/channel/src/CharacterState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a player character on the channel.
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

#ifndef SERVER_CHANNEL_SRC_CHARACTERSTATE_H
#define SERVER_CHANNEL_SRC_CHARACTERSTATE_H

// objects Includes
#include <Character.h>

// channel Includes
#include <ActiveEntityState.h>

namespace libcomp
{
class DefinitionManager;
}

namespace objects
{
class Item;
class MiSpecialConditionData;
}

namespace channel
{

/**
 * Contains the state of a player character on the channel.
 */
class CharacterState : public ActiveEntityStateImp<objects::Character>
{
public:
    /**
     * Create a new character state.
     */
    CharacterState();

    /**
     * Clean up the character state.
     */
    virtual ~CharacterState() { }

    /**
     * Get the tokusei effects IDs from the character's current equipment
     * @return Tokusei effect IDs from the character's current equipment
     */
    std::list<int32_t> GetEquipmentTokuseiIDs() const;

    /**
     * Get the conditional tokusei effect definitions from the character's
     * current equipment
     * @return Conditional tokusei effect definitions from the character's
     *  current equipment
     */
    std::list<std::shared_ptr<
        objects::MiSpecialConditionData>> GetConditionalTokusei() const;

    /**
     * Get the current number of complete quests that grant bonuses
     * @return Bonus granting complete quest count
     */
    uint32_t GetQuestBonusCount() const;

    /**
     * Get the set of tokusei effect IDs granted by quest completion
     * @return List of tokusei effect IDs
     */
    std::list<int32_t> GetQuestBonusTokuseiIDs() const;

    /**
     * Determine the tokusei effects gained for the character based upon
     * their current equipment
     * @param definitionManager Pointer to the definition manager to use
     *  for determining equipment effects
     */
    void RecalcEquipState(libcomp::DefinitionManager* definitionManager);

    /**
     * Determine the quest bonus effects gained for the character based
     * on the number of completed quests. If a quest is being completed
     * this function should be used with the optional secondary parameter
     * supplied.
     * @param definitionManager Pointer to the definition manager to use for
     *  determining quest bonus effects
     * @param completedQuestID Optional ID of a quest that should be completed
     *  before recalculating. If not supplied, the entire quest completion
     *  list should be used to recalculate
     * @return true if the recalculation resulted in more quest bonuses being
     *  applied or increased
     */
    bool UpdateQuestState(libcomp::DefinitionManager* definitionManager,
        uint32_t completedQuestID = 0);

    /**
     * Determine the character's current expertise rank for the
     * specified ID. This includes chain expertise calculations.
     * @param definitionManager Pointer to the definition manager to use
     *  for determining chain expertise relationships
     * @param expertiseID ID of the expertise to calculate
     * @return Rank of the supplied expertise ID
     */
    uint8_t GetExpertiseRank(libcomp::DefinitionManager* definitionManager,
        uint32_t expertiseID);

    virtual bool RecalcDisabledSkills(
        libcomp::DefinitionManager* definitionManager);

protected:
    virtual void BaseStatsCalculated(
        libcomp::DefinitionManager* definitionManager,
        std::shared_ptr<objects::CalculatedEntityState> calcState,
        libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments);

private:
    /**
     * Calculate and update item fuse bonuses for the supplied equipment
     * @param definitionManager Pointer to the definition manager to use
     *  for determining equipment info
     * @param equipment Pointer to the equipment to calculate bonuses for
     */
    void AdjustFuseBonus(libcomp::DefinitionManager* definitionManager,
        std::shared_ptr<objects::Item> equipment);

    /// Tokusei effect IDs available due to the character's current
    /// equipment. Sources contain mod slots, equipment sets and
    /// enchantments. Can contain duplicates.
    std::list<int32_t> mEquipmentTokuseiIDs;

    /// List of tokusei conditions that apply based upon the state
    /// of the character other than base stats
    std::list<std::shared_ptr<
        objects::MiSpecialConditionData>> mConditionalTokusei;

    /// List of tokusei conditions that apply based upon the state
    /// of the character's base stats
    std::list<std::shared_ptr<
        objects::MiSpecialConditionData>> mStatConditionalTokusei;

    /// Tokusei effect IDs available due to the number of quests completed
    std::list<int32_t> mQuestBonusTokuseiIDs;

    /// Quick access count representing the number of completed quests
    /// that can affect bonuses
    uint32_t mQuestBonusCount;

    /// Precalculated equipment fuse bonuses that are applied after base
    /// stats have been calculated (since they are all numeric adjustments)
    libcomp::EnumMap<CorrectTbl, int16_t> mEquipFuseBonuses;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHARACTERSTATE_H
