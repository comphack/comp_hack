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

#include "ServerDataManager.h"

// libcomp Includes
#include "DefinitionManager.h"
#include "Log.h"
#include "ScriptEngine.h"

// object Includes
#include <DropSet.h>
#include <EnchantSetData.h>
#include <EnchantSpecialData.h>
#include <Event.h>
#include <ServerShop.h>
#include <ServerZone.h>
#include <Tokusei.h>

using namespace libcomp;

ServerDataManager::ServerDataManager()
{
}

ServerDataManager::~ServerDataManager()
{
}

const std::shared_ptr<objects::ServerZone> ServerDataManager::GetZoneData(uint32_t id,
    uint32_t dynamicMapID)
{
    auto iter = mZoneData.find(id);
    if(iter != mZoneData.end())
    {
        if(dynamicMapID != 0)
        {
            auto dIter = iter->second.find(dynamicMapID);
            return (dIter != iter->second.end()) ? dIter->second : nullptr;
        }
        else
        {
            // Return first
            return iter->second.begin()->second;
        }
    }

    return nullptr;
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

const std::shared_ptr<objects::Event> ServerDataManager::GetEventData(const libcomp::String& id)
{
    return GetObjectByID<std::string, objects::Event>(id.C(), mEventData);
}

const std::shared_ptr<objects::ServerShop> ServerDataManager::GetShopData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::ServerShop>(id, mShopData);
}

const std::shared_ptr<objects::DropSet> ServerDataManager::GetDropSetData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::DropSet>(id, mDropSetData);
}

const std::shared_ptr<ServerScript> ServerDataManager::GetScript(const libcomp::String& name)
{
    return GetObjectByID<std::string, ServerScript>(name.C(), mScripts);
}

const std::shared_ptr<ServerScript> ServerDataManager::GetAIScript(const libcomp::String& name)
{
    return GetObjectByID<std::string, ServerScript>(name.C(), mAIScripts);
}

bool ServerDataManager::LoadData(gsl::not_null<DataStore*> pDataStore,
    DefinitionManager* definitionManager)
{
    bool failure = false;

    if(definitionManager)
    {
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
            LOG_DEBUG("Loading tokusei server definitions...\n");
            failure = !LoadObjects<objects::Tokusei>(pDataStore,
                "/tokusei", definitionManager);
        }
    }

    if(!failure)
    {
        LOG_DEBUG("Loading zone server definitions...\n");
        failure = !LoadObjects<objects::ServerZone>(pDataStore, "/zones",
            definitionManager);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading event server definitions...\n");
        failure = !LoadObjects<objects::Event>(pDataStore, "/events",
            definitionManager);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading shop server definitions...\n");
        failure = !LoadObjects<objects::ServerShop>(pDataStore, "/shops",
            definitionManager);
    }

    if(!failure)
    {
        LOG_DEBUG("Loading server scripts...\n");
        failure = !LoadScripts(pDataStore, "/scripts", &ServerDataManager::LoadScript);
    }

    return !failure;
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

        if(definitionManager && !definitionManager->GetZoneData(id))
        {
            LOG_WARNING(libcomp::String("Skipping unknown zone: %1%2\n").Arg(id)
                .Arg(id != dynamicMapID ? libcomp::String(" (%1)").Arg(dynamicMapID) : ""));
            return true;
        }

        if(mZoneData.find(id) != mZoneData.end() &&
            mZoneData[id].find(dynamicMapID) != mZoneData[id].end())
        {
            LOG_ERROR(libcomp::String("Duplicate zone encountered: %1%2\n").Arg(id)
                .Arg(id != dynamicMapID ? libcomp::String(" (%1)").Arg(dynamicMapID) : ""));
            return false;
        }

        mZoneData[id][dynamicMapID] = zone;

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

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::DropSet>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode, DefinitionManager* definitionManager)
    {
        (void)definitionManager;

        auto dropSet = std::shared_ptr<objects::DropSet>(new objects::DropSet);
        if(dropSet == nullptr || !dropSet->Load(doc, *objNode))
        {
            return false;
        }
    
        uint32_t id = dropSet->GetID();
        if(mDropSetData.find(id) != mDropSetData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate drop set encountered: %1\n").Arg(id));
            return false;
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

        mScripts[script->Name.C()] = script;

        // Check supported types here
        if(script->Type.ToLower() == "eventcondition" ||
            script->Type.ToLower() == "eventbranchlogic")
        {
            fDef = root.GetFunction("check");
            if(fDef.IsNull())
            {
                LOG_ERROR(libcomp::String("EventCondition script encountered"
                    " with no 'check' function: %1\n")
                    .Arg(script->Name.C()));
                return false;
            }
        }
        else
        {
            LOG_ERROR(libcomp::String("Invalid script type encountered: %1\n")
                .Arg(script->Type.C()));
            return false;
        }
    }

    return true;
}