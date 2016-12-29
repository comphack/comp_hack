/**
 * @file libcomp/src/EncryptedConnection.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Encrypted connection class.
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

#ifndef LIBCOMP_SRC_ENCRYPTEDCONNECTION_H
#define LIBCOMP_SRC_ENCRYPTEDCONNECTION_H

// libcomp Includes
#include "MessageQueue.h"
#include "TcpConnection.h"

// Standard C++11 Includes
#include <functional>

namespace libcomp
{

namespace Message
{

class Message;

} // namespace Message

/**
 * Represents an encrypted network connection. This connection will perform a
 * Diffie-Hellman key exchange to negotiate a shared private. This shared
 * private will then be used as a Blowfish key to encrypt all other packets.
 *
 * @section encryptseq Encryption Sequence
 *
 * Encryption is initiated by the client sending an 8 byte magic sequence to
 * the server. On receipt of this magic sequence the server will reply with a
 * Diffie-Hellman base, prime, and server public. This packet starts with 4
 * bytes of 0 value followed by hex encoded strings of the base, prime, and
 * server public (in that order). Each string begins with a 32-bit big endian
 * value indicating the size of the string followed by the string data. There
 * is no padding or zero byte on these strings.
 *
 * When the client receives this packet from the server it will perform two
 * tasks. First, it will generate a random number to use as the client private.
 * This client private will be combined with the base and prime using the DH
 * algorithm to generate the client public. This client public is sent to the
 * server. This packet consists of just the client public in a hex encoded
 * string using the same string format described above. Second, the client
 * will use the server public in addition to the base, prime, and client
 * private with the DH algorithm to generate the shared private (calculated by
 * both parties). This shared private is used as the Blowfish key for the
 * encryption of the packets described in the next section. At this point the
 * client is in an encrypted state. New packets send by the client are in the
 * format described in the next section.
 *
 * When the server receives the client public it will use this to generate the
 * same shared private value. The same procedure is used to transition into the
 * encrypted state. At this point the server is in an encrypted state. New
 * packets send by the client are in the format described in the next section.
 *
 * Once in the encrypted state it is the responsibility of the client to
 * initiate the communication flow with the first command packet.
 *
 * @section protocol Packet Protocol
 * @image html packetprotocol.png "Diagram of the Packet Protocol"
 *
 * All packets start with two sizes in 32-bit big endian format. These sizes
 * indicate the padded size (size over the wire) used for encryption and the
 * actual size of the decrypted data. These sizes are of the remaining data.
 * Neither accounts for the 8 bytes consumed by the sizes.
 *
 * The next set of data, the compression information, only appears in channel
 * connections. The magic "gzip" and the magic "lv6\0" indicate the compression
 * method and do not change. While the magic mentions "gzip" it's actually a
 * raw zlib stream without the gzip format. The uncompressed and compressed
 * sizes are in 32-bit little endian format. If the uncompressed and compressed
 * sizes are equal, the data following the compression information is assumed
 * to be uncompressed. Please note that if you are compressing a packet you
 * should check the compressed size against the uncompressed size. If they are
 * equal (or the compressed size is greater than the uncompressed size) the
 * compressed data should be thrown away and the original uncompressed data
 * should be used instead.
 *
 * After the decryption and optional decompression stage, the remaining data is
 * interpreted as a sequence of commands. Each command must have two sizes
 * followed by the command code. Each command may have a number of command
 * specific bytes following the command code. The number of command specific
 * bytes is determined by the command size(s). The command sizes are in 16-bit
 * big and little endian format. The command size is equal to the number of
 * command specific bytes plus 4. This could be adding on the size of the
 * command code and one of the sizes or both sizes but not the command code.
 * Either way, it doesn't make much sense. The command code is in 16-bit little
 * endian format and indicates what action should be taken on receipt of the
 * command. The command code will indicate how to interpret the command
 * specific bytes (if they exist for the given command).
 *
 * The encrypted connection will send a @ref libcomp::Message::Packet to the
 * message queue provided by @ref SetMessageQueue for each command parsed. The
 * message object will provide the command code and the packet data will
 * contain only the command specific data for that specific command.
 */
class EncryptedConnection : public libcomp::TcpConnection
{
public:
    /**
     * Create a new encrypted connection.
     * @param io_service ASIO service to manage this connection.
     */
    EncryptedConnection(asio::io_service& io_service);

    /**
     * Create a new encrypted connection.
     * @param socket Socket provided by the server for the new client.
     * @param pDiffieHellman Asymmetric encryption information.
     */
    EncryptedConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);

    /**
     * Cleanup the connection object.
     */
    virtual ~EncryptedConnection();

    /**
     * Close the connection to the remote host.
     * @return true on success; false otherwise.
     */
    virtual bool Close();

    /**
     * Called when a connection has been established.
     */
    virtual void ConnectionSuccess();

    /**
     * Set the message queue for this connection. When a packet is parsed by
     * the connection it will be sent as a @ref libcomp::Message::Packet to
     * the message queue set by this function.
     * @param messageQueue Message queue to be used by the connection.
     */
    void SetMessageQueue(const std::shared_ptr<MessageQueue<
        libcomp::Message::Message*>>& messageQueue);

protected:
    /**
     * Send a message to the message queue. This takes a function because it
     * may decide the message can't be sent. In this case it will save time by
     * not calling the allocation function.
     * @param messageAllocFunction Function that will construct the message.
     */
    void SendMessage(const std::function<libcomp::Message::Message*(const
        std::shared_ptr<libcomp::TcpConnection>&)>& messageAllocFunction);

    /**
     * Type for a packet parsing function. This is used for the different
     * encryption states to handle incoming packets.
     * @param packet Packet that was received.
     */
    typedef void (EncryptedConnection::*PacketParser_t)(
        libcomp::Packet& packet);

    /**
     * Parse the initial encryption packet from the server. This is used when
     * in the client role. This parser will take the reply from the server and
     * extract the Diffie-Hellman base, prime, and server public. The connection
     * will then generated the shared private and transition to the encrypted
     * state. When this happens, @ref ConnectionEncrypted will be called and a
     * message will be added onto the @ref libcomp::MessageQueue.
     * @sa ConnectionEncrypted
     * @sa SetMessageQueue
     * @sa ParseServerEncryptionStart
     * @sa ParseServerEncryptionFinish
     * @param packet Incoming packet to parse.
     */
    void ParseClientEncryptionStart(libcomp::Packet& packet);

    /**
     * Parse the initial packet from the client. This will check for the
     * encryption magic from the client. This will recognize two additional
     * magic sequences for a ping and world connection request. These allow for
     * a quick non-encrypted notification. The ping will send the client back
     * the same magic sequence and close the connection. The world connection
     * request will trigger an event that may connect back to the client. The
     * port is encoded in the magic and the address is retrieved by the
     * @ref GetRemoteAddress function. For normal encrypted connections, the
     * server will reply with the Diffie-Hellman base, prime, and the server
     * public. It is then the responsibility of the client to perform two
     * tasks. First, the client should use this information to generate the
     * shared private. The shared private should be used as the Blowfish
     * encryption key for future packets. Second, the client should reply to
     * the server with it's client public in clear text. The server will parse
     * this reply in the @ref ParseServerEncryptionFinish function.
     * @sa ParseClientEncryptionStart
     * @sa ParseServerEncryptionFinish
     * @param packet Incoming packet to parse.
     */
    void ParseServerEncryptionStart(libcomp::Packet& packet);

    /**
     * Parse the final client packet needed for encryption. This will parse the
     * client public and generate the shared private. The shared private will
     * then be used as the Blowfish key for future packets. The server will
     * then transition into the encrypted state. When this happens,
     * @ref ConnectionEncrypted will be called and a message will be added onto
     * the @ref libcomp::MessageQueue.
     * @sa ConnectionEncrypted
     * @sa SetMessageQueue
     * @sa ParseClientEncryptionStart
     * @sa ParseServerEncryptionStart
     * @param packet
     */
    void ParseServerEncryptionFinish(libcomp::Packet& packet);

    /**
     * Parse incoming encrypted packet data. This will buffer all incoming
     * data. It will then peek at the first 8 bytes to determine the size of
     * the packet. If a full packet is in the buffer it will call the other
     * version of this function.
     * @param packet Incoming packet data to parse.
     */
    void ParsePacket(libcomp::Packet& packet);

    /**
     * Parse an encrypted packet that has been fully received. This will first
     * decrypt the packet. It will then decompress the packet if needed.
     * Finally it will parse and split out each individual command. For each
     * command, a @ref libcomp::Message::Packet is generated and sent to the
     * message queue set by @ref SetMessageQueue.
     * @sa SetMessageQueue
     * @param packet Encrypted packet to parse.
     * @param paddedSize Over the wire size of the packet.
     * @param realSize Size of the packet after decryption.
     */
    void ParsePacket(libcomp::Packet& packet,
        uint32_t paddedSize, uint32_t realSize);

    /**
     * Parse additional magic sequences. See the description in
     * @ref ParseServerEncryptionStart for what this additional magic sequences
     * are for.
     * @param packet Packet to parse.
     * @return true if the packet was parsed; false otherwise.
     */
    virtual bool ParseExtensionConnection(libcomp::Packet& packet);

    /**
     * Report a socket error. This should disconnect the connection.
     * @param errorMessage Error message to report.
     */
    virtual void SocketError(const libcomp::String& errorMessage =
        libcomp::String());

    /**
     * Called when the connection transitions into the encrypted state.
     */
    virtual void ConnectionEncrypted();

    /**
     * Called after a packet has been received from the remote host.
     * @param packet Packet that was received.
     */
    virtual void PacketReceived(libcomp::Packet& packet);

    /**
     * Called to prepare packets before they are sent to the remote host. This
     * will combine commands into a single over the wire packet. It will then
     * encrypt it. If the connection is not in the encrypted state it will
     * not alter the packet data.
     * @param packets List of packets to be sent to the remote host.
     */
    virtual void PreparePackets(std::list<ReadOnlyPacket>& packets);

    /**
     * Decompress a packet. This base implementation does nothing as only some
     * connections support this part of the protocol.
     * @sa ChannelConnection
     * @sa InternalConnection
     * @sa LobbyConnection
     * @param packet Packet to decompress.
     * @param paddedSize Over the wire size of the packet.
     * @param realSize Actual size of the packet.
     * @param dataStart Offset into the packet of where the compressed data is.
     * @return true on success; false otherwise.
     */
    virtual bool DecompressPacket(libcomp::Packet& packet,
        uint32_t& paddedSize, uint32_t& realSize, uint32_t& dataStart);

    /**
     * Returns a list of packets that have been combined.
     * @return List of packet that have been combined.
     */
    virtual std::list<ReadOnlyPacket> GetCombinedPackets();

    /// Active parse being used for received packets.
    PacketParser_t mPacketParser;

    /// Shared pointer for the message queue for this connection.
    std::shared_ptr<MessageQueue<libcomp::Message::Message*>> mMessageQueue;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_ENCRYPTEDCONNECTION_H
