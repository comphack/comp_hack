/**
 * @file server/channel/src/ManagerConnection.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manager to handle channel connections to the world server.
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

#include "ManagerConnection.h"

// libcomp Includes
#include <Log.h>
#include <MessageConnectionClosed.h>
#include <MessageEncrypted.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <ChannelConfig.h>
#include <Party.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "ClientState.h"

using namespace channel;

std::list<libcomp::Message::MessageType> ManagerConnection::sSupportedTypes =
    { libcomp::Message::MessageType::MESSAGE_TYPE_CONNECTION };

ManagerConnection::ManagerConnection(std::weak_ptr<libcomp::BaseServer> server)
    : mServer(server)
{
}

ManagerConnection::~ManagerConnection()
{
}

std::list<libcomp::Message::MessageType>
ManagerConnection::GetSupportedTypes() const
{
    return sSupportedTypes;
}

bool ManagerConnection::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    const libcomp::Message::ConnectionMessage *cMessage = dynamic_cast<
        const libcomp::Message::ConnectionMessage*>(pMessage);
    
    switch(cMessage->GetConnectionMessageType())
    {
        case libcomp::Message::ConnectionMessageType::CONNECTION_MESSAGE_ENCRYPTED:
            {
                const libcomp::Message::Encrypted *encrypted = dynamic_cast<
                    const libcomp::Message::Encrypted*>(cMessage);

                auto connection = encrypted->GetConnection();

                if(mWorldConnection == connection)
                {
                    RequestWorldInfo();
                }
        
                return true;
            }
            break;
        case libcomp::Message::ConnectionMessageType::CONNECTION_MESSAGE_CONNECTION_CLOSED:
            {
                const libcomp::Message::ConnectionClosed *closed = dynamic_cast<
                    const libcomp::Message::ConnectionClosed*>(cMessage);

                auto connection = closed->GetConnection();

                auto server = mServer.lock();
                server->RemoveConnection(connection);

                auto clientConnection = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
                RemoveClientConnection(clientConnection);

                if(mWorldConnection == connection)
                {
                    LOG_INFO("World connection closed. Shutting down.\n");
                    server->Shutdown();
                }

                return true;
            }
            break;
        default:
            break;
    }
    
    return false;
}

void ManagerConnection::RequestWorldInfo()
{
    if(nullptr != mWorldConnection)
    {
        //Request world information
        libcomp::Packet packet;
        packet.WritePacketCode(InternalPacketCode_t::PACKET_GET_WORLD_INFO);

        mWorldConnection->SendPacket(packet);
    }
}

const std::shared_ptr<libcomp::InternalConnection> ManagerConnection::GetWorldConnection() const
{
    return mWorldConnection;
}

void ManagerConnection::SetWorldConnection(const std::shared_ptr<libcomp::InternalConnection>& worldConnection)
{
    mWorldConnection = worldConnection;
}

const std::shared_ptr<ChannelClientConnection> ManagerConnection::GetClientConnection(
    const libcomp::String& username)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mClientConnections.find(username);
    return iter != mClientConnections.end() ? iter->second : nullptr;
}

void ManagerConnection::SetClientConnection(const std::shared_ptr<
    ChannelClientConnection>& connection)
{
    auto state = connection->GetClientState();
    auto login = state->GetAccountLogin();
    auto account = login->GetAccount().GetCurrentReference();
    if(nullptr == account)
    {
        return;
    }

    auto username = account->GetUsername();

    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mClientConnections.find(username);
    if(iter == mClientConnections.end())
    {
        mClientConnections[username] = connection;
    }
}

void ManagerConnection::RemoveClientConnection(const std::shared_ptr<
    ChannelClientConnection>& connection)
{
    if(nullptr == connection)
    {
        return;
    }

    auto state = connection->GetClientState();
    auto login = state->GetAccountLogin();
    auto account = login->GetAccount().GetCurrentReference();
    if(nullptr == account)
    {
        return;
    }

    auto username = account->GetUsername();
    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mClientConnections.find(username);
        if(iter != mClientConnections.end())
        {
            mClientConnections.erase(iter);
            removed = true;
        }
    }

    if(removed)
    {
        // Inform the world that the connection has closed, whether they've
        // been logged in successfully or not yet
        libcomp::Packet p;
        p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
        p.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);
        p.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, username);
        GetWorldConnection()->SendPacket(p);

        auto server = std::dynamic_pointer_cast<ChannelServer>(
            mServer.lock());
        auto accountManager = server->GetAccountManager();
        accountManager->Logout(connection);
    }
}

const std::shared_ptr<ChannelClientConnection>
    ManagerConnection::GetEntityClient(int32_t id, bool worldID)
{
    auto state = ClientState::GetEntityClientState(id, worldID);
    auto cState = state != nullptr ? state->GetCharacterState() : nullptr;
    return cState && cState->GetEntity() != nullptr
        ? GetClientConnection(cState->GetEntity()->GetAccount()->GetUsername())
        : nullptr;
}

std::list<std::shared_ptr<ChannelClientConnection>>
    ManagerConnection::GatherWorldTargetClients(libcomp::ReadOnlyPacket& p, bool& success)
{
    std::list<std::shared_ptr<ChannelClientConnection>> results;
    success = false;

    if(p.Left() < 2 || p.Left() < (2 + (uint32_t)(p.PeekU16Little() * 4)))
    {
        LOG_ERROR("Invalid CID count received for world target entity list.\n");
        return results;
    }

    uint16_t cidCount = p.ReadU16Little();

    std::list<int32_t> cids;
    for(uint16_t i = 0; i < cidCount; i++)
    {
        cids.push_back(p.ReadS32Little());
    }

    for(int32_t cid : cids)
    {
        auto client = GetEntityClient(cid, true);
        if(client)
        {
            results.push_back(client);
        }
    }

    success = true;
    return results;
}

std::list<std::shared_ptr<ChannelClientConnection>>
    ManagerConnection::GetPartyConnections(const std::shared_ptr<
    ChannelClientConnection>& client, bool includeSelf, bool zoneRestrict)
{
    std::list<std::shared_ptr<ChannelClientConnection>> result;

    auto state = client->GetClientState();
    if(includeSelf)
    {
        result.push_back(client);
    }

    auto party = state->GetParty();
    if(party)
    {
        auto sourceZone = state->GetZone();
        for(auto memberID : party->GetMemberIDs())
        {
            auto memberClient = GetEntityClient(memberID, true);
            if(memberClient && memberClient != client)
            {
                auto memberState = memberClient->GetClientState();
                if(!zoneRestrict || memberState->GetZone() == sourceZone)
                {
                    result.push_back(memberClient);
                }
            }
        }
    }

    return result;
}

void ManagerConnection::BroadcastPacketToClients(libcomp::Packet& packet)
{
    std::list<std::shared_ptr<ChannelClientConnection>> clients;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto cPair : mClientConnections)
        {
            clients.push_back(cPair.second);
        }
    }

    ChannelClientConnection::BroadcastPacket(clients, packet);
}

bool ManagerConnection::ScheduleClientTimeoutHandler(uint16_t timeout)
{
    auto server = std::dynamic_pointer_cast<ChannelServer>(mServer.lock());

    // Check every 10 seconds
    ServerTime nextTime = server->GetServerTime() + 10000000ULL;
    return server->ScheduleWork(nextTime, [](ChannelServer* svr, uint16_t t)
        {
            uint64_t now = svr->GetServerTime();
            auto manager = svr->GetManagerConnection();

            manager->HandleClientTimeouts(now, t);
            manager->ScheduleClientTimeoutHandler(t);
        }, server.get(), timeout);
}

void ManagerConnection::HandleClientTimeouts(uint64_t now, uint16_t timeout)
{
    std::list<libcomp::String> timeOuts;
    {
        ServerTime expireBefore = now - (ServerTime)(timeout * 1000000ULL);

        std::lock_guard<std::mutex> lock(mLock);
        for(auto it = mClientConnections.begin();
            it != mClientConnections.end(); it++)
        {
            auto clientTimeout = it->second->GetTimeout();
            if(clientTimeout && clientTimeout <= expireBefore)
            {
                timeOuts.push_back(it->first);

                // Stop the timeout from throwing multiple times
                it->second->RefreshTimeout(0, 0);
            }
        }
    }

    if(timeOuts.size() > 0)
    {
        for(auto timedOut : timeOuts)
        {
            LOG_ERROR(libcomp::String(
                "Client connection timed out: %1\n").Arg(timedOut));

            libcomp::Packet p;
            p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
            p.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);
            p.WriteString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, timedOut);
            p.WriteU8(1);
            mWorldConnection->QueuePacket(p);
        }
        mWorldConnection->FlushOutgoing();
    }
}
