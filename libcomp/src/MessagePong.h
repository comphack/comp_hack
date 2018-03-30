/**
 * @file libcomp/src/MessagePong.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Indicates that the server responded to the ping.
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

#ifndef LIBCOMP_SRC_MESSAGEPONG_H
#define LIBCOMP_SRC_MESSAGEPONG_H

// libcomp Includes
#include "CString.h"
#include "Message.h"

// Standard C++11 Includes
#include <memory>

namespace libcomp
{

namespace Message
{

/**
 * Message that acts as a check to see if a server is responding.
 */
class Pong : public Message
{
public:
    /**
     * Create the message.
     */
    Pong();

    /**
     * Cleanup the message.
     */
    virtual ~Pong();

    virtual MessageType GetType() const;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGEPONG_H
