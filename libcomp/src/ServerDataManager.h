/**
 * @file libcomp/src/ServerDataManager.h
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

#ifndef LIBCOMP_SRC_SERVERDATAMANAGER_H
#define LIBCOMP_SRC_SERVERDATAMANAGER_H

// Standard C++14 Includes
#include <gsl/gsl>

// libcomp Includes
#include "CString.h"
#include "DataStore.h"

// tinyxml2 Includes
#include "PushIgnore.h"
#include <tinyxml2.h>
#include "PopIgnore.h"

// Standard C++11 Includes
#include <unordered_map>

namespace objects
{
class ServerZone;
}

namespace libcomp
{

/**
 * Manager class responsible for loading server specific files such as
 * zones and script files.
 */
class ServerDataManager
{
public:
    /**
     * Create a new ServerDataManager.
     */
    ServerDataManager();

    /**
     * Clean up the ServerDataManager.
     */
    ~ServerDataManager();

    /**
     * Get a server zone by definition ID
     * @param id Definition ID of a zone to load
     * @return Pointer to the server zone matching the specified id
     */
    const std::shared_ptr<objects::ServerZone> GetZoneData(uint32_t id);

    /**
     * Load all server data defintions in the data store
     * @param pDataStore Pointer to the datastore to load binary files from
     * @return true on success, false on failure
     */
    bool LoadData(gsl::not_null<DataStore*> pDataStore);

private:
    /**
     * Get a server object by ID from the supplied map of the specified
     * type
     * @param id ID of the server object to retrieve from the map
     * @param data Map of server objects to retrieve by ID
     * @return Pointer to the matching server object, null if doesn't exist
     */
    template <class T>
    std::shared_ptr<T> GetObjectByID(uint32_t id,
        std::unordered_map<uint32_t, std::shared_ptr<T>>& data)
    {
        auto iter = data.find(id);
        if(iter != data.end())
        {
            return iter->second;
        }

        return nullptr;
    }

    /**
     * Load all objects in an XML document into the specified type
     * @param doc Loaded XML document
     * @param data Output map to store the loaded objects in by ID
     * @return true on success, false on failure
     */
    template <class T>
    bool LoadObjects(const tinyxml2::XMLDocument& doc,
        std::unordered_map<uint32_t, std::shared_ptr<T>>& data)
    {
        const tinyxml2::XMLElement *rootNode = doc.RootElement();
        const tinyxml2::XMLElement *objNode = rootNode->FirstChildElement("object");

        while(nullptr != objNode)
        {
            std::shared_ptr<T> obj = std::shared_ptr<T>(new T);
            if(!obj->Load(doc, *objNode))
            {
                return false;
            }

            data[obj->GetID()] = obj;

            objNode = objNode->NextSiblingElement("object");
        }

        return true;
    }

    /// Map of server zone defintions by zone definition ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::ServerZone>> mZoneData;
};

} // namspace libcomp

#endif // LIBCOMP_SRC_SERVERDATAMANAGER_H