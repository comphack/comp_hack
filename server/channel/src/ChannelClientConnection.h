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

class ChannelClientConnection : public libcomp::ChannelConnection
{
public:
    ChannelClientConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);
    virtual ~ChannelClientConnection();

    ClientState* GetClientState() const;
    void SetClientState(const std::shared_ptr<ClientState>& state);

private:
    std::shared_ptr<ClientState> mClientState;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHANNELCLIENTCONNECTION_H
