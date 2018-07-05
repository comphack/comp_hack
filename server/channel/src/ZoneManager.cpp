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

#include "ZoneManager.h"

// libcomp Includes
#include <Constants.h>
#include <DefinitionManager.h>
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ScriptEngine.h>
#include <ServerConstants.h>
#include <ServerDataManager.h>

// objects Include
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <ActionSpawn.h>
#include <Ally.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <Enemy.h>
#include <EntityStats.h>
#include <Item.h>
#include <ItemDrop.h>
#include <LootBox.h>
#include <MiAIData.h>
#include <MiDevilData.h>
#include <MiDynamicMapData.h>
#include <MiGrowthData.h>
#include <MiSpotData.h>
#include <MiTimeLimitData.h>
#include <MiZoneData.h>
#include <MiZoneFileData.h>
#include <MiONPCData.h>
#include <Party.h>
#include <PlasmaSpawn.h>
#include <PlayerExchangeSession.h>
#include <QmpBoundary.h>
#include <QmpBoundaryLine.h>
#include <QmpElement.h>
#include <QmpFile.h>
#include <ServerBazaar.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <ServerZoneInstanceVariant.h>
#include <ServerZonePartial.h>
#include <ServerZoneSpot.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocation.h>
#include <SpawnLocationGroup.h>
#include <SpawnRestriction.h>

// channel Includes
#include "ActionManager.h"
#include "AIManager.h"
#include "AIState.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "PlasmaState.h"
#include "TokuseiManager.h"
#include "Zone.h"
#include "ZoneInstance.h"

// C++ Standard Includes
#include <cmath>

using namespace channel;

ZoneManager::ZoneManager(const std::weak_ptr<ChannelServer>& server)
    : mNextZoneID(1), mNextZoneInstanceID(1), mServer(server)
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

        auto geometry = std::make_shared<ZoneGeometry>();
        geometry->QmpFilename = filename;

        std::unordered_map<uint32_t,
            std::shared_ptr<objects::QmpElement>> elementMap;
        for(auto qmpElem : qmpFile->GetElements())
        {
            geometry->Elements.push_back(qmpElem);
            elementMap[qmpElem->GetID()] = qmpElem;
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

        uint32_t instanceID = 1;
        for(auto pair : lineMap)
        {
            auto shape =  std::make_shared<ZoneQmpShape>();
            shape->ShapeID = pair.first;
            shape->Element = elementMap[pair.first];
            shape->OneWay = shape->Element->GetType() ==
                objects::QmpElement::Type_t::ONE_WAY;

            // Build a complete shape from the lines provided
            // If there is a gap in the shape, it is a line instead
            // of a full shape
            auto lines = pair.second;

            shape->Lines.push_back(lines.front());
            lines.pop_front();
            Line firstLine = shape->Lines.front();

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
                        if(shape->OneWay)
                        {
                            LOG_DEBUG(libcomp::String("Inverted one way"
                                " directional line encountered in shape:"
                                " %1\n").Arg(shape->Element->GetName()));
                        }

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
                        shape->Element = elementMap[pair.first];
                        shape->OneWay = shape->Element->GetType() ==
                            objects::QmpElement::Type_t::ONE_WAY;

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
            auto zoneData = serverDataManager->GetZoneData(zoneID, dynamicMapID, true);
            if(zoneData->GetGlobal() &&
                (mGlobalZoneMap.find(zoneID) == mGlobalZoneMap.end() ||
                mGlobalZoneMap[zoneID].find(dynamicMapID) == mGlobalZoneMap[zoneID].end()))
            {
                std::lock_guard<std::mutex> lock(mLock);

                auto zone = CreateZone(zoneData);
                mGlobalZoneMap[zoneID][dynamicMapID] = zone->GetID();
            }
        }
    }
}

std::shared_ptr<Zone> ZoneManager::GetCurrentZone(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto worldCID = client->GetClientState()->GetWorldCID();
    return GetCurrentZone(worldCID);
}

std::shared_ptr<Zone> ZoneManager::GetCurrentZone(int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mEntityMap.find(worldCID);
    if(iter != mEntityMap.end())
    {
        return mZones[iter->second];
    }

    return nullptr;
}

std::shared_ptr<Zone> ZoneManager::GetGlobalZone(uint32_t zoneID,
    uint32_t dynamicMapID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mGlobalZoneMap.find(zoneID);
    if(iter != mGlobalZoneMap.end())
    {
        auto subIter = iter->second.find(dynamicMapID);
        if(subIter != iter->second.end())
        {
            return mZones[subIter->second];
        }
    }

    return nullptr;
}

bool ZoneManager::EnterZone(const std::shared_ptr<ChannelClientConnection>& client,
    uint32_t zoneID, uint32_t dynamicMapID)
{
    auto zoneDef = mServer.lock()->GetServerDataManager()->GetZoneData(zoneID,
        dynamicMapID);

    float xCoord = zoneDef ? zoneDef->GetStartingX() : 0.f;
    float yCoord = zoneDef ? zoneDef->GetStartingY() : 0.f;
    float rot = zoneDef ? zoneDef->GetStartingRotation() : 0.f;

    return EnterZone(client, zoneID, zoneDef ? zoneDef->GetDynamicMapID() : 0,
        xCoord, yCoord, rot, true);
}

bool ZoneManager::EnterZone(const std::shared_ptr<ChannelClientConnection>& client,
    uint32_t zoneID, uint32_t dynamicMapID, float xCoord, float yCoord, float rotation,
    bool forceLeave)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto worldCID = state->GetWorldCID();

    auto currentZone = cState->GetZone();
    auto currentInstance = currentZone ? currentZone->GetInstance() : nullptr;

    auto nextZone = GetZone(zoneID, dynamicMapID, client,
        currentInstance ? currentInstance->GetID() : 0);
    if(nextZone == nullptr)
    {
        return false;
    }

    if(nextZone->GetInstanceType() == InstanceType_t::DEMON_ONLY &&
        (!dState->GetEntity() || !dState->IsAlive()))
    {
        LOG_ERROR(libcomp::String("Request to enter a demon only"
            " zone received with no living demon summoned: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    if(forceLeave || (currentZone && currentZone != nextZone))
    {
        // Trigger zone-out actions
        if(currentZone)
        {
            TriggerZoneActions(currentZone, { cState, dState },
                ZoneTrigger_t::ON_ZONE_OUT, client);
        }

        LeaveZone(client, false, zoneID, dynamicMapID);
    }
    else if(currentZone)
    {
        auto server = mServer.lock();
        auto characterManager = server->GetCharacterManager();
        auto definitionManager = server->GetDefinitionManager();

        // Remove any opponents
        characterManager->AddRemoveOpponent(false, cState, nullptr);
        characterManager->AddRemoveOpponent(false, dState, nullptr);

        // Deactivate and save the updated status effects
        cState->SetStatusEffectsActive(false, definitionManager);
        dState->SetStatusEffectsActive(false, definitionManager);

        characterManager->UpdateStatusEffects(cState, true);
        characterManager->UpdateStatusEffects(dState, true);
    }

    auto uniqueID = nextZone->GetID();
    auto nextInstance = nextZone->GetInstance();

    bool firstConnection = false;
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEntityMap[worldCID] = uniqueID;

        // When the player enters the instance they have access to
        // revoke access so they cannot re-enter
        auto accessIter = mZoneInstanceAccess.find(worldCID);
        if(accessIter != mZoneInstanceAccess.end() &&
            nextInstance && accessIter->second == nextInstance->GetID())
        {
            mZoneInstanceAccess.erase(worldCID);
        }

        // Reactive the zone if its not active already
        if(mActiveZones.find(uniqueID) == mActiveZones.end())
        {
            mActiveZones.insert(uniqueID);
            firstConnection = true;
        }
    }

    nextZone->AddConnection(client);
    cState->SetZone(nextZone);
    dState->SetZone(nextZone);

    // Reset state values that do not persist between zones
    state->SetAcceptRevival(false);
    dState->SetDeathTimeOut(0);
    state->SetZoneInSpotID(0);

    auto server = mServer.lock();
    auto ticks = server->GetServerTime();
    auto zoneDef = nextZone->GetDefinition();

    // Bike boosting does not persist between zones
    if(state->GetBikeBoosting())
    {
        state->SetBikeBoosting(false);

        cState->RemoveAdditionalTokusei(SVR_CONST.TOKUSEI_BIKE_BOOST);

        server->GetTokuseiManager()->Recalculate(cState, true);
    }

    // Set the zone-in spot for instances
    if(nextInstance)
    {
        auto dynamicMap = nextZone->GetDynamicMap();

        if(dynamicMap)
        {
            auto spots = server->GetDefinitionManager()
                ->GetSpotData(zoneDef->GetDynamicMapID());
            for(auto spotPair : spots)
            {
                auto spot = spotPair.second;
                auto spotIter = dynamicMap->Spots.find(spotPair.first);

                // Filter valid zone-in spots only
                if(spot->GetType() == 3 && spotIter != dynamicMap->Spots.end())
                {
                    if(PointInPolygon(Point(xCoord, yCoord),
                        spotIter->second->Vertices))
                    {
                        state->SetZoneInSpotID(spotPair.first);
                        break;
                    }
                }
            }
        }

        nextInstance->RefreshPlayerState();
    }

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

    // End any previous instance specific data if leaving
    if(currentInstance && currentInstance != nextInstance)
    {
        EndInstanceTimer(currentInstance, client, false, true);
    }

    if(!nextInstance && currentZone)
    {
        // If moving to a non-instanced zone, set log out zone for the new
        // location in case of a disconnect without saving
        auto character = cState->GetEntity();
        character->SetLogoutZone(zoneID);
        character->SetLogoutX(cState->GetCurrentX());
        character->SetLogoutY(cState->GetCurrentY());
        character->SetLogoutRotation(cState->GetCurrentRotation());

        server->GetWorldDatabase()->QueueUpdate(character);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE);
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteS32Little((int32_t)nextZone->GetID());
    reply.WriteFloat(xCoord);
    reply.WriteFloat(yCoord);
    reply.WriteFloat(rotation);
    reply.WriteS32Little((int32_t)zoneDef->GetDynamicMapID());

    client->QueuePacket(reply);

    if(firstConnection && nextInstance)
    {
        StartInstanceTimer(nextInstance);
    }

    client->FlushOutgoing();

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
        if(otherCState && (otherCState != cState ||
            exchangeSession->GetSourceEntityID() != cState->GetEntityID()))
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

    uint32_t previousZoneID = 0;
    bool instanceRemoved = false;
    std::shared_ptr<Zone> zone = nullptr;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto iter = mEntityMap.find(worldCID);
        if(iter != mEntityMap.end())
        {
            uint32_t uniqueID = iter->second;
            zone = mZones[uniqueID];

            auto def = zone->GetDefinition();
            auto instance = zone->GetInstance();

            mEntityMap.erase(worldCID);
            zone->RemoveConnection(client);
            previousZoneID = def->GetID();

            // Determine actions needed if the last connection has left
            if(zone->GetConnections().size() == 0)
            {
                // Always "freeze" the zone
                mActiveZones.erase(uniqueID);

                auto instDef = instance ? instance->GetDefinition() : nullptr;

                // If the current zone is global, the next zone is the same
                // or the next zone is will be on the same instance, keep it
                bool keepZone = false;
                if(def->GetGlobal() ||
                    (def->GetID() == newZoneID &&
                        def->GetDynamicMapID() == newDynamicMapID))
                {
                    keepZone = true;
                }
                else if(zone && instDef)
                {
                    for(size_t i = 0; i < instDef->ZoneIDsCount(); i++)
                    {
                        uint32_t zoneID = instDef->GetZoneIDs(i);
                        uint32_t dynamicMapID = instDef->GetDynamicMapIDs(i);
                        if(zoneID == newZoneID &&
                            (newDynamicMapID == 0 || newDynamicMapID == dynamicMapID))
                        {
                            keepZone = true;
                            break;
                        }
                    }
                }

                // If an instance zone is being left see if it
                // is empty and can be removed
                if(!keepZone && instance)
                {
                    instanceRemoved = RemoveInstance(instance->GetID());
                }

                if(keepZone)
                {
                    // Stop all AI in place
                    uint64_t now = ChannelServer::GetServerTime();
                    for(auto eState : zone->GetEnemies())
                    {
                        eState->Stop(now);
                    }
                }
            }

            // Refresh the state of the instance being left
            if(instance && !instanceRemoved)
            {
                instance->RefreshPlayerState();
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
        TriggerZoneActions(zone, { cState, dState },
            ZoneTrigger_t::ON_ZONE_OUT, client);
        server->GetTokuseiManager()->RecalculateParty(
            state->GetParty());
    }
    else
    {
        // Set the previous zone
        cState->GetEntity()->SetPreviousZone(previousZoneID);
    }

    // If logging out, cancel zone out and log out effects (zone out effects
    // are cancelled on zone enter instead if not logging out)
    if(logOut)
    {
        characterManager->CancelStatusEffects(client, EFFECT_CANCEL_LOGOUT
            | EFFECT_CANCEL_ZONEOUT);

        auto accessIter = mZoneInstanceAccess.find(worldCID);
        if(accessIter != mZoneInstanceAccess.end())
        {
            uint32_t instanceID = accessIter->second;
            mZoneInstanceAccess.erase(accessIter);

            if(instanceID != 0)
            {
                RemoveInstance(instanceID);
            }
        }
    }

    // Deactivate and save the updated status effects
    cState->SetStatusEffectsActive(false, definitionManager);
    dState->SetStatusEffectsActive(false, definitionManager);
    characterManager->UpdateStatusEffects(cState, !logOut);
    characterManager->UpdateStatusEffects(dState, !logOut);
}

std::shared_ptr<ZoneInstance> ZoneManager::CreateInstance(const std::shared_ptr<
    ChannelClientConnection>& client, uint32_t instanceID, uint32_t variantID,
    uint32_t timerID, libcomp::String timerExpirationEventID)
{
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    auto def = serverDataManager->GetZoneInstanceData(instanceID);
    if(!def)
    {
        LOG_ERROR(libcomp::String("Attempted to create invalid zone instance"
            ": %1\n").Arg(instanceID));
        return nullptr;
    }

    auto state = client->GetClientState();

    std::set<int32_t> accessCIDs = { state->GetWorldCID() };

    auto party = state->GetParty();
    if(party)
    {
        for(auto memberCID : party->GetMemberIDs())
        {
            accessCIDs.insert(memberCID);
        }
    }

    // Make the instance
    std::lock_guard<std::mutex> lock(mLock);

    uint32_t id = mNextZoneInstanceID++;

    auto instance = std::make_shared<ZoneInstance>(id, def, accessCIDs);

    auto instType = InstanceType_t::NORMAL;
    if(variantID)
    {
        auto variant = serverDataManager->GetZoneInstanceVariantData(
            variantID);
        if(!variant)
        {
            LOG_ERROR(libcomp::String("Invalid variant encountered during"
                " instance creation: %1\n").Arg(variantID));
            return nullptr;
        }

        instance->SetVariant(variant);
        instance->SetTimerExpirationEventID(
            variant->GetTimerExpirationEventID());

        instType = variant->GetInstanceType();
    }

    if(timerID)
    {
        instance->SetTimerID(timerID);
        if(instType == InstanceType_t::NORMAL)
        {
            auto timeData = server->GetDefinitionManager()->GetTimeLimitData(
                timerID);
            if(timeData)
            {
                instance->SetTimeLimitData(timeData);
                instance->SetTimerExpirationEventID(timerExpirationEventID);
            }
            else
            {
                LOG_ERROR(libcomp::String("Invalid timer ID specified"
                    " for instance creation: %1\n").Arg(timerID));
                return nullptr;
            }
        }
        else if(instType != InstanceType_t::DEMON_ONLY)
        {
            // Demon only instances use the timer ID to specify timer color
            // 0 = bronze, 1 = silver, 2 = gold

            LOG_ERROR(libcomp::String("Attempted to specify a timer during"
                " special instance creation: %1\n").Arg(variantID));
            return nullptr;
        }
    }

    for(int32_t cid : accessCIDs)
    {
        mZoneInstanceAccess[cid] = id;
    }

    mZoneInstances[id] = instance;
    LOG_DEBUG(libcomp::String("Creating zone instance: %1 (%2%3)\n")
        .Arg(id).Arg(def->GetID())
        .Arg(variantID ? libcomp::String(": %1").Arg(variantID) : ""));

    return instance;
}

std::shared_ptr<ZoneInstance> ZoneManager::GetInstance(uint32_t instanceID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mZoneInstances.find(instanceID);
    return it != mZoneInstances.end() ? it->second : nullptr;
}

std::shared_ptr<ZoneInstance> ZoneManager::GetInstanceAccess(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();

    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mZoneInstanceAccess.find(state->GetWorldCID());
        if(it != mZoneInstanceAccess.end())
        {
            // Return next instance
            uint32_t instanceID = it->second;
            return mZoneInstances[instanceID];
        }
    }

    // Return current instance if it exists
    auto zone = state->GetCharacterState()->GetZone();
    return zone ? zone->GetInstance() : nullptr;
}

bool ZoneManager::ClearInstanceAccess(uint32_t instanceID)
{
    bool removed = false;

    std::lock_guard<std::mutex> lock(mLock);

    auto it = mZoneInstances.find(instanceID);
    if(it != mZoneInstances.end())
    {
        auto instance = it->second;
        for(int32_t cid : instance->GetAccessCIDs())
        {
            auto accessIter = mZoneInstanceAccess.find(cid);
            if(accessIter != mZoneInstanceAccess.end() &&
                accessIter->second == instanceID)
            {
                mZoneInstanceAccess.erase(accessIter);
                removed = true;
            }
        }
    }

    // If the instance is empty, remove it
    RemoveInstance(instanceID);

    return removed;
}

bool ZoneManager::SendPopulateZoneData(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();

    auto zone = GetCurrentZone(state->GetWorldCID());
    if(!zone)
    {
        // Not in a zone, quit now
        return false;
    }

    auto zoneDef = zone->GetDefinition();
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

    // Does not appear to actually need to be sent for player characters
    //PopEntityForZoneProduction(zone, cState->GetEntityID(), 0);

    // Expire zone change status effects
    characterManager->CancelStatusEffects(client, EFFECT_CANCEL_ZONEOUT);

    HandleSpecialInstancePopulate(client, zone);

    ShowEntityToZone(zone, cState->GetEntityID());
    characterManager->SendMovementSpeed(client, cState, true);

    if(dState->GetEntity())
    {
        PopEntityForZoneProduction(zone, dState->GetEntityID(), 0);
        ShowEntityToZone(zone, dState->GetEntityID());

        server->GetTokuseiManager()->SendCostAdjustments(dState->GetEntityID(),
            client);
        characterManager->SendMovementSpeed(client, dState, true);
    }

    // Activate status effects
    cState->SetStatusEffectsActive(true, definitionManager);
    dState->SetStatusEffectsActive(true, definitionManager);

    // Trigger zone-in actions
    TriggerZoneActions(zone, { cState, dState }, ZoneTrigger_t::ON_ZONE_IN,
        client);

    // All zone information is queued and sent together to minimize excess
    // communication
    for(auto enemyState : zone->GetEnemies())
    {
        SendEnemyData(client, enemyState, zone, false, true);
    }

    for(auto npcState : zone->GetNPCs())
    {
        // If an NPC's state is not 1, do not show it right now
        if(npcState->GetEntity()->GetState() == 1)
        {
            ShowNPC(zone, { client }, npcState, true);
        }
    }

    for(auto objState : zone->GetServerObjects())
    {
        // If an NPC's state is not 255, show it now
        if(objState->GetEntity()->GetState() != 255)
        {
            ShowObject(zone, { client }, objState, true);
        }
    }

    for(auto plasmaPair : zone->GetPlasma())
    {
        auto pState = plasmaPair.second;
        auto pSpawn = pState->GetEntity();

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_DATA);
        p.WriteS32Little(pState->GetEntityID());
        p.WriteS32Little((int32_t)zone->GetID());
        p.WriteS32Little((int32_t)zoneDef->GetID());
        p.WriteFloat(pState->GetCurrentX());
        p.WriteFloat(pState->GetCurrentY());
        p.WriteFloat(pState->GetCurrentRotation());
        p.WriteS8((int8_t)pSpawn->GetColor());
        p.WriteS8((int8_t)pSpawn->GetPickTime());
        p.WriteS8((int8_t)pSpawn->GetPickSpeed());
        p.WriteU16Little(pSpawn->GetPickSize());

        auto activePoints = pState->GetActivePoints();

        uint8_t pointCount = (uint8_t)activePoints.size();
        p.WriteS8((int8_t)pointCount);
        for(auto point : activePoints)
        {
            p.WriteS8((int8_t)point->GetID());
            p.WriteS32Little(point->GetState(state->GetWorldCID()));

            p.WriteFloat(point->GetX());
            p.WriteFloat(point->GetY());
            p.WriteFloat(point->GetRotation());
        }

        client->QueuePacket(p);
        ShowEntity(client, pState->GetEntityID(), true);
    }

    for(auto bState : zone->GetBazaars())
    {
        auto bazaar = bState->GetEntity();

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_DATA);
        p.WriteS32Little(bState->GetEntityID());
        p.WriteS32Little((int32_t)zone->GetID());
        p.WriteS32Little((int32_t)zoneDef->GetID());
        p.WriteFloat(bState->GetCurrentX());
        p.WriteFloat(bState->GetCurrentY());
        p.WriteFloat(bState->GetCurrentRotation());
        p.WriteS32Little((int32_t)bazaar->MarketIDsCount());

        for(uint32_t marketID : bazaar->GetMarketIDs())
        {
            auto market = bState->GetCurrentMarket(marketID);
            if(market && market->GetState() ==
                objects::BazaarData::State_t::BAZAAR_INACTIVE)
            {
                market = nullptr;
            }

            p.WriteU32Little(marketID);
            p.WriteS32Little(market ? (int32_t)market->GetState() : 0);
            p.WriteS32Little(market ? market->GetNPCType() : -1);
            p.WriteString16Little(state->GetClientStringEncoding(),
                market ? market->GetComment() : "", true);
        }

        client->QueuePacket(p);
        ShowEntity(client, bState->GetEntityID(), true);
    }

    for(auto lState : zone->GetLootBoxes())
    {
        SendLootBoxData(client, lState, nullptr, false, true);
    }

    for(auto allyState : zone->GetAllies())
    {
        SendAllyData(client, allyState, zone, false, true);
    }

    // Send all the queued NPC packets
    client->FlushOutgoing();

    std::list<std::shared_ptr<ChannelClientConnection>> self = { client };
    for(auto oConnection : otherClients)
    {
        auto oState = oConnection->GetClientState();
        auto oCState = oState->GetCharacterState();
        auto oDState = oState->GetDemonState();

        if(oCState->IsClientVisible())
        {
            characterManager->SendOtherCharacterData(self, oState);

            PopEntityForProduction(client, oCState->GetEntityID(), 0);
            ShowEntity(client, oCState->GetEntityID());
        }

        if(oDState->IsClientVisible())
        {
            characterManager->SendOtherPartnerData(self, oState);
            PopEntityForProduction(client, oDState->GetEntityID(), 0);
            ShowEntity(client, oDState->GetEntityID());

            if(oDState->GetDeathTimeOut())
            {
                UpdateDeathTimeOut(oState, 0, client);
            }
        }
    }

    return true;
}

void ZoneManager::ShowEntity(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID, bool queue)
{
    std::list<std::shared_ptr<ChannelClientConnection>> clients;
    clients.push_back(client);
    ShowEntity(clients, entityID, queue);
}

void ZoneManager::ShowEntityToZone(const std::shared_ptr<Zone>& zone, int32_t entityID)
{
    auto clients = zone->GetConnectionList();
    ShowEntity(clients, entityID, false);

    // If its an active entity, set it as displayed
    auto activeState = zone->GetActiveEntity(entityID);
    if(activeState &&
        activeState->GetDisplayState() < ActiveDisplayState_t::ACTIVE)
    {
        activeState->SetDisplayState(ActiveDisplayState_t::ACTIVE);
    }
}

void ZoneManager::ShowEntity(const std::list<std::shared_ptr<
    ChannelClientConnection>> &clients, int32_t entityID, bool queue)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_ENTITY);
    p.WriteS32Little(entityID);

    ChannelClientConnection::BroadcastPacket(clients, p, queue);
}

void ZoneManager::PopEntityForProduction(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID, int32_t type, bool queue)
{
    std::list<std::shared_ptr<ChannelClientConnection>> clients;
    clients.push_back(client);
    PopEntityForProduction(clients, entityID, type, queue);
}

void ZoneManager::PopEntityForZoneProduction(const std::shared_ptr<Zone>& zone,
    int32_t entityID, int32_t type)
{
    auto clients = zone->GetConnectionList();
    PopEntityForProduction(clients, entityID, type, false);
}

void ZoneManager::PopEntityForProduction(const std::list<std::shared_ptr<
    ChannelClientConnection>> &clients, int32_t entityID,
    int32_t type, bool queue)
{
    libcomp::Packet p;
    p.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_POP_ENTITY_FOR_PRODUCTION);
    p.WriteS32Little(entityID);
    p.WriteS32Little(type);

    ChannelClientConnection::BroadcastPacket(clients, p, queue);
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

        ChannelClientConnection::BroadcastPacket(clients, p, true);

        p.Clear();
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_OBJECT);
        p.WriteS32Little(entityID);

        ChannelClientConnection::BroadcastPacket(clients, p, true);
    }

    if(!queue)
    {
        ChannelClientConnection::FlushAllOutgoing(clients);
    }
}

void ZoneManager::ShowNPC(const std::shared_ptr<Zone>& zone,
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    const std::shared_ptr<NPCState>& npcState, bool queue)
{
    auto npc = npcState->GetEntity();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_NPC_DATA);
    p.WriteS32Little(npcState->GetEntityID());
    p.WriteU32Little(npc->GetID());
    p.WriteS32Little((int32_t)zone->GetID());
    p.WriteS32Little((int32_t)zone->GetDefinitionID());
    p.WriteFloat(npcState->GetCurrentX());
    p.WriteFloat(npcState->GetCurrentY());
    p.WriteFloat(npcState->GetCurrentRotation());
    p.WriteS16Little(0);    //Unknown

    ChannelClientConnection::BroadcastPacket(clients, p, true);

    ShowEntity(clients, npcState->GetEntityID(), true);

    if(!queue)
    {
        ChannelClientConnection::FlushAllOutgoing(clients);
    }
}

void ZoneManager::ShowObject(const std::shared_ptr<Zone>& zone,
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    const std::shared_ptr<ServerObjectState>& objState, bool queue)
{
    auto obj = objState->GetEntity();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_OBJECT_NPC_DATA);
    p.WriteS32Little(objState->GetEntityID());
    p.WriteU32Little(obj->GetID());
    p.WriteU8(obj->GetState());
    p.WriteS32Little((int32_t)zone->GetID());
    p.WriteS32Little((int32_t)zone->GetDefinitionID());
    p.WriteFloat(objState->GetCurrentX());
    p.WriteFloat(objState->GetCurrentY());
    p.WriteFloat(objState->GetCurrentRotation());

    ChannelClientConnection::BroadcastPacket(clients, p, true);

    ShowEntity(clients, objState->GetEntityID(), true);

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

        RelativeTimeMap timeMap;
        timeMap[p.Size()] = now;
        timeMap[p.Size() + 4] = fixUntil;

        auto zConnections = zone->GetConnectionList();
        ChannelClientConnection::SendRelativeTimePacket(zConnections, p, timeMap);
    }
}

void ZoneManager::ScheduleEntityRemoval(uint64_t time, const std::shared_ptr<Zone>& zone,
    const std::list<int32_t>& entityIDs, int32_t removeMode)
{
    mServer.lock()->ScheduleWork(time, [](ZoneManager* zoneManager,
        uint64_t pTime, const std::shared_ptr<Zone> pZone,
        std::list<int32_t> pEntityIDs, int32_t pRemoveMode)
        {
            std::list<int32_t> finalList;
            for(int32_t lootEntityID : pEntityIDs)
            {
                auto state = pZone->GetEntity(lootEntityID);
                auto aState = std::dynamic_pointer_cast<ActiveEntityState>(state);
                if(state && (!aState || aState->GetDeathTimeOut() <= pTime))
                {
                    pZone->RemoveEntity(lootEntityID);
                    finalList.push_back(lootEntityID);
                }
            }

            if(finalList.size() > 0)
            {
                zoneManager->RemoveEntitiesFromZone(pZone, finalList, pRemoveMode);
            }
        }, this, time, zone, entityIDs, removeMode);
}

void ZoneManager::SendLootBoxData(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<LootBoxState>& lState,
    const std::shared_ptr<ActiveEntityState>& eState, bool sendToAll, bool queue)
{
    auto box = lState->GetEntity();
    auto zone = GetCurrentZone(client);
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
    
    auto statusEffects = enemyState->GetCurrentStatusEffectStates();

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
        PopEntityForProduction(zClient, enemyState->GetEntityID(),
            sendToAll ? 3 : 0, true);
        ShowEntity(zClient, enemyState->GetEntityID(), true);
    }

    if(!queue)
    {
        ChannelClientConnection::FlushAllOutgoing(clients);
    }
}

void ZoneManager::SendAllyData(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<AllyState>& allyState, const std::shared_ptr<Zone>& zone,
    bool sendToAll, bool queue)
{
    auto stats = allyState->GetCoreStats();
    auto zoneData = zone->GetDefinition();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ALLY_DATA);
    p.WriteS32Little(allyState->GetEntityID());
    p.WriteS32Little((int32_t)allyState->GetEntity()->GetType());
    p.WriteS32Little(allyState->GetMaxHP());
    p.WriteS32Little(stats->GetHP());
    p.WriteS8(stats->GetLevel());
    p.WriteS32Little((int32_t)zone->GetID());
    p.WriteS32Little((int32_t)zoneData->GetID());
    p.WriteFloat(allyState->GetOriginX());
    p.WriteFloat(allyState->GetOriginY());
    p.WriteFloat(allyState->GetOriginRotation());

    auto statusEffects = allyState->GetCurrentStatusEffectStates();

    p.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
    for(auto ePair : statusEffects)
    {
        p.WriteU32Little(ePair.first->GetEffect());
        p.WriteS32Little((int32_t)ePair.second);
        p.WriteU8(ePair.first->GetStack());
    }

    p.WriteU32Little(allyState->GetEntity()->GetVariantType());

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
        PopEntityForProduction(zClient, allyState->GetEntityID(),
            sendToAll ? 3 : 0, true);
        ShowEntity(zClient, allyState->GetEntityID(), true);
    }

    if(!queue)
    {
        ChannelClientConnection::FlushAllOutgoing(clients);
    }
}

void ZoneManager::HandleDespawns(const std::shared_ptr<Zone>& zone)
{
    std::list<int32_t> enemyIDs;

    std::set<int32_t> despawnEntities = zone->GetDespawnEntities();
    if(despawnEntities.size() > 0)
    {
        auto characterManager = mServer.lock()->GetCharacterManager();
        for(int32_t entityID : despawnEntities)
        {
            auto eState = zone->GetEntity(entityID);
            if(eState)
            {
                switch(eState->GetEntityType())
                {
                case EntityType_t::ENEMY:
                    enemyIDs.push_back(entityID);

                    // Remove from combat first
                    characterManager->AddRemoveOpponent(false,
                        std::dynamic_pointer_cast<EnemyState>(eState),
                        nullptr);
                    break;
                case EntityType_t::PLASMA:
                    /// @todo
                    break;
                default:
                    break;
                }

                zone->RemoveEntity(entityID);
            }
        }

        if(enemyIDs.size() > 0)
        {
            RemoveEntitiesFromZone(zone, enemyIDs, 7, false);
        }
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
    auto characterManager = server->GetCharacterManager();
    auto tokuseiManager = server->GetTokuseiManager();

    std::list<libcomp::Packet> zonePackets;
    std::set<uint32_t> added, updated, removed;
    std::set<std::shared_ptr<ActiveEntityState>> displayStateModified;
    std::set<std::shared_ptr<ActiveEntityState>> statusRemoved;

    int32_t hpTDamage, mpTDamage, upkeepCost;
    for(auto entity : effectEntities)
    {
        if(!entity->PopEffectTicks(now, hpTDamage, mpTDamage, upkeepCost,
            added, updated, removed)) continue;

        if(added.size() > 0 || updated.size() > 0)
        {
            auto effectMap = entity->GetStatusEffects();

            std::list<std::shared_ptr<objects::StatusEffect>> active;
            for(uint32_t effectType : added)
            {
                auto it = effectMap.find(effectType);
                if(it != effectMap.end())
                {
                    active.push_back(it->second);
                }
            }
            
            for(uint32_t effectType : updated)
            {
                auto it = effectMap.find(effectType);
                if(it != effectMap.end())
                {
                    active.push_back(it->second);
                }
            }

            libcomp::Packet p;
            if(characterManager->GetActiveStatusesPacket(p,
                entity->GetEntityID(), active))
            {
                zonePackets.push_back(p);
            }
        }

        bool hpMpRecalc = false;

        // Regen/deal T-damage first
        if(hpTDamage != 0 || mpTDamage != 0)
        {
            int32_t hpAdjusted, mpAdjusted;
            if(entity->SetHPMP(-hpTDamage, -(mpTDamage + upkeepCost), true,
                false, 0, hpAdjusted, mpAdjusted))
            {
                if(hpAdjusted < 0)
                {
                    entity->CancelStatusEffects(EFFECT_CANCEL_DAMAGE);
                }

                displayStateModified.insert(entity);

                libcomp::Packet p;
                p.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_DO_TDAMAGE);
                p.WriteS32Little(entity->GetEntityID());
                p.WriteS32Little(hpAdjusted);
                p.WriteS32Little(mpAdjusted);
                zonePackets.push_back(p);

                hpMpRecalc = true;
            }
        }

        // Pay upkeep costs next
        if(upkeepCost != 0)
        {
            int32_t hpAdjusted, mpAdjusted;
            if(entity->SetHPMP(0, -upkeepCost, true,
                false, 0, hpAdjusted, mpAdjusted))
            {
                libcomp::Packet p;
                p.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_SKILL_UPKEEP_COST);
                p.WriteS32Little(entity->GetEntityID());
                p.WriteU32Little((uint32_t)(-mpAdjusted));
                zonePackets.push_back(p);

                hpMpRecalc = true;
            }
        }

        if(hpMpRecalc)
        {
            tokuseiManager->Recalculate(entity,
                std::set<TokuseiConditionType>
                { TokuseiConditionType::CURRENT_HP,
                    TokuseiConditionType::CURRENT_MP });
        }

        if(removed.size() > 0)
        {
            libcomp::Packet p;
            if(characterManager->GetRemovedStatusesPacket(p,
                entity->GetEntityID(), removed))
            {
                zonePackets.push_back(p);
            }

            statusRemoved.insert(entity);
        }
    }

    if(zonePackets.size() > 0)
    {
        auto zConnections = zone->GetConnectionList();
        ChannelClientConnection::BroadcastPackets(zConnections, zonePackets);
    }

    for(auto eState : statusRemoved)
    {
        // Make sure T-damage is sent first
        // Status add/update and world update handled when applying changes
        tokuseiManager->Recalculate(eState, true,
            std::set<int32_t>{ eState->GetEntityID() });
        if(characterManager->RecalculateStats(eState) & ENTITY_CALC_STAT_WORLD)
        {
            displayStateModified.erase(eState);
        }
    }
    
    if(displayStateModified.size() > 0)
    {
        characterManager->UpdateWorldDisplayState(displayStateModified);
    }
}

void ZoneManager::HandleSpecialInstancePopulate(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<Zone>& zone)
{
    auto instance = zone->GetInstance();
    auto instVariant = instance ? instance->GetVariant() : nullptr;
    if(instVariant)
    {
        auto state = client->GetClientState();
        auto cState = state->GetCharacterState();

        switch(instVariant->GetInstanceType())
        {
        case InstanceType_t::TIME_TRIAL:
            {
                SendInstanceTimer(instance, client, true);
            }
            break;
        case InstanceType_t::DEMON_ONLY:
            {
                auto characterManager = mServer.lock()->GetCharacterManager();

                // Cancel mount state if the player has it
                characterManager->CancelMount(state);

                // Refresh the demon-only status effect
                StatusEffectChanges effects;
                effects[SVR_CONST.STATUS_HIDDEN] = StatusEffectChange(
                    SVR_CONST.STATUS_HIDDEN, 1, true);
                
                characterManager->AddStatusEffectImmediate(client, cState,
                    effects);

                SendInstanceTimer(instance, client, true);
            }
            break;
        default:
            break;
        }
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
    auto eState = std::dynamic_pointer_cast<EnemyState>(
        CreateEnemy(zone, demonID, nullptr, x, y, rot));

    auto server = mServer.lock();
    server->GetAIManager()->Prepare(eState, aiType);
    zone->AddEnemy(eState);

    TriggerZoneActions(zone, { eState }, ZoneTrigger_t::ON_SPAWN);

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
    auto dynamicMap = zone->GetDynamicMap();
    auto zoneDef = zone->GetDefinition();

    // Location ID then group ID (true) or group ID then spot ID (false)
    std::list<std::pair<bool, std::pair<uint32_t, uint32_t>>> groups;
    if(actionSource)
    {
        auto mode = actionSource->GetMode();

        std::list<std::pair<bool, std::pair<uint32_t, uint32_t>>> checking;
        for(uint32_t slgID : actionSource->GetSpawnLocationGroupIDs())
        {
            std::pair<bool, std::pair<uint32_t, uint32_t>> p(true,
                std::pair<uint32_t, uint32_t>(slgID, 0));
            checking.push_back(p);
        }

        std::set<uint32_t> disabledGroupIDs = zone->GetDisabledSpawnGroups();
        for(auto gPair : actionSource->GetSpawnGroupIDs())
        {
            if(disabledGroupIDs.find(gPair.first) == disabledGroupIDs.end())
            {
                std::pair<bool, std::pair<uint32_t, uint32_t>> p(false,
                    std::pair<uint32_t, uint32_t>(gPair.first, gPair.second));
                checking.push_back(p);
            }
        }

        // Enable/disable spawn groups and despawn all work a bit different
        // than normal spawns
        if(mode == objects::ActionSpawn::Mode_t::ENABLE_GROUP ||
            mode == objects::ActionSpawn::Mode_t::DISABLE_GROUP)
        {
            std::set<uint32_t> groupIDs;
            for(auto& pair : checking)
            {
                // Filter just group IDs
                if(!pair.first)
                {
                    groupIDs.insert(pair.second.first);
                }
            }

            zone->EnableDisableSpawnGroups(groupIDs,
                mode == objects::ActionSpawn::Mode_t::ENABLE_GROUP);
            return false;
        }
        else if(mode == objects::ActionSpawn::Mode_t::DESPAWN)
        {
            // Match enemies in zone on specified locations and
            // group/location pairs
            for(auto eState : zone->GetEnemies())
            {
                auto enemy = eState->GetEntity();
                if(enemy->GetSpawnGroupID() > 0 ||
                    enemy->GetSpawnLocationGroupID() > 0)
                {
                    bool despawn = false;
                    for(auto& pair : checking)
                    {
                        if(pair.first)
                        {
                            // Location
                            uint32_t slgID = pair.second.first;
                            if(enemy->GetSpawnLocationGroupID() == slgID)
                            {
                                despawn = true;
                                break;
                            }
                        }
                        else
                        {
                            // Group
                            uint32_t sgID = pair.second.first;
                            uint32_t slgID = pair.second.second;

                            // Use specified location or any if zero
                            if(enemy->GetSpawnGroupID() == sgID && (!slgID ||
                                enemy->GetSpawnLocationGroupID() == slgID))
                            {
                                despawn = true;
                                break;
                            }
                        }
                    }

                    if(despawn)
                    {
                        zone->MarkDespawn(eState->GetEntityID());
                    }
                }
            }

            return false;
        }

        // Spawn is not a special type, continue processing
        bool spawnValidated = false;
        if(actionSource->GetSpotID() != 0 &&
            (mode == objects::ActionSpawn::Mode_t::ONE_TIME ||
             mode == objects::ActionSpawn::Mode_t::ONE_TIME_RANDOM))
        {
            if(zone->SpawnedAtSpot(actionSource->GetSpotID()))
            {
                // Nothing to do, spawns have already happened at the
                // explicit spot
                return false;
            }

            spawnValidated = true;
        }

        for(auto cPair : checking)
        {
            auto& gPair = cPair.second;

            bool add = false;
            if(spawnValidated)
            {
                add = true;
            }
            else
            {
                switch(mode)
                {
                case objects::ActionSpawn::Mode_t::ONE_TIME:
                    add = !zone->GroupHasSpawned(gPair.first, cPair.first,
                        false);
                    break;
                case objects::ActionSpawn::Mode_t::ONE_TIME_RANDOM:
                    if(!zone->GroupHasSpawned(gPair.first, cPair.first,
                        false))
                    {
                        add = true;
                    }
                    else
                    {
                        // Stop here if any have spawned
                        return false;
                    }
                    break;
                case objects::ActionSpawn::Mode_t::NONE_EXIST:
                    add = !zone->GroupHasSpawned(gPair.first, cPair.first,
                        true);
                    break;
                case objects::ActionSpawn::Mode_t::NORMAL:
                default:
                    add = true;
                    break;
                }
            }

            if(add)
            {
                groups.push_back(cPair);
            }
        }

        if(mode == objects::ActionSpawn::Mode_t::ONE_TIME_RANDOM &&
            groups.size() > 1)
        {
            auto g = libcomp::Randomizer::GetEntry(groups);
            groups.clear();
            groups.push_back(g);
        }
    }
    else if(refreshAll)
    {
        // All spawn location groups will be refreshed
        for(auto slgPair : zoneDef->GetSpawnLocationGroups())
        {
            if(slgPair.second->GetRespawnTime() > 0)
            {
                std::pair<bool, std::pair<uint32_t, uint32_t>> p(true,
                    std::pair<uint32_t, uint32_t>(slgPair.first, 0));
                groups.push_back(p);
            }
        }
    }
    else
    {
        // Determine normal spawns needed
        if(now == 0)
        {
            now = ChannelServer::GetServerTime();
        }

        auto slgIDs = zone->GetRespawnLocations(now);
        if(slgIDs.size() == 0)
        {
            return false;
        }

        for(uint32_t slgID : slgIDs)
        {
            std::pair<bool, std::pair<uint32_t, uint32_t>> p(true,
                std::pair<uint32_t, uint32_t>(slgID, 0));
            groups.push_back(p);
        }
    }

    if(groups.size() == 0)
    {
        return false;
    }

    bool containsSimpleSpawns = false;
    bool mergeEncounter = actionSource && actionSource->DefeatActionsCount() > 0;
    std::set<uint32_t> disabledGroupIDs = zone->GetDisabledSpawnGroups();

    std::list<std::list<std::shared_ptr<ActiveEntityState>>> eStateGroups;
    std::list<std::shared_ptr<objects::SpawnGroup>> spawnActionGroups;
    for(auto groupPair : groups)
    {
        uint32_t sgID = groupPair.first
            ? groupPair.second.second : groupPair.second.first;
        uint32_t slgID = groupPair.first ? groupPair.second.first : 0;
        uint32_t spotID = !groupPair.first ? groupPair.second.second : 0;

        std::set<uint32_t> spotIDs;
        std::list<std::shared_ptr<objects::SpawnLocation>> locations;
        if(actionSource && actionSource->GetSpotID() != 0)
        {
            // Explicit spot set on the action
            spotID = actionSource->GetSpotID();
        }

        std::shared_ptr<objects::SpawnLocationGroup> slg;

        if(slgID)
        {
            slg = zoneDef->GetSpawnLocationGroups(slgID);

            if(!slg)
            {
                LOG_WARNING(libcomp::String("Skipping invalid spawn location"
                    " group %1\n")
                    .Arg(groupPair.first));
                continue;
            }

            if(!spotID)
            {
                spotIDs = slg->GetSpotIDs();
            }

            locations = slg->GetLocations();

            // Get the random group now
            std::list<uint32_t> groupIDs;
            for(uint32_t groupID : slg->GetGroupIDs())
            {
                if(disabledGroupIDs.find(groupID) == disabledGroupIDs.end())
                {
                    groupIDs.push_back(groupID);
                }
            }

            if(groupIDs.size() > 0)
            {
                sgID = libcomp::Randomizer::GetEntry(groupIDs);
            }
        }

        if(sgID == 0) continue;

        if(spotID)
        {
            spotIDs.insert(spotID);
        }

        bool useSpotID = dynamicMap && spotIDs.size() > 0;

        if(!useSpotID && locations.size() == 0) continue;

        auto sg = sgID > 0 ? zoneDef->GetSpawnGroups(sgID) : nullptr;
        if(!sg)
        {
            LOG_WARNING(libcomp::String("Skipping invalid spawn group %1\n")
                .Arg(sgID));
            continue;
        }

        std::list<std::shared_ptr<ActiveEntityState>>* eStateGroup = 0;
        if(mergeEncounter)
        {
            // If the entities should all be considered a single encounter,
            // add them all to the same grouping
            if(eStateGroups.size() == 0)
            {
                eStateGroups.push_front(
                    std::list<std::shared_ptr<ActiveEntityState>>());
            }
            eStateGroup = &eStateGroups.front();
        }
        else if(sg->DefeatActionsCount() == 0)
        {
            if(!containsSimpleSpawns)
            {
                eStateGroups.push_front(
                    std::list<std::shared_ptr<ActiveEntityState>>());
                containsSimpleSpawns = true;
            }
            eStateGroup = &eStateGroups.front();
        }
        else
        {
            eStateGroups.push_back(
                std::list<std::shared_ptr<ActiveEntityState>>());
            eStateGroup = &eStateGroups.back();
        }

        bool isSpread = slg && slg->GetSpotSelection() ==
            objects::SpawnLocationGroup::SpotSelection_t::SPREAD;

        // Create each entity at a random position in the same location
        std::shared_ptr<channel::ZoneSpotShape> spot;
        std::shared_ptr<objects::SpawnLocation> location;

        if(!isSpread && !SelectSpotAndLocation(useSpotID, spotID, spotIDs,
            spot, location, dynamicMap, zoneDef, locations))
        {
            LOG_ERROR(libcomp::String("Failed to spawn group %1 at"
                " unknown spot %2\n").Arg(sgID).Arg(spotID));

            continue;
        }

        bool locationFailed = false;

        for(auto sPair : sg->GetSpawns())
        {
            auto spawn = zoneDef->GetSpawns(sPair.first);
            for(uint16_t i = 0; i < sPair.second; i++)
            {
                if(isSpread && !SelectSpotAndLocation(useSpotID, spotID,
                    spotIDs, spot, location, dynamicMap, zoneDef, locations))
                {
                    LOG_ERROR(libcomp::String("Failed to spawn group %1 at"
                        " unknown spot %2\n").Arg(sgID).Arg(spotID));

                    locationFailed = true;
                    break;
                }

                float x = 0.f, y = 0.f;
                if(useSpotID)
                {
                    // Get a random point in the polygon
                    Point p = GetRandomSpotPoint(spot->Definition);
                    Point center(spot->Definition->GetCenterX(),
                        spot->Definition->GetCenterY());

                    // Make sure a straight line can be drawn from the center
                    // point so the enemy is not spawned outside of the zone
                    Point collision;
                    Line fromCenter(center, p);

                    if(zone->Collides(fromCenter, collision))
                    {
                        // Back it off slightly
                        p = GetLinearPoint(collision.x, collision.y,
                            center.x, center.y, 10.f, false);
                    }

                    x = p.x;
                    y = p.y;
                }
                else
                {
                    // Spawn location bounding box points start in the top left corner of the
                    // rectangle and extend towards +X/-Y
                    auto rPoint = GetRandomPoint(location->GetWidth(), location->GetHeight());
                    x = location->GetX() + rPoint.x;
                    y = location->GetY() - rPoint.y;
                }

                float rot = RNG_DEC(float, 0.f, 3.14f, 2);

                // Create the entity state
                auto state = CreateEnemy(zone, spawn->GetEnemyType(), spawn, x, y, rot);

                // Set the spawn information
                auto eBase = state->GetEnemyBase();
                eBase->SetSpawnLocation(location);
                eBase->SetSpawnSpotID(spotID);
                eBase->SetSpawnGroupID(sgID);
                eBase->SetSpawnLocationGroupID(slgID);

                eStateGroup->push_back(state);

                // If this is a spread clear the spot ID so we start again.
                if(isSpread)
                {
                    spotID = 0;
                }
            }

            if(locationFailed)
            {
                break;
            }
        }

        if(locationFailed)
        {
            continue;
        }

        if(sg->SpawnActionsCount() > 0)
        {
            spawnActionGroups.push_back(sg);
        }
    }

    if(eStateGroups.size() > 0)
    {
        auto server = mServer.lock();
        auto aiManager = server->GetAIManager();
        for(auto& eStateGroup : eStateGroups)
        {
            bool encounterSpawn = !containsSimpleSpawns || eStateGroup != eStateGroups.front();
            for(auto eState : eStateGroup)
            {
                auto spawn = eState->GetEnemyBase()->GetSpawnSource();
                if(!aiManager->Prepare(eState, spawn->GetAIScriptID(), spawn->GetAggression()))
                {
                    LOG_ERROR(libcomp::String("Failed to prepare AI for enemy: %1\n")
                        .Arg(spawn->GetAIScriptID()));
                }

                if(!encounterSpawn)
                {
                    if(eState->GetEntityType() == EntityType_t::ENEMY)
                    {
                        zone->AddEnemy(std::dynamic_pointer_cast<EnemyState>(eState));
                    }
                    else
                    {
                        zone->AddAlly(std::dynamic_pointer_cast<AllyState>(eState));
                    }
                }
            }

            if(encounterSpawn)
            {
                zone->CreateEncounter(eStateGroup, actionSource);
            }

            TriggerZoneActions(zone, eStateGroup, ZoneTrigger_t::ON_SPAWN);
        }

        // Send to clients already in the zone if they exist
        auto clients = zone->GetConnections();
        if(clients.size() > 0)
        {
            auto firstClient = clients.begin()->second;
            for(auto& eStateGroup : eStateGroups)
            {
                for(auto& eState : eStateGroup)
                {
                    if(eState->GetEntityType() == EntityType_t::ENEMY)
                    {
                        auto e = std::dynamic_pointer_cast<EnemyState>(eState);
                        SendEnemyData(firstClient, e, zone, true, true);
                    }
                    else
                    {
                        auto a = std::dynamic_pointer_cast<AllyState>(eState);
                        SendAllyData(firstClient, a, zone, true, true);
                    }
                }
            }

            for(auto client : clients)
            {
                client.second->FlushOutgoing();
            }
        }

        for(auto sg : spawnActionGroups)
        {
            server->GetActionManager()->PerformActions(nullptr,
                sg->GetSpawnActions(), 0, zone, sg->GetID());
        }

        return true;
    }

    return false;
}

bool ZoneManager::SelectSpotAndLocation(bool useSpotID, uint32_t& spotID,
    const std::set<uint32_t>& spotIDs,
    std::shared_ptr<channel::ZoneSpotShape>& spot,
    std::shared_ptr<objects::SpawnLocation>& location,
    std::shared_ptr<DynamicMap>& dynamicMap,
    std::shared_ptr<objects::ServerZone>& zoneDef,
    std::list<std::shared_ptr<objects::SpawnLocation>>& locations)
{
    if(useSpotID)
    {
        if(!spotID)
        {
            spotID = libcomp::Randomizer::GetEntry(spotIDs);
        }

        auto spotPair = dynamicMap->Spots.find(spotID);
        if(spotPair != dynamicMap->Spots.end())
        {
            spot = spotPair->second;

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
            return false;
        }
    }
    else
    {
        location = libcomp::Randomizer::GetEntry(locations);
    }

    return true;
}

bool ZoneManager::UpdatePlasma(const std::shared_ptr<Zone>& zone, uint64_t now)
{
    if(zone->GetDefinition()->PlasmaSpawnsCount() == 0)
    {
        return false;
    }

    auto spots = mServer.lock()->GetDefinitionManager()
        ->GetSpotData(zone->GetDefinition()->GetDynamicMapID());
    for(auto plasmaPair : zone->GetPlasma())
    {
        auto pState = plasmaPair.second;
        auto pSpawn = pState->GetEntity();

        if(pState->HasStateChangePoints(true, now))
        {
            auto spotIter = spots.find(pSpawn->GetSpotID());

            auto hiddenPoints = pState->PopRespawnPoints(now);

            libcomp::Packet notify;
            notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_REPOP);
            notify.WriteS32Little(pState->GetEntityID());
            notify.WriteS8((int8_t)hiddenPoints.size());

            for(auto point : hiddenPoints)
            {
                if(spotIter != spots.end())
                {
                    Point rPoint = GetRandomSpotPoint(spotIter->second);
                    point->SetX(rPoint.x);
                    point->SetY(rPoint.y);
                }
                else
                {
                    // Default to the explicit location
                    point->SetX(pState->GetCurrentX());
                    point->SetY(pState->GetCurrentY());
                }

                point->Refresh();

                notify.WriteS8((int8_t)point->GetID());
                notify.WriteS32Little(point->GetState());

                notify.WriteFloat(point->GetX());
                notify.WriteFloat(point->GetY());
                notify.WriteFloat(point->GetRotation());
            }

            BroadcastPacket(zone, notify);
        }
        
        if(pState->HasStateChangePoints(false, now))
        {
            std::list<uint32_t> pointIDs;
            for(auto hidePoint : pState->PopHidePoints(now))
            {
                pointIDs.push_back(hidePoint->GetID());
            }

            if(pointIDs.size() > 0)
            {
                libcomp::Packet notify;
                pState->GetPointStatusData(notify, pointIDs);

                BroadcastPacket(zone, notify);
            }
        }
    }

    return true;
}

Point ZoneManager::RotatePoint(const Point& p, const Point& origin, float radians)
{
    float xDelta = p.x - origin.x;
    float yDelta = p.y - origin.y;

    return Point((float)((xDelta * cos(radians)) - (yDelta * sin(radians))) + origin.x,
        (float)((xDelta * sin(radians)) + (yDelta * cos(radians))) + origin.y);
}

std::shared_ptr<ActiveEntityState> ZoneManager::CreateEnemy(
    const std::shared_ptr<Zone>& zone, uint32_t demonID,
    const std::shared_ptr<objects::Spawn>& spawn, float x, float y, float rot)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetDevilData(demonID);

    if(nullptr == def)
    {
        return nullptr;
    }

    auto stats = libcomp::PersistentObject::New<objects::EntityStats>();
    stats->SetLevel(spawn && spawn->GetLevel() > 0 ? spawn->GetLevel()
        : (int8_t)def->GetGrowth()->GetBaseLevel());
    server->GetCharacterManager()->CalculateDemonBaseStats(nullptr, stats, def);

    std::shared_ptr<ActiveEntityState> state;
    std::shared_ptr<objects::EnemyBase> eBase;
    if(!spawn || !spawn->GetIsAlly())
    {
        // Building an enemy
        auto enemy = std::shared_ptr<objects::Enemy>(new objects::Enemy);
        enemy->SetCoreStats(stats);
        enemy->SetType(demonID);
        enemy->SetVariantType(spawn ? spawn->GetVariantType() : 0);
        enemy->SetSpawnSource(spawn);
        eBase = enemy;

        auto eState = std::shared_ptr<EnemyState>(new EnemyState);
        eState->SetEntity(enemy, def);
        state = eState;
    }
    else
    {
        // Building an ally
        auto ally = std::shared_ptr<objects::Ally>(new objects::Ally);
        ally->SetCoreStats(stats);
        ally->SetType(demonID);
        ally->SetVariantType(spawn ? spawn->GetVariantType() : 0);
        ally->SetSpawnSource(spawn);
        eBase = ally;

        auto aState = std::shared_ptr<AllyState>(new AllyState);
        aState->SetEntity(ally, def);
        state = aState;
    }

    state->SetEntityID(server->GetNextEntityID());
    state->SetOriginX(x);
    state->SetOriginY(y);
    state->SetOriginRotation(rot);
    state->SetDestinationX(x);
    state->SetDestinationY(y);
    state->SetDestinationRotation(rot);
    state->SetCurrentX(x);
    state->SetCurrentY(y);
    state->SetCurrentRotation(rot);
    state->SetStatusEffectsActive(true, definitionManager);
    state->SetZone(zone);

    server->GetTokuseiManager()->Recalculate(state);
    state->RecalculateStats(definitionManager);

    // Reset HP to max to account for extra HP boosts
    stats->SetHP(state->GetMaxHP());

    return state;
}

void ZoneManager::UpdateActiveZoneStates()
{
    std::list<std::shared_ptr<Zone>> zones;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto uniqueID : mActiveZones)
        {
            zones.push_back(mZones[uniqueID]);
        }
    }

    auto server = mServer.lock();

    // Spin through entities with updated status effects
    auto worldClock = server->GetWorldClockTime();
    for(auto zone : zones)
    {
        UpdateStatusEffectStates(zone,
            worldClock.SystemTime);
    }

    auto serverTime = ChannelServer::GetServerTime();
    auto aiManager = mServer.lock()->GetAIManager();

    bool isNight = worldClock.IsNight();

    for(auto zone : zones)
    {
        // Despawn first
        HandleDespawns(zone);

        // Update active AI controlled entities
        aiManager->UpdateActiveStates(zone, serverTime, isNight);

        if(zone->HasRespawns())
        {
            // Spawn new enemies next (since they should not immediately act)
            UpdateSpawnGroups(zone, false, serverTime);

            // Now update plasma spawns
            UpdatePlasma(zone, serverTime);
        }

        mTimeRestrictUpdatedZones.erase(zone->GetID());
    }

    // Get any updated time restricted zones and clear the list
    // after retrieval (essentially they "unfreeze" momentarily)
    {
        zones.clear();

        std::lock_guard<std::mutex> lock(mLock);
        if(mTimeRestrictUpdatedZones.size() > 0)
        {
            for(auto uniqueID : mTimeRestrictUpdatedZones)
            {
                zones.push_back(mZones[uniqueID]);
            }

            mTimeRestrictUpdatedZones.clear();
        }
    }

    // Handle all time restrict updated zones
    for(auto zone : zones)
    {
        // Despawn first
        HandleDespawns(zone);

        if(zone->HasRespawns())
        {
            // Spawn next
            UpdateSpawnGroups(zone, false, serverTime);
        }
    }
}

void ZoneManager::Warp(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<ActiveEntityState>& eState, float xPos, float yPos,
    float rot)
{
    ServerTime timestamp = ChannelServer::GetServerTime();

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

    RelativeTimeMap timeMap;
    timeMap[p.Size()] = timestamp;

    auto connections = GetZoneConnections(client, true);
    ChannelClientConnection::SendRelativeTimePacket(connections, p, timeMap);
}

void ZoneManager::HandleTimedActions(const WorldClock& clock,
    const WorldClockTime& lastTrigger)
{
    std::list<std::shared_ptr<Zone>> timeRestrictZones;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(uint32_t zoneID : mAllTimeRestrictZones)
        {
            auto zone = mZones[zoneID];
            if(zone)
            {
                timeRestrictZones.push_back(zone);
            }
        }
    }

    std::set<uint32_t> updated;
    for(auto zone : timeRestrictZones)
    {
        if(zone->UpdateTimedSpawns(clock))
        {
            updated.insert(zone->GetID());
        }
    }

    // Fire zone time event triggers
    if(lastTrigger.IsSet())
    {
        int32_t timeFrom = (int32_t)(lastTrigger.Hour * 100 +
            lastTrigger.Min);
        int32_t timeTo = (int32_t)(clock.Hour * 100 +
            clock.Min);
        bool timeChange = timeTo != timeFrom;
        bool timeRoll = timeTo < timeFrom;

        int32_t sTimeFrom = (int32_t)(lastTrigger.SystemHour * 100 +
            lastTrigger.SystemMin);
        int32_t sTimeTo = (int32_t)(clock.SystemHour * 100 +
            clock.SystemMin);
        bool sTimeChange = sTimeTo != sTimeFrom;
        bool sTimeRoll = sTimeTo < sTimeFrom;

        bool moonChange = clock.MoonPhase != lastTrigger.MoonPhase;
        bool moonRoll = clock.MoonPhase < lastTrigger.MoonPhase;

        for(auto zone : timeRestrictZones)
        {
            for(auto trigger : zone->GetTimeTriggers())
            {
                int32_t from = 0;
                int32_t to = 0;
                bool rollOver = false;

                switch(trigger->GetTrigger())
                {
                case objects::ServerZoneTrigger::Trigger_t::ON_TIME:
                    if(!timeChange) continue;
                    from = timeFrom;
                    to = timeTo;
                    rollOver = timeRoll;
                    break;
                case objects::ServerZoneTrigger::Trigger_t::ON_SYSTEMTIME:
                    if(!sTimeChange) continue;
                    from = sTimeFrom;
                    to = sTimeTo;
                    rollOver = sTimeRoll;
                    break;
                case objects::ServerZoneTrigger::Trigger_t::ON_MOONPHASE:
                    if(!moonChange) continue;
                    from = lastTrigger.MoonPhase;
                    to = clock.MoonPhase;
                    rollOver = moonRoll;
                    break;
                default:
                    break;
                }

                int32_t val = trigger->GetValue();
                if((!rollOver && from < val && val <= to) ||
                    (rollOver && (from < val || val <= to)))
                {
                    LOG_DEBUG(libcomp::String("Triggering timed actions"
                        " in zone %2\n").Arg(zone->GetDefinitionID()));

                    mServer.lock()->GetActionManager()->PerformActions(nullptr,
                        trigger->GetActions(), 0, zone, 0);
                    updated.insert(zone->GetID());
                }
            }
        }
    }

    if(updated.size() > 0)
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(uint32_t zoneID : updated)
        {
            mTimeRestrictUpdatedZones.insert(zoneID);
        }
    }
}

bool ZoneManager::StartInstanceTimer(const std::shared_ptr<ZoneInstance>& instance)
{
    if(!instance)
    {
        return false;
    }

    auto instVariant = instance->GetVariant();
    auto instType = instVariant ? instVariant->GetInstanceType()
        : InstanceType_t::NORMAL;

    bool sendTimer = false;
    int32_t scheduleExpiration = 0;
    switch(instType)
    {
    case InstanceType_t::TIME_TRIAL:
        {
            // Timer counts up, set start time only
            std::lock_guard<std::mutex> lock(mLock);
            if(instance->GetTimerStart() == 0)
            {
                uint64_t now = ChannelServer::GetServerTime();
                uint16_t duration = instVariant->GetTimePoints(0);

                uint64_t expireTime = now + (uint64_t)duration * 1000000ULL;
                instance->SetTimerStart(now);
                instance->SetTimerExpire(expireTime);

                scheduleExpiration = (int32_t)duration;
            }
        }
        break;
    case InstanceType_t::DEMON_ONLY:
        {
            // Timer counts down, set start and expire time
            std::lock_guard<std::mutex> lock(mLock);
            if(instance->GetTimerStart() == 0)
            {
                uint64_t now = ChannelServer::GetServerTime();
                uint16_t duration = instVariant->GetTimePoints(0);

                size_t partyMemberCount = instance->OriginalAccessCIDsCount();
                if(partyMemberCount > 1)
                {
                    // Reduce the time by a calculated or fixed amount based
                    // on how many party members existed on instance creation
                    uint16_t reduction = 0;
                    if(instVariant->TimePointsCount() == 4)
                    {
                        reduction = instVariant->GetTimePoints(3);
                    }
                    else
                    {
                        reduction = (uint16_t)(
                            floor((float)duration / 600.f) * 30);
                    }

                    duration = (uint16_t)(duration - (reduction *
                        (partyMemberCount - 1)));
                }

                uint64_t expireTime = now + (uint64_t)duration * 1000000ULL;
                instance->SetTimerStart(now);
                instance->SetTimerExpire(expireTime);

                // Add a 1 second buffer so we don't undershoot client times
                scheduleExpiration = (int32_t)(duration + 1);
            }
        }
        break;
    case InstanceType_t::NORMAL:
    default:
        {
            auto timeLimitData = instance->GetTimeLimitData();
            if(!timeLimitData)
            {
                return false;
            }

            // Timer counts down, set start and expire time from time limit
            std::lock_guard<std::mutex> lock(mLock);
            if(instance->GetTimerStart() == 0)
            {
                uint64_t now = ChannelServer::GetServerTime();
                uint16_t duration = timeLimitData->GetDuration();

                uint64_t expireTime = now + (uint64_t)duration * 1000000ULL;
                instance->SetTimerStart(now);
                instance->SetTimerExpire(expireTime);

                // Add a 1 second buffer so we don't undershoot client times
                scheduleExpiration = (int32_t)(duration + 1);
                sendTimer = true;
            }
        }
        break;
    }

    if(scheduleExpiration)
    {
        // When the instance timer expires, stop the normal timer and let it
        // handle all time out events
        mServer.lock()->GetTimerManager()->ScheduleEventIn(
            (int)scheduleExpiration, []
            (ZoneManager* pZoneManager, uint32_t pInstanceID)
        {
            auto pInstance = pZoneManager->GetInstance(pInstanceID);
            if(pInstance && !pInstance->GetTimerStop() &&
                pInstance->GetTimerExpire() <= ChannelServer::GetServerTime())
            {
                LOG_DEBUG(libcomp::String("Expiring instance timer: %1\n")
                    .Arg(pInstanceID));
                pZoneManager->StopInstanceTimer(pInstance);
            }
        }, this, instance->GetID());
    }

    if(sendTimer)
    {
        SendInstanceTimer(instance, nullptr, false);
    }

    return true;
}

bool ZoneManager::StopInstanceTimer(const std::shared_ptr<ZoneInstance>& instance)
{
    if(!instance)
    {
        return false;
    }

    auto instVariant = instance->GetVariant();
    auto instType = instVariant ? instVariant->GetInstanceType()
        : InstanceType_t::NORMAL;

    bool expired = false;
    switch(instType)
    {
    case InstanceType_t::TIME_TRIAL:
        {
            std::lock_guard<std::mutex> lock(mLock);
            if(!instance->GetTimerStop())
            {
                uint64_t now = ChannelServer::GetServerTime();
                instance->SetTimerStop(now);

                if(instance->GetTimerExpire() <= now)
                {
                    // Instance timer expired (client actually handles the
                    // time-out display)
                    instance->SetTimerStop(instance->GetTimerExpire());
                    expired = true;
                }
                else
                {
                    // Complete time trial
                    for(auto client : instance->GetConnections())
                    {
                        EndInstanceTimer(instance, client, true);
                    }
                }
            }
        }
        break;
    case InstanceType_t::DEMON_ONLY:
        {
            std::lock_guard<std::mutex> lock(mLock);
            if(instance->GetTimerExpire() && !instance->GetTimerStop())
            {
                uint64_t now = ChannelServer::GetServerTime();
                instance->SetTimerStop(now);

                if(instance->GetTimerExpire() <= now)
                {
                    // Instance timer expired
                    instance->SetTimerStop(instance->GetTimerExpire());
                    expired = true;

                    for(auto client : instance->GetConnections())
                    {
                        EndInstanceTimer(instance, client, false);
                    }

                    // Kick everyone in 5 seconds
                    mServer.lock()->GetTimerManager()->ScheduleEventIn(5, []
                        (ZoneManager* pZoneManager, uint32_t pInstanceID)
                    {
                        auto pInstance = pZoneManager->GetInstance(pInstanceID);
                        if(pInstance)
                        {
                            for(auto connection : pInstance->GetConnections())
                            {
                                pZoneManager->EnterZone(connection,
                                    pInstance->GetDefinition()->GetLobbyID(), 0);
                            }
                        }
                    }, this, instance->GetID());
                }
                else
                {
                    // Instance timer success
                    for(auto client : instance->GetConnections())
                    {
                        EndInstanceTimer(instance, client, true);
                    }
                }
            }
        }
        break;
    case InstanceType_t::NORMAL:
    default:
        {
            auto timeLimitData = instance->GetTimeLimitData();
            if(!timeLimitData)
            {
                return false;
            }

            bool stopped = false;
            {
                std::lock_guard<std::mutex> lock(mLock);
                if(!instance->GetTimerStop())
                {
                    uint64_t now = ChannelServer::GetServerTime();
                    instance->SetTimerStop(now);

                    stopped = true;
                    if(instance->GetTimerExpire() <= now)
                    {
                        instance->SetTimerStop(instance->GetTimerExpire());
                        expired = true;
                    }
                }
            }

            if(stopped && !expired)
            {
                for(auto client : instance->GetConnections())
                {
                    EndInstanceTimer(instance, client, false);
                }
            }
        }
        break;
    }

    if(expired && !instance->GetTimerExpirationEventID().IsEmpty())
    {
        // Fire the expiration event once per client
        auto eventManager = mServer.lock()->GetEventManager();
        for(auto client : instance->GetConnections())
        {
            auto state = client->GetClientState();
            auto entityID = state->GetCharacterState()->GetEntityID();
            eventManager->HandleEvent(client,
                instance->GetTimerExpirationEventID(), entityID);
        }
    }

    return true;
}

void ZoneManager::SendInstanceTimer(const std::shared_ptr<ZoneInstance>& instance,
    const std::shared_ptr<ChannelClientConnection>& client, bool queue)
{
    if(!instance || !instance->GetTimerStart())
    {
        return;
    }

    auto instVariant = instance->GetVariant();
    auto instType = instVariant ? instVariant->GetInstanceType()
        : InstanceType_t::NORMAL;

    libcomp::Packet p;
    RelativeTimeMap timeMap;

    switch(instType)
    {
    case InstanceType_t::TIME_TRIAL:
        {
            p.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TIME_TRIAL_UPDATE);
            p.WriteS8((int8_t)instVariant->GetSubID());

            timeMap[p.Size()] = instance->GetTimerStart();
            p.WriteFloat(0.f);

            p.WriteS32Little(0);    // Unknown
            p.WriteBlank(32);       // Unknown array (phases?)
            p.WriteU8(1);           // Unknown bool
        }
        break;
    case InstanceType_t::DEMON_ONLY:
        {
            uint64_t now = ChannelServer::GetServerTime();

            uint64_t expireTime = instance->GetTimerExpire();
            float timeLeft = expireTime < now ? 0.f
                : (float)((double)(expireTime - now) / 1000000.0);

            p.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_DEMON_SOLO_UPDATE);
            p.WriteU32Little(instVariant->GetSubID());
            p.WriteFloat(timeLeft);
            p.WriteS32Little(1);
            p.WriteS32Little((int32_t)instance->GetTimerID());
        }
        break;
    case InstanceType_t::NORMAL:
    default:
        {
            auto timeLimitData = instance->GetTimeLimitData();
            if(!timeLimitData || instance->GetTimerStop())
            {
                return;
            }

            p.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TIME_LIMIT_UPDATE);
            p.WriteS8((int8_t)timeLimitData->GetID());

            timeMap[p.Size()] = instance->GetTimerStart();
            p.WriteFloat(0.f);

            p.WriteU8(1);           // Unknown bool
            p.WriteU8(0);           // Unknown bool
        }
    }

    if(timeMap.size() > 0)
    {
        std::list<std::shared_ptr<ChannelClientConnection>> connections;
        if(client)
        {
            connections.push_back(client);
        }
        else
        {
            connections = instance->GetConnections();
        }

        ChannelClientConnection::SendRelativeTimePacket(connections, p,
            timeMap, queue);
    }
    else if(client)
    {
        if(queue)
        {
            client->QueuePacket(p);
        }
        else
        {
            client->SendPacket(p);
        }
    }
    else
    {
        auto connections = instance->GetConnections();
        ChannelClientConnection::BroadcastPacket(connections, p);
    }
}

void ZoneManager::UpdateDeathTimeOut(ClientState* state, int32_t time,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    if(!state)
    {
        return;
    }

    auto zone = state->GetZone();
    if(!zone)
    {
        return;
    }

    auto dState = state->GetDemonState();
    auto instance = zone ? zone->GetInstance() : nullptr;
    auto instVariant = instance ? instance->GetVariant() : nullptr;

    float timeLeft = 0.f;
    uint64_t deathTime = dState->GetDeathTimeOut();
    if(time == -1)
    {
        // Removing timeout
        dState->SetDeathTimeOut(0);
        deathTime = 0;
    }
    else if(time != 0)
    {
        // Setting timeout
        uint64_t now = ChannelServer::GetServerTime();

        deathTime = now + (uint64_t)time * (uint64_t)1000000;
        dState->SetDeathTimeOut(deathTime);
        timeLeft = (float)time;

        auto killedClient = mServer.lock()->GetManagerConnection()
            ->GetEntityClient(state->GetCharacterState()->GetEntityID());
        if(killedClient)
        {
            HandleDeathTimeOut(instance, killedClient, 0);
        }
    }
    else
    {
        // Calculate remaining time
        uint64_t now = ChannelServer::GetServerTime();

        timeLeft = (float)((double)(now - deathTime) / 1000000.0);
    }

    libcomp::Packet p;
    p.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_DEMON_SOLO_DEATH_TIME);
    p.WriteU32Little(instVariant ? instVariant->GetSubID() : 0);
    p.WriteS32Little(state->GetDemonState()->GetEntityID());
    p.WriteFloat(timeLeft);

    if(client)
    {
        client->SendPacket(p);
    }
    else
    {
        BroadcastPacket(zone, p);
    }
}

void ZoneManager::HandleDeathTimeOut(
    const std::shared_ptr<ZoneInstance>& instance,
    const std::shared_ptr<ChannelClientConnection>& client,
    uint64_t deathTime)
{
    auto state = client->GetClientState();
    auto dState = state->GetDemonState();
    auto zone = state->GetZone();
    if(!zone || zone->GetInstance() != instance ||
        (deathTime && dState->GetDeathTimeOut() != deathTime))
    {
        // No longer valid
        return;
    }

    if(deathTime)
    {
        // Disable revival and schedule removal in 5 seconds
        state->SetAcceptRevival(false);

        EndInstanceTimer(instance, client, false, false);

        mServer.lock()->GetTimerManager()->ScheduleEventIn(5, []
            (ZoneManager* pZoneManager,
                std::shared_ptr<ChannelClientConnection> pClient,
                uint32_t pInstanceID)
        {
            auto pState = pClient->GetClientState();
            auto pZone = pState->GetZone();
            auto pInstance = pZoneManager->GetInstance(pInstanceID);
            if(pInstance && pZone && pZone->GetInstance() == pInstance)
            {
                pZoneManager->EnterZone(pClient,
                    pInstance->GetDefinition()->GetLobbyID(), 0);
            }
        }, this, client, instance->GetID());
    }
    else
    {
        // Schedule the death time-out
        uint64_t now = ChannelServer::GetServerTime();
        deathTime = dState->GetDeathTimeOut();

        int timeLeft = (int)(
            (deathTime > now ? (deathTime - now) : 0) / 1000000);
        mServer.lock()->GetTimerManager()->ScheduleEventIn(timeLeft, []
            (ZoneManager* pZoneManager,
                std::shared_ptr<ChannelClientConnection> pClient,
                uint32_t pInstanceID, uint64_t pDeathTime)
        {
            auto pInstance = pZoneManager->GetInstance(pInstanceID);
            if(pInstance)
            {
                pZoneManager->HandleDeathTimeOut(pInstance, pClient,
                    pDeathTime);
            }
        }, this, client, instance->GetID(), deathTime);
    }
}

void ZoneManager::EndInstanceTimer(
    const std::shared_ptr<ZoneInstance>& instance,
    const std::shared_ptr<ChannelClientConnection>& client,
    bool isSuccess, bool queue)
{
    auto state = client->GetClientState();
    auto zone = state->GetZone();
    if(!zone || !instance)
    {
        return;
    }

    auto instVariant = instance->GetVariant();
    auto instType = instVariant ? instVariant->GetInstanceType()
        : InstanceType_t::NORMAL;

    libcomp::Packet notify;

    switch(instType)
    {
    case InstanceType_t::TIME_TRIAL:
        {
            // Quit the trial if the timer has not stopped yet
            bool quit = !instance->GetTimerStop();
            if(isSuccess == quit)
            {
                return;
            }

            auto cState = state->GetCharacterState();
            auto character = cState->GetEntity();
            auto progress = character ? character->GetProgress().Get() : nullptr;
            if(isSuccess && progress)
            {
                uint16_t timeResult = (uint16_t)((instance->GetTimerStop() -
                    instance->GetTimerStart()) / 1000000);

                // Get the best result applicable
                int8_t result = (int8_t)
                    objects::CharacterProgress::TimeTrialResult_t::FAILED;
                for(size_t rank = 0; rank < 4; rank++)
                {
                    uint16_t timePoint = instVariant->GetTimePoints(rank);
                    if(timeResult > timePoint ||
                        (rank == 0 && timeResult == timePoint))
                    {
                        // Times that match the expiration exactly still
                        // count as failures
                        break;
                    }

                    result--;
                }

                if(result == (int8_t)
                    objects::CharacterProgress::TimeTrialResult_t::FAILED)
                {
                    // Do nothing
                    return;
                }
                else
                {
                    // Set the state and register the time
                    int8_t trialID = (int8_t)instVariant->GetSubID();
                    progress->SetTimeTrialID(trialID);
                    progress->SetTimeTrialTime(timeResult);

                    uint16_t previousTime = progress->GetTimeTrialRecords(
                        (size_t)(trialID - 1));

                    bool newBest = previousTime == 0;
                    bool firstA = previousTime == 0 && result == 1;
                    if(previousTime && previousTime > timeResult)
                    {
                        newBest = true;

                        // If the previous best time was not an A-rank and the
                        // new one is, set first A-rank
                        firstA = previousTime > instVariant->GetTimePoints(3) &&
                            result == 1;
                    }

                    if(firstA)
                    {
                        result = (int8_t)objects::CharacterProgress::
                            TimeTrialResult_t::A_RANK_FIRST;
                    }

                    progress->SetTimeTrialResult(
                        (objects::CharacterProgress::TimeTrialResult_t)result);

                    if(newBest)
                    {
                        // Report the new best time to others in the instance
                        std::list<std::shared_ptr<
                            ChannelClientConnection>> connections;
                        for(auto connection : instance->GetConnections())
                        {
                            if(connection != client)
                            {
                                connections.push_back(connection);
                            }
                        }

                        if(connections.size() > 0)
                        {
                            libcomp::Packet p;
                            p.WritePacketCode(ChannelToClientPacketCode_t::
                                PACKET_DUNGEON_RECORDS_UPDATE);
                            p.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                                character->GetName(), true);

                            ChannelClientConnection::BroadcastPacket(connections, p);
                        }
                    }

                    mServer.lock()->GetWorldDatabase()->QueueUpdate(progress,
                        state->GetAccountUID());
                }
            }
            else
            {
                quit = true;
            }

            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TIME_TRIAL_END);

            if(!quit)
            {
                notify.WriteS8((int8_t)instVariant->GetSubID());
                notify.WriteFloat(state->ToClientTime(instance->GetTimerStart()));
                notify.WriteFloat(state->ToClientTime(instance->GetTimerStop()));
                notify.WriteS8(0);
            }
            else
            {
                notify.WriteS8(-1);
                notify.WriteFloat(0);
                notify.WriteFloat(0);
                notify.WriteS8(1);
            }
        }
        break;
    case InstanceType_t::DEMON_ONLY:
        {
            // A player successfully completed the instance if they're in it
            // when the timer stops before the expiration, whether they're
            // alive or not
            bool success = zone->GetInstance() == instance &&
                instance->GetTimerStop() &&
                instance->GetTimerStop() < instance->GetTimerExpire();
            if(isSuccess != success)
            {
                return;
            }

            if(success && state->GetDemonState()->GetDeathTimeOut())
            {
                // Remove the death time-out
                UpdateDeathTimeOut(state, -1);
            }

            float timeLeft = 0.f;
            int32_t rank = -1;
            int32_t spGain = 0;
            if(success)
            {
                uint16_t rankA = instVariant->GetTimePoints(1);
                uint16_t rankB = instVariant->GetTimePoints(2);

                uint16_t maxAdjusted = (uint16_t)((instance->GetTimerExpire() -
                    instance->GetTimerStart()) / 1000000ULL);
                if(maxAdjusted != instVariant->GetTimePoints(0))
                {
                    double modifier = (double)maxAdjusted /
                        (double)instVariant->GetTimePoints(0);
                    rankA = (uint16_t)((double)rankA * modifier);
                    rankB = (uint16_t)((double)rankB * modifier);
                }

                timeLeft = (float)((double)(instance->GetTimerExpire() -
                    instance->GetTimerStop()) / 1000000.0);

                if(timeLeft > (float)rankA)
                {
                    // Rank A
                    rank = 0;
                }
                else if(timeLeft > (float)rankB)
                {
                    // Rank B
                    rank = 1;
                }
                else
                {
                    // Rank C
                    rank = 2;
                }

                spGain = (int32_t)instVariant->GetFixedReward();

                int32_t rewardModifier = instVariant->GetRewardModifier();
                if(rewardModifier)
                {
                    // Increase by 10% per additional party member, decrease by
                    // percentage of time used
                    size_t partyMemberCount = instance->OriginalAccessCIDsCount();
                    double boost = (double)(partyMemberCount - 1) * 0.1;
                    float timePercentLeft = timeLeft / (float)maxAdjusted;

                    spGain = spGain + (int32_t)ceil((double)rewardModifier *
                        (1.0 + boost) * (double)timePercentLeft);
                }

                spGain = mServer.lock()->GetCharacterManager()
                    ->UpdateSoulPoints(client, spGain, true, true);
            }

            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_DEMON_SOLO_END);
            notify.WriteU32Little(instVariant->GetSubID());
            notify.WriteS32Little(success ? 0 : 1);
            notify.WriteFloat(timeLeft);
            notify.WriteS32Little(rank);
            notify.WriteS32Little(spGain);
        }
        break;
    case InstanceType_t::NORMAL:
    default:
        {
            // Time limits do not have a success or fail state
            auto timeLimitData = instance->GetTimeLimitData();
            if(!timeLimitData)
            {
                return;
            }

            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TIME_LIMIT_END);
            notify.WriteU32Little(timeLimitData->GetID());
            notify.WriteS8(0);  // Unknown
            notify.WriteU8(1);  // Unknown bool
        }
        break;
    }

    if(queue)
    {
        client->QueuePacket(notify);
    }
    else
    {
        client->SendPacket(notify);
    }
}

bool ZoneManager::TriggerZoneActions(const std::shared_ptr<Zone>& zone,
    std::list<std::shared_ptr<ActiveEntityState>> entities,
    ZoneTrigger_t trigger,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    bool executed = false;

    std::list<std::shared_ptr<objects::ServerZoneTrigger>> triggers;
    switch(trigger)
    {
    case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_IN:
        triggers = zone->GetZoneInTriggers();
        break;
    case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_OUT:
        triggers = zone->GetZoneOutTriggers();
        break;
    case objects::ServerZoneTrigger::Trigger_t::ON_SPAWN:
        triggers = zone->GetSpawnTriggers();
        break;
    case objects::ServerZoneTrigger::Trigger_t::ON_DEATH:
        triggers = zone->GetDeathTriggers();
        break;
    case objects::ServerZoneTrigger::Trigger_t::ON_REVIVAL:
        triggers = zone->GetRevivalTriggers();
        break;
    default:
        // Other types handled elsewhere
        return false;
        break;
    }

    if(triggers.size() > 0)
    {
        auto actionManager = mServer.lock()->GetActionManager();
        for(auto e : entities)
        {
            int32_t entityID = e->GetEntityID();
            if(e->Ready(true))
            {
                for(auto tr : triggers)
                {
                    actionManager->PerformActions(client,
                        tr->GetActions(), entityID, zone, 0);
                    executed = true;
                }
            }
        }
    }

    return executed;
}

bool ZoneManager::UpdateGeometryElement(const std::shared_ptr<Zone>& zone,
    std::shared_ptr<objects::ServerObject> elemObject)
{
    auto geometry = zone->GetGeometry();
    if(geometry)
    {
        bool updated = false;

        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();

        auto objDef = definitionManager->GetONPCData(elemObject->GetID());
        if(objDef && !objDef->GetBarrierName().IsEmpty())
        {
            bool disabled = IsGeometryDisabled(elemObject);

            libcomp::String name = objDef->GetBarrierName();
            for(auto elem : geometry->Elements)
            {
                if(elem->GetName() == name)
                {
                    if(disabled)
                    {
                        zone->InsertDisabledBarriers(elem->GetID());
                    }
                    else
                    {
                        zone->RemoveDisabledBarriers(elem->GetID());
                    }

                    updated = true;

                    // There should only ever be one of these but do not
                    // break just in case as there is no hard restriction
                }
            }
        }

        return updated;
    }

    return false;
}

bool ZoneManager::GetSpotPosition(uint32_t dynamicMapID, uint32_t spotID,
    float& x, float& y, float& rot) const
{
    if(spotID == 0 || dynamicMapID == 0)
    {
        return false;
    }

    auto spots = mServer.lock()->GetDefinitionManager()->GetSpotData(dynamicMapID);
    auto spotIter = spots.find(spotID);
    if(spotIter != spots.end())
    {
        x = spotIter->second->GetCenterX();
        y = spotIter->second->GetCenterY();
        rot = spotIter->second->GetRotation();

        return true;
    }

    return false;
}

Point ZoneManager::GetRandomPoint(float width, float height) const
{
    return Point(RNG_DEC(float, 0.f, (float)fabs(width), 2),
        RNG_DEC(float, 0.f, (float)fabs(height), 2));
}

Point ZoneManager::GetRandomSpotPoint(
    const std::shared_ptr<objects::MiSpotData>& spot,
    const std::shared_ptr<objects::MiZoneData>& zoneData)
{
    Point center(spot->GetCenterX(), spot->GetCenterY());

    Point untransformed = GetRandomPoint(spot->GetSpanX() * 2.f,
        spot->GetSpanY() * 2.f);
    untransformed.x += center.x - spot->GetSpanX();
    untransformed.y += center.y - spot->GetSpanY();

    Point transformed = spot->GetRotation() != 0.f
        ? RotatePoint(untransformed, center, spot->GetRotation())
        : untransformed;

    if(zoneData)
    {
        // Ensure that the random spot is in the zone boundaries
        std::shared_ptr<ZoneGeometry> geometry;

        auto qmpFile = zoneData->GetFile()->GetQmpFile();
        if(!qmpFile.IsEmpty())
        {
            std::lock_guard<std::mutex> lock(mLock);
            auto geoIter = mZoneGeometry.find(qmpFile.C());
            if(geoIter != mZoneGeometry.end())
            {
                geometry = geoIter->second;
            }
        }

        Line centerLine(center, transformed);

        Point collision;
        if(geometry && geometry->Collides(centerLine, collision))
        {
            // Move off the collision point by a small amount
            transformed = GetLinearPoint(collision.x, collision.y,
                center.x, center.y, 10.f, false);
        }
    }

    return transformed;
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
        if(eState->GetZone()->Collides(move, corrected))
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
    uint32_t dynamicMapID, const std::shared_ptr<ChannelClientConnection>& client,
    uint32_t currentInstanceID)
{
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    auto zoneDefinition = serverDataManager->GetZoneData(zoneID, dynamicMapID);
    if(!zoneDefinition)
    {
        return nullptr;
    }

    std::shared_ptr<Zone> zone;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(zoneDefinition->GetGlobal())
        {
            auto iter = mGlobalZoneMap.find(zoneID);
            if(iter != mGlobalZoneMap.end())
            {
                for(auto dPair : iter->second)
                {
                    // If dynamicMapID is 0, check all valid instances and take
                    // the first one that applies
                    if(dynamicMapID == 0 || dPair.first == dynamicMapID)
                    {
                        zone = mZones[dPair.second];
                        break;
                    }
                }
            }

            if(nullptr == zone)
            {
                LOG_ERROR(libcomp::String("Global zone encountered that has"
                    " not been instance: %1\n").Arg(zoneID));
                return nullptr;
            }
        }
        else
        {
            // Get or create the zone in the player instance
            auto state = client->GetClientState();

            uint32_t instanceID = currentInstanceID;
            if(!instanceID)
            {
                auto accessIter = mZoneInstanceAccess.find(state->GetWorldCID());
                instanceID = accessIter != mZoneInstanceAccess.end()
                    ? accessIter->second : 0;
            }

            if(instanceID == 0)
            {
                LOG_ERROR(libcomp::String("Character attempted to enter a zone instance"
                    " that does not exist: %1\n").Arg(state->GetAccountUID().ToString()));
                return nullptr;
            }

            auto instIter = mZoneInstances.find(instanceID);
            if(instIter == mZoneInstances.end())
            {
                LOG_ERROR(libcomp::String("Character could not be added to the"
                    " requested instance: %1\n").Arg(state->GetAccountUID().ToString()));
                return nullptr;
            }

            zone = GetInstanceZone(instIter->second, zoneID, dynamicMapID);
        }
    }

    return zone;
}

std::shared_ptr<Zone> ZoneManager::GetInstanceZone(
    std::shared_ptr<ZoneInstance> instance, uint32_t zoneID,
    uint32_t dynamicMapID)
{
    auto zone = instance->GetZone(zoneID, dynamicMapID);
    if(zone)
    {
        return zone;
    }

    // Ensure the zone is part of the instance definition and create it
    auto instanceDef = instance->GetDefinition();
    auto instVariant = instance->GetVariant();

    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    std::shared_ptr<objects::ServerZone> zoneDefinition;

    for(size_t i = 0; i < instanceDef->ZoneIDsCount(); i++)
    {
        uint32_t zID = instanceDef->GetZoneIDs(i);
        uint32_t dID = instanceDef->GetDynamicMapIDs(i);
        if(zID == zoneID && (dynamicMapID == 0 || dID == dynamicMapID))
        {
            std::set<uint32_t> partialIDs;
            if(instVariant)
            {
                partialIDs = instVariant->GetZonePartialIDs();
            }

            zoneDefinition = serverDataManager->GetZoneData(zID, dID, true,
                partialIDs);

            break;
        }
    }

    if(zoneDefinition)
    {
        zone = CreateZone(zoneDefinition);
        if(!instance->AddZone(zone))
        {
            LOG_ERROR(libcomp::String("Failed to add zone to"
                " instance: %1 (%2)\n").Arg(zoneID).Arg(dynamicMapID));
            zone->Cleanup();
            mZones.erase(zone->GetID());
            return nullptr;
        }

        zone->SetInstance(instance);
    }
    else
    {
        LOG_ERROR(libcomp::String("Attmpted to add invalid zone to"
            " instance: %1 (%2)\n").Arg(zoneID).Arg(dynamicMapID));
        return nullptr;
    }

    return zone;
}

std::shared_ptr<Zone> ZoneManager::CreateZone(
    const std::shared_ptr<objects::ServerZone>& definition)
{
    if(nullptr == definition)
    {
        return nullptr;
    }

    uint32_t zoneID = definition->GetID();
    uint32_t dynamicMapID = definition->GetDynamicMapID();

    uint32_t id = mNextZoneID++;

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto zoneData = definitionManager->GetZoneData(zoneID);

    auto zone = std::shared_ptr<Zone>(new Zone(id, definition));

    auto qmpFile = zoneData->GetFile()->GetQmpFile();
    auto geoIter = !qmpFile.IsEmpty()
        ? mZoneGeometry.find(qmpFile.C()) : mZoneGeometry.end();
    if(geoIter != mZoneGeometry.end())
    {
        zone->SetGeometry(geoIter->second);
    }

    auto it = mDynamicMaps.find(dynamicMapID);
    if(it != mDynamicMaps.end())
    {
        zone->SetDynamicMap(it->second);
    }

    for(auto npc : definition->GetNPCs())
    {
        auto copy = std::make_shared<objects::ServerNPC>(*npc);

        auto state = std::shared_ptr<NPCState>(new NPCState(copy));

        float x = npc->GetX();
        float y = npc->GetY();
        float rot = npc->GetRotation();
        if(npc->GetSpotID() && !GetSpotPosition(dynamicMapID, npc->GetSpotID(),
            x, y, rot))
        {
            LOG_WARNING(libcomp::String("NPC %1 in zone %2%3 is placed in"
                " an invalid spot and will be ignored.\n").Arg(npc->GetID())
                .Arg(zoneID).Arg(id != dynamicMapID
                    ? libcomp::String(" (%1)").Arg(dynamicMapID) : ""));
            continue;
        }

        state->SetCurrentX(x);
        state->SetCurrentY(y);
        state->SetCurrentRotation(rot);

        state->SetEntityID(server->GetNextEntityID());
        state->SetActions(npc->GetActions());
        zone->AddNPC(state);
    }

    for(auto obj : definition->GetObjects())
    {
        auto copy = std::make_shared<objects::ServerObject>(*obj);

        auto state = std::shared_ptr<ServerObjectState>(
            new ServerObjectState(copy));

        float x = obj->GetX();
        float y = obj->GetY();
        float rot = obj->GetRotation();
        if(obj->GetSpotID() && !GetSpotPosition(dynamicMapID, obj->GetSpotID(),
            x, y, rot))
        {
            LOG_WARNING(libcomp::String("Object %1 in zone %2%3 is placed in"
                " an invalid spot and will be ignored.\n").Arg(obj->GetID())
                .Arg(zoneID).Arg(id != dynamicMapID
                    ? libcomp::String(" (%1)").Arg(dynamicMapID) : ""));
            continue;
        }

        state->SetCurrentX(x);
        state->SetCurrentY(y);
        state->SetCurrentRotation(rot);

        state->SetEntityID(server->GetNextEntityID());
        state->SetActions(obj->GetActions());
        zone->AddObject(state);

        // Objects are assumed to be enabled by default so check geometry
        if(IsGeometryDisabled(copy))
        {
            UpdateGeometryElement(zone, copy);
        }
    }

    if(definition->PlasmaSpawnsCount() > 0)
    {
        for(auto plasmaPair : definition->GetPlasmaSpawns())
        {
            auto pSpawn = plasmaPair.second;
            auto state = std::shared_ptr<PlasmaState>(new PlasmaState(pSpawn));

            float x = pSpawn->GetX();
            float y = pSpawn->GetY();
            float rot = pSpawn->GetRotation();
            if(pSpawn->GetSpotID() && !GetSpotPosition(dynamicMapID,
                pSpawn->GetSpotID(), x, y, rot))
            {
                LOG_WARNING(libcomp::String("Plasma %1 in zone %2%3 is placed"
                    " in an invalid spot and will be ignored.\n")
                    .Arg(pSpawn->GetID()).Arg(zoneID).Arg(id != dynamicMapID
                        ? libcomp::String(" (%1)").Arg(dynamicMapID) : ""));
                continue;
            }

            state->SetCurrentX(x);
            state->SetCurrentY(y);
            state->SetCurrentRotation(rot);

            state->CreatePoints();

            state->SetEntityID(server->GetNextEntityID());
            zone->AddPlasma(state);
        }

        UpdatePlasma(zone);
    }

    if(definition->BazaarsCount() > 0)
    {
        std::list<std::shared_ptr<objects::BazaarData>> activeMarkets;
        for(auto m : objects::BazaarData::LoadBazaarDataListByZone(
            server->GetWorldDatabase(), zoneID))
        {
            if(m->GetState() == objects::BazaarData::State_t::BAZAAR_ACTIVE)
            {
                activeMarkets.push_back(m);
            }
        }

        for(auto bazaar : definition->GetBazaars())
        {
            auto state = std::shared_ptr<BazaarState>(new BazaarState(bazaar));

            float x = bazaar->GetX();
            float y = bazaar->GetY();
            float rot = bazaar->GetRotation();
            if(bazaar->GetSpotID() && !GetSpotPosition(dynamicMapID,
                bazaar->GetSpotID(), x, y, rot))
            {
                LOG_WARNING(libcomp::String("Bazaar %1 in zone %2%3 is placed"
                    " in an invalid spot and will be ignored.\n")
                    .Arg(bazaar->GetID()).Arg(zoneID).Arg(id != dynamicMapID
                        ? libcomp::String(" (%1)").Arg(dynamicMapID) : ""));
                continue;
            }

            state->SetCurrentX(x);
            state->SetCurrentY(y);
            state->SetCurrentRotation(rot);

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

    // Gather setup triggers and sort all other types from the definition
    std::list<std::shared_ptr<objects::ServerZoneTrigger>> setupTriggers;
    for(auto trigger : definition->GetTriggers())
    {
        switch(trigger->GetTrigger())
        {
        case objects::ServerZoneTrigger::Trigger_t::ON_SETUP:
            setupTriggers.push_back(trigger);
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_IN:
            zone->AppendZoneInTriggers(trigger);
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_ZONE_OUT:
            zone->AppendZoneOutTriggers(trigger);
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_SPAWN:
            zone->AppendSpawnTriggers(trigger);
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_DEATH:
            zone->AppendDeathTriggers(trigger);
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_REVIVAL:
            zone->AppendRevivalTriggers(trigger);
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_FLAG_SET:
            zone->AppendFlagSetTriggers(trigger);
            zone->InsertFlagSetKeys(trigger->GetValue());
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_TIME:
        case objects::ServerZoneTrigger::Trigger_t::ON_SYSTEMTIME:
        case objects::ServerZoneTrigger::Trigger_t::ON_MOONPHASE:
            zone->AppendTimeTriggers(trigger);
            break;
        default:
            break;
        }
    }

    mZones[id] = zone;

    // Register time restrictions and calculate current state if any exist
    if(RegisterTimeRestrictions(zone, definition))
    {
        auto clock = server->GetWorldClockTime();
        zone->UpdateTimedSpawns(clock, true);
    }

    // Run all setup actions
    if(setupTriggers.size() > 0)
    {
        auto actionManager = server->GetActionManager();
        for(auto trigger : setupTriggers)
        {
            actionManager->PerformActions(nullptr,
                trigger->GetActions(), 0, zone);
        }
    }

    // Populate all spawnpoints
    UpdateSpawnGroups(zone, true);

    return zone;
}

bool ZoneManager::RemoveInstance(uint32_t instanceID)
{
    auto instIter = mZoneInstances.find(instanceID);
    if(instIter == mZoneInstances.end())
    {
        return false;
    }

    auto instance = instIter->second;
    auto instZones = instance->GetZones();

    std::list<std::shared_ptr<Zone>> cleanupZones;
    for(auto iPair : instZones)
    {
        for(auto zPair : iPair.second)
        {
            auto z = zPair.second;
            if(z->GetConnections().size() == 0)
            {
                cleanupZones.push_back(z);
            }
            else
            {
                return false;
            }
        }
    }

    // Since the zones will all be cleaned up, drop
    // the instance and remove all access
    for(int32_t accessCID : instance->GetAccessCIDs())
    {
        auto it = mZoneInstanceAccess.find(accessCID);
        if(it != mZoneInstanceAccess.end() &&
            it->second == instance->GetID())
        {
            mZoneInstanceAccess.erase(it);
        }
    }

    LOG_DEBUG(libcomp::String("Cleaning up zone instance: %1 (%2)\n")
        .Arg(instance->GetID()).Arg(instance->GetDefinition()->GetID()));

    mZoneInstances.erase(instance->GetID());

    std::list<WorldClockTime> removeSpawnTimes;
    for(auto z : cleanupZones)
    {
        z->Cleanup();
        mZones.erase(z->GetID());
        mActiveZones.erase(z->GetID());
        mTimeRestrictUpdatedZones.erase(z->GetID());

        if(mAllTimeRestrictZones.find(z->GetID()) !=
            mAllTimeRestrictZones.end())
        {
            for(auto tPair : mSpawnTimeRestrictZones)
            {
                tPair.second.erase(z->GetID());
                if(tPair.second.size() == 0)
                {
                    removeSpawnTimes.push_back(tPair.first);
                }
            }

            mAllTimeRestrictZones.erase(z->GetID());
        }
    }

    // Clean up any time restrictions
    if(removeSpawnTimes.size() > 0)
    {
        auto server = mServer.lock();
        for(auto& t : removeSpawnTimes)
        {
            server->RegisterClockEvent(t, 1, true);
        }
    }

    return true;
}

bool ZoneManager::IsGeometryDisabled(const std::shared_ptr<
    objects::ServerObject>& obj)
{
    // Two open states and one hidden state
    return obj->GetState() == 2 || obj->GetState() == 3 ||
        obj->GetState() == 255;
}

bool ZoneManager::RegisterTimeRestrictions(const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<objects::ServerZone>& definition)
{
    std::list<WorldClockTime> spawnTimes;
    std::list<WorldClockTime> eventTimes;

    // Gather spawn restrictions from spawn groups and plasma
    std::list<std::shared_ptr<objects::SpawnRestriction>> restrictions;
    for(auto sgPair : definition->GetSpawnGroups())
    {
        auto sg = sgPair.second;
        auto restriction = sg->GetRestrictions();
        if(restriction)
        {
            restrictions.push_back(restriction);
        }
    }

    for(auto pPair : definition->GetPlasmaSpawns())
    {
        auto plasma = pPair.second;
        auto restriction = plasma->GetRestrictions();
        if(restriction)
        {
            restrictions.push_back(restriction);
        }
    }

    // Build times from spawn restrictions
    for(auto restriction : restrictions)
    {
        std::set<int8_t> phases;
        if(restriction->GetMoonRestriction() != 0xFFFF)
        {
            for(int8_t p = 0; p < 16; p++)
            {
                if(((restriction->GetMoonRestriction() >> p) & 0x01) != 0)
                {
                    // Add the phase and next phase
                    phases.insert(p);
                    phases.insert((int8_t)((p + 1) % 16));
                }
            }
        }

        if(restriction->TimeRestrictionCount() > 0)
        {
            std::list<WorldClockTime> gameTimes;
            for(auto pair : restriction->GetTimeRestriction())
            {
                WorldClockTime after;
                after.Hour = (int8_t)(pair.first / 100 % 24);
                after.Min = (int8_t)((pair.first % 100) % 60);
                gameTimes.push_back(after);

                // Actual end time is one minute later
                WorldClockTime before;
                before.Hour = (int8_t)(pair.second / 100 % 24);
                before.Min = (int8_t)((pair.second % 100) % 60);
                if(before.Min == 59)
                {
                    before.Min = 0;
                    before.Hour = (int8_t)((before.Hour + 1) % 24);
                }
                else
                {
                    before.Min++;
                }

                gameTimes.push_back(before);
            }

            if(phases.size() > 0)
            {
                // Phase and game time
                for(int8_t phase : phases)
                {
                    for(WorldClockTime t : gameTimes)
                    {
                        t.MoonPhase = phase;
                        spawnTimes.push_back(t);
                    }
                }
            }
            else
            {
                // Game time only
                for(WorldClockTime t : gameTimes)
                {
                    spawnTimes.push_back(t);
                }
            }
        }
        else if(restriction->SystemTimeRestrictionCount() > 0)
        {
            std::list<WorldClockTime> sysTimes;
            for(auto pair : restriction->GetSystemTimeRestriction())
            {
                WorldClockTime after;
                after.SystemHour = (int8_t)(pair.first / 100 % 24);
                after.SystemMin = (int8_t)((pair.first % 100) % 60);
                sysTimes.push_back(after);

                // Actual end time is one minute later
                WorldClockTime before;
                before.SystemHour = (int8_t)(pair.second / 100 % 24);
                before.SystemMin = (int8_t)((pair.second % 100) % 60);
                if(before.SystemMin == 59)
                {
                    before.SystemMin = 0;
                    before.SystemHour = (int8_t)((before.SystemHour + 1) % 24);
                }
                else
                {
                    before.SystemMin++;
                }

                sysTimes.push_back(before);
            }

            if(phases.size() > 0)
            {
                // Phase and system time
                for(int8_t phase : phases)
                {
                    for(WorldClockTime t : sysTimes)
                    {
                        t.MoonPhase = phase;
                        spawnTimes.push_back(t);
                    }
                }
            }
            else
            {
                // System time only
                for(WorldClockTime t : sysTimes)
                {
                    spawnTimes.push_back(t);
                }
            }
        }
        else if(phases.size() > 0)
        {
            // Phase only
            for(int8_t phase : phases)
            {
                WorldClockTime t;
                t.MoonPhase = phase;
                spawnTimes.push_back(t);
            }
        }

        // If day or date restrictions are set, add midnight too
        if(restriction->GetDayRestriction() != 0x8F ||
            restriction->DateRestrictionCount() > 0)
        {
            WorldClockTime t;
            t.SystemHour = 0;
            t.SystemMin = 0;
            spawnTimes.push_back(t);
        }
    }

    // Build event times
    for(auto trigger : definition->GetTriggers())
    {
        switch(trigger->GetTrigger())
        {
        case objects::ServerZoneTrigger::Trigger_t::ON_TIME:
            {
                WorldClockTime t;
                t.Hour = (int8_t)(trigger->GetValue() / 100);
                t.Min = (int8_t)(trigger->GetValue() % 100);
                eventTimes.push_back(t);
            }
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_SYSTEMTIME:
            {
                WorldClockTime t;
                t.SystemHour = (int8_t)(trigger->GetValue() / 100);
                t.SystemMin = (int8_t)(trigger->GetValue() % 100);
                eventTimes.push_back(t);
            }
            break;
        case objects::ServerZoneTrigger::Trigger_t::ON_MOONPHASE:
            {
                WorldClockTime t;
                t.MoonPhase = (int8_t)trigger->GetValue();
                eventTimes.push_back(t);
            }
            break;
        default:
            break;
        }
    }

    // Register all times
    if(spawnTimes.size() > 0 || eventTimes.size() > 0)
    {
        auto server = mServer.lock();

        for(auto& t : spawnTimes)
        {
            mSpawnTimeRestrictZones[t].insert(zone->GetID());
            server->RegisterClockEvent(t, 1, false);
        }

        for(auto& t : eventTimes)
        {
            mSpawnTimeRestrictZones[t].insert(zone->GetID());
            server->RegisterClockEvent(t, 3, false);
        }

        mAllTimeRestrictZones.insert(zone->GetID());

        return true;
    }

    return false;
}
