/**
 * @file server/channel/src/ZoneInstance.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents a zone instance containing one or many zones.
 *
 * This file is part of the Channel Server (channel).
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

#include "ZoneInstance.h"

// libcomp Includes
#include <Log.h>

// object Includes
#include <ScriptEngine.h>
#include <ServerZone.h>

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<ZoneInstance>()
    {
        if(!BindingExists("ZoneInstance", true))
        {
            Using<ActiveEntityState>();
            Using<objects::Demon>();

            Sqrat::Class<ZoneInstance> binding(mVM, "ZoneInstance");
            binding
                .Func<int32_t (ZoneInstance::*)(int32_t, int32_t, int32_t)>(
                    "GetFlagState", &ZoneInstance::GetFlagStateValue);

            Bind<ZoneInstance>("ZoneInstance", binding);
        }

        return *this;
    }
}

ZoneInstance::ZoneInstance()
{
}

ZoneInstance::ZoneInstance(uint32_t id, const std::shared_ptr<
    objects::ServerZoneInstance>& definition, std::set<int32_t>& accessCIDs)
{
    mID = id;
    mDefinition = definition;
    mAccessCIDs = accessCIDs;
}

ZoneInstance::ZoneInstance(const ZoneInstance& other)
{
    (void)other;
}

ZoneInstance::~ZoneInstance()
{
    if(mID != 0)
    {
        LOG_DEBUG(libcomp::String("Deleting zone instance: %1\n").Arg(mID));
    }
}

uint32_t ZoneInstance::GetID()
{
    return mID;
}

const std::shared_ptr<objects::ServerZoneInstance> ZoneInstance::GetDefinition()
{
    return mDefinition;
}

bool ZoneInstance::AddZone(const std::shared_ptr<Zone>& zone)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto def = zone->GetDefinition();
    if(!def)
    {
        return false;
    }

    if(mZones.find(def->GetID()) == mZones.end() &&
        mZones[def->GetID()].find(def->GetDynamicMapID()) == mZones[def->GetID()].end())
    {
        mZones[def->GetID()][def->GetDynamicMapID()] = zone;
        return true;
    }

    return false;
}

std::unordered_map<uint32_t, std::unordered_map<uint32_t,
    std::shared_ptr<Zone>>> ZoneInstance::GetZones() const
{
    return mZones;
}

std::shared_ptr<Zone> ZoneInstance::GetZone(uint32_t zoneID,
    uint32_t dynamicMapID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mZones.find(zoneID);
    if(it != mZones.end())
    {
        if(dynamicMapID == 0)
        {
            return it->second.begin()->second;
        }
        else
        {
            auto it2 = it->second.find(dynamicMapID);
            if(it2 != it->second.end())
            {
                return it2->second;
            }
        }
    }

    return nullptr;
}

std::set<int32_t> ZoneInstance::GetAccessCIDs() const
{
    return mAccessCIDs;
}

bool ZoneInstance::GetFlagState(int32_t key, int32_t& value, int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mFlagStates.find(worldCID);
    if(it != mFlagStates.end())
    {
        auto it2 = mFlagStates[worldCID].find(key);
        if(it2 != mFlagStates[worldCID].end())
        {
            value = it2->second;
            return true;
        }
    }

    return false;
}

int32_t ZoneInstance::GetFlagStateValue(int32_t key, int32_t nullDefault,
    int32_t worldCID)
{
    int32_t result;
    if(!GetFlagState(key, result, worldCID))
    {
        result = nullDefault;
    }

    return result;
}

void ZoneInstance::SetFlagState(int32_t key, int32_t value, int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);
    mFlagStates[worldCID][key] = value;
}
