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

#include "ZoneFilter.h"

// libcomp Includes
#include <PacketCodes.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ActionStartEvent.h>
#include <ActionZoneChange.h>
#include <EventChoice.h>
#include <EventDirection.h>
#include <EventExNPCMessage.h>
#include <EventGetItem.h>
#include <EventHomepoint.h>
#include <EventMessage.h>
#include <EventMultitalk.h>
#include <EventNPCMessage.h>
#include <EventOpenMenu.h>
#include <EventPlayScene.h>
#include <EventPrompt.h>
#include <EventSpecialDirection.h>
#include <EventStageEffect.h>
#include <MiCZoneRelationData.h>
#include <MiRelationZoneIDData.h>
#include <MiHNPCData.h>
#include <MiHNPCBasicData.h>
#include <MiONPCData.h>
#include <MiZoneData.h>
#include <MiZoneBasicData.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneSpot.h>

class MappedEvent
{
public:
    MappedEvent(const std::shared_ptr<objects::Event>& e,
        std::shared_ptr<objects::ServerObjectBase> obj)
    {
        event = e;
        source = obj;
        mergeCount = 0;
    }

    int32_t mergeCount;
    std::shared_ptr<objects::ServerObjectBase> source;
    std::shared_ptr<objects::Event> event;
    std::weak_ptr<MappedEvent> previous;
    std::unordered_map<int32_t, std::shared_ptr<MappedEvent>> next;
    std::unordered_map<int32_t,
        std::vector<std::shared_ptr<MappedEvent>>> nextBranch;
};

class SequenceTriggerPacket
{
public:
    SequenceTriggerPacket(uint16_t code,
        libcomp::ReadOnlyPacket& p, int32_t packetNum)
    {
        valid = true;
        commandCode = code;
        packet = p;
        packetNumber = packetNum;
    }

    bool valid;
    int32_t packetNumber;
    uint16_t commandCode;
    libcomp::ReadOnlyPacket packet;
};

class ZoneInstance
{
public:
    libcomp::String filePath;
    std::unordered_map<int32_t,
        std::shared_ptr<objects::ServerObjectBase>> entities;
    std::list<std::shared_ptr<MappedEvent>> events;

    int32_t eventResponse = -1;
    bool eventsInvalid = false;
    int32_t packetNumber = 0;

    // Keep track of the last and second last triggers as zone changes seem
    // to be queued and will fire after the event end sometimes
    std::shared_ptr<SequenceTriggerPacket> lastTrigger;
    std::shared_ptr<SequenceTriggerPacket> secondLastTrigger;

    std::shared_ptr<MappedEvent> firstSequence;
    std::shared_ptr<MappedEvent> currentSequence;
};

ZoneFilter::ZoneFilter(const char *szProgram,
    const libcomp::String& dataStorePath, bool generateEvents)
    : mStore(szProgram), mGenerateEvents(generateEvents),
    mCurrentPlayerEntityID(0), mCurrentZoneID(0)
{
    if(!mStore.AddSearchPath(dataStorePath))
    {
        std::cerr << "Failed to add search path." << std::endl;
    }

    if(!mDefinitions.LoadHNPCData(&mStore))
    {
        std::cerr << "Failed to load hNPC data." << std::endl;
    }

    if(!mDefinitions.LoadONPCData(&mStore))
    {
        std::cerr << "Failed to load oNPC data." << std::endl;
    }

    if(!mDefinitions.LoadZoneData(&mStore))
    {
        std::cerr << "Failed to load zone data." << std::endl;
    }

    if(!mDefinitions.LoadCZoneRelationData(&mStore))
    {
        std::cerr << "Failed to load zone relation data." << std::endl;
    }
}

ZoneFilter::~ZoneFilter()
{
}

bool ZoneFilter::ProcessCommand(const libcomp::String& capturePath,
    uint16_t commandCode, libcomp::ReadOnlyPacket& packet)
{
    (void)capturePath;

    bool zoneChangePacket = commandCode ==
        to_underlying(ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE);
    bool charDataPacket = commandCode ==
        to_underlying(ChannelToClientPacketCode_t::PACKET_CHARACTER_DATA);

    if(!zoneChangePacket && !charDataPacket && mCurrentZoneID == 0)
    {
        // Nothing to do
        return true;
    }

    auto instance = mCurrentZoneID != 0
        ? mZones[mCurrentZoneID]->instances.back() : nullptr;

    if(instance)
    {
        instance->packetNumber++;
    }

    switch(commandCode)
    {
    case to_underlying(ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE):
        {
            if(24 != packet.Left())
            {
                std::cerr << "Bad zone change packet found." << std::endl;

                return false;
            }

            uint32_t zoneId = packet.ReadU32Little();
            uint32_t zoneInstance = packet.ReadU32Little();
            float xPos = packet.ReadFloat();
            float yPos = packet.ReadFloat();
            float rot = packet.ReadFloat();
            uint32_t zoneDynamicMapID = packet.ReadU32Little();
            (void)zoneInstance;

            auto it = mZones.find(zoneId);

            if(mZones.end() == it)
            {
                RegisterZone(zoneId, zoneDynamicMapID);
            }

            bool newFile = instance && instance->filePath != capturePath;
            if(mCurrentZoneID != 0 && zoneId != mCurrentZoneID && !newFile)
            {
                BindZoneChangeEvent(zoneId, xPos, yPos, rot);
            }

            // Do not interpret as a zone change if it was actually a move
            if(mCurrentZoneID != zoneId || newFile)
            {
                instance = std::make_shared<ZoneInstance>();
                mZones[zoneId]->instances.push_back(instance);
                instance->filePath = capturePath;

                if(mCurrentPlayerEntityID != 0)
                {
                    /// @todo: Uses of this need to be fixed to point to
                    // the player somehow
                    instance->entities[mCurrentPlayerEntityID] = nullptr;
                }

                mCurrentZoneID = zoneId;

                EndEvent();
            }
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_CHARACTER_DATA):
        {
            if(4 > packet.Left())
            {
                std::cerr << "Bad character data packet found." << std::endl;

                return false;
            }

            mCurrentPlayerEntityID = packet.ReadS32Little();
            instance->entities[mCurrentPlayerEntityID] = nullptr;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_NPC_DATA):
        {
            if(30 != packet.Left())
            {
                std::cerr << "Bad hNPC packet found." << std::endl;

                return false;
            }

            int32_t entityId = packet.ReadS32Little();
            uint32_t objectId = packet.ReadU32Little();
            uint32_t zoneInstance = packet.ReadU32Little();
            uint32_t zoneId = packet.ReadU32Little();
            float originX = packet.ReadFloat();
            float originY = packet.ReadFloat();
            float originRotation = packet.ReadFloat();
            (void)zoneInstance;

            auto it = mZones.find(zoneId);

            if(mZones.end() == it)
            {
                std::cerr << "hNPC information sent before zone!" << std::endl;

                return false;
            }

            auto zone = it->second;

            auto obj = GetHNPC(zone, objectId, originX, originY, originRotation);
            if(!obj)
            {
                obj = std::make_shared<objects::ServerNPC>();
                obj->SetID(objectId);
                obj->SetX(originX);
                obj->SetY(originY);
                obj->SetRotation(originRotation);

                zone->AppendNPCs(obj);
            }

            instance->entities[entityId] = obj;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_OBJECT_NPC_DATA):
        {
            if(29 != packet.Left())
            {
                std::cerr << "Bad oNPC packet found." << std::endl;

                return false;
            }

            int32_t entityId = packet.ReadS32Little();
            uint32_t objectId = packet.ReadU32Little();
            uint8_t state = packet.ReadU8();
            uint32_t zoneInstance = packet.ReadU32Little();
            uint32_t zoneId = packet.ReadU32Little();
            float originX = packet.ReadFloat();
            float originY = packet.ReadFloat();
            float originRotation = packet.ReadFloat();
            (void)zoneInstance;

            auto it = mZones.find(zoneId);

            if(mZones.end() == it)
            {
                std::cerr << "oNPC information sent before zone!" << std::endl;

                return false;
            }

            auto zone = it->second;

            auto obj = GetONPC(zone, objectId, originX, originY, originRotation);
            if(!obj)
            {
                obj = std::make_shared<objects::ServerObject>();
                obj->SetID(objectId);
                obj->SetX(originX);
                obj->SetY(originY);
                obj->SetRotation(originRotation);
                obj->SetState(state);

                zone->AppendObjects(obj);
            }

            instance->entities[entityId] = obj;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_BAZAAR_DATA):
        {
            if(12 > packet.Left())
            {
                std::cerr << "Bad bazaar packet found." << std::endl;

                return false;
            }

            int32_t entityId = packet.ReadS32Little();
            uint32_t zoneInstance = packet.ReadU32Little();
            uint32_t zoneId = packet.ReadU32Little();
            (void)zoneInstance;

            auto it = mZones.find(zoneId);

            if(mZones.end() == it)
            {
                std::cerr << "Bazaar information sent before zone!" << std::endl;

                return false;
            }

            // Nothing to do with bazaars right now
            instance->entities[entityId] = nullptr;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_SKILL_COMPLETED):
        {
            if(4 > packet.Left())
            {
                std::cerr << "Bad skill copmleted packet found." << std::endl;

                return false;
            }

            // Event triggers cannot be assumed to be the cause of effects if
            // skills are fired between them and the next event

            int32_t entityId = packet.ReadS32Little();

            if(entityId == mCurrentPlayerEntityID &&
                (instance->lastTrigger != nullptr ||
                    instance->secondLastTrigger != nullptr))
            {
                auto t = std::dynamic_pointer_cast<SequenceTriggerPacket>(
                    instance->lastTrigger != nullptr
                        ? instance->lastTrigger : instance->secondLastTrigger);
                if(t)
                {
                    t->valid = false;
                }
            }
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_REMOVE_ENTITY):
    case to_underlying(ChannelToClientPacketCode_t::PACKET_REMOVE_OBJECT):
        {
            if(4 > packet.Left())
            {
                std::cerr << "Bad entity cleanup packet found." << std::endl;

                return false;
            }

            // Do not count as part of the instance packet as they occur during
            // the clean up process
            instance->packetNumber--;
        }
        break;
    default:
        if(mGenerateEvents)
        {
            return ProcessEventCommands(capturePath, commandCode, packet);
        }
        break;
    }

    return true;
}

bool ZoneFilter::ProcessEventCommands(const libcomp::String& capturePath,
    uint16_t commandCode, libcomp::ReadOnlyPacket& packet)
{
    auto instance = mCurrentZoneID != 0
        ? mZones[mCurrentZoneID]->instances.back() : nullptr;

    if(!instance)
    {
        return false;
    }
    else if(instance->eventsInvalid)
    {
        return true;
    }

    bool endEvent = false;
    bool resetResponse = instance && instance->eventResponse != -1;
    auto sequence = instance ? instance->currentSequence : nullptr;
    switch(commandCode)
    {
    case to_underlying(ClientToChannelPacketCode_t::PACKET_OBJECT_INTERACTION):
        {
            if(4 > packet.Left())
            {
                std::cerr << "Bad object interaction packet found." << std::endl;

                return false;
            }

            int32_t entityId = packet.ReadS32Little();
            packet.Rewind();

            if(!CheckUnknownEntity(capturePath, instance, entityId,
                "Object interaction"))
            {
                return true;
            }

            instance->lastTrigger = std::make_shared<
                SequenceTriggerPacket>(commandCode, packet,
                    instance->packetNumber);
            instance->secondLastTrigger = 0;

            return true;
        }
        break;
    case to_underlying(ClientToChannelPacketCode_t::PACKET_SPOT_TRIGGERED):
        {
            if(8 > packet.Left())
            {
                std::cerr << "Bad spot triggered packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = std::make_shared<
                SequenceTriggerPacket>(commandCode, packet, instance->packetNumber);
            instance->secondLastTrigger = 0;

            return true;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_MESSAGE):
        {
            if(4 > packet.Left())
            {
                std::cerr << "Bad event message packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto messageID = packet.ReadS32Little();

            auto msg = instance->currentSequence != nullptr
                ? std::dynamic_pointer_cast<objects::EventMessage>(
                    instance->currentSequence->event)
                : nullptr;
            if(!msg || std::dynamic_pointer_cast<objects::EventNPCMessage>(msg))
            {
                msg = std::make_shared<objects::EventMessage>();
                instance->currentSequence = std::make_shared<MappedEvent>(msg, nullptr);
            }

            msg->AppendMessageIDs(messageID);

            // Normal messages don't wait for a response
            instance->eventResponse = 0;
            resetResponse = false;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_NPC_MESSAGE):
        {
            if(10 > packet.Left())
            {
                std::cerr << "Bad event NPC message packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto sourceEntityID = packet.ReadS32Little();
            auto messageID = packet.ReadS32Little();

            if(!CheckUnknownEntity(capturePath, instance, sourceEntityID, "NPC message"))
            {
                return true;
            }

            auto source = sourceEntityID != 0
                ? instance->entities[sourceEntityID] : nullptr;

            // Supports old size format
            int32_t unknown = 0;
            if(packet.Left() == 2)
            {
                unknown = (int32_t)packet.ReadS16Little();
            }
            else
            {
                unknown = packet.ReadS32Little();
            }

            auto msg = instance->currentSequence != nullptr
                ? std::dynamic_pointer_cast<objects::EventNPCMessage>(
                    instance->currentSequence->event)
                : nullptr;
            if(!msg || instance->eventResponse != 0 ||
                instance->currentSequence->source != source)
            {
                msg = std::make_shared<objects::EventNPCMessage>();
                instance->currentSequence = std::make_shared<MappedEvent>(msg, source);
            }

            msg->AppendMessageIDs(messageID);
            msg->AppendUnknown(unknown);
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_EX_NPC_MESSAGE):
        {
            if(11 > packet.Left())
            {
                std::cerr << "Bad event EX NPC message packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto sourceEntityID = packet.ReadS32Little();
            auto messageID = packet.ReadS32Little();
            auto ex1 = packet.ReadS16Little();
            auto ex2Set = packet.ReadS8();
            auto ex2 = (ex2Set == 1) ? packet.ReadS32Little() : 0;

            if(!CheckUnknownEntity(capturePath, instance, sourceEntityID,
                "EX NPC message"))
            {
                return true;
            }

            auto source = sourceEntityID != 0
                ? instance->entities[sourceEntityID] : nullptr;

            auto msg = std::make_shared<objects::EventExNPCMessage>();
            instance->currentSequence = std::make_shared<MappedEvent>(msg, source);
            msg->SetMessageID(messageID);
            msg->SetEx1(ex1);
            msg->SetEx2(ex2);

            // EX NPC messages don't wait for a response
            instance->eventResponse = 0;
            resetResponse = false;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_MULTITALK):
        {
            if(8 != packet.Left())
            {
                std::cerr << "Bad event multitalk packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto sourceEntityID = packet.ReadS32Little();
            auto messageID = packet.ReadS32Little();

            if(!CheckUnknownEntity(capturePath, instance, sourceEntityID, "Multitalk"))
            {
                return true;
            }

            auto source = sourceEntityID != 0
                ? instance->entities[sourceEntityID] : nullptr;

            auto talk = std::make_shared<objects::EventMultitalk>();
            instance->currentSequence = std::make_shared<MappedEvent>(talk, source);
            talk->SetMessageID(messageID);
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_PROMPT):
        {
            if(12 > packet.Left())
            {
                std::cerr << "Bad event prompt packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto sourceEntityID = packet.ReadS32Little();
            auto promptID = packet.ReadS32Little();
            auto choiceCount = packet.ReadS32Little();

            if((uint32_t)(choiceCount * 8) != packet.Left())
            {
                std::cerr << "Prompt packet with invalid choice count encountered." << std::endl;

                return false;
            }

            if(!CheckUnknownEntity(capturePath, instance, sourceEntityID, "Prompt"))
            {
                return true;
            }

            auto source = sourceEntityID != 0 ? instance->entities[sourceEntityID] : nullptr;

            int32_t idxMax = -1;
            std::unordered_map<int32_t, std::shared_ptr<objects::EventChoice>> choices;
            for(int32_t i = 0; i < choiceCount; i++)
            {
                int32_t idx = packet.ReadS32Little();
                int32_t messageID = packet.ReadS32Little();

                if(idx < 0)
                {
                    std::cerr << "Invalid prompt message key encountered " << idx
                        << std::endl;
                    return false;
                }

                if(idx > idxMax)
                {
                    idxMax = idx;
                }

                auto choice = std::make_shared<objects::EventChoice>();
                choice->SetMessageID(messageID);
                choices[idx] = choice;
            }

            auto prompt = std::make_shared<objects::EventPrompt>();

            prompt->SetMessageID(promptID);
            for(int32_t i = 0; i <= idxMax; i++)
            {
                auto choice = choices[i];
                if(choice == nullptr)
                {
                    choice = std::make_shared<objects::EventChoice>();
                }
                prompt->AppendChoices(choice);
            }

            instance->currentSequence = std::make_shared<MappedEvent>(prompt, source);
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_PLAY_SCENE):
        {
            if(5 != packet.Left())
            {
                std::cerr << "Bad event prompt packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto sceneID = packet.ReadS32Little();
            auto unknown = packet.ReadS8();

            auto scene = std::make_shared<objects::EventPlayScene>();
            instance->currentSequence = std::make_shared<MappedEvent>(scene, nullptr);

            scene->SetSceneID(sceneID);
            scene->SetUnknown(unknown);
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_OPEN_MENU):
        {
            if(8 > packet.Left())
            {
                std::cerr << "Bad event open menu packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto sourceEntityID = packet.ReadS32Little();
            auto menuType = packet.ReadS32Little();
            auto shopID = packet.ReadS32Little();

            if(!CheckUnknownEntity(capturePath, instance, sourceEntityID, "Open menu"))
            {
                return true;
            }

            auto source = sourceEntityID != 0 ? instance->entities[sourceEntityID] : nullptr;

            auto menu = std::make_shared<objects::EventOpenMenu>();
            menu->SetMenuType(menuType);
            menu->SetShopID(shopID);

            instance->currentSequence = std::make_shared<MappedEvent>(menu, source);
            endEvent = true;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_GET_ITEMS):
        {
            if(1 > packet.Left() || (uint32_t)((packet.PeekU8() * 6) + 1) != packet.Left())
            {
                std::cerr << "Bad event get items packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto itemCount = packet.ReadS8();
            std::unordered_map<uint32_t, uint16_t> items;

            for(int8_t i = 0; i < itemCount; i++)
            {
                items[packet.ReadU32Little()] = packet.ReadU16Little();
            }

            auto getItems = std::make_shared<objects::EventGetItem>();
            instance->currentSequence = std::make_shared<MappedEvent>(getItems, nullptr);
            getItems->SetItems(items);

            // Get item events don't wait for a response
            instance->eventResponse = 0;
            resetResponse = false;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_HOMEPOINT_UPDATE):
        {
            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            /// @todo: read zone/X/Y?
            auto home = std::make_shared<objects::EventHomepoint>();

            instance->currentSequence = std::make_shared<MappedEvent>(home, nullptr);
            endEvent = true;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_STAGE_EFFECT):
        {
            if(5 > packet.Left())
            {
                std::cerr << "Bad event stage effect packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto messageID = packet.ReadS32Little();
            auto effect1 = packet.ReadS8();

            bool effect2Set = packet.Left() > 0 ? packet.ReadS8() == 1 : false;
            auto effect2 = (packet.Left() > 3 && effect2Set) ? packet.ReadS32() : 0;

            auto effect = std::make_shared<objects::EventStageEffect>();
            instance->currentSequence = std::make_shared<MappedEvent>(effect, nullptr);

            effect->SetMessageID(messageID);
            effect->SetEffect1(effect1);
            effect->SetEffect2(effect2);

            // Stage effects don't wait for a response
            instance->eventResponse = 0;
            resetResponse = false;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_DIRECTION):
        {
            if(4 != packet.Left())
            {
                std::cerr << "Bad event direction packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto direction = packet.ReadS32Little();

            auto dir = std::make_shared<objects::EventDirection>();
            instance->currentSequence = std::make_shared<MappedEvent>(dir, nullptr);

            dir->SetDirection(direction);
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_SPECIAL_DIRECTION):
        {
            if(6 != packet.Left())
            {
                std::cerr << "Bad event special direction packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;

            auto special1 = packet.ReadU8();
            auto special2 = packet.ReadU8();
            auto direction = packet.ReadS32Little();

            auto dir = std::make_shared<objects::EventSpecialDirection>();
            instance->currentSequence = std::make_shared<MappedEvent>(dir, nullptr);

            dir->SetSpecial1(special1);
            dir->SetSpecial2(special2);
            dir->SetDirection(direction);

            // Special direction events don't wait for a response
            instance->eventResponse = 0;
            resetResponse = false;
        }
        break;
    case to_underlying(ClientToChannelPacketCode_t::PACKET_EVENT_RESPONSE):
        {
            if(4 != packet.Left())
            {
                std::cerr << "Bad event response packet found." << std::endl;

                return false;
            }

            instance->lastTrigger = instance->secondLastTrigger = nullptr;
            instance->eventResponse = packet.ReadS32Little();
            return true;
        }
        break;
    case to_underlying(ChannelToClientPacketCode_t::PACKET_EVENT_END):
        {
            endEvent = true;
        }
        break;
    default:
        return true;
        break;
    }

    // Wire up the events in sequence
    if(sequence && instance->currentSequence != sequence)
    {
        if(instance->eventResponse == -1)
        {
            std::cerr << "Event fired with no response from"
                " the previous event!" << std::endl;
            std::cerr << "Path: " << capturePath << std::endl;
            return false;
        }

        if(sequence->next.find(instance->eventResponse) != sequence->next.end())
        {
            auto next = sequence->next[instance->eventResponse];
            // If the event is equivalent to the one stored, step into that
            if(MergeEvents(next, instance->currentSequence, false, true))
            {
                next->mergeCount += (instance->currentSequence->mergeCount + 1);
                instance->currentSequence = next;
            }
            else
            {
                // Else set it as an alternate branch
                sequence->nextBranch[instance->eventResponse].push_back(
                    instance->currentSequence);
            }
        }
        else
        {
            sequence->next[instance->eventResponse] = instance->currentSequence;
            instance->currentSequence->previous = sequence;
        }
    }

    if(endEvent)
    {
        EndEvent();
    }
    else if(instance->currentSequence)
    {
        if(!instance->firstSequence)
        {
            instance->firstSequence = instance->currentSequence;
        }

        if(instance->currentSequence->previous.lock() &&
            instance->currentSequence->previous.lock()->previous.lock())
        {
            // If there are 3 or more events, traverse backwards
            // and attempt to merge the newest event with any
            // past ones
            auto previous = instance->currentSequence->previous.lock();
            auto back = previous;
            auto back2 = previous->previous.lock();
            while(back2)
            {
                if(MergeEvents(back2, instance->currentSequence, false, true))
                {
                    int32_t path = -1;
                    for(auto nPair : previous->next)
                    {
                        if(nPair.second == instance->currentSequence)
                        {
                            path = nPair.first;
                            previous->next.erase(path);
                            break;
                        }
                    }

                    if(path == -1)
                    {
                        std::cerr << "Event merging failed due to an"
                            " invalid sequence!" << std::endl;
                        std::cerr << "Path: " << capturePath << std::endl;
                        return false;
                    }

                    previous->next[path] = back2;
                    back2->mergeCount += (instance->currentSequence->mergeCount + 1);
                    instance->currentSequence = back2;
                    back2 = nullptr;
                }
                else
                {
                    back2 = back2->previous.lock();
                    back = back2;
                }
            }

        }
    }

    if(resetResponse)
    {
        instance->eventResponse = -1;
    }

    return true;
}

bool ZoneFilter::PostProcess()
{
    for(auto zonePair : mZones)
    {
        auto zone = zonePair.second;

        std::list<std::shared_ptr<MappedEvent>> events;
        for(auto instance : mZones[zone->GetID()]->instances)
        {
            if(instance->eventsInvalid) continue;

            for(auto e : instance->events)
            {
                events.push_back(e);
            }
        }

        // Flatten events starting with base level to ensure, they have
        // priority for being in the final set
        std::set<std::shared_ptr<MappedEvent>> seen;
        std::vector<std::shared_ptr<MappedEvent>> allEvents;
        for(auto mappedEvent : events)
        {
            seen.insert(mappedEvent);
            allEvents.push_back(mappedEvent);
        }

        // Flatten non-base level events
        for(auto mappedEvent : events)
        {
            std::list<std::shared_ptr<MappedEvent>> pendingEvents;
            while(pendingEvents.size() > 0)
            {
                auto p = pendingEvents.front();
                for(auto nextPair : p->next)
                {
                    if(seen.find(nextPair.second) == seen.end())
                    {
                        seen.insert(nextPair.second);
                        pendingEvents.push_back(nextPair.second);
                    }
                }

                for(auto branchPair : p->nextBranch)
                {
                    for(auto branch : branchPair.second)
                    {
                        if(seen.find(branch) == seen.end())
                        {
                            seen.insert(branch);
                            pendingEvents.push_back(branch);
                        }
                    }
                }

                if(p->previous.lock())
                {
                    allEvents.push_back(p);
                }
                pendingEvents.pop_front();
            }
        }

        // Merge events at all levels (this will invalidate "previous" values)
        size_t eventCount = 0;
        while(eventCount != allEvents.size())
        {
            eventCount = allEvents.size();

            for(size_t i = 0; i < allEvents.size(); i++)
            {
                for(size_t k = allEvents.size() - 1; k > i; k--)
                {
                    if(MergeEvents(allEvents[i], allEvents[k], false, false))
                    {
                        MergeEventReferences(allEvents[k], allEvents[i], allEvents);
                        allEvents.erase(allEvents.begin() + (int)k);
                    }
                }
            }
        }

        // Now merge branching paths at all levels
        eventCount = 0;
        while(eventCount != allEvents.size())
        {
            eventCount = allEvents.size();

            for(size_t i = 0; i < allEvents.size(); i++)
            {
                for(size_t k = allEvents.size() - 1; k > i; k--)
                {
                    if(MergeEvents(allEvents[i], allEvents[k], false, true))
                    {
                        MergeEventReferences(allEvents[k], allEvents[i], allEvents);

                        // Create branches if just the event matches
                        std::list<int32_t> keys;
                        for(auto nextPair : allEvents[i]->next)
                        {
                            keys.push_back(nextPair.first);
                        }

                        for(auto key : keys)
                        {
                            if(allEvents[k]->next.find(key) != allEvents[k]->next.end() &&
                                !MergeEvents(allEvents[i]->next[key],
                                    allEvents[k]->next[key], false, false))
                            {
                                bool merged = false;
                                for(auto branch : allEvents[i]->nextBranch[key])
                                {
                                    if(MergeEvents(branch, allEvents[k]->next[key],
                                        false, true))
                                    {
                                        merged = true;
                                        break;
                                    }
                                }

                                if(!merged)
                                {
                                    allEvents[i]->nextBranch[key].push_back(
                                        allEvents[k]->next[key]);
                                }
                            }
                        }

                        allEvents.erase(allEvents.begin() + (int)k);
                    }
                }
            }
        }

        // Recreate events list from leftover events and merge/sort branches
        events.clear();
        for(auto e : allEvents)
        {
            if(!e->previous.lock())
            {
                events.push_back(e);
            }

            std::list<int32_t> keys;
            for(auto nextPair : e->next)
            {
                keys.push_back(nextPair.first);
            }

            for(auto key : keys)
            {
                std::list<std::shared_ptr<MappedEvent>> branches;
                branches.push_back(e->next[key]);
                for(auto branch : e->nextBranch[key])
                {
                    bool merged = false;
                    for(auto b : branches)
                    {
                        if(MergeEvents(b, branch, false, true))
                        {
                            MergeEventReferences(branch, b, allEvents);
                            merged = true;
                            break;
                        }
                    }

                    if(!merged)
                    {
                        branches.push_back(branch);
                    }
                }

                // Sort branches by most merged (aka: seen the most)
                branches.sort([](const std::shared_ptr<MappedEvent> &a,
                    const std::shared_ptr<MappedEvent> &b)
                {
                    return a->mergeCount > b->mergeCount;
                });

                // First entry should be the next event
                e->next[key] = branches.front();
                branches.pop_front();

                // All remaining branches should be undefined branhc
                e->nextBranch[key].clear();
                for(auto branch : branches)
                {
                    e->nextBranch[key].push_back(branch);
                }
            }
        }

        // Sort base events by NPC then most merged (aka: seen the most)
        events.sort([](const std::shared_ptr<MappedEvent> &a,
            const std::shared_ptr<MappedEvent> &b)
        {
            auto sourceID1 = a->source != nullptr ? a->source->GetID() : 0;
            auto sourceID2 = b->source != nullptr ? b->source->GetID() : 0;
            return sourceID1 < sourceID2 ||
                (sourceID1 == sourceID2 && a->mergeCount > b->mergeCount);
        });

        // Reflatten
        seen.clear();
        allEvents.clear();
        for(auto mappedEvent : events)
        {
            std::list<std::shared_ptr<MappedEvent>> pendingEvents;
            pendingEvents.push_back(mappedEvent);
            seen.insert(mappedEvent);
            while(pendingEvents.size() > 0)
            {
                auto p = pendingEvents.front();
                for(auto nextPair : p->next)
                {
                    if(seen.find(nextPair.second) == seen.end())
                    {
                        seen.insert(nextPair.second);
                        pendingEvents.push_back(nextPair.second);
                    }
                }

                for(auto branchPair : p->nextBranch)
                {
                    for(auto branch : branchPair.second)
                    {
                        if(seen.find(branch) == seen.end())
                        {
                            seen.insert(branch);
                            pendingEvents.push_back(branch);
                        }
                    }
                }

                allEvents.push_back(p);
                pendingEvents.pop_front();
            }
        }

        // Assign unique IDs to all remaining
        std::unordered_map<std::string, uint32_t> counts;
        for(auto mappedEvent : allEvents)
        {
            std::string eventID;

            auto e = mappedEvent->event;
            switch(e->GetEventType())
            {
                case objects::Event::EventType_t::MESSAGE:
                    eventID = "Z%1_MS%2";
                    break;
                case objects::Event::EventType_t::NPC_MESSAGE:
                    eventID = "Z%1_NM%2";
                    break;
                case objects::Event::EventType_t::EX_NPC_MESSAGE:
                    eventID = "Z%1_EX%2";
                    break;
                case objects::Event::EventType_t::MULTITALK:
                    eventID = "Z%1_ML%2";
                    break;
                case objects::Event::EventType_t::PROMPT:
                    eventID = "Z%1_PR%2";
                    break;
                case objects::Event::EventType_t::PLAY_SCENE:
                    eventID = "Z%1_SC%2";
                    break;
                case objects::Event::EventType_t::OPEN_MENU:
                    eventID = "Z%1_ME%2";
                    break;
                case objects::Event::EventType_t::GET_ITEMS:
                    eventID = "Z%1_IT%2";
                    break;
                case objects::Event::EventType_t::HOMEPOINT:
                    eventID = "Z%1_HP%2";
                    break;
                case objects::Event::EventType_t::STAGE_EFFECT:
                    eventID = "Z%1_SE%2";
                    break;
                case objects::Event::EventType_t::DIRECTION:
                    eventID = "Z%1_DR%2";
                    break;
                case objects::Event::EventType_t::SPECIAL_DIRECTION:
                    eventID = "Z%1_SD%2";
                    break;
                default:
                    break;
            }

            if(counts.find(eventID) == counts.end())
            {
                counts[eventID] = 0;
            }

            std::ostringstream ss;
            ss << std::setw(3) << std::setfill('0') << ++counts[eventID];
            e->SetID(libcomp::String(eventID).Arg(zone->GetID()).Arg(ss.str()));
        }

        // Map and assign events
        seen.clear();
        std::list<std::shared_ptr<objects::Event>> mappedEvents;
        std::list<std::shared_ptr<objects::Event>> unmappedEvents;
        for(auto mappedEvent : events)
        {
            std::list<std::shared_ptr<objects::Event>> eventSet;
            std::list<std::shared_ptr<MappedEvent>> pendingEvents;
            pendingEvents.push_back(mappedEvent);
            while(pendingEvents.size() > 0)
            {
                auto p = pendingEvents.front();
                auto e = p->event;

                seen.insert(p);
                for(auto nextPair : p->next)
                {
                    auto nextID = nextPair.second->event->GetID();
                    if(seen.find(nextPair.second) == seen.end())
                    {
                        pendingEvents.push_back(nextPair.second);
                        seen.insert(nextPair.second);
                    }

                    switch(e->GetEventType())
                    {
                        case objects::Event::EventType_t::PROMPT:
                            {
                                auto prompt = std::dynamic_pointer_cast<
                                    objects::EventPrompt>(e);
                                prompt->GetChoices((size_t)nextPair.first)
                                    ->SetNext(nextID);
                            }
                            break;
                        default:
                            e->SetNext(nextID);
                            break;
                    }
                }

                for(auto branchPair : p->nextBranch)
                {
                    for(auto branch : branchPair.second)
                    {
                        if(seen.find(branch) == seen.end())
                        {
                            pendingEvents.push_back(branch);
                            seen.insert(branch);
                        }

                        auto b = std::make_shared<objects::EventBase>();
                        b->SetNext(branch->event->GetID());
                        b->SetConditionID("unknown");

                        switch(e->GetEventType())
                        {
                            case objects::Event::EventType_t::PROMPT:
                                {
                                    auto prompt = std::dynamic_pointer_cast<
                                        objects::EventPrompt>(e);
                                    prompt->GetChoices((size_t)branchPair.first)
                                        ->AppendBranches(b);
                                }
                                break;
                            default:
                                e->AppendBranches(b);
                                break;
                        }
                    }
                }

                eventSet.push_back(p->event);
                pendingEvents.pop_front();
            }

            if(mappedEvent->source &&
                mappedEvent->source->ActionsCount() == 0)
            {
                auto startEvent = std::make_shared<objects::ActionStartEvent>();
                startEvent->SetEventID(mappedEvent->event->GetID());
                mappedEvent->source->AppendActions(startEvent);

                for(auto e : eventSet)
                {
                    mappedEvents.push_back(e);
                }
            }
            else
            {
                for(auto e : eventSet)
                {
                    unmappedEvents.push_back(e);
                }
            }
        }

        // Add any leftover zone change actions as unknown spots
        uint32_t unknownSpotID = 1;
        for(auto connectPair : mZones[zone->GetID()]->connections)
        {
            // Make sure we don't enter a dupe
            while(zone->GetSpots(unknownSpotID))
            {
                unknownSpotID++;
            }

            auto action = connectPair.second;

            auto spot = std::make_shared<
                objects::ServerZoneSpot>();
            spot->SetID(unknownSpotID++);
            spot->AppendActions(action);

            zone->SetSpots(spot->GetID(), spot);
        }

        // Sort the server objects by ID
        auto npcs = zone->GetNPCs();
        npcs.sort([](const std::shared_ptr<objects::ServerNPC>& a,
            const std::shared_ptr<objects::ServerNPC>& b)
            {
                return a->GetID() < b->GetID();
            });
        zone->SetNPCs(npcs);

        auto objects = zone->GetObjects();
        objects.sort([](const std::shared_ptr<objects::ServerObject>& a,
            const std::shared_ptr<objects::ServerObject>& b)
            {
                return a->GetID() < b->GetID();
            });
        zone->SetObjects(objects);

        // Save to XML
        {
            tinyxml2::XMLDocument doc;

            tinyxml2::XMLElement *pRoot = doc.NewElement("objects");
            doc.InsertEndChild(pRoot);

            if(!zone->Save(doc, *pRoot))
            {
                return false;
            }

            std::list<libcomp::String> objectNames;
            std::list<libcomp::String> npcNames;

            for(auto obj : zone->GetObjects())
            {
                auto def = mDefinitions.GetONPCData(obj->GetID());

                if(def)
                {
                    objectNames.push_back(def->GetName());
                }
                else
                {
                    objectNames.push_back(libcomp::String());
                }
            }

            auto pElement = pRoot->FirstChildElement("object"
                )->FirstChildElement("member");

            while(nullptr != pElement)
            {
                if(libcomp::String("Objects") == pElement->Attribute("name"))
                {
                    break;
                }

                pElement = pElement->NextSiblingElement("member");
            }

            if(nullptr != pElement)
            {
                auto pChild = pElement->FirstChildElement("element");

                while(nullptr != pChild)
                {
                    auto name = objectNames.front();
                    objectNames.pop_front();

                    if(!name.IsEmpty())
                    {
                        pChild->InsertFirstChild(doc.NewComment(libcomp::String(
                            " %1 ").Arg(name).C()));
                    }

                    pChild = pChild->NextSiblingElement("element");
                }
            }

            for(auto obj : zone->GetNPCs())
            {
                auto def = mDefinitions.GetHNPCData(obj->GetID());

                if(def)
                {
                    npcNames.push_back(def->GetBasic()->GetName());
                }
                else
                {
                    npcNames.push_back(libcomp::String());
                }
            }

            pElement = pRoot->FirstChildElement("object"
                )->FirstChildElement("member");

            while(nullptr != pElement)
            {
                if(libcomp::String("NPCs") == pElement->Attribute("name"))
                {
                    break;
                }

                pElement = pElement->NextSiblingElement("member");
            }

            if(nullptr != pElement)
            {
                auto pChild = pElement->FirstChildElement("element");

                while(nullptr != pChild)
                {
                    auto name = npcNames.front();
                    npcNames.pop_front();

                    if(!name.IsEmpty())
                    {
                        pChild->InsertFirstChild(doc.NewComment(libcomp::String(
                            " %1 ").Arg(name).C()));
                    }

                    pChild = pChild->NextSiblingElement("element");
                }
            }

            auto zoneDefinition = mDefinitions.GetZoneData(zone->GetID());
            if(zoneDefinition)
            {
                auto name = zoneDefinition->GetBasic()->GetName();

                if(!name.IsEmpty())
                {
                    pRoot->InsertFirstChild(doc.NewComment(libcomp::String(
                        " %1 ").Arg(name).C()));
                }
            }

            /*
            tinyxml2::XMLPrinter printer;
            doc.Print(&printer);

            std::cout << printer.CStr() << std::endl;
            */

            if(tinyxml2::XML_NO_ERROR != doc.SaveFile(libcomp::String(
                "zone-%1.xml").Arg(zone->GetID()).C()))
            {
                std::cerr << "Failed to save zone XML file." << std::endl;

                return false;
            }
        }

        if(mappedEvents.size() > 0 || unmappedEvents.size() > 0)
        {
            tinyxml2::XMLDocument eventDoc;

            tinyxml2::XMLElement *pRoot = eventDoc.NewElement("objects");
            eventDoc.InsertEndChild(pRoot);

            for(auto event : mappedEvents)
            {
                if(!event->Save(eventDoc, *pRoot))
                {
                    return false;
                }
            }

            if(unmappedEvents.size() > 0)
            {
                tinyxml2::XMLElement *invalid = eventDoc.NewElement("unmapped");
                pRoot->InsertEndChild(invalid);

                for(auto event : unmappedEvents)
                {
                    if(!event->Save(eventDoc, *invalid))
                    {
                        return false;
                    }
                }
            }

            if(tinyxml2::XML_NO_ERROR != eventDoc.SaveFile(libcomp::String(
                "zone_events-%1.xml").Arg(zone->GetID()).C()))
            {
                std::cerr << "Failed to save events XML file." << std::endl;

                return false;
            }
        }
    }

    return true;
}

std::shared_ptr<objects::ServerNPC> ZoneFilter::GetHNPC(
    std::shared_ptr<Zone> zone, uint32_t objectId,
    float originX, float originY, float originRotation)
{
    // Adjust for slight differences
    auto x = floor(originX);
    auto y = floor(originY);
    auto rot = floor(originRotation);

    for(auto obj : zone->GetNPCs())
    {
        if(obj->GetID() == objectId && floor(obj->GetX()) == x &&
            floor(obj->GetY()) == y && floor(obj->GetRotation()) == rot)
        {
            return obj;
        }
    }

    return nullptr;
}

std::shared_ptr<objects::ServerObject> ZoneFilter::GetONPC(
    std::shared_ptr<Zone> zone, uint32_t objectId,
    float originX, float originY, float originRotation)
{
    // Adjust for slight differences
    auto x = floor(originX);
    auto y = floor(originY);
    auto rot = floor(originRotation);

    for(auto obj : zone->GetObjects())
    {
        if(obj->GetID() == objectId && floor(obj->GetX()) == x &&
            floor(obj->GetY()) == y && floor(obj->GetRotation()) == rot)
        {
            return obj;
        }
    }

    return nullptr;
}

bool ZoneFilter::CheckUnknownEntity(const libcomp::String& capturePath, 
    const std::shared_ptr<ZoneInstance>& instance,
    int32_t entityID, const libcomp::String& packetType)
{
    auto it = instance->entities.find(entityID);

    if(entityID != 0 && instance->entities.end() == it)
    {
        std::cerr << "'" << packetType << "' packet mapped to an unknown entity!"
            " Events will not be used for this file." << std::endl;
        std::cerr << "Entity ID: " << entityID << std::endl;
        std::cerr << "Path: " << capturePath << std::endl;

        instance->eventsInvalid = true;
        return false;
    }

    return true;
}

void ZoneFilter::EndEvent()
{
    auto instance = mCurrentZoneID != 0
        ? mZones[mCurrentZoneID]->instances.back() : nullptr;

    if(instance)
    {
        instance->secondLastTrigger = instance->lastTrigger;
        instance->lastTrigger = nullptr;
        if(instance->currentSequence)
        {
            instance->events.push_back(instance->firstSequence != nullptr
                ? instance->firstSequence : instance->currentSequence);
            instance->firstSequence = instance->currentSequence = nullptr;
        }
    }
}

void ZoneFilter::RegisterZone(uint32_t zoneID, uint32_t dynamicMapID)
{
    auto zone = std::make_shared<Zone>();
    zone->SetID(zoneID);
    zone->SetDynamicMapID(dynamicMapID);
    zone->SetGlobal(true);
    zone->SetStartingX(0.0f);
    zone->SetStartingY(0.0f);
    zone->SetStartingRotation(0.0f);

    mZones[zoneID] = zone;

    // Add in information about connected zones.
    auto zoneRelations = mDefinitions.GetZoneRelationData(zone->GetID());
    if(zoneRelations)
    {
        for(auto connectedZone : zoneRelations->GetConnectedZones())
        {
            if(connectedZone && 0 != connectedZone->GetZoneID())
            {
                // Find the one that points back here.
                auto otherZoneRelations = mDefinitions.GetZoneRelationData(
                    connectedZone->GetZoneID());

                if(otherZoneRelations)
                {
                    // Now find the original zone in the list.
                    for(auto backConnection : otherZoneRelations->GetConnectedZones())
                    {
                        if(backConnection->GetZoneID() == zone->GetID())
                        {
                            auto action = std::make_shared<
                                objects::ActionZoneChange>();
                            action->SetActionType(
                                objects::Action::ActionType_t::ZONE_CHANGE);
                            action->SetZoneID(connectedZone->GetZoneID());
                            action->SetDestinationX(
                                backConnection->GetSourceX());
                            action->SetDestinationY(
                                backConnection->GetSourceY());
                            action->SetDestinationRotation(0.0f);
                            mZones[zoneID]->allConnections[connectedZone->GetZoneID()] = action;

                            // We are done now.
                            break;
                        }
                    }
                }
            }
        }
    }

    mZones[zoneID]->connections = mZones[zoneID]->allConnections;
}

void ZoneFilter::BindZoneChangeEvent(uint32_t zoneID, float x, float y,
    float rot)
{
    (void)x;
    (void)y;

    auto instance = mCurrentZoneID != 0
        ? mZones[mCurrentZoneID]->instances.back() : nullptr;

    if(!instance || (!instance->lastTrigger && !instance->secondLastTrigger))
    {
        return;
    }

    auto it = mZones[mCurrentZoneID]->allConnections.find(zoneID);
    if(mZones[mCurrentZoneID]->allConnections.end() == it /*||
        it->second->GetDestinationX() != x ||
        it->second->GetDestinationY() != y*/)
    {
        // Unknown/invalid, move on
        return;
    }

    if(it->second->GetDestinationRotation() == 0 &&
        it->second->GetDestinationRotation() != rot)
    {
        it->second->SetDestinationRotation(rot);
    }

    auto trigger = instance->lastTrigger != 0
        ? instance->lastTrigger : instance->secondLastTrigger;

    if(!trigger->valid)
    {
        return;
    }

    trigger->packet.Rewind();

    if(trigger->commandCode == to_underlying(
        ClientToChannelPacketCode_t::PACKET_OBJECT_INTERACTION))
    {
        auto entityID = trigger->packet.ReadS32Little();
        auto entity = instance->entities[entityID];

        // Don't add multiple actions automatically
        if(entity->ActionsCount() == 0)
        {
            entity->AppendActions(it->second);
        }
        else
        {
            return;
        }
    }
    else
    {
        // Sanity check that spot triggering fired close to the zone change event
        if((trigger->packetNumber + 10) < instance->packetNumber)
        {
            return;
        }

        auto entityID = trigger->packet.ReadS32Little();
        auto spotID = trigger->packet.ReadU32Little();
        (void)entityID;

        if(!mZones[mCurrentZoneID]->GetSpots(spotID))
        {
            auto spot = std::make_shared<
                objects::ServerZoneSpot>();
            spot->SetID(spotID);
            spot->AppendActions(it->second);

            mZones[mCurrentZoneID]->SetSpots(spotID, spot);
        }
    }

    mZones[mCurrentZoneID]->connections.erase(zoneID);
}

void ZoneFilter::MergeEventReferences(const std::shared_ptr<MappedEvent>& from,
    const std::shared_ptr<MappedEvent>& to,
    std::vector<std::shared_ptr<MappedEvent>>& allEvents)
{
    for(size_t i = 0; i < allEvents.size(); i++)
    {
        if(allEvents[i] == to || allEvents[i] == from) continue;

        // Re-associate next
        std::list<int32_t> keys;
        for(auto nextPair : allEvents[i]->next)
        {
            keys.push_back(nextPair.first);
        }

        for(auto key : keys)
        {
            if(allEvents[i]->next[key] == from)
            {
                allEvents[i]->next[key] = to;
            }
        }

        // Re-associate branches
        keys.clear();
        for(auto branchPair : allEvents[i]->nextBranch)
        {
            keys.push_back(branchPair.first);
        }

        for(auto key : keys)
        {
            for(size_t k = 0; k < allEvents[i]->nextBranch[key].size(); k++)
            {
                if(allEvents[i]->nextBranch[key][k] == from)
                {
                    allEvents[i]->nextBranch[key][k] = to;
                }
            }
        }
    }

    to->mergeCount += (from->mergeCount + 1);
}

bool ZoneFilter::MergeEvents(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    if(e1->event->GetEventType() != e2->event->GetEventType() ||
        e1->source != e2->source)
    {
        return false;
    }

    if(seen.find(e1) != seen.end() &&
        seen.find(e2) != seen.end())
    {
        return true;
    }
    seen.insert(e1);
    seen.insert(e2);

    switch(e1->event->GetEventType())
    {
        case objects::Event::EventType_t::MESSAGE:
            return MergeEventMessages(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::NPC_MESSAGE:
            return MergeEventNPCMessages(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::EX_NPC_MESSAGE:
            return MergeEventExNPCMessages(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::MULTITALK:
            return MergeEventMultitalks(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::PROMPT:
            return MergeEventPrompts(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::PLAY_SCENE:
            return MergeEventPlayScenes(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::OPEN_MENU:
            return MergeEventMenus(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::GET_ITEMS:
            return MergeEventGetItems(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::HOMEPOINT:
            // Homepoint requests currently cannot differ
            return true;
            break;
        case objects::Event::EventType_t::STAGE_EFFECT:
            return MergeEventStageEffects(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::DIRECTION:
            return MergeEventDirections(e1, e2, checkOnly, flat, seen);
            break;
        case objects::Event::EventType_t::SPECIAL_DIRECTION:
            return MergeEventSpecialDirections(e1, e2, checkOnly, flat, seen);
            break;
        default:
            return false;
    }
}

bool ZoneFilter::MergeEventMessages(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventMessage>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventMessage>(e2->event);
    if(c1->MessageIDsCount() != c2->MessageIDsCount())
    {
        return false;
    }

    size_t messageCount = c1->MessageIDsCount();
    for(size_t i = 0; i < messageCount; i++)
    {
        if(c1->GetMessageIDs(i) != c2->GetMessageIDs(i))
        {
            return false;
        }
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventNPCMessages(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventNPCMessage>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventNPCMessage>(e2->event);
    if(c1->MessageIDsCount() != c2->MessageIDsCount())
    {
        return false;
    }

    size_t messageCount = c1->MessageIDsCount();
    for(size_t i = 0; i < messageCount; i++)
    {
        auto unknown1 = c1->GetUnknown(i) != 0
            ? c1->GetUnknown(i) : c1->GetUnknownDefault();
        auto unknown2 = c2->GetUnknown(i) != 0
            ? c2->GetUnknown(i) : c2->GetUnknownDefault();
        if(c1->GetMessageIDs(i) != c2->GetMessageIDs(i) ||
            unknown1 != unknown2)
        {
            return false;
        }
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventExNPCMessages(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventExNPCMessage>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventExNPCMessage>(e2->event);
    if(c1->GetMessageID() != c2->GetMessageID())
    {
        return false;
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventMultitalks(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventMultitalk>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventMultitalk>(e2->event);
    if(c1->GetMessageID() != c2->GetMessageID())
    {
        return false;
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventPrompts(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventPrompt>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventPrompt>(e2->event);
    if(c1->GetMessageID() != c2->GetMessageID())
    {
        return false;
    }

    // Copy lists to vectors and normalize the size
    std::vector<std::shared_ptr<objects::EventChoice>> choices1;
    for(auto choice : c1->GetChoices())
    {
        choices1.push_back(choice);
    }

    std::vector<std::shared_ptr<objects::EventChoice>> choices2;
    for(auto choice : c2->GetChoices())
    {
        choices2.push_back(choice);
    }

    auto maxSize = choices1.size() > choices2.size()
        ? choices1.size() : choices2.size();
    for(auto c : { &choices1, &choices2 })
    {
        while(c->size() < maxSize)
        {
            c->push_back(nullptr);
        }
    }

    // If the events are the same or missing in either, it can merge
    for(size_t i = 0; i < maxSize; i++)
    {
        int32_t key = (int32_t)i;

        if(choices1[i] != nullptr && choices2[i] != nullptr &&
            choices1[i]->GetMessageID() != 0 && choices2[i]->GetMessageID() != 0 &&
            choices1[i]->GetMessageID() != choices2[i]->GetMessageID())
        {
            return false;
        }

        if(flat) continue;

        auto it1 = e1->next.find(key);
        auto it2 = e2->next.find(key);
        if(it1 == e1->next.end() || it2 == e2->next.end())
        {
            continue;
        }

        if(!MergeEvents(it1->second, it2->second, true, false, seen))
        {
            return false;
        }
    }

    if(checkOnly)
    {
        return true;
    }

    for(size_t i = 0; i < maxSize; i++)
    {
        // Merge all next event information
        int32_t key = (int32_t)i;

        auto it1 = e1->next.find(key);
        auto it2 = e2->next.find(key);
        if(it2 != e2->next.end())
        {
            if(it1 == e1->next.end())
            {
                e1->next[key] = it2->second;
                e1->next[key]->previous = e1;
                choices1[i] = choices2[i];
            }
            else if(!flat)
            {
                MergeEvents(it1->second, it2->second, false, false, seen);
            }
        }
        else if(it1 == e1->next.end() &&
            (choices1[i] == nullptr || choices1[i]->GetMessageID() == 0) &&
            choices2[i] != nullptr)
        {
            // Try to get the message even if the event is unknown
            choices1[i] = choices2[i];
        }
    }

    c1->ClearChoices();
    for(auto choice : choices1)
    {
        c1->AppendChoices(choice != nullptr ? choice : std::make_shared<objects::EventChoice>());
    }

    return true;
}

bool ZoneFilter::MergeEventPlayScenes(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventPlayScene>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventPlayScene>(e2->event);
    if(c1->GetSceneID() != c2->GetSceneID() ||
        c1->GetUnknown() != c2->GetUnknown())
    {
        return false;
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventMenus(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventOpenMenu>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventOpenMenu>(e2->event);
    if(c1->GetMenuType() != c2->GetMenuType() ||
        c1->GetShopID() != c2->GetShopID())
    {
        return false;
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventGetItems(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventGetItem>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventGetItem>(e2->event);
    if(c1->ItemsCount() != c2->ItemsCount())
    {
        return false;
    }

    for(auto item : c1->GetItems())
    {
        if(!c2->ItemsKeyExists(item.first) ||
            c2->GetItems(item.first) != item.second)
        {
            return false;
        }
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventStageEffects(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventStageEffect>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventStageEffect>(e2->event);
    if(c1->GetMessageID() != c2->GetMessageID() ||
        c1->GetEffect1() != c2->GetEffect1() ||
        c1->GetEffect2() != c2->GetEffect2())
    {
        return false;
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventDirections(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventDirection>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventDirection>(e2->event);
    if(c1->GetDirection() != c2->GetDirection())
    {
        return false;
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeEventSpecialDirections(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    auto c1 = std::dynamic_pointer_cast<objects::EventSpecialDirection>(e1->event);
    auto c2 = std::dynamic_pointer_cast<objects::EventSpecialDirection>(e2->event);
    if(c1->GetDirection() != c2->GetDirection() ||
        c1->GetSpecial1() != c2->GetSpecial1() ||
        c1->GetSpecial2() != c2->GetSpecial2())
    {
        return false;
    }

    return MergeNextGeneric(e1, e2, checkOnly, flat, seen);
}

bool ZoneFilter::MergeNextGeneric(std::shared_ptr<MappedEvent> e1,
    std::shared_ptr<MappedEvent> e2, bool checkOnly, bool flat,
    std::set<std::shared_ptr<MappedEvent>> seen)
{
    if(flat)
    {
        return true;
    }

    if(e1->next.size() != e2->next.size())
    {
        return false;
    }

    for(auto nPair : e1->next)
    {
        auto key = nPair.first;
        if(e2->next.find(key) == e2->next.end())
        {
            return false;
        }

        if(!MergeEvents(e1->next[key], e2->next[key], true,
            false, seen))
        {
            return false;
        }
    }

    if(checkOnly)
    {
        return true;
    }

    for(auto nPair : e1->next)
    {
        auto key = nPair.first;
        MergeEvents(e1->next[key], e2->next[key], false, false, seen);
    }

    return true;
}
