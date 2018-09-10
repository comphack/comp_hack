/**
 * @file server/channel/src/MatchManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief 
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

#include "MatchManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ChannelConfig.h>
#include <MatchEntry.h>
#include <PvPInstanceVariant.h>
#include <PvPBase.h>
#include <PvPData.h>
#include <PvPInstanceStats.h>
#include <PvPInstanceVariant.h>
#include <PvPPlayerStats.h>
#include <PvPMatch.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <Team.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "ZoneManager.h"

using namespace channel;

MatchManager::MatchManager(const std::weak_ptr<
    ChannelServer>& server) : mServer(server)
{
}

MatchManager::~MatchManager()
{
}

std::shared_ptr<objects::MatchEntry> MatchManager::GetMatchEntry(int32_t cid)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mMatchEntries.find(cid);
    return it != mMatchEntries.end() ? it->second : nullptr;
}

bool MatchManager::JoinQueue(const std::shared_ptr<
    ChannelClientConnection>& client, int8_t type)
{
    auto state = client->GetClientState();
    int32_t cid = state->GetWorldCID();

    auto team = state->GetTeam();
    if(team && team->GetLeaderCID() != cid)
    {
        // Not the leader of the team
        return false;
    }

    bool valid = false;
    int8_t teamCategory = -1;
    switch((objects::MatchEntry::MatchType_t)type)
    {
    case objects::MatchEntry::MatchType_t::PVP_FATE:
    case objects::MatchEntry::MatchType_t::PVP_VALHALLA:
        valid = !team || team->GetType() == 0;
        teamCategory = (int8_t)objects::Team::Category_t::PVP;
        break;
    default:
        break;
    }

    if(!valid)
    {
        return false;
    }

    auto server = mServer.lock();

    int32_t teamID = 0;
    std::list<std::shared_ptr<ChannelClientConnection>> teamClients;
    if(team)
    {
        auto managerConnection = server->GetManagerConnection();
        for(int32_t memberCID : team->GetMemberIDs())
        {
            auto teamClient = managerConnection->GetEntityClient(
                memberCID, true);
            if(!teamClient)
            {
                return false;
            }

            teamClients.push_back(teamClient);
        }

        teamID = team->GetID();
    }
    else
    {
        teamClients.push_back(client);
    }

    if(!ValidateMatchEntries(teamClients, teamCategory, teamID != 0, true))
    {
        return false;
    }

    // Create entries for all members
    auto syncManager = server->GetChannelSyncManager();
    bool queued = false;
    for(auto teamClient : teamClients)
    {
        auto entry = std::make_shared<objects::MatchEntry>();
        entry->SetWorldCID(teamClient->GetClientState()->GetWorldCID());
        entry->SetOwnerCID(cid);
        entry->SetMatchType((objects::MatchEntry::MatchType_t)type);
        entry->SetTeamID(teamID);

        queued |= syncManager->UpdateRecord(entry, "MatchEntry");
    }

    if(queued)
    {
        syncManager->SyncOutgoing();
    }
    else
    {
        LOG_WARNING(libcomp::String("One or more match entries failed to"
            " queue for account: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    return true;
}

bool MatchManager::CancelQueue(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    if(client->GetClientState()->GetPvPMatch())
    {
        // In a match, too late to cancel
        return false;
    }

    auto state = client->GetClientState();
    std::set<int32_t> allCIDs = { state->GetWorldCID() };

    auto team = state->GetTeam();
    if(team)
    {
        if(team->GetLeaderCID() != state->GetWorldCID())
        {
            // Not the leader of the team
            return false;
        }

        for(int32_t memberCID : team->GetMemberIDs())
        {
            allCIDs.insert(memberCID);
        }
    }

    auto server = mServer.lock();
    auto syncManager = server->GetChannelSyncManager();

    bool queued = false;
    for(int32_t cid : allCIDs)
    {
        auto entry = GetMatchEntry(cid);
        if(entry)
        {
            queued |= syncManager->RemoveRecord(entry, "MatchEntry");
        }
    }

    if(queued)
    {
        syncManager->SyncOutgoing();
    }
    else
    {
        LOG_WARNING(libcomp::String("One or more match cancellations failed"
            " to send to the world for account: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    return true;
}

void MatchManager::ConfirmMatch(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto match = state->GetPvPMatch();

    bool success = match && PvPInviteReply(client, match->GetID(), true);

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_PVP_CONFIRM);
    reply.WriteS8(0);   // Confirmed
    reply.WriteS32Little(success ? 0 : -1);

    client->SendPacket(reply);

    if(success)
    {
        MoveToInstance(client, match);
    }
}

void MatchManager::RejectMatch(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto match = state->GetPvPMatch();

    bool success = match && PvPInviteReply(client, match->GetID(), false);

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_PVP_CONFIRM);
    reply.WriteS8(1);   // Rejected
    reply.WriteS32Little(success ? 0 : -1);

    client->SendPacket(reply);
}

void MatchManager::LeaveTeam(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t teamID)
{
    auto state = client->GetClientState();

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_TEAM_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE);
    request.WriteS32Little(teamID);
    request.WriteS32Little(state->GetWorldCID());

    mServer.lock()->GetManagerConnection()->GetWorldConnection()->SendPacket(
        request);
}

bool MatchManager::RequestTeamPvPMatch(
    const std::shared_ptr<ChannelClientConnection>& client,
    uint32_t variantID, uint32_t instanceID)
{
    auto state = client->GetClientState();
    auto team = state->GetTeam();
    if(!team || team->GetLeaderCID() != state->GetWorldCID())
    {
        LOG_DEBUG(libcomp::String("Team PvP creation failed: requestor"
            " is not the team leader: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    auto server = mServer.lock();
    auto managerConnection = server->GetManagerConnection();

    std::list<std::shared_ptr<ChannelClientConnection>> teamClients;
    for(int32_t memberCID : team->GetMemberIDs())
    {
        auto teamClient = managerConnection->GetEntityClient(
            memberCID, true);
        if(!teamClient)
        {
            LOG_DEBUG(libcomp::String("Team PvP creation failed: one or more"
                " team members is not on the channel: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }

        teamClients.push_back(teamClient);
    }

    if(!ValidateMatchEntries(teamClients,
        (int8_t)objects::Team::Category_t::PVP, true, false))
    {
        LOG_DEBUG(libcomp::String("Team PvP creation failed: one or more team"
            " members is not valid: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    // Team is valid, validate variant and build match
    auto variant = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
        server->GetServerDataManager()->GetZoneInstanceVariantData(variantID));
    if(!variant)
    {
        LOG_DEBUG(libcomp::String("Team PvP creation failed: invalid variant"
            " specified for instance: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    if(variant->GetMaxPlayers() &&
        teamClients.size() > (size_t)variant->GetMaxPlayers())
    {
        LOG_DEBUG(libcomp::String("Team PvP creation failed: too many players"
            " are in the current team: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    if(!instanceID)
    {
        instanceID = variant->GetDefaultInstanceID();
    }

    auto instance = server->GetServerDataManager()->GetZoneInstanceData(
        instanceID);
    if(!instance)
    {
        LOG_DEBUG(libcomp::String("Team PvP creation failed: invalid"
            " instance requested: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    auto match = std::make_shared<objects::PvPMatch>();
    match->SetType((int8_t)variant->GetMatchType());
    match->SetVariantID(variantID);
    match->SetInstanceDefinitionID(instanceID);
    match->SetNoQueue(true);

    std::set<int32_t> cids;
    for(auto teamClient : teamClients)
    {
        cids.insert(teamClient->GetClientState()->GetWorldCID());
    }

    size_t teamSize = cids.size();
    for(size_t i = 0; i < teamSize; i++)
    {
        int32_t cid = libcomp::Randomizer::GetEntry(cids);
        cids.erase(cid);

        if(i % 2 == 0)
        {
            match->AppendBlueMemberIDs(cid);
        }
        else
        {
            match->AppendRedMemberIDs(cid);
        }
    }

    if(variant->GetLimitBlue())
    {
        // Shift entries from blue to red
        while(match->BlueMemberIDsCount() > variant->GetLimitBlue())
        {
            size_t last = (size_t)(match->BlueMemberIDsCount() - 1);
            match->AppendRedMemberIDs(match->GetBlueMemberIDs(last));
            match->RemoveBlueMemberIDs(last);
        }
    }

    if(variant->GetLimitRed())
    {
        // Shift entries from red to blue
        while(match->RedMemberIDsCount() > variant->GetLimitRed())
        {
            size_t last = (size_t)(match->RedMemberIDsCount() - 1);
            match->AppendBlueMemberIDs(match->GetRedMemberIDs(last));
            match->RemoveRedMemberIDs(last);
        }

        if(variant->GetLimitBlue() &&
            match->BlueMemberIDsCount() >= variant->GetLimitBlue())
        {
            LOG_DEBUG(libcomp::String("Team PvP creation failed: team size"
                " restrictions could not be met: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }
    }

    LOG_DEBUG(libcomp::String("Requesting team PvP match with variant %1 and"
        " instance %2: %3\n").Arg(variantID).Arg(instanceID)
        .Arg(client->GetClientState()->GetAccountUID().ToString()));

    auto syncManager = server->GetChannelSyncManager();
    if(!syncManager->UpdateRecord(match, "PvPMatch"))
    {
        LOG_DEBUG(libcomp::String("Team PvP creation failed: match could"
            " not be queued: %1\n").Arg(state->GetAccountUID().ToString()));
        return false;
    }

    syncManager->SyncOutgoing();
    return true;
}

void MatchManager::ExpirePvPAccess(uint32_t matchID)
{
    std::set<int32_t> cids;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mPendingPvPInvites.find(matchID);
        if(it != mPendingPvPInvites.end())
        {
            cids = it->second;
        }
    }

    if(cids.size() > 0)
    {
        LOG_DEBUG(libcomp::String("Expiring %1 unconfirmed PvP player(s)\n")
            .Arg(cids.size()));

        auto server = mServer.lock();
        auto managerConnection = server->GetManagerConnection();
        for(int32_t cid : cids)
        {
            auto client = managerConnection->GetEntityClient(cid, true);
            if(client)
            {
                PvPInviteReply(client, matchID, false);
            }
        }
    }
}

bool MatchManager::PvPInviteReply(const std::shared_ptr<
    ChannelClientConnection>& client, uint32_t matchID, bool accept)
{
    auto state = client->GetClientState();
    auto match = state->GetPvPMatch();
    int32_t worldCID = state->GetWorldCID();

    // Always clear match at this point
    if(match && match->GetID() == matchID)
    {
        state->SetPvPMatch(nullptr);
    }

    std::lock_guard<std::mutex> lock(mLock);
    auto it = mPendingPvPInvites.find(matchID);
    if(it != mPendingPvPInvites.end() &&
        it->second.find(worldCID) != it->second.end())
    {
        if(!accept)
        {
            // Raise penalty count
            auto cState = state->GetCharacterState();
            auto character = cState->GetEntity();
            auto pvpData = character->GetPvPData().Get();
            if(pvpData)
            {
                auto opChangeset = std::make_shared<
                    libcomp::DBOperationalChangeSet>(state->GetAccountUID());
                auto expl = std::make_shared<libcomp::DBExplicitUpdate>(
                    pvpData);
                expl->Add("PenaltyCount", 1);
                opChangeset->AddOperation(expl);

                if(!mServer.lock()->GetWorldDatabase()->ProcessChangeSet(
                    opChangeset))
                {
                    LOG_ERROR(libcomp::String("Failed to apply PvP"
                        " penalty: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                }
            }
        }

        it->second.erase(worldCID);
        if(it->second.size() == 0)
        {
            mPendingPvPInvites.erase(it);
        }

        return true;
    }

    return false;
}

std::shared_ptr<objects::PvPData> MatchManager::GetPvPData(
    const std::shared_ptr<ChannelClientConnection>& client, bool create)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    if(!character)
    {
        return nullptr;
    }

    if(character->GetPvPData().IsNull() && create)
    {
        std::lock_guard<std::mutex> lock(mLock);

        // Check again after mutex lock
        if(!character->GetPvPData().IsNull())
        {
            return character->GetPvPData().Get();
        }

        auto dbChanges = libcomp::DatabaseChangeSet::Create(state
            ->GetAccountUID());

        auto pvpData = libcomp::PersistentObject::New<objects::PvPData>(true);
        pvpData->SetCharacter(character->GetUUID());

        character->SetPvPData(pvpData);

        dbChanges->Insert(pvpData);
        dbChanges->Update(character);

        if(!mServer.lock()->GetWorldDatabase()->ProcessChangeSet(dbChanges))
        {
            // Rollback but don't kill the client
            character->SetPvPData(NULLUUID);
        }
    }

    return character->GetPvPData().Get();
}

void MatchManager::StartPvPMatch(uint32_t instanceID)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto instance = zoneManager->GetInstance(instanceID);
    auto pvpStats = instance ? instance->GetPvPStats() : nullptr;
    if(!pvpStats)
    {
        // Not a PvP instance
        return;
    }

    LOG_DEBUG(libcomp::String("Starting PvP match %1\n").Arg(pvpStats
        ->GetMatch()->GetID()));

    // Players in the match, either end if only one team is here or start
    // the match and queue up complete action
    if(MatchTeamsActive(instance))
    {
        // Start the match
        zoneManager->StartInstanceTimer(instance);

        // Fire match start event in all current zones (should be one)
        for(auto& zdPair : instance->GetZones())
        {
            for(auto& zPair : zdPair.second)
            {
                zoneManager->TriggerZoneActions(zPair.second, {},
                    ZoneTrigger_t::ON_PVP_START, nullptr);
            }
        }

        // If for some reason a player is dead when the match starts,
        // auto-revive them to prevent any player shenanigans
        auto characterManager = server->GetCharacterManager();
        for(auto client : instance->GetConnections())
        {
            auto cState = client->GetClientState()->GetCharacterState();
            if(cState && !cState->IsAlive())
            {
                characterManager->ReviveCharacter(client, REVIVE_PVP_RESPAWN);
            }
        }
    }
    else
    {
        // End the match immediately
        zoneManager->StopInstanceTimer(instance);
    }
}

bool MatchManager::EndPvPMatch(uint32_t instanceID)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto instance = zoneManager->GetInstance(instanceID);
    auto pvpStats = instance ? instance->GetPvPStats() : nullptr;
    auto match = pvpStats ? pvpStats->GetMatch() : nullptr;
    auto variant = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
        instance ? instance->GetVariant() : nullptr);
    if(!match || PvPActive(instance))
    {
        // Timer needs to have already stopped to send end notification
        return false;
    }

    LOG_DEBUG(libcomp::String("Ending PvP match %1\n").Arg(match->GetID()));

    // Determine trophies and calculate BP/GP
    size_t idx = 0;
    auto db = server->GetWorldDatabase();

    std::array<std::list<std::shared_ptr<objects::PvPPlayerStats>>, 2> teams;
    std::unordered_map<int32_t, std::shared_ptr<ActiveEntityState>> inMatch;
    std::array<bool, 2> teamQuit = { { true, true } };
    for(auto cids : { match->GetBlueMemberIDs(), match->GetRedMemberIDs() })
    {
        for(int32_t cid : cids)
        {
            for(auto& pair : pvpStats->GetPlayerStats())
            {
                auto stats = pair.second;
                if(stats->GetWorldCID() == cid &&
                    stats->GetCharacter().Get(db))
                {
                    teams[idx].push_back(stats);

                    // Determine if they are still in the instance now that
                    // the match has ended
                    for(auto& zdPair : instance->GetZones())
                    {
                        for(auto& zPair : zdPair.second)
                        {
                            auto eState = zPair.second->GetActiveEntity(
                                stats->GetEntityID());
                            if(eState)
                            {
                                inMatch[eState->GetEntityID()] = eState;
                                teamQuit[idx] = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        idx++;
    }

    bool noMatch = instance->GetTimerStart() == 0 ||
        (teamQuit[0] && teamQuit[1]);
    int32_t timeLeft = noMatch ? 0 : (int32_t)((instance->GetTimerStop() -
        instance->GetTimerStart()) / 1000000ULL);
    if(timeLeft < 0)
    {
        timeLeft = 0;
    }

    if(!noMatch)
    {
        GetPvPTrophies(instance);
    }

    std::unordered_map<int32_t, int32_t> matchGP;
    std::unordered_map<int32_t, int32_t> matchGPAdjust;
    
    for(idx = 0; idx < 2; idx++)
    {
        for(auto stats : teams[idx])
        {
            auto character = stats->GetCharacter().Get();
            auto pvpData = character
                ? character->GetPvPData().Get(db) : nullptr;

            int32_t gp = pvpData ? pvpData->GetGP() : 0;
            matchGP[stats->GetEntityID()] = gp;
            matchGPAdjust[stats->GetEntityID()] = gp +
                (pvpData && pvpData->GetRanked() ? 1000 : 0);
        }
    }

    std::unordered_map<int32_t, uint64_t> xpGain;
    auto dbChanges = libcomp::DatabaseChangeSet::Create();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PVP_RESULT);
    for(idx = 0; idx < 2; idx++)
    {
        size_t otherIdx = idx == 0 ? 1 : 0;

        int32_t result = 0;
        if(!noMatch)
        {
            if(teamQuit[otherIdx])
            {
                // Win by default
                result = 1;
            }
            else if(teamQuit[idx])
            {
                // Lose by default
                result = -1;
            }
            else if(pvpStats->GetPoints(idx) > pvpStats->GetPoints(otherIdx))
            {
                // Win
                result = 1;
            }
            else if(pvpStats->GetPoints(idx) < pvpStats->GetPoints(otherIdx))
            {
                // Lose
                result = -1;
            }
        }

        int32_t teamBasePoints = 0;
        for(auto stats : teams[idx])
        {
            teamBasePoints += stats->GetBasePoints() +
                stats->GetBaseBonusPoints();
        }

        bool lost = result == -1;

        p.WriteS32Little(result);
        p.WriteU16Little(pvpStats->GetPoints(idx));

        p.WriteS8((int8_t)teams[idx].size());

        for(auto stats : teams[idx])
        {
            auto character = stats->GetCharacter().Get();
            auto cs = character ? character->GetCoreStats().Get(db) : nullptr;

            // Always reload
            auto pvpData = character ? character->LoadPvPData(db) : nullptr;

            auto eState = inMatch[stats->GetEntityID()];
            xpGain[stats->GetEntityID()] = 0;

            int32_t oldGP = matchGP[stats->GetEntityID()];
            int32_t newGP = matchGP[stats->GetEntityID()];
            int32_t bpGained = 0;
            bool ranked = pvpData && pvpData->GetRanked();
            if(pvpData && !noMatch)
            {
                int32_t kills = (int32_t)(stats->GetKills() +
                    stats->GetDemonKills());
                int32_t deaths = (int32_t)(stats->GetDeaths() +
                    stats->GetDemonDeaths());
                if(variant)
                {
                    // Determine GP/ranked
                    if(!ranked)
                    {
                        // Pre-rank rates are fixed for win or loss
                        newGP = (int32_t)(newGP +
                            (int32_t)variant->GetPreRankedGP(lost ? 1 : 0));
                    }
                    else if(variant->GetRankedGPRate() > 0.f)
                    {
                        // Rank rates are based on the other team's GP,
                        // adjusted to be slightly higher or lower ranked
                        // according to the outcome so same rank teams are
                        // able to still gain/lose points

                        // Weight results based on team size difference
                        float weight = 1.f + (0.2f * ((float)
                            teams[otherIdx].size() - (float)teams[idx].size()));
                        if(weight < 0.5f)
                        {
                            weight = 0.5f;
                        }
                        else if(weight > 1.5f)
                        {
                            weight = 1.5f;
                        }

                        int32_t gpAdjust = matchGPAdjust[stats->GetEntityID()];

                        double calc = 0.0;
                        for(auto oStats : teams[otherIdx])
                        {
                            auto gp = matchGPAdjust[oStats->GetEntityID()];
                            int32_t gpSkew = (gp / 20) * (lost ? -1 : 1);

                            calc += (((double)gp * (double)weight) -
                                (double)gpAdjust + (double)gpSkew);
                        }

                        calc = calc / 5.0;

                        if((calc > 0.0 && !lost) || (calc < 0.0 && lost))
                        {
                            newGP += (int32_t)calc;
                            if(newGP < 0)
                            {
                                newGP = 0;
                            }
                        }
                    }

                    // GP can only lower if the player dropped
                    if(newGP > oldGP && !eState)
                    {
                        newGP = oldGP;
                    }

                    // Determine BP
                    if(!lost || !variant->GetBPWinRequired())
                    {
                        // Normal BP is determined by fixed amount and time
                        // left on the clock multiplied by the modifier
                        double calc = (double)variant->GetFixedReward();
                        if(timeLeft && variant->GetRewardModifier())
                        {
                            calc += (double)variant->GetRewardModifier() *
                                (double)timeLeft /
                                (double)variant->GetTimePoints(0);
                        }

                        // BP is scaled by kill/death values
                        double kdScale = (double)variant
                            ->GetBPKillDeathScale();
                        if(kdScale > 0.0)
                        {
                            calc = calc * (1.0 + ((double)kills * 0.10) -
                                ((double)deaths * 0.05));

                            // Result can be negative which should not affect
                            // the final number
                            if(calc < 0.0)
                            {
                                calc = 0.0;
                            }
                        }

                        // BP is then scaled by trophy boosts
                        calc *= 1.0 + (double)stats->GetTrophyBoost();

                        // Finally base points are added (if bases are involved
                        // in the match)
                        double basePoints = (double)(stats->GetBasePoints() +
                            stats->GetBaseBonusPoints());
                        double baseScale = (lost ? 20.0 : 6.0);
                        calc += ((double)teamBasePoints + basePoints) /
                            baseScale * (1.0 + (double)stats->GetTrophyBoost());

                        if(calc > 0.0)
                        {
                            bpGained = (int32_t)calc;
                        }
                    }

                    // Determine XP
                    if(eState)
                    {
                        int64_t xp = (int64_t)variant->GetXPReward();
                        if(lost)
                        {
                            xp = xp / 5;
                        }

                        xp = (int64_t)ceil((double)xp * ((double)
                            eState->GetCorrectValue(CorrectTbl::RATE_XP) *
                            0.01));

                        xpGain[stats->GetEntityID()] = (uint64_t)xp;
                    }
                }

                // Set stats
                pvpData->SetKillTotal(kills + pvpData->GetKillTotal());
                pvpData->SetDeathTotal(deaths + pvpData->GetDeathTotal());

                if(match->GetType() == 0 || match->GetType() == 1)
                {
                    // 0: Win, 1: Lose, 2: Draw
                    size_t i = (result == -1 ? 1 : (result == 0 ? 2 : 0));
                    i = (size_t)(i + (size_t)(match->GetType() * 3));

                    pvpData->SetModeStats(i, pvpData->GetModeStats(i) + 1);
                }

                for(int8_t trophy : stats->GetTrophies())
                {
                    if(trophy && trophy <= (int8_t)pvpData->TrophiesCount())
                    {
                        size_t i = (size_t)(trophy - 1);
                        pvpData->SetTrophies(i, pvpData->GetTrophies(i) + 1);
                    }
                }

                // Set GP and ranked, adjusting for limits
                if(oldGP != newGP)
                {
                    if(newGP >= 1000 && !pvpData->GetRanked())
                    {
                        pvpData->SetRanked(true);
                        ranked = true;
                        oldGP = oldGP - 1000;   // Display difference still
                        newGP = 0;
                    }
                    else if(newGP < 0)
                    {
                        newGP = 0;
                    }
                    else if(ranked && newGP > 2000)
                    {
                        newGP = 2000;
                    }

                    pvpData->SetGP(newGP);
                }

                if(!match->GetNoQueue())
                {
                    // Drop penalty count by 1 for a completed match or raise by
                    // 1 if the player dropped
                    uint8_t penaltyCount = pvpData->GetPenaltyCount();
                    if(!eState)
                    {
                        pvpData->SetPenaltyCount((uint8_t)(penaltyCount + 1));
                    }
                    else if(penaltyCount > 0)
                    {
                        pvpData->SetPenaltyCount((uint8_t)(penaltyCount - 1));
                    }
                }

                // BP cannot be gained if the player dropped
                if(bpGained && eState)
                {
                    pvpData->SetBP(bpGained + pvpData->GetBP());
                    pvpData->SetBPTotal(bpGained + pvpData->GetBPTotal());
                }

                dbChanges->Update(pvpData);
            }

            p.WriteS8(cs ? cs->GetLevel() : 0);
            p.WriteS32Little(oldGP);
            p.WriteS32Little(newGP);
            p.WriteS8(ranked ? 1 : 0);
            p.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                character ? character->GetName() : "", true);
            p.WriteU16Little((uint16_t)(stats->GetKills() +
                stats->GetDemonKills()));
            p.WriteU16Little(stats->GetDemonKills());
            p.WriteU16Little((uint16_t)(stats->GetDeaths() +
                stats->GetDemonDeaths()));
            p.WriteU16Little(stats->GetDemonDeaths());
            p.WriteS32Little(bpGained);

            p.WriteS32Little((int32_t)stats->TrophiesCount());
            for(int8_t trophy : stats->GetTrophies())
            {
                p.WriteS8(trophy);
            }

            p.WriteS32Little((int32_t)xpGain[stats->GetEntityID()]);
        }
    }

    ChannelClientConnection::BroadcastPacket(instance->GetConnections(), p);

    // Save updates
    if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
    {
        LOG_ERROR("Failed to save one or more PvP match results");
    }

    // Lastly grant XP
    auto characterManager = server->GetCharacterManager();
    auto managerConnection = server->GetManagerConnection();
    for(auto& xpPair : xpGain)
    {
        auto client = managerConnection->GetEntityClient(xpPair.first, false);
        if(client)
        {
            characterManager->ExperienceGain(client, xpPair.second,
                xpPair.first);
        }
    }

    return true;
}

void MatchManager::EnterPvP(const std::shared_ptr<
    ChannelClientConnection>& client, uint32_t instanceID)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto instance = zoneManager->GetInstance(instanceID);
    auto pvpStats = instance ? instance->GetPvPStats() : nullptr;
    auto match = pvpStats ? pvpStats->GetMatch() : nullptr;
    auto variant = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
        instance ?instance->GetVariant() : nullptr);
    if(!match || !variant)
    {
        return;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    if(!InPvPTeam(cState))
    {
        // Set up the player
        int32_t worldCID = state->GetWorldCID();
        auto dState = state->GetDemonState();

        // Determine PvP faction group
        int32_t factionGroup = 0;
        for(int32_t memberCID : match->GetBlueMemberIDs())
        {
            if(memberCID == worldCID)
            {
                // Add to blue team
                factionGroup = 1;
                break;
            }
        }

        for(int32_t memberCID : match->GetRedMemberIDs())
        {
            if(memberCID == worldCID)
            {
                // Add to red team
                factionGroup = 2;
                break;
            }
        }

        cState->SetFactionGroup(factionGroup);
        dState->SetFactionGroup(factionGroup);

        // Set entity values
        if(variant->GetPlayerValue())
        {
            pvpStats->SetEntityValues(cState->GetEntityID(),
                variant->GetPlayerValue());
        }

        if(variant->GetDemonValue())
        {
            pvpStats->SetEntityValues(dState->GetEntityID(),
                variant->GetDemonValue());
        }

        // Add player stats
        auto stats = pvpStats->GetPlayerStats(cState->GetEntityID());
        if(!stats)
        {
            stats = std::make_shared<objects::PvPPlayerStats>();
            stats->SetCharacter(cState->GetEntity());
            stats->SetEntityID(cState->GetEntityID());
            stats->SetWorldCID(state->GetWorldCID());

            pvpStats->SetPlayerStats(cState->GetEntityID(), stats);
        }
    }

    size_t pIdx = 0;
    std::array<std::list<std::shared_ptr<CharacterState>>, 2> teams;
    for(auto cids : { match->GetBlueMemberIDs(), match->GetRedMemberIDs() })
    {
        for(auto cid : cids)
        {
            auto tState = ClientState::GetEntityClientState(cid, true);
            auto tZone = tState ? tState->GetZone() : nullptr;
            if(tZone && tZone->GetInstance() == instance)
            {
                teams[pIdx].push_back(tState->GetCharacterState());
            }
        }

        pIdx++;
    }

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PVP_START);
    p.WriteS8(match->GetType());
    p.WriteS8((int8_t)cState->GetFactionGroup());   // Overhead team display
    p.WriteS8((int8_t)(cState->GetFactionGroup() - 1)); // Team sent in console

    // Timer is not typically actually started at this point
    ServerTime startTime = instance->GetTimerExpire() -
        (uint64_t)((uint64_t)variant->GetTimePoints(0) * 1000000ULL);

    p.WriteS32Little((int32_t)state->ToClientTime(startTime));
    p.WriteS32Little((int32_t)state->ToClientTime(instance->GetTimerExpire()));

    for(auto& team : teams)
    {
        p.WriteS8((int8_t)team.size());

        for(auto teamChar : team)
        {
            auto character = teamChar->GetEntity();
            p.WriteS32Little(teamChar->GetEntityID());
            p.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                character->GetName(), true);
            p.WriteS8(teamChar->GetLevel());

            auto pvpData = character->GetPvPData().Get();
            p.WriteS32(pvpData ? pvpData->GetGP() : 0);
            p.WriteS8(pvpData && pvpData->GetRanked() ? 1 : 0);
        }
    }

    auto bases = state->GetZone()->GetPvPBases();

    p.WriteS8((int8_t)bases.size());
    for(auto bState : bases)
    {
        auto base = bState->GetEntity();

        p.WriteS32Little(bState->GetEntityID());
        p.WriteFloat(bState->GetCurrentX());
        p.WriteFloat(bState->GetCurrentY());
        p.WriteU8(base->GetRank());
        p.WriteU8(base->GetSpeed());
    }

    client->SendPacket(p);

    SendPvPLocation(client, instanceID, true);
}

bool MatchManager::CaptureBase(
    const std::shared_ptr<ChannelClientConnection>& client, int32_t baseID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto zone = state->GetZone();
    if(!zone)
    {
        return false;
    }

    auto bState = zone->GetPvPBase(baseID);
    int32_t errorCode = zone->OccupyPvPBase(baseID, cState->GetEntityID(),
        false);
    if(errorCode == 0)
    {
        if(!mServer.lock()->GetEventManager()->StartSystemEvent(client,
            baseID))
        {
            // Cancel occupation
            zone->OccupyPvPBase(baseID, -1, true);
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_PVP_BASE_CAPTURE);
    reply.WriteS32Little(baseID);
    reply.WriteS32Little(errorCode);

    if(errorCode == 0)
    {
        reply.WriteS32Little(0); // Unknown
        reply.WriteS32Little(cState->GetEntityID());

        auto server = mServer.lock();
        server->GetZoneManager()->BroadcastPacket(zone, reply);

        // Bases take 5 seconds to capture
        server->GetTimerManager()->ScheduleEventIn(5, []
            (MatchManager* pMatchManager, int32_t pEntityID, int32_t pBaseID,
                uint32_t pZoneID, uint32_t pInstanceID,
                uint64_t pOccupyStartTime)
            {
                pMatchManager->CompleteBaseCapture(pEntityID, pBaseID,
                    pZoneID, pInstanceID, pOccupyStartTime);
            }, this, cState->GetEntityID(), baseID, zone->GetID(),
                zone->GetInstance()->GetID(), bState->GetEntity()
                ->GetOccupyTime());
    }
    else
    {
        client->SendPacket(reply);
    }

    return errorCode == 0;
}

bool MatchManager::LeaveBase(
    const std::shared_ptr<ChannelClientConnection>& client, int32_t baseID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto zone = state->GetZone();
    if(!zone)
    {
        return false;
    }

    int32_t sourceEntityID = state->GetEventSourceEntityID();
    auto bState = zone->GetPvPBase(baseID);
    if(bState)
    {
        int32_t errorCode = zone->OccupyPvPBase(baseID, -1, true);
        if(errorCode == 0)
        {
            auto server = mServer.lock();
            if(sourceEntityID == baseID)
            {
                // End the event first
                server->GetEventManager()->HandleEvent(client, nullptr);
            }

            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_PVP_BASE_LEFT);
            notify.WriteS32Little(baseID);
            notify.WriteS32Little(-1);
            notify.WriteS32Little(cState->GetEntityID());

            server->GetZoneManager()->BroadcastPacket(zone, notify);

            return true;
        }
    }

    return false;
}

void MatchManager::CompleteBaseCapture(int32_t entityID, int32_t baseID,
    uint32_t zoneID, uint32_t instanceID, uint64_t occupyStartTime)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto instance = zoneManager->GetInstance(instanceID);
    auto zone = instance ? instance->GetZone(zoneID) : nullptr;
    if(!zone)
    {
        return;
    }

    auto bState = zone->GetPvPBase(baseID);
    int32_t errorCode = zone->OccupyPvPBase(baseID, entityID, true,
        occupyStartTime);
    if(errorCode == 0)
    {
        // Queue the bonus right away
        QueueNextBaseBonus(baseID, zone, occupyStartTime);

        auto base = bState->GetEntity();

        uint8_t teamID = (uint8_t)(base->GetTeam());

        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_PVP_BASE_CAPTURED);
        notify.WriteS32Little(baseID);
        notify.WriteS32Little((int32_t)teamID);
        notify.WriteS32Little(entityID);

        zoneManager->BroadcastPacket(zone, notify);

        if(PvPActive(instance))
        {
            int32_t points = (int32_t)bState->GetEntity()->GetRank();
            points = UpdatePvPPoints(instance->GetID(), entityID, baseID,
                teamID, points, false);

            auto pvpStats = instance->GetPvPStats();
            if(points > 0)
            {
                if(!pvpStats->GetBaseFirstOwner())
                {
                    pvpStats->SetBaseFirstOwner(entityID);
                }

                auto stats = pvpStats->GetPlayerStats(entityID);
                if(stats)
                {
                    stats->SetBasePoints((uint16_t)(points +
                        stats->GetBasePoints()));
                }
            }

            // Fire base capture triggers if any
            auto triggers = zoneManager->GetZoneTriggers(zone,
                ZoneTrigger_t::ON_PVP_BASE_CAPTURE);
            if(triggers.size() > 0)
            {
                // Randomly select a team member and start the trigger
                auto match = pvpStats->GetMatch();

                std::set<std::shared_ptr<CharacterState>> team;
                for(int32_t memberID : teamID == 0 ? match->GetBlueMemberIDs()
                    : match->GetRedMemberIDs())
                {
                    auto state = ClientState::GetEntityClientState(memberID,
                        true);
                    if(state && state->GetZone() == zone)
                    {
                        team.insert(state->GetCharacterState());
                    }
                }

                if(team.size() > 0)
                {
                    auto member = libcomp::Randomizer::GetEntry(team);
                    zoneManager->HandleZoneTriggers(zone, triggers, member);
                }
            }
        }

        // If the client is still here, end the system event
        auto client = server->GetManagerConnection()->GetEntityClient(entityID);
        if(client)
        {
            int32_t sourceEntityID = client->GetClientState()
                ->GetEventSourceEntityID();
            if(sourceEntityID == baseID)
            {
                // End the event for the client
                server->GetEventManager()->HandleEvent(client, nullptr);
            }
        }
    }
    else if(bState)
    {
        // Check to make sure the occupier is still here
        auto base = bState->GetEntity();
        if(base->GetOccupyTime() == occupyStartTime &&
            base->GetOccupierID() == entityID && !zone->GetEntity(entityID))
        {
            // Entity is not in the zone anymore, end occupation
            zone->OccupyPvPBase(baseID, -1, true);

            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_PVP_BASE_LEFT);
            notify.WriteS32Little(baseID);
            notify.WriteS32Little(-1);
            notify.WriteS32Little(entityID);

            zoneManager->BroadcastPacket(zone, notify);
        }
    }
}

void MatchManager::IncreaseBaseBonus(int32_t baseID, uint32_t zoneID,
    uint32_t instanceID, uint64_t occupyStartTime)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto instance = zoneManager->GetInstance(instanceID);
    auto zone = instance ? instance->GetZone(zoneID) : nullptr;
    if(!zone && PvPActive(instance))
    {
        return;
    }

    auto bState = zone->GetPvPBase(baseID);
    uint16_t bonus = zone->IncreasePvPBaseBonus(baseID, occupyStartTime);
    if(bonus)
    {
        // Queue next bonus right away
        QueueNextBaseBonus(baseID, zone, occupyStartTime);

        auto base = bState->GetEntity();
        int32_t occupierID = base->GetOccupierID();
        uint8_t teamID = (uint8_t)base->GetTeam();

        int32_t points = (int32_t)(base->GetRank() +
            (base->GetSpeed() * bonus));
        points = UpdatePvPPoints(instance->GetID(), occupierID, baseID,
            teamID, points, true);

        auto pvpStats = instance->GetPvPStats();
        if(points > 0 && pvpStats)
        {
            auto stats = pvpStats->GetPlayerStats(occupierID);
            if(stats)
            {
                stats->SetBaseBonusPoints((uint16_t)(points +
                    stats->GetBaseBonusPoints()));
            }
        }
    }
}

int32_t MatchManager::UpdatePvPPoints(
    uint32_t instanceID, std::shared_ptr<ActiveEntityState> source,
    std::shared_ptr<ActiveEntityState> target, int32_t points)
{
    uint8_t teamID = 2;
    if(target && InPvPTeam(target))
    {
        // Deaths are typically what increases the counter, not kills
        teamID = (uint8_t)(target->GetFactionGroup() == 2 ? 0 : 1);
    }
    else if(source && source != target &&
        (!target || target->GetFactionGroup() != source->GetFactionGroup()) &&
        InPvPTeam(source))
    {
        // Either no target or target is not on the same team
        teamID = (uint8_t)(source->GetFactionGroup() - 1);
    }

    if(teamID < 2)
    {
        bool altMode = false;

        std::array<int32_t, 2> entityIDs = { { -1, -1 } };
        for(size_t i = 0; i < 2; i++)
        {
            auto entity = i == 0 ? source : target;
            if(entity)
            {
                entityIDs[i] = entity->GetEntityID();

                if(entity->GetEntityType() == EntityType_t::PARTNER_DEMON)
                {
                    // Change to "player's demon"
                    auto state = ClientState::GetEntityClientState(
                        entityIDs[i]);
                    if(state)
                    {
                        entityIDs[i] = state->GetCharacterState()
                            ->GetEntityID();
                        altMode |= i == 1;
                    }
                }
            }
        }

        return UpdatePvPPoints(instanceID, entityIDs[0], entityIDs[1], teamID,
            points, altMode);
    }

    return 0;
}

int32_t MatchManager::UpdatePvPPoints(uint32_t instanceID, int32_t sourceID,
    int32_t targetID, uint8_t teamID, int32_t points, bool altMode)
{
    if(teamID <= 1)
    {
        auto server = mServer.lock();
        auto zoneManager = server->GetZoneManager();

        auto instance = zoneManager->GetInstance(instanceID);
        auto pvpStats = instance ? instance->GetPvPStats() : nullptr;
        auto variant = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
            instance ? instance->GetVariant() : nullptr);
        if(PvPActive(instance))
        {
            uint16_t oldPoints = 0;
            uint16_t newPoints = 0;
            {
                std::lock_guard<std::mutex> lock(mLock);
                oldPoints = pvpStats->GetPoints(teamID);

                int32_t newVal = oldPoints + points;

                if(newVal < 0)
                {
                    newVal = 0;
                }
                else if(newVal > 50000)
                {
                    // Apply cap
                    newVal = 50000;
                }

                if(variant->GetMaxPoints() &&
                    oldPoints >= variant->GetMaxPoints())
                {
                    // Cannot lower under max when achieved
                    newVal = (int32_t)oldPoints;
                }

                newPoints = (uint16_t)newVal;

                pvpStats->SetPoints(teamID, newPoints);
            }

            int32_t adjust = (int32_t)(newPoints - oldPoints);
            if(adjust)
            {
                libcomp::Packet notify;
                notify.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_PVP_POINTS);
                notify.WriteS8((int8_t)teamID);
                notify.WriteU16Little((uint16_t)(adjust >= 0 ? adjust : 0));
                notify.WriteU16Little(newPoints);
                notify.WriteS32Little(sourceID);
                notify.WriteS32Little(targetID);
                notify.WriteS8(altMode ? 1 : 0);

                ChannelClientConnection::BroadcastPacket(
                    instance->GetConnections(), notify);

                if(variant->GetMaxPoints() &&
                    newPoints >= variant->GetMaxPoints())
                {
                    // Match is over
                    zoneManager->StopInstanceTimer(instance);
                }
            }

            return adjust;
        }
    }

    return 0;
}

void MatchManager::PlayerKilled(const std::shared_ptr<ActiveEntityState>& entity,
    const std::shared_ptr<ZoneInstance>& instance)
{
    if(entity && !entity->IsAlive() && instance && PvPActive(instance) &&
        entity->GetEntityType() == EntityType_t::CHARACTER)
    {
        // Character killed in an active PvP match, if the current variant
        // is Valhalla, queue auto-revival
        auto state = ClientState::GetEntityClientState(entity->GetEntityID());
        auto variant = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
            instance->GetVariant());
        if(state && variant->GetMatchType() ==
            objects::PvPInstanceVariant::MatchType_t::VALHALLA)
        {
            auto zoneManager = mServer.lock()->GetZoneManager();
            zoneManager->UpdateDeathTimeOut(state, 30);
        }
    }
}

void MatchManager::SendPvPLocation(const std::shared_ptr<
    ChannelClientConnection>& client, uint32_t instanceID, bool enter)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto instance = zoneManager->GetInstance(instanceID);
    if(!instance)
    {
        return;
    }

    auto clients = instance->GetConnections();
    clients.remove_if([client](
        const std::shared_ptr<ChannelClientConnection>& c)
        {
            return c == client;
        });

    if(clients.size() == 0)
    {
        return;
    }

    auto cState = client->GetClientState()->GetCharacterState();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PVP_PLAYER);
    p.WriteS8(enter ? 0 : 1);
    p.WriteS8((int8_t)(cState->GetFactionGroup() - 1));
    p.WriteS32Little(cState->GetEntityID());

    if(enter)
    {
        auto character = cState->GetEntity();
        p.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            character->GetName(), true);
        p.WriteS8(cState->GetLevel());

        auto pvpData = character->GetPvPData().Get();
        p.WriteS32(pvpData ? pvpData->GetGP() : 0);
        p.WriteS8(pvpData && pvpData->GetRanked() ? 1 : 0);
    }

    ChannelClientConnection::BroadcastPacket(clients, p);

    if(!enter && !MatchTeamsActive(instance))
    {
        // End the match prematurely since one team left
        zoneManager->StopInstanceTimer(instance);
    }
}

void MatchManager::UpdateMatchEntries(
    const std::list<std::shared_ptr<objects::MatchEntry>>& updates,
    const std::list<std::shared_ptr<objects::MatchEntry>>& removes)
{
    auto server = mServer.lock();
    auto managerConnection = server->GetManagerConnection();

    std::lock_guard<std::mutex> lock(mLock);

    std::set<int32_t> joinSelf;
    std::set<int32_t> joinTeam;
    std::unordered_map<int32_t, int8_t> updateCodes;
    for(auto update : updates)
    {
        int32_t cid = update->GetWorldCID();

        auto it = mMatchEntries.find(cid);
        if(it != mMatchEntries.end())
        {
            auto existing = it->second;
            if(update->GetReadyTime() != existing->GetReadyTime())
            {
                if(update->GetReadyTime())
                {
                    // Time set/updated
                    updateCodes[cid] = 1;
                }
                else
                {
                    // Time removed
                    updateCodes[cid] = 2;
                }
            }
            else
            {
                // No visible change
                updateCodes[cid] = 0;
            }
        }
        else if(update->GetTeamID() && cid != update->GetOwnerCID())
        {
            joinTeam.insert(cid);
        }
        else
        {
            joinSelf.insert(cid);
        }

        mMatchEntries[cid] = update;
    }

    for(auto remove : removes)
    {
        int32_t cid = remove->GetWorldCID();

        // Send cancellation if a match ID was not assigned
        if(!remove->GetMatchID())
        {
            auto client = managerConnection->GetEntityClient(cid, true);
            if(client)
            {
                if(remove->GetTeamID())
                {
                    libcomp::Packet notify;
                    notify.WritePacketCode(
                        ChannelToClientPacketCode_t::PACKET_PVP_TEAM_CANCEL);

                    client->SendPacket(notify);
                }
                else
                {
                    libcomp::Packet reply;
                    reply.WritePacketCode(
                        ChannelToClientPacketCode_t::PACKET_PVP_CANCEL);
                    reply.WriteS8(0);

                    client->SendPacket(reply);
                }
            }
        }

        mMatchEntries.erase(cid);
    }

    // Send updates to all players on the channel
    std::array<std::unordered_map<int8_t, int16_t>, 2> entryCounts;

    auto pad = server->GetWorldSharedConfig()->GetPvPGhosts();
    for(size_t i = 0; i < pad.size(); i++)
    {
        for(size_t k = 0; k < 2; k++)
        {
            entryCounts[k][(int8_t)i] = (int16_t)pad[i];
        }
    }

    for(auto& pair : mMatchEntries)
    {
        if(pair.second->GetTeamID() &&
            pair.second->GetOwnerCID() != pair.second->GetWorldCID())
        {
            // Only count team leaders
            continue;
        }

        size_t idx = pair.second->GetTeamID() ? 1 : 0;
        int8_t type = (int8_t)pair.second->GetMatchType();
        if(entryCounts[idx].find(type) != entryCounts[idx].end())
        {
            entryCounts[idx][type]++;
        }
        else
        {
            entryCounts[idx][type] = 1;
        }
    }

    if(entryCounts.size() > 0)
    {
        uint32_t now = (uint32_t)std::time(0);
        for(auto& pair : mMatchEntries)
        {
            auto entry = pair.second;
            int32_t cid = entry->GetWorldCID();
            int16_t ready = (int16_t)ChannelServer::GetExpirationInSeconds(
                entry->GetReadyTime(), now);

            auto client = managerConnection->GetEntityClient(cid, true);
            if(client)
            {
                size_t idx = entry->GetTeamID() ? 1 : 0;
                int8_t type = (int8_t)entry->GetMatchType();
                if(joinSelf.find(cid) != joinSelf.end())
                {
                    libcomp::Packet reply;
                    reply.WritePacketCode(
                        ChannelToClientPacketCode_t::PACKET_PVP_JOIN);
                    reply.WriteS8(type);
                    reply.WriteS8(0);
                    reply.WriteS16Little(entryCounts[idx][type]);
                    reply.WriteS16Little(ready ? ready : -1);

                    client->SendPacket(reply);
                }
                else if(joinTeam.find(cid) != joinTeam.end())
                {
                    libcomp::Packet reply;
                    reply.WritePacketCode(
                        ChannelToClientPacketCode_t::PACKET_PVP_TEAM_JOIN);
                    reply.WriteS8(type);

                    client->SendPacket(reply);
                }
                else
                {
                    // Just send the new matching info
                    auto it = updateCodes.find(cid);
                    int8_t updateCode = it != updateCodes.end()
                        ? it->second : 0;

                    libcomp::Packet notify;
                    notify.WritePacketCode(
                        ChannelToClientPacketCode_t::PACKET_PVP_ENTRY_COUNT);
                    notify.WriteS16Little(entryCounts[idx][type]);
                    notify.WriteS16Little(ready ? ready : -1);
                    notify.WriteS8(updateCode);
                    notify.WriteS16Little(0);

                    client->SendPacket(notify);
                }
            }
        }
    }
}

void MatchManager::UpdatePvPMatches(
    const std::list<std::shared_ptr<objects::PvPMatch>>& matches)
{
    auto server = mServer.lock();
    auto managerConnection = server->GetManagerConnection();

    uint8_t channelID = server->GetRegisteredChannel()->GetID();

    uint32_t now = (uint32_t)std::time(0);
    ServerTime serverTime = ChannelServer::GetServerTime();

    std::unordered_map<uint32_t, int32_t> localExpire;
    {
        std::lock_guard<std::mutex> lock(mLock);
        for(auto match : matches)
        {
            int32_t confirmTime = ChannelServer::GetExpirationInSeconds(
                match->GetReadyTime(), now);
            ServerTime serverConfirmTime = (ServerTime)(serverTime +
                (uint64_t)confirmTime * (uint64_t)1000000ULL);

            if(match->GetChannelID() == channelID)
            {
                if(!CreatePvPInstance(match))
                {
                    continue;
                }
            }

            // Inform all members in the match that are on this server and
            // require a response before time runs out to avoid receiving a
            // pentalty
            bool localClient = false;
            for(auto cids : { match->GetBlueMemberIDs(),
                match->GetRedMemberIDs() })
            {
                for(int32_t cid : cids)
                {
                    auto client = managerConnection->GetEntityClient(cid,
                        true);
                    if(client)
                    {
                        if(match->GetNoQueue())
                        {
                            // PvP zones do not work properly unless they are
                            // "prepped" with a confirmation first
                            libcomp::Packet request;
                            request.WritePacketCode(
                                ChannelToClientPacketCode_t::PACKET_PVP_CONFIRM);
                            request.WriteS8(0);   // Confirmed
                            request.WriteS32Little(0);

                            client->QueuePacket(request);

                            // Immediately move to the zone
                            MoveToInstance(client, match);

                            client->FlushOutgoing();
                        }
                        else
                        {
                            auto state = client->GetClientState();
                            state->SetPvPMatch(match);

                            libcomp::Packet notify;
                            notify.WritePacketCode(
                                ChannelToClientPacketCode_t::PACKET_PVP_READY);
                            notify.WriteS8(0);
                            notify.WriteS32Little((int32_t)state->ToClientTime(
                                serverConfirmTime));

                            client->SendPacket(notify);

                            mPendingPvPInvites[match->GetID()].insert(cid);
                            localClient = true;
                        }
                    }
                }
            }

            if(localClient)
            {
                localExpire[match->GetID()] = confirmTime;
            }
        }
    }

    for(auto& pair : localExpire)
    {
        server->GetTimerManager()->ScheduleEventIn((int)pair.second, []
            (MatchManager* pMatchManager, uint32_t pMatchID)
            {
                pMatchManager->ExpirePvPAccess(pMatchID);
            }, this, pair.first);
    }
}

bool MatchManager::PvPActive(const std::shared_ptr<ZoneInstance>& instance)
{
    auto pvpStats = instance ? instance->GetPvPStats() : nullptr;
    return pvpStats && instance->GetTimerStart() && !instance->GetTimerStop();
}

bool MatchManager::InPvPTeam(const std::shared_ptr<ActiveEntityState>& entity)
{
    return entity &&
        (entity->GetFactionGroup() == 1 || entity->GetFactionGroup() == 2);
}

bool MatchManager::CreatePvPInstance(const std::shared_ptr<
    objects::PvPMatch>& match)
{
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    uint32_t variantID = match->GetVariantID();
    if(!variantID)
    {
        auto variantIDs = serverDataManager->GetStandardPvPVariantIDs(
            (uint8_t)match->GetType());
        if(variantIDs.size() == 0)
        {
            LOG_ERROR(libcomp::String("PvP match creation failed due to"
                " undefined match type variants: %1\n").Arg(match->GetType()));
            return false;
        }

        variantID = libcomp::Randomizer::GetEntry(variantIDs);
        match->SetVariantID(variantID);
    }

    auto variantDef = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
        serverDataManager->GetZoneInstanceVariantData(variantID));
    if(!variantDef || !variantDef->GetDefaultInstanceID())
    {
        LOG_ERROR(libcomp::String("Invalid PvP variant encountered, match"
            " creation failed: %1\n").Arg(variantID));
        return false;
    }

    std::set<int32_t> cids;
    for(int32_t cid : match->GetBlueMemberIDs())
    {
        cids.insert(cid);
    }

    for(int32_t cid : match->GetRedMemberIDs())
    {
        cids.insert(cid);
    }

    uint32_t instanceDefID = match->GetInstanceDefinitionID();
    if(!instanceDefID)
    {
        instanceDefID = variantDef->GetDefaultInstanceID();
        match->SetInstanceDefinitionID(variantDef->GetDefaultInstanceID());
    }

    auto instance = server->GetZoneManager()->CreateInstance(instanceDefID,
        cids, variantID);
    if(!instance)
    {
        LOG_ERROR(libcomp::String("Failed to create PvP instance"
            " variant: %1\n").Arg(variantID));
        return false;
    }

    match->SetInstanceID(instance->GetID());

    auto pvpStats = std::make_shared<objects::PvPInstanceStats>();
    pvpStats->SetMatch(match);

    instance->SetPvPStats(pvpStats);

    // Schedule start time and set up expiration
    uint32_t now = (uint32_t)std::time(0);
    ServerTime serverTime = ChannelServer::GetServerTime();

    uint32_t matchStart = (uint32_t)(match->GetReadyTime() +
        instance->GetVariant()->GetTimePoints(1));
    int32_t startTime = ChannelServer::GetExpirationInSeconds(
        matchStart, now);

    instance->SetTimerExpire((ServerTime)(serverTime + ((uint64_t)startTime +
        instance->GetVariant()->GetTimePoints(0)) * 1000000ULL));
    
    server->GetTimerManager()->ScheduleEventIn((int)startTime, []
        (MatchManager* pMatchManager, uint32_t pInstanceID)
        {
            pMatchManager->StartPvPMatch(pInstanceID);
        }, this, instance->GetID());

    return true;
}

void MatchManager::GetPvPTrophies(
    const std::shared_ptr<ZoneInstance>& instance)
{
    // Trophies only apply to standard PvP modes and are listed (in a very
    // unhelpful way) in an unused binary data structure for client display
    // in mode order
    auto pvpStats = instance->GetPvPStats();
    auto match = pvpStats->GetMatch();

    std::unordered_map<int32_t, std::set<int8_t>> trophies;
    if(match->GetType() != 0 && match->GetType() != 1)
    {
        return;
    }

    const int8_t DAMAGE_FIRST = 1;
    const int8_t DAMAGE_TAKEN_FIRST = 2;
    const int8_t DAMAGE_MAX = 3;
    const int8_t DAMAGE_TAKEN_MAX = 4;
    const int8_t BASE_FIRST = 5;
    const int8_t BASE_LAST = 6;
    const int8_t KILL_MAX = 7;
    const int8_t DEATH_MIN = 8;
    const int8_t BSTATUS_MAX = 9;
    const int8_t BSTATUS_TAKEN_DRATE = 10;
    const int8_t GSTATUS_MAX = 11;
    const int8_t DAMAGE_SUM_MAX = 12;
    const int8_t DAMAGE_TAKEN_SUM_DRATE = 13;
    const int8_t BASE_POINT_MAX = 14;
    const int8_t BASE_BONUS_POINT_MAX = 15;
    const int8_t WIN_GP_MIN = 16;
    const int8_t LOSS_BASE_POINT_MAX = 17;
    const int8_t WIN_MVP = 18;

    const int8_t LOSS_DEATH_KRATE = 31;

    std::set<int8_t> validTrophies = {
            DAMAGE_FIRST,
            DAMAGE_TAKEN_FIRST,
            DAMAGE_MAX,
            DAMAGE_TAKEN_MAX,
            BASE_FIRST,
            BASE_LAST,
            KILL_MAX,
            DEATH_MIN,
            BSTATUS_MAX,
            BSTATUS_TAKEN_DRATE,
            GSTATUS_MAX,
            DAMAGE_SUM_MAX,
            DAMAGE_TAKEN_SUM_DRATE,
            BASE_POINT_MAX,
            BASE_BONUS_POINT_MAX,
            WIN_GP_MIN,
            LOSS_BASE_POINT_MAX,
            WIN_MVP
        };

    if(match->GetType() == 1)
    {
        // Remove a couple entries and add loss condition
        validTrophies.erase(BASE_FIRST);
        validTrophies.erase(BASE_LAST);
        validTrophies.erase(BASE_POINT_MAX);
        validTrophies.erase(BASE_BONUS_POINT_MAX);
        validTrophies.erase(WIN_GP_MIN);
        validTrophies.erase(LOSS_BASE_POINT_MAX);
        validTrophies.insert(LOSS_DEATH_KRATE);
    }

    size_t idx = 0;

    std::list<std::shared_ptr<objects::PvPPlayerStats>> allPlayers;
    std::list<std::shared_ptr<objects::PvPPlayerStats>> winners;
    std::list<std::shared_ptr<objects::PvPPlayerStats>> losers;
    for(auto cids : { match->GetBlueMemberIDs(), match->GetRedMemberIDs() })
    {
        size_t otherIdx = idx == 0 ? 1 : 0;

        bool lost = pvpStats->GetPoints(idx) <
            pvpStats->GetPoints(otherIdx);

        for(int32_t cid : cids)
        {
            for(auto& pair : pvpStats->GetPlayerStats())
            {
                auto stats = pair.second;
                if(stats->GetWorldCID() == cid)
                {
                    if(lost)
                    {
                        losers.push_back(stats);
                    }
                    else
                    {
                        winners.push_back(stats);
                    }

                    allPlayers.push_back(stats);
                }
            }
        }

        idx++;
    }

    std::set<int32_t> maxPlayers;
    for(int8_t trophy : validTrophies)
    {
        bool allowZero = false;
        std::unordered_map<int32_t, double> maxMap;
        switch(trophy)
        {
        case DAMAGE_FIRST:
            for(int32_t entityID : pvpStats->GetFirstDamage())
            {
                trophies[entityID].insert(DAMAGE_FIRST);
            }
            break;
        case DAMAGE_TAKEN_FIRST:
            for(int32_t entityID : pvpStats->GetFirstDamageTaken())
            {
                trophies[entityID].insert(DAMAGE_TAKEN_FIRST);
            }
            break;
        case DAMAGE_MAX:
            // Gather damage dealt max
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetDamageMax());
            }
            break;
        case DAMAGE_TAKEN_MAX:
            // Gather damage taken max
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetDamageMaxTaken());
            }
            break;
        case BASE_FIRST:
            trophies[pvpStats->GetBaseFirstOwner()].insert(BASE_FIRST);
            break;
        case BASE_LAST:
            {
                // Get last base owner(s)
                uint64_t max = 0;
                for(auto& zdPair : instance->GetZones())
                {
                    for(auto zPair : zdPair.second)
                    {
                        for(auto bState : zPair.second->GetPvPBases())
                        {
                            auto base = bState->GetEntity();
                            if(base->GetOccupyTime() > max)
                            {
                                maxPlayers.clear();
                                max = base->GetOccupyTime();
                            }

                            if(max && base->GetOccupyTime() == max)
                            {
                                maxPlayers.insert(base->GetOwnerID());
                            }
                        }
                    }
                }
            }
            break;
        case KILL_MAX:
            // Gather kills
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetKills() + stats->GetDemonKills());
            }
            break;
        case DEATH_MIN:
            // Gather (inverted) deaths
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = -(double)(
                    stats->GetDeaths() + stats->GetDemonDeaths());
            }
            allowZero = true;
            break;
        case BSTATUS_MAX:
            // Gather bad status count
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetBadStatus());
            }
            break;
        case GSTATUS_MAX:
            // Gather good status count
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetGoodStatus());
            }
            break;
        case DAMAGE_SUM_MAX:
            // Gather damage dealt
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetDamageSum());
            }
            break;
        case BSTATUS_TAKEN_DRATE:
        case DAMAGE_TAKEN_SUM_DRATE:
        case WIN_MVP:
        case LOSS_DEATH_KRATE:
            {
                // Handle max rates for a subset checking for 0
                // denominator value
                std::list<std::shared_ptr<objects::PvPPlayerStats>> pList;
                switch(trophy)
                {
                case BSTATUS_TAKEN_DRATE:
                case DAMAGE_TAKEN_SUM_DRATE:
                    pList = allPlayers;
                    break;
                case WIN_MVP:
                    pList = winners;
                    break;
                case LOSS_DEATH_KRATE:
                default:
                    pList = losers;
                    break;
                }

                std::map<int32_t, std::map<int32_t, int32_t>> m;
                for(auto stats : pList)
                {
                    int32_t key = 0;
                    int32_t val = 0;
                    switch(trophy)
                    {
                    case BSTATUS_TAKEN_DRATE:
                        // Gather bad status taken / deaths
                        key = (int32_t)(stats->GetDeaths() +
                            stats->GetDemonDeaths());
                        val = (int32_t)(stats->GetBadStatusTaken());
                        break;
                    case DAMAGE_TAKEN_SUM_DRATE:
                        // Gather damage taken / deaths
                        key = (int32_t)(stats->GetDeaths() +
                            stats->GetDemonDeaths());
                        val = (int32_t)(stats->GetDamageSum());
                        break;
                    case WIN_MVP:
                        // Gather (points + kills) / deaths
                        key = (int32_t)(stats->GetDeaths() +
                            stats->GetDemonDeaths());
                        val = (int32_t)(stats->GetKills() +
                            stats->GetDemonKills() +
                            stats->GetBasePoints() +
                            stats->GetBaseBonusPoints());
                        break;
                    case LOSS_DEATH_KRATE:
                    default:
                        // Gather deaths / kills
                        key = (int32_t)(stats->GetKills() +
                            stats->GetDemonKills());
                        val = (int32_t)(stats->GetDeaths() +
                            stats->GetDemonDeaths());
                        break;
                    }

                    m[key][stats->GetEntityID()] = val;
                }

                if(m.find(0) == m.end())
                {
                    // Get all zero key players with the highest value
                    for(auto& pair : m[0])
                    {
                        maxMap[pair.first] = (double)pair.second;
                    }
                }
                else
                {
                    // Get all players with the highest rate
                    for(auto& mPair : m)
                    {
                        for(auto& pair : mPair.second)
                        {
                            maxMap[pair.first] = (double)pair.second /
                                (double)mPair.first;
                        }
                    }
                }
            }
            break;
        case BASE_POINT_MAX:
            // Gather base points
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetBasePoints());
            }
            break;
        case BASE_BONUS_POINT_MAX:
            // Gather bonus points
            for(auto stats : allPlayers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetBaseBonusPoints());
            }
            break;
        case LOSS_BASE_POINT_MAX:
            // Gather base and bonus points
            for(auto stats : losers)
            {
                maxMap[stats->GetEntityID()] = (double)(
                    stats->GetBasePoints() + stats->GetBaseBonusPoints());
            }
            break;
        case WIN_GP_MIN:
            {
                // Gather (inverted) GP
                auto db = mServer.lock()->GetWorldDatabase();
                for(auto stats : losers)
                {
                    auto character = stats->GetCharacter().Get(db);
                    auto pvpData = character->GetPvPData().Get(db);
                    if(pvpData)
                    {
                        maxMap[stats->GetEntityID()] = -(double)(
                            pvpData->GetGP());
                    }
                }
                allowZero = true;
            }
            break;
        default:
            break;
        }

        if(maxMap.size() > 0)
        {
            // Populate list from generic max value
            double max = 0;
            for(auto& pair : maxMap)
            {
                if(pair.second > max)
                {
                    maxPlayers.clear();
                    max = pair.second;
                }

                if((allowZero || max) && pair.second == max)
                {
                    maxPlayers.insert(pair.first);
                }
            }
        }

        for(int32_t entityID : maxPlayers)
        {
            trophies[entityID].insert(trophy);
        }

        maxPlayers.clear();
    }

    // If anything was not set, remove the non-player entry
    trophies.erase(0);

    // Calculate trophy boosts
    for(auto& pair : trophies)
    {
        auto stats = pvpStats->GetPlayerStats(pair.first);
        if(stats)
        {
            for(int8_t trophy : pair.second)
            {
                float boost = 0.f;
                switch(trophy)
                {
                case WIN_MVP:
                    // 300/100%
                    boost = (match->GetType() == 0) ? 3.f : 1.f;
                    break;
                case LOSS_DEATH_KRATE:
                    // No boost
                    break;
                default:
                    // 50/10%
                    boost = (match->GetType() == 0) ? 0.5f : 0.1f;
                    break;
                }

                stats->SetTrophyBoost(boost + stats->GetTrophyBoost());
            }
        }
    }

    // Shift trophies for valhalla if needed
    if(match->GetType() == 1)
    {
        int8_t shift = 19;
        for(int8_t valid : validTrophies)
        {
            for(auto& pair : trophies)
            {
                if(pair.second.find(valid) != pair.second.end())
                {
                    pair.second.erase(valid);
                    pair.second.insert(shift);
                }
            }

            shift++;
        }
    }

    // Apply all trophies
    for(auto& pair : trophies)
    {
        auto stats = pvpStats->GetPlayerStats(pair.first);
        if(stats)
        {
            stats->SetTrophies(pair.second);
        }
    }
}

bool MatchManager::ValidateMatchEntries(
    const std::list<std::shared_ptr<ChannelClientConnection>>& clients,
    int8_t teamCategory, bool isTeam, bool checkPenalties)
{
    if(!isTeam)
    {
        if(clients.size() != 1)
        {
            return false;
        }
    }
    else
    {
        if(clients.size() < 2)
        {
            LOG_DEBUG("Match entry validation failed: teams must have"
                " at least 2 members\n");
            return false;
        }

        auto first = clients.front();
        auto state = first->GetClientState();
        auto team = state->GetTeam();

        if(!team && (int8_t)team->GetCategory() != teamCategory)
        {
            LOG_DEBUG("Match entry validation failed: invalid team"
                " type encountered\n");
            return false;
        }

        std::set<int32_t> memberIDs = team->GetMemberIDs();
        if(memberIDs.size() != clients.size())
        {
            LOG_DEBUG("Match entry validation failed: one or more team"
                " members is missing from validation\n");
            return false;
        }

        auto zone = state->GetZone();
        for(auto client : clients)
        {
            int32_t worldCID = client->GetClientState()->GetWorldCID();
            if(memberIDs.find(worldCID) == memberIDs.end())
            {
                LOG_DEBUG(libcomp::String("Match entry validation failed:"
                    " invalid team member requested: %1\n")
                    .Arg(client->GetClientState()->GetAccountUID().ToString()));
                return false;
            }

            if(client->GetClientState()->GetZone() != zone)
            {
                LOG_DEBUG(libcomp::String("Match entry validation failed: team"
                    " members must all be in the same zone: %1\n")
                    .Arg(client->GetClientState()->GetAccountUID().ToString()));
                return false;
            }
        }
    }

    for(auto client : clients)
    {
        if(client->GetClientState()->GetPvPMatch())
        {
            LOG_DEBUG(libcomp::String("Match entry validation failed: pending"
                " PvP match exists: %1\n")
                .Arg(client->GetClientState()->GetAccountUID().ToString()));
            return false;
        }

        if(GetMatchEntry(client->GetClientState()->GetWorldCID()))
        {
            LOG_DEBUG(libcomp::String("Match entry validation failed: existing"
                " queue match entry exists: %1\n")
                .Arg(client->GetClientState()->GetAccountUID().ToString()));
            return false;
        }

        if(teamCategory == (int8_t)objects::Team::Category_t::PVP)
        {
            // Create PvPData if needed and check penalty count
            auto pvpData = GetPvPData(client, true);
            if(!pvpData)
            {
                LOG_ERROR(libcomp::String("PvP entry validation failed:"
                    " PvPData could not be created: %1\n")
                    .Arg(client->GetClientState()->GetAccountUID().ToString()));
                return false;
            }

            if(checkPenalties && pvpData->GetPenaltyCount() >= 3)
            {
                LOG_DEBUG(libcomp::String("PvP entry validation failed: too"
                    " many penalties exist: %1\n")
                    .Arg(client->GetClientState()->GetAccountUID().ToString()));
                return false;
            }
        }
    }

    return true;
}

bool MatchManager::MoveToInstance(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::PvPMatch>& match)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    auto instance = zoneManager->GetInstance(match->GetInstanceID());
    auto zone = zoneManager->GetInstanceStartingZone(instance);
    if(zone)
    {
        float x, y, rot;

        auto zoneDef = zone->GetDefinition();
        if(!zoneManager->GetPvPStartPosition(client, zone, x, y, rot) ||
            !zoneManager->EnterZone(client, zoneDef->GetID(),
                zoneDef->GetDynamicMapID(), x, y, rot))
        {
            client->Kill();
            return false;
        }

        return true;
    }

    return false;
}

void MatchManager::QueueNextBaseBonus(int32_t baseID,
    const std::shared_ptr<Zone>& zone, uint64_t occupyStartTime)
{
    // Bonuses increase every 30 seconds
    if(PvPActive(zone->GetInstance()))
    {
        mServer.lock()->GetTimerManager()->ScheduleEventIn(30, []
            (MatchManager* pMatchManager, int32_t pBaseID, uint32_t pZoneID,
                uint32_t pInstanceID, uint64_t pOccupyStartTime)
            {
                pMatchManager->IncreaseBaseBonus(pBaseID, pZoneID, pInstanceID,
                    pOccupyStartTime);
            }, this, baseID, zone->GetID(), zone->GetInstance()->GetID(),
                occupyStartTime);
    }
}

bool MatchManager::MatchTeamsActive(const std::shared_ptr<
    ZoneInstance>& instance)
{
    auto clients = instance->GetConnections();
    if(clients.size() == 0)
    {
        // No one in zone
        return false;
    }

    auto variant = instance->GetVariant();
    if(!variant)
    {
        return false;
    }

    switch(variant->GetInstanceType())
    {
    case InstanceType_t::PVP:
        {
            std::array<bool, 2> teamExists = { { false, false } };
            for(auto client : clients)
            {
                auto cState = client->GetClientState()->GetCharacterState();
                int32_t factionGroup = cState->GetFactionGroup();
                if(InPvPTeam(cState))
                {
                    teamExists[(size_t)(factionGroup - 1)] = true;
                }
            }

            return teamExists[0] && teamExists[1];
        }
    default:
        return false;
    }
}
