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

// channel Includes
#include "ChannelServer.h"

using namespace channel;

ManagerConnection::ManagerConnection(std::shared_ptr<libcomp::BaseServer> server)
{
    mServer = server;
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
    const libcomp::Message::ConnectionMessage *cMessage = dynamic_cast<
        const libcomp::Message::ConnectionMessage*>(pMessage);
    
    switch(cMessage->GetConnectionMessageType())
    {
        case libcomp::Message::ConnectionMessageType::CONNECTION_MESSAGE_ENCRYPTED:
            {
                const libcomp::Message::Encrypted *encrypted = dynamic_cast<
                    const libcomp::Message::Encrypted*>(cMessage);

                auto connection = encrypted->GetConnection();
                auto server = std::dynamic_pointer_cast<ChannelServer>(mServer);

                if(mWorldConnection == connection)
                {
                    //Request world information
                    libcomp::Packet packet;
                    packet.WriteU16Little(0x1001);

                    connection->SendPacket(std::move(packet));
                }
        
                return true;
            }
            break;
        case libcomp::Message::ConnectionMessageType::CONNECTION_MESSAGE_CONNECTION_CLOSED:
            {
                const libcomp::Message::ConnectionClosed *closed = dynamic_cast<
                    const libcomp::Message::ConnectionClosed*>(cMessage);

                auto connection = closed->GetConnection();

                mServer->RemoveConnection(connection);

                if(mWorldConnection == connection)
                {
                    LOG_INFO(libcomp::String("World connection closed. Shutting down."));
                    mServer->Shutdown();
                }

                return true;
            }
            break;
    }
    
    return false;
}

void ManagerConnection::SetWorldConnection(std::shared_ptr<libcomp::InternalConnection> worldConnection)
{
    mWorldConnection = worldConnection;
}