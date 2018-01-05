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
#include <Constants.h>
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ScriptEngine.h>

// objects Include
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <ActionSpawn.h>
#include <CharacterLogin.h>
#include <Enemy.h>
#include <EntityStats.h>
#include <Item.h>
#include <ItemDrop.h>
#include <MiDevilData.h>
#include <MiDynamicMapData.h>
#include <MiGrowthData.h>
#include <MiSpotData.h>
#include <MiZoneData.h>
#include <MiZoneFileData.h>
#include <Party.h>
#include <PlayerExchangeSession.h>
#include <QmpBoundary.h>
#include <QmpBoundaryLine.h>
#include <QmpElement.h>
#include <QmpFile.h>
#include <ServerObject.h>
#include <ServerBazaar.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneSpot.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocation.h>
#include <SpawnLocationGroup.h>

// channel Includes
#include "AIState.h"
#include "ChannelServer.h"
#include "Zone.h"

// C++ Standard Includes
#include <cmath>

using namespace channel;

ZoneManager::ZoneManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server), mNextZoneInstanceID(1)
{
}

ZoneManager::~ZoneManager()
{
    for(auto zPair : mZones)
    {
        zPair.second->Cleanup();
    }
}

void ZoneManager::LoadGeometry()
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto serverDataManager = server->GetServerDataManager();

    auto zoneIDs = serverDataManager->GetAllZoneIDs();

    // Build zone geometry from QMP files
    for(auto zonePair : zoneIDs)
    {
        uint32_t zoneID = zonePair.first;
        auto zoneData = definitionManager->GetZoneData(zoneID);

        libcomp::String filename = zoneData->GetFile()->GetQmpFile();
        if(filename.IsEmpty() || mZoneGeometry.find(filename.C()) != mZoneGeometry.end()) continue;

        auto qmpFile = definitionManager->LoadQmpFile(filename, server->GetDataStore());
        if(!qmpFile)
        {
            //success = false;
            LOG_ERROR(libcomp::String("Failed to load zone geometry file: %1\n")
                .Arg(filename));
            continue;
        }

        LOG_DEBUG(libcomp::String("Loaded zone geometry file: %1\n")
            .Arg(filename));

        std::unordered_map<uint32_t, libcomp::String> elementMap;
        for(auto qmpElem : qmpFile->GetElements())
        {
            elementMap[qmpElem->GetID()] = qmpElem->GetName();
        }

        std::unordered_map<uint32_t, std::list<Line>> lineMap;
        for(auto qmpBoundary : qmpFile->GetBoundaries())
        {
            for(auto qmpLine : qmpBoundary->GetLines())
            {
                Line l(Point((float)qmpLine->GetX1(), (float)qmpLine->GetY1()),
                    Point((float)qmpLine->GetX2(), (float)qmpLine->GetY2()));
                lineMap[qmpLine->GetElementID()].push_back(l);
            }
        }

        auto geometry = std::make_shared<ZoneGeometry>();
        geometry->QmpFilename = filename;

        uint32_t instanceID = 1;
        for(auto pair : lineMap)
        {
            auto shape =  std::make_shared<ZoneQmpShape>();
            shape->ShapeID = pair.first;
            shape->ElementName = elementMap[pair.first];

            // Build a complete shape from the lines provided
            // If there is a gap in the shape, it is a line instead
            // of a full shape
            auto lines = pair.second;

            shape->Lines.push_back(lines.front());
            lines.pop_front();
            Line& firstLine = shape->Lines.front();

            Point* connectPoint = &shape->Lines.back().second;
            while(lines.size() > 0)
            {
                bool connected = false;
                for(auto it = lines.begin(); it != lines.end(); it++)
                {
                    if(it->first == *connectPoint)
                    {
                        shape->Lines.push_back(*it);
                        connected = true;
                    }
                    else if(it->second == *connectPoint)
                    {
                        shape->Lines.push_back(Line(it->second, it->first));
                        connected = true;
                    }

                    if(connected)
                    {
                        connectPoint = &shape->Lines.back().second;
                        lines.erase(it);
                        break;
                    }
                }

                if(!connected || lines.size() == 0)
                {
                    shape->InstanceID = instanceID++;

                    auto completeShape = shape;
                    if(*connectPoint == firstLine.first)
                    {
                        // Solid shape completed
                        shape->IsLine = false;
                    }

                    geometry->Shapes.push_back(shape);

                    // Determine the boundaries of the completed shape
                    std::list<float> xVals;
                    std::list<float> yVals;

                    for(Line& line : shape->Lines)
                    {
                        for(const Point& p : { line.first, line.second })
                        {
                            xVals.push_back(p.x);
                            yVals.push_back(p.y);
                        }
                    }

                    xVals.sort([](const float& a, const float& b)
                        {
                            return a < b;
                        });

                    yVals.sort([](const float& a, const float& b)
                        {
                            return a < b;
                        });

                    shape->Boundaries[0] = Point(xVals.front(), yVals.front());
                    shape->Boundaries[1] = Point(xVals.back(), yVals.back());

                    if(lines.size() > 0)
                    {
                        // Start a new shape
                        shape = std::make_shared<ZoneQmpShape>();
                        shape->ShapeID = pair.first;
                        shape->ElementName = elementMap[pair.first];

                        shape->Lines.push_back(lines.front());
                        lines.pop_front();
                        firstLine = shape->Lines.front();
                        connectPoint = &shape->Lines.back().second;
                    }
                }
            }
        }

        mZoneGeometry[filename.C()] = geometry;
    }

    // Build any existing zone spots as polygons
    // Loop through a second time instead of handling in the first loop
    // because dynamic map/QMP file combos are not the same on all zones
    for(auto zonePair : zoneIDs)
    {
        uint32_t zoneID = zonePair.first;
        auto zoneData = definitionManager->GetZoneData(zoneID);

        for(auto dynamicMapID : zonePair.second)
        {
            auto serverZone = serverDataManager->GetZoneData(zoneID, dynamicMapID);
            if(zoneData && serverZone)
            {
                auto dynamicMap = definitionManager->GetDynamicMapData(dynamicMapID);
                if(dynamicMap && mDynamicMaps.find(dynamicMapID) == mDynamicMaps.end())
                {
                    auto dMap = std::make_shared<DynamicMap>();
                    auto spots = definitionManager->GetSpotData(dynamicMapID);
                    for(auto spotPair : spots)
                    {
                        Point center(spotPair.second->GetCenterX(),
                            spotPair.second->GetCenterY());
                        float rot = spotPair.second->GetRotation();

                        float x1 = center.x - spotPair.second->GetSpanX();
                        float y1 = center.y - spotPair.second->GetSpanY();

                        float x2 = center.x + spotPair.second->GetSpanX();
                        float y2 = center.y + spotPair.second->GetSpanY();

                        // Build the unrotated rectangle
                        std::vector<Point> points;
                        points.push_back(Point(x1, y1));
                        points.push_back(Point(x2, y1));
                        points.push_back(Point(x2, y2));
                        points.push_back(Point(x1, y2));

                        auto shape = std::make_shared<ZoneSpotShape>();

                        // Rotate each point around the center
                        for(auto& p : points)
                        {
                            p = RotatePoint(p, center, rot);
                            shape->Vertices.push_back(p);
                        }

                        shape->Definition = spotPair.second;
                        shape->Lines.push_back(Line(points[0], points[1]));
                        shape->Lines.push_back(Line(points[1], points[2]));
                        shape->Lines.push_back(Line(points[2], points[3]));
                        shape->Lines.push_back(Line(points[3], points[0]));
                    
                        // Determine the boundaries of the completed shape
                        std::list<float> xVals;
                        std::list<float> yVals;

                        for(Line& line : shape->Lines)
                        {
                            for(const Point& p : { line.first, line.second })
                            {
                                xVals.push_back(p.x);
                                yVals.push_back(p.y);
                            }
                        }

                        xVals.sort([](const float& a, const float& b)
                            {
                                return a < b;
                            });

                        yVals.sort([](const float& a, const float& b)
                            {
                                return a < b;
                            });

                        shape->Boundaries[0] = Point(xVals.front(), yVals.front());
                        shape->Boundaries[1] = Point(xVals.back(), yVals.back());

                        dMap->Spots[spotPair.first] = shape;
                        dMap->SpotTypes[spotPair.second->GetType()].push_back(shape);
                    }

                    mDynamicMaps[dynamicMapID] = dMap;
                }
            }
        }
    }
}

void ZoneManager::InstanceGlobalZones()
{
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    auto zoneIDs = serverDataManager->GetAllZoneIDs();
    for(auto zonePair : zoneIDs)
    {
        uint32_t zoneID = zonePair.first;
        for(auto dynamicMapID : zonePair.second)
        {
            auto zoneData = serverDataManager->GetZoneData(zoneID, dynamicMapID);
            if(mZoneMap.find(zoneID) == mZoneMap.end() &&
                mZoneMap[zoneID].find(dynamicMapID) == mZoneMap[zoneID].end() &&
                zoneData->GetGlobal())
            {
                CreateZoneInstance(zoneData);
            }
        }
    }
}

std::shared_ptr<Zone> ZoneManager::GetZoneInstance(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto worldCID = client->GetClientState()->GetWorldCID();
    return GetZoneInstance(worldCID);
}

std::shared_ptr<Zone> ZoneManager::GetZoneInstance(int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mEntityMap.find(worldCID);
    if(iter != mEntityMap.end())
    {
        return mZones[iter->second];
    }

    return nullptr;
}

bool ZoneManager::EnterZone(const std::shared_ptr<ChannelClientConnection>& client,
    uint32_t zoneID, uint32_t dynamicMapID, float xCoord, float yCoord, float rotation,
    bool forceLeave)
{
    auto instance = GetZone(zoneID, dynamicMapID, client);
    if(instance == nullptr)
    {
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto worldCID = state->GetWorldCID();

    auto currentZone = cState->GetZone();
    if(forceLeave || (currentZone && currentZone != instance))
    {
        LeaveZone(client, false, zoneID, dynamicMapID);

        // Pull a fresh version of the zone in case it was cleaned up
        instance = GetZone(zoneID, dynamicMapID, client);
        if(instance == nullptr)
        {
            // Force the client out of any current zone so private
            // instances don't get stuck
            LeaveZone(client, false, 0, 0);
            return false;
        }
    }

    auto instanceID = instance->GetID();
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEntityMap[worldCID] = instanceID;

        // Reactive the zone if its not active already
        mActiveInstances.insert(instanceID);
    }
    instance->AddConnection(client);
    cState->SetZone(instance);
    dState->SetZone(instance);

    auto server = mServer.lock();
    auto ticks = server->GetServerTime();
    auto zoneDef = instance->GetDefinition();

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

    server->GetTokuseiManager()->RecalculateParty(state->GetParty());

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
        // Send normal zone change info
        request.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE);
    }
    request.WriteU32Little(zoneID);
    cLogin->SetZoneID(zoneID);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

    return true;
}

void ZoneManager::LeaveZone(const std::shared_ptr<ChannelClientConnection>& client,
    bool logOut, uint32_t newZoneID, uint32_t newDynamicMapID)
{
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto serverDataManager = server->GetServerDataManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto worldCID = state->GetWorldCID();

    // Detach from zone specific state info
    auto exchangeSession = state->GetExchangeSession();
    if(exchangeSession)
    {
        auto otherCState = std::dynamic_pointer_cast<CharacterState>(
            exchangeSession->GetOtherCharacterState());
        if(otherCState != cState ||
            exchangeSession->GetSourceEntityID() != cState->GetEntityID())
        {
            auto connectionManager = server->GetManagerConnection();
            auto otherClient = connectionManager->GetEntityClient(
                otherCState != cState ? otherCState->GetEntityID()
                : exchangeSession->GetSourceEntityID(), false);

            if(otherClient)
            {
                characterManager->EndExchange(otherClient);
            }
        }

        characterManager->EndExchange(client);
    }

    // Remove any opponents
    characterManager->AddRemoveOpponent(false, cState, nullptr);
    characterManager->AddRemoveOpponent(false, dState, nullptr);

    bool instanceRemoved = false;
    std::shared_ptr<Zone> zone = nullptr;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mEntityMap.find(worldCID);
        if(iter != mEntityMap.end())
        {
            uint32_t instanceID = iter->second;
            zone = mZones[instanceID];

            mEntityMap.erase(worldCID);
            zone->RemoveConnection(client);

            if(zone->GetConnections().size() == 0)
            {
                // Always "freeze" the instance
                mActiveInstances.erase(instanceID);

                auto def = zone->GetDefinition();
                auto nextZone = serverDataManager->GetZoneData(newZoneID,
                    newDynamicMapID);

                // If the current zone is global, the next zone is the same
                // or the next zone is in the same zone group, do not remove it
                bool keepZone = def->GetGlobal() || def == nextZone ||
                    (def && nextZone && !nextZone->GetGlobal() &&
                        def->GetGroupID() == nextZone->GetGroupID());

                std::list<std::shared_ptr<Zone>> cleanupZones;
                cleanupZones.push_back(zone);
                if(!def->GetGlobal() && !keepZone)
                {
                    int32_t ownerID = zone->GetOwnerID();
                    for(uint32_t privateID : mZoneOwnerMap[ownerID])
                    {
                        auto z = mZones[privateID];
                        if(z->GetDefinition()->GetGroupID() == def->GetGroupID())
                        {
                            if(z->GetConnections().size() == 0)
                            {
                                cleanupZones.push_back(z);
                            }
                            else
                            {
                                keepZone = true;
                                break;
                            }
                        }
                    }
                }

                if(!keepZone)
                {
                    for(auto z : cleanupZones)
                    {
                        z->Cleanup();
                        mZones.erase(z->GetID());

                        auto d = z->GetDefinition();
                        auto zoneDefID = d->GetID();
                        auto dynamicMapID = d->GetDynamicMapID();

                        auto& instanceMap = mZoneMap[zoneDefID];
                        auto& instances = instanceMap[dynamicMapID];

                        instances.erase(z->GetID());
                        if(instances.size() == 0)
                        {
                            instanceMap.erase(dynamicMapID);
                            if(instanceMap.size() == 0)
                            {
                                mZoneMap.erase(zoneDefID);
                            }
                            instanceRemoved = true;
                        }

                        int32_t ownerID = z->GetOwnerID();
                        mZoneOwnerMap[ownerID].erase(z->GetID());
                        if(mZoneOwnerMap[ownerID].size() == 0)
                        {
                            mZoneOwnerMap.erase(ownerID);
                        }
                    }
                }
                else
                {
                    // Stop all AI in place
                    uint64_t now = ChannelServer::GetServerTime();
                    for(auto eState : zone->GetEnemies())
                    {
                        eState->Stop(now);
                    }
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
        auto characterID = cState->GetEntityID();
        auto demonID = dState->GetEntityID();
        std::list<int32_t> entityIDs = { characterID, demonID };
        RemoveEntitiesFromZone(zone, entityIDs);
    }

    if(newZoneID == 0)
    {
        // Not entering another zone, recalculate tokusei for
        // remaining party member effects
        server->GetTokuseiManager()->RecalculateParty(
            state->GetParty());
    }

    // If logging out, cancel zone out and log out effects (zone out effects
    // are cancelled on zone enter instead if not logging out)
    if(logOut)
    {
        characterManager->CancelStatusEffects(client, EFFECT_CANCEL_LOGOUT
            | EFFECT_CANCEL_ZONEOUT);
    }

    // Deactivate and save the updated status effects
    cState->SetStatusEffectsActive(false, definitionManager);
    dState->SetStatusEffectsActive(false, definitionManager);
    characterManager->UpdateStatusEffects(cState, !logOut);
    characterManager->UpdateStatusEffects(dState, !logOut);
}

void ZoneManager::SendPopulateZoneData(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();

    auto zone = GetZoneInstance(state->GetWorldCID());
    auto zoneData = zone->GetDefinition();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    
    // Send the new connection entity data to the other clients
    auto otherClients = GetZoneConnections(client, false);
    if(otherClients.size() > 0)
    {
        characterManager->SendOtherCharacterData(otherClients, state);
        if(dState->GetEntity())
        {
            characterManager->SendOtherPartnerData(otherClients, state);
        }
    }

    // The client's partner demon will be shown elsewhere

    PopEntityForZoneProduction(zone, cState->GetEntityID(), 0);
    ShowEntityToZone(zone, cState->GetEntityID());

    // Activate status effects
    cState->SetStatusEffectsActive(true, definitionManager);
    dState->SetStatusEffectsActive(true, definitionManager);

    // Expire zone change status effects
    characterManager->CancelStatusEffects(client, EFFECT_CANCEL_ZONEOUT);

    // It seems that if entity data is sent to the client before a previous
    // entity was processed and shown, the client will force a log-out. To
    // counter-act this, all message information remaining of this type will
    // be queued and sent together at the end.
    for(auto enemyState : zone->GetEnemies())
    {
        SendEnemyData(client, enemyState, zone, false, true);
    }

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

        // If an NPC's state is not 1, do not show it
        if(npc->GetState() == 1)
        {
            ShowEntity(client, npcState->GetEntityID(), true);
        }
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

    for(auto bState : zone->GetBazaars())
    {
        auto bazaar = bState->GetEntity();

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_DATA);
        reply.WriteS32Little(bState->GetEntityID());
        reply.WriteS32Little((int32_t)zone->GetID());
        reply.WriteS32Little((int32_t)zoneData->GetID());
        reply.WriteFloat(bState->GetCurrentX());
        reply.WriteFloat(bState->GetCurrentY());
        reply.WriteFloat(bState->GetCurrentRotation());
        reply.WriteS32Little((int32_t)bazaar->MarketIDsCount());

        for(uint32_t marketID : bazaar->GetMarketIDs())
        {
            auto market = bState->GetCurrentMarket(marketID);
            if(market && market->GetState() ==
                objects::BazaarData::State_t::BAZAAR_INACTIVE)
            {
                market = nullptr;
            }

            reply.WriteU32Little(marketID);
            reply.WriteS32Little(market ? (int32_t)market->GetState() : 0);
            reply.WriteS32Little(market ? market->GetNPCType() : -1);
            reply.WriteString16Little(state->GetClientStringEncoding(),
                market ? market->GetComment() : "", true);
        }

        client->QueuePacket(reply);
        ShowEntity(client, bState->GetEntityID(), true);
    }

    for(auto lState : zone->GetLootBoxes())
    {
        SendLootBoxData(client, lState, nullptr, false, true);
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
            PopEntityForProduction(client, oDemonState->GetEntityID(), 0);
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

void ZoneManager::ShowEntityToZone(const std::shared_ptr<Zone>& zone, int32_t entityID)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_ENTITY);
    p.WriteS32Little(entityID);

    BroadcastPacket(zone, p);
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

void ZoneManager::PopEntityForZoneProduction(const std::shared_ptr<Zone>& zone,
    int32_t entityID, int32_t type)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_POP_ENTITY_FOR_PRODUCTION);
    p.WriteS32Little(entityID);
    p.WriteS32Little(type);

    BroadcastPacket(zone, p);
}

void ZoneManager::RemoveEntitiesFromZone(const std::shared_ptr<Zone>& zone,
    const std::list<int32_t>& entityIDs, int32_t removalMode, bool queue)
{
    auto clients = zone->GetConnectionList();
    RemoveEntities(clients, entityIDs, removalMode, queue);
}

void ZoneManager::RemoveEntities(const std::list <std::shared_ptr<
    ChannelClientConnection>> &clients, const std::list<int32_t>& entityIDs,
    int32_t removalMode, bool queue)
{
    for(int32_t entityID : entityIDs)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_ENTITY);
        p.WriteS32Little(entityID);
        p.WriteS32Little(removalMode);

        for(auto client : clients)
        {
            client->QueuePacketCopy(p);
        }

        p.Clear();
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_OBJECT);
        p.WriteS32Little(entityID);

        for(auto client : clients)
        {
            client->QueuePacketCopy(p);
        }
    }

    if(!queue)
    {
        ChannelClientConnection::FlushAllOutgoing(clients);
    }
}

void ZoneManager::FixCurrentPosition(const std::shared_ptr<ActiveEntityState>& eState,
    uint64_t fixUntil, uint64_t now)
{
    auto zone = eState->GetZone();
    if(zone)
    {
        if(now == 0)
        {
            now = ChannelServer::GetServerTime();
        }

        eState->RefreshCurrentPosition(now);
        eState->Stop(now);

        float x = eState->GetCurrentX();
        float y = eState->GetCurrentY();
        float rot = eState->GetCurrentRotation();


        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FIX_POSITION);
        p.WriteS32Little(eState->GetEntityID());
        p.WriteFloat(x);
        p.WriteFloat(y);
        p.WriteFloat(rot);

        std::unordered_map<uint32_t, uint64_t> timeMap;
        timeMap[16] = now;
        timeMap[20] = fixUntil;

        auto zConnections = zone->GetConnectionList();
        ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
    }
}

void ZoneManager::ScheduleEntityRemoval(uint64_t time, const std::shared_ptr<Zone>& zone,
    const std::list<int32_t>& entityIDs, int32_t removeMode)
{
    mServer.lock()->ScheduleWork(time, [](ZoneManager* zoneManager,
        const std::shared_ptr<Zone> pZone, std::list<int32_t> pEntityIDs,
        int32_t pRemoveMode)
        {
            std::list<int32_t> finalList;
            for(int32_t lootEntityID : pEntityIDs)
            {
                auto state = pZone->GetEntity(lootEntityID);
                if(state)
                {
                    pZone->RemoveEntity(lootEntityID);
                    finalList.push_back(lootEntityID);
                }
            }

            if(finalList.size() > 0)
            {
                zoneManager->RemoveEntitiesFromZone(pZone, finalList, pRemoveMode);
            }
        }, this, zone, entityIDs, removeMode);
}

void ZoneManager::SendLootBoxData(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<LootBoxState>& lState, const std::shared_ptr<EnemyState>& eState,
    bool sendToAll, bool queue)
{
    auto box = lState->GetEntity();
    auto zone = GetZoneInstance(client);
    auto zoneData = zone->GetDefinition();

    libcomp::Packet p;

    auto lootType = box->GetType();
    switch(lootType)
    {
    case objects::LootBox::Type_t::BODY:
        {
            auto enemy = box->GetEnemy();

            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOOT_BODY_DATA);
            p.WriteS32Little(lState->GetEntityID());
            p.WriteS32Little(eState ? eState->GetEntityID() : -1);
            p.WriteS32Little((int32_t)enemy->GetType());
            p.WriteS32Little((int32_t)zone->GetID());
            p.WriteS32Little((int32_t)zone->GetDefinition()->GetID());
            p.WriteFloat(lState->GetCurrentX());
            p.WriteFloat(lState->GetCurrentY());
            p.WriteFloat(lState->GetCurrentRotation());
            p.WriteU32Little(enemy->GetVariantType());
        }
        break;
    case objects::LootBox::Type_t::GIFT_BOX:
    case objects::LootBox::Type_t::EGG:
    case objects::LootBox::Type_t::BOSS_BOX:
    case objects::LootBox::Type_t::TREASURE_BOX:
        {
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOOT_BOX_DATA);
            p.WriteS32Little(lState->GetEntityID());
            p.WriteS32Little(eState ? eState->GetEntityID() : -1);
            p.WriteS8((int8_t)lootType);
            p.WriteS32Little((int32_t)zone->GetID());
            p.WriteS32Little((int32_t)zone->GetDefinition()->GetID());
            p.WriteFloat(lState->GetCurrentX());
            p.WriteFloat(lState->GetCurrentY());
            p.WriteFloat(lState->GetCurrentRotation());
            p.WriteFloat(0.f);  // Unknown
        }
        break;
    default:
        return;
    }

    std::list<std::shared_ptr<ChannelClientConnection>> clients;
    if(sendToAll)
    {
        clients = zone->GetConnectionList();
    }
    else
    {
        clients.push_back(client);
    }

    // Send the data and prepare it to show
    for(auto zClient : clients)
    {
        zClient->QueuePacketCopy(p);
        PopEntityForProduction(zClient, lState->GetEntityID(), 0, true);
    }

    // Send the loot data if it exists (except for treasure chests)
    if(lootType != objects::LootBox::Type_t::BOSS_BOX &&
        lootType != objects::LootBox::Type_t::TREASURE_BOX)
    {
        for(auto loot : box->GetLoot())
        {
            if(loot)
            {
                auto characterManager = mServer.lock()->GetCharacterManager();
                characterManager->SendLootItemData(clients, lState, true);
                break;
            }
        }
    }

    // Show the box
    for(auto zClient : clients)
    {
        ShowEntity(zClient, lState->GetEntityID(), true);
    }

    if(!queue)
    {
        ChannelClientConnection::FlushAllOutgoing(clients);
    }
}

void ZoneManager::SendBazaarMarketData(const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<BazaarState>& bState, uint32_t marketID)
{
    auto market = bState->GetCurrentMarket(marketID);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_NPC_CHANGED);
    p.WriteS32Little(bState->GetEntityID());
    p.WriteS32Little((int32_t)marketID);
    p.WriteS32Little(market ? (int32_t)market->GetNPCType() : -1);
    p.WriteS32Little(market ? 1 : 0); // State: 0 = vacant, 1 = ready, 2 = pending?
    p.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        market ? market->GetComment() : "", true);

    BroadcastPacket(zone, p);
}

void ZoneManager::ExpireBazaarMarkets(const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<BazaarState>& bState)
{
    auto server = mServer.lock();

    uint32_t now = (uint32_t)std::time(0);
    uint32_t currentExpiration = bState->GetNextExpiration();

    std::list<std::shared_ptr<objects::BazaarData>> expired;
    for(uint32_t marketID : bState->GetEntity()->GetMarketIDs())
    {
        auto market = bState->GetCurrentMarket(marketID);
        if(market && market->GetExpiration() <= now)
        {
            market->SetState(objects::BazaarData::State_t::BAZAAR_INACTIVE);
            bState->SetCurrentMarket(marketID, nullptr);

            // Send the close notification
            auto sellerAccount = market->GetAccount().Get();
            auto sellerClient = sellerAccount ? server->GetManagerConnection()
                ->GetClientConnection(sellerAccount->GetUsername()) : nullptr;

            libcomp::Packet p;
            if(sellerClient == nullptr)
            {
                // Relay the packet through the world
                p.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
                p.WriteS32Little(0);
                p.WriteU8((uint8_t)PacketRelayMode_t::RELAY_ACCOUNT);
                p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                    market->GetAccount().GetUUID().ToString(), true);
            }

            p.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_CLOSE);
            p.WriteS32Little(0);

            if(sellerClient)
            {
                // Send directly
                sellerClient->SendPacket(p);
            }
            else
            {
                // Relay
                server->GetManagerConnection()->GetWorldConnection()->SendPacket(p);
            }

            SendBazaarMarketData(zone, bState, marketID);

            expired.push_back(market);
        }
    }

    if(expired.size() > 0)
    {
        auto dbChanges = libcomp::DatabaseChangeSet::Create();
        for(auto market : expired)
        {
            dbChanges->Update(market);
        }

        server->GetWorldDatabase()->QueueChangeSet(dbChanges);
    }

    uint32_t nextExpiration = bState->SetNextExpiration();
    if(nextExpiration != 0 && nextExpiration != currentExpiration)
    {
        // If the next run is sooner than what is scheduled, schedule again
        ServerTime nextTime = ChannelServer::GetServerTime() +
            ((uint64_t)(nextExpiration - now) * 1000000ULL);

        server->ScheduleWork(nextTime, [](ZoneManager* zoneManager,
            const std::shared_ptr<Zone> pZone,
            const std::shared_ptr<BazaarState>& pState)
            {
                zoneManager->ExpireBazaarMarkets(pZone, pState);
            }, this, zone, bState);
    }
}

void ZoneManager::SendEnemyData(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<EnemyState>& enemyState, const std::shared_ptr<Zone>& zone,
    bool sendToAll, bool queue)
{
    auto stats = enemyState->GetCoreStats();
    auto zoneData = zone->GetDefinition();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENEMY_DATA);
    p.WriteS32Little(enemyState->GetEntityID());
    p.WriteS32Little((int32_t)enemyState->GetEntity()->GetType());
    p.WriteS32Little(enemyState->GetMaxHP());
    p.WriteS32Little(stats->GetHP());
    p.WriteS8(stats->GetLevel());
    p.WriteS32Little((int32_t)zone->GetID());
    p.WriteS32Little((int32_t)zoneData->GetID());
    p.WriteFloat(enemyState->GetOriginX());
    p.WriteFloat(enemyState->GetOriginY());
    p.WriteFloat(enemyState->GetOriginRotation());
    
    auto statusEffects = enemyState->GetCurrentStatusEffectStates(
        mServer.lock()->GetDefinitionManager());

    p.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
    for(auto ePair : statusEffects)
    {
        p.WriteU32Little(ePair.first->GetEffect());
        p.WriteS32Little((int32_t)ePair.second);
        p.WriteU8(ePair.first->GetStack());
    }

    p.WriteU32Little(enemyState->GetEntity()->GetVariantType());

    std::list<std::shared_ptr<ChannelClientConnection>> clients;
    if(sendToAll)
    {
        clients = zone->GetConnectionList();
    }
    else
    {
        clients.push_back(client);
    }

    for(auto zClient : clients)
    {
        zClient->QueuePacketCopy(p);
        PopEntityForProduction(zClient, enemyState->GetEntityID(), 3, true);
        ShowEntity(zClient, enemyState->GetEntityID(), true);
    }

    if(!queue)
    {
        ChannelClientConnection::FlushAllOutgoing(clients);
    }
}

void ZoneManager::UpdateStatusEffectStates(const std::shared_ptr<Zone>& zone,
    uint32_t now)
{
    auto effectEntities = zone->GetUpdatedStatusEffectEntities(now);
    if(effectEntities.size() == 0)
    {
        return;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto characterManager = server->GetCharacterManager();

    std::list<libcomp::Packet> zonePackets;
    std::set<uint32_t> added, updated, removed;
    std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
    std::set<std::shared_ptr<ActiveEntityState>> statusRemoved;
    for(auto entity : effectEntities)
    {
        int32_t hpTDamage, mpTDamage;
        if(!entity->PopEffectTicks(definitionManager, now, hpTDamage,
            mpTDamage, added, updated, removed)) continue;

        if(added.size() > 0 || updated.size() > 0)
        {
            uint32_t missing = 0;
            auto effectMap = entity->GetStatusEffects();
            for(uint32_t effectType : added)
            {
                if(effectMap.find(effectType) == effectMap.end())
                {
                    missing++;
                }
            }
            
            for(uint32_t effectType : updated)
            {
                if(effectMap.find(effectType) == effectMap.end())
                {
                    missing++;
                }
            }

            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ADD_STATUS_EFFECT);
            p.WriteS32Little(entity->GetEntityID());
            p.WriteU32Little((uint32_t)(added.size() + updated.size() - missing));

            for(uint32_t effectType : added)
            {
                auto effect = effectMap[effectType];
                if(effect)
                {
                    p.WriteU32Little(effectType);
                    p.WriteS32Little((int32_t)effect->GetExpiration());
                    p.WriteU8(effect->GetStack());
                }
            }
            
            for(uint32_t effectType : updated)
            {
                auto effect = effectMap[effectType];
                if(effect)
                {
                    p.WriteU32Little(effectType);
                    p.WriteS32Little((int32_t)effect->GetExpiration());
                    p.WriteU8(effect->GetStack());
                }
            }

            zonePackets.push_back(p);
        }

        if(hpTDamage != 0 || mpTDamage != 0)
        {
            auto cs = entity->GetCoreStats();
            int32_t hpAdjusted, mpAdjusted;
            if(entity->SetHPMP(-hpTDamage, -mpTDamage, true,
                false, hpAdjusted, mpAdjusted))
            {
                if(hpAdjusted < 0)
                {
                    entity->CancelStatusEffects(EFFECT_CANCEL_DAMAGE);
                }
                displayStateModified.insert(entity);

                libcomp::Packet p;
                p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DO_TDAMAGE);
                p.WriteS32Little(entity->GetEntityID());
                p.WriteS32Little(hpAdjusted);
                p.WriteS32Little(mpAdjusted);
                zonePackets.push_back(p);

                server->GetTokuseiManager()->Recalculate(entity,
                    std::set<TokuseiConditionType>
                    { TokuseiConditionType::CURRENT_HP,
                      TokuseiConditionType::CURRENT_MP });
            }
        }
        
        if(removed.size() > 0)
        {
            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_STATUS_EFFECT);
            p.WriteS32Little(entity->GetEntityID());
            p.WriteU32Little((uint32_t)removed.size());
            for(uint32_t effectType : removed)
            {
                p.WriteU32Little(effectType);
            }
            zonePackets.push_back(p);

            statusRemoved.insert(entity);
        }
    }

    if(zonePackets.size() > 0)
    {
        auto zConnections = zone->GetConnectionList();
        ChannelClientConnection::BroadcastPackets(zConnections, zonePackets);
    }

    for(auto entity : statusRemoved)
    {
        // Make sure T-damage is sent first
        // Status add/update and world update handled when applying changes
        server->GetTokuseiManager()->Recalculate(entity, true,
            std::set<int32_t>{ entity->GetEntityID() });
        if(characterManager->RecalculateStats(nullptr, entity->GetEntityID()) &
            ENTITY_CALC_STAT_WORLD)
        {
            displayStateModified.erase(entity);
        }
    }
    
    if(displayStateModified.size() > 0)
    {
        characterManager->UpdateWorldDisplayState(displayStateModified);
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

void ZoneManager::SendToRange(const std::shared_ptr<ChannelClientConnection>& client,
    libcomp::Packet& p, bool includeSelf)
{
    uint64_t now = mServer.lock()->GetServerTime();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    cState->RefreshCurrentPosition(now);

    std::list<std::shared_ptr<libcomp::TcpConnection>> zConnections;
    if(includeSelf)
    {
        zConnections.push_back(client);
    }

    float rSquared = (float)std::pow(CHAT_RADIUS_SAY, 2);
    for(auto zConnection : GetZoneConnections(client, false))
    {
        auto otherCState = zConnection->GetClientState()->GetCharacterState();
        otherCState->RefreshCurrentPosition(now);

        if(rSquared >= cState->GetDistance(otherCState->GetCurrentX(),
            otherCState->GetCurrentY(), true))
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

    auto worldCID = client->GetClientState()->GetWorldCID();
    std::shared_ptr<Zone> zone;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mEntityMap.find(worldCID);
        if(iter != mEntityMap.end())
        {
            zone = mZones[iter->second];
        }
    }

    if(nullptr != zone)
    {
        for(auto connectionPair : zone->GetConnections())
        {
            if(includeSelf || connectionPair.first != worldCID)
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
    auto eState = CreateEnemy(zone, demonID, 0, x, y, rot);

    auto server = mServer.lock();
    server->GetAIManager()->Prepare(eState, aiType);
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

bool ZoneManager::UpdateSpawnGroups(const std::shared_ptr<Zone>& zone,
    bool refreshAll, uint64_t now, std::shared_ptr<objects::ActionSpawn> actionSource)
{
    std::unordered_map<uint32_t, uint16_t> updateSpawnGroups;
    if(!refreshAll)
    {
        if(now == 0)
        {
            now = ChannelServer::GetServerTime();
        }

        updateSpawnGroups = zone->GetReinforceableSpawnGroups(now);
        if(updateSpawnGroups.size() == 0)
        {
            return false;
        }
    }

    std::set<uint32_t> groupIDs;
    if(actionSource)
    {
        for(uint32_t groupID : actionSource->GetSpawnGroupIDs())
        {
            switch(actionSource->GetConditions())
            {
            case objects::ActionSpawn::Conditions_t::ONE_TIME:
                if(!zone->GroupHasSpawned(groupID, false))
                {
                    groupIDs.insert(groupID);
                }
                break;
            case objects::ActionSpawn::Conditions_t::ONE_TIME_RANDOM:
                if(!zone->GroupHasSpawned(groupID, false))
                {
                    groupIDs.insert(groupID);
                }
                else
                {
                    // Stop here if any have spawned
                    return false;
                }
                break;
            case objects::ActionSpawn::Conditions_t::NONE_EXIST:
                if(!zone->GroupHasSpawned(groupID, true))
                {
                    groupIDs.insert(groupID);
                }
                break;
            case objects::ActionSpawn::Conditions_t::NONE:
            default:
                groupIDs.insert(groupID);
                break;
            }
        }

        if(actionSource->GetConditions() ==
            objects::ActionSpawn::Conditions_t::ONE_TIME_RANDOM &&
            groupIDs.size() > 1)
        {
            auto it = groupIDs.begin();

            size_t randomIdx = (size_t)RNG(int32_t, 0,
                (int32_t)(groupIDs.size()-1));
            std::advance(it, randomIdx);

            uint32_t groupID = *it;
            groupIDs.clear();
            groupIDs.insert(groupID);
        }
    }

    auto dynamicMap = zone->GetDynamicMap();
    auto zoneDef = zone->GetDefinition();

    std::unordered_map<uint32_t,
        std::list<std::shared_ptr<objects::SpawnGroup>>> groups;
    for(auto sgPair : zoneDef->GetSpawnGroups())
    {
        bool specified = groupIDs.find(sgPair.first) != groupIDs.end();

        auto sg = sgPair.second;
        if(specified || (!actionSource && refreshAll && sg->GetRespawnTime() > 0.f) ||
            updateSpawnGroups.find(sgPair.first) != updateSpawnGroups.end())
        {
            groups[sg->GetSpawnLocationGroupID()].push_back(sg);
        }
    }

    std::list<std::shared_ptr<EnemyState>> eStates;
    for(auto groupPair : groups)
    {
        auto slg = zoneDef->GetSpawnLocationGroups(groupPair.first);
        if(!slg)
        {
            LOG_WARNING(libcomp::String("Skipping invalid spawn location"
                " group %1\n")
                .Arg(groupPair.first));
            continue;
        }

        std::set<uint32_t> spotIDs = slg->GetSpotIDs();
        auto locations = slg->GetLocations();

        bool useSpotID = dynamicMap && spotIDs.size() > 0;

        if(!useSpotID && locations.size() == 0) continue;

        // Create each enemy at a random location in the group
        for(auto sg : groupPair.second)
        {
            std::list<std::shared_ptr<objects::SpawnGroup>> groupSet;
            if(sg->SubGroupsCount() == 0)
            {
                // Normal group
                groupSet.push_back(sg);
            }
            else
            {
                // Spawn group set, add sub-groups
                if(sg->GetRespawnTime() > 0)
                {
                    // Not allowed
                    /// @todo: move to zone loading check
                    LOG_ERROR(libcomp::String("Skipping spawn group %1 which contains"
                        " sub-groups and has a respawn time which is not supported\n")
                        .Arg(sg->GetID()));
                }
                else
                {
                    for(uint32_t subGroupID : sg->GetSubGroups())
                    {
                        auto sg2 = zoneDef->GetSpawnGroups(subGroupID);
                        if(sg2 && sg2->GetSpawnID())
                        {
                            groupSet.push_back(sg2);
                        }
                    }
                }
            }
            
            for(auto sge : groupSet)
            {
                auto spawn = zoneDef->GetSpawns(sge->GetSpawnID());

                uint16_t count = refreshAll ? sge->GetMaxCount() : 1;
                for(uint16_t i = 0; i < count; i++)
                {
                    uint32_t spotID = 0;
                    std::shared_ptr<objects::SpawnLocation> location;

                    float x = 0.f, y = 0.f;
                    if(useSpotID)
                    {
                        auto spotIter = spotIDs.begin();
                        if(spotIDs.size() > 1)
                        {
                            size_t randomIdx = (size_t)RNG(int32_t, 0,
                                (int32_t)(spotIDs.size()-1));
                            std::advance(spotIter, randomIdx);
                        }

                        spotID = *spotIter;

                        auto spot = dynamicMap->Spots.find(spotID);
                        if(spot != dynamicMap->Spots.end())
                        {
                            // Get a random point in the polygon
                            Point p = GetRandomSpotPoint(spot->second->Definition);
                            Point center(spot->second->Definition->GetCenterX(),
                                spot->second->Definition->GetCenterY());

                            // Make sure a straight line can be drawn from the center
                            // point so the enemy is not spawned outside of the zone
                            Point collision;
                            Line fromCenter(center, p);
                            if(zone->GetGeometry()->Collides(fromCenter, collision))
                            {
                                // Back it off slightly
                                p = GetLinearPoint(collision.x, collision.y,
                                    center.x, center.y, 10.f, false);
                            }

                            x = p.x;
                            y = p.y;

                            // If the spot is defined with a spawn area, use that as
                            // the AI wandering region
                            auto serverSpot = zoneDef->GetSpots(spotID);
                            if(serverSpot)
                            {
                                location = serverSpot->GetSpawnArea();
                            }
                        }
                        else
                        {
                            LOG_ERROR(libcomp::String("Failed to spawn %1 at unknown"
                                " spot %2\n").Arg(spawn->GetID()).Arg(spotID));
                            return false;
                        }
                    }
                    else
                    {
                        auto locIter = locations.begin();
                        if(locations.size() > 1)
                        {
                            size_t randomIdx = (size_t)RNG(int32_t, 0,
                                (int32_t)(locations.size()-1));
                            std::advance(locIter, randomIdx);
                        }

                        location = *locIter;

                        // Spawn location bounding box points start in the top left corner of the
                        // rectangle and extend towards +X/-Y
                        auto rPoint = GetRandomPoint(location->GetWidth(), location->GetHeight());
                        x = location->GetX() + rPoint.x;
                        y = location->GetY() - rPoint.y;
                    }

                    float rot = RNG_DEC(float, 0.f, 3.14f, 2);

                    // Create the enemy state
                    auto eState = CreateEnemy(zone, spawn->GetEnemyType(),
                        spawn->GetVariantType(), x, y, rot);

                    // Set the spawn information
                    auto enemy = eState->GetEntity();
                    enemy->SetSpawnSource(spawn);
                    enemy->SetSpawnLocation(location);
                    enemy->SetSpawnSpotID(spotID);

                    // Use the effective spawn group, not the actual if they differ
                    enemy->SetSpawnGroupID(sg->GetID());

                    eStates.push_back(eState);
                }
            }
        }
    }

    if(eStates.size() > 0)
    {
        bool setActionSource = actionSource && actionSource->DefeatActionsCount() > 0;

        auto server = mServer.lock();
        auto aiManager = server->GetAIManager();
        for(auto eState : eStates)
        {
            if(aiManager->Prepare(eState))
            {
                /// @todo: change this for enemies that don't wander
                eState->GetAIState()->SetStatus(AIStatus_t::WANDERING, true);
            }

            if(!setActionSource)
            {
                zone->AddEnemy(eState);
            }
        }

        if(setActionSource)
        {
            zone->CreateEncounter(eStates, actionSource);
        }

        // Send to clients already in the zone if they exist
        auto clients = zone->GetConnections();
        if(clients.size() > 0)
        {
            auto firstClient = clients.begin()->second;
            for(auto eState : eStates)
            {
                SendEnemyData(firstClient, eState, zone, true, true);
            }

            for(auto client : clients)
            {
                client.second->FlushOutgoing();
            }
        }

        return true;
    }

    return false;
}

Point ZoneManager::RotatePoint(const Point& p, const Point& origin, float radians)
{
    float xDelta = p.x - origin.x;
    float yDelta = p.y - origin.y;

    return Point((float)((xDelta * cos(radians)) - (yDelta * sin(radians))) + origin.x,
        (float)((xDelta * sin(radians)) + (yDelta * cos(radians))) + origin.y);
}

std::shared_ptr<EnemyState> ZoneManager::CreateEnemy(
    const std::shared_ptr<Zone>& zone, uint32_t demonID, uint32_t variantType,
    float x, float y, float rot)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetDevilData(demonID);

    if(nullptr == def)
    {
        return nullptr;
    }

    auto enemy = std::shared_ptr<objects::Enemy>(new objects::Enemy);
    enemy->SetType(demonID);
    enemy->SetVariantType(variantType);

    auto enemyStats = libcomp::PersistentObject::New<objects::EntityStats>();
    enemyStats->SetLevel((int8_t)def->GetGrowth()->GetBaseLevel());
    server->GetCharacterManager()->CalculateDemonBaseStats(nullptr, enemyStats, def);
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
    eState->SetStatusEffectsActive(true, definitionManager);
    eState->SetZone(zone);

    server->GetTokuseiManager()->Recalculate(eState);
    eState->RecalculateStats(definitionManager);

    // Reset HP to max to account for extra HP boosts
    enemyStats->SetHP(eState->GetMaxHP());

    return eState;
}

void ZoneManager::UpdateActiveZoneStates()
{
    std::list<std::shared_ptr<Zone>> instances;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto instanceID : mActiveInstances)
        {
            instances.push_back(mZones[instanceID]);
        }
    }

    // Spin through entities with updated status effects
    uint32_t systemTime = (uint32_t)std::time(0);
    for(auto instance : instances)
    {
        UpdateStatusEffectStates(instance, systemTime);
    }

    auto serverTime = ChannelServer::GetServerTime();
    auto aiManager = mServer.lock()->GetAIManager();

    for(auto instance : instances)
    {
        // Update active AI controlled entities
        aiManager->UpdateActiveStates(instance, serverTime);

        // Spawn new enemies next (since they should not immediately act)
        UpdateSpawnGroups(instance, false, serverTime);
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

    std::unordered_map<uint32_t, uint64_t> timeMap;
    timeMap[p.Size()] = timestamp;

    auto connections = server->GetZoneManager()->GetZoneConnections(client, true);
    ChannelClientConnection::SendRelativeTimePacket(connections, p, timeMap);
}

Point ZoneManager::GetRandomPoint(float width, float height) const
{
    return Point(RNG_DEC(float, 0.f, (float)fabs(width), 2),
        RNG_DEC(float, 0.f, (float)fabs(height), 2));
}

Point ZoneManager::GetRandomSpotPoint(const std::shared_ptr<objects::MiSpotData>& spot) const
{
    Point untransformed = GetRandomPoint(spot->GetSpanX() * 2.f,
        spot->GetSpanY() * 2.f);
    untransformed.x += spot->GetCenterX() - spot->GetSpanX();
    untransformed.y += spot->GetCenterY() - spot->GetSpanY();

    return spot->GetRotation() != 0.f
        ? RotatePoint(untransformed, Point(spot->GetCenterX(), spot->GetCenterY()),
            spot->GetRotation())
        : untransformed;
}

Point ZoneManager::GetLinearPoint(float sourceX, float sourceY,
    float targetX, float targetY, float distance, bool away)
{
    Point dest(sourceX, sourceY);
    if(targetX != sourceX)
    {
        float slope = (targetY - sourceY)/(targetX - sourceX);
        float denom = (float)std::sqrt(1.0f + std::pow(slope, 2));

        float xOffset = (float)(distance / denom);
        float yOffset = (float)fabs((slope * distance) / denom);

        dest.x = (away == (targetX > sourceX))
            ? (sourceX - xOffset) : (sourceX + xOffset);
        dest.y = (away == (targetY > sourceY))
            ? (sourceY - yOffset) : (sourceY + yOffset);
    }
    else if(targetY != sourceY)
    {
        float yOffset = (float)((targetY - sourceY)/distance);

        dest.y = (away == (targetY > sourceY))
            ? (sourceY - yOffset) : (sourceY + yOffset);
    }

    return dest;
}

Point ZoneManager::MoveRelative(const std::shared_ptr<ActiveEntityState>& eState,
    float targetX, float targetY, float distance, bool away,
    uint64_t now, uint64_t endTime)
{
    float x = eState->GetCurrentX();
    float y = eState->GetCurrentY();

    auto point = GetLinearPoint(x, y, targetX, targetY, distance, away);

    if(point.x != x || point.y != y)
    {
        // Check collision and adjust
        Line move(x, y, point.x, point.y);

        Point corrected;
        auto geometry = eState->GetZone()->GetGeometry();
        if(geometry && geometry->Collides(move, corrected))
        {
            // Move off the collision point by 10
            point = GetLinearPoint(corrected.x, corrected.y, x, y, 10.f, false);
        }

        eState->SetOriginX(x);
        eState->SetOriginY(y);
        eState->SetOriginTicks(now);

        eState->SetDestinationX(point.x);
        eState->SetDestinationY(point.y);
        eState->SetDestinationTicks(endTime);
    }

    return point;
}

bool ZoneManager::PointInPolygon(const Point& p, const std::list<Point> vertices)
{
    auto p1 = vertices.begin();
    auto p2 = vertices.begin();
    p2++;

    uint32_t crosses = 0;
    size_t count = vertices.size();
    for(size_t i = 0; i < count; i++)
    {
        // Check if the point is on the vertex
        if(p.x == p1->x && p.y == p2->y)
        {
            return true;
        }

        if(((p1->y >= p.y) != (p2->y >= p.y)) &&
            (p.x <= (p2->x - p1->x) * (p.y - p1->y) /
                (p2->y - p1->y) + p1->x))
        {
            crosses++;
        }

        p1++;
        p2++;

        if(p2 == vertices.end())
        {
            // One left, loop back to the start
            p2 = vertices.begin();
        }
    }

    return (crosses % 2) == 1;
}

std::list<std::shared_ptr<ActiveEntityState>> ZoneManager::GetEntitiesInFoV(
    const std::list<std::shared_ptr<ActiveEntityState>>& entities,
    float x, float y, float rot, float maxAngle)
{
    std::list<std::shared_ptr<ActiveEntityState>> results;

    // Max and min radians of the arc's circle
    float maxRotL = rot + maxAngle;
    float maxRotR = rot - maxAngle;

    for(auto e : entities)
    {
        float eRot = (float)atan2((float)(y - e->GetCurrentY()),
            (float)(x - e->GetCurrentX()));

        if(maxRotL >= eRot && maxRotR <= eRot)
        {
            results.push_back(e);
        }
    }

    return results;
}

std::shared_ptr<Zone> ZoneManager::GetZone(uint32_t zoneID,
    uint32_t dynamicMapID, const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto party = state->GetParty();

    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    std::set<int32_t> validOwnerIDs = { state->GetWorldCID() };
    if(party)
    {
        for(int32_t memberID : party->GetMemberIDs())
        {
            validOwnerIDs.insert(memberID);
        }
    }

    std::shared_ptr<Zone> zone;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mZoneMap.find(zoneID);
        if(iter != mZoneMap.end())
        {
            for(auto dPair : iter->second)
            {
                // If dynamicMapID is 0, check all valid instances and take the first
                // one that applies
                if(dynamicMapID == 0 || dPair.first == dynamicMapID)
                {
                    auto zoneDefinition = serverDataManager->GetZoneData(zoneID, dPair.first);
                    for(uint32_t instanceID : dPair.second)
                    {
                        auto instance = mZones[instanceID];
                        if(instance && (zoneDefinition->GetGlobal() ||
                            validOwnerIDs.find(instance->GetOwnerID()) != validOwnerIDs.end()))
                        {
                            zone = instance;
                            break;
                        }
                    }
                }
            }
        }
    }

    if(nullptr == zone)
    {
        auto zoneDefinition = serverDataManager->GetZoneData(zoneID, dynamicMapID);
        zone = CreateZoneInstance(zoneDefinition, state->GetWorldCID());
    }

    return zone;
}

std::shared_ptr<Zone> ZoneManager::CreateZoneInstance(
    const std::shared_ptr<objects::ServerZone>& definition, int32_t ownerID)
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
    auto definitionManager = server->GetDefinitionManager();
    auto zoneData = definitionManager->GetZoneData(definition->GetID());

    auto zone = std::shared_ptr<Zone>(new Zone(id, definition));

    auto qmpFile = zoneData->GetFile()->GetQmpFile();
    auto geoIter = !qmpFile.IsEmpty()
        ? mZoneGeometry.find(qmpFile.C()) : mZoneGeometry.end();
    if(geoIter != mZoneGeometry.end())
    {
        zone->SetGeometry(geoIter->second);
    }

    auto it = mDynamicMaps.find(definition->GetDynamicMapID());
    if(it != mDynamicMaps.end())
    {
        zone->SetDynamicMap(it->second);
    }

    for(auto npc : definition->GetNPCs())
    {
        auto copy = std::make_shared<objects::ServerNPC>(*npc);

        auto state = std::shared_ptr<NPCState>(new NPCState(copy));
        state->SetCurrentX(npc->GetX());
        state->SetCurrentY(npc->GetY());
        state->SetCurrentRotation(npc->GetRotation());
        state->SetEntityID(server->GetNextEntityID());
        state->SetActions(npc->GetActions());
        zone->AddNPC(state);
    }

    for(auto obj : definition->GetObjects())
    {
        auto copy = std::make_shared<objects::ServerObject>(*obj);

        auto state = std::shared_ptr<ServerObjectState>(
            new ServerObjectState(copy));
        state->SetCurrentX(obj->GetX());
        state->SetCurrentY(obj->GetY());
        state->SetCurrentRotation(obj->GetRotation());
        state->SetEntityID(server->GetNextEntityID());
        state->SetActions(obj->GetActions());
        zone->AddObject(state);
    }

    if(definition->BazaarsCount() > 0)
    {
        std::list<std::shared_ptr<objects::BazaarData>> activeMarkets;
        for(auto m : objects::BazaarData::LoadBazaarDataListByZone(
            server->GetWorldDatabase(), definition->GetID()))
        {
            if(m->GetState() == objects::BazaarData::State_t::BAZAAR_ACTIVE)
            {
                activeMarkets.push_back(m);
            }
        }

        for(auto bazaar : definition->GetBazaars())
        {
            auto state = std::shared_ptr<BazaarState>(new BazaarState(bazaar));
            state->SetCurrentX(bazaar->GetX());
            state->SetCurrentY(bazaar->GetY());
            state->SetCurrentRotation(bazaar->GetRotation());
            state->SetEntityID(server->GetNextEntityID());

            for(auto m : activeMarkets)
            {
                if(bazaar->MarketIDsContains(m->GetMarketID()))
                {
                    state->SetCurrentMarket(m->GetMarketID(), m);
                }
            }

            zone->AddBazaar(state);

            ExpireBazaarMarkets(zone, state);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mLock);
        mZones[id] = zone;
        mZoneMap[definition->GetID()][definition->GetDynamicMapID()].insert(id);

        if(ownerID > 0)
        {
            zone->SetOwnerID(ownerID);
            mZoneOwnerMap[ownerID].insert(id);
        }
    }

    // Run all setup actions
    if(definition->SetupActionsCount() > 0)
    {
        auto actionManager = server->GetActionManager();
        actionManager->PerformActions(nullptr,
            definition->GetSetupActions(), 0, zone);
    }

    // Populate all spawnpoints
    UpdateSpawnGroups(zone, true);

    return zone;
}
