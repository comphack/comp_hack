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

/**
 * Class to manage a TCP/IP connection. This class can operate in two roles:
 * @ref ROLE_SERVER and @ref ROLE_Client. The role is only used by derived
 * classes like @ref EncryptedConnection.
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    /**
     * Role the server is operating in.
     */
    typedef enum
    {
        ROLE_SERVER = 0, //!< Server role that can accept connections.
        ROLE_CLIENT,     //!< Client role that will connect to a remote server.
    } Role_t;

    /**
     * Status of the connection.
     */
    typedef enum
    {
        STATUS_NOT_CONNECTED = 0,  //!< Not connected to remote system.
        STATUS_CONNECTING,         //!< Connecting to a remote system.
        STATUS_CONNECTED,          //!< Connected to a remote system.
        STATUS_WAITING_ENCRYPTION, //!< Waiting for encryption to complete.
        STATUS_ENCRYPTED,          //!< Connection is established and encrypted.
    } ConnectionStatus_t;

    /**
     * Create a new client connection.
     * @param io_service ASIO service to manage this connection.
     */
    TcpConnection(asio::io_service& io_service);

    /**
     * Create a new server connection.
     * @param socket Socket provided by the server for the new client.
     * @param pDiffieHellman Asymmetric encryption information.
     */
    TcpConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);

    /**
     * Cleanup the connection object.
     */
    virtual ~TcpConnection();

    /**
     * Get the prime used in the Diffie-Hellman key exchange.
     * @param pDiffieHellman Object that stores the DH key.
     * @return Prime to be used in the DH key exchange.
     */
    static String GetDiffieHellmanPrime(const DH *pDiffieHellman);

    /**
     * Generate the public key for the Diffie-Hellman key exchange.
     * @param pDiffieHellman Object that stores the DH key.
     * @return Public key to be used in the DH key exchange.
     */
    static String GenerateDiffieHellmanPublic(DH *pDiffieHellman);

    /**
     * Return the shared private from the Diffie-Hellman key exchange.
     * @param pDiffieHellman Object that stores the DH key.
     * @param otherPublic Public key from the other party.
     * @return Shared private data upon completion of the DH key exchange.
     */
    static std::vector<char> GenerateDiffieHellmanSharedData(
        DH *pDiffieHellman, const String& otherPublic);

    /**
     * Connect to the remote host (client role).
     * @param host IP or DNS name of the remote host.
     * @param port Port of the remote host to connect to.
     * @param async If the connection should be asynchronous.
     * @return true on success; false otherwise.
     */
    bool Connect(const String& host, uint16_t port = 0, bool async = true);

    /**
     * Close the connection to the remote host.
     * @return true on success; false otherwise.
     */
    virtual bool Close();

    /**
     * Queue a packet to be sent.
     * @note This will not send the packet until @ref SendPacket or
     *   @ref FlushOutgoing is called.
     * @param packet Packet to send to the remote host.
     */
    virtual void QueuePacket(Packet& packet);

    /**
     * Queue a packet to be sent.
     * @note This will not send the packet until @ref SendPacket or
     *   @ref FlushOutgoing is called.
     * @param packet Packet to send to the remote host.
     */
    virtual void QueuePacket(ReadOnlyPacket& packet);

    /**
     * Queue a copy of a packet to be sent.
     * @note This will not send the packet until @ref SendPacket or
     *   @ref FlushOutgoing is called.
     * @param packet Packet to send to the remote host.
     */
    virtual void QueuePacketCopy(libcomp::Packet& packet);

    /**
     * Queue an object to be packetized and sent.
     * @note This will not send the packet until @ref SendPacket or
     *   @ref FlushOutgoing is called.
     * @param obj Object to be packetized.
     * @return true if the object could be packetized; false otherwise.
     */
    virtual bool QueueObject(const Object& obj);

    /**
     * Queue a packet and then send all queued packets to the remote host.
     * @param packet Packet to send to the remote host.
     * @param closeConnection If the connection should be closed after the
     *   send queue has been emptied.
     */
    virtual void SendPacket(Packet& packet, bool closeConnection = false);

    /**
     * Queue a packet and then send all queued packets to the remote host.
     * @param packet Packet to send to the remote host.
     * @param closeConnection If the connection should be closed after the
     *   send queue has been emptied.
     */
    virtual void SendPacket(ReadOnlyPacket& packet,
        bool closeConnection = false);

    /**
     * Queue a copy of a packet and then send all queued packets to the
     * remote host.
     * @param packet Packet to send to the remote host.
     * @param closeConnection If the connection should be closed after the
     *   send queue has been emptied.
     */
    virtual void SendPacketCopy(libcomp::Packet& packet,
        bool closeConnection = false);

    /**
     * Packetize and queue an object and then send all queued packets to the
     *   remote host.
     * @param obj Object to be packetized.
     * @param closeConnection If the connection should be closed after the
     *   send queue has been emptied.
     * @return true if the object could be packetized; false otherwise.
     */
    virtual bool SendObject(const Object& obj, bool closeConnection = false);

    /**
     * Send all queued packets to the remote host.
     * @param closeConnection If the connection should be closed after the
     *   send queue has been emptied.
     */
    void FlushOutgoing(bool closeConnection = false);


    /**
     * Start a receive request for more packet data. The @ref PacketReceived
     *   function will be called but it may return less bytes than requested.
     * @param size Number of bytes requested.
     * @return true on success; false otherwise.
     */
    bool RequestPacket(size_t size);

    /**
     * Get the role the connection is operating in.
     * @return Role the connection is operating in.
     */
    Role_t GetRole() const;

    /**
     * Get the status of the connection.
     * @return Status of the connection.
     */
    ConnectionStatus_t GetStatus() const;

    /**
     * Get the address of the remote host.
     * @return Address of the remote host.
     */
    String GetRemoteAddress() const;

    /**
     * Called when a connection has been established.
     */
    virtual void ConnectionSuccess();

    /**
     * Send a packet to a list of connections.
     * @param connections List of connections to send the packet to.
     * @param packet Packet to send to the list of connections.
     */
    static void BroadcastPacket(const std::list<std::shared_ptr<
        TcpConnection>>& connections, Packet& packet);

    /**
     * Send a packet to a list of connections.
     * @param connections List of connections to send the packet to.
     * @param packet Packet to send to the list of connections.
     */
    static void BroadcastPacket(const std::list<std::shared_ptr<
        TcpConnection>>& connections, ReadOnlyPacket& packet);

protected:
    /**
     * Internal connect function to an ASIO end point.
     * @param endpoint End point to connect to.
     * @param async If the connection should be asynchronous.
     */
    virtual void Connect(const asio::ip::tcp::endpoint& endpoint,
        bool async = true);

    /**
     * Report a socket error. This should disconnect the connection.
     * @param errorMessage Error message to report.
     */
    virtual void SocketError(const String& errorMessage = String());

    /**
     * Called if a connection attempt has failed.
     */
    virtual void ConnectionFailed();

    /**
     * Called after a packet has been sent to the remote host.
     * @param packet Packet that was sent to the remote host.
     */
    virtual void PacketSent(ReadOnlyPacket& packet);

    /**
     * Called after a packet has been received from the remote host.
     * @param packet Packet that was received.
     */
    virtual void PacketReceived(Packet& packet);

    /**
     * Called to prepare packets before they are sent to the remote host.
     * @param packets List of packets to be sent to the remote host.
     */
    virtual void PreparePackets(std::list<ReadOnlyPacket>& packets);

    /**
     * Returns a list of packets that have been combined.
     * @return List of packet that have been combined.
     */
    virtual std::list<ReadOnlyPacket> GetCombinedPackets();

    /**
     * Sets the Blowfish encryption key to be used.
     * @param data Blowfish encryption key.
     */
    void SetEncryptionKey(const std::vector<char>& data);

    /**
     * Sets the Blowfish encryption key to be used.
     * @param pData Pointer to the buffer were the key resides.
     * @param dataSize Size of the encryption key data.
     */
    void SetEncryptionKey(const void *pData, size_t dataSize);

private:
    /**
     * Send all queued packets to the remote host.
     * @param closeConnection If the connection should be closed after the
     *   send queue has been emptied.
     */
    void FlushOutgoingInside(bool closeConnection = false);

    /**
     * Used to handle a connection error code.
     * @param errorCode Error code that was encountered.
     */
    void HandleConnection(asio::error_code errorCode);

    /**
     * Sends the next packet in the queue to the remote device.
     */
    void SendNextPacket();

    /// ASIO network socket for the connection.
    asio::ip::tcp::socket mSocket;

protected:
    /// Diffie-Hellman key exchange data.
    DH *mDiffieHellman;

    /// Blowfish encryption key.
    BF_KEY mEncryptionKey;

    /// Status of the connection.
    ConnectionStatus_t mStatus;

private:
    /// Role of the connection.
    Role_t mRole;

    /// Last received packet.
    Packet mReceivedPacket;

    /// Cached address of the remote host.
    String mRemoteAddress;

protected:
    /// Mutex to ensure the outgoing packet code is executed from one thread.
    std::mutex mOutgoingMutex;

    /// List of packets to be sent to the remote host.
    std::list<ReadOnlyPacket> mOutgoingPackets;

    /// Indicates if an outgoing packet is being sent.
    bool mSendingPacket;

    /// Packet being sent to the remote host.
    ReadOnlyPacket mOutgoing;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_TCPCONNECTION_H
