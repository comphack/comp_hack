/**
 * @file libcomp/src/ServerDataManager.cpp
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Manages loading and storing server data objects.
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

#include "ServerDataManager.h"

// libcomp Includes
#include "DefinitionManager.h"
#include "Log.h"
#include "ScriptEngine.h"

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <DemonPresent.h>
#include <DemonQuestReward.h>
#include <DropSet.h>
#include <EnchantSetData.h>
#include <EnchantSpecialData.h>
#include <Event.h>
#include <MiSStatusData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <PvPInstanceVariant.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerShop.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <ServerZoneInstanceVariant.h>
#include <ServerZonePartial.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocationGroup.h>
#include <Tokusei.h>

using namespace libcomp;

ServerDataManager::ServerDataManager()
{
}

ServerDataManager::~ServerDataManager()
{
}

const std::shared_ptr<objects::ServerZone> ServerDataManager::GetZoneData(
    uint32_t id, uint32_t dynamicMapID, bool applyPartials,
    std::set<uint32_t> extraPartialIDs)
{
    std::shared_ptr<objects::ServerZone> zone;

    auto iter = mZoneData.find(id);
    if(iter != mZoneData.end())
    {
        if(dynamicMapID != 0)
        {
            auto dIter = iter->second.find(dynamicMapID);
            zone = (dIter != iter->second.end()) ? dIter->second : nullptr;
        }
        else
        {
            // Return first
            zone = iter->second.begin()->second;
        }
    }

    if(applyPartials && zone)
    {
        std::set<uint32_t> partialIDs;

        // Gather all auto-applied partials
        auto partialIter = mZonePartialMap.find(zone->GetDynamicMapID());
        if(partialIter != mZonePartialMap.end())
        {
            partialIDs = partialIter->second;
        }

        // Gather and verify all extra partials
        for(uint32_t partialID : extraPartialIDs)
        {
            auto partial = GetZonePartialData(partialID);
            if(partial && !partial->GetAutoApply() &&
                (partial->DynamicMapIDsCount() == 0 ||
                partial->DynamicMapIDsContains(zone->GetDynamicMapID())))
            {
                partialIDs.insert(partialID);
            }
        }

        if(partialIDs.size() > 0)
        {
            // Copy the definition and apply changes
            libcomp::String zoneStr = libcomp::String("%1%2")
                .Arg(id).Arg(id != dynamicMapID ? libcomp::String(" (%1)")
                    .Arg(dynamicMapID) : "");

            zone = std::make_shared<objects::ServerZone>(*zone);
            for(uint32_t partialID : partialIDs)
            {
                if(!ApplyZonePartial(zone, partialID))
                {
                    // Errored, no zone should be returned
                    return nullptr;
                }
            }

            // Now validate spawn information and correct as needed
            std::set<uint32_t> sgRemoves;
            for(auto sgPair : zone->GetSpawnGroups())
            {
                std::set<uint32_t> missingSpawns;
                for(auto sPair : sgPair.second->GetSpawns())
                {
                    if(!zone->SpawnsKeyExists(sPair.first))
                    {
                        missingSpawns.insert(sPair.first);
                    }
                }

                if(missingSpawns.size() > 0)
                {
                    if(missingSpawns.size() < sgPair.second->SpawnsCount())
                    {
                        // Copy the group and edit the spawns
                        auto sg = std::make_shared<objects::SpawnGroup>(
                            *sgPair.second);
                        for(uint32_t remove : sgRemoves)
                        {
                            sg->RemoveSpawns(remove);
                        }

                        zone->SetSpawnGroups(sgPair.first, sg);
                    }
                    else
                    {
                        sgRemoves.insert(sgPair.first);
                    }
                }
            }

            for(uint32_t sgRemove : sgRemoves)
            {
                LOG_DEBUG(libcomp::String("Removing empty spawn group %1"
                    " when generating zone: %2\n").Arg(sgRemove)
                    .Arg(zoneStr));
                zone->RemoveSpawnGroups(sgRemove);
            }

            std::set<uint32_t> slgRemoves;
            for(auto slgPair : zone->GetSpawnLocationGroups())
            {
                std::set<uint32_t> missingGroups;
                for(uint32_t sgID : slgPair.second->GetGroupIDs())
                {
                    if(!zone->SpawnGroupsKeyExists(sgID))
                    {
                        missingGroups.insert(sgID);
                    }
                }

                if(missingGroups.size() > 0)
                {
                    if(missingGroups.size() < slgPair.second->GroupIDsCount())
                    {
                        // Copy the group and edit the spawns
                        auto slg = std::make_shared<objects::SpawnLocationGroup>(
                            *slgPair.second);
                        for(uint32_t remove : sgRemoves)
                        {
                            slg->RemoveGroupIDs(remove);
                        }

                        zone->SetSpawnLocationGroups(slgPair.first, slg);
                    }
                    else
                    {
                        slgRemoves.insert(slgPair.first);
                    }
                }
            }

            for(uint32_t slgRemove : slgRemoves)
            {
                LOG_DEBUG(libcomp::String("Removing empty spawn location group"
                    " %1 when generating zone: %2\n").Arg(slgRemove)
                    .Arg(zoneStr));
                zone->RemoveSpawnLocationGroups(slgRemove);
            }
        }
    }

    return zone;
}

const std::list<std::pair<uint32_t, uint32_t>> ServerDataManager::GetFieldZoneIDs()
{
    return mFieldZoneIDs;
}

const std::unordered_map<uint32_t, std::set<uint32_t>> ServerDataManager::GetAllZoneIDs()
{
    std::unordered_map<uint32_t, std::set<uint32_t>> zoneIDs;
    for(auto pair : mZoneData)
    {
        for(auto dPair : pair.second)
        {
            zoneIDs[pair.first].insert(dPair.first);
        }
    }

    return zoneIDs;
}

const std::shared_ptr<objects::ServerZoneInstance> ServerDataManager::GetZoneInstanceData(
    uint32_t id)
{
    return GetObjectByID<uint32_t, objects::ServerZoneInstance>(id, mZoneInstanceData);
}

const std::set<uint32_t> ServerDataManager::GetAllZoneInstanceIDs()
{
    std::set<uint32_t> instanceIDs;
    for(auto pair : mZoneInstanceData)
    {
        instanceIDs.insert(pair.first);
    }

    return instanceIDs;
}

const std::shared_ptr<objects::ServerZoneInstanceVariant>
    ServerDataManager::GetZoneInstanceVariantData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::ServerZoneInstanceVariant>(id,
        mZoneInstanceVariantData);
}

std::set<uint32_t> ServerDataManager::GetStandardPvPVariantIDs(
    uint8_t type) const
{
    auto it = mStandardPvPVariantIDs.find(type);
    return it != mStandardPvPVariantIDs.end()
        ? it->second : std::set<uint32_t>();
}

bool ServerDataManager::VerifyPvPInstance(uint32_t instanceID,
    DefinitionManager* definitionManager)
{
    auto instanceDef = GetZoneInstanceData(instanceID);
    if(instanceDef && definitionManager)
    {
        for(uint32_t zoneID : instanceDef->GetZoneIDs())
        {
            auto zoneDef = definitionManager->GetZoneData(zoneID);
            if(!zoneDef || zoneDef->GetBasic()->GetType() != 7)
            {
                LOG_ERROR(libcomp::String("Instance contains non-PvP zones"
                    " and cannot be used for PvP: %1\n").Arg(instanceID));
                return false;
            }
        }

        return true;
    }

    return false;
}

const std::shared_ptr<objects::ServerZonePartial>
    ServerDataManager::GetZonePartialData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::ServerZonePartial>(id, mZonePartialData);
}

const std::shared_ptr<objects::Event> ServerDataManager::GetEventData(const libcomp::String& id)
{
    return GetObjectByID<std::string, objects::Event>(id.C(), mEventData);
}

const std::shared_ptr<objects::ServerShop> ServerDataManager::GetShopData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::ServerShop>(id, mShopData);
}

std::list<uint32_t> ServerDataManager::GetCompShopIDs() const
{
    return mCompShopIDs;
}

const std::shared_ptr<objects::DemonPresent> ServerDataManager::GetDemonPresentData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::DemonPresent>(id, mDemonPresentData);
}

std::unordered_map<uint32_t,
    std::shared_ptr<objects::DemonQuestReward>> ServerDataManager::GetDemonQuestRewardData()
{
    return mDemonQuestRewardData;
}

const std::shared_ptr<objects::DropSet> ServerDataManager::GetDropSetData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::DropSet>(id, mDropSetData);
}

const std::shared_ptr<objects::DropSet> ServerDataManager::GetGiftDropSetData(
    uint32_t giftBoxID)
{
    auto it = mGiftDropSetLookup.find(giftBoxID);
    return it != mGiftDropSetLookup.end() ? GetDropSetData(it->second) : nullptr;
}

const std::shared_ptr<ServerScript> ServerDataManager::GetScript(const libcomp::String& name)
{
    return GetObjectByID<std::string, ServerScript>(name.C(), mScripts);
}

const std::shared_ptr<ServerScript> ServerDataManager::GetAIScript(const libcomp::String& name)
{
    return GetObjectByID<std::string, ServerScript>(name.C(), mAIScripts);
}

bool ServerDataManager::LoadData(DataStore *pDataStore,
    DefinitionManager* definitionManager)
{
    bool failure = false;

    if(definitionManager)
    {
        if(!failure)
        {
            LOG_DEBUG("Loading demon present server definitions...\n");
            failure = !LoadObjectsFromFile<objects::DemonPresent>(
                pDataStore, "/data/demonpresent.xml", definitionManager);
        }

        if(!failure)
        {
            LOG_DEBUG("Loading demon quest reward server definitions...\n");
            failure = !LoadObjectsFromFile<objects::DemonQuestReward>(
                pDataStore, "/data/demonquestreward.xml", definitionManager);
        }

        if(!failure)
        {
            LOG_DEBUG("Loading drop set server definitions...\n");
            failure = !LoadObjectsFromFile<objects::DropSet>(
                pDataStore, "/data/dropset.xml", definitionManager);
        }

        if(!failure)
        {
            LOG_DEBUG("Loading enchant set server definitions...\n");
            failure = !LoadObjectsFromFile<objects::EnchantSetData>(
                pDataStore, "/data/enchantset.xml", definitionManager);
        }

        if(!failure)
        {
            LOG_DEBUG("Loading enchant special server definitions...\n");
            failure = !LoadObjectsFromFile<objects::EnchantSpecialData>(
                pDataStore, "/data/enchantspecial.xml", definitionManager);
        }

        if(!failure)
        {
            LOG_DEBUG("Loading s-status server definitions...\n");
            failure = !LoadObjectsFromFile<objects::MiSStatusData>(
                pDataStore, "/data/sstatus.xml", definitionManager);
        }

        if(!failure)
        {
            LOG_DEBUG("Loading tokusei server definitions...\n");
            failure = !LoadObjects<objects::Tokusei>(pDataStore,
                "/tokusei", definitionManager, true);
        }
    }

    if(!failure)
    {
        LOG_DEBUG("Loading zone server definitions...\n");
        failure = !LoadObjects<objects::ServerZone>(pDataStore, "/zones",
            definitionManager, false);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading zone partial server definitions...\n");
        failure = !LoadObjects<objects::ServerZonePartial>(pDataStore,
            "/zones/partial", definitionManager, true);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading event server definitions...\n");
        failure = !LoadObjects<objects::Event>(pDataStore, "/events",
            definitionManager, true);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading zone instance server definitions...\n");
        failure = !LoadObjectsFromFile<objects::ServerZoneInstance>(
            pDataStore, "/data/zoneinstance.xml", definitionManager);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading zone instance variant server definitions...\n");
        failure = !LoadObjectsFromFile<objects::ServerZoneInstanceVariant>(
            pDataStore, "/data/zoneinstancevariant.xml", definitionManager);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading shop server definitions...\n");
        failure = !LoadObjects<objects::ServerShop>(pDataStore, "/shops",
            definitionManager, true);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading server scripts...\n");
        failure = !LoadScripts(pDataStore, "/scripts", &ServerDataManager::LoadScript);
    }

    return !failure;
}

bool ServerDataManager::ApplyZonePartial(std::shared_ptr<objects::ServerZone> zone,
    uint32_t partialID)
{
    if(!zone || !partialID)
    {
        return false;
    }

    uint32_t id = zone->GetID();
    uint32_t dynamicMapID = zone->GetDynamicMapID();

    auto originDef = GetZoneData(id, dynamicMapID, false);
    if(originDef == zone)
    {
        LOG_ERROR(libcomp::String("Attempted to apply partial definition to"
            " original zone definition: %1%2\n").Arg(id).Arg(id != dynamicMapID
                ? libcomp::String(" (%1)").Arg(dynamicMapID) : ""));
        return false;
    }

    auto partial = GetZonePartialData(partialID);
    if(!partial)
    {
        LOG_ERROR(libcomp::String("Invalid zone partial ID encountered: %1\n")
            .Arg(partialID));
        return false;
    }

    // Add dropsets
    for(uint32_t dropSetID : partial->GetDropSetIDs())
    {
        zone->InsertDropSetIDs(dropSetID);
    }

    // Build new NPC set
    auto npcs = zone->GetNPCs();
    for(auto& npc : partial->GetNPCs())
    {
        // Remove any NPCs that share the same spot ID or are within
        // 10 units from the new one (X or Y)
        npcs.remove_if([npc](
            const std::shared_ptr<objects::ServerNPC>& oNPC)
            {
                return (npc->GetSpotID() &&
                    oNPC->GetSpotID() == npc->GetSpotID()) ||
                    (!npc->GetSpotID() && !oNPC->GetSpotID() &&
                        fabs(oNPC->GetX() - npc->GetX()) < 10.f &&
                        fabs(oNPC->GetY() - npc->GetY()) < 10.f);
            });

        // Removes supported via 0 ID
        if(npc->GetID())
        {
            npcs.push_back(npc);
        }
    }
    zone->SetNPCs(npcs);

    // Build new object set
    auto objects = zone->GetObjects();
    for(auto& obj : partial->GetObjects())
    {
        // Remove any objects that share the same spot ID or are within
        // 10 units from the new one (X and Y)
        objects.remove_if([obj](
            const std::shared_ptr<objects::ServerObject>& oObj)
            {
                return (obj->GetSpotID() &&
                    oObj->GetSpotID() == obj->GetSpotID()) ||
                    (!obj->GetSpotID() && !oObj->GetSpotID() &&
                        fabs(oObj->GetX() - obj->GetX()) < 10.f &&
                        fabs(oObj->GetY() - obj->GetY()) < 10.f);
            });

        // Removes supported via 0 ID
        if(obj->GetID())
        {
            objects.push_back(obj);
        }
    }
    zone->SetObjects(objects);

    // Update spawns
    for(auto& sPair : partial->GetSpawns())
    {
        zone->SetSpawns(sPair.first, sPair.second);
    }

    // Update spawn groups
    for(auto& sgPair : partial->GetSpawnGroups())
    {
        zone->SetSpawnGroups(sgPair.first, sgPair.second);
    }

    // Update spawn location groups
    for(auto& slgPair : partial->GetSpawnLocationGroups())
    {
        zone->SetSpawnLocationGroups(slgPair.first, slgPair.second);
    }

    // Update spots
    for(auto& spotPair : partial->GetSpots())
    {
        zone->SetSpots(spotPair.first, spotPair.second);
    }

    // Add triggers
    for(auto& trigger : partial->GetTriggers())
    {
        zone->AppendTriggers(trigger);
    }

    return true;
}

bool ServerDataManager::LoadScripts(gsl::not_null<DataStore*> pDataStore,
    const libcomp::String& datastorePath,
    std::function<bool(ServerDataManager&,
        const libcomp::String&, const libcomp::String&)> handler)
{
    std::list<libcomp::String> files;
    std::list<libcomp::String> dirs;
    std::list<libcomp::String> symLinks;

    (void)pDataStore->GetListing(datastorePath, files, dirs, symLinks,
        true, true);

    for (auto path : files)
    {
        if (path.Matches("^.*\\.nut$"))
        {
            std::vector<char> data = pDataStore->ReadFile(path);
            if(!handler(*this, path, std::string(data.begin(), data.end())))
            {
                LOG_ERROR(libcomp::String("Failed to load script file: %1\n").Arg(path));
                return false;
            }

            LOG_DEBUG(libcomp::String("Loaded script file: %1\n").Arg(path));
        }
    }


    return true;
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<ServerScript>()
    {
        if(!BindingExists("ServerScript", true))
        {
            Sqrat::Class<ServerScript> binding(mVM, "ServerScript");
            binding.Var("Name", &ServerScript::Name);
            binding.Var("Type", &ServerScript::Type);
            Bind<ServerScript>("ServerScript", binding);
        }

        return *this;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::ServerZone>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        auto zone = std::shared_ptr<objects::ServerZone>(new objects::ServerZone);
        if(!zone->Load(doc, *objNode))
        {
            return false;
        }

        auto id = zone->GetID();
        auto dynamicMapID = zone->GetDynamicMapID();

        libcomp::String zoneStr = libcomp::String("%1%2")
            .Arg(id).Arg(id != dynamicMapID ? libcomp::String(" (%1)")
                .Arg(dynamicMapID) : "");

        bool isField = false;
        if(definitionManager)
        {
            auto def = definitionManager->GetZoneData(id);
            if(!def)
            {
                LOG_WARNING(libcomp::String("Skipping unknown zone: %1\n")
                    .Arg(zoneStr));
                return true;
            }

            isField = def->GetBasic()->GetType() == 2;
        }

        if(mZoneData.find(id) != mZoneData.end() &&
            mZoneData[id].find(dynamicMapID) != mZoneData[id].end())
        {
            LOG_ERROR(libcomp::String("Duplicate zone encountered: %1\n")
                .Arg(zoneStr));
            return false;
        }

        // Make sure spawns are valid
        if(definitionManager)
        {
            for(auto sPair : zone->GetSpawns())
            {
                if(definitionManager->GetDevilData(
                    sPair.second->GetEnemyType()) == nullptr)
                {
                    LOG_ERROR(libcomp::String("Invalid spawn enemy type"
                        " encountered in zone %1: %2\n").Arg(zoneStr)
                        .Arg(sPair.second->GetEnemyType()));
                    return false;
                }
                else if(sPair.second->GetBossGroup() &&
                    sPair.second->GetCategory() !=
                    objects::Spawn::Category_t::BOSS)
                {
                    LOG_ERROR(libcomp::String("Invalid spawn boss group"
                        " encountered in zone %1: %2\n").Arg(zoneStr)
                        .Arg(sPair.first));
                    return false;
                }
            }
        }

        for(auto sgPair : zone->GetSpawnGroups())
        {
            for(auto sPair : sgPair.second->GetSpawns())
            {
                if(!zone->SpawnsKeyExists(sPair.first))
                {
                    LOG_ERROR(libcomp::String("Invalid spawn group spawn ID"
                        " encountered in zone %1: %2\n").Arg(zoneStr)
                        .Arg(sPair.first));
                    return false;
                }
            }
        }

        for(auto slgPair : zone->GetSpawnLocationGroups())
        {
            for(uint32_t sgID : slgPair.second->GetGroupIDs())
            {
                if(!zone->SpawnGroupsKeyExists(sgID))
                {
                    LOG_ERROR(libcomp::String("Invalid spawn location group"
                        " spawn group ID encountered in zone %1: %2\n")
                        .Arg(zoneStr).Arg(sgID));
                    return false;
                }
            }
        }

        mZoneData[id][dynamicMapID] = zone;

        if(isField)
        {
            mFieldZoneIDs.push_back(std::pair<uint32_t, uint32_t>(id,
                dynamicMapID));
        }

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::ServerZonePartial>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        (void)definitionManager;

        auto prt = std::shared_ptr<objects::ServerZonePartial>(new objects::ServerZonePartial);
        if(!prt->Load(doc, *objNode))
        {
            return false;
        }

        auto id = prt->GetID();
        if(mZonePartialData.find(id) != mZonePartialData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate zone partial encountered: %1\n").Arg(id));
            return false;
        }

        if(id == 0)
        {
            if(prt->DynamicMapIDsCount() || prt->NPCsCount() ||
                prt->ObjectsCount() || prt->SpawnsCount() ||
                prt->SpawnGroupsCount() || prt->SpawnLocationGroupsCount() ||
                prt->SpotsCount())
            {
                LOG_WARNING("Direct global partial zone definitions specified"
                    " but will be ignored\n");
            }
        }
        else
        {
            // Make sure spawns are valid
            if(definitionManager)
            {
                for(auto sPair : prt->GetSpawns())
                {
                    if(definitionManager->GetDevilData(
                        sPair.second->GetEnemyType()) == nullptr)
                    {
                        LOG_ERROR(libcomp::String("Invalid spawn enemy type"
                            " encountered in zone partial %1: %2\n").Arg(id)
                            .Arg(sPair.second->GetEnemyType()));
                        return false;
                    }
                    else if(sPair.second->GetBossGroup() &&
                        sPair.second->GetCategory() !=
                        objects::Spawn::Category_t::BOSS)
                    {
                        LOG_ERROR(libcomp::String("Invalid spawn boss group"
                            " encountered in zone paritial %1: %2\n").Arg(id)
                            .Arg(sPair.first));
                        return false;
                    }
                }
            }

            if(prt->GetAutoApply())
            {
                for(uint32_t dynamicMapID : prt->GetDynamicMapIDs())
                {
                    mZonePartialMap[dynamicMapID].insert(id);
                }
            }
        }

        mZonePartialData[id] = prt;

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::Event>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        (void)definitionManager;

        auto event = objects::Event::InheritedConstruction(objNode->Attribute("name"));
        if(event == nullptr || !event->Load(doc, *objNode))
        {
            return false;
        }

        if(event->GetID().IsEmpty())
        {
            LOG_ERROR("Event with no ID encountered\n");
            return false;
        }

        auto id = std::string(event->GetID().C());
        if(mEventData.find(id) != mEventData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate event encountered: %1\n").Arg(id));
            return false;
        }

        mEventData[id] = event;

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::ServerZoneInstance>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        auto inst = std::shared_ptr<objects::ServerZoneInstance>(new objects::ServerZoneInstance);
        if(!inst->Load(doc, *objNode))
        {
            return false;
        }

        auto id = inst->GetID();
        if(definitionManager && !definitionManager->GetZoneData(inst->GetLobbyID()))
        {
            LOG_WARNING(libcomp::String("Skipping zone instance with unknown lobby: %1\n")
                .Arg(inst->GetLobbyID()));
            return true;
        }

        // Zone and dynamic map IDs should be parallel lists
        size_t zoneIDCount = inst->ZoneIDsCount();
        if(zoneIDCount != inst->DynamicMapIDsCount())
        {
            LOG_ERROR(libcomp::String("Zone instance encountered with zone and dynamic"
                " map counts that do not match\n"));
            return false;
        }

        for(size_t i = 0; i < zoneIDCount; i++)
        {
            uint32_t zoneID = inst->GetZoneIDs(i);
            uint32_t dynamicMapID = inst->GetDynamicMapIDs(i);

            if(mZoneData.find(zoneID) == mZoneData.end() ||
                mZoneData[zoneID].find(dynamicMapID) == mZoneData[zoneID].end())
            {
                LOG_ERROR(libcomp::String("Invalid zone encountered for instance: %1 (%2)\n")
                    .Arg(zoneID).Arg(dynamicMapID));
                return false;
            }
        }

        if(mZoneInstanceData.find(id) != mZoneInstanceData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate zone instance encountered: %1\n").Arg(id));
            return false;
        }

        mZoneInstanceData[id] = inst;

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::ServerZoneInstanceVariant>(
        const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement *objNode,
        DefinitionManager* definitionManager)
    {
        (void)definitionManager;

        auto variant = objects::ServerZoneInstanceVariant::InheritedConstruction(
            objNode->Attribute("name"));
        if(!variant->Load(doc, *objNode))
        {
            return false;
        }

        auto id = variant->GetID();
        if(mZoneInstanceVariantData.find(id) != mZoneInstanceVariantData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate zone instance variant"
                " encountered: %1\n").Arg(id));
            return false;
        }

        size_t timeCount = variant->TimePointsCount();
        switch(variant->GetInstanceType())
        {
        case objects::ServerZoneInstanceVariant::InstanceType_t::TIME_TRIAL:
            if(timeCount != 4)
            {
                LOG_ERROR(libcomp::String("Time trial zone instance variant"
                    " encountered without 4 time points specified: %1\n")
                    .Arg(id));
                return false;
            }
            break;
        case objects::ServerZoneInstanceVariant::InstanceType_t::PVP:
            if(timeCount != 2 && timeCount != 3)
            {
                LOG_ERROR(libcomp::String("PVP zone instance variant"
                    " encountered without 2 or 3 time points specified: %1\n")
                    .Arg(id));
                return false;
            }
            break;
        case objects::ServerZoneInstanceVariant::InstanceType_t::DEMON_ONLY:
            if(timeCount != 3 && timeCount != 4)
            {
                LOG_ERROR(libcomp::String("Demon only zone instance variant"
                    " encountered without 3 or 4 time points specified: %1\n")
                    .Arg(id));
                return false;
            }
            break;
        case objects::ServerZoneInstanceVariant::InstanceType_t::DIASPORA:
            if(timeCount != 2)
            {
                LOG_ERROR(libcomp::String("Diaspora zone instance variant"
                    " encountered without 2 time points specified: %1\n")
                    .Arg(id));
                return false;
            }
            break;
        case objects::ServerZoneInstanceVariant::InstanceType_t::MISSION:
            if(timeCount != 1)
            {
                LOG_ERROR(libcomp::String("Mission zone instance variant"
                    " encountered without time point specified: %1\n")
                    .Arg(id));
                return false;
            }
            break;
        case objects::ServerZoneInstanceVariant::InstanceType_t::PENTALPHA:
            if(variant->GetSubID() >= 5)
            {
                LOG_ERROR(libcomp::String("Pentalpha zone instance variant"
                    " encountered with invalid sub ID: %1\n")
                    .Arg(id));
                return false;
            }
            break;
        default:
            break;
        }

        auto pvpVar = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
            variant);
        if(pvpVar)
        {
            if(definitionManager && pvpVar->GetDefaultInstanceID() &&
                !VerifyPvPInstance(pvpVar->GetDefaultInstanceID(),
                    definitionManager))
            {
                return false;
            }

            if(!pvpVar->GetSpecialMode() && pvpVar->GetMatchType() !=
                objects::PvPInstanceVariant::MatchType_t::CUSTOM)
            {
                mStandardPvPVariantIDs[(uint8_t)pvpVar->GetMatchType()]
                    .insert(id);
            }
        }

        mZoneInstanceVariantData[id] = variant;

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::ServerShop>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        (void)definitionManager;

        auto shop = std::shared_ptr<objects::ServerShop>(new objects::ServerShop);
        if(!shop->Load(doc, *objNode))
        {
            return false;
        }

        uint32_t id = (uint32_t)shop->GetShopID();
        if(mShopData.find(id) != mShopData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate shop encountered: %1\n").Arg(id));
            return false;
        }

        mShopData[id] = shop;

        if(shop->GetType() == objects::ServerShop::Type_t::COMP_SHOP)
        {
            mCompShopIDs.push_back(id);
        }

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::DemonPresent>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        (void)definitionManager;

        auto present = std::shared_ptr<objects::DemonPresent>(new objects::DemonPresent);
        if(!present->Load(doc, *objNode))
        {
            return false;
        }

        uint32_t id = present->GetID();
        if(mDemonPresentData.find(id) != mDemonPresentData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate demon present entry encountered: %1\n").Arg(id));
            return false;
        }

        mDemonPresentData[id] = present;

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::DemonQuestReward>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        (void)definitionManager;

        auto reward = std::shared_ptr<objects::DemonQuestReward>(new objects::DemonQuestReward);
        if(!reward->Load(doc, *objNode))
        {
            return false;
        }

        uint32_t id = reward->GetID();
        if(mDemonQuestRewardData.find(id) != mDemonQuestRewardData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate demon quest reward entry encountered: %1\n").Arg(id));
            return false;
        }

        mDemonQuestRewardData[id] = reward;

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::DropSet>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        (void)definitionManager;

        auto dropSet = std::shared_ptr<objects::DropSet>(new objects::DropSet);
        if(!dropSet->Load(doc, *objNode))
        {
            return false;
        }

        uint32_t id = dropSet->GetID();
        uint32_t giftBoxID = dropSet->GetGiftBoxID();
        if(mDropSetData.find(id) != mDropSetData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate drop set encountered: %1\n").Arg(id));
            return false;
        }

        if(giftBoxID)
        {
            if(mGiftDropSetLookup.find(giftBoxID) != mGiftDropSetLookup.end())
            {
                LOG_ERROR(libcomp::String("Duplicate drop set gift box ID"
                    " encountered: %1\n").Arg(giftBoxID));
                return false;
            }

            mGiftDropSetLookup[giftBoxID] = id;
        }

        mDropSetData[id] = dropSet;

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::EnchantSetData>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        auto eSet = std::shared_ptr<objects::EnchantSetData>(new objects::EnchantSetData);
        if(!eSet->Load(doc, *objNode))
        {
            return false;
        }

        return definitionManager && definitionManager->RegisterServerSideDefinition(eSet);
    }

    template<>
    bool ServerDataManager::LoadObject<objects::EnchantSpecialData>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        auto eSpecial = std::shared_ptr<objects::EnchantSpecialData>(new objects::EnchantSpecialData);
        if(!eSpecial->Load(doc, *objNode))
        {
            return false;
        }

        return definitionManager && definitionManager->RegisterServerSideDefinition(eSpecial);
    }

    template<>
    bool ServerDataManager::LoadObject<objects::MiSStatusData>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        auto sStatus = std::shared_ptr<objects::MiSStatusData>(new objects::MiSStatusData);
        if(!sStatus->Load(doc, *objNode))
        {
            return false;
        }

        return definitionManager && definitionManager->RegisterServerSideDefinition(sStatus);
    }

    template<>
    bool ServerDataManager::LoadObject<objects::Tokusei>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        auto tokusei = std::shared_ptr<objects::Tokusei>(new objects::Tokusei);
        if(!tokusei->Load(doc, *objNode))
        {
            return false;
        }

        return definitionManager && definitionManager->RegisterServerSideDefinition(tokusei);
    }
}

bool ServerDataManager::LoadScript(const libcomp::String& path,
    const libcomp::String& source)
{
    ScriptEngine engine;
    engine.Using<ServerScript>();
    if(!engine.Eval(source))
    {
        LOG_ERROR(libcomp::String("Improperly formatted script encountered: %1\n")
            .Arg(path));
        return false;
    }

    auto root = Sqrat::RootTable(engine.GetVM());
    auto fDef = root.GetFunction("define");
    if(fDef.IsNull())
    {
        LOG_ERROR(libcomp::String("Invalid script encountered: %1\n").Arg(path));
        return false;
    }

    auto script = std::make_shared<ServerScript>();
    auto result = fDef.Evaluate<int>(script);
    if(!result || *result != 0 || script->Name.IsEmpty() || script->Type.IsEmpty())
    {
        LOG_ERROR(libcomp::String("Script is not properly defined: %1\n").Arg(path));
        return false;
    }

    script->Path = path;
    script->Source = source;

    if(script->Type.ToLower() == "ai")
    {
        if(mAIScripts.find(script->Name.C()) != mAIScripts.end())
        {
            LOG_ERROR(libcomp::String("Duplicate AI script encountered: %1\n")
                .Arg(script->Name.C()));
            return false;
        }

        fDef = root.GetFunction("prepare");
        if(fDef.IsNull())
        {
            LOG_ERROR(libcomp::String("AI script encountered"
                " with no 'prepare' function: %1\n").Arg(script->Name.C()));
            return false;
        }

        mAIScripts[script->Name.C()] = script;
    }
    else
    {
        if(mScripts.find(script->Name.C()) != mScripts.end())
        {
            LOG_ERROR(libcomp::String("Duplicate script encountered: %1\n")
                .Arg(script->Name.C()));
            return false;
        }

        // Check supported types here
        auto type = script->Type.ToLower();
        if(type == "eventcondition" || type == "eventbranchlogic")
        {
            fDef = root.GetFunction("check");
            if(fDef.IsNull())
            {
                LOG_ERROR(libcomp::String("Event conditional script encountered"
                    " with no 'check' function: %1\n")
                    .Arg(script->Name.C()));
                return false;
            }
        }
        else if(type == "actiontransform" || type == "eventtransform")
        {
            fDef = root.GetFunction("transform");
            if(fDef.IsNull())
            {
                LOG_ERROR(libcomp::String("Transform script encountered"
                    " with no 'transform' function: %1\n")
                    .Arg(script->Name.C()));
                return false;
            }

            fDef = root.GetFunction("prepare");
            if(!fDef.IsNull())
            {
                LOG_ERROR(libcomp::String("Transform script encountered"
                    " with reserved function name 'prepare': %1\n")
                    .Arg(script->Name.C()));
                return false;
            }
        }
        else if(type == "actioncustom")
        {
            fDef = root.GetFunction("run");
            if(fDef.IsNull())
            {
                LOG_ERROR(libcomp::String("Custom action script encountered"
                    " with no 'run' function: %1\n").Arg(script->Name.C()));
                return false;
            }
        }
        else
        {
            LOG_ERROR(libcomp::String("Invalid script type encountered: %1\n")
                .Arg(script->Type.C()));
            return false;
        }

        mScripts[script->Name.C()] = script;
    }

    return true;
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<ServerDataManager>()
    {
        if(!BindingExists("ServerDataManager"))
        {
            Sqrat::Class<ServerDataManager> binding(
                mVM, "ServerDataManager");
            Bind<ServerDataManager>("ServerDataManager", binding);

            // These are needed for some methods.
            Using<DefinitionManager>();

            binding
                .Func("LoadData", &ServerDataManager::LoadData)
                ; // Last call to binding
        }

        return *this;
    }
}
