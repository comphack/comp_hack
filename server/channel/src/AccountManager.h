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

namespace libcomp
{

class Database;

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
     * Authenticate an account by its connection.
     * @param client Pointer to the client connection
     */
    void Authenticate(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

private:
    /**
     * Create character data if not initialized or load
     * if it has been initaliazed.
     * @param character Character to initialize
     * @param state Pointer to the client state the character
     *  belongs to
     * @return true on success, false on failure
     */
    bool InitializeCharacter(libcomp::ObjectReference<
        objects::Character>& character,
        channel::ClientState* state);

    /**
     * Persist character data associated to a client that is
     * logging out.
     * @param state Pointer to the client state the character
     *  belongs to
     * @param delay Optional parameter to perform the normal
     *  logout save actions but do not unregister anything.
     *  If this is specified, a second pass to this function
     *  should be performed later.
     * @return true on success, false on failure
     */
    bool LogoutCharacter(channel::ClientState* state,
        bool delay = false);

    /**
     * Unload an object from references and update it in the DB.
     * @param obj Pointer to the object to clean up
     * @param db Pointer to the database to use
     * @param doSave Indicates if the record should be updated
     * @param unregister Indicates if the record should be unloaded
     *  and unregistered
     * @return true on success, false on failure
     */
    template <class T>
    bool Cleanup(const std::shared_ptr<T>& obj,
        const std::shared_ptr<libcomp::Database>& db,
        bool doSave, bool unregister = true)
    {
        if(obj != nullptr)
        {
            if(unregister)
            {
                libcomp::ObjectReference<T>::Unload(obj->GetUUID());
                obj->Unregister();
            }
            return !doSave || obj->Update(db);
        }
        return true;
    }

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ACCOUNTMANAGER_H
