/**
 * @file libcomp/src/InternalServer.cpp
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

#include "InternalServer.h"

 // libcomp Includes
#include "InternalConnection.h"
#include "Log.h"

using namespace libcomp;

InternalServer::InternalServer(String listenAddress, uint16_t port) :
    TcpServer(listenAddress, port)
{
}

InternalServer::~InternalServer()
{
    //todo: stop workers

    if (hostConnection != nullptr)
    {
        //todo: disconnect properly
    }
}

bool InternalServer::ConnectToHostServer(asio::io_service& service, const String& host, int port)
{
    hostConnection = std::shared_ptr<libcomp::InternalConnection>(new libcomp::InternalConnection(service));
    hostConnection->Connect(host, port, false);

    bool connected = hostConnection->GetStatus() == libcomp::TcpConnection::STATUS_CONNECTED;
    if (connected)
    {
        hostConnection->SetMessageQueue(mWorker.GetMessageQueue());
    }
    return connected;
}

std::shared_ptr<libcomp::TcpConnection> InternalServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto connection = std::shared_ptr<libcomp::TcpConnection>(new InternalConnection(socket, CopyDiffieHellman(
                                                GetDiffieHellman())
                                            ));

    std::dynamic_pointer_cast<libcomp::InternalConnection>(connection)->SetMessageQueue(mWorker.GetMessageQueue());

    // Make sure this is called after connecting.
    connection->SetSelf(connection);
    connection->ConnectionSuccess();

    return connection;
}