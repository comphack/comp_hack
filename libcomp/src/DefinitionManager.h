/**
 * @file libcomp/src/DefinitionManager.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Manages parsing and storing binary game data definitions.
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

#ifndef LIBCOMP_SRC_DEFINITIONMANAGER_H
#define LIBCOMP_SRC_DEFINITIONMANAGER_H

// Standard C++14 Includes
#include <gsl/gsl>

// libcomp Includes
#include "CString.h"
#include "DataStore.h"
#include "Decrypt.h"
#include "Object.h"

// Standard C++11 Includes
#include <unordered_map>

namespace objects
{
class MiCItemData;
class MiDevilData;
class MiDevilLVUpRateData;
class MiExpertData;
class MiHNPCData;
class MiItemData;
class MiONPCData;
class MiSkillData;
}

namespace libcomp
{

class DefinitionManager
{
public:
    DefinitionManager();
    ~DefinitionManager();

    const std::shared_ptr<objects::MiDevilData> GetDevilData(uint32_t id);
    const std::shared_ptr<objects::MiDevilData> GetDevilData(const libcomp::String& name);
    const std::shared_ptr<objects::MiDevilLVUpRateData> GetDevilLVUpRateData(uint32_t id);
    const std::shared_ptr<objects::MiExpertData> GetExpertClassData(uint32_t id);
    const std::shared_ptr<objects::MiHNPCData> GetHNPCData(uint32_t id);
    const std::shared_ptr<objects::MiItemData> GetItemData(uint32_t id);
    const std::shared_ptr<objects::MiItemData> GetItemData(const libcomp::String& name);
    const std::shared_ptr<objects::MiONPCData> GetONPCData(uint32_t id);
    const std::shared_ptr<objects::MiSkillData> GetSkillData(uint32_t id);

    const std::list<uint32_t> GetDefaultCharacterSkills();

    bool LoadAllData(gsl::not_null<DataStore*> pDataStore);

    bool LoadCItemData(gsl::not_null<DataStore*> pDataStore);
    bool LoadDevilData(gsl::not_null<DataStore*> pDataStore);
    bool LoadDevilLVUpRateData(gsl::not_null<DataStore*> pDataStore);
    bool LoadExpertClassData(gsl::not_null<DataStore*> pDataStore);
    bool LoadHNPCData(gsl::not_null<DataStore*> pDataStore);
    bool LoadItemData(gsl::not_null<DataStore*> pDataStore);
    bool LoadONPCData(gsl::not_null<DataStore*> pDataStore);
    bool LoadSkillData(gsl::not_null<DataStore*> pDataStore);

private:
    template <class T>
    bool LoadBinaryData(gsl::not_null<DataStore*> pDataStore,
        const libcomp::String& binaryFile, bool decrypt,
        uint16_t tablesExpected, std::list<std::shared_ptr<T>>& records)
    {
        std::vector<char> data;

        auto path = libcomp::String("/BinaryData/") + binaryFile;

        if(decrypt)
        {
            data = pDataStore->DecryptFile(path);
        }
        else
        {
            data = pDataStore->ReadFile(path);
        }

        if(data.empty())
        {
            return false;
        }

        std::stringstream ss(std::string(data.begin(), data.end()));
        libcomp::ObjectInStream ois(ss);

        uint16_t entryCount, tableCount;
        if(!LoadBinaryDataHeader(ois, binaryFile, tablesExpected,
            entryCount, tableCount))
        {
            return false;
        }

        size_t dynamicCounts = (size_t)(entryCount * tableCount);
        for(uint16_t i = 0; i < dynamicCounts; i++)
        {
            uint16_t ds;
            ois.stream.read(reinterpret_cast<char*>(&ds), sizeof(ds));
            ois.dynamicSizes.push_back(ds);
        }

	    for(uint16_t i = 0; i < entryCount; i++)
	    {
            auto entry = std::shared_ptr<T>(new T);

            if(!entry->Load(ois))
            {
                PrintLoadResult(binaryFile, false, entryCount, records.size());
                return false;
            }

            records.push_back(entry);
	    }

        bool success = entryCount == records.size() && ois.stream.good();
        PrintLoadResult(binaryFile, success, entryCount, records.size());

        return success;
    }

    bool LoadBinaryDataHeader(libcomp::ObjectInStream& ois,
        const libcomp::String& binaryFile, uint16_t tablesExpected,
        uint16_t& entryCount, uint16_t& tableCount);

    void PrintLoadResult(const libcomp::String& binaryFile, bool success,
        uint16_t entriesExpected, size_t loadedEntries);

    template <class T>
    std::shared_ptr<T> GetRecordByID(uint32_t id,
        std::unordered_map<uint32_t, std::shared_ptr<T>>& data)
    {
        auto iter = data.find(id);
        if(iter != data.end())
        {
            return iter->second;
        }

        return nullptr;
    }

    std::list<uint32_t> mDefaultCharacterSkills;

    std::unordered_map<libcomp::String, uint32_t> mCItemNameLookup;
    
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiDevilData>> mDevilData;
    // Names are not unique across entries
    std::unordered_map<libcomp::String, uint32_t> mDevilNameLookup;
    
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiDevilLVUpRateData>> mDevilLVUpRateData;

    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiExpertData>> mExpertData;
    
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiHNPCData>> mHNPCData;

    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiItemData>> mItemData;
    
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiONPCData>> mONPCData;

    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiSkillData>> mSkillData;
};

} // namspace libcomp

#endif // LIBCOMP_SRC_DEFINITIONMANAGER_H
