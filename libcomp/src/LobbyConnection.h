/**
 * @file libcomp/src/LobbyConnection.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby connection class.
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

#ifndef LIBCOMP_SRC_LOBBYCONNECTION_H
#define LIBCOMP_SRC_LOBBYCONNECTION_H

// libcomp Includes
#include "MessageQueue.h"
#include "TcpConnection.h"

namespace libcomp
{

namespace Message
{

class Message;

} // namespace Message

class LobbyConnection : public libcomp::TcpConnection
{
public:
    LobbyConnection(asio::io_service& io_service);
    LobbyConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);
    virtual ~LobbyConnection();

    virtual void ConnectionSuccess();

    void SetMessageQueue(const std::shared_ptr<MessageQueue<
        libcomp::Message::Message*>>& messageQueue);

protected:
    typedef void (LobbyConnection::*PacketParser_t)(libcomp::Packet& packet);

    void ParseClientEncryptionStart(libcomp::Packet& packet);
    void ParseServerEncryptionStart(libcomp::Packet& packet);
    void ParseServerEncryptionFinish(libcomp::Packet& packet);
    void ParsePacket(libcomp::Packet& packet);
    void ParsePacket(libcomp::Packet& packet,
        uint32_t paddedSize, uint32_t realSize);

    virtual void SocketError(const libcomp::String& errorMessage =
        libcomp::String());

    virtual void ConnectionEncrypted();

    virtual void PacketReceived(libcomp::Packet& packet);

    virtual void PreparePackets(std::list<ReadOnlyPacket>& packets);

    virtual std::list<ReadOnlyPacket> GetCombinedPackets();

    PacketParser_t mPacketParser;

    std::shared_ptr<MessageQueue<libcomp::Message::Message*>> mMessageQueue;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_LOBBYCONNECTION_H
