/**
 * @file libcomp/src/DataSyncManager.cpp
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Manages synchronizing data between two or more servers.
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

#include "DataSyncManager.h"

// libcomp Includes
#include "Log.h"
#include "Packet.h"
#include "PacketCodes.h"
#include "PersistentObject.h"
#include "ScriptEngine.h"

using namespace libcomp;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<DataSyncManager>()
    {
        if(!BindingExists("DataSyncManager", true))
        {
            Sqrat::Class<DataSyncManager,
                Sqrat::NoConstructor<DataSyncManager>> binding(mVM, "DataSyncManager");
            binding
                .Func("UpdateRecord", &DataSyncManager::UpdateRecord)
                .Func("RemoveRecord", &DataSyncManager::RemoveRecord)
                .Func("SyncOutgoing", &DataSyncManager::SyncOutgoing);

            Bind<DataSyncManager>("DataSyncManager", binding);
        }

        return *this;
    }
}

DataSyncManager::DataSyncManager()
{
}

DataSyncManager::~DataSyncManager()
{
}

bool DataSyncManager::RegisterConnection(const std::shared_ptr<
    InternalConnection>& connection, std::set<std::string> types)
{
    std::lock_guard<std::mutex> lock(mLock);

    // Check to see if the connection is already here
    for(auto pair : mConnections)
    {
        if(pair.first == connection)
        {
            return false;
        }
    }

    // The connection does not already exist, add it with the
    // requested types
    mConnections[connection] = types;

    return true;
}

bool DataSyncManager::RemoveConnection(const std::shared_ptr<
    InternalConnection>& connection)
{
    std::lock_guard<std::mutex> lock(mLock);
    for(auto pair : mConnections)
    {
        if(pair.first == connection)
        {
            mConnections.erase(pair.first);
            return true;
        }
    }

    return false;
}

void DataSyncManager::SyncOutgoing()
{
    std::lock_guard<std::mutex> lock(mLock);

    if(mOutboundRemoves.size() == 0 && mOutboundUpdates.size() == 0)
    {
        // Nothing to do
        return;
    }

    for(auto pair : mConnections)
    {
        // Send one packet per connection per object
        std::set<std::string> updateTypes;
        std::set<std::string> removeTypes;
        std::set<std::string> allTypes;

        for(std::string type : pair.second)
        {
            if(mOutboundUpdates.find(type) != mOutboundUpdates.end())
            {
                updateTypes.insert(type);
                allTypes.insert(type);
            }

            if(mOutboundRemoves.find(type) != mOutboundRemoves.end())
            {
                removeTypes.insert(type);
                allTypes.insert(type);
            }
        }

        if(allTypes.size() == 0) continue;

        for(std::string type : allTypes)
        {
            String lType(type);

            QueueOutgoing(lType, pair.first, mOutboundUpdates[type],
                mOutboundRemoves[type]);
        }

        pair.first->FlushOutgoing();
    }

    mOutboundUpdates.clear();
    mOutboundRemoves.clear();
}

bool DataSyncManager::SyncIncoming(libcomp::ReadOnlyPacket& p,
    const libcomp::String& source)
{
    if(p.Left() < 6)
    {
        return false;
    }

    std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>> completed;

    std::shared_ptr<libcomp::DataSyncManager::ObjectConfig> config;
    {
        std::lock_guard<std::mutex> lock(mLock);

        String lType(p.ReadString16Little(libcomp::Convert::ENCODING_UTF8,
            true));

        std::string type(lType.C());

        auto configIter = mRegisteredTypes.find(type);
        if(configIter == mRegisteredTypes.end())
        {
            LOG_WARNING(libcomp::String("Ignoring sync request for"
                " unregistered type: %1\n").Arg(lType));
            return true;
        }

        config = configIter->second;

        bool isPersistent = false;
        auto typeHash = PersistentObject::GetTypeHashByName(type,
            isPersistent);
    
        if(!isPersistent)
        {
            if(!config->BuildHandler)
            {
                LOG_ERROR(libcomp::String("Non-persistent object type without"
                    " a registered build handler encountered: %1\n")
                    .Arg(type));
                return false;
            }
            else if(!config->UpdateHandler && !config->SyncCompleteHandler)
            {
                LOG_ERROR(libcomp::String("Object type without a registered"
                    " update or sync complete handler encountered: %1\n")
                    .Arg(type));
                return false;
            }
        }

        if(p.Left() < 4)
        {
            return false;
        }

        // Read updates
        uint16_t recordsCount = p.ReadU16Little();

        std::list<std::shared_ptr<libcomp::Object>> records;
        if(isPersistent)
        {
            // Load by UUID
            for(uint16_t k = 0; k < recordsCount; k++)
            {
                String uidStr(p.ReadString16Little(
                    libcomp::Convert::ENCODING_UTF8, true));

                libobjgen::UUID uid(uidStr.C());
                if(!uid.IsNull())
                {
                    auto obj = PersistentObject::LoadObjectByUUID(typeHash,
                        mRegisteredTypes[type]->DB, uid, true);
                    if(obj)
                    {
                        records.push_back(obj);
                    }
                }
                else
                {
                    // Skip null UIDs
                    LOG_ERROR(libcomp::String("Null UID encountered for"
                        " updated sync record of type: %1\n").Arg(type));
                }
            }
        }
        else
        {
            // Load from datastream
            for(uint16_t k = 0; k < recordsCount; k++)
            {
                auto obj = config->BuildHandler(*this);
                if(!obj->LoadPacket(p, false))
                {
                    LOG_ERROR(libcomp::String("Invalid update data stream"
                        " received from non-persistent object of type: %1\n")
                        .Arg(type));
                    return false;
                }

                records.push_back(obj);
            }
        }

        bool customHandler = config->UpdateHandler != nullptr;
        bool completeHandler = config->SyncCompleteHandler != nullptr;
        if(customHandler || completeHandler)
        {
            for(auto obj : records)
            {
                bool complete = true;
                if(customHandler)
                {
                    int8_t result = config->UpdateHandler(*this, lType, obj,
                        false, source);
                    if(result == SYNC_UPDATED)
                    {
                        if(config->ServerOwned)
                        {
                            // Queue up to report to the other server(s)
                            mOutboundUpdates[type].insert(obj);
                        }
                    }
                    else if(result == SYNC_FAILED)
                    {
                        LOG_ERROR(libcomp::String("Failed to sync update of"
                            " record of type: %1\n").Arg(type));
                        complete = false;
                    }
                }

                if(complete && completeHandler)
                {
                    completed.push_back(std::make_pair(obj, false));
                }
            }
        }

        if(p.Left() < 2)
        {
            return false;
        }

        // Read removes
        recordsCount = p.ReadU16Little();

        records.clear();
        if(isPersistent)
        {
            auto db = config->DB;

            // Get by UUID and clear from queues if found
            // If the record is server owned, attempt to load it
            for(uint16_t k = 0; k < recordsCount; k++)
            {
                String uidStr(p.ReadString16Little(
                    libcomp::Convert::ENCODING_UTF8, true));

                libobjgen::UUID uid(uidStr.C());
                if(!uid.IsNull())
                {
                    auto obj = config->ServerOwned && db
                        ? PersistentObject::LoadObjectByUUID(typeHash, db, uid)
                        : PersistentObject::GetObjectByUUID(uid);
                    if(obj)
                    {
                        // Remove from the queues
                        mOutboundUpdates[type].erase(obj);
                        mOutboundRemoves[type].erase(obj);

                        records.push_back(obj);
                    }
                }
                else
                {
                    // Skip null UIDs
                    LOG_ERROR(libcomp::String("Null UID encountered for"
                        " removed sync record of type: %1\n").Arg(type));
                }
            }
        }
        else
        {
            // Load from datastream
            for(uint16_t k = 0; k < recordsCount; k++)
            {
                auto obj = config->BuildHandler(*this);
                if(!obj->LoadPacket(p, false))
                {
                    LOG_ERROR(libcomp::String("Invalid remove data stream"
                        " received from non-persistent object of type: %1\n")
                        .Arg(type));
                    return false;
                }

                records.push_back(obj);
            }
        }

        if(customHandler || completeHandler)
        {
            for(auto obj : records)
            {
                bool complete = true;
                if(customHandler)
                {
                    int8_t result = config->UpdateHandler(*this, lType, obj,
                        true, source);
                    if(result == SYNC_UPDATED)
                    {
                        if(config->ServerOwned)
                        {
                            // Queue up to report to the other server(s)
                            mOutboundRemoves[type].insert(obj);
                        }
                    }
                    else if(result == SYNC_FAILED)
                    {
                        LOG_ERROR(libcomp::String("Failed to sync removal of"
                            " record of type: %1\n").Arg(type));
                        complete = false;
                    }
                }

                if(complete && completeHandler)
                {
                    completed.push_back(std::make_pair(obj, true));
                }
            }
        }
    }

    if(config->SyncCompleteHandler)
    {
        config->SyncCompleteHandler(*this, config->Name, completed, source);
    }

    return true;
}

bool DataSyncManager::UpdateRecord(const std::shared_ptr<libcomp::Object>& record,
    const libcomp::String& type)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto configIter = mRegisteredTypes.find(type.C());
    if(record != nullptr && configIter != mRegisteredTypes.end())
    {
        if((configIter->second->ServerOwned ||
            configIter->second->DynamicHandler) &&
            configIter->second->UpdateHandler)
        {
            int8_t result = configIter->second->UpdateHandler(*this, type,
                record, false, "");
            if(result == SYNC_HANDLED)
            {
                return false;
            }
        }

        for(auto pair : mConnections)
        {
            if(pair.second.find(type.C()) != pair.second.end())
            {
                mOutboundUpdates[type.C()].insert(record);
                return true;
            }
        }
    }

    return false;
}

bool DataSyncManager::SyncRecordUpdate(const std::shared_ptr<
    libcomp::Object>& record, const libcomp::String& type)
{
    if(UpdateRecord(record, type))
    {
        SyncOutgoing();
        return true;
    }

    return false;
}

bool DataSyncManager::RemoveRecord(const std::shared_ptr<libcomp::Object>& record,
    const libcomp::String& type)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto configIter = mRegisteredTypes.find(type.C());
    if(record != nullptr && configIter != mRegisteredTypes.end())
    {
        if((configIter->second->ServerOwned ||
            configIter->second->DynamicHandler) &&
            configIter->second->UpdateHandler)
        {
            int8_t result = configIter->second->UpdateHandler(*this, type,
                record, true, "");
            if(result == SYNC_HANDLED)
            {
                return false;
            }
        }

        for(auto pair : mConnections)
        {
            if(pair.second.find(type.C()) != pair.second.end())
            {
                mOutboundRemoves[type.C()].insert(record);
                return true;
            }
        }
    }

    return false;
}

bool DataSyncManager::SyncRecordRemoval(const std::shared_ptr<
    libcomp::Object>& record, const libcomp::String& type)
{
    if(RemoveRecord(record, type))
    {
        SyncOutgoing();
        return true;
    }

    return false;
}

void DataSyncManager::QueueOutgoing(const libcomp::String& type,
    const std::shared_ptr<InternalConnection>& connection,
    const std::set<std::shared_ptr<libcomp::Object>>& updates,
    const std::set<std::shared_ptr<libcomp::Object>>& removes)
{
    if(updates.size() == 0 && removes.size() == 0) return;

    libcomp::Packet p;
    p.WritePacketCode(InternalPacketCode_t::PACKET_DATA_SYNC);

    bool isPersistent = false;
    PersistentObject::GetTypeHashByName(type.C(), isPersistent);

    p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
        type, true);

    WriteOutgoingRecords(p, isPersistent, updates);
    WriteOutgoingRecords(p, isPersistent, removes);

    connection->QueuePacket(p);
}

void DataSyncManager::WriteOutgoingRecord(libcomp::Packet& p, bool isPersistent,
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& record)
{
    p.WritePacketCode(InternalPacketCode_t::PACKET_DATA_SYNC);

    p.WriteString16Little(libcomp::Convert::ENCODING_UTF8, type, true);
    p.WriteU16Little(1);

    if(isPersistent)
    {
        // Write the UUID
        auto pObj = std::dynamic_pointer_cast<
            PersistentObject>(record);
        p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
            pObj->GetUUID().ToString(), true);
    }
    else
    {
        // Write the datastream
        record->SavePacket(p, false);
    }

    p.WriteU16Little(0);    // No deletes
}

void DataSyncManager::WriteOutgoingRecords(libcomp::Packet& p,
    bool isPersistent, const std::set<std::shared_ptr<libcomp::Object>>& records)
{
    p.WriteU16Little((uint16_t)records.size());
    if(isPersistent)
    {
        // Write the UUID
        for(auto obj : records)
        {
            auto pObj = std::dynamic_pointer_cast<
                PersistentObject>(obj);
            p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                pObj->GetUUID().ToString(), true);
        }
    }
    else
    {
        // Write the datastream
        for(auto obj : records)
        {
            obj->SavePacket(p, false);
        }
    }
}