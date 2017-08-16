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
#include "MiCorrectTbl.h"
#include "Object.h"

// Standard C++11 Includes
#include <unordered_map>

namespace objects
{
class MiCItemData;
class MiCZoneRelationData;
class MiDevilData;
class MiDevilLVUpRateData;
class MiDynamicMapData;
class MiExpertData;
class MiHNPCData;
class MiItemData;
class MiONPCData;
class MiQuestData;
class MiShopProductData;
class MiSkillData;
class MiStatusData;
class MiTriUnionSpecialData;
class MiZoneData;
}

namespace libcomp
{

/**
 * Manager class responsible for loading binary files that are accessible
 * client side to use as server definitions.
 */
class DefinitionManager
{
public:
    /**
     * Create a new DefinitionManager.
     */
    DefinitionManager();

    /**
     * Clean up the DefinitionManager.
     */
    ~DefinitionManager();

    /**
     * Get the devil definition corresponding to an ID
     * @param id Devil ID to retrieve
     * @return Pointer to the matching devil definition, null if it does not exist
     */
    const std::shared_ptr<objects::MiDevilData> GetDevilData(uint32_t id);

    /**
     * Get a devil definition corresponding to a name
     * @param name Devil name to retrieve
     * @return Pointer to the matching devil definition, null if it does not exist
     */
    const std::shared_ptr<objects::MiDevilData> GetDevilData(const libcomp::String& name);

    /**
     * Get the devil level up information corresponding to an ID
     * @param id Devil level up information ID to retrieve
     * @return Pointer to the matching devil level up information, null if it
     *  does not exist
     */
    const std::shared_ptr<objects::MiDevilLVUpRateData> GetDevilLVUpRateData(uint32_t id);

    /**
     * Get the dynamic map information corresponding to an ID
     * @param id Dynamic map information ID to retrieve
     * @return Pointer to the matching dynamic map information, null if it
     *  does not exist
     */
    const std::shared_ptr<objects::MiDynamicMapData> GetDynamicMapData(uint32_t id);

    /**
     * Get the character expertise information corresponding to an ID
     * @param id Expertise ID to retrieve
     * @return Pointer to the matching character expertise information, null
     *  if it does not exist
     */
    const std::shared_ptr<objects::MiExpertData> GetExpertClassData(uint32_t id);

    /**
     * Get the list of fusion range level and demon ID pairs associated to
     * the supplied demon race
     * @param raceID Demon race ID
     * @return List of maximum fusion range level (key) and demon ID (value) pairs
     */
    const std::list<std::pair<uint8_t, uint32_t>> GetFusionRanges(uint8_t raceID);

    /**
     * Get the human NPC definition corresponding to an ID
     * @param id Human NPC ID to retrieve
     * @return Pointer to the matching human NPC definition, null if it does
     *  not exist
     */
    const std::shared_ptr<objects::MiHNPCData> GetHNPCData(uint32_t id);

    /**
     * Get the item definition corresponding to an ID
     * @param id Item ID to retrieve
     * @return Pointer to the matching item definition, null if it does not exist
     */
    const std::shared_ptr<objects::MiItemData> GetItemData(uint32_t id);

    /**
     * Get the item definition corresponding to a name
     * @param name Item name to retrieve
     * @return Pointer to the matching item definition, null if it does not exist
     */
    const std::shared_ptr<objects::MiItemData> GetItemData(const libcomp::String& name);

    /**
     * Get the server object NPC definition corresponding to an ID
     * @param id Server object NPC ID to retrieve
     * @return Pointer to the matching server object NPC definition, null if it
     *  does not exist
     */
    const std::shared_ptr<objects::MiONPCData> GetONPCData(uint32_t id);

    /**
     * Get the quest definition corresponding to an ID
     * @param id Quest ID to retrieve
     * @return Pointer to the matching quest, null if it does not exist
     */
    const std::shared_ptr<objects::MiQuestData> GetQuestData(uint32_t id);

    /**
     * Get the shop product definition corresponding to an ID
     * @param id Shop product ID to retrieve
     * @return Pointer to the matching shop product definition, null if it does not exist
     */
    const std::shared_ptr<objects::MiShopProductData> GetShopProductData(uint32_t id);

    /**
     * Get the skill definition corresponding to an ID
     * @param id Skill ID to retrieve
     * @return Pointer to the matching skill definition, null if it does not exist
     */
    const std::shared_ptr<objects::MiSkillData> GetSkillData(uint32_t id);

    /**
     * Get the status definition corresponding to an ID
     * @param id Status ID to retrieve
     * @return Pointer to the matching skill definition, null if it does not exist
     */
    const std::shared_ptr<objects::MiStatusData> GetStatusData(uint32_t id);

    /**
     * Get the list of pointers to special fusion definitions by the ID of a source
     * demon involved
     * @param sourceDemonTypeID ID of a source demon for the special fusion
     * @return List of pointers to special fusion definitions
     */
    const std::list<std::shared_ptr<objects::MiTriUnionSpecialData>>
        GetTriUnionSpecialData(uint32_t sourceDemonTypeID);

    /**
     * Get the zone definition corresponding to an ID
     * @param id Zone ID to retrieve
     * @return Pointer to the matching zone definition, null if it does not exist
     */
    const std::shared_ptr<objects::MiZoneData> GetZoneData(uint32_t id);

    /**
     * Get the zone relation information corresponding to an ID
     * @param id Zone relation information ID to retrieve
     * @return Pointer to the matching zone relation information, null if it does
     *  not exist
     */
    const std::shared_ptr<objects::MiCZoneRelationData> GetZoneRelationData(uint32_t id);

    /**
     * Get the default skills to add to a newly created character
     * @return List of skill IDs to add to a newly created character
     */
    const std::list<uint32_t> GetDefaultCharacterSkills();

    /**
     * Load all binary data definitions
     * @param pDataStore Pointer to the datastore to load binary files from
     * @return true on success, false on failure
     */
    bool LoadAllData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the client item binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadCItemData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the client zone relation binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadCZoneRelationData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the devil binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadDevilData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the devil level information binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadDevilLVUpRateData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the dynamic map information binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadDynamicMapData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the character expertise binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadExpertClassData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the human NPC binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadHNPCData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the item binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadItemData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the server object NPC binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadONPCData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the quest binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadQuestData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the shop product binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadShopProductData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the skill binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadSkillData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the status binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadStatusData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the special fusion binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadTriUnionSpecialData(gsl::not_null<DataStore*> pDataStore);

    /**
     * Load the zone binary data definitions
     * @param pDataStore Pointer to the datastore to load binary file from
     * @return true on success, false on failure
     */
    bool LoadZoneData(gsl::not_null<DataStore*> pDataStore);

private:
    /**
     * Load a binary file from the specified data store location
     * @param pDataStore Pointer to a data store location to check
     *  for the file
     * @param binaryFile Relative file path to a binary file
     * @param decrypt true if the file is encrypted and must be
     *  decrypted first, false if it can just be loaded
     * @param tablesExpected Number of tables expected in the file
     *  format
     * @param records Output list to load records into
     * @return true if the file was loaded, false if it was not
     */
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

    /**
     * Load the data header containing the number of entries and
     * tables that make up the format of the rest of the file
     * @param ois Binary stream of the file's contents
     * @param binaryFile Relative file path to a binary file
     * @param tablesExpected Number of tables expected in the file
     *  format.  If this number does not match the tableCount result
     *  an error will be printed and will result in failure.
     * @param entryCount Output param containing the number of
     *  individual records contained in the file
     * @param tableCount Output param containing the number of
     *  dynamically sized "table" data members for each entry
     * @return true if the data header was read, false if it failed
     */
    bool LoadBinaryDataHeader(libcomp::ObjectInStream& ois,
        const libcomp::String& binaryFile, uint16_t tablesExpected,
        uint16_t& entryCount, uint16_t& tableCount);

    /**
     * Utility function to print the result of loading a binary file
     * @param binaryFile Relative file path to a binary file
     * @param success true if the load succeeded, false if it failed
     * @param entriesExpected Number of entries that were supposed to
     *  be in the file according to the data header
     * @param loadedEntries NUmber of entries successfully loaded
     */
    void PrintLoadResult(const libcomp::String& binaryFile, bool success,
        uint16_t entriesExpected, size_t loadedEntries);

    /**
     * Utility function to pull the templated type from a standard
     * definition map of ID to a pointer of that type
     * @param id ID of the definition to retrieve from the map
     * @param data Map to retrieve the data from
     * @return Pointer to the record in the map matching the
     *  supplied ID, null if it does not exist
     */
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

    /// List of default skill IDs characters are created with
    std::list<uint32_t> mDefaultCharacterSkills;

    /// Map of item names to IDs
    std::unordered_map<libcomp::String, uint32_t> mCItemNameLookup;

    /// Map of devil definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiDevilData>> mDevilData;

    /// Map of devil names to IDs which are NOT unique across entries
    std::unordered_map<libcomp::String, uint32_t> mDevilNameLookup;

    /// Map of devil level up information by devil ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiDevilLVUpRateData>> mDevilLVUpRateData;

    /// Map of dynamic map information by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiDynamicMapData>> mDynamicMapData;

    /// Map of character expertise information by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiExpertData>> mExpertData;

    /// Map of demon race IDs to maximum fusion range levels paired
    /// with their corresponding result demon
    std::unordered_map<uint8_t,
        std::list<std::pair<uint8_t, uint32_t>>> mFusionRanges;

    /// Map of human NPC definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiHNPCData>> mHNPCData;

    /// Map of item definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiItemData>> mItemData;

    /// Map of server object NPC definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiONPCData>> mONPCData;

    /// Map of quest definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiQuestData>> mQuestData;

    /// Map of shop product definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiShopProductData>> mShopProductData;

    /// Map of skill definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiSkillData>> mSkillData;

    /// Map of status definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiStatusData>> mStatusData;

    /// Map of special fusion definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiTriUnionSpecialData>> mTriUnionSpecialData;

    /// Map of source demon IDs to special fusions they belong to
    std::unordered_map<uint32_t,
        std::list<uint32_t>> mTriUnionSpecialDataBySourceID;

    /// Map of zone definitions by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiZoneData>> mZoneData;

    /// Map of zone relational information by ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::MiCZoneRelationData>> mZoneRelationData;
};

} // namspace libcomp

#endif // LIBCOMP_SRC_DEFINITIONMANAGER_H
