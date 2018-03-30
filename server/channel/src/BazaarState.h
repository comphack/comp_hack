/**
 * @file server/channel/src/BazaarState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a bazaar on the channel.
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

#ifndef SERVER_CHANNEL_SRC_BAZAARSTATE_H
#define SERVER_CHANNEL_SRC_BAZAARSTATE_H

// objects Includes
#include <BazaarData.h>
#include <EntityState.h>
#include <ServerBazaar.h>

namespace libcomp
{
class DatabaseChangeSet;
}

namespace channel
{

class ClientState;

/**
 * Contains the state of an bazaar related to a channel.
 */
class BazaarState : public EntityState<objects::ServerBazaar>
{
public:
    /**
     * Create a bazaar state.
     */
    BazaarState(const std::shared_ptr<objects::ServerBazaar>& bazaar);

    /**
     * Clean up the bazaar state.
     */
    virtual ~BazaarState() { }

    /**
     * Get the current market associated to the supplied market ID
     * @param marketID ID of the market to retrieve
     * @return Pointer to the bazaar data representing the current market
     */
    std::shared_ptr<objects::BazaarData> GetCurrentMarket(uint32_t marketID);

    /**
     * Set the current market mapped to the supplied market ID
     * @param marketID ID of the market to set
     * @param data Pointer to the bazaar data representing the current market
     */
    void SetCurrentMarket(uint32_t marketID, const std::shared_ptr<
        objects::BazaarData>& data);

    /**
     * Add an item to the supplied client account's bazaar market. This is thread
     * safe and ensures the add is valid before anything is modified.
     * @param state Pointer to the client state
     * @param slot Destination market slot for the item
     * @param itemID Client object ID of the item to add
     * @param price Price that the item should be sold for
     * @param dbChanges Output pointer to the DatabaseChangeSet to write all
     *  changes to
     * @return true on success, false on failure
     */
    bool AddItem(ClientState* state, int8_t slot, int64_t itemID,
        int32_t price, std::shared_ptr<libcomp::DatabaseChangeSet>& dbChanges);

    /**
     * Drop an item from the supplied client account's bazaar market. This is
     * only thread safe if there is a BazaarState associated to the client. This
     * function is responsible for stopping item drops for active markets.
     * @param state Pointer to the client state
     * @param srcSlot Source market slot for the item
     * @param itemID Client object ID of the item to drop
     * @param destSlot Destination inventory slot for the item
     * @param dbChanges Output pointer to the DatabaseChangeSet to write all
     *  changes to
     * @return true on success, false on failure
     */
    static bool DropItemFromMarket(ClientState* state, int8_t srcSlot, int64_t itemID,
        int8_t destSlot, std::shared_ptr<libcomp::DatabaseChangeSet>& dbChanges);

    /**
     * Get the item at the specified market that matches the requested
     * information if it is currently available to purchase. This is thread
     * safe and should be followed up with a call to BuyItem upon success.
     * @param state Pointer to the client state of the buyer
     * @param marketID Market ID for the item's market
     * @param slot Market slot where the item exists
     * @param itemID Client object ID of the item
     * @param price Price (in macca) that the buyer is expecting to pay
     * @return Pointer to the item that matches the requested information
     *  or nullptr if the request was not valid
     */
    std::shared_ptr<objects::BazaarItem> TryBuyItem(ClientState* state,
        uint32_t marketID, int8_t slot, int64_t itemID, int32_t price);

    /**
     * Update the supplied BazaarItem to be marked as sold if no one else has
     * already bought it
     * @param bItem Pointer to the item being purchased
     * @return true if the item can be bought, false if it cannot
     */
    bool BuyItem(std::shared_ptr<objects::BazaarItem> bItem);

    /**
     * Get the next market expiration associated to the bazaar in system time
     * @return Next market expiration in system time
     */
    uint32_t GetNextExpiration() const;

    /**
     * Set the next market expiration associated to the bazaar in system time
     * from its current markets
     * @return Next market expiration in system time
     */
    uint32_t SetNextExpiration();

private:
    /**
     * Verify if the supplied bazaar market matches the current market
     * assigned to its market ID
     * @param data Pointer to the bazaar market to verify
     * @return true if the market matches, false if it does not
     */
    bool VerifyMarket(const std::shared_ptr<objects::BazaarData>& data);

    /**
     * Drop an item from the supplied client account's bazaar market. This is
     * thread safe and ensures the drop is valid before anything is modified.
     * @param state Pointer to the client state
     * @param srcSlot Source market slot for the item
     * @param itemID Client object ID of the item to drop
     * @param destSlot Destination inventory slot for the item
     * @param dbChanges Output pointer to the DatabaseChangeSet to write all
     *  changes to
     * @return true on success, false on failure
     */
    bool DropItem(ClientState* state, int8_t srcSlot, int64_t itemID,
        int8_t destSlot, std::shared_ptr<libcomp::DatabaseChangeSet>& dbChanges);

    /**
     * Drop an item from the supplied client account's bazaar market. This ensures
     * the drop is valid before anything is modified.
     * @param state Pointer to the client state
     * @param bState Pointer to the BazaarState to
     * @param srcSlot Source market slot for the item
     * @param itemID Client object ID of the item to drop
     * @param destSlot Destination inventory slot for the item
     * @param dbChanges Output pointer to the DatabaseChangeSet to write all
     *  changes to
     * @return true on success, false on failure
     */
    static bool DropItemInternal(ClientState* state, int8_t srcSlot, int64_t itemID,
        int8_t destSlot, std::shared_ptr<libcomp::DatabaseChangeSet>& dbChanges);

    /// Map of market IDs to account owned bazaar data representing an
    /// open market
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::BazaarData>> mCurrentMarkets;

    /// Next market expiration time that will occur
    uint32_t mNextExpiration;

    /// Lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_BAZAARSTATE_H
