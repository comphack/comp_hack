/**
 * @file libcomp/src/DefinitionManager.cpp
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

#include "DefinitionManager.h"

// libcomp Includes
#include "Log.h"

// object Includes
#include <MiCItemBaseData.h>
#include <MiCItemData.h>
#include <MiDevilData.h>
#include <MiDevilLVUpRateData.h>
#include <MiExpertClassData.h>
#include <MiExpertData.h>
#include <MiExpertRankData.h>
#include <MiHNPCBasicData.h>
#include <MiHNPCData.h>
#include <MiItemData.h>
#include <MiNPCBasicData.h>
#include <MiONPCData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>

using namespace libcomp;

DefinitionManager::DefinitionManager()
{
}

DefinitionManager::~DefinitionManager()
{
}

const std::shared_ptr<objects::MiDevilData> DefinitionManager::GetDevilData(uint32_t id)
{
    return GetRecordByID<objects::MiDevilData>(id, mDevilData);
}

const std::shared_ptr<objects::MiDevilData> DefinitionManager::GetDevilData(const libcomp::String& name)
{
    auto iter = mDevilNameLookup.find(name);
    if(iter != mDevilNameLookup.end())
    {
        return GetDevilData(iter->second);
    }

    return nullptr;
}

const std::shared_ptr<objects::MiDevilLVUpRateData> DefinitionManager::GetDevilLVUpRateData(uint32_t id)
{
    return GetRecordByID<objects::MiDevilLVUpRateData>(id, mDevilLVUpRateData);
}

const std::shared_ptr<objects::MiExpertData> DefinitionManager::GetExpertClassData(uint32_t id)
{
    return GetRecordByID<objects::MiExpertData>(id, mExpertData);
}

const std::shared_ptr<objects::MiHNPCData> DefinitionManager::GetHNPCData(uint32_t id)
{
    return GetRecordByID<objects::MiHNPCData>(id, mHNPCData);
}

const std::shared_ptr<objects::MiItemData> DefinitionManager::GetItemData(uint32_t id)
{
    return GetRecordByID<objects::MiItemData>(id, mItemData);
}

const std::shared_ptr<objects::MiItemData> DefinitionManager::GetItemData(const libcomp::String& name)
{
    auto iter = mCItemNameLookup.find(name);
    if(iter != mCItemNameLookup.end())
    {
        return GetRecordByID<objects::MiItemData>(iter->second, mItemData);
    }

    return nullptr;
}

const std::shared_ptr<objects::MiONPCData> DefinitionManager::GetONPCData(uint32_t id)
{
    return GetRecordByID<objects::MiONPCData>(id, mONPCData);
}

const std::shared_ptr<objects::MiSkillData> DefinitionManager::GetSkillData(uint32_t id)
{
    return GetRecordByID<objects::MiSkillData>(id, mSkillData);
}

const std::list<uint32_t> DefinitionManager::GetDefaultCharacterSkills()
{
    return mDefaultCharacterSkills;
}

bool DefinitionManager::LoadAllData(const libcomp::String& path)
{
    LOG_INFO("Loading binary data definitions...\n");
    bool success = true;
    success &= LoadCItemData(path);
    success &= LoadDevilData(path);
    success &= LoadDevilLVUpRateData(path);
    success &= LoadExpertClassData(path);
    success &= LoadHNPCData(path);
    success &= LoadItemData(path);
    success &= LoadONPCData(path);
    success &= LoadSkillData(path);

    if(success)
    {
        LOG_INFO("Definition loading complete.\n");
    }
    else
    {
        LOG_CRITICAL("Definition loading failed.\n");
    }

    return success;
}

bool DefinitionManager::LoadCItemData(const libcomp::String& path)
{
    std::list<std::shared_ptr<objects::MiCItemData>> records;
    bool success = LoadBinaryData<objects::MiCItemData>(path, "Shield/CItemData.sbin",
        true, 0, records);
    for(auto record : records)
    {
        auto id = record->GetBaseData()->GetID();
        auto name = record->GetBaseData()->GetName();
        if(mCItemNameLookup.find(name) == mCItemNameLookup.end())
        {
            mCItemNameLookup[name] = id;
        }
    }
    
    return success;
}

bool DefinitionManager::LoadDevilData(const libcomp::String& path)
{
    std::list<std::shared_ptr<objects::MiDevilData>> records;
    bool success = LoadBinaryData<objects::MiDevilData>(path, "Shield/DevilData.sbin",
        true, 0, records);
    for(auto record : records)
    {
        auto id = record->GetBasic()->GetID();
        auto name = record->GetBasic()->GetName();

        mDevilData[id] = record;
        if(mDevilNameLookup.find(name) == mDevilNameLookup.end())
        {
            mDevilNameLookup[name] = id;
        }
    }
    
    return success;
}

bool DefinitionManager::LoadDevilLVUpRateData(const libcomp::String& path)
{
    std::list<std::shared_ptr<objects::MiDevilLVUpRateData>> records;
    bool success = LoadBinaryData<objects::MiDevilLVUpRateData>(path, "Shield/DevilLVUpRateData.sbin",
        true, 0, records);
    for(auto record : records)
    {
        mDevilLVUpRateData[record->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadExpertClassData(const libcomp::String& path)
{
    std::list<std::shared_ptr<objects::MiExpertData>> records;
    bool success = LoadBinaryData<objects::MiExpertData>(path, "Shield/ExpertClassData.sbin",
        true, 0, records);
    for(auto record : records)
    {
        mExpertData[record->GetID()] = record;

        if(!record->GetDisabled())
        {
            auto class0Data = record->GetClassData(0);
            auto rank0Data = class0Data->GetRankData(0);
            for(uint32_t i = 0; i < rank0Data->GetSkillCount(); i++)
            {
                mDefaultCharacterSkills.push_back(rank0Data->GetSkill((size_t)i));
            }
        }
    }

    return success;
}

bool DefinitionManager::LoadHNPCData(const libcomp::String& path)
{
    std::list<std::shared_ptr<objects::MiHNPCData>> records;
    bool success = LoadBinaryData<objects::MiHNPCData>(path, "Shield/hNPCData.sbin",
        true, 0, records);
    for(auto record : records)
    {
        mHNPCData[record->GetBasic()->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadItemData(const libcomp::String& path)
{
    std::list<std::shared_ptr<objects::MiItemData>> records;
    bool success = LoadBinaryData<objects::MiItemData>(path, "Shield/ItemData.sbin",
        true, 2, records);
    for(auto record : records)
    {
        mItemData[record->GetCommon()->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadONPCData(const libcomp::String& path)
{
    std::list<std::shared_ptr<objects::MiONPCData>> records;
    bool success = LoadBinaryData<objects::MiONPCData>(path, "Shield/oNPCData.sbin",
        true, 0, records);
    for(auto record : records)
    {
        mONPCData[record->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadSkillData(const libcomp::String& path)
{
    std::list<std::shared_ptr<objects::MiSkillData>> records;
    bool success = LoadBinaryData<objects::MiSkillData>(path, "Shield/SkillData.sbin",
        true, 4, records);
    for(auto record : records)
    {
        mSkillData[record->GetCommon()->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadBinaryDataHeader(libcomp::ObjectInStream& ois,
    const libcomp::String& binaryFile, uint16_t tablesExpected,
    uint16_t& entryCount, uint16_t& tableCount)
{
    ois.stream.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
    ois.stream.read(reinterpret_cast<char*>(&tableCount), sizeof(tableCount));

	if(!ois.stream.good())
	{
		LOG_CRITICAL(libcomp::String("Failed to load/decrypt '%1'.\n")
            .Arg(binaryFile));
        return false;
	}

    if(tablesExpected > 0 && tablesExpected != tableCount)
    {
        LOG_CRITICAL(libcomp::String("Expected %1 table(s) in file '%2'"
            " but encountered %3.\n").Arg(tablesExpected).Arg(binaryFile)
            .Arg(tableCount));
        return false;
    }
    return true;
}

void DefinitionManager::PrintLoadResult(const libcomp::String& binaryFile,
    bool success, uint16_t entriesExpected, size_t loadedEntries)
{
    if(success)
    {
        LOG_DEBUG(libcomp::String("Successfully loaded %1/%2 records from %3.\n")
            .Arg(loadedEntries).Arg(entriesExpected).Arg(binaryFile));
    }
    else
    {
        LOG_ERROR(libcomp::String("Failed after loading %1/%2 records from %3.\n")
            .Arg(loadedEntries).Arg(entriesExpected).Arg(binaryFile));
    }
}
