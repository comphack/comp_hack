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

#ifndef SERVER_CHANNEL_SRC_DEMONSTATE_H
#define SERVER_CHANNEL_SRC_DEMONSTATE_H

// objects Includes
#include <ActiveEntityState.h>
#include <Demon.h>

namespace objects
{
class Character;
class CharacterState;
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
     * Get the current unique entry count in the compendium or count specific
     * to entries matching a supplied race or family ID
     * @param groupID If specified, instead return the number of entries that
     *  match a family or race ID
     * @param familyGroup If true the groupID is a family ID, if false the
     *  groupID is a race ID
     * @return Unique entry count in the compendium
     */
    uint16_t GetCompendiumCount(uint8_t groupID = 0, bool familyGroup = true);

    /**
     * Get the set of tokusei effect IDs granted by compendium
     * completion
     * @return List of tokusei effect IDs
     */
    std::list<int32_t> GetCompendiumTokuseiIDs() const;

    /**
     * Get the set of tokusei effect IDs granted to the current demon
     * @return List of tokusei effect IDs
     */
    std::list<int32_t> GetDemonTokuseiIDs() const;

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
     * Update all state information that pertains to the current partner.
     * Both demon state and character skills can affect this.
     * @param definitionManager Pointer to the definition manager to use
     *  when calculating demon data
     * @return true if an update occurred, false if one did not
     */
    bool UpdateDemonState(libcomp::DefinitionManager* definitionManager);

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

    virtual const libobjgen::UUID GetEntityUUID();

    virtual uint8_t RecalculateStats(libcomp::DefinitionManager* definitionManager,
        std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr);

    virtual std::set<uint32_t> GetAllSkills(
        libcomp::DefinitionManager* definitionManager, bool includeTokusei);

    virtual uint8_t GetLNCType();

    virtual int8_t GetGender();

    virtual bool HasSpecialTDamage();

private:
    /// Map of inherited skills not yet maxed by affinity ID. This
    /// map is refreshed by calling RefreshLearningSkills
    std::unordered_map<uint8_t, std::list<
        std::shared_ptr<objects::InheritedSkill>>> mLearningSkills;

    /// Tokusei effect IDs available due to the demon's current state
    std::list<int32_t> mDemonTokuseiIDs;

    /// Tokusei effect IDs available due to the character's demonic
    /// compendium completion level
    std::list<int32_t> mCompendiumTokuseiIDs;

    /// Quick access count representing the number of unique entries
    /// in the demonic compendium
    uint16_t mCompendiumCount;

    /// Quick access count representing the number of entries in the
    /// demonic compendium by family
    std::unordered_map<uint8_t, uint16_t> mCompendiumFamilyCounts;

    /// Quick access count representing the number of entries in the
    /// demonic compendium by race
    std::unordered_map<uint8_t, uint16_t> mCompendiumRaceCounts;

    /// Map of bonus stats gained from the character
    libcomp::EnumMap<CorrectTbl, int16_t> mCharacterBonuses;

    /// Shared state property specific mutex lock
    std::mutex mSharedLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_DEMONSTATE_H
