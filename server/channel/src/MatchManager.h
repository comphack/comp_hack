/**
 * @file server/channel/src/MatchManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manager class in charge of handling any client side match or
 *  team logic. Match types include PvP, Ultimate Battle, etc.
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

#ifndef SERVER_CHANNEL_SRC_MATCHMANAGER_H
#define SERVER_CHANNEL_SRC_MATCHMANAGER_H

 // channel Includes
#include "ChannelClientConnection.h"

namespace objects
{
class MatchEntry;
class PvPData;
class PvPMatch;
}

namespace channel
{

class ChannelServer;
class ZoneInstance;

/**
 * Manager class used to handle all channel side match logic.
 */
class MatchManager
{
public:
    /**
     * Create a new MatchManager
     * @param server Pointer back to the channel server this belongs to.
     */
    MatchManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the MatchManager
     */
    virtual ~MatchManager();

    /**
     * Get current match entry associated to a world CID
     * @param cid World CID of the entry to retrieve
     * @return Pointer to the match entry or null if none exists
     */
    std::shared_ptr<objects::MatchEntry> GetMatchEntry(int32_t cid);

    /**
     * Queue the supplied client for a match of the specified type
     * @param client Pointer to the client connection
     * @param type Match type to join
     */
    bool JoinQueue(const std::shared_ptr<ChannelClientConnection>& client,
        int8_t type);

    /**
     * Remove the supplied client from the match queue
     * @param client Pointer to the client connection
     */
    bool CancelQueue(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Confirm entrance into a currently ready match
     * @param client Pointer to the client connection
     */
    void ConfirmMatch(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Reject entrance into a currently ready match
     * @param client Pointer to the client connection
     */
    void RejectMatch(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Request to leave the client's team
     * @param client Pointer to the client connection
     * @param teamID ID of the team being left
     */
    void LeaveTeam(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t teamID);

    /**
     * Validate and request a custom PvP match formed from the player's current
     * team
     * @param client Pointer to the client connection
     * @param variantID ID of the variant to create
     * @param instanceID ID of the instance to create
     * @return true if the validation succeeded and the request was sent to the
     *  world server, false if the process failed
     */
    bool RequestTeamPvPMatch(
        const std::shared_ptr<ChannelClientConnection>& client,
        uint32_t variantID, uint32_t instanceID);

    /**
     * Expire all pending invites to a PvP match
     * @param matchID Match ID to expire access to
     */
    void ExpirePvPAccess(uint32_t matchID);

    /**
     * Respond to a PvP invite with with either an acceptane or rejection.
     * If the invite is rejected, the player's penalty count will be increased.
     * @param client Pointer to the client connection
     * @param matchID Match ID to reply to
     * @param accept If true the invite is being accepted
     * @return true if the response was valid, false if it was not
     */
    bool PvPInviteReply(const std::shared_ptr<ChannelClientConnection>& client,
        uint32_t matchID, bool accept);

    /**
     * Get or create the specified client's PvPData
     * @param client Pointer to the client connection
     * @param create Optional parameter to create the data if it does not exist
     * @return Pointer to the PvPData, null if it was not created
     */
    std::shared_ptr<objects::PvPData> GetPvPData(
        const std::shared_ptr<ChannelClientConnection>& client,
        bool create = false);

    /**
     * Start the PvP match in the specified instance
     * @param instanceID ID of the instance to start the match in
     */
    void StartPvPMatch(uint32_t instanceID);

    /**
     * End the PvP match in the specified instance. If the timer has not
     * been stopped already, this will fail. All updates are performed here
     * and results are then sent to the players still in the match.
     * @param instanceID ID of the instance to stop the match in
     * @return true if the match was stopped, false if it was not
     */
    bool EndPvPMatch(uint32_t instanceID);

    /**
     * Notify all PvP entries that a player has joined or changed zones and
     * send the new player the PvP match info
     * @param client Pointer to the client connection
     * @param instanceID ID of the instance the player should be in
     */
    void EnterPvP(const std::shared_ptr<ChannelClientConnection>& client,
        uint32_t instanceID);

    /**
     * Request to start capturing a base in the instance
     * @param client Pointer to the client connection
     * @param baseID Entity ID of the base to capture
     * @return true if the base capture was valid
     */
    bool CaptureBase(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t baseID);

    /**
     * Request to stop capturing a base in the instance
     * @param client Pointer to the client connection
     * @param baseID Entity ID of the base to leave
     * @return true if the base capture cancel was valid
     */
    bool LeaveBase(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t baseID);

    /**
     * Complete capturing a base in the instance and grant points if applicable
     * @param entityID ID of the entity capturing the base
     * @param baseID Entity ID of the base to capture
     * @param zoneID Unique ID of the zone the entity is in
     * @param instanceID Unique ID of the instance the entity is in
     * @param occupyStartTime Start time of the entity's base occupation. If
     *  this does not match the value on the base, the capture will fail.
     */
    void CompleteBaseCapture(int32_t entityID, int32_t baseID, uint32_t zoneID,
        uint32_t instanceID, uint64_t occupyStartTime);

    /**
     * Increase a captured base's bonus in the instance and grant points if
     * applicable
     * @param baseID Entity ID of the base to increase
     * @param zoneID Unique ID of the zone the entity is in
     * @param instanceID Unique ID of the instance the base is in
     * @param occupyStartTime Start time of the entity's base occupation. If
     *  this does not match the value on the base, the increase will fail.
     */
    void IncreaseBaseBonus(int32_t baseID, uint32_t zoneID,
        uint32_t instanceID, uint64_t occupyStartTime);

    /**
     * Update PvP match points for a specific source and target entity
     * @param instanceID ID of the instance where the match is taking place
     * @param source Pointer to the source of the point update
     * @param target Pointer to the target of the point update
     * @param points Number of points to add (or subtract)
     * @return Total number of points that were added
     */
    int32_t UpdatePvPPoints(
        uint32_t instanceID, std::shared_ptr<ActiveEntityState> source,
        std::shared_ptr<ActiveEntityState> target, int32_t points);

    /**
     * Update PvP match points
     * @param instanceID ID of the instance where the match is taking place
     * @param sourceID Entity ID of the source of the point update
     * @param targetID Entity ID of the target of the point update
     * @param points Number of points to add (or subtract)
     * @param altMode Alternative point mode indicator that changes how
     *  the client displays the point notification
     * @return Total number of points that were added
     */
    int32_t UpdatePvPPoints(uint32_t instanceID, int32_t sourceID,
        int32_t targetID, uint8_t teamID, int32_t points, bool altMode);

    /**
     * Special match handler for any primary player entities that were killed
     * during a match. This is responsible for doing things like scheduling
     * auto-revival.
     * @param entity Pointer to the player entity that was killed
     * @param instance Pointer to the instance the player was in when they
     *  were killed
     */
    void PlayerKilled(const std::shared_ptr<ActiveEntityState>& entity,
        const std::shared_ptr<ZoneInstance>& instance);

    /**
     * Notify all PvP entries that a player has entered or left the instance
     * @param client Pointer to the client connection
     * @param instanceID ID of the instance affected
     * @param enter true if the instance is being entered, false if it is
     *  being left
     */
    void SendPvPLocation(const std::shared_ptr<ChannelClientConnection>& client,
        uint32_t instanceID, bool enter);

    /**
     * Synchronization handler for match queue entries that have been updated
     * by the world server. This is responsible for relaying packet information
     * to the player when requests are sent to the world.
     * @param updates List of pointers to match entry updates
     * @param removes List of pointers to match entry removes
     */
    void UpdateMatchEntries(
        const std::list<std::shared_ptr<objects::MatchEntry>>& updates,
        const std::list<std::shared_ptr<objects::MatchEntry>>& removes);

    /**
     * Synchronization handler for PvP matches that have been updated (or
     * created) by the world server. This is responsible for preparing matches
     * after the world has built or updated them.
     * @param matches List of pointers to PvP match updates
     */
    void UpdatePvPMatches(
        const std::list<std::shared_ptr<objects::PvPMatch>>& matches);

    /**
     * Determine if the supplied instance has an active PvP match being
     * played. Both pre and post timer active times will result in the
     * match reporting as inactive.
     * @param instance Pointer to the instance to check
     * @return true if the match is active, false if it is not
     */
    static bool PvPActive(const std::shared_ptr<ZoneInstance>& instance);

    /**
     * Determine if the supplied entity is in a PvP team based upon the
     * assigned faction group.
     * @param entity Pointer to the entity to check
     * @return true if the entity is in a team, false if they are not
     */
    static bool InPvPTeam(const std::shared_ptr<ActiveEntityState>& entity);

private:
    /**
     * Create a new PvP instance from the supplied match information
     * @param match Pointer to the PvP match definition
     * @return true if the match was created, false if was not
     */
    bool CreatePvPInstance(const std::shared_ptr<objects::PvPMatch>& match);

    /**
     * Calculate and assign all trophies earned during an instance's PvP match
     * to the entity statistics for that instance
     * @param instance Pointer to the instance
     */
    void GetPvPTrophies(const std::shared_ptr<ZoneInstance>& instance);

    /**
     * Determine if the supplied player clients are valid for match entry
     * creation.
     * @param clients Pointer to clients attempting to enter the queue
     * @param teamCategory Category of the type of team that would be created
     *  (even if the entry list is a solo player)
     * @param isTeam true if the clients are going to be part of a team,
     *  false if it is a solo entry
     * @param checkPenalties true if the number of PvP penalties that exist
     *  should be checked, assuming this is a PvP match request
     * @return true if the players validated, false if they did not
     */
    bool ValidateMatchEntries(
        const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
        int8_t teamCategory, bool isTeam, bool checkPenalties);

    /**
     * Move the supplied client to the PvP match instance after determining
     * their starting position based upon their team
     * @param client Pointer to client connection
     * @param match Pointer to the PvP match definition
     * @return true if the move was successful, false if it was not
     */
    bool MoveToInstance(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<objects::PvPMatch>& match);

    /**
     * Queue the next asynchronous base bonus increase in a match instance
     * @param baseID Entity ID of the base to update
     * @param zone Pointer to the zone the base is in
     * @param occupyStartTime Start time of the entity's base occupation
     */
    void QueueNextBaseBonus(int32_t baseID, const std::shared_ptr<Zone>& zone,
        uint64_t occupyStartTime);

    /**
     * Determine if the supplied instance has active team members on both
     * sides of a match
     * @param instance Pointer to the instance to check
     * @return true if both teams are active, false if at least one is not
     */
    static bool MatchTeamsActive(const std::shared_ptr<ZoneInstance>& instance);

    /// Local channel server copy of all current match queue entrants that
    /// are synchronized with the world server
    std::unordered_map<int32_t,
        std::shared_ptr<objects::MatchEntry>> mMatchEntries;

    /// Time limited map of system expiration times to world CIDs of players
    /// that have been sent PvP match invitations. Useful for expiring players
    /// who do not respond in time.
    std::unordered_map<uint32_t, std::set<int32_t>> mPendingPvPInvites;

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_MATCHMANAGER_H
