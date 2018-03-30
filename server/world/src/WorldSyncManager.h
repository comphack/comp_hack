/**
 * @file server/world/src/WorldSyncManager.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief World specific implementation of the DataSyncManager in
 *  charge of performing server side update operations.
 *
 * This file is part of the World Server (world).
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

#ifndef SERVER_WORLD_SRC_WORLDSYNCMANAGER_H
#define SERVER_WORLD_SRC_WORLDSYNCMANAGER_H

// libcomp Includes
#include <DataSyncManager.h>
#include <EnumMap.h>

// object Includes
#include <SearchEntry.h>

namespace objects
{
class Character;
}

namespace world
{

class WorldServer;

/**
 * World specific implementation of the DataSyncManager in charge of
 * performing server side update operations.
 */
class WorldSyncManager : public libcomp::DataSyncManager
{
public:
    /**
     * Create a new WorldSyncManager
     * @param server Pointer back to the channel server this belongs to.
     */
    WorldSyncManager(const std::weak_ptr<WorldServer>& server);

    /**
     * Clean up the WorldSyncManager
     */
    virtual ~WorldSyncManager();

    /**
     * Initialize the WorldSyncManager after the server has been initialized.
     * @return false if any errors were encountered and the server should
     *  be shut down.
     */
    bool Initialize();

    /**
     * Server specific handler for explicit types of non-persistent records
     * being updated.
     * @param type Type name of the object being updated
     * @param obj Pointer to the record definition
     * @param isRemove true if the record is being removed, false if it is
     *  either an insert or an update
     * @param source Source server identifier
     * @return Response codes matching the internal DataSyncManager set
     */
    template<class T> int8_t Update(const libcomp::String& type,
        const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
        const libcomp::String& source);

    virtual bool RemoveRecord(const std::shared_ptr<libcomp::Object>& record,
        const libcomp::String& type);

    /**
     * Expire the existing record with a matching entry ID and expiration time
     * of the templated type. If either does not match, nothing will be expired.
     * @param entryID Entry ID of the record
     * @param expirationTime Expiration time of the record
     */
    template<class T> void Expire(int32_t entryID, uint32_t expirationTime);

    /**
     * Perform cleanup operations based upon a character logging off (or
     * logging on) based upon world level data being synchronized.
     * @param worldCID CID of the character logging off
     * @param flushOutgoing Optional parameter to send any updates done
     *  immediately
     * @return true if updates were performed during this operation
     */
    bool CleanUpCharacterLogin(int32_t worldCID,
        bool flushOutgoing = false);

    /**
     * Send all existing sync records to the specified connection.
     * @param connection Pointer to the connection
     */
    void SyncExistingChannelRecords(const std::shared_ptr<
        libcomp::InternalConnection>& connection);

private:
    /**
     * Update the number of search entries associated to a specific character
     * for quick access operations later.
     * @param sourceCID CID of the character to update
     * @param type Type of search entry counts to update
     * @param increment true if the count should be increased, false if it
     *  should be decreased
     */
    void AdjustSearchEntryCount(int32_t sourceCID,
        objects::SearchEntry::Type_t type, bool increment);

    /// List of all search entries registered with the server
    std::list<std::shared_ptr<objects::SearchEntry>> mSearchEntries;

    /// Map of character CIDs to registered search entry type counts used
    /// for quick access operations
    std::unordered_map<int32_t, libcomp::EnumMap<
        objects::SearchEntry::Type_t, uint16_t>> mSearchEntryCounts;

    /// Pointer to the channel server.
    std::weak_ptr<WorldServer> mServer;
};

} // namespace world

#endif // SERVER_WORLD_SRC_WORLDSYNCMANAGER_H
