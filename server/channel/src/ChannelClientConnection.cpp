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
    DH *pDiffieHellman) : ChannelConnection(socket, pDiffieHellman),
    mClientState(std::shared_ptr<ClientState>(new ClientState)), mTimeout(0)
{
}

ChannelClientConnection::~ChannelClientConnection()
{
}

ClientState* ChannelClientConnection::GetClientState() const
{
    return mClientState.get();
}

void ChannelClientConnection::RefreshTimeout(uint64_t now, uint16_t aliveUntil)
{
    mTimeout = now + (uint64_t)(aliveUntil * 1000000);
}

uint64_t ChannelClientConnection::GetTimeout() const
{
    return mTimeout;
}

void ChannelClientConnection::BroadcastPacket(const std::list<std::shared_ptr<
    ChannelClientConnection>>& clients, libcomp::Packet& packet)
{
    std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
    for(auto client : clients)
    {
        connections.push_back(client);
    }

    libcomp::TcpConnection::BroadcastPacket(connections, packet);
}

void ChannelClientConnection::BroadcastPackets(const std::list<std::shared_ptr<
    ChannelClientConnection>> &clients, std::list<libcomp::Packet>& packets)
{
    for(auto client : clients)
    {
        for(auto& packet : packets)
        {
            client->QueuePacket(packet);
        }

        client->FlushOutgoing();
    }
}

void ChannelClientConnection::SendRelativeTimePacket(
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    libcomp::Packet& packet, const std::unordered_map<uint32_t, uint64_t>& timeMap,
    bool queue)
{
    for(auto client : clients)
    {
        libcomp::Packet pCopy(packet);

        auto state = client->GetClientState();
        for(auto tPair : timeMap)
        {
            pCopy.Seek(tPair.first);
            pCopy.WriteFloat(state->ToClientTime(tPair.second));
        }

        if(queue)
        {
            client->QueuePacket(pCopy);
        }
        else
        {
            client->SendPacket(pCopy);
        }
    }
}
