/**
 * @file tools/logger/src/LobbyConnection.h
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the class used to control a connection to the lobby
 * server.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_LOGGER_SRC_LOBBYCONNECTION_H
#define TOOLS_LOGGER_SRC_LOBBYCONNECTION_H

#include <Packet.h>

#include <PushIgnore.h>
#include <QFile>
#include <QThread>
#include <QTcpSocket>
#include <PopIgnore.h>

#include <openssl/blowfish.h>

class LoggerServer;

/**
 * Proxy connection between the logger and the lobby server.
 */
class LobbyConnection : public QThread
{
    Q_OBJECT

public:
    /**
     * Create a new lobby connection.
     * @arg server @ref LoggerServer this connection was made from.
     * @arg socketDescriptor Netowrk socket for the connection.
     * @arg clientID ID to identify this client in the log.
     * @arg parent Parent object that this object belongs to. Should remain 0.
     */
    LobbyConnection(LoggerServer *server, qintptr socketDescriptor,
        uint32_t clientID, QObject *parent = 0);

    /**
     * Delete the connection.
     */
    virtual ~LobbyConnection();

protected slots:
    /**
     * This method is called when the client closes the connection. The
     * connection to the lobby server will be closed and the connection object
     * will be deleted.
     */
    void clientLost();

    /**
     * This method is called when new data has arrived from the client. The
     * data will be parsed and then sent to the lobby server.
     */
    void clientReady();

    /**
     * This method is called when the lobby server closes the connection. The
     * connection to the client will be closed and the connection object will
     * be deleted.
     */
    void serverLost();

    /**
     * This method is called when new data has arrived from the lobby server.
     * The data will be parsed and then sent to the client.
     */
    void serverReady();

    /**
     * This method is called when the connection to the lobby server is opened.
     * The client hello packet will be sent to the lobby server.
     */
    void sendClientHello();

protected:
    /**
     * This method is called when the connection thread starts executing.
     */
    virtual void run();

    /**
     * Generate a timestamp to be used in the log.
     * @returns The timestamp string.
     */
    QString timestamp() const;

    /**
     * Exchange encryption keys with the lobby server.
     */
    void exchangeKeys();

    /**
     * Encrypt a packet.
     * @arg key Blowfish key to encrypt with.
     * @arg p Packet to be encrypted.
     */
    void encryptPacket(const BF_KEY *key, libcomp::Packet& p);

    /**
     * Decrypt a packet.
     * @arg key Blowfish key to decrypt with.
     * @arg p Packet to be decrypted.
     */
    void decryptPacket(const BF_KEY *key, libcomp::Packet& p);

    /**
     * Encrypt a packet that has been modified. This method will adjust the
     * sizes and padding of the packet as required.
     * @arg key Blowfish key to encrypt with.
     * @arg p Packet to be encrypted.
     */
    void encryptCustomPacket(const BF_KEY *key, libcomp::Packet& p);

    /**
     * Log a message to the console and GUI.
     * @arg msg Message to log.
     */
    void logMessage(const QString& msg);

    /**
     * Log a packet to the capture file and send it to the live capgrep.
     * @arg p Packet to log.
     * @arg source Source of the packet. 0 if the packet came from the client,
     * 1 if the packet came from the server.
     */
    void logPacket(libcomp::Packet& p, uint8_t source);

    /**
     * Parse the start game packet, modify it to direct to the logger, and send
     * the modified packet to the client.
     * @arg p Packet that contains the original start game data.
     */
    void parseStartGamePacket(libcomp::Packet& p);

private:
    /**
     * State of the lobby connection.
     */
    typedef enum _ConnectionState
    {
        /// Socket is not connected.
        NotConnected = 0,
        /// Socket is connected awaiting data.
        Connected,
        /// Encryption key exchange has started.
        ExchangeStarted,
        /// Connection is encrypted and running normally.
        Encrypted
    }ConnectionState;

    /**
     * Cryptographic data for one of the connections.
     */
    typedef struct _CryptData
    {
        /// Blowfish key schedule.
        BF_KEY key;

        /// Base integer for the Diffie-Hellman key exchange.
        libcomp::String base;

        /// Prime integer for the Diffie-Hellman key exchange.
        libcomp::String prime;

        /// Generated secret integer for the Diffie-Hellman key exchange.
        libcomp::String secret;

        /// Public value for the Diffie-Hellman key exchange provided by the
        /// remote connection.
        libcomp::String serverPublic;

        /// Public value for the Diffie-Hellman key exchange generated by the
        /// lobby connection.
        libcomp::String clientPublic;

        /// The shared key converted into bytes.
        QByteArray keys;

        /// The shared key from the Diffie-Hellman key exchange.
        libcomp::String sharedKey;
    }CryptData;

    /// Logger server this connection belongs to.
    LoggerServer *mServer;

    /// Connection state for the connection to the client.
    ConnectionState mClientState;

    /// Connection state for the connection to the target server.
    ConnectionState mServerState;

    /// Buffer to store the login packet before it is sent to the server.
    uint8_t *mClientLoginPacket;

    /// Size of the login packet that is waiting to be sent to the server.
    uint32_t mClientLoginPacketSize;

    /// Crypto data for the connection to the client.
    CryptData mClientCryptData;

    /// Crypto data for the connection to the target server.
    CryptData mServerCryptData;

    /// Generated packet to be sent to the client upon connection.
    libcomp::Packet mKeyExchangePacket;

    /// Connection to the client.
    QTcpSocket *mClientSocket;

    /// Connection to the target server.
    QTcpSocket *mServerSocket;

    /// Username used to log into the lobby server.
    libcomp::String mUsername;

    /// IP address of the client connection.
    QString mClientAddress;

    /// Version of the client used (1.234U would be 1234).
    uint32_t mClientVer;

    /// Socket descriptior of the client connection.
    qintptr mSocketDescriptor;

    /// Unique lobby ID of this client connection.
    uint32_t mClientID;

    /// File to write the capture log to.
    QFile mCaptureLog;
};

#endif // TOOLS_LOGGER_SRC_LOBBYCONNECTION_H
