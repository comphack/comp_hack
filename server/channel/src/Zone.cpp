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

Zone::Zone(uint32_t id, const std::shared_ptr<objects::ServerZone>& definition)
    : mServerZone(definition), mID(id)
{
}

Zone::~Zone()
{
}

uint32_t Zone::GetID()
{
    return mID;
}

void Zone::AddConnection(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto cState = client->GetClientState()->GetCharacterState();
    auto dState = client->GetClientState()->GetDemonState();

    RegisterEntityState(cState);
    RegisterEntityState(dState);

    std::lock_guard<std::mutex> lock(mLock);
    mConnections[cState->GetEntityID()] = client;
}

void Zone::RemoveConnection(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto cEntityID = client->GetClientState()->GetCharacterState()->GetEntityID();
    auto dEntityID = client->GetClientState()->GetDemonState()->GetEntityID();

    UnregisterEntityState(cEntityID);
    UnregisterEntityState(dEntityID);

    std::lock_guard<std::mutex> lock(mLock);
    mConnections.erase(cEntityID);
}

void Zone::AddNPC(const std::shared_ptr<NPCState>& npc)
{
    mNPCs.push_back(npc);
    RegisterEntityState(npc);
}

void Zone::AddObject(const std::shared_ptr<ServerObjectState>& object)
{
    mObjects.push_back(object);
    RegisterEntityState(object);
}

std::unordered_map<int32_t,
    std::shared_ptr<ChannelClientConnection>> Zone::GetConnections()
{
    return mConnections;
}

const std::shared_ptr<ActiveEntityState> Zone::GetActiveEntityState(int32_t entityID)
{
    return std::dynamic_pointer_cast<ActiveEntityState>(GetEntity(entityID));
}

const std::list<std::shared_ptr<NPCState>> Zone::GetNPCs() const
{
    return mNPCs;
}

const std::list<std::shared_ptr<ServerObjectState>> Zone::GetServerObjects() const
{
    return mObjects;
}

const std::shared_ptr<objects::ServerZone> Zone::GetDefinition()
{
    return mServerZone;
}

void Zone::RegisterEntityState(const std::shared_ptr<objects::EntityStateObject>& state)
{
    std::lock_guard<std::mutex> lock(mLock);
    mAllEntities[state->GetEntityID()] = state;
}

void Zone::UnregisterEntityState(int32_t entityID)
{
    std::lock_guard<std::mutex> lock(mLock);
    mAllEntities.erase(entityID);
}

std::shared_ptr<objects::EntityStateObject> Zone::GetEntity(int32_t id)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mAllEntities.find(id);

    if(mAllEntities.end() != it)
    {
        return it->second;
    }

    return {};
}

std::shared_ptr<NPCState> Zone::GetNPC(int32_t id)
{
    return std::dynamic_pointer_cast<NPCState>(GetEntity(id));
}

std::shared_ptr<ServerObjectState> Zone::GetServerObject(int32_t id)
{
    return std::dynamic_pointer_cast<ServerObjectState>(GetEntity(id));
}
