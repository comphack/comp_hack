/**
 * @file libcomp/src/MessageConnectionClosed.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Indicates that a connection has closed and should be cleaned up.
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

#ifndef LIBCOMP_SRC_MESSAGECONNECTIONCLOSED_H
#define LIBCOMP_SRC_MESSAGECONNECTIONCLOSED_H

// libcomp Includes
#include "CString.h"
#include "ConnectionMessage.h"
#include "TcpConnection.h"

namespace libcomp
{

namespace Message
{

class ConnectionClosed : public ConnectionMessage
{
public:
    ConnectionClosed(std::shared_ptr<TcpConnection> connection);
    virtual ~ConnectionClosed();

    std::shared_ptr<TcpConnection> GetConnection() const;

    virtual ConnectionMessageType GetConnectionMessageType() const;

private:
    std::shared_ptr<TcpConnection> mConnection;
};

} // namespace Message

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGECONNECTIONCLOSED_H
