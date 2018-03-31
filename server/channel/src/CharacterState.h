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
     * Determine the tokusei effects gained for the character based upon
     * their current equipment
     * @param definitionManager Pointer to the definition manager to use
     *  for determining equipment effects
     */
    void RecalcEquipState(libcomp::DefinitionManager* definitionManager);

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

    virtual bool RecalcDisabledSkills(libcomp::DefinitionManager* definitionManager);

protected:
    virtual void BaseStatsCalculated(libcomp::DefinitionManager* definitionManager,
        std::shared_ptr<objects::CalculatedEntityState> calcState,
        libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments);

private:
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
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHARACTERSTATE_H
