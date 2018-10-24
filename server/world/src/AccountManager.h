/**
 * @file server/world/src/AccountManager.h
 * @ingroup world
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to track accounts that are logged in.
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

#ifndef SERVER_WORLD_SRC_ACCOUNTMANAGER_H
#define SERVER_WORLD_SRC_ACCOUNTMANAGER_H

// libcomp Includes
#include <CString.h>
#include <InternalConnection.h>

// Standard C++11 Includes
#include <unordered_map>

// object Includes
#include <AccountLogin.h>

namespace objects
{
class AccountWorldData;
class ChannelLogin;
class Character;
class WebGameSession;
}

namespace world
{

class WorldServer;

/**
 * Manages logged in user accounts.
 */
class AccountManager
{
public:
    /**
     * Create a new account manager
     * @param server Pointer back to the world server this
     *  belongs to
     */
    AccountManager(const std::weak_ptr<WorldServer>& server);

    /**
     * Check if a user is logged in.
     * @param username Username for the account to check.
     * @param channel Channel the user is logged into or -1 if they are in the
     * lobby server.
     * @return true if the user is logged in; false otherwise.
     */
    bool IsLoggedIn(const libcomp::String& username,
        int8_t& channel);

    /**
     * Register the supplied login with the world if it has not been already.
     * @param login Login information associated to the account.
     * @return true if the user was logged in; false if the user is already
     *  logged in.
     */
    bool LobbyLogin(std::shared_ptr<objects::AccountLogin> login);

    /**
     * Update the supplied login, set the login state to CHANNEL and the
     * character status to ONLINE. Also perform any "on login" actions.
     * @param login Login information associated to the account.
     * @return true if the account was successfully updated; false if an
     *  error occurred.
     */
    bool ChannelLogin(std::shared_ptr<objects::AccountLogin> login);

    /**
     * Transition the login from CHANNEL to CHANNEL_TO_CHANNEL and schedule
     * a timeout.
     * @param login Login information associated to the account.
     * @param switchDef Definition of the connection that will be moved to
     * @return true if the update succeeded, false if it did not
     */
    bool SwitchChannel(std::shared_ptr<objects::AccountLogin> login,
        const std::shared_ptr<objects::ChannelLogin>& switchDef);

    /**
     * Get the current user login state independent of world.
     * @param username Username for the account to login.
     * @return Pointer to the login state; null if it does not exist.
     */
    std::shared_ptr<objects::AccountLogin> GetUserLogin(
        const libcomp::String& username);

    /**
     * Mark the user logged out of the given channel.
     * @param username Username for the account to log out.
     * @param channel Channel the user is logged into or -1 if they are in the
     * lobby server.
     * @return Pointer to the AccountLogin representing the logged out user,
     *  nullptr if they were not registered.
     */
    std::shared_ptr<objects::AccountLogin> LogoutUser(
        const libcomp::String& username, int8_t channel = -1);

    /**
     * Expire the user session and log out the account. If the session key
     * is not matched or the account has completed its channel login
     * (CHANNEL state) this is ignored.
     * @param username Username for account to check.
     * @param key Session key that has expired.
     * @return true if the session was expired, false if it was not
     */
    bool ExpireSession(const libcomp::String& username,
        uint32_t key);

    /**
     * Log out all users on a given channel. This should only be called
     * when a channel disconnects.
     * @param channel Channel to log out all users from.
     * @return List of AccountLogins representing the logged out users.
     */
    std::list<std::shared_ptr<objects::AccountLogin>>
        LogoutUsersOnChannel(int8_t channel);

    /**
     * Handle a request to connect to a channel from the lobby
     * @param login Pointer to the AccountLogin request
     */
    void HandleLobbyLogin(const std::shared_ptr<objects::AccountLogin>& login);

    /**
     * Complete the request to connect to a channel from the lobby. If the
     * server is configured for multiple channels, the request may have been
     * relayed from the primary channel intead with the proper target channel
     * and zone supplied.
     * @param login Pointer to the AccountLogin request
     * @param channelLogin Optional parameter supplied by the primary channel
     *  when multiple channels are configured containing target login info
     */
    void CompleteLobbyLogin(
        const std::shared_ptr<objects::AccountLogin>& login,
        const std::shared_ptr<objects::ChannelLogin>& channelLogin = nullptr);

    /**
     * Handle a request to complete a channel login
     * @param requestConnection Pointer to the channel connection where the
     *  request came from
     * @param sessionKey Session key supplied to the channel from the client
     *  used for login validation
     * @param username Username of the account logging in
     */
    void HandleChannelLogin(
        const std::shared_ptr<libcomp::InternalConnection>& requestConnection,
        uint32_t sessionKey, const libcomp::String& username);

    /**
     * Check if any existing channel switch signifier exists for the specified
     * account
     * @param username Username of the account set to check
    * @param channel Output parameter to store the channel being switched
    *  to. If no channel switch is stored, this will not be set.
     * @return Pointer to the channel switch definition
     */
    bool ChannelSwitchPending(const libcomp::String& username,
        int8_t& channel);

    /**
     * "Pop" any existing channel switch signifier from the manager for
     * the specified account and return it
     * @param username Username of the account set to switch channels
     * @return Pointer to the channel switch definition
     */
    std::shared_ptr<objects::ChannelLogin> PopChannelSwitch(
        const libcomp::String& username);

    /**
     * Perform all clean up operations related to each AccountWorldData entry
     * with CleanupRequired currently flagged.
     */
    void CleanupAccountWorldData();

    /**
     * Delete a character requested from the lobby or otherwise marked for
     * deletion by cleaning up all related character objects. If the character
     * is somehow still logged in, a disconnect request will be sent and the
     * character will be marked for deletion for the next time this process
     * runs.
     * @param character Pointer to the character to delete
     * @return true if the delete was successful, false if the delete failed
     */
    bool DeleteCharacter(const std::shared_ptr<objects::Character>& character);

    /**
     * Start a web-game session for the specified user who is currently playing
     * that remains valid until a remove request is received or the account
     * leaves the channel. Once it is created, the lobby will be notified. The
     * channel will be notified after the lobby returns a success message.
     * @param gameSession Pointer to the session's definition
     * @return true if the session was started successfully, false if it
     *  was not
     */
    bool StartWebGameSession(const std::shared_ptr<
        objects::WebGameSession>& gameSession);

    /**
     * Get the current web-game session for the specified user
     * @param username Username of the account the session belongs to
     * @return Pointer to the game session, null if none exists
     */
    std::shared_ptr<objects::WebGameSession> GetGameSession(
        const libcomp::String& username);

    /**
     * Start any web-game session for the specified user and notify the other
     * servers as needed. Even if no currently active session exists, the other
     * servers will be sent a remove request.
     * @param username Username of the account the session belongs to
     * @param notifyLobby true if the lobby should be notified
     * @param notifyChannel true if the channel should be notified
     * @return true if the session existed and was removed, false if it did not
     */
    bool EndWebGameSession(const libcomp::String& username,
        bool notifyLobby, bool notifyChannel);

private:
    /**
     * Update the session key of the supplied login.  The lobby must be
     * notified of this update or the login information will become out
     * of sync.
     * @param login Pointer to the account login to update
     */
    void UpdateSessionKey(std::shared_ptr<objects::AccountLogin> login);

    /**
     * Utility function to free up references to an AccountLogin loaded
     * by the world.
     * @param login Pointer to the AcccountLogin to clean up
     */
    void Cleanup(const std::shared_ptr<objects::AccountLogin>& login);

    /**
     * "Push" a channel switch signifier to the manager for the specified
     * account. The next logout request will pop this value and await a
     * reconnect to the specified channel rather than log the account out.
     * @param username Username of the account that will switch channels
     * @param switchDef Definition of the connection that will be moved to
     */
    void PushChannelSwitch(const libcomp::String& username,
        const std::shared_ptr<objects::ChannelLogin>& switchDef);

    /**
     * Utility function to free up references to a PersistentObject loaded
     * by the world.
     * @param login Pointer to the PersistentObject to clean up
     */
    template <class T>
    void Cleanup(const std::shared_ptr<T>& obj)
    {
        if(obj != nullptr)
        {
            libcomp::ObjectReference<T>::Unload(obj->GetUUID());
        }
    }

    /// Pointer back to theworld server this belongs to
    std::weak_ptr<WorldServer> mServer;

    /// Map of account login information by username
    std::unordered_map<libcomp::String,
        std::shared_ptr<objects::AccountLogin>> mAccountMap;

    /// Map of account usernames associated to accounts set to switch
    /// channel upon next disconnect from a channel
    std::unordered_map<libcomp::String,
        std::shared_ptr<objects::ChannelLogin>> mChannelSwitches;

    /// Map of account usernames associated to accounts to web-game sessions
    /// either pending or active for a character currently playing
    std::unordered_map<libcomp::String, std::shared_ptr<
        objects::WebGameSession>> mWebGameSessions;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace world

#endif // SERVER_WORLD_SRC_ACCOUNTMANAGER_H
