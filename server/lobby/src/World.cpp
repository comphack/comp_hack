/**
 * @file server/lobby/src/World.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief World definition in regards to the lobby containing an active connection to the world server.
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

#include "World.h"

// libcomp includes
#include <Packet.h>
#include <PacketCodes.h>

using namespace lobby;

World::World()
{
}

World::~World()
{
}

std::shared_ptr<libcomp::InternalConnection> World::GetConnection() const
{
    return mConnection;
}

void World::SetConnection(const std::shared_ptr<libcomp::InternalConnection>& connection)
{
    mConnection = connection;
}

const std::list<std::shared_ptr<objects::RegisteredServer>> World::GetChannels() const
{
    return mChannels;
}

std::shared_ptr<objects::RegisteredServer> World::GetChannelByID(uint8_t id) const
{
    for(auto channel : mChannels)
    {
        if(channel->GetID() == id)
        {
            return channel;
        }
    }

    return nullptr;
}

bool World::RemoveChannelByID(uint8_t id)
{
    for(auto iter = mChannels.begin(); iter != mChannels.end(); iter++)
    {
        if((*iter)->GetID() == id)
        {
            mChannels.erase(iter);
            return true;
        }
    }

    return false;
}

std::shared_ptr<libcomp::Database> World::GetWorldDatabase() const
{
    return mDatabase;
}

void World::SetWorldDatabase(const std::shared_ptr<libcomp::Database>& database)
{
    mDatabase = database;
}

void World::RegisterChannel(const std::shared_ptr<
    objects::RegisteredServer>& channel)
{
    auto svr = GetChannelByID(channel->GetID());
    if(nullptr == svr)
    {
        mChannels.push_back(channel);
    }
}

const std::shared_ptr<objects::RegisteredServer> World::GetRegisteredServer() const
{
    return mRegisteredServer;
}

void World::SetRegisteredServer(const std::shared_ptr<objects::RegisteredServer>& registeredServer)
{
    mRegisteredServer = registeredServer;
}
