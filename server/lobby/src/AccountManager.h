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

#ifndef SERVER_LOBBY_SRC_ACCOUNTMANAGER_H
#define SERVER_LOBBY_SRC_ACCOUNTMANAGER_H

// libcomp Includes
#include <CString.h>

// Standard C++11 Includes
#include <mutex>
#include <unordered_map>

// object Includes
#include <AccountLogin.h>

namespace lobby
{

class LobbyServer;

/**
 * Manages logged in user accounts.
 */
class AccountManager
{
public:
    /**
     * Check if a user is logged in.
     * @param username Username for the account to check.
     * @return true if the user is logged in; false otherwise.
     */
    bool IsLoggedIn(const libcomp::String& username);

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
     * Mark the user logged into the given world.
     * @param username Username for the account to login.
     * @param login Login information associated to the account.
     * @return true if the user was logged in; false if the user is already
     * logged in to another world.
     */
    bool LoginUser(const libcomp::String& username,
        std::shared_ptr<objects::AccountLogin> login = nullptr);

    /*
     * Updates the session ID of the login associated to a username.
     * @param username Username for the logged in account.
     * @param sid Session ID to update.
     * @return true if the user is logged in; false if not
     */
    bool UpdateSessionID(const libcomp::String& username,
        const libcomp::String& sid);

    /**
     * Get the current user login state independent of world.
     * @param username Username for the account to login.
     * @return Pointer to the login state; null if it does not exist.
     */
    std::shared_ptr<objects::AccountLogin> GetUserLogin(
        const libcomp::String& username);

    /**
     * Mark the user logged out of the given world.
     * @param username Username for the account to log out.
     * @param world World the user is logged into or -1 if they are in the
     * lobby server.
     * @return true if the user was logged out; false if the user is not
     * logged in to the specified world.
     */
    bool LogoutUser(const libcomp::String& username, int8_t world = -1);

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
     * Get characters on an account with a KillTime that has passed.
     * This assumes characters have already been loaded and will not
     * load them if they are not.
     * @param username Username of the account that the characters belong to.
     * @return List of CIDs for characters to delete.
     */
    std::list<uint8_t> GetCharactersForDeletion(
        const libcomp::String& username);

    /**
     * Delete a character by CID and update the Characters array on
     * the account.  This assumes the character has already been loaded and
     * will not load them if they are not.
     * @param username Username of the account that the character belongs to.
     * @param cid CID of the character to delete.
     * @param server Pointer to the lobby server.
     * @return true if the character was deleted and the account was updated,
     *  false otherwise.
     */
    bool DeleteCharacter(const libcomp::String& username,
        uint8_t cid, std::shared_ptr<LobbyServer>& server);

private:
    /// Mutex to lock access to the account map.
    std::mutex mAccountLock;

    /// Map of accounts with associated login information.
    std::unordered_map<libcomp::String,
        std::shared_ptr<objects::AccountLogin>> mAccountMap;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_ACCOUNTMANAGER_H
