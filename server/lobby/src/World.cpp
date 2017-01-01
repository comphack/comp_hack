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

World::World(std::shared_ptr<libcomp::InternalConnection> connection)
    : mConnection(connection)
{
}

World::~World()
{
}

std::shared_ptr<libcomp::InternalConnection> World::GetConnection() const
{
    return mConnection;
}

std::shared_ptr<objects::WorldDescription> World::GetWorldDescription() const
{
    return mWorldDescription;
}

const std::list<std::shared_ptr<objects::ChannelDescription>> World::GetChannelDescriptions() const
{
    return mChannelDescriptions;
}

std::shared_ptr<objects::ChannelDescription> World::GetChannelDescriptionByID(uint8_t id) const
{
    for(auto channel : mChannelDescriptions)
    {
        if(channel->GetID() == id)
        {
            return channel;
        }
    }

    return nullptr;
}

bool World::RemoveChannelDescriptionByID(uint8_t id)
{
    for(auto iter = mChannelDescriptions.begin(); iter != mChannelDescriptions.end(); iter++)
    {
        if((*iter)->GetID() == id)
        {
            mChannelDescriptions.erase(iter);
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

void World::SetWorldDescription(const std::shared_ptr<objects::WorldDescription>& worldDescription)
{
    mWorldDescription = worldDescription;
}

void World::SetChannelDescription(const std::shared_ptr<
    objects::ChannelDescription>& channelDescription)
{
    auto desc = GetChannelDescriptionByID(channelDescription->GetID());
    if(nullptr == desc)
    {
        mChannelDescriptions.push_back(channelDescription);
    }
    else
    {
        desc->SetName(channelDescription->GetName());
    }
}