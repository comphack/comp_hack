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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "ChannelServer.h"

using namespace channel;

ChannelClientConnection::ChannelClientConnection(asio::ip::tcp::socket& socket,
    const std::shared_ptr<libcomp::Crypto::DiffieHellman>& diffieHellman) :
    ChannelConnection(socket, diffieHellman), mClientState(
        std::shared_ptr<ClientState>(new ClientState)), mTimeout(0)
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

void ChannelClientConnection::Kill()
{
    mClientState->SetLogoutSave(false);
    Close();
}

void ChannelClientConnection::BroadcastPacket(const std::list<std::shared_ptr<
    ChannelClientConnection>>& clients, libcomp::Packet& packet, bool queue)
{
    if(queue)
    {
        for(auto client : clients)
        {
            client->QueuePacketCopy(packet);
        }
    }
    else
    {
        std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
        for(auto client : clients)
        {
            connections.push_back(client);
        }

        libcomp::TcpConnection::BroadcastPacket(connections, packet);
    }
}

void ChannelClientConnection::BroadcastPackets(const std::list<std::shared_ptr<
    ChannelClientConnection>>& clients, std::list<libcomp::Packet>& packets)
{
    for(auto client : clients)
    {
        for(auto& packet : packets)
        {
            client->QueuePacketCopy(packet);
        }

        client->FlushOutgoing();
    }
}

void ChannelClientConnection::FlushAllOutgoing(const std::list<std::shared_ptr<
    ChannelClientConnection>>& clients)
{
    for(auto client : clients)
    {
        client->FlushOutgoing();
    }
}

void ChannelClientConnection::SendRelativeTimePacket(
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    libcomp::Packet& packet, const RelativeTimeMap& timeMap,
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
