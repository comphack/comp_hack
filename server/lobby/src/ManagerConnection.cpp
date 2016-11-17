/**
 * @file server/lobby/src/ManagerConnection.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Manager to handle lobby connections to world servers.
 *
 * This file is part of the Lobby Server (lobby).
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

// lobby Includes
#include "MessageWorldNotification.h"

using namespace lobby;

ManagerConnection::ManagerConnection(std::shared_ptr<asio::io_service> service,
    std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>> messageQueue)
{
    mService = service;
    mMessageQueue = messageQueue;
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
    // Only one type of message passes through here (for the time being) so no need for dedicated handlers
    const libcomp::Message::WorldNotification *notification = dynamic_cast<
        const libcomp::Message::WorldNotification*>(pMessage);

    if(nullptr != notification)
    {
        uint16_t port = notification->GetPort();
        libcomp::String address = notification->GetAddress();

        LOG_DEBUG(libcomp::String("Attempting to connect back to World: %1:%2\n").Arg(address).Arg(port));

        std::shared_ptr<libcomp::InternalConnection> worldConnection(
            new libcomp::InternalConnection(*mService));

        worldConnection->SetSelf(worldConnection);
        worldConnection->SetMessageQueue(mMessageQueue);

        // Connect and stay connected until either of us shutdown
        if(worldConnection->Connect(address, port, false))
        {
            auto world = std::shared_ptr<lobby::World>(new lobby::World(worldConnection));

            if (world->Initialize())
            {
                LOG_INFO(libcomp::String("New World connection established: %1:%2\n").Arg(address).Arg(port));
                mWorlds.push_back(world);
            }

            return true;
        }
        
        LOG_ERROR(libcomp::String("World connection failed: %1:%2\n").Arg(address).Arg(port));
    }
    
    return false;
}
