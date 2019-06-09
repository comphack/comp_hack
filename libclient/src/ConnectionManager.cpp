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
#include <Decrypt.h>
#include <EnumUtils.h>
#include <ErrorCodes.h>
#include <LobbyConnection.h>
#include <Log.h>
#include <LogicWorker.h>
#include <MessageClient.h>
#include <MessageConnectionClosed.h>
#include <MessageEncrypted.h>
#include <PacketCodes.h>

// packets Includes
#include <PacketLobbyAuth.h>
#include <PacketLobbyAuthReply.h>
#include <PacketLobbyLogin.h>
#include <PacketLobbyLoginReply.h>

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
    libcomp::Manager(), mLogicWorker(pLogicWorker), mMessageQueue(messageQueue),
    mClientVersion(1666)
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
        case to_underlying(MessageType::MESSAGE_TYPE_PACKET):
            return ProcessPacketMessage(
                (const libcomp::Message::Packet*)pMessage);
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

bool ConnectionManager::ProcessPacketMessage(
        const libcomp::Message::Packet *pMessage)
{
    libcomp::ReadOnlyPacket p(pMessage->GetPacket());

    switch(pMessage->GetCommandCode())
    {
        case to_underlying(LobbyToClientPacketCode_t::PACKET_LOGIN):
            return HandlePacketLobbyLogin(p);
        case to_underlying(LobbyToClientPacketCode_t::PACKET_AUTH):
            return HandlePacketLobbyAuth(p);
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
                    AuthenticateLobby();
                }
                else
                {
                    AuthenticateChannel();
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
            const MessageConnectToLobby *pInfo = reinterpret_cast<
                const MessageConnectToLobby*>(pMessage);
            mUsername = pInfo->GetUsername();
            mPassword = pInfo->GetPassword();
            mClientVersion = pInfo->GetClientVersion();

            /// @todo Handle if this fails.
            if(!ConnectLobby(pInfo->GetConnectionID(), pInfo->GetHost(),
                pInfo->GetPort()))
            {
                LOG_ERROR("Failed to connect to lobby server!\n");
            }

            return true;
        }
        case to_underlying(MessageClientType::CONNECT_TO_CHANNEL):
        {
            const MessageConnectToChannel *pInfo = reinterpret_cast<
                const MessageConnectToChannel*>(pMessage);

            /// @todo Handle if this fails.
            if(!ConnectChannel(pInfo->GetConnectionID(), pInfo->GetHost(),
                pInfo->GetPort()))
            {
                LOG_ERROR("Failed to connect to channel server!\n");
            }

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

        // Restart so the service may be used again.
        mService.restart();
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

    LOG_DEBUG(libcomp::String("Connecting to %1:%2\n").Arg(host).Arg(port));

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

void ConnectionManager::AuthenticateLobby()
{
    // Send the login packet and await the response.
    packets::PacketLobbyLogin p;
    p.SetPacketCode(to_underlying(ClientToLobbyPacketCode_t::PACKET_LOGIN));
    p.SetUsername(mUsername);
    p.SetClientVersion(mClientVersion);
    p.SetUnknown(0);

    mActiveConnection->SendObject(p);
}

void ConnectionManager::AuthenticateChannel()
{
    // mLogicWorker->SendToGame(new MessageConnectedToChannel(
    //     mActiveConnection->GetName()));
}

bool ConnectionManager::HandlePacketLobbyLogin(libcomp::ReadOnlyPacket& p)
{
    packets::PacketLobbyLoginReply obj;

    ErrorCodes_t errorCode = ErrorCodes_t::SUCCESS;

    if(sizeof(errorCode) == p.Size())
    {
        errorCode = (ErrorCodes_t)p.ReadS32Little();

        // If this happens the packet is wrong.
        if(ErrorCodes_t::SUCCESS == errorCode)
        {
            return false;
        }
    }
    else if(!obj.LoadPacket(p) || p.Left())
    {
        return false;
    }

    if(ErrorCodes_t::SUCCESS == errorCode)
    {
        auto hash = libcomp::Decrypt::HashPassword(
            libcomp::Decrypt::HashPassword(mPassword, obj.GetSalt()),
            libcomp::String("%1").Arg(obj.GetChallenge()));

        // Send the auth packet and await the response.
        packets::PacketLobbyAuth reply;
        reply.SetPacketCode(to_underlying(
            ClientToLobbyPacketCode_t::PACKET_AUTH));
        reply.SetHash(hash);

        mActiveConnection->SendObject(reply);
    }
    else
    {
        // Save this before closing the connection.
        auto connectionID = mActiveConnection->GetName();

        // An error occurred so close the connection and pass it along.
        CloseConnection();
        mLogicWorker->SendToGame(new MessageConnectedToLobby(
            connectionID, errorCode));
    }

    return true;
}

bool ConnectionManager::HandlePacketLobbyAuth(libcomp::ReadOnlyPacket& p)
{
    packets::PacketLobbyAuthReply obj;

    ErrorCodes_t errorCode = ErrorCodes_t::SUCCESS;

    if(sizeof(errorCode) == p.Size())
    {
        errorCode = (ErrorCodes_t)p.ReadS32Little();

        // If this happens the packet is wrong.
        if(ErrorCodes_t::SUCCESS == errorCode)
        {
            return false;
        }
    }
    else if(!obj.LoadPacket(p) || p.Left())
    {
        return false;
    }

    if(ErrorCodes_t::SUCCESS == errorCode)
    {
        mLogicWorker->SendToGame(new MessageConnectedToLobby(
            mActiveConnection->GetName(), errorCode, obj.GetSID()));
    }
    else
    {
        // Save this before closing the connection.
        auto connectionID = mActiveConnection->GetName();

        // An error occurred so close the connection and pass it along.
        CloseConnection();
        mLogicWorker->SendToGame(new MessageConnectedToLobby(
            connectionID, errorCode));
    }

    return true;
}
