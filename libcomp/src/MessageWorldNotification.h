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

class WorldNotification : public ConnectionMessage
{
public:
    WorldNotification(const String& address, uint16_t port);
    virtual ~WorldNotification();

    String GetAddress() const;

    uint16_t GetPort() const;

    virtual ConnectionMessageType GetConnectionMessageType() const;

private:
    String mAddress;
    uint16_t mPort;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGEWORLDNOTIFICATION_H
