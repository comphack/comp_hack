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
#include "AccountWorldData.h"
#include "Character.h"
#include "CharacterProgress.h"
#include "Demon.h"
#include "EntityStats.h"
#include "Expertise.h"
#include "Hotbar.h"
#include "InheritedSkill.h"
#include "Item.h"
#include "ItemBox.h"
#include "Quest.h"
#include "RegisteredChannel.h"
#include "RegisteredWorld.h"
#include "StatusEffect.h"

using namespace libcomp;

std::unordered_map<std::string, std::weak_ptr<PersistentObject>> PersistentObject::sCached;
std::mutex PersistentObject::mCacheLock;
PersistentObject::TypeMap PersistentObject::sTypeMap;
std::unordered_map<std::string, size_t> PersistentObject::sTypeNames;
std::unordered_map<size_t, std::function<PersistentObject*()>> PersistentObject::sFactory;

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
        std::lock_guard<std::mutex> lock(mCacheLock);

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

bool PersistentObject::Register(const std::shared_ptr<PersistentObject>& self,
    const libobjgen::UUID& pUuid)
{
    if(!self->IsDeleted())
    {
        bool registered = false;

        libobjgen::UUID& uuid = self->mUUID;

        std::lock_guard<std::mutex> lock(mCacheLock);
        if(uuid.IsNull())
        {
            if(!pUuid.IsNull())
            {
                uuid = pUuid;
            }
            else
            {
                uuid = libobjgen::UUID::Random();
                registered = true;
            }
        }
        
        if(!registered && sCached.find(uuid.ToString()) == sCached.end())
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

    std::lock_guard<std::mutex> lock(mCacheLock);

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
    std::lock_guard<std::mutex> lock(mCacheLock);

    auto iter = sCached.find(uuid.ToString());
    if(iter != sCached.end())
    {
        return iter->second.lock();
    }

    return nullptr;
}

std::shared_ptr<PersistentObject> PersistentObject::LoadObjectByUUID(size_t typeHash,
    const std::shared_ptr<Database>& db,  const libobjgen::UUID& uuid, bool reload)
{
    auto obj = !reload ? GetObjectByUUID(uuid) : nullptr;

    if(nullptr == obj)
    {
        auto bind = new DatabaseBindUUID("UID", uuid);

        obj = LoadObject(typeHash, db, bind);

        delete bind;

        if(nullptr == obj)
        {
            LOG_ERROR(String("Unknown UUID '%1' for '%2' failed to load\n")
                .Arg(uuid.ToString()).Arg(sTypeMap[typeHash]->GetName()));
        }
    }

    return obj;
}

std::shared_ptr<PersistentObject> PersistentObject::LoadObject(
    size_t typeHash, const std::shared_ptr<Database>& db,
    DatabaseBind *pValue)
{
    std::shared_ptr<PersistentObject> obj;

    if(nullptr != db)
    {
        obj = db->LoadSingleObject(typeHash, pValue);
    }

    return obj;
}

std::list<std::shared_ptr<PersistentObject>> PersistentObject::LoadObjects(
    size_t typeHash, const std::shared_ptr<Database>& db,
    DatabaseBind *pValue)
{
    if(nullptr != db)
    {
        return db->LoadObjects(typeHash, pValue);
    }

    return std::list<std::shared_ptr<PersistentObject>>();
}

void PersistentObject::RegisterType(std::type_index type,
    const std::shared_ptr<libobjgen::MetaObject>& obj,
    const std::function<PersistentObject*()>& f)
{
    size_t typeHash = type.hash_code();

    sTypeMap[typeHash] = obj;
    sTypeNames[obj->GetName()] = typeHash;
    sFactory[typeHash] = f;
}

PersistentObject::TypeMap& PersistentObject::GetRegistry()
{
    return sTypeMap;
}

size_t PersistentObject::GetTypeHashByName(std::string name, bool& result)
{
    auto iter = sTypeNames.find(name);
    result = iter != sTypeNames.end();
    return result ? iter->second : 0;
}

const std::shared_ptr<libobjgen::MetaObject> PersistentObject::GetRegisteredMetadata(size_t typeHash)
{
    auto iter = sTypeMap.find(typeHash);
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

std::shared_ptr<PersistentObject> PersistentObject::New(size_t typeHash)
{
    auto iter = sFactory.find(typeHash);
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

    RegisterType(typeid(objects::AccountWorldData), objects::AccountWorldData::GetMetadata(),
        []() {  return (PersistentObject*)new objects::AccountWorldData(); });

    RegisterType(typeid(objects::Character), objects::Character::GetMetadata(),
        []() {  return (PersistentObject*)new objects::Character(); });

    RegisterType(typeid(objects::CharacterProgress), objects::CharacterProgress::GetMetadata(),
        []() {  return (PersistentObject*)new objects::CharacterProgress(); });

    RegisterType(typeid(objects::Demon), objects::Demon::GetMetadata(),
        []() {  return (PersistentObject*)new objects::Demon(); });

    RegisterType(typeid(objects::EntityStats), objects::EntityStats::GetMetadata(),
        []() {  return (PersistentObject*)new objects::EntityStats(); });

    RegisterType(typeid(objects::Expertise), objects::Expertise::GetMetadata(),
        []() {  return (PersistentObject*)new objects::Expertise(); });

    RegisterType(typeid(objects::Hotbar), objects::Hotbar::GetMetadata(),
        []() {  return (PersistentObject*)new objects::Hotbar(); });

    RegisterType(typeid(objects::InheritedSkill), objects::InheritedSkill::GetMetadata(),
        []() {  return (PersistentObject*)new objects::InheritedSkill(); });

    RegisterType(typeid(objects::Item), objects::Item::GetMetadata(),
        []() {  return (PersistentObject*)new objects::Item(); });

    RegisterType(typeid(objects::ItemBox), objects::ItemBox::GetMetadata(),
        []() {  return (PersistentObject*)new objects::ItemBox(); });

    RegisterType(typeid(objects::Quest), objects::Quest::GetMetadata(),
        []() {  return (PersistentObject*)new objects::Quest(); });

    RegisterType(typeid(objects::RegisteredChannel), objects::RegisteredChannel::GetMetadata(),
        []() {  return (PersistentObject*)new objects::RegisteredChannel(); });

    RegisterType(typeid(objects::RegisteredWorld), objects::RegisteredWorld::GetMetadata(),
        []() {  return (PersistentObject*)new objects::RegisteredWorld(); });

    RegisterType(typeid(objects::StatusEffect), objects::StatusEffect::GetMetadata(),
        []() {  return (PersistentObject*)new objects::StatusEffect(); });

    return !sInitializationFailed;
}
