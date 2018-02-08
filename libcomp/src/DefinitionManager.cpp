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
#include <EnchantSetData.h>
#include <EnchantSpecialData.h>
#include <MiAIData.h>
#include <MiCItemBaseData.h>
#include <MiCItemData.h>
#include <MiCZoneRelationData.h>
#include <MiDCategoryData.h>
#include <MiDevilCrystalData.h>
#include <MiDevilData.h>
#include <MiDevilLVUpRateData.h>
#include <MiDisassemblyData.h>
#include <MiDisassemblyTriggerData.h>
#include <MiDynamicMapData.h>
#include <MiEnchantData.h>
#include <MiEquipmentSetData.h>
#include <MiExpertClassData.h>
#include <MiExpertData.h>
#include <MiExpertRankData.h>
#include <MiGrowthData.h>
#include <MiHNPCBasicData.h>
#include <MiHNPCData.h>
#include <MiItemData.h>
#include <MiModificationData.h>
#include <MiModificationExtEffectData.h>
#include <MiModificationExtRecipeData.h>
#include <MiModificationTriggerData.h>
#include <MiModifiedEffectData.h>
#include <MiNPCBarterData.h>
#include <MiNPCBasicData.h>
#include <MiONPCData.h>
#include <MiQuestData.h>
#include <MiShopProductData.h>
#include <MiSItemData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpotData.h>
#include <MiStatusData.h>
#include <MiTriUnionSpecialData.h>
#include <MiUnionData.h>
#include <MiZoneData.h>
#include <MiZoneBasicData.h>
#include <QmpFile.h>
#include <Tokusei.h>

using namespace libcomp;

DefinitionManager::DefinitionManager()
{
}

DefinitionManager::~DefinitionManager()
{
}

const std::shared_ptr<objects::MiAIData> DefinitionManager::GetAIData(uint32_t id)
{
    return GetRecordByID(id, mAIData);
}

const std::shared_ptr<objects::MiDevilData> DefinitionManager::GetDevilData(uint32_t id)
{
    return GetRecordByID(id, mDevilData);
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
    return GetRecordByID(id, mDevilLVUpRateData);
}

const std::shared_ptr<objects::MiDisassemblyData> DefinitionManager::GetDisassemblyData(uint32_t id)
{
    return GetRecordByID(id, mDisassemblyData);
}

const std::shared_ptr<objects::MiDisassemblyData> DefinitionManager::GetDisassemblyDataByItemID(
    uint32_t itemID)
{
    auto iter = mDisassemblyLookup.find(itemID);
    if(iter != mDisassemblyLookup.end())
    {
        return GetRecordByID(iter->second, mDisassemblyData);
    }

    return nullptr;
}

const std::shared_ptr<objects::MiDisassemblyTriggerData> DefinitionManager::GetDisassemblyTriggerData(
    uint32_t id)
{
    return GetRecordByID(id, mDisassemblyTriggerData);
}

const std::list<uint32_t> DefinitionManager::GetDisassembledItemIDs()
{
    return mDisassembledItemIDs;
}

const std::shared_ptr<objects::MiDynamicMapData> DefinitionManager::GetDynamicMapData(uint32_t id)
{
    return GetRecordByID(id, mDynamicMapData);
}

const std::shared_ptr<objects::MiEnchantData> DefinitionManager::GetEnchantData(int16_t id)
{
    return GetRecordByID(id, mEnchantData);
}

std::unordered_map<int16_t,
    std::shared_ptr<objects::MiEnchantData>> DefinitionManager::GetAllEnchantData()
{
    return mEnchantData;
}

const std::shared_ptr<objects::MiEnchantData> DefinitionManager::GetEnchantDataByDemonID(uint32_t demonID)
{
    auto iter = mEnchantDemonLookup.find(demonID);
    if(iter != mEnchantDemonLookup.end())
    {
        return GetEnchantData(iter->second);
    }

    return nullptr;
}

const std::shared_ptr<objects::MiEnchantData> DefinitionManager::GetEnchantDataByItemID(uint32_t itemID)
{
    auto iter = mEnchantItemLookup.find(itemID);
    if(iter != mEnchantItemLookup.end())
    {
        return GetEnchantData(iter->second);
    }

    return nullptr;
}

const std::shared_ptr<objects::MiEquipmentSetData> DefinitionManager::GetEquipmentSetData(uint32_t id)
{
    return GetRecordByID(id, mEquipmentSetData);
}

std::list<std::shared_ptr<objects::MiEquipmentSetData>> DefinitionManager::GetEquipmentSetDataByItem(
    uint32_t equipmentID)
{
    std::list<std::shared_ptr<objects::MiEquipmentSetData>> retval;

    auto it = mEquipmentSetLookup.find(equipmentID);
    if(it != mEquipmentSetLookup.end())
    {
        for(auto setID : it->second)
        {
            auto equipmentSet = GetEquipmentSetData(setID);
            if(equipmentSet)
            {
                retval.push_back(equipmentSet);
            }
        }
    }

    return retval;
}

const std::shared_ptr<objects::MiExpertData> DefinitionManager::GetExpertClassData(uint32_t id)
{
    return GetRecordByID(id, mExpertData);
}

const std::list<std::pair<uint8_t, uint32_t>> DefinitionManager::GetFusionRanges(uint8_t raceID)
{
    auto iter = mFusionRanges.find(raceID);
    if(iter != mFusionRanges.end())
    {
        return iter->second;
    }

    return std::list<std::pair<uint8_t, uint32_t>>();
}

const std::shared_ptr<objects::MiHNPCData> DefinitionManager::GetHNPCData(uint32_t id)
{
    return GetRecordByID(id, mHNPCData);
}

const std::shared_ptr<objects::MiItemData> DefinitionManager::GetItemData(uint32_t id)
{
    return GetRecordByID(id, mItemData);
}

const std::shared_ptr<objects::MiModificationData> DefinitionManager::GetModificationData(uint32_t id)
{
    return GetRecordByID(id, mModificationData);
}

const std::shared_ptr<objects::MiModificationData> DefinitionManager::GetModificationDataByItemID(
    uint32_t itemID)
{
    auto iter = mModificationLookup.find(itemID);
    if(iter != mModificationLookup.end())
    {
        return GetRecordByID(iter->second, mModificationData);
    }

    return nullptr;
}

const std::shared_ptr<objects::MiModificationExtEffectData>
    DefinitionManager::GetModificationExtEffectData(uint8_t groupID, uint8_t slot, uint16_t subID)
{
    auto iter1 = mModificationExtEffectData.find(groupID);
    if(iter1 != mModificationExtEffectData.end())
    {
        auto iter2 = iter1->second.find(slot);
        if(iter2 != iter1->second.end())
        {
            return GetRecordByID(subID, iter2->second);
        }
    }

    return nullptr;
}

const std::shared_ptr<objects::MiModificationExtRecipeData>
    DefinitionManager::GetModificationExtRecipeData(uint32_t id)
{
    return GetRecordByID(id, mModificationExtRecipeData);
}

const std::shared_ptr<objects::MiModificationExtRecipeData>
    DefinitionManager::GetModificationExtRecipeDataByItemID(uint32_t itemID)
{
    auto iter = mModificationExtRecipeLookup.find(itemID);
    if(iter != mModificationExtRecipeLookup.end())
    {
        return GetRecordByID(iter->second, mModificationExtRecipeData);
    }

    return nullptr;
}

const std::shared_ptr<objects::MiModificationTriggerData> DefinitionManager::GetModificationTriggerData(
    uint16_t id)
{
    return GetRecordByID(id, mModificationTriggerData);
}

const std::shared_ptr<objects::MiModifiedEffectData> DefinitionManager::GetModifiedEffectData(
    uint16_t id)
{
    return GetRecordByID(id, mModifiedEffectData);
}

const std::shared_ptr<objects::MiItemData> DefinitionManager::GetItemData(const libcomp::String& name)
{
    auto iter = mCItemNameLookup.find(name);
    if(iter != mCItemNameLookup.end())
    {
        return GetRecordByID(iter->second, mItemData);
    }

    return nullptr;
}

const std::shared_ptr<objects::MiNPCBarterData> DefinitionManager::GetNPCBarterData(uint16_t id)
{
    return GetRecordByID(id, mNPCBarterData);
}

const std::shared_ptr<objects::MiONPCData> DefinitionManager::GetONPCData(uint32_t id)
{
    return GetRecordByID(id, mONPCData);
}

const std::shared_ptr<objects::MiQuestData> DefinitionManager::GetQuestData(uint32_t id)
{
    return GetRecordByID(id, mQuestData);
}

const std::shared_ptr<objects::MiShopProductData> DefinitionManager::GetShopProductData(uint32_t id)
{
    return GetRecordByID(id, mShopProductData);
}

const std::shared_ptr<objects::MiSItemData> DefinitionManager::GetSItemData(uint32_t id)
{
    return GetRecordByID(id, mSItemData);
}

const std::shared_ptr<objects::MiSkillData> DefinitionManager::GetSkillData(uint32_t id)
{
    return GetRecordByID(id, mSkillData);
}

const std::unordered_map<uint32_t,
    std::shared_ptr<objects::MiSpotData>> DefinitionManager::GetSpotData(uint32_t dynamicMapID)
{
    std::unordered_map<uint32_t, std::shared_ptr<objects::MiSpotData>> result;

    auto dynamicMap = GetDynamicMapData(dynamicMapID);
    if(dynamicMap)
    {
        std::string filename(dynamicMap->GetSpotDataFile().C());

        auto it = mSpotData.find(filename);
        if(it != mSpotData.end())
        {
            result = it->second;
        }
    }

    return result;
}

const std::shared_ptr<objects::MiStatusData> DefinitionManager::GetStatusData(uint32_t id)
{
    return GetRecordByID(id, mStatusData);
}


const std::list<std::shared_ptr<objects::MiTriUnionSpecialData>>
    DefinitionManager::GetTriUnionSpecialData(uint32_t sourceDemonTypeID)
{
    std::list<std::shared_ptr<objects::MiTriUnionSpecialData>> result;
    auto it = mTriUnionSpecialDataBySourceID.find(sourceDemonTypeID);
    if(it != mTriUnionSpecialDataBySourceID.end())
    {
        for(auto specialID : it->second)
        {
            result.push_back(mTriUnionSpecialData[specialID]);
        }
    }

    return result;
}

const std::shared_ptr<objects::MiZoneData> DefinitionManager::GetZoneData(uint32_t id)
{
    return GetRecordByID(id, mZoneData);
}

const std::shared_ptr<objects::MiCZoneRelationData> DefinitionManager::GetZoneRelationData(uint32_t id)
{
    return GetRecordByID(id, mZoneRelationData);
}

const std::shared_ptr<objects::EnchantSetData>  DefinitionManager::GetEnchantSetData(uint32_t id)
{
    return GetRecordByID(id, mEnchantSetData);
}

std::list<std::shared_ptr<objects::EnchantSetData>>  DefinitionManager::GetEnchantSetDataByEffect(
    int16_t effectID)
{
    std::list<std::shared_ptr<objects::EnchantSetData>> retval;

    auto it = mEnchantSetLookup.find(effectID);
    if(it != mEnchantSetLookup.end())
    {
        for(auto setID : it->second)
        {
            auto enchantSet = GetEnchantSetData(setID);
            if(enchantSet)
            {
                retval.push_back(enchantSet);
            }
        }
    }

    return retval;
}

const std::unordered_map<uint32_t,
    std::shared_ptr<objects::EnchantSetData>> DefinitionManager::GetAllEnchantSetData()
{
    return mEnchantSetData;
}

const std::shared_ptr<objects::EnchantSpecialData>  DefinitionManager::GetEnchantSpecialData(uint32_t id)
{
    return GetRecordByID(id, mEnchantSpecialData);
}

std::list<std::shared_ptr<objects::EnchantSpecialData>>
    DefinitionManager::GetEnchantSpecialDataByInputItem(uint32_t itemID)
{
    std::list<std::shared_ptr<objects::EnchantSpecialData>> retval;

    auto it = mEnchantSpecialLookup.find(itemID);
    if(it != mEnchantSpecialLookup.end())
    {
        for(auto specialID : it->second)
        {
            auto specialSet = GetEnchantSpecialData(specialID);
            if(specialSet)
            {
                retval.push_back(specialSet);
            }
        }
    }

    return retval;
}

const std::shared_ptr<objects::Tokusei> DefinitionManager::GetTokuseiData(int32_t id)
{
    return GetRecordByID(id, mTokuseiData);
}

const std::unordered_map<int32_t,
    std::shared_ptr<objects::Tokusei>> DefinitionManager::GetAllTokuseiData()
{
    return mTokuseiData;
}

const std::list<uint32_t> DefinitionManager::GetDefaultCharacterSkills()
{
    return mDefaultCharacterSkills;
}

bool DefinitionManager::LoadAllData(gsl::not_null<DataStore*> pDataStore)
{
    LOG_INFO("Loading binary data definitions...\n");
    bool success = true;
    success &= LoadAIData(pDataStore);
    success &= LoadCItemData(pDataStore);
    success &= LoadDevilData(pDataStore);
    success &= LoadDevilLVUpRateData(pDataStore);
    success &= LoadDisassemblyData(pDataStore);
    success &= LoadDisassemblyTriggerData(pDataStore);
    success &= LoadDynamicMapData(pDataStore);
    success &= LoadEnchantData(pDataStore);
    success &= LoadEquipmentSetData(pDataStore);
    success &= LoadExpertClassData(pDataStore);
    success &= LoadHNPCData(pDataStore);
    success &= LoadItemData(pDataStore);
    success &= LoadModificationData(pDataStore);
    success &= LoadModificationExtEffectData(pDataStore);
    success &= LoadModificationExtRecipeData(pDataStore);
    success &= LoadModificationTriggerData(pDataStore);
    success &= LoadModifiedEffectData(pDataStore);
    success &= LoadNPCBarterData(pDataStore);
    success &= LoadONPCData(pDataStore);
    success &= LoadQuestData(pDataStore);
    success &= LoadShopProductData(pDataStore);
    success &= LoadSItemData(pDataStore);
    success &= LoadSkillData(pDataStore);
    success &= LoadStatusData(pDataStore);
    success &= LoadTriUnionSpecialData(pDataStore);
    success &= LoadZoneData(pDataStore);

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

bool DefinitionManager::LoadAIData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiAIData>> records;
    bool success = LoadBinaryData<objects::MiAIData>(pDataStore,
        "Shield/AIData.sbin", true, 0, records);
    for(auto record : records)
    {
        mAIData[record->GetID()] = record;
    }

    return success;
}

bool DefinitionManager::LoadCItemData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiCItemData>> records;
    bool success = LoadBinaryData<objects::MiCItemData>(pDataStore,
        "Shield/CItemData.sbin", true, 0, records);
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

bool DefinitionManager::LoadCZoneRelationData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiCZoneRelationData>> records;
    bool success = LoadBinaryData<objects::MiCZoneRelationData>(pDataStore,
        "Shield/CZoneRelationData.sbin", true, 0, records);
    for(auto record : records)
    {
        mZoneRelationData[record->GetID()] = record;
    }

    return success;
}

bool DefinitionManager::LoadDevilData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiDevilData>> records;
    bool success = LoadBinaryData<objects::MiDevilData>(pDataStore,
        "Shield/DevilData.sbin", true, 0, records);
    for(auto record : records)
    {
        auto id = record->GetBasic()->GetID();
        auto name = record->GetBasic()->GetName();

        mDevilData[id] = record;
        if(mDevilNameLookup.find(name) == mDevilNameLookup.end())
        {
            mDevilNameLookup[name] = id;
        }

        // If the fusion options contain 2-way fusion result, add to
        // the fusion range map
        auto fusionOptions = record->GetUnionData()->GetFusionOptions();
        if(fusionOptions & 2)
        {
            mFusionRanges[(uint8_t)record->GetCategory()->GetRace()].
                push_back(std::pair<uint8_t, uint32_t>(
                (uint8_t)(record->GetGrowth()->GetBaseLevel() * 2 - 1), id));
        }
    }

    // Sort the fusion ranges
    for(auto& pair : mFusionRanges)
    {
        auto& ranges = pair.second;
        ranges.sort([](const std::pair<uint8_t, uint32_t>& a,
            const std::pair<uint8_t, uint32_t>& b)
        {
            return a.first < b.first;
        });
    }
    
    return success;
}

bool DefinitionManager::LoadDevilLVUpRateData(
    gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiDevilLVUpRateData>> records;
    bool success = LoadBinaryData<objects::MiDevilLVUpRateData>(pDataStore,
        "Shield/DevilLVUpRateData.sbin", true, 0, records);
    for(auto record : records)
    {
        mDevilLVUpRateData[record->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadDisassemblyData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiDisassemblyData>> records;
    bool success = LoadBinaryData<objects::MiDisassemblyData>(pDataStore,
        "Shield/DisassemblyData.sbin", true, 0, records);
    for(auto record : records)
    {
        uint32_t id = record->GetID();
        mDisassemblyData[id] = record;
        mDisassemblyLookup[record->GetItemID()] = id;
    }
    
    return success;
}

bool DefinitionManager::LoadDisassemblyTriggerData(gsl::not_null<
    DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiDisassemblyTriggerData>> records;
    bool success = LoadBinaryData<objects::MiDisassemblyTriggerData>(pDataStore,
        "Shield/DisassemblyTriggerData.sbin", true, 0, records);
    for(auto record : records)
    {
        mDisassemblyTriggerData[record->GetID()] = record;
        mDisassembledItemIDs.push_back(record->GetID());
    }
    
    return success;
}

bool DefinitionManager::LoadDynamicMapData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiDynamicMapData>> records;
    bool success = LoadBinaryData<objects::MiDynamicMapData>(pDataStore,
        "Client/DynamicMapData.bin", false, 0, records);

    uint16_t spotLoadCount = 0;
    for(auto record : records)
    {
        mDynamicMapData[record->GetID()] = record;

        std::string filename(record->GetSpotDataFile().C());
        if(filename.length() > 0 && mSpotData.find(filename) == mSpotData.end())
        {
            spotLoadCount++;

            std::list<std::shared_ptr<objects::MiSpotData>> spotRecords;
            bool spotSuccess = LoadBinaryData<objects::MiSpotData>(pDataStore,
                libcomp::String("Client/%1").Arg(filename), false, 0, spotRecords,
                false);
            if(spotSuccess)
            {
                for(auto spotRecord : spotRecords)
                {
                    mSpotData[filename][spotRecord->GetID()] = spotRecord;
                }
            }
        }
    }

    if(spotLoadCount != (uint16_t)mSpotData.size())
    {
        LOG_WARNING(libcomp::String("Loaded %1/%2 map spot definition files.\n")
            .Arg(mSpotData.size()).Arg(spotLoadCount));
    }
    else
    {
        LOG_DEBUG(libcomp::String("Loaded %1/%2 map spot definition files.\n")
            .Arg(spotLoadCount).Arg(spotLoadCount));
    }

    return success;
}

bool DefinitionManager::LoadEnchantData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiEnchantData>> records;
    bool success = LoadBinaryData<objects::MiEnchantData>(pDataStore,
        "Shield/EnchantData.sbin", true, 0, records);
    for(auto record : records)
    {
        int16_t id = record->GetID();
        uint32_t demonID = record->GetDevilCrystal()->GetDemonID();
        uint32_t itemID = record->GetDevilCrystal()->GetItemID();

        mEnchantData[id] = record;
        mEnchantDemonLookup[demonID] = id;
        mEnchantItemLookup[itemID] = id;
    }
    
    return success;
}

bool DefinitionManager::LoadEquipmentSetData(
    gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiEquipmentSetData>> records;
    bool success = LoadBinaryData<objects::MiEquipmentSetData>(pDataStore,
        "Shield/EquipmentSetData.sbin", true, 0, records);
    for(auto record : records)
    {
        bool equipmentFound = false;
        for(uint32_t equipmentID : record->GetEquipment())
        {
            if(equipmentID)
            {
                mEquipmentSetLookup[equipmentID].push_back(record->GetID());
                equipmentFound = true;
            }
        }

        if(equipmentFound)
        {
            mEquipmentSetData[record->GetID()] = record;
        }
    }

    return success;
}

bool DefinitionManager::LoadExpertClassData(
    gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiExpertData>> records;
    bool success = LoadBinaryData<objects::MiExpertData>(pDataStore,
        "Shield/ExpertClassData.sbin", true, 0, records);
    for(auto record : records)
    {
        mExpertData[record->GetID()] = record;

        if(!record->GetIsChain())
        {
            auto class0Data = record->GetClassData(0);
            auto rank0Data = class0Data->GetRankData(0);
            for(uint32_t i = 0; i < rank0Data->GetSkillCount(); i++)
            {
                mDefaultCharacterSkills.push_back(
                    rank0Data->GetSkill((size_t)i));
            }
        }
    }

    return success;
}

bool DefinitionManager::LoadHNPCData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiHNPCData>> records;
    bool success = LoadBinaryData<objects::MiHNPCData>(pDataStore,
        "Shield/hNPCData.sbin", true, 0, records);
    for(auto record : records)
    {
        mHNPCData[record->GetBasic()->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadItemData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiItemData>> records;
    bool success = LoadBinaryData<objects::MiItemData>(pDataStore,
        "Shield/ItemData.sbin", true, 2, records);
    for(auto record : records)
    {
        mItemData[record->GetCommon()->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadModificationData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiModificationData>> records;
    bool success = LoadBinaryData<objects::MiModificationData>(pDataStore,
        "Shield/ModificationData.sbin", true, 0, records);
    for(auto record : records)
    {
        uint32_t id = record->GetID();
        mModificationData[id] = record;
        mModificationLookup[record->GetItemID()] = id;
    }
    
    return success;
}

bool DefinitionManager::LoadModificationExtEffectData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiModificationExtEffectData>> records;
    bool success = LoadBinaryData<objects::MiModificationExtEffectData>(pDataStore,
        "Shield/ModificationExtEffectData.sbin", true, 0, records);
    for(auto record : records)
    {
        mModificationExtEffectData[record->GetGroupID()]
            [record->GetSlot()][record->GetSubID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadModificationExtRecipeData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiModificationExtRecipeData>> records;
    bool success = LoadBinaryData<objects::MiModificationExtRecipeData>(pDataStore,
        "Shield/ModificationExtRecipeData.sbin", true, 0, records);
    for(auto record : records)
    {
        uint32_t itemID = record->GetItemID();
        if(itemID == static_cast<uint32_t>(-1)) continue;

        uint32_t id = record->GetID();
        mModificationExtRecipeData[id] = record;
        mModificationExtRecipeLookup[itemID] = id;
    }
    
    return success;
}

bool DefinitionManager::LoadModificationTriggerData(gsl::not_null<
    DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiModificationTriggerData>> records;
    bool success = LoadBinaryData<objects::MiModificationTriggerData>(pDataStore,
        "Shield/ModificationTriggerData.sbin", true, 0, records);
    for(auto record : records)
    {
        mModificationTriggerData[record->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadModifiedEffectData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiModifiedEffectData>> records;
    bool success = LoadBinaryData<objects::MiModifiedEffectData>(pDataStore,
        "Shield/ModifiedEffectData.sbin", true, 0, records);
    for(auto record : records)
    {
        mModifiedEffectData[record->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadNPCBarterData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiNPCBarterData>> records;
    bool success = LoadBinaryData<objects::MiNPCBarterData>(pDataStore,
        "Shield/NPCBarterData.sbin", true, 0, records);
    for(auto record : records)
    {
        mNPCBarterData[record->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadONPCData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiONPCData>> records;
    bool success = LoadBinaryData<objects::MiONPCData>(pDataStore,
        "Shield/oNPCData.sbin", true, 0, records);
    for(auto record : records)
    {
        mONPCData[record->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadQuestData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiQuestData>> records;
    bool success = LoadBinaryData<objects::MiQuestData>(pDataStore,
        "Shield/QuestData.sbin", true, 0, records);
    for(auto record : records)
    {
        mQuestData[record->GetID()] = record;
    }

    return success;
}

bool DefinitionManager::LoadShopProductData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiShopProductData>> records;
    bool success = LoadBinaryData<objects::MiShopProductData>(pDataStore,
        "Shield/ShopProductData.sbin", true, 0, records);
    for(auto record : records)
    {
        mShopProductData[record->GetID()] = record;
    }

    return success;
}

bool DefinitionManager::LoadSItemData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiSItemData>> records;
    bool success = LoadBinaryData<objects::MiSItemData>(pDataStore,
        "Shield/SItemData.sbin", true, 0, records);
    for(auto record : records)
    {
        // Only store it if a tokusei is set
        for(auto tokuseiID : record->GetTokusei())
        {
            if(tokuseiID != 0)
            {
                mSItemData[record->GetID()] = record;
                break;
            }
        }
    }

    return success;
}

bool DefinitionManager::LoadSkillData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiSkillData>> records;
    bool success = LoadBinaryData<objects::MiSkillData>(pDataStore,
        "Shield/SkillData.sbin", true, 4, records);
    for(auto record : records)
    {
        mSkillData[record->GetCommon()->GetID()] = record;
    }

    return success;
}

bool DefinitionManager::LoadStatusData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiStatusData>> records;
    bool success = LoadBinaryData<objects::MiStatusData>(pDataStore,
        "Shield/StatusData.sbin", true, 1, records);
    for(auto record : records)
    {
        mStatusData[record->GetCommon()->GetID()] = record;
    }
    
    return success;
}

bool DefinitionManager::LoadTriUnionSpecialData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiTriUnionSpecialData>> records;
    bool success = LoadBinaryData<objects::MiTriUnionSpecialData>(pDataStore,
        "Shield/TriUnionSpecialData.sbin", true, 0, records);
    for(auto record : records)
    {
        auto id = record->GetID();
        mTriUnionSpecialData[id] = record;

        for(auto sourceID : { record->GetSourceID1(), record->GetSourceID2(),
            record->GetSourceID3() })
        {
            if(sourceID)
            {
                mTriUnionSpecialDataBySourceID[sourceID].push_back(id);
            }
        }
    }
    
    return success;
}

bool DefinitionManager::LoadZoneData(gsl::not_null<DataStore*> pDataStore)
{
    std::list<std::shared_ptr<objects::MiZoneData>> records;
    bool success = LoadBinaryData<objects::MiZoneData>(pDataStore,
        "Shield/ZoneData.sbin", true, 0, records);
    for(auto record : records)
    {
        mZoneData[record->GetBasic()->GetID()] = record;
    }
    
    return success;
}

std::shared_ptr<objects::QmpFile> DefinitionManager::LoadQmpFile(
    const libcomp::String& fileName, gsl::not_null<DataStore*> pDataStore)
{
    auto path = libcomp::String("/Map/Zone/Model/") + fileName;

    std::vector<char> data = pDataStore->ReadFile(path);
    if(data.empty())
    {
        return nullptr;
    }

    std::stringstream ss(std::string(data.begin(), data.end()));

    uint32_t magic;
    ss.read(reinterpret_cast<char*>(&magic), sizeof(magic));

    if(magic != 0x3F800000)
    {
        return nullptr;
    }

    auto file = std::make_shared<objects::QmpFile>();
    if(!file->Load(ss))
    {
        return nullptr;
    }

    return file;
}

namespace libcomp
{
    template<>
    bool DefinitionManager::RegisterServerSideDefinition<objects::EnchantSetData>(
        const std::shared_ptr<objects::EnchantSetData>& record)
    {
        uint32_t id = record->GetID();
        if(mEnchantSetData.find(id) != mEnchantSetData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate enchant set encountered: %1\n").Arg(id));
            return false;
        }

        mEnchantSetData[id] = record;

        for(int16_t effectID : record->GetEffects())
        {
            if(effectID != 0)
            {
                mEnchantSetLookup[effectID].push_back(id);
            }
        }

        return true;
    }

    template<>
    bool DefinitionManager::RegisterServerSideDefinition<objects::EnchantSpecialData>(
        const std::shared_ptr<objects::EnchantSpecialData>& record)
    {
        uint32_t id = record->GetID();
        if(mEnchantSpecialData.find(id) != mEnchantSpecialData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate enchant special encountered: %1\n").Arg(id));
            return false;
        }

        mEnchantSpecialData[id] = record;
        mEnchantSpecialLookup[record->GetInputItem()].push_back(id);

        return true;
    }

    template<>
    bool DefinitionManager::RegisterServerSideDefinition<objects::Tokusei>(
        const std::shared_ptr<objects::Tokusei>& record)
    {
        int32_t id = record->GetID();
        if(mTokuseiData.find(id) != mTokuseiData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate tokusei encountered: %1\n").Arg(id));
            return false;
        }

        mTokuseiData[id] = record;

        return true;
    }
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
