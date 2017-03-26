/**
 * @file tools/bf2comp/src/main.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to filter through a directory of captures.
 *
 * This tool will filter through a directory of captures.
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

// libcomp Includes
#include <CString.h>
#include <DataStore.h>
#include <DefinitionManager.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <MiHNPCData.h>
#include <MiHNPCBasicData.h>
#include <MiONPCData.h>
#include <MiZoneData.h>
#include <MiZoneBasicData.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>

// Qt Includes
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>

// tinyxml2 Includes
#include <PushIgnore.h>
#include <tinyxml2.h>
#include <PopIgnore.h>

// C++11 Includes
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unordered_map>

class CaptureFile;

class CaptureEvent
{
    friend class CaptureFile;

public:
    CaptureEvent();

    std::list<std::pair<uint16_t, libcomp::ReadOnlyPacket>
        >::const_iterator begin() const;
    std::list<std::pair<uint16_t, libcomp::ReadOnlyPacket>
        >::const_iterator end() const;

protected:
    bool Load(std::istream& file, uint32_t version);

private:
    bool ParsePacket(libcomp::Packet& packet);
    bool DecompressPacket(libcomp::Packet& packet, uint32_t& paddedSize,
        uint32_t& realSize, uint32_t& dataStart);

    uint64_t mTimestamp;
    uint64_t mMicroTime;
    uint8_t mSource;

    libcomp::ReadOnlyPacket mPacket;
    std::list<std::pair<uint16_t, libcomp::ReadOnlyPacket>> mCommands;
};

class CaptureFile
{
public:
    CaptureFile();
    ~CaptureFile();

    bool Load(const libcomp::String& path);

    std::list<CaptureEvent*>::const_iterator begin() const;
    std::list<CaptureEvent*>::const_iterator end() const;

    // HACK
    static const uint32_t FORMAT_MAGIC = 0x4B434148;

    // Major, Minor, Patch (1.0.0)
    static const uint32_t FORMAT_VER1  = 0x00010000;

    // Major, Minor, Patch (1.1.0)
    static const uint32_t FORMAT_VER2  = 0x00010100;

private:
    libcomp::String mPath;
    std::list<CaptureEvent*> mEvents;
};

CaptureEvent::CaptureEvent()
{
}

std::list<std::pair<uint16_t, libcomp::ReadOnlyPacket>
    >::const_iterator CaptureEvent::begin() const
{
    return mCommands.begin();
}

std::list<std::pair<uint16_t, libcomp::ReadOnlyPacket>
    >::const_iterator CaptureEvent::end() const
{
    return mCommands.end();
}

bool CaptureEvent::DecompressPacket(libcomp::Packet& packet,
    uint32_t& paddedSize, uint32_t& realSize, uint32_t& dataStart)
{
    // Make sure we are at the right spot (right after the sizes).
    packet.Seek(2 * sizeof(uint32_t));

    // All packets that support compression have this.
    if(0x677A6970 != packet.ReadU32Big()) // "gzip"
    {
        return false;
    }

    // Read the sizes.
    int32_t uncompressedSize = packet.ReadS32Little();
    int32_t compressedSize = packet.ReadS32Little();

    // Sanity check the sizes.
    if(0 > uncompressedSize || 0 > compressedSize)
    {
        return false;
    }

    // Check that the compression is as expected.
    if(0x6C763600 != packet.ReadU32Big()) // "lv6\0"
    {
        return false;
    }

    // Calculate how much data is padding
    uint32_t padding = paddedSize - realSize;

    // Make sure the packet is the right size.
    if(packet.Left() != (static_cast<uint32_t>(compressedSize) + padding))
    {
        return false;
    }

    // Only decompress if the sizes are not the same.
    if(compressedSize != uncompressedSize)
    {
        // Attempt to decompress.
        int32_t sz = packet.Decompress(compressedSize);

        // Check the uncompressed size matches the recorded size.
        if(sz != uncompressedSize)
        {
            return false;
        }

        // The is no padding anymore.
        paddedSize = realSize = static_cast<uint32_t>(sz);
    }

    // Skip over: gzip, lv0, uncompressedSize, compressedSize.
    dataStart += static_cast<uint32_t>(sizeof(uint32_t) * 4);

    return true;
}

bool CaptureEvent::ParsePacket(libcomp::Packet& packet)
{
    // Read the sizes.
    uint32_t paddedSize = packet.ReadU32Big();
    uint32_t realSize = packet.ReadU32Big();

    // This is where to find the data.
    uint32_t dataStart = 2 * sizeof(uint32_t);

    // Decompress the packet.
    if(!DecompressPacket(packet, paddedSize, realSize, dataStart))
    {
        return false;
    }

    // Move the packet into a read only copy.
    libcomp::ReadOnlyPacket copy(std::move(packet));

    // Make sure we are at the right spot (right after the sizes).
    copy.Seek(dataStart);

    // Calculate how much data is padding.
    uint32_t padding = paddedSize - realSize;

    // This will stop the command parsing.
    bool errorFound = false;

    // Keep reading each command (sometimes called a packet) inside the
    // decrypted packet from the network socket.
    while(!errorFound && copy.Left() > padding)
    {
        // Make sure there is enough data
        if(copy.Left() < 3 * sizeof(uint16_t))
        {
            errorFound = true;
        }
        else
        {
            // Skip over the big endian size (we think).
            copy.Skip(2);

            // Remember where this command started so we may advance over it
            // after it has been parsed.
            uint32_t commandStart = copy.Tell();
            uint16_t commandSize = copy.ReadU16Little();
            uint16_t commandCode = copy.ReadU16Little();

            // With no data, the command size is 4 bytes (code + a size).
            if(commandSize < 2 * sizeof(uint16_t))
            {
                errorFound = true;
            }

            // Check there is enough packet left for the command data.
            if(!errorFound && copy.Left() < (uint32_t)(commandSize -
                2 * sizeof(uint16_t)))
            {
                errorFound = true;
            }

            if(!errorFound)
            {
                // This is a shallow copy of the command data.
                libcomp::ReadOnlyPacket command(copy, commandStart +
                    2 * static_cast<uint32_t>(sizeof(uint16_t)),
                    commandSize - 2 * static_cast<uint32_t>(
                    sizeof(uint16_t)));

                mCommands.push_back(std::make_pair(commandCode, command));
            }

            // Move to the next command.
            if(!errorFound)
            {
                copy.Seek(commandStart + commandSize);
            }
        }
    } // while(!errorFound && packet.Left() > padding)

    if(!errorFound)
    {
        // Skip the padding
        copy.Skip(padding);
    }

    if(errorFound || copy.Left() != 0)
    {
        return false;
    }

    mPacket = copy;

    return true;
}

bool CaptureEvent::Load(std::istream& file, uint32_t version)
{
    if(!file.read(reinterpret_cast<char*>(&mSource), sizeof(mSource)).good())
    {
        return file.eof() ? true : false;
    }

    if(CaptureFile::FORMAT_VER1 == version)
    {
        if(!file.read(reinterpret_cast<char*>(&mTimestamp),
            sizeof(uint32_t)).good())
        {
            return false;
        }
    }
    else
    {
        if(!file.read(reinterpret_cast<char*>(&mTimestamp),
            sizeof(uint64_t)).good())
        {
            return false;
        }

        if(!file.read(reinterpret_cast<char*>(&mMicroTime),
            sizeof(mMicroTime)).good())
        {
            return false;
        }
    }

    uint32_t size;

    if(!file.read(reinterpret_cast<char*>(&size), sizeof(size)).good())
    {
        return false;
    }

    char *pBuffer = new char[size];

    if(nullptr == pBuffer)
    {
        return false;
    }

    if(!file.read(pBuffer, size).good())
    {
        delete[] pBuffer;

        return false;
    }

    libcomp::Packet packet(pBuffer, size);

    try
    {
        if(!ParsePacket(packet))
        {
            delete[] pBuffer;

            return false;
        }
    }
    catch(...)
    {
        delete[] pBuffer;

        return false;
    }

    delete[] pBuffer;

    return true;
}

CaptureFile::CaptureFile()
{
}

CaptureFile::~CaptureFile()
{
    for(auto pEvent : mEvents)
    {
        delete pEvent;
    }
}

bool CaptureFile::Load(const libcomp::String& path)
{
    std::ifstream file;
    file.open(path.C(), std::ifstream::binary);

    if(!file.good())
    {
        return false;
    }

    uint32_t magic;

    if(!file.read(reinterpret_cast<char*>(&magic), sizeof(magic)).good() ||
        FORMAT_MAGIC != magic)
    {
        return false;
    }

    uint32_t version;

    if(!file.read(reinterpret_cast<char*>(&version), sizeof(version)).good() ||
        (FORMAT_VER1 != version && FORMAT_VER2 != version))
    {
        return false;
    }

    uint64_t stamp;
    uint32_t addrlen;
    char address[1024];

    memset(address, 0, 1024);

    if(FORMAT_VER1 == version)
    {
        if(!file.read(reinterpret_cast<char*>(&stamp), sizeof(uint32_t)))
        {
            return false;
        }
    }
    else
    {
        if(!file.read(reinterpret_cast<char*>(&stamp), sizeof(uint64_t)))
        {
            return false;
        }
    }

    if(!file.read(reinterpret_cast<char*>(&addrlen), sizeof(addrlen)))
    {
        return false;
    }

    if(!file.read(address, addrlen))
    {
        return false;
    }

    while(!file.eof())
    {
        auto pEvent = new CaptureEvent;

        if(!pEvent->Load(file, version))
        {
            return false;
        }

        mEvents.push_back(pEvent);
    }

    mPath = path;

    return true;
}

std::list<CaptureEvent*>::const_iterator CaptureFile::begin() const
{
    return mEvents.begin();
}

std::list<CaptureEvent*>::const_iterator CaptureFile::end() const
{
    return mEvents.end();
}

class CommandFilter
{
public:
    virtual ~CommandFilter();

    virtual bool ProcessCommand(uint16_t commandCode,
        libcomp::ReadOnlyPacket& packet) = 0;
    virtual bool PostProcess() = 0;
};

CommandFilter::~CommandFilter()
{
}

class ZoneFilter : public CommandFilter
{
public:
    ZoneFilter(const char *szProgram, const libcomp::String& dataStorePath);
    virtual ~ZoneFilter();

    virtual bool ProcessCommand(uint16_t commandCode,
        libcomp::ReadOnlyPacket& packet);
    virtual bool PostProcess();

private:
    bool HasHNPC(const std::shared_ptr<objects::ServerZone>& zone,
        uint32_t objectId, float originX, float originY, float originRotation);
    bool HasONPC(const std::shared_ptr<objects::ServerZone>& zone,
        uint32_t objectId, float originX, float originY, float originRotation);

    libcomp::DataStore mStore;
    libcomp::DefinitionManager mDefinitions;

    //std::shared_ptr<objects::ServerZone> mActiveZone;
    std::unordered_map<uint64_t, std::shared_ptr<objects::ServerZone>> mZones;
};

ZoneFilter::ZoneFilter(const char *szProgram,
    const libcomp::String& dataStorePath) : mStore(szProgram)
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
}

ZoneFilter::~ZoneFilter()
{
}

bool ZoneFilter::ProcessCommand(uint16_t commandCode,
    libcomp::ReadOnlyPacket& packet)
{
    if(commandCode ==
        to_underlying(ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE))
    {
        if(24 != packet.Left())
        {
            std::cerr << "Bad zone change packet found." << std::endl;

            return false;
        }

        uint32_t zoneId = packet.ReadU32Little();
        uint32_t zoneDynamicMapID = packet.ReadU32Little();
        packet.Skip(3 * sizeof(float));
        uint32_t zoneInstance = packet.ReadU32Little();
        (void)zoneInstance;

        auto it = mZones.find(zoneId);

        if(mZones.end() == it)
        {
            auto zone = std::make_shared<objects::ServerZone>();
            zone->SetID(zoneId);
            zone->SetDynamicMapID(zoneDynamicMapID);
            zone->SetGlobal(true);
            zone->SetStartingX(0.0f);
            zone->SetStartingY(0.0f);
            zone->SetStartingRotation(0.0f);

            mZones[zoneId] = zone;
            //mActiveZone = zone;
        }
        else
        {
            //mActiveZone = it->second;
        }
    }
    else if(commandCode ==
        to_underlying(ChannelToClientPacketCode_t::PACKET_NPC_DATA))
    {
        /*if(!mActiveZone)
        {
            std::cerr << "hNPC found without a zone." << std::endl;

            return false;
        }*/

        if(30 != packet.Left())
        {
            std::cerr << "Bad hNPC packet found." << std::endl;

            return false;
        }

        int32_t entityId = packet.ReadS32Little();
        uint32_t objectId = packet.ReadU32Little();
        uint32_t zoneDynamicMapID = packet.ReadU32Little();
        uint32_t zoneId = packet.ReadU32Little();
        float originX = packet.ReadFloat();
        float originY = packet.ReadFloat();
        float originRotation = packet.ReadFloat();
        (void)entityId;

        auto it = mZones.find(zoneId);

        if(mZones.end() == it)
        {
            auto zone = std::make_shared<objects::ServerZone>();
            zone->SetID(zoneId);
            zone->SetDynamicMapID(zoneDynamicMapID);
            zone->SetGlobal(true);
            zone->SetStartingX(0.0f);
            zone->SetStartingY(0.0f);
            zone->SetStartingRotation(0.0f);

            mZones[zoneId] = zone;
            //mActiveZone = zone;

            it = mZones.find(zoneId);
        }

        auto zone = it->second;

        if(zoneId != zone->GetID())
        {
            std::cerr << "hNPC does not match zone ID!" << std::endl;
            std::cerr << "Expected: " << zone->GetID() << std::endl;
            std::cerr << "Actual: " << zoneId << std::endl;

            return false;
        }

        if(zoneDynamicMapID != zone->GetDynamicMapID())
        {
            std::cerr << "hNPC does not match dynamic map ID!" << std::endl;
            std::cerr << "Expected: " << zone->GetDynamicMapID() << std::endl;
            std::cerr << "Actual: " << zoneDynamicMapID << std::endl;

            return false;
        }

        if(!HasHNPC(zone, objectId, originX, originY, originRotation))
        {
            auto obj = std::make_shared<objects::ServerNPC>();
            obj->SetID(objectId);
            obj->SetX(originX);
            obj->SetY(originY);
            obj->SetRotation(originRotation);

            zone->AppendNPCs(obj);
        }
    }
    else if(commandCode ==
        to_underlying(ChannelToClientPacketCode_t::PACKET_OBJECT_NPC_DATA))
    {
        /*if(!mActiveZone)
        {
            std::cerr << "oNPC found without a zone." << std::endl;

            return false;
        }*/

        if(29 != packet.Left())
        {
            std::cerr << "Bad oNPC packet found." << std::endl;

            return false;
        }

        int32_t entityId = packet.ReadS32Little();
        uint32_t objectId = packet.ReadU32Little();
        uint8_t state = packet.ReadU8();
        uint32_t zoneDynamicMapID = packet.ReadU32Little();
        uint32_t zoneId = packet.ReadU32Little();
        float originX = packet.ReadFloat();
        float originY = packet.ReadFloat();
        float originRotation = packet.ReadFloat();
        (void)entityId;

        auto it = mZones.find(zoneId);

        if(mZones.end() == it)
        {
            auto zone = std::make_shared<objects::ServerZone>();
            zone->SetID(zoneId);
            zone->SetDynamicMapID(zoneDynamicMapID);
            zone->SetGlobal(true);
            zone->SetStartingX(0.0f);
            zone->SetStartingY(0.0f);
            zone->SetStartingRotation(0.0f);

            mZones[zoneId] = zone;
            //mActiveZone = zone;

            it = mZones.find(zoneId);
        }

        auto zone = it->second;

        if(zoneId != zone->GetID())
        {
            std::cerr << "oNPC does not match zone ID!" << std::endl;
            std::cerr << "Expected: " << zone->GetID() << std::endl;
            std::cerr << "Actual: " << zoneId << std::endl;

            return false;
        }

        if(zoneDynamicMapID != zone->GetDynamicMapID())
        {
            std::cerr << "oNPC does not match dynamic map ID!" << std::endl;
            std::cerr << "Expected: " << zone->GetDynamicMapID() << std::endl;
            std::cerr << "Actual: " << zoneDynamicMapID << std::endl;

            return false;
        }

        if(!HasONPC(zone, objectId, originX, originY, originRotation))
        {
            auto obj = std::make_shared<objects::ServerObject>();
            obj->SetID(objectId);
            obj->SetX(originX);
            obj->SetY(originY);
            obj->SetRotation(originRotation);
            obj->SetState(state);

            zone->AppendObjects(obj);
        }
    }

    return true;
}

bool ZoneFilter::PostProcess()
{
    for(auto zonePair : mZones)
    {
        auto zone = zonePair.second;
        auto zoneDefinition = mDefinitions.GetZoneData(zone->GetID());

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
            std::cerr << "Failed to save XML file." << std::endl;

            return false;
        }
    }

    return true;
}

bool ZoneFilter::HasHNPC(const std::shared_ptr<objects::ServerZone>& zone,
    uint32_t objectId, float originX, float originY, float originRotation)
{
    for(auto obj : zone->GetNPCs())
    {
        if(obj->GetID() == objectId && obj->GetX() == originX &&
            obj->GetY() == originY && obj->GetRotation() == originRotation)
        {
            return true;
        }
    }

    return false;
}

bool ZoneFilter::HasONPC(const std::shared_ptr<objects::ServerZone>& zone,
    uint32_t objectId, float originX, float originY, float originRotation)
{
    for(auto obj : zone->GetObjects())
    {
        if(obj->GetID() == objectId && obj->GetX() == originX &&
            obj->GetY() == originY && obj->GetRotation() == originRotation)
        {
            return true;
        }
    }

    return false;
}

int main(int argc, char *argv[])
{
    if(3 != argc)
    {
        std::cerr << "USAGE: " << argv[0]
            << " DATASTORE_DIR CAPTURE_DIR" << std::endl;

        return EXIT_FAILURE;
    }

    const char *szDataStore = argv[1];
    const char *szCapturePath = argv[2];

    QCoreApplication app(argc, argv);

    QDirIterator it(szCapturePath, QStringList() << "*.hack",
        QDir::Files, QDirIterator::Subdirectories);

    /// @todo Command line option to pick one?
    CommandFilter *pFilter = new ZoneFilter(argv[0], szDataStore);

    while (it.hasNext())
    {
        CaptureFile f;

        auto capturePath = it.next();

        if(!f.Load(capturePath.toUtf8().constData()))
        {
            std::cerr << "Failed to parse capture: "
                << capturePath.toUtf8().constData() << std::endl;

            return EXIT_FAILURE;
        }

        for(auto evt : f)
        {
            for(auto cmd : *evt)
            {
                libcomp::ReadOnlyPacket p(cmd.second);

                if(!pFilter->ProcessCommand(cmd.first, p))
                {
                    return EXIT_FAILURE;
                }
            }
        }
    }

    return pFilter->PostProcess() ? EXIT_SUCCESS : EXIT_FAILURE;
}
