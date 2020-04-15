/**
 * @file libcomp/src/MessageClient.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Base message class for connection based messages.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "Message.h"

#ifndef LIBCOMP_SRC_MESSAGECLIENT_H
#define LIBCOMP_SRC_MESSAGECLIENT_H

namespace libcomp
{

namespace Message
{

/**
 * Specific connection message type.
 */
enum class MessageClientType : int32_t
{
    //
    // ConnectionManager related requests
    //

    /// Connect to the lobby server
    CONNECT_TO_LOBBY = 1000,
    /// Connect to the channel server
    CONNECT_TO_CHANNEL,
    /// Close the active connection
    CONNECTION_CLOSE,

    //
    // ConnectionManager related events
    //

    /// Now connected to the lobby
    CONNECTED_TO_LOBBY = 2000,
    /// Now connected to the channel
    CONNECTED_TO_CHANNEL,
};

/**
 * Message signifying that a connection based action has occurred.
 */
class MessageClient : public Message
{
public:
    /**
     * Cleanup the message.
     */
    virtual ~MessageClient() { }

    virtual MessageType GetType() const { return MessageType::MESSAGE_TYPE_CLIENT; }

    /**
     * Get the specific client message type.
     * @return The message's client message type
     */
    virtual MessageClientType GetMessageClientType() const = 0;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGECLIENT_H
