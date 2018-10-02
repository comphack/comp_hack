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
class MatchEntry;
class PentalphaMatch;
class PvPMatch;
class UBTournament;
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

    /**
     * Server specific handler for an operation following explicit types of
     * non-persistent records being synced.
     * @param type Type name of the object being updated
     * @param objs List of pointers to the records being updated and a flag
     *  indicating if the object was being removed
     * @param source Source server identifier
     */
    template<class T> void SyncComplete(const libcomp::String& type,
        const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
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
     * Get any existing match entry by world CID
     * @param worldCID World CID of the entry
     * @return Pointer to a match entry
     */
    std::shared_ptr<objects::MatchEntry> GetMatchEntry(int32_t worldCID);

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

    /**
     * Start (after rechecking) a new solo PvP match based upon the current
     * entries in the queues of the specified type.
     * @param time Ready time that has been communicated as a countdown to
     *  each player in the queue. This helps in deciding if there are still
     *  enough players in the ready state to join the match.
     * @param type PvP match type to start
     */
    void StartPvPMatch(uint32_t time, uint8_t type);

    /**
     * Start (after rechecking) a new team PvP match based upon the current
     * entries in the queues of the specified type.
     * @param time Ready time that has been communicated as a countdown to
     *  each player in the queue. This helps in deciding if there are still
     *  enough players in the ready state to join the match.
     * @param type PvP match type to start
     */
    void StartTeamPvPMatch(uint32_t time, uint8_t type);

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

    /**
     * Determine if a solo PvP match of the specified type can be started and
     * queue it to start if it is.
     * @param type Type ID of the PvP match
     * @param updated Set of world CIDs that have just entered the queue to
     *  determine if the queue countdown needs to restart
     * @return true if the match can be started, false if it cannot
     */
    bool DeterminePvPMatch(uint8_t type, std::set<int32_t> updated = {});

    /**
     * Determine if a team PvP match of the specified type can be started and
     * queue it to start if it is.
     * @param type Type ID of the PvP match
     * @param updated Set of world CIDs that have just entered the queue to
     *  determine if the queue countdown needs to restart
     * @return true if the match can be started, false if it cannot
     */
    bool DetermineTeamPvPMatch(uint8_t type, std::set<int32_t> updated = {});

    /**
     * Create or update a PvP match to send to the channels
     * @param type Type ID of the PvP match
     * @param existing Optional pointer to an existing match requested from
     *  a channel server
     * @return Pointer to the prepared PvP match
     */
    std::shared_ptr<objects::PvPMatch> CreatePvPMatch(int8_t type,
        std::shared_ptr<objects::PvPMatch> existing = nullptr);

    /**
     * Get all match entries by their team ID (including no team)
     * @param type Type ID of the PvP match
     * @return Map of team IDs to entries in that team
     */
    std::unordered_map<int32_t, std::list<std::shared_ptr<
        objects::MatchEntry>>> GetMatchEntryTeams(uint8_t type);

    /**
     * End the supplied PentalphaMatch by properly updating all participating
     * players and closing out their match entries
     * @param match Pointer to the PentalphaMatch to end
     * @return true if the match and all related entries were updated properly
     */
    bool EndMatch(const std::shared_ptr<objects::PentalphaMatch>& match);

    /**
     * Recalculate all rankings for a spectific UBTournament
     * @param tournamentUID UID of the tournament to recalculate
     * @return true if the rankings were updated, false if an error occurred
     */
    bool RecalculateTournamentRankings(const libobjgen::UUID& tournamentUID);

    /**
     * Recalculate all tournament independent UBResult rankings
     * @return true if the rankings were updated, false if an error occurred
     */
    bool RecalculateUBRankings();

    /**
     * End the supplied UBTournament by properly updating the rankings and
     * send the results to the channels
     * @param tournament Pointer to the UBTournament to end
     * @return true if the tournament and all related entries were updated
     *  properly
     */
    bool EndTournament(
        const std::shared_ptr<objects::UBTournament>& tournament);

    /// List of all search entries registered with the server
    std::list<std::shared_ptr<objects::SearchEntry>> mSearchEntries;

    /// Map of character CIDs to registered search entry type counts used
    /// for quick access operations
    std::unordered_map<int32_t, libcomp::EnumMap<
        objects::SearchEntry::Type_t, uint16_t>> mSearchEntryCounts;

    /// Map of character CIDs to queued match entries
    std::unordered_map<int32_t,
        std::shared_ptr<objects::MatchEntry>> mMatchEntries;

    /// Pointer to the currently active pentalpha match
    std::shared_ptr<objects::PentalphaMatch> mPentalphaMatch;

    /// Pointer to the currently active UB tournament
    std::shared_ptr<objects::UBTournament> mUBTournament;

    /// System timestamps for pending PvP ready times indexed by solo entries
    /// (0) or team (1) then PVP type
    std::array<std::array<uint32_t, 2>, 2> mPvPReadyTimes;

    /// Lowest points required to cause a recalculation of the current top
    /// 10 UB total score, top total points and top all time points (in that
    /// index order)
    std::array<uint32_t, 3> mUBRecalcMin;

    /// Next match ID to use for any matches prepared by the server
    uint32_t mNextMatchID;

    /// Pointer to the channel server.
    std::weak_ptr<WorldServer> mServer;
};

} // namespace world

#endif // SERVER_WORLD_SRC_WORLDSYNCMANAGER_H
