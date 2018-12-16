/**
 * @file libcomp/src/MessageTimeout.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Indicates that a timeout has occurred.
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

#ifndef LIBCOMP_SRC_MESSAGETIMEOUT_H
#define LIBCOMP_SRC_MESSAGETIMEOUT_H

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
 * Message that signifies a connection has timed out.
 */
class Timeout : public Message
{
public:
    /**
     * Create the message.
     */
    Timeout();

    /**
     * Cleanup the message.
     */
    virtual ~Timeout();

    virtual MessageType GetType() const;

    virtual libcomp::String Dump() const override;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGETIMEOUT_H
