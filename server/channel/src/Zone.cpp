/**
 * @file server/channel/src/Zone.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents an instance of a zone.
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

#include "Zone.h"

using namespace channel;

Zone::Zone(uint64_t id, const std::shared_ptr<objects::ServerZone>& definition)
    : mServerZone(definition), mID(id)
{
}

Zone::~Zone()
{
}

uint64_t Zone::GetID()
{
    return mID;
}

void Zone::AddConnection(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto primaryEntityID = client->GetClientState()->GetCharacterState()->GetEntityID();

    std::lock_guard<std::mutex> lock(mLock);
    mConnections[primaryEntityID] = client;
}

void Zone::RemoveConnection(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto primaryEntityID = client->GetClientState()->GetCharacterState()->GetEntityID();

    std::lock_guard<std::mutex> lock(mLock);
    mConnections.erase(primaryEntityID);
}

std::shared_ptr<NPCState> Zone::AddNPC(const std::shared_ptr<objects::ServerNPC>& npc)
{
    auto state = std::shared_ptr<NPCState>(new NPCState(npc));
    mNPCs.push_back(state);
    return state;
}

std::shared_ptr<ServerObjectState> Zone::AddObject(
    const std::shared_ptr<objects::ServerObject>& object)
{
    auto state = std::shared_ptr<ServerObjectState>(new ServerObjectState(object));
    mObjects.push_back(state);
    return state;
}


std::unordered_map<int32_t,
    std::shared_ptr<ChannelClientConnection>> Zone::GetConnections()
{
    return mConnections;
}

const std::list<std::shared_ptr<NPCState>> Zone::GetNPCs()
{
    return mNPCs;
}

const std::list<std::shared_ptr<ServerObjectState>> Zone::GetObjects()
{
    return mObjects;
}

const std::shared_ptr<objects::ServerZone> Zone::GetDefinition()
{
    return mServerZone;
}
