/**
 * @file server/channel/src/Zone.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents a global or instanced zone on the channel.
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

#include "Zone.h"

// libcomp Includes
#include <Log.h>
#include <ScriptEngine.h>

// C++ Standard Includes
#include <cmath>

// object Includes
#include <Ally.h>
#include <CultureMachineState.h>
#include <Loot.h>
#include <LootBox.h>
#include <PlasmaState.h>
#include <PvPBase.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <SpawnGroup.h>
#include <SpawnLocationGroup.h>
#include <SpawnRestriction.h>

// channel Includes
#include "ChannelServer.h"
#include "WorldClock.h"
#include "ZoneInstance.h"

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<Zone>()
    {
        if(!BindingExists("Zone", true))
        {
            Using<objects::ZoneObject>();
            Using<ActiveEntityState>();
            Using<objects::Demon>();
            Using<ZoneInstance>();

            Sqrat::DerivedClass<Zone,
                objects::ZoneObject> binding(mVM, "Zone");
            binding
                .Func("GetDefinitionID", &Zone::GetDefinitionID)
                .Func("GetFlagState", &Zone::GetFlagStateValue)
                .Func("GetZoneInstance", &Zone::GetInstance)
                .Func("GroupHasSpawned", &Zone::GroupHasSpawned);

            Bind<Zone>("Zone", binding);
        }

        return *this;
    }
}

Zone::Zone()
{
}

Zone::Zone(uint32_t id, const std::shared_ptr<objects::ServerZone>& definition)
    : mNextRentalExpiration(0), mNextEncounterID(1)
{
    SetDefinition(definition);
    SetID(id);

    mHasRespawns = definition->PlasmaSpawnsCount() > 0;

    if(!mHasRespawns)
    {
        for(auto slgPair : definition->GetSpawnLocationGroups())
        {
            if(slgPair.second->GetRespawnTime())
            {
                mHasRespawns = true;
                break;
            }
        }
    }

    // Mark groups that start at disabled
    std::set<uint32_t> disabledGroupIDs;
    for(auto sgPair : definition->GetSpawnGroups())
    {
        auto restriction = sgPair.second
            ? sgPair.second->GetRestrictions() : nullptr;
        if(restriction && restriction->GetDisabled())
        {
            disabledGroupIDs.insert(sgPair.first);
        }
    }

    if(disabledGroupIDs.size() > 0)
    {
        DisableSpawnGroups(disabledGroupIDs, true);
    }
}

Zone::Zone(const Zone& other)
{
    (void)other;
}

Zone::~Zone()
{
}

uint32_t Zone::GetDefinitionID()
{
    return GetDefinition()->GetID();
}

const std::shared_ptr<ZoneGeometry> Zone::GetGeometry() const
{
    return mGeometry;
}

void Zone::SetGeometry(const std::shared_ptr<ZoneGeometry>& geometry)
{
    mGeometry = geometry;
}

std::shared_ptr<ZoneInstance> Zone::GetInstance() const
{
    return mZoneInstance;
}

InstanceType_t Zone::GetInstanceType() const
{
    auto variant = mZoneInstance ? mZoneInstance->GetVariant() : nullptr;
    return variant ? variant->GetInstanceType() : InstanceType_t::NORMAL;
}

void Zone::SetInstance(const std::shared_ptr<ZoneInstance>& instance)
{
    mZoneInstance = instance;
}

const std::shared_ptr<DynamicMap> Zone::GetDynamicMap() const
{
    return mDynamicMap;
}

bool Zone::HasRespawns() const
{
    return mHasRespawns;
}

void Zone::SetDynamicMap(const std::shared_ptr<DynamicMap>& map)
{
    mDynamicMap = map;
}

bool Zone::AddConnection(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();

    if(!GetInvalid())
    {
        RegisterEntityState(cState);
        RegisterEntityState(dState);

        std::lock_guard<std::mutex> lock(mLock);
        mConnections[state->GetWorldCID()] = client;

        return true;
    }
    else
    {
        return false;
    }
}

void Zone::RemoveConnection(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();

    auto cEntityID = cState->GetEntityID();
    auto dEntityID = dState->GetEntityID();
    int32_t worldCID = state->GetWorldCID();

    UnregisterEntityState(cEntityID);
    UnregisterEntityState(dEntityID);

    cState->SetZone(0);
    dState->SetZone(0);

    std::lock_guard<std::mutex> lock(mLock);
    mConnections.erase(state->GetWorldCID());

    // If this zone is not part of an instance, clear the character
    // specific flags
    if(mZoneInstance)
    {
        mFlagStates.erase(worldCID);
    }
}

void Zone::RemoveEntity(int32_t entityID, uint32_t spawnDelay)
{
    auto state = GetEntity(entityID);

    if(state)
    {
        std::lock_guard<std::mutex> lock(mLock);

        std::shared_ptr<ActiveEntityState> removeSpawn;
        uint32_t sgID = 0, slgID = 0, encounterID = 0;
        switch(state->GetEntityType())
        {
        case EntityType_t::ALLY:
            {
                mAllies.remove_if([entityID]
                    (const std::shared_ptr<AllyState>& a)
                    {
                        return a->GetEntityID() == entityID;
                    });

                auto aState = std::dynamic_pointer_cast<AllyState>(state);
                auto ally = aState->GetEntity();

                removeSpawn = aState;
                sgID = ally->GetSpawnGroupID();
                slgID = ally->GetSpawnLocationGroupID();
                encounterID = ally->GetEncounterID();
            }
            break;
        case EntityType_t::ENEMY:
            {
                mEnemies.remove_if([entityID]
                    (const std::shared_ptr<EnemyState>& e)
                    {
                        return e->GetEntityID() == entityID;
                    });

                auto eState = std::dynamic_pointer_cast<EnemyState>(state);
                auto enemy = eState->GetEntity();

                removeSpawn = eState;
                sgID = enemy->GetSpawnGroupID();
                slgID = enemy->GetSpawnLocationGroupID();
                encounterID = enemy->GetEncounterID();
            }
            break;
        case EntityType_t::LOOT_BOX:
            {
                auto lState = std::dynamic_pointer_cast<LootBoxState>(state);
                mLootBoxes.remove_if([entityID]
                    (const std::shared_ptr<LootBoxState>& e)
                    {
                        return e->GetEntityID() == entityID;
                    });

                if(lState->GetEntity()->GetType() ==
                    objects::LootBox::Type_t::BOSS_BOX)
                {
                    // Remove from the boss box group if it exists
                    for(auto pair : mBossBoxGroups)
                    {
                        if(pair.second.find(lState->GetEntityID()) !=
                            pair.second.end())
                        {
                            pair.second.erase(lState->GetEntityID());
                            if(pair.second.size() == 0)
                            {
                                // Remove the group if its empty now
                                mBossBoxGroups.erase(pair.first);
                                mBossBoxOwners.erase(pair.first);
                            }

                            break;
                        }
                    }
                }
            }
            break;
        default:
            break;
        }

        if(removeSpawn)
        {
            if(sgID)
            {
                mSpawnGroups[sgID].remove_if([removeSpawn](
                    const std::shared_ptr<ActiveEntityState>& e)
                    {
                        return e == removeSpawn;
                    });
            }

            if(slgID)
            {
                mSpawnLocationGroups[slgID].remove_if([removeSpawn](
                    const std::shared_ptr<ActiveEntityState>& e)
                    {
                        return e == removeSpawn;
                    });

                auto slg = GetDefinition()->GetSpawnLocationGroups(slgID);
                if(slg->GetRespawnTime() > 0.f &&
                    mSpawnLocationGroups[slgID].size() == 0)
                {
                    // Update the respawn time for the group, exit if found
                    for(auto rPair : mRespawnTimes)
                    {
                        if(rPair.second.find(slgID) != rPair.second.end())
                        {
                            return;
                        }
                    }

                    uint64_t rTime = ChannelServer::GetServerTime()
                        + (uint64_t)((double)slg->GetRespawnTime() *
                            1000000.0 + (double)(spawnDelay * 1000));

                    mRespawnTimes[rTime].insert(slgID);
                }
            }

            if(encounterID)
            {
                // Remove if the encounter exists but do not remove
                // the encounter itself until EcnounterDefeated is
                // called
                auto eIter = mEncounters.find(encounterID);
                if(eIter != mEncounters.end())
                {
                    eIter->second.erase(removeSpawn);
                }
            }
        }
    }

    UnregisterEntityState(entityID);
}

void Zone::AddAlly(const std::shared_ptr<AllyState>& ally)
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        mAllies.push_back(ally);

        uint32_t spotID = ally->GetEntity()->GetSpawnSpotID();
        uint32_t sgID = ally->GetEntity()->GetSpawnGroupID();
        uint32_t slgID = ally->GetEntity()->GetSpawnLocationGroupID();
        AddSpawnedEntity(ally, spotID, sgID, slgID);

        ally->SetDisplayState(ActiveDisplayState_t::ACTIVE);
    }

    RegisterEntityState(ally);
}

void Zone::AddBazaar(const std::shared_ptr<BazaarState>& bazaar)
{
    mBazaars.push_back(bazaar);
    RegisterEntityState(bazaar);
}

void Zone::AddCultureMachine(const std::shared_ptr<
    CultureMachineState>& machine)
{
    if(mCultureMachines.find(machine->GetMachineID()) ==
        mCultureMachines.end())
    {
        mCultureMachines[machine->GetMachineID()] = machine;
        RegisterEntityState(machine);
    }
}

void Zone::AddEnemy(const std::shared_ptr<EnemyState>& enemy)
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        mEnemies.push_back(enemy);

        uint32_t spotID = enemy->GetEntity()->GetSpawnSpotID();
        uint32_t sgID = enemy->GetEntity()->GetSpawnGroupID();
        uint32_t slgID = enemy->GetEntity()->GetSpawnLocationGroupID();
        AddSpawnedEntity(enemy, spotID, sgID, slgID);

        enemy->SetDisplayState(ActiveDisplayState_t::ACTIVE);
    }

    RegisterEntityState(enemy);
}

void Zone::AddLootBox(const std::shared_ptr<LootBoxState>& box,
    uint32_t bossGroupID)
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        mLootBoxes.push_back(box);

        if(bossGroupID > 0 &&
            box->GetEntity()->GetType() == objects::LootBox::Type_t::BOSS_BOX)
        {
            mBossBoxGroups[bossGroupID].insert(box->GetEntityID());
        }
    }
    RegisterEntityState(box);
}

void Zone::AddNPC(const std::shared_ptr<NPCState>& npc)
{
    mNPCs.push_back(npc);
    RegisterEntityState(npc);

    int32_t actorID = npc->GetEntity()->GetActorID();
    if(actorID)
    {
        mActors[actorID] = npc;
    }
}

void Zone::AddObject(const std::shared_ptr<ServerObjectState>& object)
{
    mObjects.push_back(object);
    RegisterEntityState(object);
    
    int32_t actorID = object->GetEntity()->GetActorID();
    if(actorID)
    {
        mActors[actorID] = object;
    }
}

void Zone::AddPlasma(const std::shared_ptr<PlasmaState>& plasma)
{
    mPlasma[plasma->GetEntity()->GetID()] = plasma;
    RegisterEntityState(plasma);
}

void Zone::AddPvPBase(const std::shared_ptr<PvPBaseState>& base)
{
    mPvPBases.push_back(base);
    RegisterEntityState(base);
}

std::unordered_map<int32_t,
    std::shared_ptr<ChannelClientConnection>> Zone::GetConnections()
{
    return mConnections;
}

std::list<std::shared_ptr<ChannelClientConnection>> Zone::GetConnectionList()
{
    std::lock_guard<std::mutex> lock(mLock);
    std::list<std::shared_ptr<ChannelClientConnection>> connections;
    for(auto cPair : mConnections)
    {
        connections.push_back(cPair.second);
    }
    return connections;
}

const std::shared_ptr<ActiveEntityState> Zone::GetActiveEntity(int32_t entityID)
{
    return std::dynamic_pointer_cast<ActiveEntityState>(GetEntity(entityID));
}

const std::list<std::shared_ptr<ActiveEntityState>> Zone::GetActiveEntities()
{
    std::list<std::shared_ptr<ActiveEntityState>> results;

    std::lock_guard<std::mutex> lock(mLock);
    for(auto ePair : mAllEntities)
    {
        auto active = std::dynamic_pointer_cast<ActiveEntityState>(ePair.second);
        if(active)
        {
            results.push_back(active);
        }
    }

    return results;
}

const std::list<std::shared_ptr<ActiveEntityState>>
    Zone::GetActiveEntitiesInRadius(float x, float y, double radius)
{
    std::list<std::shared_ptr<ActiveEntityState>> results;

    uint64_t now = ChannelServer::GetServerTime();

    float rSquared = (float)std::pow(radius, 2);

    for(auto active : GetActiveEntities())
    {
        active->RefreshCurrentPosition(now);
        if(rSquared >= active->GetDistance(x, y, true))
        {
            results.push_back(active);
        }
    }

    return results;
}

std::shared_ptr<AllyState> Zone::GetAlly(int32_t id)
{
    return std::dynamic_pointer_cast<AllyState>(GetEntity(id));
}

const std::list<std::shared_ptr<AllyState>> Zone::GetAllies() const
{
    return mAllies;
}

std::shared_ptr<BazaarState> Zone::GetBazaar(int32_t id)
{
    return std::dynamic_pointer_cast<BazaarState>(GetEntity(id));
}

const std::list<std::shared_ptr<BazaarState>> Zone::GetBazaars() const
{
    return mBazaars;
}

std::shared_ptr<CultureMachineState> Zone::GetCultureMachine(int32_t id)
{
    return std::dynamic_pointer_cast<CultureMachineState>(GetEntity(id));
}

const std::unordered_map<uint32_t,
    std::shared_ptr<CultureMachineState>> Zone::GetCultureMachines() const
{
    return mCultureMachines;
}

std::shared_ptr<EnemyState> Zone::GetEnemy(int32_t id)
{
    return std::dynamic_pointer_cast<EnemyState>(GetEntity(id));
}

const std::list<std::shared_ptr<EnemyState>> Zone::GetEnemies() const
{
    return mEnemies;
}

std::shared_ptr<LootBoxState> Zone::GetLootBox(int32_t id)
{
    return std::dynamic_pointer_cast<LootBoxState>(GetEntity(id));
}

const std::list<std::shared_ptr<LootBoxState>> Zone::GetLootBoxes() const
{
    return mLootBoxes;
}

bool Zone::ClaimBossBox(int32_t id, int32_t looterID)
{
    auto lState = GetLootBox(id);
    auto lBox = lState ? lState->GetEntity() : nullptr;
    if(!lState || (lBox->ValidLooterIDsCount() > 0 &&
        !lBox->ValidLooterIDsContains(looterID)))
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mLock);
    uint32_t groupID = 0;
    for(auto pair : mBossBoxGroups)
    {
        if(pair.second.find(lState->GetEntityID()) != pair.second.end())
        {
            groupID = pair.first;
            break;
        }
    }

    if(groupID == 0 || lBox->ValidLooterIDsContains(looterID))
    {
        return true;
    }

    if(mBossBoxOwners[groupID].find(looterID) == mBossBoxOwners[groupID].end())
    {
        // No boss box from this group looted yet
        std::set<int32_t> looterIDs = { looterID };
        lState->GetEntity()->SetValidLooterIDs(looterIDs);

        mBossBoxOwners[groupID].insert(looterID);

        return true;
    }

    return false;
}

int32_t Zone::OccupyPvPBase(int32_t baseID, int32_t occupierID, bool complete,
    uint64_t occupyStartTime)
{
    auto bState = GetPvPBase(baseID);

    std::lock_guard<std::mutex> lock(mLock);

    auto state = occupierID > 0
        ? ClientState::GetEntityClientState(occupierID) : nullptr;
    auto sZone = state ? state->GetZone() : nullptr;
    if(!bState || (occupierID > 0 && (!sZone || sZone.get() != this)))
    {
        // It seems like there should be other error codes but the
        // client does not respond differently to any of them
        return -1;
    }

    auto base = bState->GetEntity();
    if(occupierID <= 0)
    {
        if(complete)
        {
            // Remove occupier
            base->SetOccupyTime(0);
            base->SetOccupierID(0);
            return 0;
        }
        else
        {
            // Cannot start occupation with no entity
            return -1;
        }
    }

    if(!state)
    {
        // Player entity required past this point
        return -1;
    }

    int32_t teamID = (int32_t)(state->GetCharacterState()
        ->GetFactionGroup() - 1);
    if(teamID != 0 && teamID != 1)
    {
        // Not on a PvP team
        return -1;
    }

    if(!complete)
    {
        // Requesting to start occupation
        if(base->GetOccupierID())
        {
            // Already being occupied
            return -1;
        }

        if(base->GetTeam() != 2 && (int32_t)base->GetTeam() == teamID)
        {
            // Already owned by the same team
            return -1;
        }

        // Occupation valid
        base->SetOccupyTime(ChannelServer::GetServerTime());
        base->SetOccupierID(occupierID);
    }
    else
    {
        // Requesting to finish occupation
        if(base->GetOccupierID() != occupierID)
        {
            // Not the current occupier
            return -1;
        }

        if(base->GetOccupyTime() != occupyStartTime)
        {
            // Time has been reset
            return -1;
        }

        base->SetTeam((int8_t)teamID);
        base->SetOccupierID(0);
        base->SetBonusCount(0);
    }

    return 0;
}

uint16_t Zone::IncreasePvPBaseBonus(int32_t baseID, uint64_t occupyStartTime)
{
    auto bState = GetPvPBase(baseID);
    if(!bState)
    {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mLock);

    auto base = bState->GetEntity();
    if(base->GetOccupyTime() == occupyStartTime)
    {
        uint16_t bCount = (uint16_t)(base->GetBonusCount() + 1);
        base->SetBonusCount(bCount);
        return bCount;
    }

    return 0;
}

const std::list<std::shared_ptr<NPCState>> Zone::GetNPCs() const
{
    return mNPCs;
}

std::shared_ptr<PlasmaState> Zone::GetPlasma(uint32_t id)
{
    auto it = mPlasma.find(id);
    return it != mPlasma.end() ? it->second : nullptr;
}

const std::unordered_map<uint32_t,
    std::shared_ptr<PlasmaState>> Zone::GetPlasma() const
{
    return mPlasma;
}

std::shared_ptr<PvPBaseState> Zone::GetPvPBase(int32_t id)
{
    return std::dynamic_pointer_cast<PvPBaseState>(GetEntity(id));
}

const std::list<std::shared_ptr<PvPBaseState>> Zone::GetPvPBases() const
{
    return mPvPBases;
}

const std::list<std::shared_ptr<ServerObjectState>> Zone::GetServerObjects() const
{
    return mObjects;
}

void Zone::RegisterEntityState(const std::shared_ptr<objects::EntityStateObject>& state)
{
    std::lock_guard<std::mutex> lock(mLock);
    mAllEntities[state->GetEntityID()] = state;
}

void Zone::UnregisterEntityState(int32_t entityID)
{
    std::lock_guard<std::mutex> lock(mLock);
    mAllEntities.erase(entityID);
    mPendingDespawnEntities.erase(entityID);
}

std::shared_ptr<objects::EntityStateObject> Zone::GetEntity(int32_t id)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mAllEntities.find(id);

    if(mAllEntities.end() != it)
    {
        return it->second;
    }

    return {};
}

std::shared_ptr<objects::EntityStateObject> Zone::GetActor(int32_t actorID)
{
    auto it = mActors.find(actorID);
    return it != mActors.end() ? it->second : nullptr;
}

std::shared_ptr<NPCState> Zone::GetNPC(int32_t id)
{
    return std::dynamic_pointer_cast<NPCState>(GetEntity(id));
}

std::shared_ptr<ServerObjectState> Zone::GetServerObject(int32_t id)
{
    return std::dynamic_pointer_cast<ServerObjectState>(GetEntity(id));
}

void Zone::SetNextStatusEffectTime(uint32_t time, int32_t entityID)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(time)
    {
        mNextEntityStatusTimes[time].insert(entityID);
    }
    else
    {
        for(auto pair : mNextEntityStatusTimes)
        {
            pair.second.erase(entityID);
        }
    }
}

std::list<std::shared_ptr<ActiveEntityState>>
    Zone::GetUpdatedStatusEffectEntities(uint32_t now)
{
    std::list<std::shared_ptr<ActiveEntityState>> result;
    std::set<uint32_t> passed;

    std::lock_guard<std::mutex> lock(mLock);
    for(auto pair : mNextEntityStatusTimes)
    {
        if(pair.first > now) break;

        passed.insert(pair.first);
        for(auto entityID : pair.second)
        {
            auto it = mAllEntities.find(entityID);
            auto active = it != mAllEntities.end()
                ? std::dynamic_pointer_cast<ActiveEntityState>(it->second)
                : nullptr;
            if(active)
            {
                result.push_back(active);
            }
        }
    }

    for(auto p : passed)
    {
        mNextEntityStatusTimes.erase(p);
    }

    return result;
}

bool Zone::GroupHasSpawned(uint32_t groupID, bool isLocation, bool aliveOnly)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto& m = isLocation ? mSpawnLocationGroups : mSpawnGroups;
    auto it = m.find(groupID);
    if(it == m.end())
    {
        return false;
    }
    else if(!aliveOnly)
    {
        return true;
    }

    for(auto eState : it->second)
    {
        if(eState->IsAlive())
        {
            return true;
        }
    }

    return false;
}

bool Zone::SpawnedAtSpot(uint32_t spotID)
{
    std::lock_guard<std::mutex> lock(mLock);
    return mSpotsSpawned.find(spotID) != mSpotsSpawned.end();
}

void Zone::CreateEncounter(
    const std::list<std::shared_ptr<ActiveEntityState>>& entities,
    std::shared_ptr<objects::ActionSpawn> spawnSource)
{
    if(entities.size() > 0)
    {
        std::lock_guard<std::mutex> lock(mLock);

        uint32_t encounterID = mNextEncounterID++;
        for(auto entity : entities)
        {
            auto eBase = entity->GetEnemyBase();
            if(eBase)
            {
                eBase->SetEncounterID(encounterID);
                mEncounters[encounterID].insert(entity);
            }
        }

        if(spawnSource)
        {
            mEncounterSpawnSources[encounterID] = spawnSource;
        }
    }

    for(auto entity : entities)
    {
        if(entity->GetEntityType() == EntityType_t::ENEMY)
        {
            AddEnemy(std::dynamic_pointer_cast<EnemyState>(entity));
        }
        else if(entity->GetEntityType() == EntityType_t::ALLY)
        {
            AddAlly(std::dynamic_pointer_cast<AllyState>(entity));
        }
    }
}

bool Zone::EncounterDefeated(uint32_t encounterID,
    std::shared_ptr<objects::ActionSpawn>& defeatActionSource)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mEncounters.find(encounterID);
    if(it != mEncounters.end())
    {
        for(auto eState : it->second)
        {
            if(eState->IsAlive())
            {
                return false;
            }
        }

        mEncounters.erase(encounterID);

        auto dIter = mEncounterSpawnSources.find(encounterID);
        if(dIter != mEncounterSpawnSources.end())
        {
            defeatActionSource = dIter->second;
        }
        else
        {
            defeatActionSource = nullptr;
        }
        mEncounterSpawnSources.erase(encounterID);

        return true;
    }

    return false;
}

std::set<int32_t> Zone::GetDespawnEntities()
{
    std::lock_guard<std::mutex> lock(mLock);
    return mPendingDespawnEntities;
}

std::set<uint32_t> Zone::GetDisabledSpawnGroups()
{
    std::lock_guard<std::mutex> lock(mLock);
    return mDisabledSpawnGroups;
}

void Zone::MarkDespawn(int32_t entityID)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(mAllEntities.find(entityID) != mAllEntities.end())
    {
        mPendingDespawnEntities.insert(entityID);
    }
}

bool Zone::UpdateTimedSpawns(const WorldClock& clock,
    bool initializing)
{
    bool updated = false;
    std::set<uint32_t> enableSpawnGroups;
    std::set<uint32_t> disableSpawnGroups;

    std::lock_guard<std::mutex> lock(mLock);
    for(auto sgPair : GetDefinition()->GetSpawnGroups())
    {
        auto sg = sgPair.second;
        auto restriction = sg->GetRestrictions();
        if(restriction)
        {
            if(TimeRestrictionActive(clock, restriction))
            {
                enableSpawnGroups.insert(sgPair.first);
            }
            else
            {
                disableSpawnGroups.insert(sgPair.first);
            }
        }
    }

    if(enableSpawnGroups.size() > 0)
    {
        EnableSpawnGroups(enableSpawnGroups, initializing);
    }

    if(disableSpawnGroups.size() > 0)
    {
        updated = DisableSpawnGroups(disableSpawnGroups,
            initializing);
    }

    for(auto pPair : GetDefinition()->GetPlasmaSpawns())
    {
        auto plasma = pPair.second;
        auto restriction = plasma->GetRestrictions();
        if(restriction)
        {
            if(TimeRestrictionActive(clock, restriction))
            {
                // Plasma active
                /// @todo
            }
            else
            {
                // Plasma inactive
                /// @todo
            }
        }
    }

    return updated;
}

bool Zone::EnableDisableSpawnGroups(const std::set<uint32_t>& spawnGroupIDs,
    bool enable)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(enable)
    {
        EnableSpawnGroups(spawnGroupIDs, false);
        return false;
    }
    else
    {
        return DisableSpawnGroups(spawnGroupIDs, false);
    }
}

std::set<uint32_t> Zone::GetRespawnLocations(uint64_t now)
{
    std::set<uint32_t> result;

    std::set<uint64_t> passed;

    std::lock_guard<std::mutex> lock(mLock);
    for(auto pair : mRespawnTimes)
    {
        if(pair.first > now) break;

        passed.insert(pair.first);

        for(uint32_t slgID : pair.second)
        {
            // Make sure we don't add the location twice
            if(result.find(slgID) == result.end() &&
                mSpawnLocationGroups[slgID].size() == 0)
            {
                result.insert(slgID);
            }
        }
    }

    for(auto p : passed)
    {
        mRespawnTimes.erase(p);
    }

    return result;
}

bool Zone::GetFlagState(int32_t key, int32_t& value, int32_t worldCID)
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
    Zone::GetFlagStates()
{
    std::lock_guard<std::mutex> lock(mLock);

    return mFlagStates;
}

int32_t Zone::GetFlagStateValue(int32_t key, int32_t nullDefault,
    int32_t worldCID)
{
    int32_t result;
    if(!GetFlagState(key, result, worldCID))
    {
        result = nullDefault;
    }

    return result;
}

void Zone::SetFlagState(int32_t key, int32_t value, int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);
    mFlagStates[worldCID][key] = value;
}

float Zone::GetXPMultiplier()
{
    auto def = GetDefinition();
    return def->GetXPMultiplier() +
        (mZoneInstance ? mZoneInstance->GetXPMultiplier() : 0.f);
}

std::unordered_map<size_t, std::shared_ptr<objects::Loot>>
    Zone::TakeLoot(std::shared_ptr<objects::LootBox> lBox, std::set<int8_t> slots,
    size_t freeSlots, std::unordered_map<uint32_t, uint16_t> stacksFree)
{
    std::unordered_map<size_t, std::shared_ptr<objects::Loot>> result;
    size_t ignoreCount = 0;

    std::lock_guard<std::mutex> lock(mLock);
    auto loot = lBox->GetLoot();
    for(size_t i = 0; (size_t)(result.size() - ignoreCount) < freeSlots &&
        i < lBox->LootCount(); i++)
    {
        auto l = loot[i];
        if(l && l->GetCount() > 0 &&
            (slots.size() == 0 || slots.find((int8_t)i) != slots.end()))
        {
            result[i] = l;
            loot[i] = nullptr;

            auto it = stacksFree.find(l->GetType());
            if(it != stacksFree.end() && it->second > 0)
            {
                // If there are existing stacks, determine if the loot
                // can be held in one of them
                if(it->second >= l->GetCount())
                {
                    stacksFree[l->GetType()] = (uint16_t)(
                        it->second - l->GetCount());
                    ignoreCount++;
                }
                else
                {
                    stacksFree[l->GetType()] = 0;
                }
            }
        }
    }
    lBox->SetLoot(loot);

    return result;
}

uint32_t Zone::GetNextRentalExpiration() const
{
    return mNextRentalExpiration;
}

uint32_t Zone::SetNextRentalExpiration()
{
    std::lock_guard<std::mutex> lock(mLock);

    // Start with no expiration
    mNextRentalExpiration = 0;

    // Set from bazaar markets
    for(auto& bState : mBazaars)
    {
        for(uint32_t marketID : bState->GetEntity()->GetMarketIDs())
        {
            auto market = bState->GetCurrentMarket(marketID);
            if(market && market->GetState() !=
                objects::BazaarData::State_t::BAZAAR_INACTIVE)
            {
                if(mNextRentalExpiration == 0 ||
                    mNextRentalExpiration > market->GetExpiration())
                {
                    mNextRentalExpiration = market->GetExpiration();
                }
            }
        }
    }

    // Set from culture machines
    for(auto& cmPair : mCultureMachines)
    {
        auto rental = cmPair.second->GetRentalData();
        if(rental && (mNextRentalExpiration == 0 ||
            mNextRentalExpiration > rental->GetExpiration()))
        {
            mNextRentalExpiration = rental->GetExpiration();
        }
    }

    return mNextRentalExpiration;
}

bool Zone::Collides(const Line& path, Point& point,
    Line& surface, std::shared_ptr<ZoneShape>& shape) const
{
    return mGeometry && mGeometry->Collides(path, point, surface, shape,
        GetDisabledBarriers());
}

bool Zone::Collides(const Line& path, Point& point) const
{
    Line surface;
    std::shared_ptr<ZoneShape> shape;
    return Collides(path, point, surface, shape);
}

void Zone::Cleanup()
{
    std::lock_guard<std::mutex> lock(mLock);
    for(auto pair : mAllEntities)
    {
        auto active = std::dynamic_pointer_cast<ActiveEntityState>(pair.second);
        if(active)
        {
            active->SetZone(0, false);
        }
    }

    mEnemies.clear();
    mNPCs.clear();
    mObjects.clear();
    mAllEntities.clear();
    mSpawnGroups.clear();
    mSpawnLocationGroups.clear();

    mZoneInstance = nullptr;

    // Zone is no longer valid for use
    SetInvalid(true);
}

bool Zone::TimeRestrictionActive(const WorldClock& clock,
    const std::shared_ptr<objects::SpawnRestriction>& restriction)
{
    // One of each designated restriction must be valid, compare
    // the most significant restrictions first
    if(restriction->DateRestrictionCount() > 0)
    {
        bool dateActive = false;

        uint16_t dateSum = (uint16_t)(clock.Month * 100 + clock.Day);
        for(auto pair : restriction->GetDateRestriction())
        {
            if(pair.first < pair.second)
            {
                // Normal compare
                dateActive = pair.first <= dateSum &&
                    dateSum <= pair.second;
            }
            else
            {
                // Rollover compare
                dateActive = pair.first <= dateSum ||
                    dateSum <= pair.second;
            }

            if(dateActive)
            {
                break;
            }
        }

        if(!dateActive)
        {
            return false;
        }
    }

    if(((restriction->GetDayRestriction() >> (clock.WeekDay - 1)) & 1) == 0)
    {
        return false;
    }

    if(restriction->SystemTimeRestrictionCount() > 0)
    {
        bool timeActive = false;

        uint16_t timeSum = (uint16_t)(clock.SystemHour * 100 + clock.SystemMin);
        for(auto pair : restriction->GetSystemTimeRestriction())
        {
            if(pair.first < pair.second)
            {
                // Normal compare
                timeActive = pair.first <= timeSum &&
                    timeSum <= pair.second;
            }
            else
            {
                // Rollover compare
                timeActive = pair.first <= timeSum ||
                    timeSum <= pair.second;
            }

            if(timeActive)
            {
                break;
            }
        }

        if(!timeActive)
        {
            return false;
        }
    }

    if(restriction->GetMoonRestriction() != 0xFFFF &&
        ((restriction->GetMoonRestriction() >> clock.MoonPhase) & 0x01) == 0)
    {
        return false;
    }

    if(restriction->TimeRestrictionCount() > 0)
    {
        bool timeActive = false;

        uint16_t timeSum = (uint16_t)(clock.Hour * 100 + clock.Min);
        for(auto pair : restriction->GetTimeRestriction())
        {
            if(pair.first < pair.second)
            {
                // Normal compare
                timeActive = pair.first <= timeSum &&
                    timeSum <= pair.second;
            }
            else
            {
                // Rollover compare
                timeActive = pair.first <= timeSum ||
                    timeSum <= pair.second;
            }

            if(timeActive)
            {
                break;
            }
        }

        if(!timeActive)
        {
            return false;
        }
    }

    return true;
}

void Zone::AddSpawnedEntity(const std::shared_ptr<ActiveEntityState>& state,
    uint32_t spotID, uint32_t sgID, uint32_t slgID)
{
    if(spotID != 0)
    {
        mSpotsSpawned.insert(spotID);
    }

    if(GetDefinition()->SpawnGroupsKeyExists(sgID))
    {
        mSpawnGroups[sgID].push_back(state);
    }

    auto slg = GetDefinition()->GetSpawnLocationGroups(slgID);
    if(slg)
    {
        mSpawnLocationGroups[slgID].push_back(state);

        // Be sure to clear the respawn time
        if(slg->GetRespawnTime() > 0.f)
        {
            for(auto pair : mRespawnTimes)
            {
                pair.second.erase(slgID);
            }
        }
    }
}

void Zone::EnableSpawnGroups(const std::set<uint32_t>& spawnGroupIDs,
    bool initializing)
{
    std::set<uint32_t> enabled;
    for(uint32_t sgID : spawnGroupIDs)
    {
        if(mDisabledSpawnGroups.find(sgID) != mDisabledSpawnGroups.end())
        {
            if(!initializing)
            {
                LOG_DEBUG(libcomp::String("Enabling spawn group %1 in"
                    " zone %2\n").Arg(sgID).Arg(GetDefinitionID()));
            }

            enabled.insert(sgID);
            mDisabledSpawnGroups.erase(sgID);
        }
    }

    if(enabled.size() == 0)
    {
        // Nothing to do
        return;
    }

    uint64_t now = ChannelServer::GetServerTime();

    // Re-enable SLGs and reset respawns
    enabled.clear();
    for(uint32_t slgID : mDisabledSpawnLocationGroups)
    {
        bool respawn = false;

        auto slg = GetDefinition()->GetSpawnLocationGroups(slgID);
        for(uint32_t sgID : slg->GetGroupIDs())
        {
            if(spawnGroupIDs.find(sgID) != spawnGroupIDs.end())
            {
                enabled.insert(slgID);

                respawn = slg->GetRespawnTime() > 0.f;
                break;
            }
        }

        if(respawn)
        {
            // Group respawns either immediately or after the respawn
            // period starting from now
            uint64_t rTime = now;
            if(!slg->GetImmediateSpawn())
            {
                rTime = now + (uint64_t)(
                    (double)slg->GetRespawnTime() * 1000000.0);
            }

            mRespawnTimes[rTime].insert(slgID);
        }
    }

    for(uint32_t slgID : enabled)
    {
        mDisabledSpawnLocationGroups.erase(slgID);
    }
}

bool Zone::DisableSpawnGroups(const std::set<uint32_t>& spawnGroupIDs,
    bool initializing)
{
    bool updated = false;

    std::set<uint32_t> disabled;
    for(uint32_t sgID : spawnGroupIDs)
    {
        if(mDisabledSpawnGroups.find(sgID) == mDisabledSpawnGroups.end())
        {
            auto gIter = mSpawnGroups.find(sgID);
            if(gIter != mSpawnGroups.end())
            {
                // Enemies are spawned, despawn
                for(auto eState : gIter->second)
                {
                    mPendingDespawnEntities.insert(eState->GetEntityID());
                    updated = true;
                }
            }

            if(!initializing)
            {
                LOG_DEBUG(libcomp::String("Disabling spawn group %1 in"
                    " zone %2\n").Arg(sgID).Arg(GetDefinitionID()));
            }

            mDisabledSpawnGroups.insert(sgID);
            disabled.insert(sgID);
        }
    }

    if(disabled.size() == 0)
    {
        return false;
    }

    // Disable SLGs and clear respawns
    disabled.clear();
    for(auto slgPair : GetDefinition()->GetSpawnLocationGroups())
    {
        auto slg = slgPair.second;
        if(mDisabledSpawnLocationGroups.find(slgPair.first) ==
            mDisabledSpawnLocationGroups.end())
        {
            // If no spawn group is active, de-activate
            bool disable = true;
            for(uint32_t sgID : slg->GetGroupIDs())
            {
                if(mDisabledSpawnGroups.find(sgID) ==
                    mDisabledSpawnGroups.end())
                {
                    disable = false;
                    break;
                }
            }

            if(disable)
            {
                disabled.insert(slg->GetID());
            }
        }
    }

    if(disabled.size() > 0)
    {
        std::set<uint64_t> clearTimes;
        for(uint32_t slgID : disabled)
        {
            mDisabledSpawnLocationGroups.insert(slgID);
            for(auto& pair : mRespawnTimes)
            {
                pair.second.erase(slgID);
                if(pair.second.size() == 0)
                {
                    clearTimes.insert(pair.first);
                }
            }

            for(uint64_t t : clearTimes)
            {
                mRespawnTimes.erase(t);
            }
            clearTimes.clear();
        }
    }

    return updated;
}
