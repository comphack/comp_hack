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
    return hostConnection->GetStatus() == libcomp::TcpConnection::STATUS_CONNECTED;
}

std::shared_ptr<libcomp::TcpConnection> InternalServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto iConnection = new InternalConnection(socket, CopyDiffieHellman(
                                                GetDiffieHellman())
                                            );

    iConnection->SetMessageQueue(mMessageQueue);

    auto connection = std::shared_ptr<libcomp::TcpConnection>(iConnection);
    // Make sure this is called after connecting.
    connection->SetSelf(connection);
    connection->ConnectionSuccess();

    return connection;
}

void InternalServer::Run()
{
    std::list<libcomp::Message::Message*> mQueue;

    //Loop until the program shuts down
    while (true)
    {
        //Pull new messages
        //todo: make sure messages that need to be processed together are either one message or are in the queue at the same time
        mMessageQueue->DequeueAll(mQueue);
        if (mQueue.size() > 0)
        {
            //todo: handle server side messages

            //Process messages
            for (auto iter = mQueue.begin(); iter != mQueue.end(); iter++)
            {
                auto handler = GetMessageHandler(**iter);
                if (handler != nullptr)
                {
                    //todo: add worker message handling
                }
                else
                {
                    //todo: display unrecognized message error
                }
            }

            //Clear queue and move on
            mQueue.clear();
        }

        //todo: clean up connections

        //todo: replace with?
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}