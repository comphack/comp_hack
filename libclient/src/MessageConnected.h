/**
 * @file libcomp/src/MessageConnected.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Client message.
 *
 * This file is part of the COMP_hack Client Library (libclient).
 *
 * Copyright (C) 2012-2019 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCLIENT_SRC_MESSAGECONNECTED_H
#define LIBCLIENT_SRC_MESSAGECONNECTED_H

// libcomp Includes
#include <CString.h>
#include <EnumUtils.h>
#include <ErrorCodes.h>
#include <MessageClient.h>

namespace logic
{

/**
 * Message signifying that a connection has been established.
 */
class MessageConnected : public libcomp::Message::MessageClient
{
public:
    /**
     * Create the message.
     * @param connectionID ID for the connection.
     * @param errorCode Error code from authentication.
     */
    MessageConnected(const libcomp::String& connectionID,
        ErrorCodes_t errorCode) : mConnectionID(connectionID),
        mErrorCode(errorCode) { }

    /**
     * Cleanup the message.
     */
    ~MessageConnected() override { }

    /**
     * Get the connection ID.
     * @returns Connection ID.
     */
    libcomp::String GetConnectionID() const { return mConnectionID; }

    /**
     * Get the error code from authentication.
     * @returns Error code from authentication.
     */
    ErrorCodes_t GetErrorCode() const { return mErrorCode; }

protected:
    /// Connection ID.
    libcomp::String mConnectionID;

    /// Error code from authentication.
    ErrorCodes_t mErrorCode;
};

/**
 * Message signifying that a connection has been established to the lobby.
 */
class MessageConnectedToLobby : public MessageConnected
{
public:
    /**
     * Create the message.
     * @param connectionID ID for the connection.
     * @param errorCode Error code from authentication.
     * @param sid Session ID for this connection.
     */
    MessageConnectedToLobby(const libcomp::String& connectionID,
        ErrorCodes_t errorCode, const libcomp::String& sid = {}) :
        MessageConnected(connectionID, errorCode), mSID(sid) { }

    /**
     * Cleanup the message.
     */
    ~MessageConnectedToLobby() override { }

    /**
     * Get the session ID for this connection.
     * @returns Session ID for this connection.
     */
    libcomp::String GetSID() const { return mSID; }

    /**
     * Get the specific client message type.
     * @return The message's client message type
     */
    libcomp::Message::MessageClientType GetMessageClientType() const override {
        return libcomp::Message::MessageClientType::CONNECTED_TO_LOBBY;
    }

    /**
     * Dump the message for logging.
     * @return String representation of the message.
     */
    libcomp::String Dump() const override
    {
        return libcomp::String("Message: Connected to lobby server\n"
            "ID: %1\nError: %2").Arg(mConnectionID).Arg(
            to_underlying(mErrorCode));
    }

private:
    /// Session ID for this connection.
    libcomp::String mSID;
};

/**
 * Message signifying that a connection has been established to the channel.
 */
class MessageConnectedToChannel : public MessageConnected
{
public:
    /**
     * Create the message.
     * @param connectionID ID for the connection.
     * @param errorCode Error code from authentication.
     */
    MessageConnectedToChannel(const libcomp::String& connectionID,
        ErrorCodes_t errorCode) : MessageConnected(connectionID, errorCode) { }

    /**
     * Cleanup the message.
     */
    ~MessageConnectedToChannel() override { }

    /**
     * Get the specific client message type.
     * @return The message's client message type
     */
    libcomp::Message::MessageClientType GetMessageClientType() const override {
        return libcomp::Message::MessageClientType::CONNECTED_TO_CHANNEL;
    }

    /**
     * Dump the message for logging.
     * @return String representation of the message.
     */
    libcomp::String Dump() const override
    {
        return libcomp::String("Message: Connected to channel server\n"
            "ID: %1\nError: %2").Arg(mConnectionID).Arg(
            to_underlying(mErrorCode));
    }
};

} // namespace logic

#endif // LIBCLIENT_SRC_MESSAGECONNECTED_H
