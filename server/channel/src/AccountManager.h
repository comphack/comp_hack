/**
 * @file server/channel/src/AccountManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages accounts on the channel.
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

#ifndef SERVER_CHANNEL_SRC_ACCOUNTMANAGER_H
#define SERVER_CHANNEL_SRC_ACCOUNTMANAGER_H

// channel Includes
#include "ChannelClientConnection.h"

namespace libcomp
{
class Database;
}

namespace objects
{
class Account;
class ChannelLogin;
class CharacterLogin;
}

namespace channel
{

class ChannelServer;

/**
 * Codes sent from the client to request a logout related action.
 */
enum LogoutCode_t : uint8_t
{
    LOGOUT_CODE_UNKNOWN_MIN = 5,
    LOGOUT_CODE_QUIT = 6,
    LOGOUT_CODE_CANCEL = 7,
    LOGOUT_CODE_SWITCH = 8,
    LOGOUT_CODE_UNKNOWN_MAX = 9,
};

/**
 * Manager to handle Account focused actions.
 */
class AccountManager
{
public:
    /**
     * Create a new AccountManager.
     * @param server Pointer back to the channel server this
     *  belongs to
     */
    AccountManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the AccountManager.
     */
    ~AccountManager();

    /**
     * Request information from the world to log an account
     * in by their username.
     * @param client Pointer to the client connection
     * @param username Username to log in with
     * @param sessionKey Session key to validate
     */
    void HandleLoginRequest(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const libcomp::String& username, uint32_t sessionKey);

    /**
     * Respond to the game client with the result of the login
     * request.
     * @param client Pointer to the client connection
     */
    void HandleLoginResponse(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Handle the client's logout request.
     * @param client Pointer to the client connection
     * @param code Action being requested
     * @param channelIdx Index of the channel to connect
     * to after logging out. This will only be used if
     * the the logout code is LOGOUT_CODE_SWITCH.
     */
    void HandleLogoutRequest(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        LogoutCode_t code, uint8_t channelIdx = 0);

    /**
     * Log out a user by their connection.
     * @param client Pointer to the client connection
     * @param delay Optional parameter to perform the normal
     *  logout save actions but leave the connection open
     *  to be removed upon the connection actually closing
     */
    void Logout(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        bool delay = false);

    /**
     * Request that a client disconnect from the server. Typically used
     * following an initial logout request from the client.
     * @param client Pointer to the client connection
     */
    void RequestDisconnect(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Create a channel change ChannelLogin for the supplied client and save
     * all logout information now. The world communication must be handled
     * elsewhere.
     * @param client Pointer to the client connection
     * @param zoneID ID of the zone the player will enter on the other channel.
     *  If 0 the current character state will be used.
     * @param dynamicMapID Dynamic map ID of the zone the player will enter on
     *  the other channel. If 0 the current character state will be used.
     * @param channelID ID of the channel being moved to
     * @return Pointer to the channel change ChannelLogin
     */
    std::shared_ptr<objects::ChannelLogin> PrepareChannelChange(
        const std::shared_ptr<channel::ChannelClientConnection>& client,
        uint32_t zoneID, uint32_t dynamicMapID, uint8_t channelID);

    /**
     * Authenticate an account by its connection.
     * @param client Pointer to the client connection
     */
    void Authenticate(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Increase the account's current CP balance.
     * @param account Pointer to the account to update
     * @param addAmount Amount of CP to add to the account
     * @return true if the amount was updated, false if it
     *  could not be updated
     */
    bool IncreaseCP(const std::shared_ptr<
        objects::Account>& account, int64_t addAmount);

    /**
     * Send the account's current CP balance.
     * @param client Pointer to the client connection
     */
    void SendCPBalance(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Get all active CharacterLogins associated to the world
     * @return Map of active CharacterLogins by world CID
     */
    std::unordered_map<int32_t,
        std::shared_ptr<objects::CharacterLogin>> GetActiveLogins();

    /**
     * Get an active CharacterLogin by world CID
     * @param characterUID Character UUID to retrieve
     * @return Pointer to the active CharacterLogin or null if no matching
     *  UID is active
     */
    std::shared_ptr<objects::CharacterLogin> GetActiveLogin(
        const libobjgen::UUID& characterUID);

    /**
     * Update all registered CharacterLogins on the server
     * @param update List of updated CharacterLogins
     * @param removes List of removed CharacterLogins
     */
    void UpdateLogins(
        std::list<std::shared_ptr<objects::CharacterLogin>> updates,
        std::list<std::shared_ptr<objects::CharacterLogin>> removes);

    /**
     * Dump the account and return it. This account data can then be
     * imported into another server.
     * @param state ClientState object for the account to dump.
     * @returns Dump of the account or an empty string on error.
     */
    libcomp::String DumpAccount(channel::ClientState *state);

private:
    /**
     * Delete a <member> from an object in the XML DOM.
     * @param pElement Object element to delete the <member> from.
     * @param field Name of the member field to delete.
     */
    void WipeMember(tinyxml2::XMLElement *pElement,
        const std::string& field);

    /**
     * Create/load character data for use upon logging in.
     * @param character Character to initialize
     * @param state Pointer to the client state the character
     *  belongs to
     * @return true on success, false on failure
     */
    bool InitializeCharacter(libcomp::ObjectReference<
        objects::Character>& character,
        channel::ClientState* state);

    /**
     * Create character data if not initialized.
     * Supported objects are as follows:
     * - Character (limited fields)
     * - CharacterProgress (only one per file)
     * - Item (including starting equipment)
     * - Demon (limited to COMP slots)
     * - EntityStats (stats/level for character, level only for demon,
     *   must by linked via UID)
     * - Expertise (must be linked via UID)
     * - Hotbar (must be linked via UID)
     * @param character Character to initialize
     * @return true on success, false on failure
     */
    bool InitializeNewCharacter(std::shared_ptr<
        objects::Character> character);

    /**
     * Persist character data associated to a client that is
     * logging out.
     * @param state Pointer to the client state the character
     *  belongs to
     * @return true on success, false on failure
     */
    bool LogoutCharacter(channel::ClientState* state);

    /// Map of all character logins active on the world by world CID
    std::unordered_map<int32_t,
        std::shared_ptr<objects::CharacterLogin>> mActiveLogins;

    /// Map of character UUIDs to world CID for any active login
    std::unordered_map<libcomp::String, int32_t> mCIDMap;

    /// Server lock for shared resources
    std::mutex mLock;

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ACCOUNTMANAGER_H
