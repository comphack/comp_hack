/**
 * @file server/lobby/src/AccountManager.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to track accounts that are logged in.
 *
 * This file is part of the Lobby Server (lobby).
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

#ifndef SERVER_LOBBY_SRC_ACCOUNTMANAGER_H
#define SERVER_LOBBY_SRC_ACCOUNTMANAGER_H

// libcomp Includes
#include <CString.h>
#include <ErrorCodes.h>

// Standard C++11 Includes
#include <mutex>
#include <unordered_map>

// object Includes
#include <AccountLogin.h>

namespace objects
{

class Character;
class WebGameSession;

} // namespace objects

namespace lobby
{

class LobbyServer;
class WebGameApiSession;

/**
 * Manages logged in user accounts.
 */
class AccountManager
{
public:
    /**
     * Construct the account manager.
     * @param Pointer to the lobby server this account manager is part of.
     * @note The server must remain valid during any account manager request.
     */
    explicit AccountManager(LobbyServer *pServer);

    /**
     * Transitions the user login state from OFFLINE to LOBBY_WAIT. This
     * operation provides a session ID for the user to pass to a lobby server
     * connection. If the user does not login within a specified period of
     * time the session ID is invalidated and the user transitions back to
     * the OFFLINE state.
     * @param username Username of the account to authenticate.
     * @param password Password of the account to authenticate.
     * @param clientVersion Client version converted into an integer.
     * @param sid Reference to a string to save the session ID into.
     * @param checkPassword Do not change this (used by API version).
     * @returns Error code indicating the success or failure of this
     * operation. This function can return one of:
     * - SUCCESS (login was valid and a session ID was generated)
     * - SYSTEM_ERROR (some internal error occurred)
     * - BAD_USERNAME_PASSWORD
     * - ACCOUNT_STILL_LOGGED_IN (account not in OFFLINE or LOBBY_WAIT)
     * - SERVER_FULL (too many accounts are online)
     * - WRONG_CLIENT_VERSION
     * - ACCOUNT_DISABLED (your account has been disabled/banned)
     * @note This function is thread safe.
     */
    ErrorCodes_t WebAuthLogin(const libcomp::String& username,
        const libcomp::String& password, uint32_t clientVersion,
        libcomp::String& sid, bool checkPassword = true);

    /**
     * Transitions the user login state from OFFLINE to LOBBY_WAIT. This
     * operation provides a session ID for the user to pass to a lobby server
     * connection. If the user does not login within a specified period of
     * time the session ID is invalidated and the user transitions back to
     * the OFFLINE state.
     * @param username Username of the account to authenticate.
     * @param clientVersion Client version converted into an integer.
     * @param sid Reference to a string to save the session ID into.
     * @returns Error code indicating the success or failure of this
     * operation. This function can return one of:
     * - SUCCESS (login was valid and a session ID was generated)
     * - SYSTEM_ERROR (some internal error occurred)
     * - BAD_USERNAME_PASSWORD
     * - ACCOUNT_STILL_LOGGED_IN (account not in OFFLINE or LOBBY_WAIT)
     * - SERVER_FULL (too many accounts are online)
     * - WRONG_CLIENT_VERSION
     * - ACCOUNT_DISABLED (your account has been disabled/banned)
     * @note This function is thread safe.
     * @note The password is not required because it is assumed to have been
     *   checked by the API subsystem. Do NOT use this for normal logins.
     */
    ErrorCodes_t WebAuthLoginApi(const libcomp::String& username,
        uint32_t clientVersion, libcomp::String& sid);

    /**
     * Transitions the user login state from LOBBY_WAIT to LOBBY. This
     * operation provides a session ID for the user to pass to a lobby server
     * connection.
     * @param username Username of the account to authenticate.
     * @param sid Session ID given by the web authentication.
     * @param sid2 Session ID to give back to the client.
     * @returns Error code indicating the success or failure of this
     * operation. This function can return one of:
     * - SUCCESS (login was valid and a session ID was generated)
     * - SYSTEM_ERROR (some internal error occurred)
     * - BAD_USERNAME_PASSWORD
     * - ACCOUNT_STILL_LOGGED_IN (account not in OFFLINE or LOBBY_WAIT)
     * @note This function is thread safe.
     */
    ErrorCodes_t LobbyLogin(const libcomp::String& username,
        const libcomp::String& sid, libcomp::String& sid2);

    /**
     * Transitions the user login state from OFFLINE to LOBBY. It is assumed
     * the client version and the password hash has already been checked by
     * the classic login packet handlers.
     * @param username Username of the account to authenticate.
     * @param sid2 Session ID to give back to the client.
     * @returns Error code indicating the success or failure of this
     * operation. This function can return one of:
     * - SUCCESS (login was valid and a session ID was generated)
     * - SYSTEM_ERROR (some internal error occurred)
     * - BAD_USERNAME_PASSWORD
     * - ACCOUNT_STILL_LOGGED_IN (account not in OFFLINE or LOBBY_WAIT)
     * - SERVER_FULL (too many accounts are online)
     * - ACCOUNT_DISABLED (your account has been disabled/banned)
     * @note This function is thread safe.
     */
    ErrorCodes_t LobbyLogin(const libcomp::String& username,
        libcomp::String& sid2);

    /**
     * Set the character associated to the supplied account in preparation
     * for world communication and channel connection.
     * @param username Username of the account to set the character on.
     * @param character Pointer to the character to set on the account.
     * @returns Pointer to the matching AccountLogin or null if it does
     *  not exist
     * @note This function is thread safe.
     */
    std::shared_ptr<objects::AccountLogin> StartChannelLogin(
        const libcomp::String& username,
        const std::shared_ptr<objects::Character>& character);

    /**
     * Transitions the user login state from LOBBY to LOBBY_TO_CHANNEL,
     * set the supplied world and channel IDs and await world communication
     * that the player has successfully entered the channel. A timeout should
     * be used in conjunction with this to ensure the account does not
     * get stuck.
     * @param username Username of the account to update.
     * @param worldID ID of the world the account is being moved to.
     * @param channelID ID of the channel the account is being moved to.
     * @returns Error code indicating the success or failure of this
     * operation. This function can return one of:
     * - SUCCESS (change was valid and login information was updated)
     * - SYSTEM_ERROR (some internal error occurred)
     * @note This function is thread safe.
     */
    ErrorCodes_t SwitchToChannel(const libcomp::String& username,
        int8_t worldID, int8_t channelID);

    /**
     * Transitions the user login state from LOBBY_TO_CHANNEL to CHANNEL,
     * completing a sucessful channel login.
     * @param username Username of the account to update.
     * @param worldID ID of the world the account was connected to.
     * @param channelID ID of the channel the account was connected to.
     * @returns Error code indicating the success or failure of this
     * operation. This function can return one of:
     * - SUCCESS (change was valid and login information was updated)
     * - SYSTEM_ERROR (some internal error occurred)
     * @note This function is thread safe.
     */
    ErrorCodes_t CompleteChannelLogin(const libcomp::String& username,
        int8_t worldID, int8_t channelID);

    /**
     * Transitions the user login state from CHANNEL to CHANNEL_TO_CHANNEL,
     * and update the session key.
     * @param username Username of the account to update.
     * @param channelID ID of the channel the account was switched to.
     * @param sesssionKey New session key set by the world
     * @returns true upon successful update, false if an error occurred
     * @note This function is thread safe.
     */
    bool ChannelToChannelSwitch(const libcomp::String& username,
        int8_t channelID, uint32_t sessionKey);

    /**
     * Transitions the user to the OFFLINE state.
     * @param username Username of the account to log out.
     * @returns true if the account was logged out; false otherwise.
     * @note This function is thread safe.
     */
    bool Logout(const libcomp::String& username);

    /**
     * Expire a session key. If the session key is not matched or the account
     * is not awaiting login anymore (LOBBY_WAIT state) this is ignored.
     * @param username Username for account the session key was create for.
     * @param sid Session ID that has expired.
     * @note This function is thread safe.
     */
    void ExpireSession(const libcomp::String& username,
        const libcomp::String& sid);

    /**
     * Check if a user is logged in.
     * @param username Username for the account to check.
     * @param world World the user is logged into or -1 if they are in the
     * lobby server.
     * @return true if the user is logged in; false otherwise.
     */
    bool IsLoggedIn(const libcomp::String& username,
        int8_t& world);

    /**
     * Get the current user login state independent of world.
     * @param username Username for the account to login.
     * @return Pointer to the login state; null if it does not exist.
     */
    std::shared_ptr<objects::AccountLogin> GetUserLogin(
        const libcomp::String& username);

    /**
     * Get all users in a given world (and optionally on a specific channel).
     * @param world World to get all users from.
     * @param channel Channel in the world to get all users from. If
     * this is empty, all users in the world will be retrieved.
     * @return List of usernames in the world (and channel).
     */
    std::list<libcomp::String> GetUsersInWorld(int8_t world,
        int8_t channel = -1);

    /**
     * Log out all users in a given world (and optionally on a specific
     * channel). This should only be called when a world or channel disconnects.
     * @param world World to log out all users from.
     * @param channel Channel in the world to log out all users from. If
     * this is empty, all users in the world will be logged out.
     * @return List of usernames that were logged out.
     */
    std::list<libcomp::String> LogoutUsersInWorld(int8_t world,
        int8_t channel = -1);

    /**
     * Mark or clear a character by CID for deletion.  This assumes the character
     * has already been loaded and will not load them if they are not.
     * @param username Username of the account that the character belongs to.
     * @param cid CID of the character to update.
     * @param server Pointer to the lobby server.
     * @return true if the character was updated, false otherwise.
     */
    bool UpdateKillTime(const libcomp::String& username,
        uint8_t cid, std::shared_ptr<LobbyServer>& server);

    /**
     * Delete all characters with an exceeded kill time on a connected world.
     * @param worldID World ID to delete characters on
     * @return true if the process completed successfully, false if an error
     *  occurred
     */
    bool DeleteKillTimeExceededCharacters(uint8_t worldID);

    /**
     * Set a character on the specified account's character array and save it
     * @param account Pointer to the account to create the character on
     * @param character Pointer to the character to create
     * @return true if the character was created and the account was updated,
     *  false otherwise
     */
    bool SetCharacterOnAccount(const std::shared_ptr<objects::Account>& account,
        const std::shared_ptr<objects::Character>& character);

    /**
     * Get characters on an account with a KillTime that has passed.
     * This assumes characters have already been loaded and will not
     * load them if they are not.
     * @param account Pointer to the account to gather deletes for.
     * @return List of pointers to characters to delete.
     */
    std::list<std::shared_ptr<objects::Character>> GetCharactersForDeletion(
        const std::shared_ptr<objects::Account>& account);

    /**
     * Delete a character by CID and update the Characters array on
     * the account.  This assumes the character has already been loaded and
     * will not load them if they are not.
     * @param account Pointer to the account to delete the character from.
     * @param character Pointer to the character to delete.
     * @return true if the character was deleted and the account was updated,
     *  false otherwise.
     */
    bool DeleteCharacter(const std::shared_ptr<objects::Account>& account,
        const std::shared_ptr<objects::Character>& character);

    /**
     * Start a web-game session for the specified user who is currently playing
     * that remains valid until a remove request is received or the account
     * leaves the channel.
     * @param username Username of the account the session belongs to
     * @param gameSession Pointer to the session's definition
     * @return true if the session was started successfully, false if it
     *  was not
     */
    bool StartWebGameSession(const libcomp::String& username,
        const std::shared_ptr<objects::WebGameSession>& gameSession);

    /**
     * Get or create a web-game API session based upon a valid web-game
     * session if one exists and matches the supplied values. Sessions created
     * here last only as long as the game session exists.
     * @param username Username of the account the session belongs to
     * @param sessionID Session ID supplied by the world server which changes
     *  each time a new web-game session is requested
     * @param clientAddress Base address of the client requesting the session
     *  used for prevention of two active sessions for the same user
     * @return Pointer to the web-game API session, null if the supplied
     *  values were not valid
     */
    std::shared_ptr<WebGameApiSession> GetWebGameApiSession(
        const libcomp::String& username, const libcomp::String& sessionID,
        const libcomp::String& clientAddress);

    /**
     * Start any web-game session for the specified user
     * @param username Username of the account the session belongs to
     * @return true if the session existed and was removed, false if it did not
     */
    bool EndWebGameSession(const libcomp::String& username);

protected:
    /**
     * Return the existing login object for the given username or create a
     * new login object if one does not already exist.
     * @param username Username for the login object to return.
     * @returns The login object for the given username or null on error.
     * @note This function is NOT thread safe. You MUST lock access first!
     */
    std::shared_ptr<objects::AccountLogin> GetOrCreateLogin(
        const libcomp::String& username);

    /**
     * This will remove a login entry for the account map. Accounts not in
     * the map are considered OFFLINE.
     * @param username Username of the account to remove from the login map.
     * @note This function is NOT thread safe. You MUST lock access first!
     */
    void EraseLogin(const libcomp::String& username);

    /**
     * Print the status of the accounts managed by this object.
     * @note This function is NOT thread safe. You MUST lock access first!
     */
    void PrintAccounts() const;

    /**
     * Update the server debug state for systems configured to display it.
     * @note This function is NOT thread safe. You MUST lock access first!
     */
    void UpdateDebugStatus() const;

private:
    /// Pointer to the lobby server.
    LobbyServer *mServer;

    /// Mutex to lock access to the account map.
    std::mutex mAccountLock;

    /// Map of accounts with associated login information.
    std::unordered_map<libcomp::String,
        std::shared_ptr<objects::AccountLogin>> mAccountMap;

    /// Map of account usernames associated to accounts to web-game sessions
    /// either pending or active for a character currently playing
    std::unordered_map<libcomp::String, std::shared_ptr<
        objects::WebGameSession>> mWebGameSessions;

    // List of web-game API sessions by username. Only valid for as long
    // as there is a web-game session active
    std::unordered_map<libcomp::String,
        std::shared_ptr<WebGameApiSession>> mWebGameAPISessions;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_ACCOUNTMANAGER_H
