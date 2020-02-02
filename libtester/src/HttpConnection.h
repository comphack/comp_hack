/**
 * @file libtester/src/HttpConnection.h
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief HTTP connection for the lobby login sequence.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBTESTER_SRC_HTTPCONNECTION_H
#define LIBTESTER_SRC_HTTPCONNECTION_H

// libcomp Includes
#include <MessageQueue.h>
#include <TcpConnection.h>

namespace libcomp
{

namespace Message
{

class Message;

} // namespace Message

} // namespace libcomp

namespace libtester
{

class HttpConnection : public libcomp::TcpConnection
{
public:
    HttpConnection(asio::io_service& io_service);

    virtual void ConnectionSuccess();

    void SetMessageQueue(const std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>>& messageQueue);

protected:
    virtual void PacketReceived(libcomp::Packet& packet);

    std::string mRequest;
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mMessageQueue;
};

} // namespace libtester

#endif // LIBTESTER_SRC_HTTPCONNECTION_H
