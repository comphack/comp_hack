/**
 * @file libcomp/src/Message.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base message class.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_MESSAGE_H
#define LIBCOMP_SRC_MESSAGE_H

namespace libcomp
{

namespace Message
{

/**
 * Message type used to determine what type of @ref Manager should handle it.
 */
enum class MessageType
{
    MESSAGE_TYPE_SYSTEM,        //!< Message is a special system message type.
    MESSAGE_TYPE_PACKET,        //!< Message is of type @ref MessagePacket.
    MESSAGE_TYPE_CONNECTION,    //!< Message is of type @ref ConnectionMessage.
};

/**
 * Abstract base class representing a message to be handled when received by
 * a @ref MessageQueue.
 */
class Message
{
public:
    /**
     * Cleanup the message.
     */
    virtual ~Message() { }

    /**
     * Get the message's type.
     * @return The message's type.
     */
    virtual MessageType GetType() const = 0;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGE_H
