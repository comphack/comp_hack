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
     * @param channel Channel the user is logged into or -1 if they are in the
     * lobby server.
     * @return true if the user is logged in; false otherwise.
     */
    bool IsLoggedIn(const libcomp::String& username,
        int8_t& channel);

    /**
     * Mark the user logged into the given channel.
     * @param login Login information associated to the account.
     * @return true if the user was logged in; false if the user is already
     * logged in to another channel.
     */
    bool LoginUser(std::shared_ptr<objects::AccountLogin> login);

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
     * Log out all users on a given channel. This should only be called
     * when a channel disconnects.
     * @param channel Channel to log out all users from.
     * @return List of AccountLogins representing the logged out users.
     */
    std::list<std::shared_ptr<objects::AccountLogin>>
        LogoutUsersOnChannel(int8_t channel);

private:
    /**
     * Utility function to free up references to an AccountLogin loaded
     * by the world.
     * @param login Pointer to the AcccountLogin to clean up
     */
    void Cleanup(const std::shared_ptr<objects::AccountLogin>& login);

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
            obj->Unregister();
        }
    }

    /// Map of account login information by username
    std::unordered_map<libcomp::String,
        std::shared_ptr<objects::AccountLogin>> mAccountMap;

    /// Highest session key divvied out. This can break if you log in
    /// 2,147,483,649 times without restarting the server :P
    uint32_t mMaxSessionKey;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace world

#endif // SERVER_WORLD_SRC_ACCOUNTMANAGER_H
