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

#ifndef LIBCOMP_SRC_MESSAGE_H
#define LIBCOMP_SRC_MESSAGE_H

namespace libcomp
{

namespace Message
{

enum class MessageType
{
    MESSAGE_TYPE_PACKET,
    MESSAGE_TYPE_CONNECTION,
};

class Message
{
public:
    virtual ~Message() { }

    virtual MessageType GetType() const = 0;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGE_H
