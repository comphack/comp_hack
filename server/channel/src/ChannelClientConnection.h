/**
 * @file server/channel/src/ChannelClientConnection.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Channel client connection class.
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_CHANNELCLIENTCONNECTION_H
#define SERVER_CHANNEL_SRC_CHANNELCLIENTCONNECTION_H

// channel Includes
#include "ClientState.h"

// libcomp Includes
#include <ChannelConnection.h>

namespace channel
{

typedef std::unordered_map<uint32_t, uint64_t> RelativeTimeMap;

/**
 * Represents a connection to the game client.
 */
class ChannelClientConnection : public libcomp::ChannelConnection
{
public:
    /**
     * Create a new connection.
     * @param socket Socket provided by the server for the new client.
     * @param pDiffieHellman Asymmetric encryption information.
     */
    ChannelClientConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);

    /**
     * Clean up the connection.
     */
    virtual ~ChannelClientConnection();

    /**
     * Get the state of the client.
     * @return Pointer to the ClientState
     */
    ClientState* GetClientState() const;

    /**
     * Refresh the client timeout.
     * @param now Current server time
     * @param aliveUntil Time in seconds that needs to pass before the
     *  refresh time is no longer valid and the timeout countdown starts
     */
    void RefreshTimeout(uint64_t now, uint16_t aliveUntil);

    /**
     * Get the next client timeout timestamp.
     * @return Server time representation of the next timeout timestamp
     */
    uint64_t GetTimeout() const;

    /**
     * Close the connection after marking it to not save any logout data.
     * This is useful for if the client gets into a state that is too complex
     * to recover from to avoid data corruption.
     */
    void Kill();

    /**
     * Broadcast the supplied packet to each client connection in the list.
     * @param clients List of client connections to send the packet to
     * @param packet Packet to send to the supplied clients
     * @param queue Optional parameter to queue packets for the supplied connections
     *  instead of sending immediately
     */
    static void BroadcastPacket(const std::list<std::shared_ptr<
        ChannelClientConnection>>& clients, libcomp::Packet& packet,
        bool queue = false);

    /**
     * Broadcast the supplied list of packets to each client connection in the list.
     * @param clients List of client connections to send the packet to
     * @param packets Packets to send to the supplied clients
     */
    static void BroadcastPackets(const std::list<std::shared_ptr<
        ChannelClientConnection>>& clients, std::list<libcomp::Packet>& packets);

    /**
     * Flush all client connection outgoing packets.
     * @param clients List of client connections to flush
     */
    static void FlushAllOutgoing(const std::list<std::shared_ptr<
        ChannelClientConnection>>& clients);

    /**
     * Send (or queue) a packet to a list of client connections. Server
     * tick times are converted to relative client times before sending.
     * @param clients List of client connections to send the packet to
     * @param packet Packet to send to the supplied clients
     * @param timeMap Map of packet positions to server times to transform
     * @param queue Optional parameter to queue packets for the supplied connections
     *  instead of sending immediately
     */
    static void SendRelativeTimePacket(
        const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
        libcomp::Packet& p, const RelativeTimeMap& timeMap,
        bool queue = false);

private:
    /// State of the client
    std::shared_ptr<ClientState> mClientState;

    /// Server timestamp used to disconnect the client should it pass
    /// without refreshing beforehand.
    uint64_t mTimeout;
};

static inline ClientState* state(
    const std::shared_ptr<libcomp::TcpConnection>& connection)
{
    ClientState *pState = nullptr;

    auto conn = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    if(nullptr != conn)
    {
        pState = conn->GetClientState();
    }

    return pState;
}

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHANNELCLIENTCONNECTION_H
