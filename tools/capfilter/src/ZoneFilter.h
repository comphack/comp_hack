/**
 * @file tools/capfilter/src/ZoneFilter.h
 * @ingroup capfilter
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet filter dialog.
 *
 * Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_CAPFILTER_SRC_ZONEFILTER_H
#define TOOLS_CAPFILTER_SRC_ZONEFILTER_H

// libcomp Includes
#include <DataStore.h>
#include <DefinitionManager.h>
#include <ServerBazaar.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>

// C++11 Includes
#include <unordered_map>

// comp_capfilter
#include "CommandFilter.h"

namespace objects
{
class ActionZoneChange;
class Event;
}

class MappedEvent;
class ZoneInstance;

class ZoneFilter : public CommandFilter
{
public:
    ZoneFilter(const char *szProgram, const libcomp::String& dataStorePath,
        bool generateEvents);
    virtual ~ZoneFilter();

    virtual bool ProcessCommand(const libcomp::String& capturePath,
        uint16_t commandCode, libcomp::ReadOnlyPacket& packet);
    virtual bool PostProcess();

private:
    class Zone : public objects::ServerZone
    {
    public:
        std::list<std::shared_ptr<ZoneInstance>> instances;
        std::unordered_map<uint32_t,
            std::shared_ptr<objects::ActionZoneChange>> connections;
        std::unordered_map<uint32_t,
            std::shared_ptr<objects::ActionZoneChange>> allConnections;
    };

    bool ProcessEventCommands(const libcomp::String& capturePath,
        uint16_t commandCode, libcomp::ReadOnlyPacket& packet);

    std::shared_ptr<objects::ServerNPC> GetHNPC(
        std::shared_ptr<Zone> zone, uint32_t objectId,
        float originX, float originY, float originRotation);
    std::shared_ptr<objects::ServerObject> GetONPC(
        std::shared_ptr<Zone> zone, uint32_t objectId,
        float originX, float originY, float originRotation);
    std::shared_ptr<objects::ServerBazaar> GetBazaar(
        std::shared_ptr<Zone> zone,
        float originX, float originY, float originRotation);

    bool CheckUnknownEntity(const libcomp::String& capturePath,
        const std::shared_ptr<ZoneInstance>& instance,
        int32_t entityID, const libcomp::String& packetType);

    void EndEvent();

    void RegisterZone(uint32_t zoneID, uint32_t dynamicMapID);

    void BindZoneChangeEvent(uint32_t zoneID, float x, float y, float rot);

    void MergeEventReferences(const std::shared_ptr<MappedEvent>& from,
        const std::shared_ptr<MappedEvent>& to,
        std::vector<std::shared_ptr<MappedEvent>>& allEvents);

    bool MergeEvents(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen = {});

    bool MergeEventMessages(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventNPCMessages(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventExNPCMessages(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventMultitalks(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventPrompts(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventPlayScenes(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventMenus(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventGetItems(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventStageEffects(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventDirections(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventSpecialDirections(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventSoundEffects(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventPlayBGMs(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventStopBGMs(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);
    bool MergeEventPerformActions(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);

    bool MergeNextGeneric(std::shared_ptr<MappedEvent> e1,
        std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
        std::set<std::shared_ptr<MappedEvent>> seen);

    libcomp::DataStore mStore;
    libcomp::DefinitionManager mDefinitions;
    bool mGenerateEvents;

    int32_t mCurrentPlayerEntityID;
    uint64_t mCurrentZoneID;
    int16_t mCurrentLNC;
    libcomp::String mCurrentMaps;
    libcomp::String mCurrentPlugins;
    libcomp::String mCurrentValuables;
    std::unordered_map<uint64_t, std::shared_ptr<Zone>> mZones;
};

#endif // TOOLS_CAPFILTER_SRC_ZONEFILTER_H
