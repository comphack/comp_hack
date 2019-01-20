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
#include <DestinyBox.h>
#include <InstanceAccess.h>
#include <MiDCategoryData.h>
#include <MiDevilData.h>
#include <MiTimeLimitData.h>
#include <MiUnionData.h>
#include <ScriptEngine.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>

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

            Sqrat::DerivedClass<ZoneInstance,
                objects::ZoneInstanceObject,
                Sqrat::NoConstructor<ZoneInstance>> binding(mVM, "ZoneInstance");
            binding
                .Func("GetDefinitionID", &ZoneInstance::GetDefinitionID)
                .Func("GetFlagState", &ZoneInstance::GetFlagStateValue)
                .Func("SetFlagState", &ZoneInstance::SetFlagState);

            Bind<ZoneInstance>("ZoneInstance", binding);
        }

        return *this;
    }
}

ZoneInstance::ZoneInstance(uint32_t id, const std::shared_ptr<
    objects::ServerZoneInstance>& definition,
    const std::shared_ptr<objects::InstanceAccess>& access)
{
    SetID(id);
    SetDefinition(definition);
    SetAccess(access);

    if(access)
    {
        SetOriginalAccessCIDs(access->GetAccessCIDs());
    }
}

ZoneInstance::~ZoneInstance()
{
    if(GetID() != 0)
    {
        LOG_DEBUG(libcomp::String("Deleting zone instance: %1\n")
            .Arg(GetID()));
    }
}

uint32_t ZoneInstance::GetDefinitionID()
{
    return GetDefinition()->GetID();
}

bool ZoneInstance::AddZone(const std::shared_ptr<Zone>& zone)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto def = zone->GetDefinition();
    if(!def)
    {
        return false;
    }

    if(mZones.find(def->GetID()) == mZones.end() ||
        mZones[def->GetID()].find(def->GetDynamicMapID()) == mZones[def->GetID()].end())
    {
        mZones[def->GetID()][def->GetDynamicMapID()] = zone;
        return true;
    }

    return false;
}

std::list<std::shared_ptr<Zone>> ZoneInstance::GetZones()
{
    std::list<std::shared_ptr<Zone>> zones;

    std::lock_guard<std::mutex> lock(mLock);
    for(auto& zdPair : mZones)
    {
        for(auto zPair : zdPair.second)
        {
            zones.push_back(zPair.second);
        }
    }

    return zones;
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

std::shared_ptr<Zone> ZoneInstance::GetZone(uint32_t uniqueID)
{
    for(auto zone : GetZones())
    {
        if(zone->GetID() == uniqueID)
        {
            return zone;
        }
    }

    return nullptr;
}

std::list<std::shared_ptr<ChannelClientConnection>> ZoneInstance::GetConnections()
{
    std::list<std::shared_ptr<ChannelClientConnection>> connections;
    for(auto zone : GetZones())
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

std::shared_ptr<objects::DestinyBox> ZoneInstance::GetDestinyBox(
    int32_t worldCID)
{
    // Default to own box
    auto dBox = GetDestinyBoxes(worldCID);
    if(!dBox && worldCID)
    {
        // Return shared if own box does not exist
        dBox = GetDestinyBoxes(0);
    }

    auto variant = GetVariant();
    if(!dBox && variant)
    {
        uint8_t size = variant->GetDestinyBoxSize();
        if(size)
        {
            // Box should exist but does not, create it an allocate slots
            std::lock_guard<std::mutex> lock(mLock);

            dBox = std::make_shared<objects::DestinyBox>();
            for(uint8_t i = 0; i < size; i++)
            {
                dBox->AppendLoot(nullptr);
            }

            int32_t ownerCID = variant->GetDestinyBoxShared() ? 0 : worldCID;
            dBox->SetOwnerCID(ownerCID);

            SetDestinyBoxes(ownerCID, dBox);
        }
    }

    return dBox;
}

std::unordered_map<uint8_t, std::shared_ptr<objects::Loot>>
    ZoneInstance::UpdateDestinyBox(int32_t worldCID, uint8_t& newNext,
    const std::list<std::shared_ptr<objects::Loot>> &add,
    const std::set<uint8_t>& remove)
{
    std::unordered_map<uint8_t, std::shared_ptr<objects::Loot>> results;

    newNext = 0;

    auto dBox = GetDestinyBox(worldCID);
    if(!dBox)
    {
        return results;
    }

    std::lock_guard<std::mutex> lock(mLock);

    size_t size = dBox->LootCount();

    // Do removes first (append to end to "shift" forward)
    for(uint8_t slot : remove)
    {
        if(dBox->GetLoot(slot))
        {
            dBox->InsertLoot(slot, nullptr);
            dBox->RemoveLoot((size_t)(slot + 1));
            results[slot] = nullptr;
        }
    }

    // Update the next position
    newNext = dBox->GetNextPosition();
    if(remove.size() > 0)
    {
        // Always maximize the amount of spaces between next and the first
        // item that will be overwritten based on the current position
        size_t seen = 0;
        while(!dBox->GetLoot(newNext) && seen < size)
        {
            uint8_t previous = 0;
            if(newNext == 0)
            {
                previous = (uint8_t)(size - 1);
            }
            else
            {
                previous = (uint8_t)((size_t)(newNext - 1) % size);
            }

            if(!dBox->GetLoot(previous))
            {
                newNext = previous;
                seen++;
            }
            else
            {
                break;
            }
        }

        if(dBox->GetLoot(newNext))
        {
            // Removed elements did not include starting next, jump forward
            // to the first empty element
            for(size_t i = 0; i < size; i++)
            {
                size_t slot = (size_t)((newNext + i) % size);
                if(!dBox->GetLoot(slot))
                {
                    newNext = (uint8_t)slot;
                    break;
                }
            }
        }
        else if(seen == size)
        {
            // All removed, reset to start
            newNext = 0;
        }
    }

    // Next do updates
    for(auto loot : add)
    {
        dBox->InsertLoot(newNext, loot);
        dBox->RemoveLoot((size_t)(newNext + 1));
        results[newNext] = loot;

        newNext = (uint8_t)(newNext + 1);
        if(newNext >= (uint8_t)size)
        {
            newNext = 0;
        }
    }

    // Set the new next position
    dBox->SetNextPosition(newNext);

    return results;
}
