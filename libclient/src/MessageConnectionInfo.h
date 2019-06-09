/**
 * @file libcomp/src/MessageConnectionInfo.h
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

#ifndef LIBCLIENT_SRC_MESSAGECONNECTIONINFO_H
#define LIBCLIENT_SRC_MESSAGECONNECTIONINFO_H

// libcomp Includes
#include <CString.h>
#include <MessageClient.h>

namespace logic
{

/**
 * Message signifying that a connection should be made to a server.
 */
class MessageConnectionInfo : public libcomp::Message::MessageClient
{
public:
    /**
     * Create the message.
     * @param connectionID ID for the connection.
     * @param host Host to connect to.
     * @param port Port on the host to connect to.
     */
    MessageConnectionInfo(const libcomp::String& connectionID,
        const libcomp::String& host, uint16_t port) :
        mHost(host), mPort(port), mConnectionID(connectionID) { }

    /**
     * Cleanup the message.
     */
    ~MessageConnectionInfo() override { }

    /**
     * Get the host to connect to.
     * @returns Host to connect to.
     */
    libcomp::String GetHost() const { return mHost; }

    /**
     * Get the port to connect to.
     * @returns Port on the host to connect to.
     */
    uint16_t GetPort() const { return mPort; }

    /**
     * Get the connection ID.
     * @returns Connection ID.
     */
    libcomp::String GetConnectionID() const { return mConnectionID; }

protected:
    /// Host to connect to.
    libcomp::String mHost;

    /// Port on the host to connect to.
    uint16_t mPort;

    /// Connection ID.
    libcomp::String mConnectionID;
};

/**
 * Message signifying that a connection should be made to a lobby server.
 */
class MessageConnectToLobby : public MessageConnectionInfo
{
public:
    /**
     * Create the message.
     * @param connectionID ID for the connection.
     * @param host Host to connect to.
     * @param port Port on the host to connect to.
     */
    MessageConnectToLobby(const libcomp::String& connectionID = "lobby",
        const libcomp::String& host = "127.0.0.1", uint16_t port = 10666) :
        MessageConnectionInfo(connectionID, host, port) { }

    /**
     * Cleanup the message.
     */
    ~MessageConnectToLobby() override { }

    /**
     * Get the specific client message type.
     * @return The message's client message type
     */
    libcomp::Message::MessageClientType GetMessageClientType() const override {
        return libcomp::Message::MessageClientType::CONNECT_TO_LOBBY;
    }

    /**
     * Dump the message for logging.
     * @return String representation of the message.
     */
    libcomp::String Dump() const override
    {
        return libcomp::String("Message: Connect to lobby server\n"
            "ID: %1\nServer: %2:%3").Arg(mConnectionID).Arg(mHost).Arg(mPort);
    }
};

/**
 * Message signifying that a connection should be made to a channel server.
 */
class MessageConnectToChannel : public MessageConnectionInfo
{
public:
    /**
     * Create the message.
     * @param connectionID ID for the connection.
     * @param host Host to connect to.
     * @param port Port on the host to connect to.
     */
    MessageConnectToChannel(const libcomp::String& connectionID = "channel",
        const libcomp::String& host = "127.0.0.1", uint16_t port = 14666) :
        MessageConnectionInfo(connectionID, host, port) { }

    /**
     * Cleanup the message.
     */
    ~MessageConnectToChannel() override { }

    /**
     * Get the specific client message type.
     * @return The message's client message type
     */
    libcomp::Message::MessageClientType GetMessageClientType() const override {
        return libcomp::Message::MessageClientType::CONNECT_TO_CHANNEL;
    }

    /**
     * Dump the message for logging.
     * @return String representation of the message.
     */
    libcomp::String Dump() const override
    {
        return libcomp::String("Message: Connect to channel server\n"
            "ID: %1\nServer: %2:%3").Arg(mConnectionID).Arg(mHost).Arg(mPort);
    }
};

} // namespace logic

#endif // LIBCLIENT_SRC_MESSAGECONNECTIONINFO_H
