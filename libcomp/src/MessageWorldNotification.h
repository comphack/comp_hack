/**
 * @file libcomp/src/MessageWorldNotification.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Indicates that a world server has been started.
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

#ifndef LIBCOMP_SRC_MESSAGEWORLDNOTIFICATION_H
#define LIBCOMP_SRC_MESSAGEWORLDNOTIFICATION_H

// libcomp Includes
#include "CString.h"
#include "ConnectionMessage.h"

// Standard C++11 Includes
#include <memory>

namespace libcomp
{

namespace Message
{

/**
 * Message that signifies that a world wants to connect to the
 * lobby.  Upon successfully receiving this message the lobby
 * will close the connection and "reverse it" so the lobby
 * maintains the connections instead.
 */
class WorldNotification : public ConnectionMessage
{
public:
    /**
     * Create the message.
     * @param address The address the connection is coming from
     * @param port The port the connection is coming from
     */
    WorldNotification(const String& address, uint16_t port);

    /**
     * Cleanup the message.
     */
    virtual ~WorldNotification();

    /**
     * Get the address the connection is coming from.
     * @return The address the connection is coming from
     */
    String GetAddress() const;

    /**
     * Get the port the connection is coming from.
     * @return The port the connection is coming from
     */
    uint16_t GetPort() const;

    virtual ConnectionMessageType GetConnectionMessageType() const;

    virtual libcomp::String Dump() const override;

private:
    /// The address the connection is coming from
    String mAddress;

    /// The port the connection is coming from
    uint16_t mPort;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGEWORLDNOTIFICATION_H
