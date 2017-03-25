/**
 * @file tools/bdpatch/src/main.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to read and write BinaryData files.
 *
 * This tool will read and write BinaryData files.
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

// Standard C++11 Includes
#include <fstream>
#include <iostream>
#include <map>

// libcomp Includes
#include <Object.h>

// object Includes
#include <MiCItemBaseData.h>
#include <MiCItemData.h>

#include <MiDevilData.h>
#include <MiNPCBasicData.h>

#include <MiZoneData.h>
#include <MiZoneBasicData.h>

#include <MiHNPCData.h>
#include <MiHNPCBasicData.h>

#include <MiONPCData.h>

// tinyxml2 Includes
#include <PushIgnore.h>
#include <tinyxml2.h>
#include <PopIgnore.h>

class BinaryDataSet
{
public:
    BinaryDataSet(std::function<std::shared_ptr<libcomp::Object>()> allocator,
        std::function<uint32_t(const std::shared_ptr<
            libcomp::Object>&)> mapper);

    bool Load(std::istream& file);
    bool Save(std::ostream& file) const;

    bool LoadXml(tinyxml2::XMLDocument& doc);

    std::string GetXml() const;

    std::list<std::shared_ptr<libcomp::Object>> GetObjects() const;
    std::shared_ptr<libcomp::Object> GetObjectByID(uint32_t id) const;

private:
    std::function<std::shared_ptr<libcomp::Object>()> mObjectAllocator;
    std::function<uint32_t(const std::shared_ptr<
        libcomp::Object>&)> mObjectMapper;

    std::list<std::shared_ptr<libcomp::Object>> mObjects;
    std::map<uint32_t, std::shared_ptr<libcomp::Object>> mObjectMap;
};

BinaryDataSet::BinaryDataSet(std::function<std::shared_ptr<
    libcomp::Object>()> allocator, std::function<uint32_t(
    const std::shared_ptr<libcomp::Object>&)> mapper) :
    mObjectAllocator(allocator), mObjectMapper(mapper)
{
}

bool BinaryDataSet::Load(std::istream& file)
{
    mObjects = libcomp::Object::LoadBinaryData(file,
        mObjectAllocator);

    for(auto obj : mObjects)
    {
        mObjectMap[mObjectMapper(obj)] = obj;
    }

    return !mObjects.empty();
}

bool BinaryDataSet::Save(std::ostream& file) const
{
    return libcomp::Object::SaveBinaryData(file, mObjects);
}

bool BinaryDataSet::LoadXml(tinyxml2::XMLDocument& doc)
{
    std::list<std::shared_ptr<libcomp::Object>> objs;

    auto pRoot = doc.RootElement();

    if(nullptr == pRoot)
    {
        return false;
    }

    auto objElement = pRoot->FirstChildElement("object");

    while(nullptr != objElement)
    {
        auto obj = mObjectAllocator();

        if(!obj->Load(doc, *objElement))
        {
            return false;
        }

        objs.push_back(obj);

        objElement = objElement->NextSiblingElement("object");
    }

    mObjects = objs;
    mObjectMap.clear();

    for(auto obj : mObjects)
    {
        mObjectMap[mObjectMapper(obj)] = obj;
    }

    return !mObjects.empty();
}

std::string BinaryDataSet::GetXml() const
{
    tinyxml2::XMLDocument doc;

    tinyxml2::XMLElement *pRoot = doc.NewElement("objects");
    doc.InsertEndChild(pRoot);

    for(auto obj : mObjects)
    {
        if(!obj->Save(doc, *pRoot))
        {
            return {};
        }
    }

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    return printer.CStr();
}

std::list<std::shared_ptr<libcomp::Object>> BinaryDataSet::GetObjects() const
{
    return mObjects;
}

std::shared_ptr<libcomp::Object> BinaryDataSet::GetObjectByID(
    uint32_t id) const
{
    auto it = mObjectMap.find(id);

    if(mObjectMap.end() != it)
    {
        return it->second;
    }

    return {};
}

int Usage(const char *szAppName)
{
    std::cerr << "USAGE: " << szAppName << " load TYPE IN OUT" << std::endl;
    std::cerr << "USAGE: " << szAppName << " save TYPE IN OUT" << std::endl;
    std::cerr << std::endl;
    std::cerr << "TYPE indicates the format of the BinaryData and can "
        << "be one of:" << std::endl;
    std::cerr << "  citem Format for CItemData.sbin" << std::endl;
    std::cerr << "  devil Format for DevilData.sbin" << std::endl;
    std::cerr << "  hnpc Format for hNPCData.sbin" << std::endl;
    std::cerr << "  onpc Format for oNPCData.sbin" << std::endl;
    std::cerr << "  zone  Format for ZoneData.sbin" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Mode 'load' will take the input BinaryData file and "
        << "write the output XML file." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Mode 'save' will take the input XML file and "
        << "write the output BinaryData file." << std::endl;

    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    if(5 != argc)
    {
        return Usage(argv[0]);
    }

    libcomp::String mode = argv[1];
    libcomp::String bdType = argv[2];
    const char *szInPath = argv[3];
    const char *szOutPath = argv[4];

    if("load" != mode && "save" != mode)
    {
        return Usage(argv[0]);
    }

    BinaryDataSet *pSet = nullptr;

    if("citem" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiCItemData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiCItemData>(
                    obj)->GetBaseData()->GetID();
            }
        );
    }
    else if("devil" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiDevilData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiDevilData>(
                    obj)->GetBasic()->GetID();
            }
        );
    }
    else if("hnpc" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiHNPCData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiHNPCData>(
                    obj)->GetBasic()->GetID();
            }
        );
    }
    else if("onpc" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiONPCData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiONPCData>(
                    obj)->GetID();
            }
        );
    }
    else if("zone" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiZoneData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiZoneData>(
                    obj)->GetBasic()->GetID();
            }
        );
    }
    else
    {
        return Usage(argv[0]);
    }

    if("load" == mode)
    {
        std::ifstream file;
        file.open(szInPath, std::ifstream::binary);

        if(!pSet->Load(file))
        {
            std::cerr << "Failed to load file: " << szInPath << std::endl;

            return EXIT_FAILURE;
        }

        std::ofstream out;
        out.open(szOutPath);

        out << pSet->GetXml().c_str();

        if(!out.good())
        {
            std::cerr << "Failed to save file: " << szOutPath << std::endl;

            return EXIT_FAILURE;
        }
    }
    else if("save" == mode)
    {
        tinyxml2::XMLDocument doc;

        if(tinyxml2::XML_NO_ERROR != doc.LoadFile(szInPath))
        {
            std::cerr << "Failed to parse file: " << szInPath << std::endl;

            return EXIT_FAILURE;
        }

        if(!pSet->LoadXml(doc))
        {
            std::cerr << "Failed to load file: " << szInPath << std::endl;

            return EXIT_FAILURE;
        }

        std::ofstream out;
        out.open(szOutPath, std::ofstream::binary);

        if(!pSet->Save(out))
        {
            std::cerr << "Failed to save file: " << szOutPath << std::endl;

            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
