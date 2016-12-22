/**
 * @file libcomp/src/MessageTimeout.cpp
 * @ingroup libcomp
 *
 * @brief Indicates that a timeout has occurred.
 *
 * @brief Indicates that the server should shutdown cleanly.
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

#include "MessageTimeout.h"

using namespace libcomp;

Message::Timeout::Timeout()
{
}

Message::Timeout::~Timeout()
{
}

Message::MessageType Message::Timeout::GetType() const
{
    return MessageType::MESSAGE_TYPE_SYSTEM;
}
