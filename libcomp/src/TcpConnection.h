/**
 * @file libcomp/src/TcpServer.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base TCP/IP connection class.
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

#ifndef LIBCOMP_SRC_TCPCONNECTION_H
#define LIBCOMP_SRC_TCPCONNECTION_H

// libcomp Includes
#include "CString.h"
#include "Packet.h"

// Boost ASIO Includes
#include "PushIgnore.h"
#include <asio.hpp>
#include "PopIgnore.h"

// OpenSSL Includes
#include <openssl/dh.h>
#include <openssl/blowfish.h>

// Standard C++11 Includes
#include <mutex>

namespace libcomp
{

class Object;

class TcpConnection
{
public:
    typedef enum
    {
        ROLE_SERVER = 0,
        ROLE_CLIENT,
    } Role_t;

    typedef enum
    {
        STATUS_NOT_CONNECTED = 0,
        STATUS_CONNECTING,
        STATUS_CONNECTED,
        STATUS_WAITING_ENCRYPTION,
        STATUS_ENCRYPTED,
    } ConnectionStatus_t;

    TcpConnection(asio::io_service& io_service);
    TcpConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);
    virtual ~TcpConnection();

    static String GetDiffieHellmanPrime(const DH *pDiffieHellman);
    static String GenerateDiffieHellmanPublic(DH *pDiffieHellman);
    static std::vector<char> GenerateDiffieHellmanSharedData(
        DH *pDiffieHellman, const String& otherPublic);

    bool Connect(const String& host, uint16_t port = 0, bool async = true);

    virtual bool Close();

    virtual void QueuePacket(Packet& packet);
    virtual void QueuePacket(ReadOnlyPacket& packet);

    virtual bool QueueObject(const Object& obj);

    virtual void SendPacket(Packet& packet, bool closeConnection = false);
    virtual void SendPacket(ReadOnlyPacket& packet,
        bool closeConnection = false);

    virtual bool SendObject(const Object& obj, bool closeConnection = false);

    void FlushOutgoing(bool closeConnection = false);

    bool RequestPacket(size_t size);

    Role_t GetRole() const;
    ConnectionStatus_t GetStatus() const;

    String GetRemoteAddress() const;

    void SetSelf(const std::weak_ptr<libcomp::TcpConnection>& self);

    virtual void ConnectionSuccess();

    static void BroadcastPacket(const std::list<std::shared_ptr<
        TcpConnection>>& connections, Packet& packet);
    static void BroadcastPacket(const std::list<std::shared_ptr<
        TcpConnection>>& connections, ReadOnlyPacket& packet);

protected:
    virtual void Connect(const asio::ip::tcp::endpoint& endpoint, bool async = true);

    virtual void SocketError(const String& errorMessage = String());

    virtual void ConnectionFailed();

    virtual void PacketSent(ReadOnlyPacket& packet);
    virtual void PacketReceived(Packet& packet);

    virtual void PreparePackets(std::list<ReadOnlyPacket>& packets);

    virtual std::list<ReadOnlyPacket> GetCombinedPackets();

    void SetEncryptionKey(const std::vector<char>& data);
    void SetEncryptionKey(const void *pData, size_t dataSize);

private:
    void HandleConnection(asio::error_code errorCode);
    void SendNextPacket();

    asio::ip::tcp::socket mSocket;

protected:
    DH *mDiffieHellman;
    BF_KEY mEncryptionKey;

    ConnectionStatus_t mStatus;

    std::weak_ptr<libcomp::TcpConnection> mSelf;

private:
    Role_t mRole;

    Packet mReceivedPacket;

    String mRemoteAddress;

protected:
    std::mutex mOutgoingMutex;
    std::list<ReadOnlyPacket> mOutgoingPackets;

    bool mSendingPacket;
    ReadOnlyPacket mOutgoing;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_TCPCONNECTION_H
