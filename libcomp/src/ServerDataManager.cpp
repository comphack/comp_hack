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

bool ServerDataManager::LoadData(gsl::not_null<DataStore*> pDataStore)
{
    bool failure = false;

    if(!failure)
    {
        std::list<libcomp::String> files;
        std::list<libcomp::String> dirs;
        std::list<libcomp::String> symLinks;

        (void)pDataStore->GetListing("/zones", files, dirs, symLinks,
            true, true);

        for(auto path : files)
        {
            if(path.Matches("^.*\\.xml$"))
            {
                tinyxml2::XMLDocument objsDoc;

                std::vector<char> data = pDataStore->ReadFile(path);

                if(data.empty() || tinyxml2::XML_SUCCESS !=
                    objsDoc.Parse(&data[0], data.size()))
                {
                    failure = true;
                    break;
                }

                if(!LoadObjects<objects::ServerZone>(objsDoc, mZoneData))
                {
                    failure = true;
                    break;
                }

                LOG_DEBUG(libcomp::String("Loaded zone file: %1\n").Arg(path));
            }
        }
    }

    return !failure;
}
