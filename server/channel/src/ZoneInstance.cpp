﻿/**
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
#include <MiDCategoryData.h>
#include <MiDevilData.h>
#include <MiTimeLimitData.h>
#include <MiUnionData.h>
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
            Using<objects::ZoneInstanceObject>();
            Using<ActiveEntityState>();
            Using<objects::Demon>();

            Sqrat::DerivedClass<ZoneInstance,
                objects::ZoneInstanceObject> binding(mVM, "ZoneInstance");
            binding
                .Func("GetFlagState", &ZoneInstance::GetFlagStateValue)
                .Func("GetTimerID", &ZoneInstance::GetTimerID);

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
    SetID(id);
    SetDefinition(definition);
    SetAccessCIDs(accessCIDs);
    SetOriginalAccessCIDs(accessCIDs);
}

ZoneInstance::ZoneInstance(const ZoneInstance& other)
{
    (void)other;
}

ZoneInstance::~ZoneInstance()
{
    if(GetID() != 0)
    {
        LOG_DEBUG(libcomp::String("Deleting zone instance: %1\n")
            .Arg(GetID()));
    }
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

std::list<std::shared_ptr<ChannelClientConnection>> ZoneInstance::GetConnections()
{
    std::list<std::shared_ptr<ChannelClientConnection>> connections;

    std::list<std::shared_ptr<Zone>> zones;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto& zPair : mZones)
        {
            for(auto& zPair2 : zPair.second)
            {
                zones.push_back(zPair2.second);
            }
        }
    }

    for(auto zone : zones)
    {
        for(auto connection : zone->GetConnectionList())
        {
            connections.push_back(connection);
        }
    }

    return connections;
}

void ZoneInstance::RefreshPlayerState()
{
    auto variant = GetVariant();
    if(variant && variant->GetInstanceType() == InstanceType_t::DEMON_ONLY)
    {
        // XP multiplier depends on the current state of demons in it
        auto connections = GetConnections();

        std::lock_guard<std::mutex> lock(mLock);

        // Demon only dungeons get a flat 100% XP boost if no others apply
        float xpMultiplier = 1.f;

        if(connections.size() > 1)
        {
            // If more than one player is in the instance, apply link bonus
            std::set<uint8_t> families;
            std::set<uint8_t> races;
            std::set<uint32_t> types;

            for(auto client : connections)
            {
                auto state = client->GetClientState();
                auto dState = state->GetDemonState();
                auto demonDef = dState->GetDevilData();
                if(demonDef)
                {
                    auto cat = demonDef->GetCategory();
                    families.insert((uint8_t)cat->GetFamily());
                    races.insert((uint8_t)cat->GetRace());
                    types.insert(demonDef->GetUnionData()->GetBaseDemonID());
                }
            }

            if(types.size() == 1)
            {
                // All same base demon type
                xpMultiplier = 3.f;
            }
            else if(races.size() == 1)
            {
                // All same race
                xpMultiplier = 2.f;
            }
            else if(families.size() == 1)
            {
                // All same family
                xpMultiplier = 1.5f;
            }
        }

        SetXPMultiplier(xpMultiplier);
    }
}

bool ZoneInstance::GetFlagState(int32_t key, int32_t& value, int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto it = mFlagStates.find(worldCID);

    if(mFlagStates.end() != it)
    {
        auto& m = it->second;
        auto it2 = m.find(key);

        if(m.end() != it2)
        {
            value = it2->second;

            return true;
        }
    }

    return false;
}

std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>>
    ZoneInstance::GetFlagStates()
{
    std::lock_guard<std::mutex> lock(mLock);

    return mFlagStates;
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

uint32_t ZoneInstance::GetTimerID()
{
    auto timeLimitData = GetTimeLimitData();
    return timeLimitData ? timeLimitData->GetID() : 0;
}
