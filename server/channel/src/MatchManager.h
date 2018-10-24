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
class PentalphaEntry;
class PentalphaMatch;
class PvPData;
class PvPMatch;
class UBResult;
class UBTournament;
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
     * @param matchID ID of the match sent to the client for UB matches
     */
    void ConfirmMatch(const std::shared_ptr<ChannelClientConnection>& client,
        uint32_t matchID = 0);

    /**
     * Reject entrance into a currently ready match
     * @param client Pointer to the client connection
     */
    void RejectPvPMatch(const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Request to leave the client's team
     * @param client Pointer to the client connection
     * @param teamID ID of the team being left
     */
    void LeaveTeam(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t teamID);

    /**
     * Update the supplied client's Ultimate Battle points for the current
     * tournament
     * @param client Pointer to the client connection
     * @param adjust Number of points to add
     * @return true if the update was valid, false if it was not
     */
    bool UpdateUBPoints(const std::shared_ptr<ChannelClientConnection>& client,
        int32_t adjust);

    /**
     * Update the small and large ziotite values for a team. Updates set
     * here will be sent to the world which has the final say for if the
     * update was valid.
     * @param team Pointer to the team to update
     * @param sZiotite Amount of small ziotite to add
     * @param lZiotite Amount of large ziotite to add
     * @param worldCID World CID of the player performing the update
     * @return true if the update was valid, false if it was not
     */
    bool UpdateZiotite(const std::shared_ptr<objects::Team>& team,
        int32_t sZiotite, int8_t lZiotite, int32_t worldCID = 0);

    /**
     * Send the supplied team's ziotite values to the supplied client or
     * the full team
     * @param team Pointer to the team
     * @param client Optional parameter to send the update to only one
     *  client instead of the full team
     */
    void SendZiotite(const std::shared_ptr<objects::Team>& team,
        const std::shared_ptr<ChannelClientConnection>& client = nullptr);

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
     * Queue the supplied client for the Ultimate Battle match in the specified
     * zone
     * @param worldCID World CID of the player to queue
     * @param zone Pointer to the zone with the pending match
     */
    bool JoinUltimateBattleQueue(int32_t worldCID,
        const std::shared_ptr<Zone>& zone);

    /**
     * Expire all pending invites to a PvP match
     * @param matchID Match ID to expire access to
     */
    void ExpirePvPAccess(uint32_t matchID);

    /**
     * Remove the supplied client from their current match and end the match
     * if configured to do so when no members exist
     * @param client Pointer to the client connection
     * @return true if the update occurred, false if it did not
     */
    bool CleanupPendingMatch(
        const std::shared_ptr<ChannelClientConnection>& client);

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
     * Notify all Diaspora entries that a player has joined or changed zones and
     * send the new player match information
     * @param client Pointer to the client connection
     * @param zone Pointer to the zone in the instance that is being entered
     */
    void EnterDiaspora(const std::shared_ptr<ChannelClientConnection>& client,
        const std::shared_ptr<Zone>& zone);

    /**
     * Notify all UB entries that a player has joined if they are a participant
     * and send the new player match information. If the player is spectator,
     * they will be hidden and sent appropriate match information.
     * @param client Pointer to the client connection
     * @param zone Pointer to the zone the player is entering
     */
    void EnterUltimateBattle(const std::shared_ptr<
        ChannelClientConnection>& client, const std::shared_ptr<Zone>& zone);

    /**
     * Notify all UB entries that a player has left if they are a participant
     * @param client Pointer to the client connection
     * @param zone Pointer to the zone the player is leaving
     */
    void LeaveUltimateBattle(const std::shared_ptr<
        ChannelClientConnection>& client, const std::shared_ptr<Zone>& zone);

    /**
     * Request to start capturing a PvP base in the instance
     * @param client Pointer to the client connection
     * @param baseID Entity ID of the PvP base to capture
     * @return true if the base capture was valid
     */
    bool StartPvPBaseCapture(const std::shared_ptr<
        ChannelClientConnection>& client, int32_t baseID);

    /**
     * Request to stop capturing a PvP base in the instance
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
     * Capture or reset a Diaspora base in the supplied zone
     * @param zone Pointer to the zone the base is in
     * @param baseID Entity ID of the base being toggled
     * @param sourceEntityID ID of the entity affecting the base
     * @param capture true if the base is being captured, false if it is
     *  being reset
     * @return true if the base was updated, false if it was not
     */
    bool ToggleDiasporaBase(const std::shared_ptr<Zone>& zone,
        int32_t baseID, int32_t sourceEntityID, bool capture);

    /**
     * Handler for Diaspora base automatic reset
     * @param zoneID Unique zone ID in the instance
     * @param instanceID ID of the zone instance the base is in
     * @param baseID Entity ID of the base
     * @param resetTime Timestamp that should match the capture time on the
     *  base. If it does not the operation will fail
     * @return true if the base was updated, false if it was not
     */
    bool ResetDiasporaBase(uint32_t zoneID, uint32_t instanceID,
        int32_t baseID, uint64_t resetTime);

    /**
     * Start or stop a match associated to the supplied zone, its instance
     * and other zones if they exist. Ultimate Battle matches are restricted
     * to only the supplied zone.
     * @param zone Pointer to the zone to update
     * @match Pointer to the match (or null) to set on the zone(s) and instance
     * @return true if the update was successful, false if nothing changed
     */
    bool StartStopMatch(const std::shared_ptr<Zone>& zone,
        const std::shared_ptr<objects::Match>& match);

    /**
     * Move the Ultimate Battle match in the supplied zone from the Pre-Match
     * to Ready state and send out any queue/recruit notifications or remove
     * the match if no one is currently attempting to enter. The match timer
     * will not actually begin until the first player enters the zone.
     * @param zone Pointer to the zone to update
     * @return true if the match started, false if it did not
     */
    bool StartUltimateBattle(const std::shared_ptr<Zone>& zone);

    /**
     * Send a recruit request to players still waiting in the queue who were
     * not selected for the initial set. If the match is still full of players,
     * the queue members will be notified accordingly.
     * @param zoneID Definition ID of the zone to update
     * @param dynamicMapID Dynamic Map ID of the zone to update
     * @param instanceID Instance ID of the zone to update
     */
    void UltimateBattleRecruit(uint32_t zoneID, uint32_t dynamicMapID,
        uint32_t instanceID);

    /**
     * Kick off the Ultimate Battle timer and begin the match. If no members
     * are currently there, the match will end instead.
     * @param zoneID Definition ID of the zone to update
     * @param dynamicMapID Dynamic Map ID of the zone to update
     * @param instanceID Instance ID of the zone to update
     */
    void UltimateBattleBegin(uint32_t zoneID, uint32_t dynamicMapID,
        uint32_t instanceID);

    /**
     * Register the supplied world CID as an Ultimate Battle spectator
     * for the specified zone's match. The player still has to enter
     * the zone after calling this.
     * @param worldCID World CID of the player that will spectate
     * @param zone Pointer to the zone with an active or pending match
     * @return true if the player was registered successfully, false if
     *  they were not
     */
    bool UltimateBattleSpectate(int32_t worldCID,
        const std::shared_ptr<Zone>& zone);

    /**
     * Start a new timer for an Ultimate Battle match
     * @param zone Pointer to the zone with an active match
     * @param duration Duration of the timer
     * @param eventID ID of the event to fire when the timer expires
     * @param endPhase true if the phase is ending and the results should
     *  be sent, false if only the timer scheduling should be done
     * @return true if the timer was set, false if it was not
     */
    bool StartUltimateBattleTimer(const std::shared_ptr<Zone>& zone,
        uint32_t duration, const libcomp::String& eventID, bool endPhase);

    /**
     * Advance the supplied zone's current phase
     * @param zone Pointer to the zone with a match
     * @param to New phase to move to, defaults to "next phase"
     * @param from Phase to move from, defaults to "current phase"
     * @return true if the phase was updated, false if it was not
     */
    bool AdvancePhase(const std::shared_ptr<Zone>& zone, int8_t to = -1,
        int8_t from = -1);

    /**
     * Pending Ultimate Battle queue handler that notifies players in the
     * lobby before the match begins and causes the match to start
     * @param zoneID Definition ID of the zone with a match
     * @param dynamicMapID Dynamic Map ID of the zone with a match
     * @param instanceID Instance ID of the zone with a match
     */
    void UltimateBattleQueue(uint32_t zoneID, uint32_t dynamicMapID,
        uint32_t instanceID);

    /**
     * Active Ultimate Battle "tick" handler that triggers actions for
     * spawns and adjusts the gauge for non-UA matches
     * @param zoneID Definition ID of the zone with a match
     * @param dynamicMapID Dynamic Map ID of the zone with a match
     * @param instanceID Instance ID of the zone with a match
     */
    void UltimateBattleTick(uint32_t zoneID, uint32_t dynamicMapID,
        uint32_t instanceID);

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
     * Notify all Diaspora entries that a player has entered or left the instance
     * @param client Pointer to the client connection
     * @param instanceID ID of the instance affected
     * @param enter true if the instance is being entered, false if it is
     *  being left
     * @param others Optional parameter to perform the inverse notification to
     *  other members
     */
    void SendDiasporaLocation(const std::shared_ptr<ChannelClientConnection>& client,
        uint32_t instanceID, bool enter, bool others = false);

    /**
     * Notify the client of their current Ultimate Battle rank information
     * @param client Pointer to the client connection
     */
    void SendUltimateBattleRankings(
        const std::shared_ptr<ChannelClientConnection>& client);

    /**
     * Get the currently active Ultimate Battle Tournament
     * @return Pointer to the active Ultimate Battle Tournament
     */
    std::shared_ptr<objects::UBTournament> GetUBTournament() const;

    /**
     * Set the active Ultimate Battle Tournament and reload all ranking
     * information
     * @param tournament Pointer to the active Ultimate Battle Tournament
     */
    void UpdateUBTournament(const std::shared_ptr<
        objects::UBTournament>& tournament);

    /**
     * Get the currently active (or previous) Pentalpha Match
     * @param previous If true the previous active match will be returned, if
     *  false the current active match will be returned
     * @return Pointer to a Pentalpha Match
     */
    std::shared_ptr<objects::PentalphaMatch> GetPentalphaMatch(bool previous);

    /**
     * Set the active Pentalpha Match and reload all entry info
     * @param match Pointer to the active Pentalpha Match
     */
    void UpdatePentalphaMatch(const std::shared_ptr<
        objects::PentalphaMatch>& match);

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
     * Synchronization handler for Ultimate Battle results that have been
     * updated by the world server. If none are specified, the entire ranking
     * dataset will be reloaded
     * @param updated List of pointers to UBResults
     */
    void UpdateUBRankings(
        const std::list<std::shared_ptr<objects::UBResult>>& updated);

    /**
     * Load and cache all Ultimate Battle information associated to the client
     * @param client Pointer to the client connection
     * @param idxFlags Flags indicating which information to load
     *  0x01) Current results
     *  0x02) All time results
     * @param createMissing If true, any missing record requested will be
     *  created
     * @return Pointer to the first requested result, can be null
     */
    std::shared_ptr<objects::UBResult> LoadUltimateBattleData(
        const std::shared_ptr<ChannelClientConnection>& client,
        uint8_t idxFlags = 0x03, bool createMissing = false);

    /**
     * Load and cache all Pentalpha Match information associated to the client
     * @param client Pointer to the client connection
     * @param idxFlags Flags indicating which information to load
     *  0x01) Current match
     *  0x02) Previous match
     * @return Pointer to the first requested entry, can be null
     */
    std::shared_ptr<objects::PentalphaEntry> LoadPentalphaData(
        const std::shared_ptr<ChannelClientConnection>& client,
        uint8_t idxFlags = 0x03);

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

    /**
     * Determine if the supplied client is spectating a zone's Ultimate
     * Battle match
     * @param client Pointer to the client connection
     * @param zone Pointer to the zone to check
     * @return true if they are configured to spectate, false if they are not
     */
    static bool SpectatingMatch(const std::shared_ptr<
        ChannelClientConnection>& client, const std::shared_ptr<Zone>& zone);

private:
    /**
     * Create a new PvP instance from the supplied match information
     * @param match Pointer to the PvP match definition
     * @return true if the match was created, false if was not
     */
    bool CreatePvPInstance(const std::shared_ptr<objects::PvPMatch>& match);

    /**
     * Create a PvPMatch with all pre-instance creation values and send it
     * to the world to use if the match begins at the specified time. Only
     * one channel needs to handle this when new ready times are seen.
     * @param type Standard PvP match type
     * @param readyTime Match ready time
     */
    void QueuePendingPvPMatch(uint8_t type, uint32_t readyTime);

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
     * Perform all end of phase actions for an Ultimate Battle zone for the
     * current phase
     * @param zone Pointer to a zone with a match
     * @param matchOver If true the match ending actions will be performed as
     *  well such as registering UB points with the world
     * @return true if the phase was ended succesfully, false if an error
     *  occurred
     */
    bool EndUltimateBattlePhase(const std::shared_ptr<Zone>& zone,
        bool matchOver);

    /**
     * Perform all end of match actions for an Ultimate Battle zone
     * @param zone Pointer to a zone with a match
     */
    void EndUltimateBattle(const std::shared_ptr<Zone>& zone);

    /**
     * Queue the next asynchronous base bonus increase in a match instance
     * @param baseID Entity ID of the base to update
     * @param zone Pointer to the zone the base is in
     * @param occupyStartTime Start time of the entity's base occupation
     */
    void QueueNextBaseBonus(int32_t baseID, const std::shared_ptr<Zone>& zone,
        uint64_t occupyStartTime);

    /**
     * Fire phase triggers associated to the supplied zone and any of
     * its related zones or instance
     * @param zone Pointer to a zone with a match
     * @param phase Phase to fire triggers for
     */
    void FirePhaseTriggers(const std::shared_ptr<Zone>& zone, int8_t phase);

    /**
     * Notify all players in a match zone of the current phase
     * @param zone Pointer to a zone with a match
     * @param timerStart Optional parameter to snap the timer reported to
     *  the full time instead of current time left for better client sync
     * @param client Pointer to client connection to send to instead of
     *  the entire zone
     */
    void SendPhase(const std::shared_ptr<Zone>& zone, bool timerStart = false,
        const std::shared_ptr<ChannelClientConnection>& client = nullptr);

    /**
     * Notify players in an Ultimate Battle zone of the current match 
     * participants. This is also how the client acknowledges spectating.
     * @param zone Pointer to a zone with a match
     * @param client Pointer to client connection to send to instead of
     *  the entire zone
     */
    void SendUltimateBattleMembers(const std::shared_ptr<Zone>& zone,
        const std::shared_ptr<ChannelClientConnection>& client = nullptr);

    /**
     * Notify players in an Ultimate Battle zone of the current match 
     * participant states
     * @param zone Pointer to a zone with a match
     * @param client Pointer to client connection to send to instead of
     *  the entire zone
     */
    void SendUltimateBattleMemberState(const std::shared_ptr<Zone>& zone,
        const std::shared_ptr<ChannelClientConnection>& client = nullptr);

    /**
     * Determine if the supplied instance has active team members on both
     * sides of a match
     * @param instance Pointer to the instance to check
     * @return true if both teams are active, false if at least one is not
     */
    static bool MatchTeamsActive(const std::shared_ptr<ZoneInstance>& instance);

    /**
     * Determine if the supplied match entry is a PvP entry
     * @param entry Pointer to the MatchEntry to check
     * @return true if the entry is a PvP entry
     */
    static bool IsPvPMatchEntry(const std::shared_ptr<objects::MatchEntry>& entry);

    /// Local channel server copy of all current match queue entrants that
    /// are synchronized with the world server
    std::unordered_map<int32_t,
        std::shared_ptr<objects::MatchEntry>> mMatchEntries;

    /// Time limited map of system expiration times to world CIDs of players
    /// that have been sent PvP match invitations. Useful for expiring players
    /// who do not respond in time.
    std::unordered_map<uint32_t, std::set<int32_t>> mPendingPvPInvites;

    /// Pointer to the current active UBTournament
    std::shared_ptr<objects::UBTournament> mUBTournament;

    /// Map of currently reported UB top 10 rankings in the following order:
    /// all time total, top points, previous tournament, current tournament
    std::array<std::array<std::shared_ptr<objects::UBResult>, 10>, 4> mUBRankings;

    /// Pointers to the current and previous Pentalpha Matches
    std::array<std::shared_ptr<objects::PentalphaMatch>, 2> mPentalphaMatches;

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_MATCHMANAGER_H
