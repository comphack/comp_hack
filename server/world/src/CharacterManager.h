/**
 * @file server/world/src/CharacterManager.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Manager to handle world level character actions.
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

#ifndef SERVER_WORLD_SRC_CHARACTERMANAGER_H
#define SERVER_WORLD_SRC_CHARACTERMANAGER_H

// Standard C++11 Includes
#include <unordered_map>

// object Includes
#include <CharacterLogin.h>
#include <Clan.h>
#include <ClanInfo.h>
#include <Party.h>
#include <PartyCharacter.h>
#include <Team.h>

/// Related characters should be retrieved from the friends list
const uint8_t RELATED_FRIENDS = 0x01;

/// Related characters should be retrieved from the current party
const uint8_t RELATED_PARTY = 0x02;

/// Related characters should be retrieved from the same clan
const uint8_t RELATED_CLAN = 0x04;

/// Related characters should be retrieved from the same team
const uint8_t RELATED_TEAM = 0x08;

namespace libcomp
{
class Packet;
class TcpConnection;
}

namespace world
{

class WorldServer;

/**
 * Manages world level character actions.
 */
class CharacterManager
{
public:
    /**
     * Create a new CharacterManager
     * @param server Pointer back to the world server this
     *  belongs to
     */
    CharacterManager(const std::weak_ptr<WorldServer>& server);

    /**
     * Register a CharacterLogin with the manager. Characters registered here will
     * remain until the server restarts unless they are deleted.
     * @param cLogin CharacterLogin to register
     * @return Pointer to the CharacterLogin registered with the server. null is
     *  never returned and the value sent back should always replace the value
     *  passed in to keep the servers in sync
     */
    std::shared_ptr<objects::CharacterLogin> RegisterCharacter(
        std::shared_ptr<objects::CharacterLogin> cLogin);

    /**
     * Unregister a CharacterLogin with the manager. This should only be used
     * if the character is being deleted.
     * @param cLogin CharacterLogin to unregister
     * @return true if the character was removed, false if they were not
     *  registered
     */
    bool UnregisterCharacter(std::shared_ptr<objects::CharacterLogin> cLogin);

    /**
     * Retrieve a CharacterLogin registered with the server by UUID
     * @param uuid UUID associated to the character
     * @return Pointer to the CharacterLogin registered with the server
     */
    std::shared_ptr<objects::CharacterLogin> GetCharacterLogin(
        const libobjgen::UUID& uuid);

    /**
     * Retrieve a CharacterLogin registered with the server by world CID
     * @param worldCID World CID associated to the character
     * @return Pointer to the CharacterLogin registered with the server
     */
    std::shared_ptr<objects::CharacterLogin> GetCharacterLogin(int32_t worldCID);

    /**
     * Retrieve a CharacterLogin registered with the server by name
     * @param characterName Name of the character
     * @return Pointer to the CharacterLogin registered with the server
     */
    std::shared_ptr<objects::CharacterLogin> GetCharacterLogin(
        const libcomp::String& characterName);

    /**
     * Retrieve all currently active CharacterLogins
     * @return List of pointers to active CharacterLogins
     */
    std::list<std::shared_ptr<objects::CharacterLogin>> GetActiveCharacters();

    /**
     * Match the supplied world CID with a registered character login and
     * send a request to the channel they are currently logged into to
     * disconnect that account
     * @param worldCID World CID associated to the character
     * @return true if the request was sent, false if either no channel exists
     *  or no channel is has the character
     */
    bool RequestChannelDisconnect(int32_t worldCID);

    /**
     * Send a packet to the specified logins
     * @param p Packet to send
     * @param cLogins CharacterLogins to map channel connections to when sending
     *  the packet
     * @param cidOffset Position in bytes after the packet code where the list
     *  of CIDs should be inserted. If the value is larger than the packet, it will
     *  be appended to the end.
     * @return false if an error occurs
     */
    bool SendToCharacters(libcomp::Packet& p,
        const std::list<std::shared_ptr<objects::CharacterLogin>>& cLogins,
        uint32_t cidOffset);

    /**
     * Insert space in a packet for a count denoted list of world CID targets and
     * seek to the position of the first CID in the list.
     * @param p Packet to convert
     * @param cidOffset Position in bytes after the packet code where the list
     *  of CIDs should be inserted
     * @param cidCount Number of CIDs that space needs to be allocated for
     */
    void ConvertToTargetCIDPacket(libcomp::Packet& p, uint32_t cidOffset, size_t cidCount);

    /**
     * Send a packet to various characters related to the supplied world CID
     * @param p Packet to send
     * @param worldCID World CID used to find related characters
     * @param cidOffset Position in bytes after the packet code where the list
     *  of CIDs should be inserted. If the value is larger than the packet, it will
     *  be appended to the end.
     * @param relatedTypes Flags indicating what types of related characters to send
     *  the packet to. Use the constants in the CharacterManager header to signify these.
     * @param includeSelf Optional parameter to include the character associated
     *  to the world CID. Defaults to false.
     * @param zoneRestrict Optional parameter to restrict the characters being
     *  retrieved to ones in the same zone as the supplied CID. Defaults to false.
     * @return false if an error occurs
     */
    bool SendToRelatedCharacters(libcomp::Packet& p, int32_t worldCID,
        uint32_t cidOffset, uint8_t relatedTypes, bool includeSelf = false,
        bool zoneRestrict = false);

    /**
     * Retrieves characters related to the supplied CharacterLogin
     * @param cLogin CharacterLogin to find related characters for
     * @param relatedTypes Flags indicating what types of related characters to send
     *  the packet to. Use the constants in the CharacterManager header to signify these.
     * @return List of pointers to the related CharacterLogins
     */
    std::list<std::shared_ptr<objects::CharacterLogin>>
        GetRelatedCharacterLogins(std::shared_ptr<objects::CharacterLogin> cLogin,
            uint8_t relatedTypes);

    /**
     * Send packets containing CharacterLogin information about the supplied logins
     * contextual to other related characters
     * @param cLogins CharacterLogins to send information about
     * @param updateFlags CharacterLogin packet specific flags indicating what is
     *  being sent.  Information will be filtered out in this function as needed.
     *  For example if party information is being sent but the character is not in
     *  a party, that specific character's part information will not be sent.  If
     *  the filtering results in no flags, no data will be sent.
     * @param zoneRestrict Optional parameter to restrict the characters being
     *  notified to ones in the same zone as the supplied CID. Defaults to false.
     */
    void SendStatusToRelatedCharacters(
        const std::list<std::shared_ptr<objects::CharacterLogin>>& cLogins,
        uint8_t updateFlags, bool zoneRestrict = false);

    /**
     * Builds a status packet associated to the supplied CharacterLogin to be sent
     * to related characters
     * @param p Packet to write the data to
     * @param cLogin CharacterLogin to send information about
     * @param updateFlags CharacterLogin packet specific flags indicating what is
     *  being sent.  Information will be filtered out in this function as needed.
     * @return true if data was written, false if no data applied
     */
    bool GetStatusPacket(libcomp::Packet& p,
        const std::shared_ptr<objects::CharacterLogin>& cLogin, uint8_t& updateFlags);

    /**
     * Get an active party by ID
     * @param partyID ID of the party to retrieve
     * @return Pointer to the active party
     */
    std::shared_ptr<objects::Party> GetParty(uint32_t partyID);

    /**
     * Get an active or pending party member by CharacterLogin
     * @param worldCID World CID of the party member info to retrieve
     * @return Pointer to the party member
     */
    std::shared_ptr<objects::PartyCharacter> GetPartyMember(int32_t worldCID);

    /**
     * Add a party member to the specified party
     * @param member Pointer to the party member to add
     * @param partyID ID of the party to add the member to
     * @return true on success, false on failure
     */
    bool AddToParty(std::shared_ptr<objects::PartyCharacter> member,
        uint32_t partyID);

    /**
     * Attempt to have a party member join the specified party
     * @param member Pointer to the party member that will join. All
     *  necessary packet communication is handled within.
     * @param targetName Name of a character that sent a join request,
     *  can be left blank if joining directly
     * @param partyID ID of the party to add the member to
     * @param sourceConnection Connection associated to the member
     *  joining the party
     * @return true on success, false on failure
     */
    bool PartyJoin(std::shared_ptr<objects::PartyCharacter> member,
        const libcomp::String targetName, uint32_t partyID,
        std::shared_ptr<libcomp::TcpConnection> sourceConnection);

    /**
     * Attempt to have a party member join the specified party via
     * a recruit request (which is the inverse of a join request)
     * @param member Pointer to the party member that will join. All
     *  necessary packet communication is handled within.
     * @param targetName Name of a character that sent a recruit request
     * @param partyID ID of the party to add the member to
     * @param sourceConnection Connection associated to the member
     *  joining the party
     * @return true on success, false on failure
     */
    bool PartyRecruit(std::shared_ptr<objects::PartyCharacter> member,
        const libcomp::String targetName, uint32_t partyID,
        std::shared_ptr<libcomp::TcpConnection> sourceConnection);

    /**
     * Attempt to have a party member leave their current party. All
     * necessary packet communication is handled within.
     * @param member Pointer to the CharacterLogin representation of
     *  the party member leaving
     * @param requestConnection Optional parameter for the connection
     *  that sent the request
     */
    void PartyLeave(std::shared_ptr<objects::CharacterLogin> cLogin,
        std::shared_ptr<libcomp::TcpConnection> requestConnection);

    /**
     * Attempt to disband the specified party. All necessary packet
     * communication is handled within.
     * @param partyID ID of the party to disband
     * @param sourceCID Optional ID of character who is disbanding the party
     * @param requestConnection Optional parameter for the connection
     *  that sent the request
     */
    void PartyDisband(uint32_t partyID, int32_t sourceCID = 0,
        std::shared_ptr<libcomp::TcpConnection> requestConnection = nullptr);

    /**
     * Attempt to set the leader of the specified party. All necessary packet
     * communication is handled within.
     * @param partyID ID of the party to update
     * @param sourceCID Optional ID of character who is setting the leader
     * @param requestConnection Optional parameter for the connection
     *  that sent the request
     * @param targetCID CID of the new leader to set
     */
    void PartyLeaderUpdate(uint32_t partyID, int32_t sourceCID,
        std::shared_ptr<libcomp::TcpConnection> requestConnection, int32_t targetCID);

    /**
     * Attempt to kick a player from their current party. All necessary packet
     * communication is handled within.
     * @param cLogin CharacterLogin of the character who sent the request
     * @param targetCID CID of the character being kicked
     */
    void PartyKick(std::shared_ptr<objects::CharacterLogin> cLogin, int32_t targetCID);

    /**
     * Send base level info about the specified party ID to every member to
     * act as a refresh for channel level drop rule and member info.
     * @param partyID ID of the party to update
     * @param cids Additional CIDs to send to, useful when removing members
     */
    void SendPartyInfo(uint32_t partyID, const std::list<int32_t>& cids = {});

    /**
     * Get the clan info associated to the specified clan ID.
     * @param clanID Clan instance ID
     * @param Pointer to the related clan info or null if it does not exist
     */
    std::shared_ptr<objects::ClanInfo> GetClan(int32_t clanID);

    /**
     * Get (and register if needed) the clan info associated to the specified
     * clan UUID.
     * @param uuid UUID of the clan to retrieve
     * @param Pointer to the related clan info
     */
    std::shared_ptr<objects::ClanInfo> GetClan(const libobjgen::UUID& uuid);

    /**
     * Add the specified character to an existing clan
     * @param cLogin Pointer to the CharacterLogin to add to the clan
     * @param clanID Clan instance ID
     * @return true if the character joined successfully, false if they did not
     */
    bool ClanJoin(std::shared_ptr<objects::CharacterLogin> cLogin, int32_t clanID);

    /**
     * Remove the specified character from an existing clan
     * @param cLogin Pointer to the CharacterLogin to remove from the clan
     * @param clanID Clan instance ID
     * @param requestConnection Pointer to the connection requesting the change
     * @return true if the character was successfully removed, false if they
     *  were not
     */
    bool ClanLeave(std::shared_ptr<objects::CharacterLogin> cLogin, int32_t clanID,
        std::shared_ptr<libcomp::TcpConnection> requestConnection);

    /**
     * Disband an existing clan
     * @param clanID Clan instance ID
     * @param sourceCID World CID of the character requesting the disband. This is
     *  only checked when the requestConnection is specified as well
     * @param requestConnection Pointer to the connection requesting the change
     */
    void ClanDisband(int32_t clanID, int32_t sourceCID = 0,
        std::shared_ptr<libcomp::TcpConnection> requestConnection = nullptr);

    /**
     * Kick a character from a clan
     * @param cLogin Pointer to the CharacterLogin requesting the change
     * @param clanID Clan instance ID
     * @param targetCID World CID of the character to kick
     * @param requestConnection Pointer to the connection requesting the change
     */
    void ClanKick(std::shared_ptr<objects::CharacterLogin> cLogin, int32_t clanID,
        int32_t targetCID, std::shared_ptr<libcomp::TcpConnection> requestConnection);

    /**
     * Send clan level or clan member level details to the specified character
     * @param cLogin Pointer to the CharacterLogin requesting the information
     * @param requestConnection Pointer to the connection requesting the change
     * @param memberIDs Optional list of member world CIDs being requested. If
     *  this is left empty, the clan level information will be sent instead.
     */
    void SendClanDetails(std::shared_ptr<objects::CharacterLogin> cLogin,
        std::shared_ptr<libcomp::TcpConnection> requestConnection,
        std::list<int32_t> memberIDs = {});

    /**
     * Send clan information to specific members or all members to update
     * channel side
     * @param clanID Clan instance ID
     * @param updateFlags Flags specifying what information to send. Available
     *  options are:
     *  0x01: Clan name
     *  0x02: Clan emblem
     *  0x04: Clan level
     *  0x08: Indicates that the clan instance ID has updated (ex: joined or left)
     *  Defaults to name, emblem, level
     * @param cids Specific CIDs to send to, sends to all if empty
     */
    void SendClanInfo(int32_t clanID, uint8_t updateFlags = 0x07,
        const std::list<int32_t>& cids = {});

    /**
     * Send clan member updates about the specified player character
     * @param cLogin Pointer to the CharacterLogin to describe
     * @param updateFlags Flags specifying what information to send. Available
     *  options are:
     *  0x01: Member specified status
     *  0x02: Member zone
     *  0x04: Member channel
     *  0x08: Member message
     *  0x10: Member login points
     *  0x20: Member level
     *  Defaults to login points and level as the rest are sent via
     *  CharacterManager::SendStatusToRelatedCharacters
     */
    void SendClanMemberInfo(std::shared_ptr<objects::CharacterLogin> cLogin,
        uint8_t updateFlags = 0x30);

    /**
     * Recalculate a clan's level based on a summation of each member's
     * login points. Every 10,000 points grants a new level starting at 20,000
     * and ending at 100,000.
     * @param clanID Clan instance ID
     * @param sendUpdate If true and the level is changed by the calculation, it
     *  is broadcast to each logged in clan member.
     */
    void RecalculateClanLevel(int32_t clanID, bool sendUpdate = true);

    /**
     * Get an active team by ID
     * @param teamID ID of the team to retrieve
     * @return Pointer to the active team
     */
    std::shared_ptr<objects::Team> GetTeam(int32_t teamID);

    /**
     * Get the maximum team size for a specific team category
     * @param category Team category that groups together types
     * @return Maximum number of team members the team can have
     */
    size_t GetTeamMaxSize(int8_t category) const;

    /**
     * Add a character to the specified team
     * @param worldCID World CID of the character to add
     * @param teamID ID of the team to add the character to
     * @return ID of the team the character was added to
     */
    int32_t AddToTeam(int32_t worldCID, int32_t teamID);

    /**
     * Attempt to have a character join the specified team
     * @param worldCID World CID of the character to add
     * @param teamID ID of the team to add the character to
     * @param sourceConnection Connection associated to the character
     *  joining the team
     * @return true on success, false on failure
     */
    bool TeamJoin(int32_t worldCID, int32_t teamID,
        std::shared_ptr<libcomp::TcpConnection> sourceConnection);

    /**
     * Attempt to have a character leave their current team. All
     * necessary packet communication is handled within.
     * @param cLogin Pointer to the CharacterLogin representation of
     *  the character leaving
     */
    void TeamLeave(std::shared_ptr<objects::CharacterLogin> cLogin);

    /**
     * Attempt to disband the specified team. All necessary packet
     * communication is handled within.
     * @param teamID ID of the team to disband
     * @param sourceCID Optional ID of character who is disbanding the team
     * @param toDiaspora If true the team is being converted into a Diaspora
     *  team and should be notified as part of this
     */
    void TeamDisband(int32_t teamID, int32_t sourceCID = 0,
        bool toDiaspora = false);

    /**
     * Attempt to set the leader of the specified team. All necessary packet
     * communication is handled within.
     * @param teamID ID of the team to update
     * @param sourceCID Optional ID of character who is setting the leader
     * @param requestConnection Optional parameter for the connection
     *  that sent the request
     * @param targetCID CID of the new leader to set
     */
    void TeamLeaderUpdate(int32_t teamID, int32_t sourceCID,
        std::shared_ptr<libcomp::TcpConnection> requestConnection,
        int32_t targetCID);

    /**
     * Attempt to kick a player from a team. All necessary packet
     * communication is handled within.
     * @param cLogin CharacterLogin of the character who sent the request
     * @param targetCID CID of the character being kicked
     * @param teamID ID of the team to update
     * @param requestConnection Optional parameter for the connection
     *  that sent the request
     */
    void TeamKick(std::shared_ptr<objects::CharacterLogin> cLogin,
        int32_t targetCID, int32_t teamID, std::shared_ptr<
        libcomp::TcpConnection> requestConnection);

    /**
     * Update the small and large ziotite values and refresh the related
     * channels. If no values are supplied, the teams will just be refreshed.
     * @param teamID ID of the team to update
     * @param source Pointer to the player performing the update
     * @param sZiotite Amount of small ziotite to add
     * @param lZiotite Amount of large ziotite to add
     * @return true if the update was valid, false if it was not
     */
    bool TeamZiotiteUpdate(int32_t teamID,
        const std::shared_ptr<objects::CharacterLogin>& source = nullptr,
        int32_t sZiotite = 0, int8_t lZiotite = 0);

    /**
     * Send base level info about the specified team ID to every member to
     * act as a refresh for any team info.
     * @param teamID ID of the team to update
     * @param cids Additional CIDs to send to, useful when removing members
     */
    void SendTeamInfo(int32_t teamID, const std::list<int32_t>& cids = {});

    /**
     * Send party member information to the current players in the party
     * @param member Pointer to the new party member
     * @param partyID ID of the party
     * @param newParty true if the party was just created, false if it already
     *  existed
     * @param sourceConnection Optional parameter for the connection
     *  that sent the request
     */
    void SendPartyMember(std::shared_ptr<objects::PartyCharacter> member,
        uint32_t partyID, bool newParty, bool isRefresh,
        std::shared_ptr<libcomp::TcpConnection> sourceConnection);

private:
    /**
     * Create a new party and set the supplied member as the leader
     * @param member Party member to designate as the leader of a new party
     * @return The ID of a the new party, 0 upon failure
     */
    uint32_t CreateParty(std::shared_ptr<objects::PartyCharacter> member);

    /**
     * Remove the supplied CharacterLogin from their current party
     * @param cLogin CharacterLogin to remove
     * @param partyID ID of the party to remove the member from
     * @return true on success, false if they were not in a party
     */
    bool RemoveFromParty(std::shared_ptr<objects::CharacterLogin> cLogin,
        uint32_t partyID);

    /**
     * Remove the supplied CharacterLogin from the specified clan
     * @param cLogin CharacterLogin to remove
     * @param clanID Clan instance ID
     * @return true on success, false if they were not in specified clan
     */
    bool RemoveFromClan(std::shared_ptr<objects::CharacterLogin> cLogin,
        int32_t clanID);

    /**
     * Remove the supplied CharacterLogin from their current team
     * @param cLogin CharacterLogin to remove
     * @param teamID ID of the team to remove the member from
     * @return true on success, false if they were not in the team
     */
    bool RemoveFromTeam(std::shared_ptr<objects::CharacterLogin> cLogin,
        int32_t teamID);

    /// Pointer back to theworld server this belongs to
    std::weak_ptr<WorldServer> mServer;

    /// Map of character login information by UUID
    std::unordered_map<libcomp::String,
        std::shared_ptr<objects::CharacterLogin>> mCharacterMap;

    /// Map of character login information by world CID
    std::unordered_map<int32_t,
        std::shared_ptr<objects::CharacterLogin>> mCharacterCIDMap;

    /// Map of party IDs to parties registered with the server.
    /// The party ID 0 is used for characters awaiting a join request
    /// response
    std::unordered_map<uint32_t, std::shared_ptr<objects::Party>> mParties;

    /// Map of party characters by world CID
    std::unordered_map<int32_t,
        std::shared_ptr<objects::PartyCharacter>> mPartyCharacters;

    /// Map of clan IDs to clans loaded on the server.
    std::unordered_map<int32_t, std::shared_ptr<objects::ClanInfo>> mClans;

    /// Map of clan UUIDs to clan ID assigned when loaded on the server.
    std::unordered_map<libcomp::String, int32_t> mClanMap;

    std::unordered_map<int32_t, std::shared_ptr<objects::Team>> mTeams;

    /// Highest CID registered for a logged in character
    int32_t mMaxCID;

    /// Highest party ID registered with the server
    uint32_t mMaxPartyID;

    /// Highest clan ID registered with the server
    int32_t mMaxClanID;

    /// Highest team ID registered with the server
    int32_t mMaxTeamID;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace world

#endif // SERVER_WORLD_SRC_CHARACTERMANAGER_H
