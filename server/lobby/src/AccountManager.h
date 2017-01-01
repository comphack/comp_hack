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

namespace lobby
{

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
     * @param world World the user is logged into or -1 if they are in the
     * lobby server.
     * @return true if the user was logged in; false if the user is already
     * logged in to another world.
     */
    bool LoginUser(const libcomp::String& username, int8_t world = -1);

    /**
     * Mark the user logged out of the given world.
     * @param username Username for the account to log out.
     * @param world World the user is logged into or -1 if they are in the
     * lobby server.
     * @return true if the user was logged out; false if the user is not
     * logged in to the specified world.
     */
    bool LogoutUser(const libcomp::String& username, int8_t world = -1);

private:
    /// Mutex to lock access to the account map.
    std::mutex mAccountLock;

    /// Map of accounts with associated world.
    std::unordered_map<libcomp::String, int8_t> mAccountMap;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_ACCOUNTMANAGER_H
