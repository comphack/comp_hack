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

// C++ Standard Includes
#include <cmath>

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
    auto cState = client->GetClientState()->GetCharacterState();
    auto dState = client->GetClientState()->GetDemonState();

    auto cEntityID = cState->GetEntityID();
    auto dEntityID = dState->GetEntityID();

    UnregisterEntityState(cEntityID);
    UnregisterEntityState(dEntityID);

    cState->SetZone(0);
    dState->SetZone(0);

    std::lock_guard<std::mutex> lock(mLock);
    mConnections.erase(cEntityID);
}

void Zone::AddEnemy(const std::shared_ptr<EnemyState>& enemy)
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEnemies.push_back(enemy);
    }
    RegisterEntityState(enemy);
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

std::list<std::shared_ptr<ChannelClientConnection>> Zone::GetConnectionList()
{
    std::lock_guard<std::mutex> lock(mLock);
    std::list<std::shared_ptr<ChannelClientConnection>> connections;
    for(auto cPair : mConnections)
    {
        connections.push_back(cPair.second);
    }
    return connections;
}

const std::shared_ptr<ActiveEntityState> Zone::GetActiveEntity(int32_t entityID)
{
    return std::dynamic_pointer_cast<ActiveEntityState>(GetEntity(entityID));
}


const std::list<std::shared_ptr<ActiveEntityState>>
    Zone::GetActiveEntitiesInRadius(float x, float y, double radius)
{
    float rSquared = (float)std::pow(radius, 2);

    std::lock_guard<std::mutex> lock(mLock);
    std::list<std::shared_ptr<ActiveEntityState>> results;
    for(auto ePair : mAllEntities)
    {
        auto active = std::dynamic_pointer_cast<ActiveEntityState>(ePair.second);
        if(active && rSquared >= active->GetDistance(x, y, true))
        {
            results.push_back(active);
        }
    }

    return results;
}

std::shared_ptr<EnemyState> Zone::GetEnemy(int32_t id)
{
    return std::dynamic_pointer_cast<EnemyState>(GetEntity(id));
}

const std::list<std::shared_ptr<EnemyState>> Zone::GetEnemies() const
{
    return mEnemies;
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

void Zone::SetNextStatusEffectTime(uint32_t time, int32_t entityID)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(time)
    {
        mNextEntityStatusTimes[time].insert(entityID);
    }
    else
    {
        for(auto pair : mNextEntityStatusTimes)
        {
            pair.second.erase(entityID);
        }
    }
}

std::list<std::shared_ptr<ActiveEntityState>>
    Zone::GetUpdatedStatusEffectEntities(uint32_t now)
{
    std::list<std::shared_ptr<ActiveEntityState>> result;
    std::set<uint32_t> passed;

    std::lock_guard<std::mutex> lock(mLock);
    for(auto pair : mNextEntityStatusTimes)
    {
        if(pair.first > now) break;

        passed.insert(pair.first);
        for(auto entityID : pair.second)
        {
            auto it = mAllEntities.find(entityID);
            auto active = it != mAllEntities.end()
                ? std::dynamic_pointer_cast<ActiveEntityState>(it->second)
                : nullptr;
            if(active)
            {
                result.push_back(active);
            }
        }
    }

    for(auto p : passed)
    {
        mNextEntityStatusTimes.erase(p);
    }

    return result;
}

void Zone::Cleanup()
{
    std::lock_guard<std::mutex> lock(mLock);
    for(auto pair : mAllEntities)
    {
        auto active = std::dynamic_pointer_cast<ActiveEntityState>(pair.second);
        if(active)
        {
            active->SetZone(0, false);
        }
    }

    mEnemies.clear();
    mNPCs.clear();
    mObjects.clear();
    mAllEntities.clear();
}
