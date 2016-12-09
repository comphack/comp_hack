/**
 * @file libcomp/src/ConnectionMessage.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Base message class for connection based messages.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#include "Message.h"

#ifndef LIBCOMP_SRC_CONNECTIONMESSAGE_H
#define LIBCOMP_SRC_CONNECTIONMESSAGE_H

namespace libcomp
{

namespace Message
{

enum class ConnectionMessageType
{
    CONNECTION_MESSAGE_ENCRYPTED,
    CONNECTION_MESSAGE_CONNECTION_CLOSED,
    CONNECTION_MESSAGE_WORLD_NOTIFICATION,
};

class ConnectionMessage : public Message
{
public:
    virtual ~ConnectionMessage() { }

    virtual MessageType GetType() const { return MessageType::MESSAGE_TYPE_CONNECTION; }
    virtual ConnectionMessageType GetConnectionMessageType() const = 0;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_CONNECTIONMESSAGE_H
