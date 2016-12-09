/**
 * @file server/world/src/ManagerConnection.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Manager to handle world connections to lobby and channel servers.
 *
 * This file is part of the World Server (world).
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

#include "ManagerConnection.h"

// libcomp Includes
#include "Log.h"
#include "MessageConnectionClosed.h"
#include "MessageEncrypted.h"

// world Includes
#include "WorldServer.h"

using namespace world;

ManagerConnection::ManagerConnection(std::shared_ptr<libcomp::BaseServer> server)
    : mServer(server)
{
}

ManagerConnection::~ManagerConnection()
{
}

std::list<libcomp::Message::MessageType>
ManagerConnection::GetSupportedTypes() const
{
    std::list<libcomp::Message::MessageType> supportedTypes;

    supportedTypes.push_back(
        libcomp::Message::MessageType::MESSAGE_TYPE_CONNECTION);

    return supportedTypes;
}

bool ManagerConnection::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    const libcomp::Message::Encrypted *encrypted = dynamic_cast<
        const libcomp::Message::Encrypted*>(pMessage);

    if(nullptr != encrypted)
    {
        if(!LobbyConnected())
        {
            auto connection = encrypted->GetConnection();

            //todo: verify this is in fact the lobby

            mLobbyConnection = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);
        }
        else
        {
            //Nothing to do upon encrypting a channel connection (for now)
        }
        
        return true;
    }

    const libcomp::Message::ConnectionClosed *closed = dynamic_cast<
        const libcomp::Message::ConnectionClosed*>(pMessage);

    if(nullptr != closed)
    {
        auto connection = closed->GetConnection();

        mServer->RemoveConnection(connection);

        if(mLobbyConnection == connection)
        {
            LOG_INFO(libcomp::String("Lobby connection closed. Shutting down."));
            mServer->Shutdown();
        }
        else
        {
            objects::ChannelDescription channelDesc;
            auto server = std::dynamic_pointer_cast<WorldServer>(mServer);
            auto iConnection = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);
            if(server->GetChannelDescriptionByConnection(iConnection, channelDesc))
            {
                server->RemoveChannelDescription(iConnection);

                //Channel disconnected
                libcomp::Packet packet;
                packet.WriteU16Little(0x1002);
                packet.WriteU8(0);  //0: Remove
                channelDesc.SavePacket(packet);
                mLobbyConnection->SendPacket(packet);
            }
        }

        return true;
    }
    
    return false;
}

std::shared_ptr<libcomp::InternalConnection> ManagerConnection::GetLobbyConnection()
{
    return mLobbyConnection;
}

bool ManagerConnection::LobbyConnected()
{
    return nullptr != mLobbyConnection;
}