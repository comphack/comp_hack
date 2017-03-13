/**
 * @file server/channel/src/ZoneManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages zone instance objects and connections.
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

#include "ZoneManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// objects Include
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"
#include "Zone.h"

using namespace channel;

ZoneManager::ZoneManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server), mNextZoneInstanceID(1)
{
}

ZoneManager::~ZoneManager()
{
}

std::shared_ptr<Zone> ZoneManager::GetZoneInstance(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto primaryEntityID = client->GetClientState()->GetCharacterState()->GetEntityID();
    return GetZoneInstance(primaryEntityID);
}

std::shared_ptr<Zone> ZoneManager::GetZoneInstance(int32_t primaryEntityID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mEntityMap.find(primaryEntityID);
    if(iter != mEntityMap.end())
    {
        return mZones[iter->second];
    }

    return nullptr;
}

bool ZoneManager::EnterZone(const std::shared_ptr<ChannelClientConnection>& client,
    uint32_t zoneID)
{
    auto instance = GetZone(zoneID);
    if(instance == nullptr)
    {
        return false;
    }

    auto instanceID = instance->GetID();

    auto state = client->GetClientState();
    auto primaryEntityID = state->GetCharacterState()->GetEntityID();
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEntityMap[primaryEntityID] = instanceID;
        mZones[instanceID]->AddConnection(client);
    }

    return true;
}

void ZoneManager::LeaveZone(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto primaryEntityID = state->GetCharacterState()->GetEntityID();
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mEntityMap.find(primaryEntityID);
        if(iter != mEntityMap.end())
        {
            uint32_t instanceID = iter->second;
            auto zone = mZones[instanceID];

            mEntityMap.erase(primaryEntityID);
            zone->RemoveConnection(client);
            if(zone->GetConnections().size() == 0)
            {
                zone->RemoveConnection(client);
                mZones.erase(instanceID);

                auto zoneDefID = zone->GetDefinition()->GetID();
                std::set<uint32_t>& instances = mZoneMap[zoneDefID];
                instances.erase(instanceID);
                if(instances.size() == 0)
                {
                    mZoneMap.erase(zoneDefID);
                }
            }
        }
    }
}

void ZoneManager::SendPopulateZoneData(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto characterEntityID = cState->GetEntityID();

    auto zone = GetZoneInstance(characterEntityID);
    auto zoneData = zone->GetDefinition();
    auto zoneConnections = zone->GetConnections();
    auto characterManager = server->GetCharacterManager();
    
    // Send the new connection entity data to the other clients
    for(auto connectionPair : zoneConnections)
    {
        if(connectionPair.first != characterEntityID)
        {
            auto oConnection = connectionPair.second;
            characterManager->SendOtherCharacterData(oConnection, state);
            
            if(nullptr != dState->GetEntity())
            {
                characterManager->SendOtherPartnerData(oConnection, state);
            }
        }
    }

    // The client's partner demon will be shown elsewhere

    ShowEntityToZone(client, characterEntityID);

    /// @todo: Populate enemies

    // It seems that if NPC data is sent to the client before a previous
    // NPC was processed and shown, the client will force a log-out. To
    // counter-act this, all message information of this type will be queued
    // and sent together at the end.
    for(auto npcState : zone->GetNPCs())
    {
        auto npc = npcState->GetEntity();

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_NPC_DATA);
        reply.WriteS32Little(npcState->GetEntityID());
        reply.WriteU32Little(npc->GetID());
        reply.WriteS32Little((int32_t)zoneData->GetSet());
        reply.WriteS32Little((int32_t)zoneData->GetID());
        reply.WriteFloat(npcState->GetOriginX());
        reply.WriteFloat(npcState->GetOriginY());
        reply.WriteFloat(npcState->GetOriginRotation());
        reply.WriteS16Little(0);    //Unknown

        client->QueuePacket(reply);
        ShowEntity(client, npcState->GetEntityID(), true);
    }

    for(auto objState : zone->GetObjects())
    {
        auto obj = objState->GetEntity();

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_OBJECT_NPC_DATA);
        reply.WriteS32Little(objState->GetEntityID());
        reply.WriteU32Little(obj->GetID());
        reply.WriteU8(obj->GetState());
        reply.WriteS32Little((int32_t)zoneData->GetSet());
        reply.WriteS32Little((int32_t)zoneData->GetID());
        reply.WriteFloat(objState->GetOriginX());
        reply.WriteFloat(objState->GetOriginY());
        reply.WriteFloat(objState->GetOriginRotation());

        client->QueuePacket(reply);
        ShowEntity(client, objState->GetEntityID(), true);
    }

    // Send all the queued NPC packets
    client->FlushOutgoing();
    
    for(auto connectionPair : zoneConnections)
    {
        if(connectionPair.first != characterEntityID)
        {
            auto oConnection = connectionPair.second;
            auto oState = oConnection->GetClientState();
            auto oCharacterState = oState->GetCharacterState();
            auto oDemonState = oState->GetDemonState();

            characterManager->SendOtherCharacterData(client, oState);
            ShowEntity(client, oCharacterState->GetEntityID());

            if(nullptr != oDemonState->GetEntity())
            {
                characterManager->SendOtherPartnerData(client, oState);
                ShowEntity(client, oDemonState->GetEntityID());
            }
        }
    }
}

void ZoneManager::ShowEntity(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID, bool queue)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_ENTITY);
    reply.WriteS32Little(entityID);

    if(queue)
    {
        client->QueuePacket(reply);
    }
    else
    {
        client->SendPacket(reply);
    }
}

void ZoneManager::ShowEntityToZone(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_ENTITY);
    reply.WriteS32Little(entityID);

    mServer.lock()->GetZoneManager()->BroadcastPacket(client, reply, true);
}

void ZoneManager::BroadcastPacket(const std::shared_ptr<ChannelClientConnection>& client,
    libcomp::Packet& p, bool includeSelf)
{
    auto primaryEntityID = client->GetClientState()->GetCharacterState()->GetEntityID();
    std::shared_ptr<Zone> zone;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mEntityMap.find(primaryEntityID);
        if(iter != mEntityMap.end())
        {
            zone = mZones[iter->second];
        }
    }

    if(nullptr != zone)
    {
        std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
        for(auto connectionPair : zone->GetConnections())
        {
            if(includeSelf || connectionPair.first != primaryEntityID)
            {
                connections.push_back(connectionPair.second);
            }
        }

        libcomp::TcpConnection::BroadcastPacket(connections, p);
    }
}

std::shared_ptr<Zone> ZoneManager::GetZone(uint32_t zoneID)
{
    std::shared_ptr<Zone> zone;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mZoneMap.find(zoneID);
        if(iter != mZoneMap.end())
        {
            for(uint32_t instanceID : iter->second)
            {
                auto instance = mZones[instanceID];
                /// @todo: replace with public/private logic
                if(nullptr != instance)
                {
                    zone = instance;
                    break;
                }
            }
        }
    }

    if(nullptr == zone)
    {
        auto server = mServer.lock();
        auto zoneDefinition = server->GetServerDataManager()
            ->GetZoneData(zoneID);
        zone = CreateZoneInstance(zoneDefinition);
    }

    return zone;
}

std::shared_ptr<Zone> ZoneManager::CreateZoneInstance(
    const std::shared_ptr<objects::ServerZone>& definition)
{
    if(nullptr == definition)
    {
        return nullptr;
    }

    uint32_t id;
    {
        std::lock_guard<std::mutex> lock(mLock);
        id = mNextZoneInstanceID++;
    }

    auto server = mServer.lock();
    auto zone = std::shared_ptr<Zone>(new Zone(id, definition));
    for(auto npc : definition->GetNPCs())
    {
        auto state = std::shared_ptr<NPCState>(new NPCState(npc));
        state->SetOriginX(npc->GetX());
        state->SetOriginY(npc->GetY());
        state->SetOriginRotation(npc->GetRotation());
        state->SetEntityID(server->GetNextEntityID());
        zone->AddNPC(state);
    }
    
    for(auto obj : definition->GetObjects())
    {
        auto state = std::shared_ptr<ServerObjectState>(
            new ServerObjectState(obj));
        state->SetOriginX(obj->GetX());
        state->SetOriginY(obj->GetY());
        state->SetOriginRotation(obj->GetRotation());
        state->SetEntityID(server->GetNextEntityID());
        zone->AddObject(state);
    }

    {
        std::lock_guard<std::mutex> lock(mLock);
        mZones[id] = zone;
        mZoneMap[definition->GetID()].insert(id);
    }

    return zone;
}
