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

#ifndef SERVER_CHANNEL_SRC_CHARACTERSTATE_H
#define SERVER_CHANNEL_SRC_CHARACTERSTATE_H

// objects Includes
#include <ActiveEntityState.h>
#include <Character.h>

namespace libcomp
{
class DefinitionManager;
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
     * Get the equipment set IDs of the character's current equipment
     * @return Equipment set IDs of the character's current equipment
     */
    std::set<uint32_t> GetEquippedSets() const;

    /**
     * Determine and set the equipment set IDs of the character's
     * current equipment
     * @param definitionManager Pointer to the definition manager to use
     *  when determining which equipment sets are equipped
     */
    void SetEquippedSets(libcomp::DefinitionManager* definitionManager);

private:
    /// Equipment set IDs of the character's current equipment
    std::set<uint32_t> mEquippedSets;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHARACTERSTATE_H
