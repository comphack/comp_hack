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
class InheritedSkill;
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

    /**
     * Get list of skills currently being learned by affinity ID
     * @param affinity Affinity ID to retrieve skills for
     * @return List of InheritedSkills being learned
     */
    std::list<std::shared_ptr<objects::InheritedSkill>>
        GetLearningSkills(uint8_t affinity);

    /**
     * Update the set of InheritedSkills being learned either by
     * specific affinity or all currently associated to the entity
     * @param affinity Affinity ID to refresh
     * @param definitionManager Pointer to the definition manager to use
     *  when determining skill affinity
     */
    void RefreshLearningSkills(uint8_t affinity,
        libcomp::DefinitionManager* definitionManager);

    /**
     * Update an InheritedSkill skill's progress points
     * @param iSkill Pointer to the InheritedSkill from the demon
     * @param points Number of progress points to add to the skill
     * @return Final progress point count for the skill
     */
    int16_t UpdateLearningSkill(const std::shared_ptr<
        objects::InheritedSkill>& iSkill, uint16_t points);

private:
    /// Map of inherited skills not yet maxed by affinity ID. This
    /// map is refreshed by calling RefreshLearningSkills
    std::unordered_map<uint8_t, std::list<
        std::shared_ptr<objects::InheritedSkill>>> mLearningSkills;

    /// Tokusei effect IDs available due to the character's demonic
    /// compendium completion level
    std::list<int32_t> mCompendiumTokuseiIDs;

    /// Quick access count representing the number of unique entries
    /// in he demonic compendium
    uint32_t mCompendiumCount;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_DEMONSTATE_H
