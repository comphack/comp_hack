/**
 * @file libcomp/src/ConnectionManager.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manages the active client connection to the server.
 *
 * This file is part of the COMP_hack Client Library (libclient).
 *
 * Copyright (C) 2012-2019 COMP_hack Team <compomega@tutanota.com>
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

#include "ConnectionManager.h"

// libcomp Includes
#include <ChannelConnection.h>
#include <ConnectionMessage.h>
#include <EnumUtils.h>
#include <LobbyConnection.h>
#include <LogicWorker.h>
#include <MessageClient.h>
#include <MessageConnectionClosed.h>
#include <MessageEncrypted.h>

// logic Messages
#include "MessageConnected.h"
#include "MessageConnectionInfo.h"

using namespace logic;

using libcomp::Message::MessageType;
using libcomp::Message::ConnectionMessageType;
using libcomp::Message::MessageClientType;

ConnectionManager::ConnectionManager(LogicWorker *pLogicWorker,
    const std::weak_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>>& messageQueue) :
    libcomp::Manager(), mLogicWorker(pLogicWorker), mMessageQueue(messageQueue)
{
}

ConnectionManager::~ConnectionManager()
{
    mService.stop();

    if(mServiceThread.joinable())
    {
        mServiceThread.join();
    }
}

std::list<libcomp::Message::MessageType>
    ConnectionManager::GetSupportedTypes() const
{
    return {
        MessageType::MESSAGE_TYPE_PACKET,
        MessageType::MESSAGE_TYPE_CONNECTION,
        MessageType::MESSAGE_TYPE_CLIENT,
    };
}

bool ConnectionManager::ProcessMessage(
    const libcomp::Message::Message *pMessage)
{
    switch(to_underlying(pMessage->GetType()))
    {
        case to_underlying(MessageType::MESSAGE_TYPE_CONNECTION):
            return ProcessConnectionMessage(
                (const libcomp::Message::ConnectionMessage*)pMessage);
        case to_underlying(MessageType::MESSAGE_TYPE_CLIENT):
            return ProcessClientMessage(
                (const libcomp::Message::MessageClient*)pMessage);
        default:
            break;
    }

    return false;
}

bool ConnectionManager::ProcessConnectionMessage(
    const libcomp::Message::ConnectionMessage *pMessage)
{
    switch(to_underlying(pMessage->GetConnectionMessageType()))
    {
        case to_underlying(ConnectionMessageType::CONNECTION_MESSAGE_ENCRYPTED):
        {
            const libcomp::Message::Encrypted *pMsg = reinterpret_cast<
                const libcomp::Message::Encrypted*>(pMessage);

            if(pMsg->GetConnection() == mActiveConnection)
            {
                if(IsLobbyConnection())
                {
                    mLogicWorker->SendToGame(new MessageConnectedToLobby(
                        mActiveConnection->GetName()));
                }
                else
                {
                    mLogicWorker->SendToGame(new MessageConnectedToChannel(
                        mActiveConnection->GetName()));
                }
            }

            return true;
        }
        case to_underlying(ConnectionMessageType::CONNECTION_MESSAGE_CONNECTION_CLOSED):
        {
            const libcomp::Message::ConnectionClosed *pMsg = reinterpret_cast<
                const libcomp::Message::ConnectionClosed*>(pMessage);

            if(pMsg->GetConnection() == mActiveConnection)
            {
                /// @todo Send an event
            }

            return true;
        }
        default:
            break;
    }

    return false;
}

bool ConnectionManager::ProcessClientMessage(
    const libcomp::Message::MessageClient *pMessage)
{
    switch(to_underlying(pMessage->GetMessageClientType()))
    {
        case to_underlying(MessageClientType::CONNECT_TO_LOBBY):
        {
            const MessageConnectionInfo *pInfo = reinterpret_cast<
                const MessageConnectionInfo*>(pMessage);
            ConnectLobby(pInfo->GetConnectionID(), pInfo->GetHost(),
                pInfo->GetPort());

            return true;
        }
        case to_underlying(MessageClientType::CONNECT_TO_CHANNEL):
        {
            const MessageConnectionInfo *pInfo = reinterpret_cast<
                const MessageConnectionInfo*>(pMessage);
            ConnectChannel(pInfo->GetConnectionID(), pInfo->GetHost(),
                pInfo->GetPort());

            return true;
        }
        default:
            break;
    }

    return false;
}

bool ConnectionManager::ConnectLobby(const libcomp::String& connectionID,
    const libcomp::String& host, uint16_t port)
{
    return SetupConnection(std::make_shared<libcomp::LobbyConnection>(
        mService), connectionID, host, port);
}

bool ConnectionManager::ConnectChannel(const libcomp::String& connectionID,
    const libcomp::String& host, uint16_t port)
{
    return SetupConnection(std::make_shared<libcomp::ChannelConnection>(
        mService), connectionID, host, port);
}

bool ConnectionManager::CloseConnection()
{
    // Close an existing active connection.
    if(mActiveConnection)
    {
        if(!mActiveConnection->Close())
        {
            return false;
        }

        // Free the connection.
        mActiveConnection.reset();

        // Stop the service.
        mService.stop();

        // Now join the service thread.
        if(mServiceThread.joinable())
        {
            mServiceThread.join();
        }
    }

    return true;
}

bool ConnectionManager::SetupConnection(const std::shared_ptr<
    libcomp::EncryptedConnection>& conn, const libcomp::String& connectionID,
    const libcomp::String& host, uint16_t port)
{
    if(!CloseConnection())
    {
        return false;
    }

    mActiveConnection = conn;
    mActiveConnection->SetMessageQueue(mMessageQueue);
    mActiveConnection->SetName(connectionID);

    bool result = mActiveConnection->Connect(host, port);

    // Start the service thread for the new connection.
    mServiceThread = std::thread([&]()
    {
        mService.run();
    });

    return result;
}

void ConnectionManager::SendPacket(libcomp::Packet& packet)
{
    if(mActiveConnection)
    {
        mActiveConnection->SendPacket(packet);
    }
}

void ConnectionManager::SendPacket(libcomp::ReadOnlyPacket& packet)
{
    if(mActiveConnection)
    {
        mActiveConnection->SendPacket(packet);
    }
}

void ConnectionManager::SendPackets(
    const std::list<libcomp::Packet*>& packets)
{
    if(mActiveConnection)
    {
        for(auto packet : packets)
        {
            mActiveConnection->QueuePacket(*packet);
        }

        mActiveConnection->FlushOutgoing();
    }
}

void ConnectionManager::SendPackets(
    const std::list<libcomp::ReadOnlyPacket*>& packets)
{
    if(mActiveConnection)
    {
        for(auto packet : packets)
        {
            mActiveConnection->QueuePacket(*packet);
        }

        mActiveConnection->FlushOutgoing();
    }
}

bool ConnectionManager::SendObject(std::shared_ptr<libcomp::Object>& obj)
{
    if(mActiveConnection)
    {
        return mActiveConnection->SendObject(*obj.get());
    }

    return false;
}

bool ConnectionManager::SendObjects(const std::list<
    std::shared_ptr<libcomp::Object>>& objs)
{
    if(mActiveConnection)
    {
        for(auto obj : objs)
        {
            if(!mActiveConnection->QueueObject(*obj.get()))
            {
                return false;
            }
        }

        mActiveConnection->FlushOutgoing();
    }

    return false;
}

bool ConnectionManager::IsConnected() const
{
    if(mActiveConnection)
    {
        return libcomp::TcpConnection::STATUS_ENCRYPTED ==
            mActiveConnection->GetStatus();
    }

    return false;
}

bool ConnectionManager::IsLobbyConnection() const
{
    return nullptr != std::dynamic_pointer_cast<libcomp::LobbyConnection>(
        mActiveConnection).get();
}

bool ConnectionManager::IsChannelConnection() const
{
    return nullptr != std::dynamic_pointer_cast<libcomp::ChannelConnection>(
        mActiveConnection).get();
}

std::shared_ptr<libcomp::EncryptedConnection>
    ConnectionManager::GetConnection() const
{
    return mActiveConnection;
}
