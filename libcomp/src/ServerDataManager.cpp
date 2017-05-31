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

// object Includes
#include "Event.h"
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

    return !failure;
}

namespace libcomp
{
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
}