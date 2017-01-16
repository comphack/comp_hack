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

#ifndef SERVER_WORLD_SRC_ACCOUNTMANAGER_H
#define SERVER_WORLD_SRC_ACCOUNTMANAGER_H

// libcomp Includes
#include <CString.h>

// Standard C++11 Includes
#include <unordered_map>

// object Includes
#include <AccountLogin.h>

namespace world
{

/**
 * Manages logged in user accounts.
 * @note This is not thread safe.
 */
class AccountManager
{
public:
    /**
     * Create a new account manager
     */
    AccountManager();

    /**
     * Check if a user is logged in.
     * @param username Username for the account to check.
     * @return true if the user is logged in; false otherwise.
     */
    bool IsLoggedIn(const libcomp::String& username) const;

    /**
     * Check if a user is logged in.
     * @param username Username for the account to check.
     * @param channel Channel the user is logged into or -1 if they are in the
     * lobby server.
     * @return true if the user is logged in; false otherwise.
     */
    bool IsLoggedIn(const libcomp::String& username,
        int8_t& channel) const;

    /**
     * Mark the user logged into the given channel.
     * @param username Username for the account to login.
     * @param login Login information associated to the account.
     * @return true if the user was logged in; false if the user is already
     * logged in to another channel.
     */
    bool LoginUser(const libcomp::String& username,
        std::shared_ptr<objects::AccountLogin> login);

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
     * @return true if the user was logged out; false if the user is not
     * logged in to the specified channel.
     */
    bool LogoutUser(const libcomp::String& username, int8_t channel = -1);

    /**
     * Log out all users on a given channel. This should only be called
     * when a channel disconnects.
     * @param channel Channel to log out all users from.
     * @return List of usernames that were logged out.
     */
    std::list<libcomp::String> LogoutUsersOnChannel(int8_t channel);

private:
    /// Map of accounts with associated channel.
    std::unordered_map<libcomp::String,
        std::shared_ptr<objects::AccountLogin>> mAccountMap;

    /// Highest session key divvied out. This can break if you log in
    /// 2,147,483,649 times without restarting the server :P
    uint32_t mMaxSessionKey;
};

} // namespace world

#endif // SERVER_WORLD_SRC_ACCOUNTMANAGER_H
