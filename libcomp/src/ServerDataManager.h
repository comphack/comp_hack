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
#include "Log.h"

// tinyxml2 Includes
#include "PushIgnore.h"
#include <tinyxml2.h>
#include "PopIgnore.h"

// Standard C++11 Includes
#include <unordered_map>

namespace objects
{
class Event;
class ServerShop;
class ServerZone;
}

namespace libcomp
{

/**
 * Container for AI script information.
 */
struct ServerAIScript
{
    String Name;
    String Path;
    String Source;
};

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
     * Get an event by definition ID
     * @param id Definition ID of an event to load
     * @return Pointer to the event matching the specified id
     */
    const std::shared_ptr<objects::Event> GetEventData(const libcomp::String& id);

    /**
     * Get a shop by definition ID
     * @param id Definition ID of a shop to load
     * @return Pointer to the shop matching the specified id
     */
    const std::shared_ptr<objects::ServerShop> GetShopData(uint32_t id);

    /**
     * Get an AI script by name
     * @param name Name of the script to load
     * @return Pointer to the script definition
     */
    const std::shared_ptr<ServerAIScript> GetAIScript(const libcomp::String& name);

    /**
     * Load all server data defintions in the data store
     * @param pDataStore Pointer to the datastore to load binary files from
     * @return true on success, false on failure
     */
    bool LoadData(gsl::not_null<DataStore*> pDataStore);

private:
    /**
     * Get a server object by ID from the supplied map of the specified
     * key and value type
     * @param id ID of the server object to retrieve from the map
     * @param data Map of server objects to retrieve by ID
     * @return Pointer to the matching server object, null if doesn't exist
     */
    template <class K, class T>
    std::shared_ptr<T> GetObjectByID(K id,
        std::unordered_map<K, std::shared_ptr<T>>& data)
    {
        auto iter = data.find(id);
        if(iter != data.end())
        {
            return iter->second;
        }

        return nullptr;
    }

    /**
     * Load all objects from files in a datastore path
     * @param pDataStore Pointer to the datastore to use
     * @param datastorePath Path within the data store to load files from
     * @return true on success, false on failure
     */
    template <class T>
    bool LoadObjects(gsl::not_null<DataStore*> pDataStore,
        const libcomp::String& datastorePath)
    {
        std::list<libcomp::String> files;
        std::list<libcomp::String> dirs;
        std::list<libcomp::String> symLinks;

        (void)pDataStore->GetListing(datastorePath, files, dirs, symLinks,
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
                    return false;
                }
                
                const tinyxml2::XMLElement *rootNode = objsDoc.RootElement();
                const tinyxml2::XMLElement *objNode = rootNode->FirstChildElement("object");

                while(nullptr != objNode)
                {
                    if(!LoadObject<T>(objsDoc, objNode))
                    {
                        LOG_ERROR(libcomp::String("Failed to load XML file: %1\n").Arg(path));
                        return false;
                    }

                    objNode = objNode->NextSiblingElement("object");
                }

                LOG_DEBUG(libcomp::String("Loaded XML file: %1\n").Arg(path));
            }
        }


        return true;
    }

    /**
     * Load an object of the templated type from an XML node
     * @param doc XML document being loaded from
     * @param objNode XML node being loaded from
     * @return true on success, false on failure
     */
    template <class T>
    bool LoadObject(const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement *objNode);
    

    /**
     * Load all script files in the specified datastore
     * @param pDataStore Pointer to the datastore to use
     * @param datastorePath Path within the data store to load scripts from
     * @param handler Function pointer to call upon successful load
     * @return true on success, false on failure
     */
    bool LoadScripts(gsl::not_null<DataStore*> pDataStore,
        const libcomp::String& datastorePath,
        std::function<bool(ServerDataManager&,
            const libcomp::String&, const libcomp::String&)> handler);

    /**
     * Verify and store a successfully loaded AI script
     * @param path Path to the AI script file
     * @param source Source contained within the AI script file
     * @return true on success, false on failure
     */
    bool LoadAIScript(const libcomp::String& path, const libcomp::String& source);

    /// Map of server zone defintions by zone definition ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::ServerZone>> mZoneData;
    
    /// Map of events by definition ID
    std::unordered_map<std::string,
        std::shared_ptr<objects::Event>> mEventData;

    /// Map of shops by definition ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::ServerShop>> mShopData;

    /// Map of AI scripts by name
    std::unordered_map<std::string, std::shared_ptr<ServerAIScript>> mAIScripts;
};

} // namspace libcomp

#endif // LIBCOMP_SRC_SERVERDATAMANAGER_H