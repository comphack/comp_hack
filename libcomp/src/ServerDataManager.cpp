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
#include "Log.h"
#include "ScriptEngine.h"

// object Includes
#include "Event.h"
#include "ServerShop.h"
#include "ServerZone.h"

using namespace libcomp;

ServerDataManager::ServerDataManager()
{
}

ServerDataManager::~ServerDataManager()
{
}

const std::shared_ptr<objects::ServerZone> ServerDataManager::GetZoneData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::ServerZone>(id, mZoneData);
}

const std::shared_ptr<objects::Event> ServerDataManager::GetEventData(const libcomp::String& id)
{
    return GetObjectByID<std::string, objects::Event>(id.C(), mEventData);
}

const std::shared_ptr<objects::ServerShop> ServerDataManager::GetShopData(uint32_t id)
{
    return GetObjectByID<uint32_t, objects::ServerShop>(id, mShopData);
}

const std::shared_ptr<ServerAIScript> ServerDataManager::GetAIScript(const libcomp::String& name)
{
    return GetObjectByID<std::string, ServerAIScript>(name.C(), mAIScripts);
}

bool ServerDataManager::LoadData(gsl::not_null<DataStore*> pDataStore)
{
    bool failure = false;

    if(!failure)
    {
        failure = !LoadObjects<objects::ServerZone>(pDataStore, "/zones");
    }

    if(!failure)
    {
        failure = !LoadObjects<objects::Event>(pDataStore, "/events");
    }

    if(!failure)
    {
        failure = !LoadObjects<objects::ServerShop>(pDataStore, "/shops");
    }

    if(!failure)
    {
        failure = !LoadScripts(pDataStore, "/ai", &ServerDataManager::LoadAIScript);
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
    ScriptEngine& ScriptEngine::Using<ServerAIScript>()
    {
        if(!BindingExists("ServerAIScript"))
        {
            Sqrat::Class<ServerAIScript> binding(mVM, "ServerAIScript");
            binding.Var("Name", &ServerAIScript::Name);
            Bind<ServerAIScript>("ServerAIScript", binding);
        }

        return *this;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::ServerZone>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode)
    {
        auto zone = std::shared_ptr<objects::ServerZone>(new objects::ServerZone);
        if(!zone->Load(doc, *objNode))
        {
            return false;
        }

        auto id = zone->GetID();
        if(mZoneData.find(id) != mZoneData.end())
        {
            LOG_ERROR(libcomp::String("Duplicate zone encountered: %1\n").Arg(id));
            return false;
        }

        mZoneData[id] = zone;

        return true;
    }

    template<>
    bool ServerDataManager::LoadObject<objects::Event>(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement *objNode)
    {
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
        const tinyxml2::XMLElement *objNode)
    {
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
}

bool ServerDataManager::LoadAIScript(const libcomp::String& path,
    const libcomp::String& source)
{
    ScriptEngine engine;
    engine.Using<ServerAIScript>();
    if(!engine.Eval(source))
    {
        LOG_ERROR(libcomp::String("Improperly formatted AI script encountered: %1\n")
            .Arg(path));
        return false;
    }

    auto root = Sqrat::RootTable(engine.GetVM());
    auto fDef = root.GetFunction("define");
    auto fUpdate = root.GetFunction("update");
    if(fDef.IsNull() || fUpdate.IsNull())
    {
        LOG_ERROR(libcomp::String("Invalid AI script encountered: %1\n").Arg(path));
        return false;
    }

    auto script = std::make_shared<ServerAIScript>();
    auto result = fDef.Evaluate<int>(script);
    if(!result || *result != 0 || script->Name.IsEmpty())
    {
        LOG_ERROR(libcomp::String("AI script is not properly defined: %1\n").Arg(path));
        return false;
    }
    script->Path = path;
    script->Source = source;

    if(mAIScripts.find(script->Name.C()) != mAIScripts.end())
    {
        LOG_ERROR(libcomp::String("Duplicate AI script encountered: %1\n").Arg(script->Name.C()));
        return false;
    }
    mAIScripts[script->Name.C()] = script;
    return true;
}