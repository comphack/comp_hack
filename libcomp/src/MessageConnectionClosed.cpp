/**
 * @file libcomp/src/MessageConnectionClosed.cpp
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Indicates that a connection has closed and should be cleaned up.
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

#include "MessageConnectionClosed.h"

using namespace libcomp;

Message::ConnectionClosed::ConnectionClosed(TcpConnection* connection) :
    mConnection(connection)
{
}

Message::ConnectionClosed::~ConnectionClosed()
{
}

TcpConnection* Message::ConnectionClosed::GetConnection() const
{
    return mConnection;
}

Message::MessageType Message::ConnectionClosed::GetType() const
{
    return MessageType::MESSAGE_TYPE_CONNECTION;
}
