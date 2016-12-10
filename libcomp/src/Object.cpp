/**
 * @file libcomp/src/Object.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class for an object generated by the object generator (objgen).
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#include "Object.h"

// libcomp Includes
#include <Packet.h>
#include <PacketStream.h>
#include <ReadOnlyPacket.h>

using namespace libcomp;

Object::Object() : mUUID()
{
}

Object::~Object()
{
}

bool Object::LoadPacket(libcomp::ReadOnlyPacket& p)
{
    ReadOnlyPacketStream buffer(p);
    std::istream in(&buffer);

    //Skip what we've already read
    auto skip = p.Tell();
    if(skip > 0)
    {
        in.ignore(skip);
    }

    return Load(in, true);
}

bool Object::SavePacket(libcomp::Packet& p) const
{
    PacketStream buffer(p);
    std::ostream out(&buffer);

    return Save(out, true);
}

const tinyxml2::XMLElement*
    Object::GetXmlChild(const tinyxml2::XMLElement& root, const std::string name) const
{
    auto children = GetXmlChildren(root, name);
    return children.size() > 0 ? children[0] : nullptr;
}

const std::vector<const tinyxml2::XMLElement*>
    Object::GetXmlChildren(const tinyxml2::XMLElement& root, const std::string name) const
{
    std::vector<const tinyxml2::XMLElement*> retval;

    const tinyxml2::XMLElement *cMember = root.FirstChildElement();
    while(nullptr != cMember)
    {
        if(name == cMember->Name())
        {
            retval.push_back(cMember);
        }

        cMember = cMember->NextSiblingElement();
    }
    return retval;
}

std::unordered_map<std::string, const tinyxml2::XMLElement*>
    Object::GetXmlMembers(const tinyxml2::XMLElement& root) const
{
    std::unordered_map<std::string, const tinyxml2::XMLElement*> members;

    const tinyxml2::XMLElement *pMember = root.FirstChildElement("member");

    while(nullptr != pMember)
    {
        const char *szName = pMember->Attribute("name");

        if(nullptr != szName)
        {
            members[szName] = pMember;
        }

        pMember = pMember->NextSiblingElement("member");
    }

    return members;
}

std::string Object::GetXmlText(const tinyxml2::XMLElement& root) const
{
    std::string value;

    const tinyxml2::XMLNode *pChild = root.FirstChild();

    if(nullptr != pChild)
    {
        const tinyxml2::XMLText *pText = pChild->ToText();

        if(nullptr != pText)
        {
            const char *szText = pText->Value();

            if(nullptr != szText)
            {
                value = szText;
            }
        }
    }

    return value;
}

std::list<std::shared_ptr<Object>> Object::LoadBinaryData(
    std::istream& stream,
    const std::function<std::shared_ptr<Object>()>& objectAllocator)
{
    std::list<std::shared_ptr<Object>> objects;

    uint16_t objectCount;
    uint16_t dynamicSizeCount;

    stream.read(reinterpret_cast<char*>(&objectCount),
        sizeof(objectCount));

    if(!stream.good())
    {
        return {};
    }

    stream.read(reinterpret_cast<char*>(&dynamicSizeCount),
        sizeof(dynamicSizeCount));


    if(!stream.good())
    {
        return {};
    }

    ObjectInStream objectStream(stream);

    for(uint16_t i = 0; i < objectCount; ++i)
    {
        for(uint16_t j = 0; j < dynamicSizeCount; ++j)
        {
            uint16_t dyanmicSize;

            stream.read(reinterpret_cast<char*>(&dyanmicSize),
                sizeof(dyanmicSize));

            if(!stream.good())
            {
                return {};
            }

            objectStream.dynamicSizes.push_back(dyanmicSize);
        }
    }

    for(uint16_t i = 0; i < objectCount; ++i)
    {
        std::shared_ptr<Object> obj(objectAllocator());

        if(!obj->Load(objectStream) || !stream.good())
        {
            return {};
        }

        objects.push_back(obj);
    }

    return objects;
}

libobjgen::UUID Object::GetUUID() const
{
    return mUUID;
}
