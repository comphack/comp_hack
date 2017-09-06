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
#include <MiCEventMessageData.h>

#include <MiCItemBaseData.h>
#include <MiCItemData.h>

#include <MiCMessageData.h>

#include <MiCMultiTalkData.h>
#include <MiMultiTalkCmdTbl.h>

#include <MiCPolygonMovieData.h>
#include <MiPMBaseInfo.h>
#include <MiPMCameraKeyTbl.h>
#include <MiPMMsgKeyTbl.h>
#include <MiPMBGMKeyTbl.h>
#include <MiPMSEKeyTbl.h>
#include <MiPMEffectKeyTbl.h>
#include <MiPMFadeKeyTbl.h>
#include <MiPMGouraudKeyTbl.h>
#include <MiPMFogKeyTbl.h>
#include <MiPMScalingHelperTbl.h>
#include <MiPMAttachCharacterTbl.h>
#include <MiPMMotionKeyTbl.h>

#include <MiCTalkMessageData.h>

#include <MiCZoneRelationData.h>

#include <MiCQuestData.h>
#include <MiNextEpisodeInfo.h>

#include <MiDevilData.h>
#include <MiNPCBasicData.h>

#include <MiDynamicMapData.h>

#include <MiHNPCData.h>
#include <MiHNPCBasicData.h>

#include <MiONPCData.h>

#include <MiSpotData.h>

#include <MiZoneData.h>
#include <MiZoneBasicData.h>

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
    std::string GetTabular() const;

    std::list<std::shared_ptr<libcomp::Object>> GetObjects() const;
    std::shared_ptr<libcomp::Object> GetObjectByID(uint32_t id) const;

private:
    std::list<std::string> ReadNodes(tinyxml2::XMLElement *node,
        int16_t dataMode) const;

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

std::list<std::string> BinaryDataSet::ReadNodes(tinyxml2::XMLElement *node,
    int16_t dataMode) const
{
    std::list<std::string> data;
    std::list<tinyxml2::XMLElement*> nodes;
    while(node != nullptr)
    {
        if(node->FirstChildElement() != nullptr)
        {
            nodes.push_back(node);
            node = node->FirstChildElement();

            //Doesn't handle map
            if(std::string(node->Name()) == "element")
            {
                if(dataMode > 1)
                {
                    std::list<std::string> nData;
                    while(node != nullptr)
                    {
                        std::list<std::string> sData;
                        if(node->FirstChildElement() != nullptr)
                        {
                            sData = ReadNodes(node->FirstChildElement(), 3);
                        }
                        else
                        {
                            const char* txt = node->GetText();
                            auto str = txt != 0 ? std::string(txt) : std::string();
                            sData.push_back(str);
                        }

                        std::stringstream ss;
                        ss << "{ ";
                        bool first = true;
                        for(auto d : sData)
                        {
                            if(!first) ss << ", ";
                            ss << d;
                            first = false;
                        }
                        ss << " }";
                        nData.push_back(ss.str());

                        node = node->NextSiblingElement();
                    }

                    std::stringstream ss;
                    bool first = true;
                    for(auto d : nData)
                    {
                        if(!first) ss << ", ";
                        ss << d;
                        first = false;
                    }
                    data.push_back(ss.str());
                }
                else
                {
                    //Pull the name and skip
                    data.push_back(std::string(nodes.back()->Attribute("name")));
                    node = nullptr;
                }
            }
        }
        else
        {
            const char* txt = node->GetText();
            auto name = std::string(node->Attribute("name"));
            auto val = txt != 0 ? std::string(txt) : std::string();
            val.erase(std::remove(val.begin(), val.end(), '\n'), val.end());
            switch(dataMode)
            {
                case 1:
                    data.push_back(name);
                    break;
                case 2:
                    data.push_back(val);
                    break;
                default:
                    data.push_back(name + ": " + val);
                    break;
            }

            node = node->NextSiblingElement();
        }

        while(node == nullptr && nodes.size() > 0)
        {
            node = nodes.back();
            nodes.pop_back();
            node = node->NextSiblingElement();
        }
    }

    return data;
}

std::string BinaryDataSet::GetTabular() const
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

    std::stringstream ss;

    tinyxml2::XMLNode *pNode = doc.FirstChild()->FirstChild();

    if(pNode != nullptr)
    {
        for(auto d : ReadNodes(pNode->FirstChildElement(), 1))
        {
            ss << d << "\t";
        }

        ss << std::endl;
    }

    while(pNode != nullptr)
    {
        std::list<tinyxml2::XMLElement*> nodes;

        tinyxml2::XMLElement *cNode = pNode->FirstChildElement();
        auto data = ReadNodes(cNode, 2);
        for(auto d : data)
        {
            ss << d << "\t";
        }

        ss << std::endl;
        pNode = pNode->NextSiblingElement();
    }

    return ss.str();
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
    std::cerr << "USAGE: " << szAppName << " flatten TYPE IN OUT" << std::endl;
    std::cerr << std::endl;
    std::cerr << "TYPE indicates the format of the BinaryData and can "
        << "be one of:" << std::endl;
    std::cerr << "  ceventmessage Format for CEventMessageData.sbin" << std::endl;
    std::cerr << "  citem         Format for CItemData.sbin" << std::endl;
    std::cerr << "  cmultitalk    Format for CMultiTalkData.sbin" << std::endl;
    std::cerr << "  cmessage      Format for CMessageData.sbin" << std::endl;
    std::cerr << "  cpolygonmovie Format for CPolygonMoveData.sbin" << std::endl;
    std::cerr << "  cquest        Format for CQuestData.sbin" << std::endl;
    std::cerr << "  ctalkmessage  Format for CTalkMessageData.sbin" << std::endl;
    std::cerr << "  czonerelation Format for CZoneRelationData.sbin" << std::endl;
    std::cerr << "  devil         Format for DevilData.sbin" << std::endl;
    std::cerr << "  dynamicmap    Format for DynamicMapData.bin" << std::endl;
    std::cerr << "  hnpc          Format for hNPCData.sbin" << std::endl;
    std::cerr << "  onpc          Format for oNPCData.sbin" << std::endl;
    std::cerr << "  spot          Format for SpotData.bin" << std::endl;
    std::cerr << "  zone          Format for ZoneData.sbin" << std::endl;
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

    if("load" != mode && "save" != mode && "flatten" != mode)
    {
        return Usage(argv[0]);
    }

    BinaryDataSet *pSet = nullptr;

    if("ceventmessage" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiCEventMessageData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiCEventMessageData>(
                    obj)->GetID();
            }
        );
    }
    else if("citem" == bdType)
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
    else if("cmultitalk" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiCMultiTalkData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiCMultiTalkData>(
                    obj)->GetID();
            }
        );
    }
    else if("cmessage" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiCMessageData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiCMessageData>(
                    obj)->GetID();
            }
        );
    }
    else if("cpolygonmovie" == bdType)
    {
        static uint32_t nextID = 0;

        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiCPolygonMovieData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                (void)obj;

                return nextID++;
            }
        );
    }
    else if("cquest" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiCQuestData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiCQuestData>(
                    obj)->GetID();
            }
        );
    }
    else if("ctalkmessage" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiCTalkMessageData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiCTalkMessageData>(
                    obj)->GetID();
            }
        );
    }
    else if("czonerelation" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiCZoneRelationData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiCZoneRelationData>(
                    obj)->GetID();
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
    else if("dynamicmap" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiDynamicMapData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiDynamicMapData>(
                    obj)->GetID();
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
    else if("spot" == bdType)
    {
        pSet = new BinaryDataSet(
            []()
            {
                return std::make_shared<objects::MiSpotData>();
            },

            [](const std::shared_ptr<libcomp::Object>& obj)
            {
                return std::dynamic_pointer_cast<objects::MiSpotData>(
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
    else if("flatten" == mode)
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

        out << pSet->GetTabular().c_str();

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
