/**
 * @file server/channel/src/ZoneGeometryLoader.cpp
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Loads the geometry for the zones.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2019 COMP_hack Team <compomega@tutanota.com>
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

#include "ZoneGeometryLoader.h"

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
#include <ActionStartEvent.h>
#include <Ally.h>
#include <ChannelLogin.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <DestinyBox.h>
#include <DiasporaBase.h>
#include <DigitalizeState.h>
#include <Enemy.h>
#include <EntityStats.h>
#include <InstanceAccess.h>
#include <Item.h>
#include <ItemDrop.h>
#include <Loot.h>
#include <LootBox.h>
#include <Match.h>
#include <MiAIData.h>
#include <MiDevilData.h>
#include <MiDynamicMapData.h>
#include <MiGrowthData.h>
#include <MiSpotData.h>
#include <MiTimeLimitData.h>
#include <MiUraFieldTowerData.h>
#include <MiZoneData.h>
#include <MiZoneFileData.h>
#include <MiONPCData.h>
#include <Party.h>
#include <PentalphaEntry.h>
#include <PlasmaSpawn.h>
#include <PlayerExchangeSession.h>
#include <PvPBase.h>
#include <PvPInstanceStats.h>
#include <PvPInstanceVariant.h>
#include <PvPMatch.h>
#include <QmpBoundary.h>
#include <QmpBoundaryLine.h>
#include <QmpElement.h>
#include <QmpFile.h>
#include <QmpNavPoint.h>
#include <ServerBazaar.h>
#include <ServerCultureMachineSet.h>
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
#include <Team.h>
#include <UBMatch.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "AccountManager.h"
#include "ActionManager.h"
#include "AIManager.h"
#include "AIState.h"
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"
#include "CultureMachineState.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "MatchManager.h"
#include "PlasmaState.h"
#include "SkillManager.h"
#include "TokuseiManager.h"
#include "Zone.h"
#include "ZoneInstance.h"

// Standard C++11 Includes
#include <cmath>
#include <thread>

using namespace channel;

std::unordered_map<std::string,
    std::shared_ptr<ZoneGeometry>> ZoneGeometryLoader::LoadQMP(
        std::unordered_map<uint32_t, std::set<uint32_t>> localZoneIDs,
        const std::shared_ptr<ChannelServer>& server)
{
    for(auto zonePair : localZoneIDs)
    {
        mZonePairs.push_back(zonePair);
    }

    std::list<std::thread*> threads;

    for(uint32_t i = 0; i < std::thread::hardware_concurrency(); ++i)
    {
        threads.push_back(new std::thread(
            [&](const std::shared_ptr<ChannelServer>& _server) {
                while(LoadZoneQMP(_server)) { }
            }, server));
    }

    for(auto thread : threads)
    {
        thread->join();

        delete thread;
    }

    return mZoneGeometry;
}

bool ZoneGeometryLoader::LoadZoneQMP(
    const std::shared_ptr<ChannelServer>& server)
{
    mDataLock.lock();

    if(mZonePairs.empty())
    {
        mDataLock.unlock();

        return false;
    }

    auto definitionManager = server->GetDefinitionManager();
    auto zonePair = mZonePairs.front();
    mZonePairs.pop_front();

    mDataLock.unlock();

    uint32_t zoneID = zonePair.first;
    auto zoneData = definitionManager->GetZoneData(zoneID);

    libcomp::String filename = zoneData->GetFile()->GetQmpFile();
    if(filename.IsEmpty() || mZoneGeometry.find(filename.C()) !=
        mZoneGeometry.end())
    {
        return true;
    }

    auto qmpFile = definitionManager->LoadQmpFile(filename, server->GetDataStore());
    if(!qmpFile)
    {
        //success = false;
        LOG_ERROR(libcomp::String("Failed to load zone geometry file: %1\n")
            .Arg(filename));
        return true;
    }

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
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::QmpNavPoint>> navPoints;
    for(auto qmpBoundary : qmpFile->GetBoundaries())
    {
        for(auto qmpLine : qmpBoundary->GetLines())
        {
            Line l(Point((float)qmpLine->GetX1(), (float)qmpLine->GetY1()),
                Point((float)qmpLine->GetX2(), (float)qmpLine->GetY2()));
            lineMap[qmpLine->GetElementID()].push_back(l);
        }

        for(auto navPoint : qmpBoundary->GetNavPoints())
        {
            navPoints[navPoint->GetPointID()] = navPoint;
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

    // If any zone-in spots exist, remove all navpoints that are outside
    // of all play areas by checking if the center point of zone-in spot
    // connects to the points (in large zones this often times cuts the
    // number of points in half)
    std::list<Point> zoneInPoints;
    for(auto dynamicMapID : zonePair.second)
    {
        auto spots = definitionManager->GetSpotData(dynamicMapID);
        for(auto spotPair : spots)
        {
            if(spotPair.second->GetType() == 3)
            {
                zoneInPoints.push_back(Point(spotPair.second->GetCenterX(),
                    spotPair.second->GetCenterY()));
            }
        }
    }

    size_t navTotal = navPoints.size();
    if(zoneInPoints.size() > 0)
    {
        // Gather all toggle enabled barriers to simulate everything being
        // open
        std::set<uint32_t> toggleBarriers;
        for(auto qmpElem : qmpFile->GetElements())
        {
            if(qmpElem->GetType() == objects::QmpElement::Type_t::TOGGLE ||
               qmpElem->GetType() == objects::QmpElement::Type_t::TOGGLE_2)
            {
                toggleBarriers.insert(qmpElem->GetID());
            }
        }

        // Gather all points directly visible to a zone-in point
        std::set<uint32_t> validPoints;

        Point pOut;
        Line lOut;
        std::shared_ptr<ZoneShape> sOut;
        for(Point& p : zoneInPoints)
        {
            for(auto& nPair : navPoints)
            {
                if(validPoints.find(nPair.first) == validPoints.end())
                {
                    auto n = nPair.second;

                    Line l(p, Point((float)n->GetX(), (float)n->GetY()));
                    if(!geometry->Collides(l, pOut, lOut, sOut,
                        toggleBarriers))
                    {
                        validPoints.insert(nPair.first);

                        // Pull all registered distance points as we go to
                        // minimize geometry checks needed
                        for(auto& dist : n->GetDistances())
                        {
                            validPoints.insert(dist.first);
                        }
                    }
                }
            }
        }

        // All direct points loaded, add direct path points from nav map
        std::set<uint32_t> checked;
        std::set<uint32_t> check = validPoints;
        while(check.size() > 0)
        {
            uint32_t pointID = *check.begin();
            check.erase(pointID);
            checked.insert(pointID);

            auto n = navPoints[pointID];
            if(n)
            {
                for(auto& dist : n->GetDistances())
                {
                    uint32_t pointID2 = dist.first;
                    if(checked.find(pointID2) == checked.end())
                    {
                        check.insert(pointID2);
                        validPoints.insert(pointID2);
                    }
                }
            }
        }

        // Filter down the points
        std::set<uint32_t> invalidPoints;
        for(auto& pair : navPoints)
        {
            if(validPoints.find(pair.first) == validPoints.end())
            {
                invalidPoints.insert(pair.first);
            }
        }

        for(uint32_t pointID : invalidPoints)
        {
            navPoints.erase(pointID);
        }
    }

    geometry->NavPoints = navPoints;

    libcomp::String filterString;
    if(navPoints.size() != navTotal)
    {
        filterString = libcomp::String(" (Nav points: %1 => %2)")
            .Arg(navTotal).Arg(navPoints.size());
    }

    LOG_DEBUG(libcomp::String("Loaded zone geometry file: %1%2\n")
        .Arg(filename).Arg(filterString));

    mDataLock.lock();
    mZoneGeometry[filename.C()] = geometry;
    mDataLock.unlock();

    return true;
}
