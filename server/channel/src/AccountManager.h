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

#ifndef SERVER_CHANNEL_SRC_ACCOUNTMANAGER_H
#define SERVER_CHANNEL_SRC_ACCOUNTMANAGER_H

// channel Includes
#include "ChannelClientConnection.h"

namespace channel
{

class ChannelServer;

/**
 * Manager to handle Account focused actions.
 */
class AccountManager
{
public:
    /**
     * Create a new AccountManager.
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
     * Log out a user by username.
     * @param username Username to log out
     */
    void Logout(const libcomp::String& username);

    /**
     * Authenticate an account by its connection.
     * @param client Pointer to the client connection
     */
    void Authenticate(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

private:
    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ACCOUNTMANAGER_H
