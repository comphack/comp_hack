/**
 * @file server/channel/src/CharacterState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a character on the channel.
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

#ifndef SERVER_CHANNEL_SRC_CHARACTERSTATE_H
#define SERVER_CHANNEL_SRC_CHARACTERSTATE_H

// objects Includes
#include <CharacterStateObject.h>

namespace libcomp
{
class DefinitionManager;
}

namespace channel
{

/**
 * Contains the state of a character from a game client currently connected
 * to the channel.
 */
class CharacterState : public objects::CharacterStateObject
{
public:
    /**
     * Create a new character state.
     */
    CharacterState();

    /**
     * Clean up the character state.
     */
    virtual ~CharacterState();

    /**
     * Recalculate the character's current stats, adjusted by equipment and
     * effects.
     * @param Pointer to the server's definition manager to use for calculations
     * @return true if the calculation succeeded, false if it errored
     */
    bool RecalculateStats(libcomp::DefinitionManager* definitionManager);

    /**
     * Check if the character state has everything needed to start
     * being used.
     * @return true if the state is ready to use, otherwise false
     */
    bool Ready();
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHARACTERSTATE_H
