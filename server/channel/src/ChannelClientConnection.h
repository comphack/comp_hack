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

#ifndef SERVER_CHANNEL_SRC_CHANNELCLIENTCONNECTION_H
#define SERVER_CHANNEL_SRC_CHANNELCLIENTCONNECTION_H

// channel Includes
#include "ClientState.h"

// libcomp Includes
#include <ChannelConnection.h>

namespace channel
{

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
     */
    void RefreshTimeout(uint64_t now);

    /**
     * Get the next client timeout timestamp.
     * @return Server time representation of the next timeout timestamp
     */
    uint64_t GetTimeout() const;

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
