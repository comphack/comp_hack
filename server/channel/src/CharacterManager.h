/**
 * @file server/channel/src/CharacterManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages characters on the channel.
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

#ifndef SERVER_CHANNEL_SRC_CHARACTERMANAGER_H
#define SERVER_CHANNEL_SRC_CHARACTERMANAGER_H

// channel Includes
#include "ChannelClientConnection.h"

namespace libcomp
{
class Packet;
}

namespace objects
{
class Character;
class Demon;
class EntityStats;
class Item;
class ItemBox;
class MiDevilData;
class MiDevilLVUpData;
}

namespace channel
{

class ChannelServer;

/**
 * Manager to handle Character focused actions.
 */
class CharacterManager
{
public:
    /**
     * Create a new CharacterManager.
     * @param server Pointer back to the channel server this
     *  belongs to
     */
    CharacterManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the CharacterManager.
     */
    ~CharacterManager();

    /**
     * Send updated character data to the game client.
     * @param client Pointer to the client connection
     */
    void SendCharacterData(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Tell the game client to show an entity.
     * @param client Pointer to the client connection
     * @param entityID ID of the entity to show
     */
    void ShowEntity(const std::shared_ptr<
        ChannelClientConnection>& client, int32_t entityID);

    /**
     * Send updated data about the active demon of the game client.
     * @param client Pointer to the client connection
     */
    void SendPartnerData(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Send updated data about a demon in the COMP to the game client.
     * @param client Pointer to the client connection
     * @param box Demon container ID, should always be zero
     * @param slot Slot of the demon to send data for
     * @param id ID of the demon to send data for
     */
    void SendCOMPDemonData(const std::shared_ptr<
        ChannelClientConnection>& client,
        int8_t box, int8_t slot, int64_t id);

    /**
     * Send a character's status icon to the game clients.
     * @param client Pointer to the client connection containing
     *  the character
     */
    void SendStatusIcon(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Summon the demon matching the supplied ID on the client's character.
     * @param client Pointer to the client connection containing
     *  the character
     * @param demonID ID of the demon to summon
     */
    void SummonDemon(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int64_t demonID);

    /**
     * Return the current demon of the client's character to the COMP.
     * @param client Pointer to the client connection containing
     *  the character
     */
    void StoreDemon(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Send information about the specified box to the client.
     * @param client Pointer to the client connection containing
     *  the box
     * @param boxID Box index ID
     */
    void SendItemBoxData(const std::shared_ptr<
        ChannelClientConnection>& client, int64_t boxID);

    /**
     * Send information about the specified box slots to the client.
     * @param client Pointer to the client connection containing
     *  the box
     * @param boxID Box index ID
     * @param slots List of slots to send information about
     */
    void SendItemBoxData(const std::shared_ptr<
        ChannelClientConnection>& client, int64_t boxID,
        const std::list<uint16_t>& slots);

    /**
     * Get all items in a character's inventory that match the supplied
     * itemID.
     * @param character Pointer to the character
     * @param itemID Item ID to find in the inventory
     * @return List of pointers to the items of the matching ID
     */
    std::list<std::shared_ptr<objects::Item>> GetExistingItems(
        const std::shared_ptr<objects::Character>& character,
        uint32_t itemID);

    /**
     * Generate an item with the specified stack size.  The stack size
     * will not be checked for the maximum allowed value.
     * @param itemID Item ID to generate an item from
     * @param stackSize Number of items in the stack to generate
     * @return Pointer to the generated item
     */
    std::shared_ptr<objects::Item> GenerateItem(uint32_t itemID,
        uint16_t stackSize);

    /**
     * Add or remove items to a character's inventory.
     * @param client Pointer to the client connection containing
     *  the character
     * @param itemID Item ID to modify the amount of in the inventory
     * @param quantity Number of items to add or remove
     * @param add true if items should be added, false if they should
     *  be removed
     * @param skillTargetID Object ID of an item used for a skill. If
     *  this is specified for a remove command, it will be removed first
     */
    bool AddRemoveItem(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint32_t itemID,
        uint16_t quantity, bool add, int64_t skillTargetID = 0);

    /**
     * Equip an item matching the supplied ID on the client's character.
     * @param client Pointer to the client connection containing
     *  the character
     * @param itemID Object ID matching an item in the character's item
     *  box
     */
    void EquipItem(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int64_t itemID);

    /**
     * Update the client's character's LNC value.
     * @param client Pointer to the client connection containing
     *  the character
     * @param lnc LNC alignment value
     */
    void UpdateLNC(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int16_t lnc);

    /**
     * Create or copy an existing demon and add them to a character's COMP.
     * @param character Pointer to the character that should receive the demon
     * @param demonData Pointer to a demon's definition that represents
     *  the demon to create
     * @param demon Optional pointer to an existing demon to make a copy from
     * @return Pointer to the newly created demon
     */
    std::shared_ptr<objects::Demon> ContractDemon(
        const std::shared_ptr<objects::Character>& character,
        const std::shared_ptr<objects::MiDevilData>& demonData,
        const std::shared_ptr<objects::Demon>& demon = nullptr);

    /**
     * Update the client's character or demon's experience and level
     * up if the level threshold is passed.
     * @param client Pointer to the client connection
     * @param xpGain Experience amount to gain
     * @param entityID Character or demon ID to gain experience
     */
    void ExperienceGain(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint64_t xpGain,
        int32_t entityID);

    /**
     * Increase the level of a client's character or demon.
     * @param client Pointer to the client connection
     * @param level Number of levels to gain
     * @param entityID Character or demon ID to level up
     */
    void LevelUp(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int8_t level,
        int32_t entityID);

    /**
     * Update a client's character's exptertise for a given skill.
     * @param client Pointer to the client connection
     * @param skillID Skill ID that will have it's corresponding
     *  expertise updated if it is enabled
     */
    void UpdateExpertise(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint32_t skillID);

    /**
     * Calculate the base stats of a character.
     * @param cs Pointer to the core stats of a character
     */
    void CalculateCharacterBaseStats(const std::shared_ptr<
        objects::EntityStats>& cs);

    /**
     * Calculate the base stats of a demon.
     * @param ds Pointer to the core stats of a demon
     * @param demonData Pointer to the demon's definition 
     */
    void CalculateDemonBaseStats(const std::shared_ptr<objects::EntityStats>& ds,
        const std::shared_ptr<objects::MiDevilData>& demonData);

    /**
     * Retrieve a map of correct table indexes to corresponding stat values.
     * @param cs Pointer to the core stats of a character
     * @return Map of correct table indexes to corresponding stat values
     */
    static std::unordered_map<uint8_t, int16_t> GetCharacterBaseStatMap(
        const std::shared_ptr<objects::EntityStats>& cs);

    /**
     * Calculate the dependent stats (ex: max HP, close range damage) for a character
     * or demon and update the values of a correct table map.
     * @param stats Reference to a correct table map
     * @param level Current level of the character or demon
     * @param isDemon true if the entity is a demon, false if it is character
     */
    static void CalculateDependentStats(std::unordered_map<uint8_t, int16_t>& stats,
        int8_t level, bool isDemon);

    /**
     * Add COMP slot data to a packet.
     * @param p Packet to populate with COMP data
     * @param client Current client to retrieve COMP information from
     * @param slot COMP slot index to populate information from
     */
    void GetCOMPSlotPacketData(libcomp::Packet& p, 
        const std::shared_ptr<channel::ChannelClientConnection>& client, size_t slot);

    /**
     * Add the core stat data from the supplied EntityStats instance
     * and state object to a packet.
     * @param p Packet to populate with stat data
     * @param coreStats EntityStats pointer to the core stats of either
     *  a character or demon
     * @param state Pointer to the current state of the entity in a
     *  zone.  If this parameter is supplied as null, only the core
     *  stats will be populated.
     * @param boostFormat true if the format should match the one used when
     *  stat changes are reported to the client, false if the format should
     *  match the one used for full entity descriptions
     */
    void GetEntityStatsPacketData(libcomp::Packet& p,
        const std::shared_ptr<objects::EntityStats>& coreStats,
        const std::shared_ptr<objects::EntityStateObject>& state,
        bool boostFormat);

private:
    /**
     * Update the stats of a demon by a specified boost level determined by the
     * demon's actual level.
     * @param stats Reference to a correct table map containing stats that may
     *  or may not have already been boosted
     * @param data Pointer to the level up definition of a demon
     * @param boostLevel Boost level to use when calculating the stat increases
     */
    void BoostStats(std::unordered_map<uint8_t, int16_t>& stats,
        const std::shared_ptr<objects::MiDevilLVUpData>& data, int boostLevel);

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHARACTERMANAGER_H
