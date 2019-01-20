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

#ifndef SERVER_CHANNEL_SRC_CHARACTERMANAGER_H
#define SERVER_CHANNEL_SRC_CHARACTERMANAGER_H

// libcomp Includes
#include <EnumMap.h>

// object Includes
#include <MiCorrectTbl.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "Zone.h"

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
class ItemDrop;
class MiDevilData;
class MiDevilLVUpData;
class MiDevilLVUpRateData;
class MiMitamaReunionSetBonusData;
class MiItemData;
class PostItem;
}

typedef objects::MiCorrectTbl::ID_t CorrectTbl;

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
     * Send updated character data to a different game client.
     * @param clients List of pointers to the client connections to send
     *  the data to
     * @param otherState Pointer to the state of another client
     */
    void SendOtherCharacterData(const std::list<std::shared_ptr<
        ChannelClientConnection>>& clients,
        ClientState *otherState);

    /**
     * Send updated data about the active demon of the game client.
     * @param client Pointer to the client connection
     */
    void SendPartnerData(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Send updated data about the active demon of a different game client.
     * @param clients List of pointers to the client connections to send
     *  the data to
     * @param otherState Pointer to the state of another client
     */
    void SendOtherPartnerData(const std::list<std::shared_ptr<
        ChannelClientConnection>>& clients,
        ClientState *otherState);

    /**
     * Send updated data about a demon in a box to the game client.
     * @param client Pointer to the client connection
     * @param boxID Demon box ID
     * @param slot Slot of the demon to send data for
     * @param demonID ID of the demon to send data for
     */
    void SendDemonData(const std::shared_ptr<
        ChannelClientConnection>& client,
        int8_t boxID, int8_t slot, int64_t demonID);

    /**
     * Recalculate the stats of an entity belonging to the supplied
     * client and send any resulting changes to the zone (and world
     * if applicable).
     * @param eState Pointer to the state of the entity being recalculated
     * @param client Optional pointer to the client connection. If not
     *  supplied but needed, it will be loaded from the entity ID
     * @param updateSourceClient true if the changes should be sent
     *  to the client itself as well, false if the client will be
     *  communicated about the changes later
     * @return 1 if the calculation resulted in a change to the stats
     *  that should be sent to the client, 2 if one of the changes should
     *  be communicated to the world (for party members etc), 0 otherwise
     */
    uint8_t RecalculateStats(const std::shared_ptr<ActiveEntityState>& eState,
        std::shared_ptr<ChannelClientConnection> client = nullptr,
        bool updateSourceClient = true);

    /**
     * Recalculate the tokusei and stats of an entity belonging to the
     * supplied client and send any resulting changes to the zone (and world
     * if applicable).
     * @param eState Pointer to the state of the entity being recalculated
     * @param client Pointer to the client connection of the entity
     * @return 1 if the calculation resulted in a change to the stats
     *  that should be sent to the client, 2 if one of the changes should
     *  be communicated to the world (for party members etc), 0 otherwise
     */
    uint8_t RecalculateTokuseiAndStats(
        const std::shared_ptr<ActiveEntityState>& eState,
        std::shared_ptr<ChannelClientConnection> client);

    /**
     * Send the stats of an entity that has been recalculated to the
     * zone (and world if applicable).
     * @param client Pointer to the client connection
     * @param entityID ID of the entity associated to the client to
     *  recalculate the stats of
     * @param includeSelf true if the packet should be sent to the
     *  source client as well
     */
    void SendEntityStats(std::shared_ptr<
        ChannelClientConnection> client, int32_t entityID,
        bool includeSelf = true);

    /**
     * Revive the client's character (or demon) in a specific way
     * @param client Pointer to the client connection
     * @param revivalMode Specific revival mode to use. Types are listed
     *  in constants.
     */
    void ReviveCharacter(std::shared_ptr<
        ChannelClientConnection> client, int32_t revivalMode);

    /**
     * Calculate and update a character based how much XP is lost by reviving
     * @param cState Pointer to the character state
     * @param lossPercent Percentage of XP lost for their current level
     * @return true if XP was adjusted, false if it was not
     */
    bool UpdateRevivalXP(const std::shared_ptr<CharacterState>& cState,
        float lossPercent);

    /**
     * Write a revival action notification packet.
     * @param p Packet to write the data to
     * @param entity Entity that has had a revival action performed
     *  on them
     * @param action Revival action that has taken place. The valid
     *  values are:
     *  -1) Revival done: Signifies that no other revival action will
     *      be sent following this one
     *   1) Revive and wait: Revive the player character and wait for
     *      a "revival done" notification. This is typically followed
     *      with a zone change.
     *   3) Normal: The entity has been revived by a skill or item
     *   4) Accept revival: The entity is a character and is accepting
     *      revival from other players
     *   5) Deny revival: The entity is a character and is not accepting
     *      revival from other players
     * @return false if the entity is not valid
     */
    bool GetEntityRevivalPacket(libcomp::Packet& p,
        const std::shared_ptr<ActiveEntityState>& eState,
        int8_t action);

    /**
     * Set the character's status icon and send to the game clients
     * if it has changed.
     * @param client Pointer to the client connection containing
     *  the character
     * @param icon Value of the new status icon to set
     */
    void SetStatusIcon(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int8_t icon = 0);

    /**
     * Send the character title to the client's current zone.
     * @param client Pointer to the client connection containing
     *  the character
     * @param includeSelf true if the client should be sent the title as
     *  well, false if only the rest of the zone should be notified
     */
    void SendCharacterTitle(const std::shared_ptr<
        ChannelClientConnection>& client, bool includeSelf);

    /**
     * Send the specified entity's non-battle movement speed to the client
     * @param client Pointer to the client connection
     * @param eState Pointer to the entity
     * @param diffOnly true if the packet should only be sent if the speed
     *  is not equal to the entity's normal speed
     * @param queue true if the packet should be queued, false if it
     *  should be sent right away
     */
    void SendMovementSpeed(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::shared_ptr<ActiveEntityState>& eState, bool diffOnly,
        bool queue = false);

    /**
     * Send the client player's auto-recovery settings
     * @param client Pointer to the client connection
     */
    void SendAutoRecovery(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Summon the demon matching the supplied ID on the client's character.
     * @param client Pointer to the client connection containing
     *  the character
     * @param demonID ID of the demon to summon
     * @param updatePartyState true if the world should be updated
     *  with the character's new party state, false if it will be updated
     *  later
     */
    void SummonDemon(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int64_t demonID,
        bool updatePartyState = true);

    /**
     * Return the current demon of the client's character to the COMP.
     * @param client Pointer to the client connection containing
     *  the character
     * @param updatePartyState true if the world should be updated
     *  with the character's new party state, false if it will be updated
     *  later
     * @param removeMode Zone removal mode to use when storing the demon
     */
    void StoreDemon(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        bool updatePartyState = true, int32_t removeMode = 2);

    /**
     * Send information about the specified demon box to the client.
     * @param client Pointer to the client connection containing
     *  the box
     * @param boxID Demon box ID
     * @param slots Optional param to signify only specific slotss have
     *  been updated
     */
    void SendDemonBoxData(const std::shared_ptr<
        ChannelClientConnection>& client, int8_t boxID,
        std::set<int8_t> slots = {});

    /**
     * Get the client's character or account owned demon box by ID.
     * @param client Pointer to the client connection containing
     *  the box
     * @param boxID Demon box ID to retrieve. 0 represents the COMP
     *  and anything higher is a depo box
     * @return Pointer to the demon box with the specified ID
     */
    std::shared_ptr<objects::DemonBox> GetDemonBox(ClientState* state,
        int8_t boxID);

    /**
     * Get the client's character or account owned item box by ID.
     * @param client Pointer to the client connection containing
     *  the box
     * @param boxType Type of item box to retrieve
     * @param boxID Item box ID to retrieve by index
     * @return Pointer to the item box with the specified type and ID
     */
    std::shared_ptr<objects::ItemBox> GetItemBox(ClientState* state,
        int8_t boxType, int64_t boxID);

    /**
     * Send information about the specified item box to the client.
     * @param client Pointer to the client connection containing
     *  the box
     * @param box Box to send information regarding
     */
    void SendItemBoxData(const std::shared_ptr<
        ChannelClientConnection>& client,
        const std::shared_ptr<objects::ItemBox>& box);

    /**
     * Send information about the specified box slots to the client.
     * @param client Pointer to the client connection containing
     *  the box
     * @param box Box to send information regarding
     * @param slots List of slots to send information about
     * @param adjustCounts Optional parameter to respond to any item counts
     *  that may have changed and notify the client accordingly. Defaults
     *  to true.
     */
    void SendItemBoxData(const std::shared_ptr<
        ChannelClientConnection>& client, const std::shared_ptr<
        objects::ItemBox>& box, const std::list<uint16_t>& slots,
        bool adjustCounts = true);

    /**
     * Get all items in the specified box that match the supplied
     * itemID.
     * @param character Pointer to the character
     * @param itemID Item ID to find in the inventory
     * @param box Box to return items for
     * @return List of pointers to the items of the matching ID
     */
    std::list<std::shared_ptr<objects::Item>> GetExistingItems(
        const std::shared_ptr<objects::Character>& character,
        uint32_t itemID, std::shared_ptr<objects::ItemBox> box = nullptr);

    /**
     * Get the count of all item stacks in the specified box that
     * match the supplied itemID.
     * @param character Pointer to the character
     * @param itemID Item ID to find in the inventory
     * @param box Box to count items from
     * @return Total stack size of all matching items in the box
     */
    uint32_t GetExistingItemCount(
        const std::shared_ptr<objects::Character>& character,
        uint32_t itemID, std::shared_ptr<objects::ItemBox> box = nullptr);

    /**
     * Get all the free slots within the supplied item box.
     * @param client Pointer to the client connection containing
     *  the box
     * @param box Box to return slots for, if null the inventory
     *  will be used
     * @return Set of all free slots in the box
     */
    std::set<size_t> GetFreeSlots(const std::shared_ptr<
        ChannelClientConnection>& client,
        std::shared_ptr<objects::ItemBox> box = nullptr);

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
     * @param itemCounts Item type to quantity to add
     * @param add true if items should be added, false if they should
     *  be removed
     * @param skillTargetID Object ID of an item used for a skill. If
     *  this is specified for a remove command, it will be removed first
     * @return true if the items were added, false if one or more couldn't
     *  be added
     */
    bool AddRemoveItems(const std::shared_ptr<
        channel::ChannelClientConnection>& client, 
        std::unordered_map<uint32_t, uint32_t> itemCounts, bool add,
        int64_t skillTargetID = 0);

    /**
     * Get the total macca amount from the supplied character's inventory
     * @param character Pointer to the character to calculate macca for
     * @return Total macca
     */
    uint64_t GetTotalMacca(const std::shared_ptr<objects::Character>& character);

    /**
     * Pay a specified macca amount
     * @param client Pointer to the client connection to pay macca
     * @param amount Amount of macca to pay
     * @return true if the amount was paid, false it was not
     */
    bool PayMacca(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint64_t amount);

    /**
     * Calculate the item updates needed to pay a specified macca amount
     * @param client Pointer to the client connection to pay macca
     * @param amount Amount of macca to pay
     * @param insertItems Output list of new items to insert
     * @param stackAdjustItems Output map of items to adjust the stack size of
     *  including deletes listed with stack size 0
     * @return true if the amount can be paid, false it cannot
     */
    bool CalculateMaccaPayment(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint64_t amount,
        std::list<std::shared_ptr<objects::Item>>& insertItems,
        std::unordered_map<std::shared_ptr<objects::Item>, uint16_t>& stackAdjustItems);

    /**
     * Calculate the item updates needed to remove a number of items of a
     * specific type
     * @param client Pointer to the client connection
     * @param itemID Item type to reduce
     * @param amount Amount to reduce the stacks by
     * @param stackAdjustItems Output map of items to adjust the stack size of
     *  including deletes listed with stack size 0
     * @return Amount that could not be removed
     */
    uint64_t CalculateItemRemoval(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint32_t itemID, uint64_t amount,
        std::unordered_map<std::shared_ptr<objects::Item>, uint16_t>& stackAdjustItems);

    /**
     * Update or validate a set of item changes to be applied simultaneously,
     * accounting for deletes that will clear up slots for inserts etc.
     * @param client Pointer to the client connection to apply the changes to
     * @param validateOnly true if the update should just be checked, false if
     *  the changes should actually be applied
     * @param insertItems List of new items to insert
     * @param stackAdjustItems Map of items to adjust the stack size of including
     *  deletes listed with stack size 0
     * @param notifyClient Optional parameter to not send the updated slot information
     *  to the client
     * @return true if the changes could be applied (or validated), false if
     *  they cannot
     */
    bool UpdateItems(const std::shared_ptr<
        channel::ChannelClientConnection>& client, bool validateOnly,
        std::list<std::shared_ptr<objects::Item>>& insertItems,
        std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems,
        bool notifyClient = true);

    /**
     * Retrieve the item associated to the client character's CultureData
     * @param client Pointer to the client connection
     * @return true if an item was retrieved, false if one was not
     */
    bool CultureItemPickup(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Filter a set of item drops based on drop rate and luck.
     * @param drops List of pointers to the item drops to determine what
     *  should be "dropped"
     * @param luck Current luck value to use when calculating drop chances
     * @param minLast Optional param to specify if the set needs at least
     *  one item in which case the last item will be used
     * @return List of item drops that should be "dropped"
     */
    std::list<std::shared_ptr<objects::ItemDrop>> DetermineDrops(
        const std::list<std::shared_ptr<objects::ItemDrop>>& drops,
        int16_t luck, bool minLast = false);

    /**
     * Create loot from drops based upon the supplied luck value (can be 0)
     * and add them to the supplied loot box.
     * @param box Pointer to the loot box
     * @param drops List of pointers to the item drops to determine what
     *  should be added to the box
     * @param luck Current luck value to use when calculating drop chances
     * @param minLast Optional param to specify if the box needs at least
     *  one item in which case the last item will be used
     * @return true if one or more item was added, false if none were
     */
    bool CreateLootFromDrops(const std::shared_ptr<objects::LootBox>& box,
        const std::list<std::shared_ptr<objects::ItemDrop>>& drops, int16_t luck,
        bool minLast = false);

    /**
     * Create loot from pre-filtered drops
     * @param drops List of pointers to item drops
     * @return List of pointers to converted Loot representations
     */
    std::list<std::shared_ptr<objects::Loot>> CreateLootFromDrops(
        const std::list<std::shared_ptr<objects::ItemDrop>>& drops);

    /**
     * Send the loot item data related to a loot box to one or more clients.
     * @param clients List of clients to send the packet to
     * @param lState Pointer to the entity state of the loot box
     * @param queue true if the packet should be queued, false if it
     *  should be sent right away
     */
    void SendLootItemData(const std::list<std::shared_ptr<
        ChannelClientConnection>>& clients, const std::shared_ptr<
        LootBoxState>& lState, bool queue = false);

    /**
     * Equip or unequip an item matching the supplied ID on the
     * client's character.
     * @param client Pointer to the client connection containing
     *  the character
     * @param itemID Object ID matching an item in the character's item
     *  box
     */
    void EquipItem(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int64_t itemID);

    /**
     * Unequip the supplied item from the client's character.  Unlike
     * EquipItem, the item will not be toggled if it isn't equipped.
     * @param client Pointer to the client connection containing
     *  the character
     * @param item Pointer to the item to unequip
     * @return true if the item was unequipped, false if it was not
     *  equipped to begin with
     */
    bool UnequipItem(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::shared_ptr<objects::Item>& item);

    /**
     * Update the current or max durability of the supplied item by
     * the specified amount. The item will be saved and the client will
     * be notified only if a visible value is updated.
     * @param client Pointer to the client connection
     * @param item Pointer to the item to adjust
     * @param points Durability points to adjust by
     * @param isAdjust true if the points supplied should be adjusted
     *  from the existing value, false if it should be explicitly set
     * @param updateMax true if the max durability is being updated,
     *  false if the current durability is being updated
     * @param sendPacket true if the client should be notified upon
     *  visible update, false if this will be handled elseswhere
     * @return true if visible durability was updated, false it was not
     */
    bool UpdateDurability(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::shared_ptr<objects::Item>& item, int32_t points,
        bool isAdjust = true, bool updateMax = false, bool sendPacket = true);

    /**
     * Update the current or max durability of the supplied items by
     * the specified amounts. The item will be saved and the client will
     * be notified only if a visible value is updated.
     * @param client Pointer to the client connection
     * @param items Map of pointers to the items to adjust to their point
     *  values to be adjusted by
     * @param isAdjust true if the points supplied should be adjusted
     *  from the existing value, false if it should be explicitly set
     * @param updateMax true if the max durability is being updated,
     *  false if the current durability is being updated
     * @param sendPacket true if the client should be notified upon
     *  visible update, false if this will be handled elseswhere
     * @return true if visible durability was updated, false it was not
     */
    bool UpdateDurability(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::unordered_map<std::shared_ptr<objects::Item>, int32_t>& items,
        bool isAdjust = true, bool updateMax = false, bool sendPacket = true);

    /**
     * Check if the specified item definition belongs to a CP item
     * @param itemData Pointer to the item definition
     * @return true if the definition belongs to a CP item, false if
     *  it does not
     */
    bool IsCPItem(const std::shared_ptr<objects::MiItemData>& itemData) const;

    /**
     * End the current exchange session for the client with various outcome
     * types. All exchange logic is handled elsewhere and the other
     * participants must be notified via this function as well.
     * @param client Client to notify of the end of an exchange
     * @param outcome Defaults to being ended by player, options are
     *  contextual to the exchange type
     */
    void EndExchange(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t outcome = 1);

    /**
     * Update the client's character's LNC value.
     * @param client Pointer to the client connection containing
     *  the character
     * @param lnc LNC alignment value
     */
    void UpdateLNC(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int16_t lnc);

    /**
     * Create a demon and add it to the client character's COMP.
     * @param client Pointer to the client connection
     * @param demonData Pointer to a demon's definition that represents
     *  the demon to create
     * @param sourceEntityID Entity ID of the demon egg used to contract
     *  the demon
     * @param familiarity Optional familiarity level on the contracted
     *  demon, defaults to 0
     * @return Pointer to the newly created demon
     */
    std::shared_ptr<objects::Demon> ContractDemon(
        const std::shared_ptr<channel::ChannelClientConnection>& client,
        const std::shared_ptr<objects::MiDevilData>& demonData,
        int32_t sourceEntityID, uint16_t familiarity = 0);

    /**
     * Create a demon and add it to a character's COMP.
     * @param character Pointer to the character that should receive the demon
     * @param demonData Pointer to a demon's definition that represents
     *  the demon to create
     * @param familiarity Optional familiarity level on the contracted
     *  demon, defaults to 0
     * @return Pointer to the newly created demon
     */
    std::shared_ptr<objects::Demon> ContractDemon(
        const std::shared_ptr<objects::Character>& character,
        const std::shared_ptr<objects::MiDevilData>& demonData,
        uint16_t familiarity = 0);

    /**
     * Create a demon.
     * @param demonData Pointer to a demon's definition that represents
     *  the demon to create
     * @param familiarity Optional default demon familiarity points
     * @return Pointer to the newly created demon
     */
    std::shared_ptr<objects::Demon> GenerateDemon(
        const std::shared_ptr<objects::MiDevilData>& demonData,
        uint16_t familiarity = 0);

    /**
     * Perform reunion on the supplied client's partner demon if they
     * match the supplied ID.
     * @param client Pointer to the client connection
     * @param demonID Object ID of the demon currently summoned
     * @param growthType New growth type to use for the demon
     * @param costItemType Item type used for payment that must match
     *  the predefined types for the growth type
     * @param replyToClient If true the client will be sent a requested
     *  reunion reply
     * @param force If true only base validations will be performed,
     *  skipping costs and most growth type validations
     * @param forceRank Explicit rank to of the reunion bonuses to receive
     *  when forcing reunion to a normal type. This value cannot exceed
     *  the server configured maximum value.
     * @return true if the reunion succeeded, false if it did not
     */
    bool ReunionDemon(const std::shared_ptr<ChannelClientConnection> client,
        int64_t demonID, uint8_t growthType, uint32_t costItemType,
        bool replyToClient, bool force = false, int8_t forceRank = -1);

    /**
     * Get the total number of reunion ranks achieved by the supplied demon
     * @param demon Pointer to the demon
     * @return Total number of reunion ranks achieved
     */
    uint16_t GetReunionRankTotal(const std::shared_ptr<objects::Demon> demon);

    /**
     * Convert a demon into its upgraded mitama demon type
     * @param client Pointer to the client connection
     * @param demonID Object ID of the demon to convert
     * @param growthType New growth type to use for the demon
     * @param mitamaType Mitama type to use for the demon
     * @return true if the demon was converted successfully
     */
    bool MitamaDemon(const std::shared_ptr<ChannelClientConnection> client,
        int64_t demonID, uint8_t growthType, uint8_t mitamaType);

    /**
     * Check if the specified devil definition is a mitama'd type
     * @param devilData Pointer to the demon definition
     * @return true if the definition belongs to a mitama'd type, false if
     *  it does not
     */
    bool IsMitamaDemon(const std::shared_ptr<
        objects::MiDevilData>& devilData) const;

    /**
     * Apply special effects that occur as part of the normal regen synced
     * "t-damage" processing that occurs every 10 seconds.
     * @param eState Pointer to the entity that just applied regen
     */
    void ApplyTDamageSpecial(const std::shared_ptr<ActiveEntityState>& eState);

    /**
     * Get a demon's familiarity rank from their current familiarity points.
     * @param familiarity Demon's familiarity points
     * @return Converted familiarity rank from -3 to +4
     */
    static int8_t GetFamiliarityRank(uint16_t familiarity);

    /**
     * Update the current partner demon's familiarity.
     * @param client Pointer to the client connection
     * @param familiarity Set or adjusted familiarity points to
     *  update the demon with
     * @param isAdjust true if the familiarity value should be added
     *  to the current value, false if it should replace the curent value
     * @param sendPacket true if the zone should be informed of the
     *  updated demon familiarity
     */
    void UpdateFamiliarity(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t familiarity,
        bool isAdjust = false, bool sendPacket = true);

    /**
     * Update the current partner demon's soul points.
     * @param client Pointer to the client connection
     * @param points Set or adjusted soul points to update the demon with
     * @param isAdjust true if the points value should be added
     *  to the current value, false if it should replace the curent value
     * @param applyRate If true the soul point rate will be used to adjust
     *  the amount gained
     * @return Adjusted SP amount gained or lossed (before limits are applied)
     */
    int32_t UpdateSoulPoints(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t points,
        bool isAdjust = false, bool applyRate = false);

    /**
     * Update the player's fusion gauge.
     * @param client Pointer to the client connection
     * @param points Set or adjusted fusion gauge points to update
     * @param isAdjust true if the points value should be added to the current
     *  value, false if it should replace the curent value
     */
    void UpdateFusionGauge(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t points,
        bool isAdjust);

    /**
     * Send the player's fusion gauge to the client.
     * @param client Pointer to the client connection
     */
    void SendFusionGauge(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Update the player's casino coin total.
     * @param client Pointer to the client connection
     * @param amount Set or adjusted coin amount to update
     * @param isAdjust true if the amount value should be added to the current
     *  value, false if it should replace the curent value
     */
    bool UpdateCoinTotal(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int64_t amount,
        bool isAdjust);

    /**
     * Send the client character's current casino coin total.
     * @param client Pointer to the client connection
     * @param isUpdate Signifies that the amount is not being sent due to
     *  initial character login
     */
    void SendCoinTotal(const std::shared_ptr<
        channel::ChannelClientConnection>& client, bool isUpdate);

    /**
     * Update the player's current BP total.
     * @param client Pointer to the client connection
     * @param points Set or adjusted BP value to update
     * @param isAdjust true if the point value should be added to the current
     *  value, false if it should replace the curent value
     */
    bool UpdateBP(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t points,
        bool isAdjust);

    /**
     * Send the client character's current PvP information.
     * @param client Pointer to the client connection
     */
    void SendPvPCharacterInfo(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Update the character's bethel value for heir current Pentalpha match
     * entry
     * @param client Pointer to the client connection
     * @param bethel Set or adjusted bethel value to update
     * @param adjust true if the point value should be adjusted via tokusei
     * @return Final point value added
     */
    int32_t UpdateBethel(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t bethel,
        bool adjust);

    /**
     * Update the character's cowrie and bethel values independent of a match
     * @param client Pointer to the client connection
     * @param cowrie Amount of cowrie to add
     * @param bethel Amount of bethel to add to each of the 5 types
     * @return true if the update succeeded, false if it did not
     */
    bool UpdateCowrieBethel(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t cowrie,
        const std::array<int32_t, 5>& bethel);

    /**
     * Send the client character's current cowrie and bethel values
     * @param client Pointer to the client connection
     */
    void SendCowrieBethel(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Update or create a character assigned event counter
     * @param client Pointer to the client connection
     * @param type Type of event to update
     * @param value Value to add/assign to the counter
     * @param noSync Optional parameter to update the counter but
     *  not sync with the world. Useful for events that are not summed.
     * @return true if the update succeeded, false if it did not
     */
    bool UpdateEventCounter(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t type,
        int32_t value, bool noSync = false);

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
     * Update a client's character's expertise for a given skill.
     * @param client Pointer to the client connection
     * @param skillID Skill ID that will have it's corresponding
     *  expertise updated if it is enabled
     * @param rateBoost Optional flat value to add to the expertise
     *  rates for the skill (before calculating final amount)
     * @param multiplier Expertise point multiplier, defaults to -1
     *  to differentiate from explicitly being set to 1. If this is
     *  not set, the character's expertise acquisition rate will be
     *  used.
     */
    void UpdateExpertise(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint32_t skillID,
        uint16_t rateBoost = 0, float multiplier = -1.0f);

    /**
     * Calculate how many expertise points would be gained for a specific
     * character, expertise and growth rate. Inactive expertise will always
     * return 0.
     * @param cState Pointer to the character state
     * @param expertiseID ID of the expertise to calculate
     * @param growthRate Growth rate to apply to the calculation, typically
     *  from a skill's growth data
     * @param rateBoost Optional flat value to add to the expertise
     *  rates for the calculation (before calculating final amount)
     * @param multiplier Expertise point multiplier, defaults to -1
     *  to differentiate from explicitly being set to 1. If this is
     *  not set, the character's expertise acquisition rate will be
     *  used.
     * @return Expertise point gain, will never be negative
     */
    int32_t CalculateExpertiseGain(const std::shared_ptr<CharacterState>& cState,
        uint8_t expertiseID, float growthRate,
        uint16_t rateBoost = 0, float multiplier = -1.0f);

    /**
     * Update a client's character's expertise for a set of expertise IDs.
     * If an expertise is supplied that is not enabled it will still be
     * updated.
     * @param client Pointer to the client connection
     * @param pointMap Map of expertise IDs to points that will be added.
     *  The order the expertise are passed in is the order they will be
     *  processed in in case the limit is reached.
     * @param force If false, no updates will be performed should the
     *  matching expertise be locked or not created already.
     */
    void UpdateExpertisePoints(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<std::pair<uint8_t, int32_t>>& pointMap,
        bool force = false);

    /**
     * Get the maximum number of expertise points available to the
     * supplied characater.
     * @param character Pointer to the character
     * @return Maximum expertise points available to the character
     */
    int32_t GetMaxExpertisePoints(const std::shared_ptr<
        objects::Character>& character);

    /**
     * Send the character's current expertise extension amount.
     * @param client Pointer to the client connection containing
     *  the character
     */
    void SendExpertiseExtension(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Increase the client's character available skill points.
     * @param client Pointer to the client connection
     * @param points Number of points to gain
     */
    void UpdateSkillPoints(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t points);

    /**
     * Update the specified entity to learn a skill by ID.
     * @param client Pointer to the client connection
     * @param entityID ID of the entity to learn the skill
     * @param skillID Skill ID to update the entity to learn
     * @return true if the skill was learned or is already learned,
     *  false if it failed
     */
    bool LearnSkill(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t entityID,
        uint32_t skillID);

    /**
     * Determine various elements of the outcome of a synth exchange
     * but do not perform the synth itself. Supports demon crystallization
     * as well as tarot and soul enchantment.
     * @param synthState Pointer to the synth character's ClientState
     * @param exchangeSession Pointer to the synth exchange session
     * @param outcomeItemType Output parameter for the new item that will result
     *  from the synth, -1 for none
     * @param successRates Output parameter containing any success rates
     *  associated with the operation
     * @param effectID Output parameter only used by enchantment to contain
     *  the effect that will be applied
     * @return true if the calculation succeeded, false if an error occured
     */
    bool GetSynthOutcome(ClientState* synthState,
        const std::shared_ptr<objects::PlayerExchangeSession>& exchangeSession,
        uint32_t& outcomeItemType, std::list<int32_t>& successRates,
        int16_t* effectID = 0);

    /**
     * Convert the supplied ID to its corresponding flag array index
     * and shift value for use with fields stored as arrays of flags
     * stored as single bytes
     * @param uint16_t ID to convert
     * @param index Output parameter that will be updated to the index
     *  of the flag array for the ID's corresponding byte
     * @param shiftVal Ouptput parameter that will be updated to the
     *  ID's mask position at the index in the flag array
     */
    static void ConvertIDToMaskValues(uint16_t id, size_t& index,
        uint8_t& shiftVal);

    /**
     * Add a map that the client character has obtained.
     * @param client Pointer to the client connection
     * @param mapID ID representing both the map that was obtained and its
     *  byte shift position in the storage array
     * @return true if the map is valid and was either added or already
     *  obtained, false it was not
     */
    bool AddMap(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint16_t mapID);

    /**
     * Send the client character's obtained maps stored as an array of flags
     * @param client Pointer to the client connection
     */
    void SendMapFlags(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Add or remove a valuable that the client character has obtained.
     * @param client Pointer to the client connection
     * @param valuableID ID representing both the valuable that was obtained
     *  and its byte shift position in the storage array
     * @param remove true if the value should be removed instead of added
     * @return true if the valuable is valid and was either added, removed
     *  or already updated in the way requested, false it was not
     */
    bool AddRemoveValuable(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint16_t valuableID,
        bool remove);

    /**
     * Determine if the specified character has the specified valuable
     * @param character Pointer to the character to check
     * @param valuableID ID of the valuable to look for
     * @return true if the valuable has been obtained
     */
    static bool HasValuable(const std::shared_ptr<objects::Character>& character,
        uint16_t valuableID);

    /**
     * Send the client character's obtained valuables stored as an array of flags
     * @param client Pointer to the client connection
     */
    void SendValuableFlags(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Add a fusion plugin that the client character has obtained.
     * @param client Pointer to the client connection
     * @param pluginID ID representing both the plugin that was obtained and its
     *  byte shift position in the storage array
     * @return true if the plugin is valid and was either added or already
     *  obtained, false it was not
     */
    bool AddPlugin(const std::shared_ptr<
        channel::ChannelClientConnection>& client, uint16_t pluginID);

    /**
     * Send the client character's obtained plugins stored as an array of flags
     * @param client Pointer to the client connection
     */
    void SendPluginFlags(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Add a specific title option to the player's character
     * @param client Pointer to the client connection
     * @param titleID ID of the title to add
     * @return true if the title was added, false if it was not
     */
    bool AddTitle(const std::shared_ptr<ChannelClientConnection>& client,
        int16_t titleID);

    /**
     * Send the client character's material container contents
     * @param client Pointer to the client connection
     * @param updates If specified only the material types that match will
     *  be sent as an update, otherwise the full box set be sent
     */
    void SendMaterials(const std::shared_ptr<
        ChannelClientConnection>& client, std::set<uint32_t> updates = {});

    /**
     * Send the client character's demonic compendium list
     * @param client Pointer to the client connection
     */
    void SendDevilBook(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Send the client character's current Invoke event status
     * @param client Pointer to the client connection
     * @param force If true, send the notification whether any cooldowns
     *  are active or not. If false, only send if a cooldown is active.
     * @param queue true if the packet should be queued, false if it
     *  should be sent right away
     */
    void SendInvokeStatus(const std::shared_ptr<
        ChannelClientConnection>& client, bool force, bool queue = false);

    /**
     * Send post items not already "distributed" to the client and update
     * them so they do not send a second time.
     * @param client Pointer to the client connection
     * @param post List of post items pending distribution
     */
    void NotifyItemDistribution(const std::shared_ptr<
        ChannelClientConnection>& client,
        std::list<std::shared_ptr<objects::PostItem>> post);

    /**
     * Update the status effects active on a demon that is not currently
     * summoned
     * @param demon Pointer to an unsummoned demon
     * @param accountUID UID of the account the demon belongs to
     * @param queueSave When true, any updates that take place will be queued
     *  instead of saved immediately
     * @return true if the update completed without error
     */
    bool UpdateStatusEffects(const std::shared_ptr<objects::Demon>& demon,
        const libobjgen::UUID& accountUID, bool queueSave);

    /**
     * Update the status effects assigned directly on a character or demon
     * entity from the current status effect states and save them to the
     * database.
     * @param eState Pointer to the entity to save the status effects for
     * @param queueSave true if the save action should be queued, false
     *  if it should be done synchronously
     */
    bool UpdateStatusEffects(const std::shared_ptr<
        ActiveEntityState>& eState, bool queueSave = false);

    /**
     * Add one or more status effects to a client entity and immediately
     * notify the zone instead of waiting for the queued change to be
     * picked up by zone processing. Useful when another packet is needed
     * immediately after a status effect is added.
     * @param client Pointer to the client connection
     * @param eState Pointer to the entity to add statuse effects to
     * @param effects Set of status effects to add
     * @return true if any effects were actually added
     */
    bool AddStatusEffectImmediate(const std::shared_ptr<
        ChannelClientConnection>& client, const std::shared_ptr<
        ActiveEntityState>& eState, const StatusEffectChanges& effects);

    /**
     * Cancel status effects associated to a client's character and demon
     * based upon an action that affects both entities.
     * @param client Pointer to the client connection
     * @param cancelFlags Flags indicating conditions that can cause status
     *  effects to be cancelled. The set of valid status conditions are listed
     *  as constants on ActiveEntityState
     */
    void CancelStatusEffects(const std::shared_ptr<
        ChannelClientConnection>& client, uint8_t cancelFlags);

    /**
     * Cancel the mount status effects on both the character and demon of
     * the supplied ClientState. If the entity's are not mounted, no work
     * will be done.
     * @param state Pointer to the client state
     * @return true if the status effects existed and were cancelled
     */
    bool CancelMount(ClientState* state);

    /**
     * Build a packet containing a status effect add/update notification
     * @param p Packet to write the data to
     * @param entityID ID of the entity with the status effects
     * @param active List of status effects that have been changed
     * @return true if any status effects were supplied and the packet should
     *  be sent
     */
    bool GetActiveStatusesPacket(libcomp::Packet& p, int32_t entityID,
        const std::list<std::shared_ptr<objects::StatusEffect>>& active);

    /**
     * Build a packet containing a status effect removal notification
     * @param p Packet to write the data to
     * @param entityID ID of the entity that had the status effects
     * @param active List of status effect IDs that have been removed
     * @return true if any status effects were supplied and the packet should
     *  be sent
     */
    bool GetRemovedStatusesPacket(libcomp::Packet& p, int32_t entityID,
        const std::set<uint32_t>& removed);

    /**
     * Add or remove both supplied entities to the other's opponent set.
     * If adding and either of the supplied entities is a player character
     * or partner demon, both of those entities will be added instead.
     * @param add true if the opponents should be added, false if they should
     *  be removed
     * @param eState1 Pointer to the primary entity to modify
     * @param eState2 Pointer ot the secondary entity to modify. If removing
     *  and this is not supplied, all entities will be removed from the
     *  primary entity
     * @return true if the update occurred without issue
     */
    bool AddRemoveOpponent(bool add, const std::shared_ptr<
        ActiveEntityState>& eState1, const std::shared_ptr<
        ActiveEntityState>& eState2);

    /**
     * Increase the digitalize XP points of one or more demon race for the
     * supplied client. If the race is not unlocked yet, the unlock criteria
     * will be checked before adding any points.
     * @param client Pointer to the client connection
     * @param pointMap Map of race IDs to points to add
     * @param allowAdjust If true, tokusei effects can increase the amount of
     *  points beinga added
     * @param validate Optional parameter to disable checking unlock criteria
     *  and current digitalize state level limits
     * @return true if an updateable digitalize race was supplied, false
     *  if they were all invalid or not unlockable
     */
    bool UpdateDigitalizePoints(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::unordered_map<uint8_t, int32_t>& pointMap,
        bool allowAdjust, bool validate = true);

    /**
     * Enter a player character into the digitalize state using the
     * supplied demon
     * @param client Pointer to the client connection associated to the
     *  character to digitalize
     * @param demon Pointer to the digitalize target demon
     * @return true if digitalization started, false if a failure occurred
     */
    bool DigitalizeStart(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::shared_ptr<objects::Demon>& demon);

    /**
     * End the digitalize state of a player character
     * @param client Pointer to the client connection associated to the
     *  digitalized character
     * @return true if digitalization ended, false if was not active or
     *  could not be ended
     */
    bool DigitalizeEnd(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Update the state of the supplied entities on the world server.
     * @param entities List of pointers to entities to update on the world
     *  server. If the supplied entities are not associated to any others
     *  at a world server level, nothing will be sent.
     */
    void UpdateWorldDisplayState(const std::set<
        std::shared_ptr<ActiveEntityState>> &entities);

    /**
     * Calculate the base stats of a character.
     * @param cs Pointer to the core stats of a character
     */
    void CalculateCharacterBaseStats(const std::shared_ptr<
        objects::EntityStats>& cs);

    /**
     * Calculate the base stats of a demon.
     * @param demon Pointer to the demon being calculated
     * @param ds Pointer to the core stats of a demon
     * @param demonData Pointer to the demon's definition
     * @param setHPMP If true, the current HP and MP will be set to the
     *  calculated max values
     */
    void CalculateDemonBaseStats(const std::shared_ptr<objects::Demon>& demon,
        std::shared_ptr<objects::EntityStats> ds = nullptr,
        std::shared_ptr<objects::MiDevilData> demonData = nullptr,
        bool setHPMP = true);

    /**
     * Retrieve a map of correct table indexes to corresponding stat values
     * for the supplied demon definition without any adjustments
     * @param demonData Pointer to the demon's definition
     * @return Map of correct table indexes to corresponding stat values
     */
    static libcomp::EnumMap<CorrectTbl, int16_t> GetDemonBaseStats(
        const std::shared_ptr<objects::MiDevilData>& demonData);

    /**
     * Retrieve a map of correct table indexes to corresponding stat values
     * for the supplied demon definition adjusted for current level and growth
     * type
     * @param demonData Pointer to the demon's definition
     * @param definitionManager Pointer to the definition manager to use to
     *  retrieve growth information
     * @param growthType Demon growth type to calculate stats for
     * @param level Current level to calculate stats for
     * @return Map of correct table indexes to corresponding stat values
     */
    static libcomp::EnumMap<CorrectTbl, int16_t> GetDemonBaseStats(
        const std::shared_ptr<objects::MiDevilData>& demonData,
        libcomp::DefinitionManager* definitionManager, uint8_t growthType,
        int8_t level);

    /**
     * Apply demon familiarity boost to stats
     * @param familiarity Demon's current familiarity level
     * @param stats Reference to a correct table map
     * @param levelRate Level up rate to use for applying familiarity boosts
     */
    static void FamiliarityBoostStats(uint16_t familiarity,
        libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        std::shared_ptr<objects::MiDevilLVUpRateData> levelRate);

    /**
     * Adjust the base stats of a demon being calculated.
     * @param demon Pointer to the demon being calculated
     * @param stats Reference to a correct table map
     * @param baseCalc When true, stats are being calculated to set directly on
     *  the demon. When false, stats are being calculated for when the demon is
     *  displayed as the active partner.
     * @param readOnly If true no values will be written directly to the demon
     */
    static void AdjustDemonBaseStats(const std::shared_ptr<
        objects::Demon>& demon, libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        bool baseCalc, bool readOnly = false);

    /**
     * Adjust mitama specific stats of a demon being calculated.
     * @param demon Pointer to the demon being calculated
     * @param stats Reference to a correct table map
     * @param definitionManager Pointer to the definition manager to use to
     *  retrieve stat information
     * @param reunionMode Adjust all reunion inreased stats (0), only base
     *  stats (1) or only non-base stats (2)
     * @param entityID ID of the demon entity if the one being calculated is
     *  summoned, defaults to unsummoned
     * @param includeSetBonuses Mitama set bonuses will be included in the
     *  calculation if true and ignored if false
     */
    static void AdjustMitamaStats(const std::shared_ptr<objects::Demon>& demon,
        libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        libcomp::DefinitionManager* definitionManager, uint8_t reunionMode,
        int32_t entityID = 0, bool includeSetBonuses = true);

    /**
     * Get all direct and set mitama bonuses for the supplied demon
     * @param demon Pointer to a demon
     * @param definitionManager Pointer to the definition manager to use to
     *  retrieve bonus information
     * @param bonuses Output variable to store direct bonus IDs (and counts)
     * @param setBonuses Output variable to store all active set bonuses
     * @param excludeTokusei If true, tokusei based set bonuses will be ignored
     * @return true if one or more bonus is active, false if no bonuses are
     *  active
     */
    static bool GetMitamaBonuses(const std::shared_ptr<objects::Demon>& demon,
        libcomp::DefinitionManager* definitionManager,
        std::unordered_map<uint8_t, uint8_t>& bonuses,
        std::set<uint32_t>& setBonuses, bool excludeTokusei);

    /**
     * Get all trait skill IDs on the supplied demon directly or from equipment
     * @param demon Pointer to a demon
     * @param demonData Pointer to the demon's definition
     * @param definitionManager Pointer to the definition manager to use to
     *  retrieve demon information
     * @return Set of active trait skill IDs
     */
    static std::set<uint32_t> GetTraitSkills(
        const std::shared_ptr<objects::Demon>& demon,
        const std::shared_ptr<objects::MiDevilData>& demonData,
        libcomp::DefinitionManager* definitionManager);

    /**
     * Retrieve a map of correct table indexes to corresponding stat values.
     * @param cs Pointer to the core stats of a character
     * @return Map of correct table indexes to corresponding stat values
     */
    static libcomp::EnumMap<CorrectTbl, int16_t> GetCharacterBaseStats(
        const std::shared_ptr<objects::EntityStats>& cs);

    /**
     * Calculate the dependent stats (ex: max HP, close range damage) for a character
     * or demon and update the values of a correct table map.
     * @param stats Reference to a correct table map
     * @param level Current level of the character or demon
     * @param isDemon true if the entity is a demon, false if it is character
     */
    static void CalculateDependentStats(libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        int8_t level, bool isDemon);

    /**
     * Correct a map of calculated stat values to not exceed the maximum or
     * minimum values possible.
     * @param stats Reference to a correct table map
     * @param limitMax If true, maximum stat limits will be adjusted as well
     */
    static void AdjustStatBounds(libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        bool limitMax);

    /**
     * Determine what item should be given as a demon specific present.
     * @param demonType Demon type
     * @param level Demon level
     * @param familiarity Demon current familiarity
     * @param rarity Output parameter to store the rarity level of the item if
     *  one is returned
     * @return Item type to give as a present or zero if none will be given
     */
    uint32_t GetDemonPresent(uint32_t demonType, int8_t level, uint16_t familiarity,
        int8_t& rarity) const;

    /**
     * Add data to a packet about a demon in a box.
     * @param p Packet to populate with demon data
     * @param client Current client to retrieve demon information from
     * @param box Demon box to populate information from
     * @param slot Demon box slot index to populate information from
     */
    void GetDemonPacketData(libcomp::Packet& p, 
        const std::shared_ptr<channel::ChannelClientConnection>& client,
        const std::shared_ptr<objects::DemonBox>& box, int8_t slot);

    /**
     * Add data to a packet containing varying degrees of item details.
     * @param p Packet to populate with item data
     * @param item Pointer to the item
     * @param detailLevel Detail level to add to the packet ranging from
     *  0 (least) to 2 (most). Defaults to 2.
     */
    void GetItemDetailPacketData(libcomp::Packet& p, const std::shared_ptr<
        objects::Item>& item, uint8_t detailLevel = 2);

    /**
     * Add the core stat data from the supplied EntityStats instance
     * and state object to a packet.
     * @param p Packet to populate with stat data
     * @param coreStats EntityStats pointer to the core stats of either
     *  a character or demon
     * @param state Pointer to the current state of the entity in a
     *  zone.  If this parameter is supplied as null, only the core
     *  stats will be populated.
     * @param format Format to write the data in from the followin options:
     *  0) Standard format used when intially describing a new entity
     *  1) "Boost" format used when describing an entity that
     *     has leveled up or had its non-calculated stats adjusted
     *  2) Recalc format used when stats have been recalculated caused
     *     by equipment or status effects being modified.
     *  3) Recalc extended format
     * @param coreBoosts Core stat boosts active on an entity that is not
     *  not currently active (ex: familiarity boosts for COMP demons)
     */
    void GetEntityStatsPacketData(libcomp::Packet& p,
        const std::shared_ptr<objects::EntityStats>& coreStats,
        const std::shared_ptr<ActiveEntityState>& state, uint8_t format,
        libcomp::EnumMap<CorrectTbl, int16_t> coreBoosts = {});

    /**
     * Mark the supplied demon and its related data as deleted in the
     * supplied changeset. If the demon is in a demon box, the box will
     * be updated as well.
     * @param demon Pointer to the demon that will be deleted
     * @param changes Pointer to the changeset to apply the changes to
     */
    void DeleteDemon(const std::shared_ptr<objects::Demon>& demon,
        const std::shared_ptr<libcomp::DatabaseChangeSet>& changes);

private:
    /**
     * Update the stats of a demon by a specified boost level determined by the
     * demon's actual level.
     * @param stats Reference to a correct table map containing stats that may
     *  or may not have already been boosted
     * @param data Pointer to the level up definition of a demon
     * @param boostLevel Boost level to use when calculating the stat increases
     */
    static void BoostStats(libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        const std::shared_ptr<objects::MiDevilLVUpData>& data, int boostLevel);

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHARACTERMANAGER_H
