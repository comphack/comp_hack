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

/**
 * Static manager to handle Account focused actions.
 */
class AccountManager
{
public:
    /**
     * Log an account in by their username.
     * @param client Pointer to the client connection
     * @param username Username to log in with
     * @param sessionKey Session key to validate
     */
    static void Login(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const libcomp::String& username, uint32_t sessionKey);

    /**
     * Authenticate an account by its connection.
     * @param client Pointer to the client connection
     */
    static void Authenticate(const std::shared_ptr<
        channel::ChannelClientConnection>& client);
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ACCOUNTMANAGER_H
