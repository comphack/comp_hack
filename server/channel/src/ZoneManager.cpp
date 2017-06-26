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
#include <Constants.h>
#include <ScriptEngine.h>

// objects Include
#include <Account.h>
#include <AccountLogin.h>
#include <CharacterLogin.h>
#include <Enemy.h>
#include <EntityStats.h>
#include <MiDevilData.h>
#include <MiGrowthData.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <TradeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "Zone.h"

// Other Includes
#include <cmath>

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
    uint32_t zoneID, float xCoord, float yCoord, float rotation, bool forceLeave)
{
    auto instance = GetZone(zoneID);
    if(instance == nullptr)
    {
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto primaryEntityID = cState->GetEntityID();

    if(forceLeave)
    {
        LeaveZone(client);

        // Pull a fresh version of the zone in case it was cleaned up
        instance = GetZone(zoneID);
        if(instance == nullptr)
        {
            return false;
        }
    }

    auto instanceID = instance->GetID();
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEntityMap[primaryEntityID] = instanceID;
    }
    instance->AddConnection(client);

    auto server = mServer.lock();
    auto ticks = server->GetServerTime();

    // Move the entity to the new location.
    cState->SetOriginX(xCoord);
    cState->SetOriginY(yCoord);
    cState->SetOriginRotation(rotation);
    cState->SetOriginTicks(ticks);
    cState->SetDestinationX(xCoord);
    cState->SetDestinationY(yCoord);
    cState->SetDestinationRotation(rotation);
    cState->SetDestinationTicks(ticks);
    cState->SetCurrentX(xCoord);
    cState->SetCurrentY(yCoord);
    cState->SetCurrentRotation(rotation);

    dState->SetOriginX(xCoord);
    dState->SetOriginY(yCoord);
    dState->SetOriginRotation(rotation);
    dState->SetOriginTicks(ticks);
    dState->SetDestinationX(xCoord);
    dState->SetDestinationY(yCoord);
    dState->SetDestinationRotation(rotation);
    dState->SetDestinationTicks(ticks);
    dState->SetCurrentX(xCoord);
    dState->SetCurrentY(yCoord);
    dState->SetCurrentRotation(rotation);

    auto zoneDef = instance->GetDefinition();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE);
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteS32Little((int32_t)instance->GetID());
    reply.WriteFloat(xCoord);
    reply.WriteFloat(yCoord);
    reply.WriteFloat(rotation);
    reply.WriteS32Little((int32_t)zoneDef->GetDynamicMapID());

    client->SendPacket(reply);

    // Tell the world that the character has changed zones
    auto cLogin = state->GetAccountLogin()->GetCharacterLogin();

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
    request.WriteS32Little(cLogin->GetWorldCID());
    if(cLogin->GetZoneID() == 0)
    {
        // Send first zone in info
        request.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_STATUS
            | (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE);
        request.WriteS8((int8_t)cLogin->GetStatus());
    }
    else
    {
        // Send normal zone chang info
        request.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE);
    }
    request.WriteU32Little(zoneID);
    cLogin->SetZoneID(zoneID);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

    return true;
}

void ZoneManager::LeaveZone(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto primaryEntityID = state->GetCharacterState()->GetEntityID();

    // Detach from zone specific state info
    if(state->GetTradeSession()->GetOtherCharacterState() != nullptr)
    {
        auto server = mServer.lock();
        auto characterManager = server->GetCharacterManager();
        auto connectionManager = server->GetManagerConnection();

        auto otherCState = std::dynamic_pointer_cast<CharacterState>(
            state->GetTradeSession()->GetOtherCharacterState());
        auto otherChar = otherCState->GetEntity();
        auto otherClient = otherChar != nullptr ? 
            connectionManager->GetClientConnection(
                otherChar->GetAccount()->GetUsername()) : nullptr;

        if(otherClient)
        {
            characterManager->EndTrade(otherClient);
        }

        characterManager->EndTrade(client);
    }

    bool instanceRemoved = false;
    std::shared_ptr<Zone> zone = nullptr;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mEntityMap.find(primaryEntityID);
        if(iter != mEntityMap.end())
        {
            uint32_t instanceID = iter->second;
            zone = mZones[instanceID];

            mEntityMap.erase(primaryEntityID);
            zone->RemoveConnection(client);
            if(zone->GetConnections().size() == 0)
            {
                mZones.erase(instanceID);

                auto zoneDefID = zone->GetDefinition()->GetID();
                std::set<uint32_t>& instances = mZoneMap[zoneDefID];
                instances.erase(instanceID);
                if(instances.size() == 0)
                {
                    mZoneMap.erase(zoneDefID);
                    instanceRemoved = true;
                }
            }
        }
        else
        {
            // Not in a zone, nothing to do
            return;
        }
    }

    if(!instanceRemoved)
    {
        auto demonID = state->GetDemonState()->GetEntityID();
        std::list<int32_t> entityIDs = { primaryEntityID, demonID };
        RemoveEntitiesFromZone(zone, entityIDs);
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
    auto characterManager = server->GetCharacterManager();
    
    // Send the new connection entity data to the other clients
    auto otherClients = GetZoneConnections(client, false);
    if(otherClients.size() > 0)
    {
        characterManager->SendOtherCharacterData(otherClients, state);
        if(nullptr != dState->GetEntity())
        {
            characterManager->SendOtherPartnerData(otherClients, state);
        }
    }

    // The client's partner demon will be shown elsewhere

    PopEntityForZoneProduction(client, characterEntityID, 0);
    ShowEntityToZone(client, characterEntityID);

    // It seems that if entity data is sent to the client before a previous
    // entity was processed and shown, the client will force a log-out. To
    // counter-act this, all message information remaining of this type will
    // be queued and sent together at the end.
    for(auto enemyState : zone->GetEnemies())
    {
        SendEnemyData(client, enemyState, zone, true);
    }

    /// @todo: send corpses

    for(auto npcState : zone->GetNPCs())
    {
        auto npc = npcState->GetEntity();

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_NPC_DATA);
        reply.WriteS32Little(npcState->GetEntityID());
        reply.WriteU32Little(npc->GetID());
        reply.WriteS32Little((int32_t)zone->GetID());
        reply.WriteS32Little((int32_t)zoneData->GetID());
        reply.WriteFloat(npcState->GetCurrentX());
        reply.WriteFloat(npcState->GetCurrentY());
        reply.WriteFloat(npcState->GetCurrentRotation());
        reply.WriteS16Little(0);    //Unknown

        client->QueuePacket(reply);
        ShowEntity(client, npcState->GetEntityID(), true);
    }

    for(auto objState : zone->GetServerObjects())
    {
        auto obj = objState->GetEntity();

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_OBJECT_NPC_DATA);
        reply.WriteS32Little(objState->GetEntityID());
        reply.WriteU32Little(obj->GetID());
        reply.WriteU8(obj->GetState());
        reply.WriteS32Little((int32_t)zone->GetID());
        reply.WriteS32Little((int32_t)zoneData->GetID());
        reply.WriteFloat(objState->GetCurrentX());
        reply.WriteFloat(objState->GetCurrentY());
        reply.WriteFloat(objState->GetCurrentRotation());

        client->QueuePacket(reply);
        ShowEntity(client, objState->GetEntityID(), true);
    }

    // Send all the queued NPC packets
    client->FlushOutgoing();
    
    std::list<std::shared_ptr<ChannelClientConnection>> self = { client };
    for(auto oConnection : otherClients)
    {
        auto oState = oConnection->GetClientState();
        auto oCharacterState = oState->GetCharacterState();
        auto oDemonState = oState->GetDemonState();

        characterManager->SendOtherCharacterData(self, oState);
        PopEntityForProduction(client, oCharacterState->GetEntityID(), 0);
        ShowEntity(client, oCharacterState->GetEntityID());

        if(nullptr != oDemonState->GetEntity())
        {
            characterManager->SendOtherPartnerData(self, oState);
            PopEntityForProduction(client, oDemonState->GetEntityID(), 2);
            ShowEntity(client, oDemonState->GetEntityID());
        }
    }
}

void ZoneManager::ShowEntity(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID, bool queue)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_ENTITY);
    p.WriteS32Little(entityID);

    if(queue)
    {
        client->QueuePacket(p);
    }
    else
    {
        client->SendPacket(p);
    }
}

void ZoneManager::ShowEntityToZone(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_ENTITY);
    p.WriteS32Little(entityID);

    BroadcastPacket(client, p, true);
}

void ZoneManager::PopEntityForProduction(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID, int32_t type, bool queue)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_POP_ENTITY_FOR_PRODUCTION);
    p.WriteS32Little(entityID);
    p.WriteS32Little(type);

    if(queue)
    {
        client->QueuePacket(p);
    }
    else
    {
        client->SendPacket(p);
    }
}

void ZoneManager::PopEntityForZoneProduction(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID, int32_t type)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_POP_ENTITY_FOR_PRODUCTION);
    p.WriteS32Little(entityID);
    p.WriteS32Little(type);

    BroadcastPacket(client, p, true);
}

void ZoneManager::RemoveEntitiesFromZone(const std::shared_ptr<Zone>& zone,
    const std::list<int32_t>& entityIDs)
{
    for(int32_t entityID : entityIDs)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_ENTITY);
        p.WriteS32Little(entityID);
        p.WriteS32Little(0);

        BroadcastPacket(zone, p);
    }
}

void ZoneManager::SendEnemyData(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<EnemyState>& enemyState, const std::shared_ptr<Zone>& zone,
    bool sendToAll, bool queue)
{
    auto stats = enemyState->GetCoreStats();
    auto zoneData = zone->GetDefinition();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENEMY);
    p.WriteS32Little(enemyState->GetEntityID());
    p.WriteS32Little((int32_t)enemyState->GetEntity()->GetType());
    p.WriteS32Little((int32_t)stats->GetMaxHP());
    p.WriteS32Little((int32_t)stats->GetHP());
    p.WriteS8(stats->GetLevel());
    p.WriteS32Little((int32_t)zone->GetID());
    p.WriteS32Little((int32_t)zoneData->GetID());
    p.WriteFloat(enemyState->GetOriginX());
    p.WriteFloat(enemyState->GetOriginY());
    p.WriteFloat(enemyState->GetOriginRotation());

    uint32_t unknownCount = 0;
    p.WriteU32Little(unknownCount);
    for(uint32_t i = 0; i < unknownCount; i++)
    {
        p.WriteU32Little(0);
        p.WriteS32Little(0);
        p.WriteU8(0);
    }

    //Unknown
    p.WriteU32Little(0);

    if(!sendToAll)
    {
        if(queue)
        {
            client->QueuePacket(p);
        }
        else
        {
            client->SendPacket(p);
        }

        PopEntityForProduction(client, enemyState->GetEntityID(), 3, queue);
        ShowEntity(client, enemyState->GetEntityID(), queue);
    }
    else
    {
        BroadcastPacket(client, p, true);
        PopEntityForZoneProduction(client, enemyState->GetEntityID(), 3);
        ShowEntityToZone(client, enemyState->GetEntityID());
    }
}

void ZoneManager::BroadcastPacket(const std::shared_ptr<ChannelClientConnection>& client,
    libcomp::Packet& p, bool includeSelf)
{
    std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
    for(auto connection : GetZoneConnections(client, includeSelf))
    {
        connections.push_back(connection);
    }

    libcomp::TcpConnection::BroadcastPacket(connections, p);
}

void ZoneManager::BroadcastPacket(const std::shared_ptr<Zone>& zone, libcomp::Packet& p)
{
    if(nullptr != zone)
    {
        std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
        for(auto connectionPair : zone->GetConnections())
        {
            connections.push_back(connectionPair.second);
        }

        libcomp::TcpConnection::BroadcastPacket(connections, p);
    }
}

void ZoneManager::SendToRange(const std::shared_ptr<ChannelClientConnection>& client, libcomp::Packet& p, bool includeSelf)
{
    //get all clients in range
    auto state = client->GetClientState(); //Get sender's client's state
    auto cState = state->GetCharacterState(); //Get sender's entity state
    auto MyX = cState->GetDestinationX();  //Get Sending Char's X
    auto MyY = cState->GetDestinationY();  //Get Sending Char's Y
    auto r = CHAT_RADIUS_SAY;

    std::list<std::shared_ptr<libcomp::TcpConnection>> zConnections;
    for(auto zConnection : GetZoneConnections(client, includeSelf))
    {
        auto zState = zConnection->GetClientState()->GetCharacterState();
        //ZoneX is the x-coord of the other character in the zone
        auto ZoneX = zState->GetDestinationX();
        //ZoneY is the y-coord of the other character in question.
        auto ZoneY = zState->GetDestinationY();
            //This checks to see if the other character is in range of the sender
        if(std::pow(r,2) >= std::pow((MyX-ZoneX),2)+std::pow((MyY-ZoneY),2))
        {
            zConnections.push_back(zConnection);
        }
    }
    libcomp::TcpConnection::BroadcastPacket(zConnections, p);
}

std::list<std::shared_ptr<ChannelClientConnection>> ZoneManager::GetZoneConnections(
    const std::shared_ptr<ChannelClientConnection>& client, bool includeSelf)
{
    std::list<std::shared_ptr<ChannelClientConnection>> connections;

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
        for(auto connectionPair : zone->GetConnections())
        {
            if(includeSelf || connectionPair.first != primaryEntityID)
            {
                connections.push_back(connectionPair.second);
            }
        }
    }

    return connections;
}

bool ZoneManager::SpawnEnemy(const std::shared_ptr<Zone>& zone, uint32_t demonID,
    float x, float y, float rot, const libcomp::String& aiType)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto serverDataManager = server->GetServerDataManager();
    auto def = definitionManager->GetDevilData(demonID);

    if(nullptr == def)
    {
        return false;
    }

    auto enemy = std::shared_ptr<objects::Enemy>(new objects::Enemy);
    enemy->SetType(demonID);

    auto enemyStats = libcomp::PersistentObject::New<objects::EntityStats>();
    enemyStats->SetLevel((int8_t)def->GetGrowth()->GetBaseLevel());
    server->GetCharacterManager()->CalculateDemonBaseStats(enemyStats, def);
    enemy->SetCoreStats(enemyStats);

    auto eState = std::shared_ptr<EnemyState>(new EnemyState);
    eState->SetEntityID(server->GetNextEntityID());
    eState->SetOriginX(x);
    eState->SetOriginY(y);
    eState->SetOriginRotation(rot);
    eState->SetDestinationX(x);
    eState->SetDestinationY(y);
    eState->SetDestinationRotation(rot);
    eState->SetCurrentX(x);
    eState->SetCurrentY(y);
    eState->SetCurrentRotation(rot);
    eState->SetEntity(enemy);
    eState->Prepare(eState, aiType, serverDataManager);

    eState->RecalculateStats(definitionManager);

    zone->AddEnemy(eState);

    //If anyone is currently connected, immediately send the enemy's info
    auto clients = zone->GetConnections();
    if(clients.size() > 0)
    {
        auto firstClient = clients.begin()->second;
        SendEnemyData(firstClient, eState, zone, true, false);
    }

    return true;
}

void ZoneManager::UpdateActiveZoneStates()
{
    /// @todo: keep track of active zones
    std::list<std::shared_ptr<Zone>> instances;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto zPair : mZoneMap)
        {
            for(auto instanceID : zPair.second)
            {
                instances.push_back(mZones[instanceID]);
            }
        }
    }

    auto currentTime = mServer.lock()->GetServerTime();
    std::set<std::shared_ptr<ChannelClientConnection>> clients;
    for(auto instance : instances)
    {
        std::list<std::shared_ptr<EnemyState>> updated;
        for(auto enemy : instance->GetEnemies())
        {
            if(enemy->UpdateState(currentTime))
            {
                updated.push_back(enemy);
            }
        }

        for(auto enemy : updated)
        {
            // Update the clients with what the enemy is doing

            // Check if the enemy's position or rotation has updated
            if(currentTime == enemy->GetOriginTicks())
            {
                if(enemy->IsMoving())
                {
                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MOVE);
                    p.WriteS32Little(enemy->GetEntityID());
                    p.WriteFloat(enemy->GetDestinationX());
                    p.WriteFloat(enemy->GetDestinationY());
                    p.WriteFloat(enemy->GetOriginX());
                    p.WriteFloat(enemy->GetOriginY());
                    p.WriteFloat(1.f);      /// @todo: correct rate per second

                    uint32_t timePos = p.Size();
                    for(auto zPair : instance->GetConnections())
                    {
                        auto client = zPair.second;
                        auto state = client->GetClientState();
                        float startAdjust = state->ToClientTime(currentTime);
                        float stopAdjust = state->ToClientTime(
                            enemy->GetDestinationTicks());

                        p.Seek(timePos);
                        p.WriteFloat(startAdjust);
                        p.WriteFloat(stopAdjust);

                        client->QueuePacket(p);
                        clients.insert(client);
                    }
                }
                else if(enemy->IsRotating())
                {
                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ROTATE);
                    p.WriteS32Little(enemy->GetEntityID());
                    p.WriteFloat(enemy->GetDestinationRotation());

                    uint32_t timePos = p.Size();
                    for(auto zPair : instance->GetConnections())
                    {
                        auto client = zPair.second;

                        auto state = client->GetClientState();
                        float startAdjust = state->ToClientTime(currentTime);
                        float stopAdjust = state->ToClientTime(
                            enemy->GetDestinationTicks());

                        p.Seek(timePos);
                        p.WriteFloat(startAdjust);
                        p.WriteFloat(stopAdjust);

                        client->QueuePacket(p);
                        clients.insert(client);
                    }
                }
                else
                {
                    // The movement was actually a stop

                    libcomp::Packet p;
                    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_STOP_MOVEMENT);
                    p.WriteS32Little(enemy->GetEntityID());
                    p.WriteFloat(enemy->GetDestinationX());
                    p.WriteFloat(enemy->GetDestinationY());

                    // Times must be sent relative to the other players
                    uint32_t timePos = p.Size();
                    for(auto zPair : instance->GetConnections())
                    {
                        auto client = zPair.second;

                        auto state = client->GetClientState();
                        float stopAdjust = state->ToClientTime(
                            enemy->GetDestinationTicks());

                        p.Seek(timePos);
                        p.WriteFloat(stopAdjust);

                        client->QueuePacket(p);
                        clients.insert(client);
                    }
                }
            }

            /// @todo: check for ability usage
        }
    }

    // Send all of the updates
    for(auto client : clients)
    {
        client->FlushOutgoing();
    }
}

void ZoneManager::Warp(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<ActiveEntityState>& eState, float xPos, float yPos,
    float rot)
{
    auto server = mServer.lock();
    ServerTime timestamp = server->GetServerTime();

    eState->SetOriginX(xPos);
    eState->SetOriginY(yPos);
    eState->SetOriginTicks(timestamp);
    eState->SetDestinationX(xPos);
    eState->SetDestinationY(yPos);
    eState->SetDestinationTicks(timestamp);
    eState->SetCurrentX(xPos);
    eState->SetCurrentY(yPos);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_WARP);
    p.WriteS32Little(eState->GetEntityID());
    p.WriteFloat(xPos);
    p.WriteFloat(yPos);
    p.WriteFloat(0.0f);  // Unknown
    p.WriteFloat(rot);

    uint32_t timePos = p.Size();
    auto connections = server->GetZoneManager()->GetZoneConnections(client, true);
    for(auto zConnection : connections)
    {
        auto otherState = zConnection->GetClientState();
        float timeAdjust = otherState->ToClientTime(timestamp);

        p.Seek(timePos);
        p.WriteFloat(timeAdjust);

        zConnection->SendPacket(p);
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
        state->SetCurrentX(npc->GetX());
        state->SetCurrentY(npc->GetY());
        state->SetCurrentRotation(npc->GetRotation());
        state->SetEntityID(server->GetNextEntityID());
        state->SetActions(npc->GetActions());
        zone->AddNPC(state);
    }
    
    for(auto obj : definition->GetObjects())
    {
        auto state = std::shared_ptr<ServerObjectState>(
            new ServerObjectState(obj));
        state->SetCurrentX(obj->GetX());
        state->SetCurrentY(obj->GetY());
        state->SetCurrentRotation(obj->GetRotation());
        state->SetEntityID(server->GetNextEntityID());
        state->SetActions(obj->GetActions());
        zone->AddObject(state);
    }

    {
        std::lock_guard<std::mutex> lock(mLock);
        mZones[id] = zone;
        mZoneMap[definition->GetID()].insert(id);
    }

    return zone;
}
