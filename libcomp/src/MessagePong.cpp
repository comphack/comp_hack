/**
 * @file libcomp/src/MessagePong.cpp
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

#include "MessagePong.h"

using namespace libcomp;

Message::Pong::Pong()
{
}

Message::Pong::~Pong()
{
}

Message::MessageType Message::Pong::GetType() const
{
    return MessageType::MESSAGE_TYPE_CONNECTION;
}

libcomp::String Message::Pong::Dump() const
{
    return "Message: Pong";
}
