/**
 * @file server/channel/src/ChannelClientConnection.cpp
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

#include "ChannelClientConnection.h"

using namespace channel;

ChannelClientConnection::ChannelClientConnection(asio::ip::tcp::socket& socket,
    DH *pDiffieHellman) : ChannelConnection(socket, pDiffieHellman)
{
}

ChannelClientConnection::~ChannelClientConnection()
{
}

ClientState* ChannelClientConnection::GetClientState() const
{
    return mClientState.get();
}

void ChannelClientConnection::SetClientState(
    const std::shared_ptr<ClientState>& state)
{
    mClientState = state;
}
