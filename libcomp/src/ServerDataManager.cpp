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
#include "ServerNPC.h"
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
    return GetObjectByID<objects::ServerZone>(id, mZoneData);
}

bool ServerDataManager::LoadData(const libcomp::String& definitionPath)
{
    tinyxml2::XMLDocument defDoc;
    if(tinyxml2::XML_SUCCESS != defDoc.LoadFile(definitionPath.C()))
    {
        LOG_WARNING(libcomp::String("Failed to parse server definitions file: %1\n").Arg(
            definitionPath));
        return false;
    }

    LOG_DEBUG(libcomp::String("Reading server definitions file: %1\n").Arg(
        definitionPath));

    const tinyxml2::XMLElement *defRootNode = defDoc.RootElement();
    const tinyxml2::XMLElement *defNode = defRootNode->FirstChildElement("definition");

    bool failure = false;
    while(nullptr != defNode)
    {
        const char *szPath = defNode->Attribute("path");
        const char *szType = defNode->Attribute("type");

        if(nullptr == szPath || nullptr == szType)
        {
            failure = true;
            break;
        }

        tinyxml2::XMLDocument objsDoc;
        if(tinyxml2::XML_SUCCESS != objsDoc.LoadFile(szPath))
        {
            failure = true;
            break;
        }

        std::string type(szType);
        if(type == "zone")
        {
            if(!LoadObjects<objects::ServerZone>(objsDoc, mZoneData))
            {
                failure = true;
                break;
            }
        }
        
        defNode = defNode->NextSiblingElement("definition");
    }

    if(failure)
    {
        LOG_ERROR(libcomp::String("Error encountered while loading server data.\n"));
    }
    else
    {
        size_t loadCount = 0;
        for(auto z : mZoneData)
        {
            loadCount += z.second->NPCsCount() + z.second->ObjectsCount();
        }

        LOG_DEBUG(libcomp::String("Successfully loaded %1 server object(s).\n").Arg(
            loadCount));
    }

    return !failure;
}
