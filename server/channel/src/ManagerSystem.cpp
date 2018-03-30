/**
 * @file server/channel/src/ManagerSystem.cpp
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

#include "ManagerSystem.h"

// libcomp Includes
#include <Log.h>
#include <MessageTick.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

std::list<libcomp::Message::MessageType> ManagerSystem::sSupportedTypes =
    { libcomp::Message::MessageType::MESSAGE_TYPE_SYSTEM };

ManagerSystem::ManagerSystem(std::weak_ptr<libcomp::BaseServer> server)
    : mServer(server)
{
}

ManagerSystem::~ManagerSystem()
{
}

std::list<libcomp::Message::MessageType>
ManagerSystem::GetSupportedTypes() const
{
    return sSupportedTypes;
}

bool ManagerSystem::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    const libcomp::Message::Tick *tick = dynamic_cast<
        const libcomp::Message::Tick*>(pMessage);

    if(nullptr != tick)
    {
        auto server = std::dynamic_pointer_cast<ChannelServer>(
            mServer.lock());

        server->Tick();

        return true;
    }

    return false;
}
