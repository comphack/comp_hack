/**
 * @file libcomp/src/BinaryDataSet.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage an objgen XML or binary data file.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#include "BinaryDataSet.h"

// Standard C++11 Includes
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace libcomp;

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
