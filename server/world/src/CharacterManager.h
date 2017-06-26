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

#ifndef SERVER_WORLD_SRC_CHARACTERMANAGER_H
#define SERVER_WORLD_SRC_CHARACTERMANAGER_H

// Standard C++11 Includes
#include <unordered_map>

// object Includes
#include <CharacterLogin.h>
#include <Party.h>
#include <PartyCharacter.h>

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
     * remain until the server restarts.
     * @param cLogin CharacterLogins to register
     * @return Pointer to the CharacterLogin registered with the server. null is
     *  never returned and the value sent back should always replace the value
     *  passed in to keep the servers in sync
     */
    std::shared_ptr<objects::CharacterLogin> RegisterCharacter(
        std::shared_ptr<objects::CharacterLogin> cLogin);

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
     * Send a packet to the specified logins
     * @param p Packet to send
     * @param cLogins CharacterLogins to map channel connections to when sending
     *  the packet
     * @param appendCIDs true if the list of world CIDs from the associated logins
     *  should be appended to the end of the packet, false if it should be sent
     *  as is
     * @return false if an error occurs
     */
    bool SendToCharacters(libcomp::Packet& p,
        const std::list<std::shared_ptr<objects::CharacterLogin>>& cLogins,
        bool appendCIDs);

    /**
     * Send a packet to various characters related to the supplied world CID
     * @param p Packet to send
     * @param worldCID World CID used to find related characters
     * @param appendCIDs true if the list of world CIDs from the characters found
     *  should be appended to the end of the packet, false if it should be sent as is
     * @param friends true if characters from the related friend list should be included
     * @param party true if characters from the same party should be included
     * @param includeSelf Optional parameter to include the character associated
     *  to the world CID. Defaults to false.
     * @param zoneRestrict Optional parameter to restrict the characters being
     *  retrieved to ones in the same zone as the supplied CID. Defaults to false.
     * @return false if an error occurs
     */
    bool SendToRelatedCharacters(libcomp::Packet& p, int32_t worldCID,
        bool appendCIDs, bool friends, bool party, bool includeSelf = false,
        bool zoneRestrict = false);

    /**
     * Retrieves characters related to the supplied CharacterLogin
     * @param cLogin CharacterLogin to find related characters for
     * @param friends true if characters from the related friend list should be included
     * @param party true if characters from the same party should be included
     * @return List of pointers to the related CharacterLogins
     */
    std::list<std::shared_ptr<objects::CharacterLogin>>
        GetRelatedCharacterLogins(std::shared_ptr<objects::CharacterLogin> cLogin,
            bool friends, bool party);

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
     * @param cLogin CharacterLogin to retrieve the party member by
     * @return Pointer to the party member
     */
    std::shared_ptr<objects::PartyCharacter>
        GetPartyMember(std::shared_ptr<objects::CharacterLogin> cLogin);

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
     * necessary packet communication is handled within.
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
     * Attempt to have a party member leave their current party. All
     * necessary packet communication is handled within.
     * @param member Pointer to the CharacterLogin representation of
     *  the party member leaving
     * @param requestConnection Optional parameter for the connection
     *  that sent the request
     * @param tempLeave If true, the character will be able to join again
     *  if the party remains.  Useful for handling log out joining.
     */
    void PartyLeave(std::shared_ptr<objects::CharacterLogin> cLogin,
        std::shared_ptr<libcomp::TcpConnection> requestConnection, bool tempLeave);

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
     * @return true on succes, false if they were not in a party
     */
    bool RemoveFromParty(std::shared_ptr<objects::CharacterLogin> cLogin);

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

    /// Highest CID registered for a logged in character
    int32_t mMaxCID;

    /// Highest party ID registered with the server
    uint32_t mMaxPartyID;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace world

#endif // SERVER_WORLD_SRC_CHARACTERMANAGER_H
