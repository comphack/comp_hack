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
#include "DatabaseBind.h"
#include "Log.h"
#include "MetaObjectXmlParser.h"
#include "MetaVariable.h"

// All libcomp PersistentObject Includes
#include "Account.h"
#include "RegisteredServer.h"

using namespace libcomp;

std::unordered_map<std::string, std::weak_ptr<PersistentObject>> PersistentObject::sCached;
PersistentObject::TypeMap PersistentObject::sTypeMap;
std::unordered_map<std::type_index, std::function<PersistentObject*()>> PersistentObject::sFactory;

PersistentObject::PersistentObject() : Object(), mUUID(), mDeleted(false)
{
}

PersistentObject::PersistentObject(const PersistentObject& other) : Object(), mUUID(), mDeleted(false)
{
    (void)other;

    mSelf = std::weak_ptr<PersistentObject>();
}

PersistentObject::~PersistentObject()
{
    if(!mUUID.IsNull() && !IsDeleted())
    {
        std::string strUUID = mUUID.ToString();
        if(sCached.find(strUUID) != sCached.end())
        {
            sCached.erase(strUUID);
        }
        else
        {
            LOG_ERROR(String("Uncached UUID detected during cleanup: %1\n").Arg(
                strUUID));
        }
    }
}

libobjgen::UUID PersistentObject::GetUUID() const
{
    return mUUID;
}

bool PersistentObject::Register(const std::shared_ptr<PersistentObject>& self)
{
    if(!self->IsDeleted())
    {
        bool registered = false;

        libobjgen::UUID& uuid = self->mUUID;

        if(uuid.IsNull())
        {
            uuid = libobjgen::UUID::Random();

            registered = true;
        }
        else if(sCached.find(uuid.ToString()) == sCached.end())
        {
            registered = true;
        }

        std::string uuidString = uuid.ToString();

        if(registered)
        {
            self->mSelf = self;
            sCached[uuidString] = self;

            return true;
        }
        else
        {
            LOG_ERROR(String("Duplicate object detected: %1\n").Arg(
                uuidString));
        }
    }

    return false;
}

void PersistentObject::Unregister()
{
    mDeleted = true;
    auto iter = sCached.find(mUUID.ToString());
    if(iter != sCached.end())
    {
        sCached.erase(iter);
    }
}

bool PersistentObject::IsDeleted()
{
    return mDeleted;
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
    const std::shared_ptr<Database>& db,  const libobjgen::UUID& uuid)
{
    auto obj = GetObjectByUUID(uuid);

    if(nullptr == obj)
    {
        std::list<DatabaseBind*> bindings;

        auto bind = new DatabaseBindUUID("uid", uuid);
        bindings.push_back(bind);

        obj = LoadObject(type, db, bindings);

        delete bind;

        if(nullptr == obj)
        {
            LOG_ERROR(String("Unknown UUID '%1' for '%2' failed to load")
                .Arg(uuid.ToString()).Arg(sTypeMap[type]->GetName()));
        }
    }

    return obj;
}

std::shared_ptr<PersistentObject> PersistentObject::LoadObject(
    std::type_index type, const std::shared_ptr<Database>& db,
    const std::list<DatabaseBind*>& pValues)
{
    std::shared_ptr<PersistentObject> obj;

    if(nullptr != db)
    {
        obj = db->LoadSingleObject(type, pValues);
    }

    return obj;
}

std::list<std::shared_ptr<PersistentObject>> PersistentObject::LoadObjects(
    std::type_index type, const std::shared_ptr<Database>& db,
    const std::list<DatabaseBind*>& pValues)
{
    if(nullptr != db)
    {
        return db->LoadObjects(type, pValues);
    }

    return std::list<std::shared_ptr<PersistentObject>>();
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

std::shared_ptr<libobjgen::MetaObject> PersistentObject::GetMetadataFromBytes(const char* bytes,
    size_t length)
{
    if(length == 0)
    {
        return nullptr;
    }

    std::string definition(bytes, length);
    std::stringstream ss(definition);

    auto obj = std::shared_ptr<libobjgen::MetaObject>(new libobjgen::MetaObject());
    if(!obj->Load(ss))
    {
        obj = nullptr;
    }

    return obj;
}

std::shared_ptr<PersistentObject> PersistentObject::New(std::type_index type)
{
    auto iter = sFactory.find(type);
    return iter != sFactory.end() ? std::shared_ptr<PersistentObject>(iter->second()) : nullptr;
}

bool PersistentObject::Insert(const std::shared_ptr<Database>& db)
{
    if(mSelf.use_count() > 0)
    {
        auto self = mSelf.lock();
        return nullptr != db ? db->InsertSingleObject(self) : false;
    }

    return false;
}

bool PersistentObject::Update(const std::shared_ptr<Database>& db)
{
    if(mSelf.use_count() > 0)
    {
        auto self = mSelf.lock();
        return nullptr != db ? db->UpdateSingleObject(self) : false;
    }

    return false;
}

bool PersistentObject::Delete(const std::shared_ptr<Database>& db)
{
    if(mSelf.use_count() > 0)
    {
        auto self = mSelf.lock();
        return nullptr == db || db->DeleteSingleObject(self);
    }

    return false;
}

bool PersistentObject::sInitializationFailed = false;
bool PersistentObject::Initialize()
{
    RegisterType(typeid(objects::Account), objects::Account::GetMetadata(),
        []() {  return (PersistentObject*)new objects::Account(); });
    RegisterType(typeid(objects::RegisteredServer), objects::RegisteredServer::GetMetadata(),
        []() {  return (PersistentObject*)new objects::RegisteredServer(); });

    return !sInitializationFailed;
}
