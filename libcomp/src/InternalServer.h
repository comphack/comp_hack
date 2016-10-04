/**
 * @file libcomp/src/InternalServer.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Internal server class.
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

#ifndef LIBCOMP_SRC_INTERNALSERVER_H
#define LIBCOMP_SRC_INTERNALSERVER_H

// libcomp Includes
#include "InternalConnection.h"
#include "TcpServer.h"
#include "Worker.h"

namespace libcomp
{

class InternalServer : public libcomp::TcpServer
{
public:
    InternalServer(String listenAddress, uint16_t port);
    virtual ~InternalServer();

    //todo: override Start and add main loop

    bool ConnectToHostServer(asio::io_service& service, const String& host, int port);

protected:
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    //todo: replace with multiple workers for multi-threading
    libcomp::Worker mWorker;

    std::shared_ptr<libcomp::InternalConnection> hostConnection;
    std::shared_ptr<MessageQueue<libcomp::Message::Message*>> mMessageQueue;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_INTERNALSERVER_H
