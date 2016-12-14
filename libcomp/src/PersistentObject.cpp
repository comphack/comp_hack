/**
 * @file libcomp/src/PersistentObject.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Derived class from Object and base class for peristed objgen objects.
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

#include "PersistentObject.h"

// libcomp Includes
#include "Database.h"
#include "Log.h"
#include "MetaVariable.h"

// All libcomp PersistentObject Includes
#include "Account.h"

using namespace libcomp;

std::unordered_map<std::string, std::weak_ptr<PersistentObject>> PersistentObject::sCached;
PersistentObject::TypeMap PersistentObject::sTypeMap;
std::unordered_map<std::type_index, std::function<PersistentObject*()>> PersistentObject::sFactory;

PersistentObject::PersistentObject() : Object()
{
}

PersistentObject::PersistentObject(const PersistentObject& other) : Object()
{
    mUUID = libobjgen::UUID();
    mSelf = std::weak_ptr<PersistentObject>();
}

bool PersistentObject::Register(std::shared_ptr<PersistentObject>& self, const libobjgen::UUID& uuid)
{
    if(mUUID.IsNull())
    {
        bool registered = false;
        if(uuid.IsNull())
        {
            mUUID = libobjgen::UUID::Random();
            registered = true;
        }
        else if(sCached.find(uuid.ToString()) == sCached.end())
        {
            mUUID = uuid;
            registered = true;
        }

        if(registered)
        {
            mSelf = self;
            sCached[mUUID.ToString()] = mSelf;
        }
        else
        {
            /// @todo: update the cache?
        }
    }

    return false;
}

PersistentObject::~PersistentObject()
{
    if(!mUUID.IsNull())
    {
        std::string strUUID = mUUID.ToString();
        if(sCached.find(strUUID) != sCached.end())
        {
            sCached.erase(strUUID);
        }
        else
        {
            LOG_ERROR(libcomp::String("Uncached UUID detected during cleanup: %1").Arg(strUUID));
        }
    }
}

std::shared_ptr<PersistentObject> PersistentObject::GetObjectByUUID(const libobjgen::UUID& uuid)
{
    auto iter = sCached.find(uuid.ToString());
    if(iter != sCached.end())
    {
        return iter->second.lock();
    }

    return nullptr;
}

std::shared_ptr<PersistentObject> PersistentObject::LoadObjectByUUID(std::type_index type,
    const libobjgen::UUID& uuid)
{
    auto obj = GetObjectByUUID(uuid);

    if(nullptr == obj)
    {
        obj = LoadObject(type, "UID", uuid.ToString());

        if(nullptr == obj)
        {
            LOG_ERROR(libcomp::String("Unknown UUID '%1' for '%2' failed to load")
                .Arg(uuid.ToString()).Arg(sTypeMap[type]->GetName()));
        }
    }

    return obj;
}

std::shared_ptr<PersistentObject> PersistentObject::LoadObject(std::type_index type,
    const std::string& fieldName, const std::string& value)
{
    return Database::GetMainDatabase()->LoadSingleObject(type, fieldName, value);
}

void PersistentObject::RegisterType(std::type_index type,
    const std::shared_ptr<libobjgen::MetaObject>& obj,
    const std::function<PersistentObject*()>& f)
{
    sTypeMap[type] = obj;
    sFactory[type] = f;
}

PersistentObject::TypeMap& PersistentObject::GetRegistry()
{
    return sTypeMap;
}

const std::shared_ptr<libobjgen::MetaObject> PersistentObject::GetRegisteredMetadata(std::type_index type)
{
    auto iter = sTypeMap.find(type);
    return iter != sTypeMap.end() ? iter->second : nullptr;
}

std::shared_ptr<libobjgen::MetaObject> PersistentObject::GetMetadataFromXml(const std::string& xml)
{
    auto obj = std::shared_ptr<libobjgen::MetaObject>(new libobjgen::MetaObject());

    tinyxml2::XMLDocument doc;
    auto err = doc.Parse(xml.c_str(), xml.length());
    if(err == tinyxml2::XML_NO_ERROR)
    {
        if(!obj->Load(doc, *doc.FirstChildElement()))
        {
            //Should never happen to generated objects
            obj = nullptr;
        }
    }
    
    return obj;
}

std::shared_ptr<PersistentObject> PersistentObject::New(std::type_index type)
{
    auto iter = sFactory.find(type);
    return iter != sFactory.end() ? std::shared_ptr<PersistentObject>(iter->second()) : nullptr;
}

void PersistentObject::Initialize()
{
    RegisterType(typeid(objects::Account), objects::Account::GetMetadata(),
        []() {  return (PersistentObject*)new objects::Account(); });
}
