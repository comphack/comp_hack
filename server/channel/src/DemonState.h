/**
 * @file server/channel/src/DemonState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a partner demon on the channel.
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

#ifndef SERVER_CHANNEL_SRC_DEMONSTATE_H
#define SERVER_CHANNEL_SRC_DEMONSTATE_H

// objects Includes
#include <ActiveEntityState.h>
#include <Demon.h>

namespace objects
{
class Character;
}

namespace channel
{

/**
 * Contains the state of a partner demon related to a channel.
 */
class DemonState : public ActiveEntityStateImp<objects::Demon>
{
public:
    /**
     * Create a new demon state.
     */
    DemonState();

    /**
     * Clean up the demon state.
     */
    virtual ~DemonState() { }

    /**
     * Get the current unique entry count in the compendium
     * @return Unique entry count in the compendium
     */
    uint32_t GetCompendiumCount() const;

    /**
     * Get the set of tokusei effect IDs granted by compendium
     * completion
     * @return List of tokusei effect IDs
     */
    std::list<int32_t> GetCompendiumTokuseiIDs() const;

    /**
     * Update all character relative, demon independent information
     * that pertains to the current partner's state
     * @param character Pointer to the character that owns the demon state
     * @param definitionManager Pointer to the definition manager to use
     *  when calculating shared demon data
     * @return true if an update occurred, false if one did not
     */
    bool UpdateSharedState(const std::shared_ptr<objects::Character>& character,
        libcomp::DefinitionManager* definitionManager);

private:
    /// Tokusei effect IDs available due to the character's demonic
    /// compendium completion level
    std::list<int32_t> mCompendiumTokuseiIDs;

    /// Quick access count representing the number of unique entries
    /// in he demonic compendium
    uint32_t mCompendiumCount;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_DEMONSTATE_H
