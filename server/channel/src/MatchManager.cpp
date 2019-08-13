/**
 * @file server/channel/src/MatchManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manager class in charge of handling any client side match or
 *  team logic. Match types include PvP, Ultimate Battle, etc.
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
#include <ServerConstants.h>
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ChannelConfig.h>
#include <DiasporaBase.h>
#include <InstanceAccess.h>
#include <MatchEntry.h>
#include <MiUraFieldTowerData.h>
#include <PentalphaEntry.h>
#include <PentalphaMatch.h>
#include <PvPInstanceVariant.h>
#include <PvPBase.h>
#include <PvPData.h>
#include <PvPInstanceStats.h>
#include <PvPInstanceVariant.h>
#include <PvPPlayerStats.h>
#include <PvPMatch.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <ServerZoneTrigger.h>
#include <Spawn.h>
#include <Team.h>
#include <UBMatch.h>
#include <UBResult.h>
#include <UBTournament.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ActionManager.h"
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "ZoneManager.h"

using namespace channel;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<MatchManager>()
    {
        if(!BindingExists("MatchManager", true))
        {
            Using<objects::MatchEntry>();
            Using<objects::PentalphaMatch>();
            Using<Zone>();

            Sqrat::Class<MatchManager,
                Sqrat::NoConstructor<MatchManager>> binding(mVM, "MatchManager");
            binding
                .Func("GetMatchEntry", &MatchManager::GetMatchEntry)
                .Func("EndPvPMatch", &MatchManager::EndPvPMatch)
                .Func("JoinUltimateBattleQueue", &MatchManager::JoinUltimateBattleQueue)
                .Func("ToggleDiasporaBase", &MatchManager::ToggleDiasporaBase)
                .Func("StartStopMatch", &MatchManager::StartStopMatch)
                .Func("StartUltimateBattle", &MatchManager::StartUltimateBattle)
                .Func("StartUltimateBattleTimer", &MatchManager::StartUltimateBattleTimer)
                .Func("UltimateBattleSpectate", &MatchManager::UltimateBattleSpectate)
                .Func("AdvancePhase", &MatchManager::AdvancePhase)
                .Func("GetUBTournament", &MatchManager::GetUBTournament)
                .Func("GetPentalphaMatch", &MatchManager::GetPentalphaMatch)
                .StaticFunc("PvPActive", &MatchManager::PvPActive)
                .StaticFunc("InPvPTeam", &MatchManager::InPvPTeam);

            Bind<MatchManager>("MatchManager", binding);
        }

        return *this;
    }
}

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
    case objects::MatchEntry::MatchType_t::ULTIMATE_BATTLE:
        // Handled elsewhere
        valid = false;
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
        LogMatchManagerWarning([&]()
        {
            return libcomp::String("One or more match entries failed to"
                " queue for account: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        return false;
    }

    return true;
}

bool MatchManager::CancelQueue(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto match = state->GetPendingMatch();
    if(match)
    {
        // Too late to cancel PvP, must be UB
        auto ubMatch = std::dynamic_pointer_cast<objects::UBMatch>(match);
        if(ubMatch)
        {
            CleanupPendingMatch(client);

            return true;
        }

        return false;
    }
    else
    {
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
            LogMatchManagerWarning([&]()
            {
                return libcomp::String("One or more match cancellations"
                    " failed to send to the world for account: %1\n")
                    .Arg(state->GetAccountUID().ToString());
            });

            return false;
        }

        return true;
    }
}

void MatchManager::ConfirmMatch(const std::shared_ptr<
    ChannelClientConnection>& client, uint32_t matchID)
{
    auto state = client->GetClientState();
    auto match = state->GetPendingMatch();
    if(!match)
    {
        return;
    }

    switch(match->GetType())
    {
    case objects::Match::Type_t::PVP_FATE:
    case objects::Match::Type_t::PVP_VALHALLA:
        {
            auto zoneManager = mServer.lock()->GetZoneManager();

            auto pvpMatch = std::dynamic_pointer_cast<objects::PvPMatch>(
                match);

            bool success = pvpMatch && PvPInviteReply(client, match->GetID(),
                true);

            libcomp::Packet reply;
            reply.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_PVP_CONFIRM);
            reply.WriteS8(0);   // Confirmed
            reply.WriteS32Little(success ? 0 : -1);

            client->SendPacket(reply);

            if(success)
            {
                zoneManager->MoveToInstance(client);
            }
        }
        break;
    case objects::Match::Type_t::ULTIMATE_BATTLE:
        {
            auto ubMatch = std::dynamic_pointer_cast<objects::UBMatch>(match);
            if(ubMatch)
            {
                auto zoneManager = mServer.lock()->GetZoneManager();
                auto zone = zoneManager->GetGlobalZone(ubMatch
                    ->GetZoneDefinitionID(), ubMatch->GetDynamicMapID());

                bool success = false;
                {
                    std::lock_guard<std::mutex> lock(mLock);
                    size_t memberLimit = (size_t)ubMatch->GetMemberLimit();
                    success = zone && ubMatch->MemberIDsCount() < memberLimit &&
                        (!ubMatch->GetID() || ubMatch->GetID() == matchID) &&
                        ubMatch->GetState() == objects::UBMatch::State_t::READY;
                    if(success)
                    {
                        ubMatch->InsertMemberIDs(state->GetWorldCID());
                    }
                }

                libcomp::Packet reply;
                reply.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_UB_LOTTO_JOIN);
                reply.WriteS8(success ? 0 : -1);

                client->SendPacket(reply);

                if(success)
                {
                    float x, y, rot;
                    zoneManager->GetMatchStartPosition(client, zone, x, y,
                        rot);

                    zoneManager->EnterZone(client, ubMatch
                        ->GetZoneDefinitionID(), ubMatch->GetDynamicMapID(),
                        x, y, rot);
                }
            }

            CleanupPendingMatch(client);
        }
        break;
    default:
        break;
    }
}

void MatchManager::RejectPvPMatch(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto match = state->GetPendingMatch();
    if(!match)
    {
        return;
    }

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

bool MatchManager::UpdateUBPoints(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t adjust)
{
    // UB points are either queued until the current match round
    // completes or added directly if there is no applicable match

    auto state = client->GetClientState();
    auto zone = state->GetZone();
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    if(ubMatch)
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(ubMatch->GetState() != objects::UBMatch::State_t::COMPLETE)
        {
            // Add to the match instead
            int32_t worldCID = state->GetWorldCID();
            ubMatch->SetPoints(worldCID,
                ubMatch->GetPoints(worldCID) + adjust);
            return true;
        }
    }

    // Add directly to the UBResult
    std::list<std::shared_ptr<objects::UBResult>> updatedResults;

    auto current = LoadUltimateBattleData(client, 0x03, true);
    auto allTime = state->GetUltimateBattleData(1).Get();
    for(auto result : { current, allTime })
    {
        if(result)
        {
            int32_t newPoints = (int32_t)result->GetPoints() + adjust;
            if(newPoints < 0)
            {
                newPoints = 0;
            }

            result->SetPoints((uint32_t)newPoints);

            updatedResults.push_back(result);
        }
    }

    if(updatedResults.size() > 0)
    {
        // Sync all results with the world
        auto server = mServer.lock();
        auto syncManager = server->GetChannelSyncManager();

        auto dbChanges = libcomp::DatabaseChangeSet::Create();

        for(auto update : updatedResults)
        {
            dbChanges->Update(update);

            syncManager->UpdateRecord(update, "UBResult");
        }

        server->GetWorldDatabase()->ProcessChangeSet(dbChanges);

        syncManager->SyncOutgoing();

        return true;
    }

    return false;
}

bool MatchManager::UpdateZiotite(const std::shared_ptr<objects::Team>& team,
    int32_t sZiotite, int8_t lZiotite, int32_t worldCID)
{
    if(!team)
    {
        return false;
    }

    if(!worldCID)
    {
        // Update valid, set directly and send update
        {
            std::lock_guard<std::mutex> lock(mLock);

            team->SetSmallZiotite(sZiotite);
            team->SetLargeZiotite(lZiotite);
        }

        SendZiotite(team);

        return true;
    }
    else
    {
        // Stage the change and send to the world to refresh
        int32_t newSAmount = 0;
        int8_t newLAmount = 0;
        {
            std::lock_guard<std::mutex> lock(mLock);
            newSAmount = team->GetSmallZiotite() + sZiotite;
            newLAmount = (int8_t)(team->GetLargeZiotite() + lZiotite);

            if(newSAmount < 0 || newLAmount < 0)
            {
                return false;
            }

            // Apply limits
            if(newLAmount > 3)
            {
                newLAmount = 3;
            }

            int32_t sLimit = (int32_t)(team->MemberIDsCount() * 10000);
            if(newSAmount > sLimit)
            {
                newSAmount = sLimit;
            }

            if(newSAmount == team->GetSmallZiotite() &&
                newLAmount == team->GetLargeZiotite())
            {
                // No update
                return true;
            }

            sZiotite = newSAmount - team->GetSmallZiotite();
            lZiotite = (int8_t)(newLAmount - team->GetLargeZiotite());

            team->SetSmallZiotite(newSAmount);
            team->SetLargeZiotite(newLAmount);
        }

        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_TEAM_UPDATE);
        request.WriteU8((int8_t)
            InternalPacketAction_t::PACKET_ACTION_TEAM_ZIOTITE);
        request.WriteS32Little(team->GetID());
        request.WriteS32Little(worldCID);
        request.WriteS32Little(sZiotite);
        request.WriteS8(lZiotite);

        mServer.lock()->GetManagerConnection()->GetWorldConnection()
            ->SendPacket(request);

        return true;
    }
}

void MatchManager::SendZiotite(const std::shared_ptr<objects::Team>& team,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    if(team)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ZIOTITE_UPDATE);
        p.WriteS32Little(team->GetSmallZiotite());
        p.WriteS8(team->GetLargeZiotite());
        p.WriteS8((int8_t)team->MemberIDsCount());

        if(client)
        {
            client->SendPacket(p);
        }
        else
        {
            auto clients = mServer.lock()->GetManagerConnection()
                ->GetEntityClients(team->GetMemberIDs(), true);

            ChannelClientConnection::BroadcastPacket(clients, p);
        }
    }
}

bool MatchManager::RequestTeamPvPMatch(
    const std::shared_ptr<ChannelClientConnection>& client,
    uint32_t variantID, uint32_t instanceID)
{
    auto state = client->GetClientState();
    auto team = state->GetTeam();
    if(!team || team->GetLeaderCID() != state->GetWorldCID())
    {
        LogMatchManagerDebug([&]()
        {
            return libcomp::String("Team PvP creation failed: requestor"
                " is not the team leader: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

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
            LogMatchManagerDebug([&]()
            {
                return libcomp::String("Team PvP creation failed: one or more"
                    " team members is not on the channel: %1\n")
                    .Arg(state->GetAccountUID().ToString());
            });

            return false;
        }

        teamClients.push_back(teamClient);
    }

    if(!ValidateMatchEntries(teamClients,
        (int8_t)objects::Team::Category_t::PVP, true, false))
    {
        LogMatchManagerDebug([&]()
        {
            return libcomp::String("Team PvP creation failed: one or more team"
                " members is not valid: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        return false;
    }

    // Team is valid, validate variant and build match
    auto variant = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
        server->GetServerDataManager()->GetZoneInstanceVariantData(variantID));
    if(!variant)
    {
        LogMatchManagerDebug([&]()
        {
            return libcomp::String("Team PvP creation failed: invalid variant"
                " specified for instance: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        return false;
    }

    if(variant->GetMaxPlayers() &&
        teamClients.size() > (size_t)variant->GetMaxPlayers())
    {
        LogMatchManagerDebug([&]()
        {
            return libcomp::String("Team PvP creation failed: too many players"
                " are in the current team: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

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
        LogMatchManagerDebug([&]()
        {
            return libcomp::String("Team PvP creation failed: invalid"
                " instance requested: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        return false;
    }

    auto match = std::make_shared<objects::PvPMatch>();
    match->SetType((objects::Match::Type_t)variant->GetMatchType());
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

        match->InsertMemberIDs(cid);
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
            LogMatchManagerDebug([&]()
            {
                return libcomp::String("Team PvP creation failed: team size"
                    " restrictions could not be met: %1\n")
                    .Arg(state->GetAccountUID().ToString());
            });

            return false;
        }
    }

    LogMatchManagerDebug([&]()
    {
        return libcomp::String("Requesting team PvP match with variant %1 and"
            " instance %2: %3\n").Arg(variantID).Arg(instanceID)
            .Arg(client->GetClientState()->GetAccountUID().ToString());
    });

    if(!server->GetChannelSyncManager()->SyncRecordUpdate(match, "PvPMatch"))
    {
        LogMatchManagerDebug([&]()
        {
            return libcomp::String("Team PvP creation failed: match could"
                " not be queued: %1\n").Arg(state->GetAccountUID().ToString());
        });

        return false;
    }

    return true;
}

bool MatchManager::JoinUltimateBattleQueue(int32_t worldCID,
    const std::shared_ptr<Zone>& zone)
{
    if(!zone)
    {
        return false;
    }

    auto ubMatch = zone->GetUBMatch();
    if(!ubMatch)
    {
        return false;
    }

    if(ubMatch->GetCategory() == objects::UBMatch::Category_t::UA)
    {
        // UA participants can join directly

        // Must be in ready state
        if(ubMatch->GetState() != objects::UBMatch::State_t::READY)
        {
            return false;
        }

        // Limit must not be exceeded (already joined is fine)
        std::lock_guard<std::mutex> lock(mLock);
        if(!ubMatch->MemberIDsContains(worldCID) &&
            ubMatch->MemberIDsCount() >= (size_t)ubMatch->GetMemberLimit())
        {
            return false;
        }

        ubMatch->InsertMemberIDs(worldCID);

        return true;
    }
    else
    {
        // UB participants need to join the queue and wait to be selected

        // Must be in pre-match state
        if(ubMatch->GetState() != objects::UBMatch::State_t::PREMATCH)
        {
            return false;
        }

        auto client = mServer.lock()->GetManagerConnection()->GetEntityClient(
            worldCID, true);
        if(client)
        {
            // Can't have already joined the queue
            auto entry = GetMatchEntry(worldCID);
            if(entry)
            {
                return false;
            }

            std::lock_guard<std::mutex> lock(mLock);

            entry = std::make_shared<objects::MatchEntry>();
            entry->SetWorldCID(worldCID);
            entry->SetOwnerCID(worldCID);
            entry->SetMatchType(
                objects::MatchEntry::MatchType_t::ULTIMATE_BATTLE);

            ubMatch->SetPendingEntries(worldCID, entry);
            client->GetClientState()->SetPendingMatch(ubMatch);

            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_UB_LOTTO_STATUS);
            notify.WriteS32Little((int32_t)ubMatch->PendingEntriesCount());

            client->SendPacket(notify);

            return true;
        }
    }

    return false;
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
        LogMatchManagerDebug([&]()
        {
            return libcomp::String("Expiring %1 unconfirmed PvP player(s)\n")
                .Arg(cids.size());
        });

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

bool MatchManager::CleanupPendingMatch(
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto match = state->GetPendingMatch();
    if(!match)
    {
        return false;
    }

    bool removeMatch = false;
    switch(match->GetType())
    {
    case objects::Match::Type_t::PVP_FATE:
    case objects::Match::Type_t::PVP_VALHALLA:
        PvPInviteReply(client, match->GetID(), false);
        break;
    case objects::Match::Type_t::ULTIMATE_BATTLE:
    default:
        {
            auto ubMatch = std::dynamic_pointer_cast<objects::UBMatch>(match);
            if(ubMatch)
            {
                // Remove from the member IDs if set there
                ubMatch->RemovePendingEntries(state->GetWorldCID());

                // Remove the entire match if no one will join
                removeMatch = ubMatch->PendingEntriesCount() == 0 &&
                    ubMatch->MemberIDsCount() == 0;
            }

            auto entry = GetMatchEntry(state->GetWorldCID());
            if(entry)
            {
                std::lock_guard<std::mutex> lock(mLock);
                mMatchEntries.erase(state->GetWorldCID());
            }
        }
        break;
    }

    state->SetPendingMatch(nullptr);

    if(removeMatch)
    {
        auto zone = mServer.lock()->GetZoneManager()->GetExistingZone(
            match->GetZoneDefinitionID(), match->GetDynamicMapID(),
            match->GetInstanceID());
        if(zone && zone->GetMatch() == match)
        {
            StartStopMatch(zone, nullptr);
        }
    }

    return true;
}

bool MatchManager::PvPInviteReply(const std::shared_ptr<
    ChannelClientConnection>& client, uint32_t matchID, bool accept)
{
    auto state = client->GetClientState();
    auto match = state->GetPendingMatch();
    int32_t worldCID = state->GetWorldCID();

    // Always clear match at this point
    if(match && match->GetID() == matchID)
    {
        state->SetPendingMatch(nullptr);
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
                    LogMatchManagerError([&]()
                    {
                        return libcomp::String("Failed to apply PvP"
                        " penalty: %1\n")
                        .Arg(state->GetAccountUID().ToString());
                    });
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

    LogMatchManagerCritical([&]()
    {
        return libcomp::String("Starting PvP match %1\n")
            .Arg(pvpStats->GetMatch()->GetID());
    });

    // Players in the match, either end if only one team is here or start
    // the match and queue up complete action
    if(MatchTeamsActive(instance))
    {
        // Start the match
        zoneManager->StartInstanceTimer(instance);

        // Fire match start event in all current zones (should be one)
        for(auto zone : instance->GetZones())
        {
            zoneManager->TriggerZoneActions(zone, {},
                ZoneTrigger_t::ON_PVP_START, nullptr);
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

    LogMatchManagerDebug([&]()
    {
        return libcomp::String("Ending PvP match %1\n").Arg(match->GetID());
    });

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
                    for(auto& zone : instance->GetZones())
                    {
                        auto eState = zone->GetActiveEntity(
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

                if((int8_t)match->GetType() == 0 ||
                    (int8_t)match->GetType() == 1)
                {
                    // 0: Win, 1: Lose, 2: Draw
                    size_t i = (result == -1 ? 1 : (result == 0 ? 2 : 0));
                    i = (size_t)(i + (size_t)((int8_t)match->GetType() * 3));

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
        LogMatchManagerErrorMsg("Failed to save one or more PvP match results");
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
        instance ? instance->GetVariant() : nullptr);
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
            cState->SetKillValue(variant->GetPlayerValue());
        }

        if(variant->GetDemonValue())
        {
            dState->SetKillValue(variant->GetDemonValue());
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
    p.WriteS8((int8_t)match->GetType());
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

    std::list<std::shared_ptr<PvPBaseState>> bases;

    // Should only be one zone in these instances but support multiple
    // if someone decides to make one
    auto zone = state->GetZone();
    if(zone)
    {
        bases = zone->GetPvPBases();
    }

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

void MatchManager::EnterDiaspora(const std::shared_ptr<
    ChannelClientConnection>& client, const std::shared_ptr<Zone>& zone)
{
    auto instance = zone->GetInstance();
    auto zoneManager = mServer.lock()->GetZoneManager();

    zoneManager->SendInstanceTimer(instance, client, true);

    SendPhase(zone, false, client);

    // Send existing member locations
    SendDiasporaLocation(client, instance->GetID(), true, true);

    // Send own location
    SendDiasporaLocation(client, instance->GetID(), true, false);

    // Update player positions
    zoneManager->UpdateTrackedZone(zone);

    // Send base information
    auto bases = zone->GetDiasporaBases();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DIASPORA_BASE_INFO);
    p.WriteU32Little((uint32_t)bases.size());

    uint64_t now = ChannelServer::GetServerTime();
    for(auto bState : bases)
    {
        auto base = bState->GetEntity();
        auto def = base->GetDefinition();

        uint64_t reset = base->GetResetTime();
        float timeLeft = reset < now ? 0.f
            : (float)((double)(reset - now) / 1000000.0);

        p.WriteU32Little(def->GetID());
        p.WriteS32Little(bState->GetEntityID());
        p.WriteFloat(bState->GetCurrentX());
        p.WriteFloat(bState->GetCurrentY());
        p.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            def->GetName(), true);
        p.WriteU32Little(def->GetCaptureItem());
        p.WriteFloat(timeLeft);
    }

    client->SendPacket(p);
}

void MatchManager::EnterUltimateBattle(const std::shared_ptr<
    ChannelClientConnection>& client, const std::shared_ptr<Zone>& zone)
{
    auto state = client->GetClientState();
    auto ubMatch = zone->GetUBMatch();

    bool firstEntry = true;
    if(!ubMatch)
    {
        return;
    }
    else
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(ubMatch->GetTimerStart() == 0 &&
          ubMatch->GetState() == objects::UBMatch::State_t::READY)
        {
            // Set the ready timer and schedule match beginning
            uint64_t now = ChannelServer::GetServerTime();
            ubMatch->SetTimerStart(now);
            ubMatch->SetTimerExpire(now + (uint64_t)(
                (uint64_t)ubMatch->GetReadyDuration() * 1000000ULL));

            firstEntry = true;
        }
    }

    auto server = mServer.lock();

    if(firstEntry)
    {
        UltimateBattleTick(zone->GetDefinitionID(), zone->GetDynamicMapID(),
            zone->GetInstanceID());

        // Start the pre-match timer now
        server->GetTimerManager()->ScheduleEventIn(
            (int)ubMatch->GetReadyDuration(), []
            (MatchManager* pMatchManager, uint32_t pZoneID,
            uint32_t pDynamicMapID, uint32_t pInstanceID)
            {
                pMatchManager->UltimateBattleBegin(pZoneID, pDynamicMapID,
                    pInstanceID);
            }, this, zone->GetDefinitionID(), zone->GetDynamicMapID(),
                zone->GetInstanceID());
    }

    // Shouldn't get to this point if not participating or spectating
    // but default to spectating
    bool spectating = !ubMatch->MemberIDsContains(state->GetWorldCID());
    if(spectating)
    {
        state->GetCharacterState()->SetDisplayState(
            ActiveDisplayState_t::UB_SPECTATE);
        state->GetDemonState()->SetDisplayState(
            ActiveDisplayState_t::UB_SPECTATE);
    }

    SendUltimateBattleMembers(zone);

    SendPhase(zone, false, client);

    SendUltimateBattleMemberState(zone, client);
}

void MatchManager::LeaveUltimateBattle(const std::shared_ptr<
    ChannelClientConnection>& client, const std::shared_ptr<Zone>& zone)
{
    auto state = client->GetClientState();
    auto ubMatch = zone->GetUBMatch();
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(!ubMatch)
        {
            return;
        }
        else if(ubMatch->GetState() <= objects::UBMatch::State_t::READY)
        {
            // Remove from match but don't do anything else because
            // people can still get in later
            ubMatch->RemoveMemberIDs(state->GetWorldCID());
            ubMatch->RemoveSpectatorIDs(state->GetWorldCID());
            return;
        }
    }

    bool end = zone->GetConnectionList().size() == 0;
    if(!end)
    {
        // If no players are left, end the match
        end = ubMatch->MemberIDsCount() == 0;
    }

    if(ubMatch->MemberIDsContains(state->GetWorldCID()))
    {
        SendUltimateBattleMembers(zone);
    }

    if(end)
    {
        EndUltimateBattle(zone);
    }
}

bool MatchManager::StartPvPBaseCapture(
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
        notify.WriteS8((int8_t)teamID);
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

bool MatchManager::ToggleDiasporaBase(const std::shared_ptr<Zone>& zone,
    int32_t baseID, int32_t sourceEntityID, bool capture)
{
    if(!zone)
    {
        return false;
    }

    auto instance = zone->GetInstance();
    auto variant = instance ? instance->GetVariant() : nullptr;

    auto bState = zone->GetDiasporaBase(baseID);
    if(bState && variant &&
        variant->GetInstanceType() == InstanceType_t::DIASPORA)
    {
        auto base = bState->GetEntity();
        auto def = base->GetDefinition();

        float resetTime = 0.f;
        {
            std::lock_guard<std::mutex> lock(mLock);
            if(capture)
            {
                if(base->GetCaptured())
                {
                    // Captured by someone else
                    return false;
                }

                uint32_t duration = def->GetCaptureDuration();

                base->SetCaptured(true);

                resetTime = (float)duration;

                // Don't bother actually scheduling the reset time if it is longer
                // than the entire match
                if(duration <= (uint32_t)variant->GetTimePoints(1))
                {
                    uint64_t reset = ChannelServer::GetServerTime() +
                        (uint64_t)((uint64_t)duration * 1000000ULL);
                    base->SetResetTime(reset);

                    mServer.lock()->GetTimerManager()->ScheduleEventIn(
                        (int)duration, []
                        (MatchManager* pMatchManager, int32_t pBaseID,
                            uint32_t pZoneID, uint32_t pInstanceID,
                            uint64_t pReset)
                        {
                            pMatchManager->ResetDiasporaBase(pZoneID, pInstanceID,
                                pBaseID, pReset);
                        }, this, baseID, zone->GetID(), instance->GetID(), reset);
                }
            }
            else
            {
                if(!base->GetCaptured())
                {
                    // Nothing to do
                    return true;
                }

                base->SetCaptured(false);
                base->SetResetTime(0);
            }
        }

        auto server = mServer.lock();
        auto zoneManager = server->GetZoneManager();

        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_DIASPORA_BASE_STATUS);
        notify.WriteS32Little(baseID);
        notify.WriteS32Little(capture ? 0 : 1);
        notify.WriteFloat(resetTime);

        zoneManager->BroadcastPacket(zone, notify);

        // Fire triggers for all default (0) triggers or any that match the
        // ID of the tower that was reset/captured
        auto triggers = zoneManager->GetZoneTriggers(zone,
            capture ? ZoneTrigger_t::ON_DIASPORA_BASE_CAPTURE :
            ZoneTrigger_t::ON_DIASPORA_BASE_RESET);
        triggers.remove_if([def](
            const std::shared_ptr<objects::ServerZoneTrigger>& t)
            {
                return t->GetValue() != 0 &&
                    (uint32_t)t->GetValue() != def->GetID();
            });

        if(triggers.size() > 0)
        {
            zoneManager->HandleZoneTriggers(zone, triggers);
        }

        // Also execute any actions directly on the bound object if they
        // exist (with source entity bound)
        if(capture)
        {
            auto obj = base->GetBoundObject();
            if(obj && obj->ActionsCount() > 0)
            {
                server->GetActionManager()->PerformActions(nullptr,
                    obj->GetActions(), sourceEntityID, zone);
            }
        }

        return true;
    }

    return false;
}

bool MatchManager::ResetDiasporaBase(uint32_t zoneID, uint32_t instanceID,
    int32_t baseID, uint64_t resetTime)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto instance = zoneManager->GetInstance(instanceID);
    auto zone = instance ? instance->GetZone(zoneID) : nullptr;
    auto bState = zone ? zone->GetDiasporaBase(baseID) : nullptr;
    if(!bState || bState->GetEntity()->GetResetTime() != resetTime)
    {
        return false;
    }

    return ToggleDiasporaBase(zone, baseID, 0, false);
}

bool MatchManager::StartStopMatch(const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<objects::Match>& match)
{
    if(!zone)
    {
        return false;
    }

    if(match && match->GetType() == objects::Match::Type_t::ULTIMATE_BATTLE)
    {
        auto ubMatch = std::dynamic_pointer_cast<objects::UBMatch>(match);
        if(!ubMatch)
        {
            // UB matches need to be the derived type
            return false;
        }
        else if(ubMatch->GetQueueDuration() < ubMatch->GetAnnounceDuration())
        {
            // Timers set incorrectly
            return false;
        }
        else if(zone->GetInstance() && zone->GetInstance()->GetMatch())
        {
            return false;
        }
    }

    if(zone && (match != nullptr) == (zone->GetMatch() == nullptr))
    {
        auto ubMatch = std::dynamic_pointer_cast<objects::UBMatch>(match);

        auto instance = !ubMatch ? zone->GetInstance() : nullptr;

        std::list<std::shared_ptr<Zone>> zones = { zone };
        {
            std::lock_guard<std::mutex> lock(mLock);

            if(instance)
            {
                // All instances and zones must have the same match state
                if(instance->GetMatch() != zone->GetMatch())
                {
                    return false;
                }

                zones = instance->GetZones();
                for(auto z : zones)
                {
                    if(z->GetMatch() != zone->GetMatch())
                    {
                        return false;
                    }
                }

                instance->SetMatch(match);

                if(match)
                {
                    match->SetInstanceID(instance->GetID());
                    match->SetInstanceDefinitionID(instance->GetDefinition()
                        ->GetID());

                    auto variant = instance->GetVariant();
                    match->SetVariantID(variant ? variant->GetID() : 0);
                }
            }
            else if(match)
            {
                match->SetZoneDefinitionID(zone->GetDefinitionID());
                match->SetDynamicMapID(zone->GetDynamicMapID());
            }

            for(auto z : zones)
            {
                z->SetMatch(match);
            }

            if(ubMatch)
            {
                LogMatchManagerDebugMsg("Queueing Ultimate Battle match\n");

                ubMatch->SetState(objects::UBMatch::State_t::PREMATCH);

                uint8_t queueTime = ubMatch->GetQueueDuration();

                uint64_t now = ChannelServer::GetServerTime();
                ubMatch->SetTimerStart(now);
                ubMatch->SetTimerExpire(now +
                    (uint64_t)((uint64_t)queueTime * 60000000ULL));
            }
        }

        if(ubMatch)
        {
            // Kick everyone currently in the match zone(s)
            auto zoneManager = mServer.lock()->GetZoneManager();
            for(auto z : zones)
            {
                for(auto client : z->GetConnectionList())
                {
                    zoneManager->MoveToLobby(client);
                }
            }

            if(ubMatch->GetCategory() == objects::UBMatch::Category_t::UA)
            {
                // Start match right away
                StartUltimateBattle(zone);
            }
            else
            {
                // Start the queue
                UltimateBattleQueue(zone->GetDefinitionID(),
                    zone->GetDynamicMapID(), zone->GetInstanceID());
            }
        }
        else if(!match)
        {
            // Fire the -1 phase trigger for any cleanup actions
            FirePhaseTriggers(zone, -1);
        }

        return true;
    }

    return false;
}

bool MatchManager::StartUltimateBattle(const std::shared_ptr<Zone>& zone)
{
    if(!zone)
    {
        return false;
    }

    auto ubMatch = zone->GetUBMatch();
    if(!ubMatch)
    {
        return false;
    }

    std::set<int32_t> accepted;
    std::set<int32_t> rejected;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(ubMatch->GetState() != objects::UBMatch::State_t::PREMATCH)
        {
            return false;
        }

        ubMatch->SetState(objects::UBMatch::State_t::READY);
        ubMatch->SetTimerStart(0);
        ubMatch->SetTimerExpire(0);

        if(ubMatch->GetCategory() == objects::UBMatch::Category_t::UB)
        {
            std::set<int32_t> pending;
            for(auto& pair : ubMatch->GetPendingEntries())
            {
                pending.insert(pair.first);
            }

            auto memberLimit = (size_t)ubMatch->GetMemberLimit();
            for(size_t i = 0; i < memberLimit && pending.size() > 0; i++)
            {
                int32_t worldCID = libcomp::Randomizer::GetEntry(pending);
                pending.erase(worldCID);

                ubMatch->RemovePendingEntries(worldCID);
                accepted.insert(worldCID);
            }

            rejected = pending;
        }
    }

    if(ubMatch->GetCategory() == objects::UBMatch::Category_t::UA)
    {
        // Ultimate attack has no queue or pending entries so start
        // the ready timer now and let it expire if no one joins
        uint64_t now = ChannelServer::GetServerTime();
        ubMatch->SetTimerStart(now);
        ubMatch->SetTimerExpire(now + (uint64_t)(
            (uint64_t)ubMatch->GetReadyDuration() * 1000000ULL));

        // Start the pre-match timer now
        UltimateBattleTick(zone->GetDefinitionID(),
            zone->GetDynamicMapID(), zone->GetInstanceID());

        mServer.lock()->GetTimerManager()->ScheduleEventIn(
            (int)ubMatch->GetReadyDuration(), []
            (MatchManager* pMatchManager, uint32_t pZoneID,
            uint32_t pDynamicMapID, uint32_t pInstanceID)
            {
                pMatchManager->UltimateBattleBegin(pZoneID, pDynamicMapID,
                    pInstanceID);
            }, this, zone->GetDefinitionID(), zone->GetDynamicMapID(),
                zone->GetInstanceID());
    }
    else
    {
        bool stopMatch = true;

        auto server = mServer.lock();
        auto clients = server->GetManagerConnection()->GetEntityClients(
            accepted, true);
        if(clients.size() > 0)
        {
            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_UB_LOTTO_RESULT);
            notify.WriteS32Little(1);

            ChannelClientConnection::BroadcastPacket(clients, notify);
            stopMatch = false;
        }

        clients = server->GetManagerConnection()->GetEntityClients(
            rejected, true);
        if(clients.size() > 0)
        {
            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_UB_LOTTO_RESULT);
            notify.WriteS32Little(0);   // Not selected

            ChannelClientConnection::BroadcastPacket(clients, notify);
            stopMatch = false;

            // Schedule recruiting for when the timers expire (10s + countdown)
            server->GetTimerManager()->ScheduleEventIn((int)40, []
                (MatchManager* pMatchManager, uint32_t pZoneID,
                uint32_t pDynamicMapID, uint32_t pInstanceID)
                {
                    pMatchManager->UltimateBattleRecruit(pZoneID,
                        pDynamicMapID, pInstanceID);
                }, this, zone->GetDefinitionID(), zone->GetDynamicMapID(),
                    zone->GetInstanceID());
        }

        if(stopMatch)
        {
            LogMatchManagerDebugMsg(
                "Skipping no entry Ultimate Battle match\n");

            StartStopMatch(zone, nullptr);
            return false;
        }
    }

    LogMatchManagerDebugMsg("Starting Ultimate Battle\n");

    return true;
}

void MatchManager::UltimateBattleRecruit(uint32_t zoneID,
    uint32_t dynamicMapID, uint32_t instanceID)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    auto zone = zoneManager->GetExistingZone(zoneID, dynamicMapID, instanceID);
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(!ubMatch ||
            ubMatch->MemberIDsCount() >= (size_t)ubMatch->GetMemberLimit())
        {
            // No recruiting
            return;
        }
    }

    std::set<int32_t> pendingCIDs;
    for(auto& pair : ubMatch->GetPendingEntries())
    {
        pendingCIDs.insert(pair.first);
    }

    // Get a random ID for confirmations
    uint32_t matchID = RNG(uint32_t, 1, 0x7FFFFFFF);
    ubMatch->SetID(matchID);

    auto clients = server->GetManagerConnection()->GetEntityClients(
        pendingCIDs, true);
    if(clients.size() == 0 && ubMatch->MemberIDsCount() == 0)
    {
        // No one left
        StartStopMatch(zone, nullptr);
        return;
    }

    libcomp::Packet notify;
    notify.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_UB_RECRUIT);
    notify.WriteS8(1);

    ChannelClientConnection::BroadcastPacket(clients, notify);

    // Open up offers in 10 seconds
    server->GetTimerManager()->ScheduleEventIn((int)10, []
        (std::shared_ptr<ChannelServer> pServer,
         std::shared_ptr<objects::UBMatch> pMatch)
        {
            std::set<int32_t> pPendingCIDs;
            for(auto& pair : pMatch->GetPendingEntries())
            {
                pPendingCIDs.insert(pair.first);
            }

            auto pClients = pServer->GetManagerConnection()->GetEntityClients(
                pPendingCIDs, true);

            libcomp::Packet p;
            p.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_UB_RECRUIT_START);
            p.WriteS32Little((int32_t)pMatch->GetID());

            ChannelClientConnection::BroadcastPacket(pClients, p);
        }, server, ubMatch);
}

void MatchManager::UltimateBattleBegin(uint32_t zoneID, uint32_t dynamicMapID,
    uint32_t instanceID)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    auto zone = zoneManager->GetExistingZone(zoneID, dynamicMapID, instanceID);
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    bool end = false;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(!ubMatch ||
            ubMatch->GetState() > objects::UBMatch::State_t::READY)
        {
            // Nothing to do
            return;
        }

        if(ubMatch->MemberIDsCount() == 0)
        {
            ubMatch->SetState(objects::UBMatch::State_t::COMPLETE);
            end = true;
        }
        else
        {
            ubMatch->SetState(objects::UBMatch::State_t::ROUND);
        }
    }

    // Reject any pending entries
    std::set<int32_t> pendingCIDs;
    for(auto& pair : ubMatch->GetPendingEntries())
    {
        pendingCIDs.insert(pair.first);
    }

    if(pendingCIDs.size() > 0)
    {
        auto clients = server->GetManagerConnection()->GetEntityClients(
            pendingCIDs, true);
        for(auto client : clients)
        {
            CleanupPendingMatch(client);
        }

        libcomp::Packet p;
        p.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_UB_RECRUIT);
        p.WriteS8(0);

        ChannelClientConnection::BroadcastPacket(clients, p);
    }

    if(!end)
    {
        // Begin phase 0 to kick off UB
        FirePhaseTriggers(zone, 0);
    }
    else
    {
        EndUltimateBattle(zone);
    }
}

bool MatchManager::UltimateBattleSpectate(int32_t worldCID,
    const std::shared_ptr<Zone>& zone)
{
    auto server = mServer.lock();
    auto client = server->GetManagerConnection()->GetEntityClient(worldCID,
        true);
    if(!client)
    {
        return false;
    }

    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(!ubMatch ||
            ubMatch->GetState() == objects::UBMatch::State_t::PREMATCH ||
            ubMatch->GetState() == objects::UBMatch::State_t::COMPLETE)
        {
            return false;
        }
        else if(ubMatch->SpectatorIDsContains(worldCID))
        {
            // Already spectating
            return true;
        }
        else if(ubMatch->SpectatorIDsCount() >=
            (size_t)ubMatch->GetSpectatorLimit())
        {
            return false;
        }

        ubMatch->InsertSpectatorIDs(worldCID);
    }

    return true;
}

bool MatchManager::StartUltimateBattleTimer(const std::shared_ptr<Zone>& zone,
    uint32_t duration, const libcomp::String& eventID, bool endPhase)
{
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(!ubMatch ||
            ubMatch->GetState() == objects::UBMatch::State_t::COMPLETE ||
            ubMatch->GetState() < objects::UBMatch::State_t::ROUND)
        {
            // Cannot start timer now
            return false;
        }

        if(endPhase)
        {
            EndUltimateBattlePhase(zone, false);
        }
    }

    uint64_t serverTime = ChannelServer::GetServerTime();

    uint64_t expire = (uint64_t)(serverTime +
        (uint64_t)(duration * 1000000ULL));

    ubMatch->SetTimerStart(serverTime);
    ubMatch->SetTimerExpire(expire);
    ubMatch->SetTimerEventID(eventID);

    LogMatchManagerDebug([&]()
    {
        return libcomp::String("Starting Ultimate Battle timer: %1s"
            " (phase %2)\n").Arg(duration).Arg(ubMatch->GetPhase());
    });

    SendPhase(zone, true);

    return true;
}

bool MatchManager::AdvancePhase(const std::shared_ptr<Zone>& zone, int8_t to,
    int8_t from)
{
    auto instance = zone->GetInstance();
    auto match = zone ? zone->GetMatch() : nullptr;

    int8_t oldPhase = 0;
    bool explicitTo = to != -1;
    if(!match)
    {
        return false;
    }
    else
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(to == -1)
        {
            // Advance to next
            to = (int8_t)(match->GetPhase() + 1);
        }

        if(from >= 0 && match->GetPhase() != from)
        {
            // Current phase does not match
            return false;
        }

        oldPhase = match->GetPhase();
        if(oldPhase > (int8_t)to)
        {
            // Invalid next phase
            return false;
        }

        bool valid = false;
        switch(match->GetType())
        {
        case objects::Match::Type_t::DIASPORA:
            if(instance)
            {
                valid = to <= DIASPORA_PHASE_END;
            }
            break;
        case objects::Match::Type_t::ULTIMATE_BATTLE:
            valid = to <= (UB_PHASE_MAX + 1);
            break;
        default:
            // No restrictions
            valid = true;
            break;
        }

        if(valid)
        {
            match->SetPhase(to);
        }
        else
        {
            return false;
        }
    }

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    switch(match->GetType())
    {
    case objects::Match::Type_t::DIASPORA:
        {
            SendPhase(zone);

            if(to == 1)
            {
                // Reset the timer for the actual match
                instance->SetTimerStart(0);
                instance->SetTimerStop(0);
                instance->SetTimerExpire(0);

                zoneManager->StartInstanceTimer(instance);
            }
            else if(to == DIASPORA_PHASE_END)
            {
                // Instance complete
                zoneManager->StopInstanceTimer(instance);
            }
        }
        break;
    case objects::Match::Type_t::ULTIMATE_BATTLE:
        // Phases are sent by timer updates since they are all timed
        if(to > UB_PHASE_MAX)
        {
            // Do not actually advance, just end the match
            match->SetPhase(oldPhase);

            auto ubMatch = std::dynamic_pointer_cast<objects::UBMatch>(match);
            if(explicitTo && ubMatch)
            {
                // Explicitly setting outside of the bounds of the limit
                // is a failure signifier
                ubMatch->SetResult(1);  // Generic failure
            }

            EndUltimateBattle(zone);

            // Do not fire post final round trigger
            return true;
        }
        break;
    default:
        break;
    }

    FirePhaseTriggers(zone, to);

    switch(match->GetType())
    {
    case objects::Match::Type_t::DIASPORA:
        // Update zone tracking
        for(auto z : instance->GetZones())
        {
            zoneManager->UpdateTrackedZone(z);
        }
        break;
    default:
        break;
    }

    return true;
}

void MatchManager::UltimateBattleQueue(uint32_t zoneID, uint32_t dynamicMapID,
    uint32_t instanceID)
{
    auto server = mServer.lock();
    auto zone = server->GetZoneManager()->GetExistingZone(zoneID, dynamicMapID,
        instanceID);
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    if(!ubMatch || ubMatch->GetState() != objects::UBMatch::State_t::PREMATCH)
    {
        return;
    }

    uint64_t now = ChannelServer::GetServerTime();
    uint64_t start = ubMatch->GetTimerStart();
    uint64_t expire = ubMatch->GetTimerExpire();
    int32_t timeLeft = expire > now
        ? (int32_t)((expire - now) / 1000000ULL) : 0;

    if(!timeLeft || !start || start > expire)
    {
        StartUltimateBattle(zone);
    }
    else
    {
        // Notify players in the lobby zone and reschedule

        // UA does not announce anything
        if(ubMatch->GetCategory() == objects::UBMatch::Category_t::UB)
        {
            // Round up to the closest minute so the time displays right
            int32_t minutesLeft = (int32_t)ceil((double)timeLeft / 60.0);

            auto lobbyDef = server->GetServerDataManager()->GetZoneData(
                zone->GetDefinition()->GetGroupID(), 0);
            auto lobby = lobbyDef ?server->GetZoneManager()->GetGlobalZone(
                lobbyDef->GetID(), lobbyDef->GetDynamicMapID()) : nullptr;
            if(lobby && minutesLeft <= (int32_t)ubMatch->GetAnnounceDuration())
            {
                libcomp::Packet notify;
                notify.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_UB_LOTTO_UPDATE);
                notify.WriteU32Little(ubMatch->GetSubType());
                notify.WriteS32Little((int32_t)(minutesLeft * 60));
                notify.WriteS8(1);  // Unknown
                notify.WriteS32Little((int32_t)ubMatch->PendingEntriesCount());

                server->GetZoneManager()->BroadcastPacket(lobby, notify);
            }
        }

        // Notify every 60 seconds
        int next = (int)timeLeft;
        if(next > 60)
        {
            next = 60;
        }

        server->GetTimerManager()->ScheduleEventIn(next, []
            (MatchManager* pMatchManager, uint32_t pZoneID,
            uint32_t pDynamicMapID, uint32_t pInstanceID)
            {
                pMatchManager->UltimateBattleQueue(pZoneID, pDynamicMapID,
                    pInstanceID);
            }, this, zoneID, dynamicMapID, instanceID);
    }
}

void MatchManager::UltimateBattleTick(uint32_t zoneID, uint32_t dynamicMapID,
    uint32_t instanceID)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    auto zone = zoneManager->GetExistingZone(zoneID, dynamicMapID, instanceID);
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    if(!ubMatch)
    {
        return;
    }

    uint64_t now = ChannelServer::GetServerTime();
    uint64_t next = ubMatch->GetNextTick();

    if(now >= next)
    {
        // Process normal tick actions
        if(!next)
        {
            next = now;
        }

        ubMatch->SetPreviousTick(next);

        // Tick every 5 seconds
        ubMatch->SetNextTick((uint64_t)(next + 5000000ULL));

        // Update member states
        SendUltimateBattleMemberState(zone);

        if(ubMatch->GetState() == objects::UBMatch::State_t::ROUND)
        {
            // Determine the current gauge "speed" based upon original kill
            // values instead of the decreased ones
            int32_t gSpeed = 0;

            for(auto enemy : zone->GetEnemies())
            {
                auto spawn = enemy->GetEnemyBase()->GetSpawnSource();
                if(spawn && spawn->GetKillValueType() ==
                    objects::Spawn::KillValueType_t::UB_POINTS)
                {
                    int32_t currentKill = enemy->GetKillValue();
                    int32_t spawnKill = spawn->GetKillValue();

                    // Kill values lower each tick (maximum 1/10)
                    int32_t minKill = (int32_t)floor(spawnKill / 10);
                    if(currentKill > minKill)
                    {
                        currentKill = (int32_t)floor((double)currentKill *
                            (double)ubMatch->GetKillValueDecay());
                        if(currentKill < minKill)
                        {
                            currentKill = minKill;
                        }

                        enemy->SetKillValue(currentKill);
                    }

                    // Raise the gauge speed
                    gSpeed = gSpeed + (int32_t)((double)spawnKill *
                        (double)ubMatch->GetGaugeScale());
                }
            }

            // Defeated enemies lower the gauge speed
            auto killed = ubMatch->GetRecentlyKilled();
            if(killed.size() > 0)
            {
                for(size_t i = 0; i < killed.size(); i++)
                {
                    // Remove one by one in case another kill is added
                    // right now
                    ubMatch->RemoveRecentlyKilled(0);
                }

                // Add constant decay if anything was killed
                gSpeed = gSpeed - (int32_t)ubMatch->GetGaugeDecay();

                for(auto spawn : killed)
                {
                    gSpeed = gSpeed - (int32_t)((double)spawn->GetKillValue() *
                        (double)ubMatch->GetGaugeDecayScale());
                }
            }
            else if(!gSpeed)
            {
                // Add only constant decay if nothing increased it
                gSpeed = gSpeed - (int32_t)ubMatch->GetGaugeDecay();
            }

            if(ubMatch->GetCategory() == objects::UBMatch::Category_t::UA)
            {
                // No gauge
                gSpeed = 0;
            }

            int32_t gauge = (int32_t)ubMatch->GetGauge();
            if(gSpeed)
            {
                gauge = gauge + gSpeed;

                // Check min/max
                if(gauge < 0)
                {
                    gauge = 0;
                }
                else if(gauge > 1000000)
                {
                    gauge = 1000000;
                }

                ubMatch->SetGauge((uint32_t)gauge);
            }

            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_UB_STATE);
            p.WriteU32Little(ubMatch->GetSubType());
            p.WriteFloat((float)gauge * 0.0001f);
            p.WriteS32Little(0);
            p.WriteU32Little(ubMatch->GetDarkLimit() &&
                ubMatch->GetGauge() >= ubMatch->GetDarkLimit() ? 1 : 0);

            ChannelClientConnection::BroadcastPacket(zone
                ->GetConnectionList(), p);

            uint32_t pGauge = ubMatch->GetPreviousGauge();
            ubMatch->SetPreviousGauge((uint32_t)gauge);

            // Fire gauge/tick triggers
            auto triggers = zoneManager->GetZoneTriggers(zone,
                ZoneTrigger_t::ON_UB_TICK);
            for(auto trigger : zoneManager->GetZoneTriggers(zone,
                ZoneTrigger_t::ON_UB_GAUGE_OVER))
            {
                if(trigger->GetValue() < (int32_t)pGauge &&
                    trigger->GetValue() >= (int32_t)gauge)
                {
                    triggers.push_back(trigger);
                }
            }

            for(auto trigger : zoneManager->GetZoneTriggers(zone,
                ZoneTrigger_t::ON_UB_GAUGE_UNDER))
            {
                if(trigger->GetValue() > (int32_t)pGauge &&
                    trigger->GetValue() <= (int32_t)gauge)
                {
                    triggers.push_back(trigger);
                }
            }

            zoneManager->HandleZoneTriggers(zone, triggers);
        }
    }

    uint64_t expire = ubMatch->GetTimerExpire();
    if(expire && expire <= now)
    {
        // Handle timer expiration
        libcomp::String eventID = ubMatch->GetTimerEventID();

        ubMatch->SetTimerStart(0);
        ubMatch->SetTimerExpire(0);
        ubMatch->SetTimerEventID("");

        if(!eventID.IsEmpty())
        {
            server->GetEventManager()->HandleEvent(nullptr,
                eventID, 0, zone);
        }

        // Get the new expiratin if set
        expire = ubMatch->GetTimerExpire();
    }

    // Offset tick to match timer events as needed
    if(expire && expire < next)
    {
        next = expire;
    }

    server->ScheduleWork(next, []
        (MatchManager* pMatchManager, uint32_t pZoneID,
        uint32_t pDynamicMapID, uint32_t pInstanceID)
        {
            pMatchManager->UltimateBattleTick(pZoneID, pDynamicMapID,
                pInstanceID);
        }, this, zoneID, dynamicMapID, instanceID);
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
            character ? character->GetName() : "", true);
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

void MatchManager::SendDiasporaLocation(const std::shared_ptr<
    ChannelClientConnection>& client, uint32_t instanceID, bool enter,
    bool others)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    auto instance = zoneManager->GetInstance(instanceID);
    if(!instance)
    {
        return;
    }

    // Only send to clients that are ready so players are not sent
    // multiple times
    auto clients = instance->GetConnections();
    clients.remove_if([client](
        const std::shared_ptr<ChannelClientConnection>& c)
        {
            return c == client || !c->GetClientState()
                ->GetCharacterState()->Ready();
        });

    if(clients.size() == 0)
    {
        return;
    }

    auto send = clients;
    auto sendTo = clients;
    if(others || !enter)
    {
        send.clear();
        send.push_back(client);
    }
    else
    {
        sendTo.clear();
        sendTo.push_back(client);
    }

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DIASPORA_MEMBER);
    p.WriteU32Little((uint32_t)send.size());

    for(auto c : send)
    {
        auto state = c->GetClientState();

        p.WriteS32Little(state->GetWorldCID());
        p.WriteS32Little(enter ? 0 : 1);

        if(enter)
        {
            auto cState = state->GetCharacterState();
            auto character = cState->GetEntity();

            p.WriteS32Little(cState->GetEntityID());
            p.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                character ? character->GetName() : "", true);
        }
    }

    ChannelClientConnection::BroadcastPacket(sendTo, p);
}

void MatchManager::SendUltimateBattleRankings(
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto state = client->GetClientState();

    time_t systemTime = std::time(0) + server->GetServerTimeOffset();
    tm* t = gmtime(&systemTime);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_UB_RANKING);
    p.WriteU32Little(1);    // Unknown, always 1
    p.WriteS32Little((int32_t)t->tm_year);  // Current year

    // Normal UB rankings
    auto current = LoadUltimateBattleData(client);
    auto allTime = state->GetUltimateBattleData(1).Get();
    auto ubRankings = mUBRankings;

    p.WriteS8(4);   // Categories
    for(size_t i = 0; i < 4; i++)
    {
        std::list<std::shared_ptr<objects::UBResult>> results;
        for(size_t k = 0; k < 10; k++)
        {
            auto result = ubRankings[i][k];
            if(result)
            {
                results.push_back(result);
            }
            else
            {
                break;
            }
        }

        p.WriteS32Little((int32_t)results.size());
        for(auto result : results)
        {
            auto character = libcomp::PersistentObject::
                LoadObjectByUUID<objects::Character>(server
                    ->GetWorldDatabase(), result->GetCharacter());
            switch(i)
            {
            case 0:
                p.WriteS8((int8_t)result->GetAllTimeRank());
                break;
            case 1:
                p.WriteS8((int8_t)result->GetTopPointRank());
                break;
            case 2:
            case 3:
            default:
                p.WriteS8((int8_t)result->GetTournamentRank());
                break;
            }

            p.WriteString16Little(state->GetClientStringEncoding(),
                character ? character->GetName() : "", true);

            switch(i)
            {
            case 1:
                p.WriteU32Little(result ? result->GetTopPoints() : 0);
                break;
            default:
                p.WriteU32Little(result ? result->GetPoints() : 0);
                break;
            }
        }
    }

    p.WriteU32Little(allTime ? allTime->GetPoints() : 0);
    p.WriteU32Little(allTime ? allTime->GetTopPoints() : 0);
    p.WriteU32Little(current ? current->GetPoints() : 0);

    client->SendPacket(p);
}

std::shared_ptr<objects::UBTournament>MatchManager::GetUBTournament() const
{
    return mUBTournament;
}

void MatchManager::UpdateUBTournament(const std::shared_ptr<
    objects::UBTournament>& tournament)
{
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(mUBTournament != tournament)
        {
            // Tournament updated
            mUBTournament = tournament;
        }
        else
        {
            // No need to update
            return;
        }
    }

    // Now update all rankings
    std::list<std::shared_ptr<objects::UBResult>> empty;
    UpdateUBRankings(empty);
}

void MatchManager::UpdateUBRankings(
    const std::list<std::shared_ptr<objects::UBResult>>& updated)
{
    std::lock_guard<std::mutex> lock(mLock);

    bool reload = updated.size() == 0;
    if(!reload)
    {
        // Pull all results into a set to see if any have been updated
        std::set<std::shared_ptr<objects::UBResult>> currentResults;
        for(size_t i = 0; i < 4; i++)
        {
            for(size_t k = 0; k < 10; k++)
            {
                if(mUBRankings[i][k])
                {
                    currentResults.insert(mUBRankings[i][k]);
                }
            }
        }

        for(auto result : updated)
        {
            // If the result is already in the set or is ranked, reload
            // the set
            if(currentResults.find(result) != currentResults.end() ||
              result->GetRanked() || result->GetTournamentRank())
            {
                reload = true;
                break;
            }
        }
    }

    if(!reload)
    {
        return;
    }

    std::unordered_map<size_t, libobjgen::UUID> tournamentIDs;
    tournamentIDs[2] = mUBTournament ? mUBTournament->GetPrevious() : NULLUUID;
    tournamentIDs[3] = mUBTournament ? mUBTournament->GetUUID() : NULLUUID;

    // Load results for current and previous
    for(size_t i = 2; i < 4; i++)
    {
        mUBRankings[i] = { { nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr } };

        if(tournamentIDs[i] != NULLUUID)
        {
            auto results = objects::UBResult::LoadUBResultListByTournament(
                mServer.lock()->GetWorldDatabase(), tournamentIDs[i]);
            results.sort([](const std::shared_ptr<objects::UBResult>& a,
                const std::shared_ptr<objects::UBResult>& b)
                {
                    return a->GetTournamentRank() < b->GetTournamentRank();
                });

            for(size_t k = 0; k < 10; k++)
            {
                if(results.size() > 0)
                {
                    mUBRankings[i][k] = results.front();
                    results.pop_front();
                }
            }
        }
    }

    // Now load the results independent of tournaments
    std::list<std::shared_ptr<objects::UBResult>> allTime;
    std::list<std::shared_ptr<objects::UBResult>> topPoint;
    for(auto result : objects::UBResult::LoadUBResultListByRanked(
        mServer.lock()->GetWorldDatabase(), true))
    {
        if(result->GetAllTimeRank())
        {
            allTime.push_back(result);
        }

        if(result->GetTopPointRank())
        {
            topPoint.push_back(result);
        }
    }

    allTime.sort([](const std::shared_ptr<objects::UBResult>& a,
        const std::shared_ptr<objects::UBResult>& b)
        {
            return a->GetAllTimeRank() < b->GetAllTimeRank();
        });

    topPoint.sort([](const std::shared_ptr<objects::UBResult>& a,
        const std::shared_ptr<objects::UBResult>& b)
        {
            return a->GetTopPointRank() < b->GetTopPointRank();
        });

    size_t idx = 0;
    for(auto* resultSet : { &allTime, &topPoint })
    {
        mUBRankings[idx] = { { nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr } };

        for(size_t i = 0; i < 10; i++)
        {
            if(resultSet->size() > 0)
            {
                mUBRankings[idx][i] = resultSet->front();
                resultSet->pop_front();
            }
        }

        idx++;
    }
}

std::shared_ptr<objects::PentalphaMatch> MatchManager::GetPentalphaMatch(
    bool previous)
{
    std::lock_guard<std::mutex> lock(mLock);
    return mPentalphaMatches[previous ? 1 : 0];
}

void MatchManager::UpdatePentalphaMatch(const std::shared_ptr<
    objects::PentalphaMatch>& match)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(mPentalphaMatches[0] != match)
    {
        // Match updated
        mPentalphaMatches[0] = match;

        // Load previous
        mPentalphaMatches[1] = nullptr;
        if(match && !match->GetPrevious().IsNull())
        {
            auto db = mServer.lock()->GetWorldDatabase();
            mPentalphaMatches[1] = libcomp::PersistentObject::LoadObjectByUUID<
                objects::PentalphaMatch>(db, match->GetPrevious());
        }
    }
}

void MatchManager::UpdateMatchEntries(
    const std::list<std::shared_ptr<objects::MatchEntry>>& updates,
    const std::list<std::shared_ptr<objects::MatchEntry>>& removes)
{
    auto server = mServer.lock();
    auto managerConnection = server->GetManagerConnection();

    std::lock_guard<std::mutex> lock(mLock);

    // Leave PvP queuing up to the primary channel
    if(server->GetChannelID() == 0)
    {
        std::unordered_map<int8_t, std::set<uint32_t>> currentReadyTimes;
        for(auto& pair : mMatchEntries)
        {
            auto entry = pair.second;
            if(IsPvPMatchEntry(entry) && entry->GetReadyTime())
            {
                int8_t type = (int8_t)entry->GetMatchType();
                currentReadyTimes[type].insert(entry->GetReadyTime());
            }
        }

        std::unordered_map<int8_t, std::set<uint32_t>> newReadyTimes;
        for(auto update : updates)
        {
            if(IsPvPMatchEntry(update) && update->GetReadyTime())
            {
                int8_t type = (int8_t)update->GetMatchType();
                newReadyTimes[type].insert(update->GetReadyTime());
            }
        }

        for(auto& pair : newReadyTimes)
        {
            for(uint32_t time : pair.second)
            {
                if(currentReadyTimes[pair.first].find(time) ==
                    currentReadyTimes[pair.first].end())
                {
                    QueuePendingPvPMatch((uint8_t)pair.first, time);
                }
            }
        }
    }

    bool pvpModified = false;

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

        pvpModified |= IsPvPMatchEntry(update);
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

        pvpModified |= IsPvPMatchEntry(remove);
    }

    if(!pvpModified)
    {
        return;
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

    std::list<std::shared_ptr<objects::MatchEntry>> pvpEntries;
    for(auto& pair : mMatchEntries)
    {
        if(IsPvPMatchEntry(pair.second))
        {
            pvpEntries.push_back(pair.second);
        }
        else
        {
            continue;
        }

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
        for(auto entry : pvpEntries)
        {
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

    uint8_t channelID = server->GetChannelID();

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
                            server->GetZoneManager()->MoveToInstance(client);

                            client->FlushOutgoing();
                        }
                        else
                        {
                            auto state = client->GetClientState();
                            state->SetPendingMatch(match);

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

std::shared_ptr<objects::UBResult> MatchManager::LoadUltimateBattleData(
    const std::shared_ptr<ChannelClientConnection>& client, uint8_t idxFlags,
    bool createMissing)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    bool reload = false;
    auto current = GetUBTournament();
    if(idxFlags & 0x01)
    {
        auto result = state->GetUltimateBattleData(0).Get();
        if(current && (!result ||
            (result->GetTournament().GetUUID() != current->GetUUID())))
        {
            reload = true;
        }
    }

    if((idxFlags & 0x02) && !state->GetUltimateBattleData(1).Get())
    {
        reload = true;
    }

    if(reload)
    {
        // Clear current results
        state->SetUltimateBattleData(0, NULLUUID);
        state->SetUltimateBattleData(1, NULLUUID);

        for(auto result : objects::UBResult::LoadUBResultListByCharacter(
            mServer.lock()->GetWorldDatabase(), cState->GetEntityUUID()))
        {
            if(!result->GetTournament().IsNull())
            {
                // Current results
                if(current && result->GetTournament().GetUUID() ==
                    current->GetUUID())
                {
                    state->SetUltimateBattleData(0, result);
                }
            }
            else
            {
                // All time results
                state->SetUltimateBattleData(1, result);
            }
        }

        bool missingCurrent = state->GetUltimateBattleData(0).IsNull() &&
            current;
        if(createMissing)
        {
            auto dbChanges = libcomp::DatabaseChangeSet::Create(state
                ->GetAccountUID());

            if((idxFlags & 0x01) && missingCurrent)
            {
                auto result = libcomp::PersistentObject::New<
                    objects::UBResult>(true);
                result->SetCharacter(cState->GetEntityUUID());
                result->SetTournament(current->GetUUID());
                state->SetUltimateBattleData(0, result);

                dbChanges->Insert(result);
            }

            if((idxFlags & 0x02) && state->GetUltimateBattleData(1).IsNull())
            {
                auto result = libcomp::PersistentObject::New<
                    objects::UBResult>(true);
                result->SetCharacter(cState->GetEntityUUID());
                state->SetUltimateBattleData(1, result);

                dbChanges->Insert(result);
            }

            mServer.lock()->GetWorldDatabase()->ProcessChangeSet(dbChanges);
        }
    }

    // Return the first requested entry
    for(uint8_t i = 0; i < 2; i++)
    {
        if(idxFlags & (1 >> i))
        {
            return state->GetUltimateBattleData(i).Get();
        }
    }

    return nullptr;
}

std::shared_ptr<objects::PentalphaEntry> MatchManager::LoadPentalphaData(
    const std::shared_ptr<ChannelClientConnection>& client, uint8_t idxFlags)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    bool first = true;
    std::shared_ptr<objects::PentalphaEntry> result;
    for(uint8_t i = 0; i < 2; i++)
    {
        if((idxFlags & (1 >> i)) == 0) continue;

        auto entry = state->GetPentalphaData(i).Get();
        auto match = GetPentalphaMatch(i == 1);
        if(match && (!entry || (entry->GetMatch() != match->GetUUID())))
        {
            // Entry does not belong to current/previous, reload
            entry = nullptr;
            for(auto e : objects::PentalphaEntry::LoadPentalphaEntryListByMatch(
                mServer.lock()->GetWorldDatabase(), match->GetUUID()))
            {
                if(e->GetCharacter() == cState->GetEntityUUID())
                {
                    entry = e;
                    break;
                }
            }

            state->SetPentalphaData(i, entry);
        }

        if(first)
        {
            result = entry;
        }

        first = false;
    }

    return result;
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

bool MatchManager::SpectatingMatch(const std::shared_ptr<
    ChannelClientConnection>& client, const std::shared_ptr<Zone>& zone)
{
    auto state = client->GetClientState();
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    return ubMatch && !ubMatch->MemberIDsContains(state->GetWorldCID());
}

bool MatchManager::CreatePvPInstance(const std::shared_ptr<
    objects::PvPMatch>& match)
{
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    auto variantDef = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
        serverDataManager->GetZoneInstanceVariantData(match->GetVariantID()));
    if(!variantDef || !variantDef->GetDefaultInstanceID())
    {
        LogMatchManagerError([&]()
        {
            return libcomp::String("Invalid PvP variant encountered, match"
                " creation failed: %1\n").Arg(match->GetVariantID());
        });

        return false;
    }

    auto cids = match->GetMemberIDs();
    uint32_t instanceDefID = match->GetInstanceDefinitionID();
    if(!instanceDefID)
    {
        instanceDefID = variantDef->GetDefaultInstanceID();
        match->SetInstanceDefinitionID(variantDef->GetDefaultInstanceID());
    }

    auto instAccess = std::make_shared<objects::InstanceAccess>();
    instAccess->SetAccessCIDs(cids);
    instAccess->SetDefinitionID(instanceDefID);
    instAccess->SetVariantID(match->GetVariantID());

    server->GetZoneManager()->CreateInstance(instAccess);
    auto instance = server->GetZoneManager()->GetInstance(instAccess
        ->GetInstanceID());
    if(!instance)
    {
        LogMatchManagerError([&]()
        {
            return libcomp::String("Failed to create PvP instance"
                " variant: %1\n").Arg(match->GetVariantID());
        });

        return false;
    }

    match->SetInstanceID(instance->GetID());

    auto pvpStats = std::make_shared<objects::PvPInstanceStats>();
    pvpStats->SetMatch(match);

    instance->SetMatch(match);
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

void MatchManager::QueuePendingPvPMatch(uint8_t type, uint32_t readyTime)
{
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    auto variantIDs = serverDataManager->GetStandardPvPVariantIDs(type);
    if(variantIDs.size() == 0)
    {
        LogMatchManagerError([&]()
        {
            return libcomp::String("PvP match queuing failed due to"
                " undefined match type variants: %1\n").Arg(type);
        });

        return;
    }

    uint32_t variantID = libcomp::Randomizer::GetEntry(variantIDs);
    auto variantDef = std::dynamic_pointer_cast<objects::PvPInstanceVariant>(
        serverDataManager->GetZoneInstanceVariantData(variantID));
    if(!variantDef || !variantDef->GetDefaultInstanceID())
    {
        LogMatchManagerError([&]()
        {
            return libcomp::String("Invalid PvP variant encountered, match"
                " creation failed: %1\n").Arg(variantID);
        });

        return;
    }

    auto instDef = serverDataManager->GetZoneInstanceData(variantDef
        ->GetDefaultInstanceID());

    auto pvpMatch = std::make_shared<objects::PvPMatch>();
    pvpMatch->SetType((objects::PvPMatch::Type_t)type);
    pvpMatch->SetReadyTime(readyTime);
    pvpMatch->SetZoneDefinitionID(instDef->GetZoneIDs(0));
    pvpMatch->SetDynamicMapID(instDef->GetDynamicMapIDs(0));
    pvpMatch->SetInstanceDefinitionID(instDef->GetID());
    pvpMatch->SetVariantID(variantID);

    // Channel modes does not matter here, one needs to be picked
    pvpMatch->SetChannelID(server->GetWorldSharedConfig()
        ->GetChannelDistribution(instDef->GetGroupID()));

    server->GetChannelSyncManager()->SyncRecordUpdate(pvpMatch, "PvPMatch");
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
    if(match->GetType() != objects::Match::Type_t::PVP_FATE &&
        match->GetType() != objects::Match::Type_t::PVP_VALHALLA)
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

    if(match->GetType() == objects::Match::Type_t::PVP_VALHALLA)
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
                for(auto zone : instance->GetZones())
                {
                    for(auto bState : zone->GetPvPBases())
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
                    boost = ((int8_t)match->GetType() == 0) ? 3.f : 1.f;
                    break;
                case LOSS_DEATH_KRATE:
                    // No boost
                    break;
                default:
                    // 50/10%
                    boost = ((int8_t)match->GetType() == 0) ? 0.5f : 0.1f;
                    break;
                }

                stats->SetTrophyBoost(boost + stats->GetTrophyBoost());
            }
        }
    }

    // Shift trophies for valhalla if needed
    if(match->GetType() == objects::Match::Type_t::PVP_VALHALLA)
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
            LogMatchManagerDebugMsg("Match entry validation failed: teams must "
                "have at least 2 members\n");

            return false;
        }

        auto first = clients.front();
        auto state = first->GetClientState();
        auto team = state->GetTeam();

        if(!team && (int8_t)team->GetCategory() != teamCategory)
        {
            LogMatchManagerDebugMsg("Match entry validation failed: invalid "
                "team type encountered\n");

            return false;
        }

        std::set<int32_t> memberIDs = team->GetMemberIDs();
        if(memberIDs.size() != clients.size())
        {
            LogMatchManagerDebugMsg("Match entry validation failed: one or "
                "more team members is missing from validation\n");

            return false;
        }

        auto zone = state->GetZone();
        for(auto client : clients)
        {
            int32_t worldCID = client->GetClientState()->GetWorldCID();
            if(memberIDs.find(worldCID) == memberIDs.end())
            {
                LogMatchManagerDebug([&]()
                {
                    return libcomp::String("Match entry validation failed:"
                        " invalid team member requested: %1\n")
                        .Arg(client->GetClientState()->GetAccountUID()
                        .ToString());
                });

                return false;
            }

            if(client->GetClientState()->GetZone() != zone)
            {
                LogMatchManagerDebug([&]()
                {
                    return libcomp::String("Match entry validation failed: team"
                        " members must all be in the same zone: %1\n")
                        .Arg(client->GetClientState()->GetAccountUID()
                        .ToString());
                });

                return false;
            }
        }
    }

    for(auto client : clients)
    {
        if(client->GetClientState()->GetPendingMatch())
        {
            LogMatchManagerDebug([&]()
            {
                return libcomp::String("Match entry validation failed: pending"
                    " match exists: %1\n")
                    .Arg(client->GetClientState()->GetAccountUID().ToString());
            });

            return false;
        }

        if(GetMatchEntry(client->GetClientState()->GetWorldCID()))
        {
            LogMatchManagerDebug([&]()
            {
                return libcomp::String("Match entry validation failed: existing"
                    " queue match entry exists: %1\n")
                    .Arg(client->GetClientState()->GetAccountUID().ToString());
            });

            return false;
        }

        if(teamCategory == (int8_t)objects::Team::Category_t::PVP)
        {
            // Create PvPData if needed and check penalty count
            auto pvpData = GetPvPData(client, true);
            if(!pvpData)
            {
                LogMatchManagerError([&]()
                {
                    return libcomp::String("PvP entry validation failed:"
                        " PvPData could not be created: %1\n")
                        .Arg(client->GetClientState()->GetAccountUID()
                        .ToString());
                });

                return false;
            }

            if(checkPenalties && pvpData->GetPenaltyCount() >= 3)
            {
                LogMatchManagerDebug([&]()
                {
                    return libcomp::String("PvP entry validation failed: too"
                        " many penalties exist: %1\n")
                        .Arg(client->GetClientState()->GetAccountUID()
                        .ToString());
                });

                return false;
            }
        }
    }

    return true;
}

bool MatchManager::EndUltimateBattlePhase(const std::shared_ptr<Zone>& zone,
    bool matchOver)
{
    auto ubMatch = zone->GetUBMatch();
    if(!ubMatch)
    {
        return false;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();

    auto tournament = GetUBTournament();
    std::list<std::shared_ptr<objects::UBResult>> updatedResults;

    auto clients = zone->GetConnectionList();
    std::list<std::shared_ptr<ChannelClientConnection>> players;
    for(auto client : clients)
    {
        auto state = client->GetClientState();
        auto cState = state->GetCharacterState();

        if(!ubMatch->MemberIDsContains(state->GetWorldCID())) continue;

        players.push_back(client);

        int32_t points = ubMatch->GetPoints(state->GetWorldCID());
        int32_t coins = ubMatch->GetCoins(state->GetWorldCID());
        if(tournament && matchOver)
        {
            // Update UBResult objects
            auto current = LoadUltimateBattleData(client, 0x03, true);
            auto allTime = state->GetUltimateBattleData(1).Get();
            if(current && allTime)
            {
                for(auto result : { current, allTime })
                {
                    result->SetPoints(result->GetPoints() + (uint32_t)points);
                    if(result->GetTopPoints() < (uint32_t)points)
                    {
                        result->SetTopPoints((uint32_t)points);
                    }

                    result->SetMatches(result->GetMatches() + 1);

                    updatedResults.push_back(result);
                }
            }
            else
            {
                LogMatchManagerError([&]()
                {
                    return libcomp::String("Failed to load Ultimate Battle data"
                        " for character when ending match: %1\n")
                        .Arg(cState->GetEntityUUID().ToString());
                });
            }
        }

        // Always increase coins, no tournament needed either
        if(points > coins && coins < (int32_t)ubMatch->GetCoinLimit())
        {
            int32_t delta = points - coins;
            if((delta + coins) >(int32_t)ubMatch->GetCoinLimit())
            {
                delta = (int32_t)ubMatch->GetCoinLimit() - coins;
            }

            characterManager->UpdateCoinTotal(client, delta, true);

            ubMatch->SetCoins(state->GetWorldCID(), coins + delta);
        }
    }

    players.sort([ubMatch](
        const std::shared_ptr<ChannelClientConnection>& a,
        const std::shared_ptr<ChannelClientConnection>& b)
        {
            return ubMatch->GetPoints(a->GetClientState()->GetWorldCID()) >
                ubMatch->GetPoints(b->GetClientState()->GetWorldCID());
        });

    uint32_t phaseBoss = ubMatch->GetPhaseBoss();

    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_UB_RESULT);
    notify.WriteU32Little(ubMatch->GetSubType());
    notify.WriteS32Little((int32_t)ubMatch->GetPhase());
    notify.WriteS32Little(ubMatch->GetResult());
    notify.WriteU32Little(phaseBoss ? phaseBoss : static_cast<uint32_t>(-1));

    notify.WriteS32Little((int32_t)players.size());

    int8_t rank = 0;
    int32_t lastPoints = -1;
    for(auto client : players)
    {
        auto state = client->GetClientState();
        auto cState = state->GetCharacterState();

        uint32_t points = (uint32_t)ubMatch->GetPoints(state->GetWorldCID());
        if(lastPoints == -1 || (uint32_t)lastPoints > points)
        {
            // Players can tie
            rank = (int8_t)(rank + 1);
            lastPoints = (int32_t)points;
        }

        notify.WriteS8(rank);
        notify.WriteS32Little(cState->GetEntityID());
        notify.WriteS8(cState->GetLevel());
        notify.WriteU32Little(points);
        notify.WriteS8(0);  // Unknown
        notify.WriteU32Little((uint32_t)ubMatch->GetCoins(state
            ->GetWorldCID()));
    }

    ChannelClientConnection::BroadcastPacket(clients, notify);

    if(updatedResults.size() > 0)
    {
        // Sync all results with the world
        auto syncManager = server->GetChannelSyncManager();

        auto dbChanges = libcomp::DatabaseChangeSet::Create();

        for(auto update : updatedResults)
        {
            dbChanges->Update(update);
        }

        server->GetWorldDatabase()->ProcessChangeSet(dbChanges);

        for(auto update : updatedResults)
        {
            syncManager->UpdateRecord(update, "UBResult");
        }

        syncManager->SyncOutgoing();
    }

    ubMatch->SetPhaseBoss(0);

    return true;
}

void MatchManager::EndUltimateBattle(const std::shared_ptr<Zone>& zone)
{
    auto ubMatch = zone->GetUBMatch();
    if(ubMatch)
    {
        // Match over
        LogMatchManagerDebugMsg("Ending Ultimate Battle\n");

        if(ubMatch->GetPhase() < UB_PHASE_MAX && ubMatch->GetResult() == 0)
        {
            ubMatch->SetResult(1);  // Generic failure
        }

        // Update points
        EndUltimateBattlePhase(zone, true);

        ubMatch->SetState(objects::UBMatch::State_t::COMPLETE);
        ubMatch->SetPhase(0);
        ubMatch->SetNextTick(0);
        ubMatch->SetPreviousTick(0);

        SendPhase(zone);

        // Fire special "match over" phase
        FirePhaseTriggers(zone, -2);

        StartStopMatch(zone, nullptr);
    }
}

void MatchManager::QueueNextBaseBonus(int32_t baseID,
    const std::shared_ptr<Zone>& zone, uint64_t occupyStartTime)
{
    auto instance = zone->GetInstance();
    if(PvPActive(instance))
    {
        int nextBonus = (int)instance->GetVariant()->GetTimePoints(2);
        if(nextBonus <= 0)
        {
            nextBonus = 30;
        }

        mServer.lock()->GetTimerManager()->ScheduleEventIn(nextBonus, []
            (MatchManager* pMatchManager, int32_t pBaseID, uint32_t pZoneID,
                uint32_t pInstanceID, uint64_t pOccupyStartTime)
            {
                pMatchManager->IncreaseBaseBonus(pBaseID, pZoneID, pInstanceID,
                    pOccupyStartTime);
            }, this, baseID, zone->GetID(), instance->GetID(),
                occupyStartTime);
    }
}

void MatchManager::FirePhaseTriggers(const std::shared_ptr<Zone>& zone,
    int8_t phase)
{
    auto server = mServer.lock();
    auto actionManager = server->GetActionManager();
    auto zoneManager = server->GetZoneManager();

    auto instance = zone->GetInstance();

    std::list<std::shared_ptr<Zone>> zones = { zone };
    if(instance)
    {
        zones = instance->GetZones();
    }

    // Fire phase trigger once per zone
    for(auto z : zones)
    {
        for(auto trigger : zoneManager->GetZoneTriggers(z,
            ZoneTrigger_t::ON_PHASE))
        {
            if(trigger->GetValue() == (int32_t)phase)
            {
                actionManager->PerformActions(nullptr, trigger->GetActions(),
                    0, zone);
            }
        }
    }
}

void MatchManager::SendPhase(
    const std::shared_ptr<Zone>& zone, bool timerStart,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto match = zone->GetMatch();
    auto ubMatch = std::dynamic_pointer_cast<objects::UBMatch>(match);
    if(!match)
    {
        return;
    }

    std::list<std::shared_ptr<ChannelClientConnection>> clients;
    if(client)
    {
        clients.push_back(client);
    }
    else
    {
        auto instance = !ubMatch ? zone->GetInstance() : nullptr;
        if(instance)
        {
            clients = instance->GetConnections();
        }
        else
        {
            clients = zone->GetConnectionList();
        }
    }

    switch(match->GetType())
    {
    case objects::Match::Type_t::DIASPORA:
        {
            libcomp::Packet p;
            p.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_DIASPORA_PHASE);
            p.WriteS32Little((int32_t)match->GetPhase());

            ChannelClientConnection::BroadcastPacket(clients, p);
        }
        break;
    case objects::Match::Type_t::ULTIMATE_BATTLE:
        {
            uint64_t now = ChannelServer::GetServerTime();
            uint64_t stop = ubMatch->GetTimerExpire();

            float timeLeft = 0.f;
            if(timerStart)
            {
                // Send the full time since the client rounds down
                uint64_t start = ubMatch->GetTimerStart();
                timeLeft = (start < stop)
                    ? (float)((double)(stop - start) / 1000000.0) : 0.f;
            }
            else
            {
                timeLeft = (now < stop)
                    ? (float)((double)(stop - now) / 1000000.0) : 0.f;
            }

            float timeSinceReady = 0.f;

            int32_t timerStyle = 0;
            switch(ubMatch->GetState())
            {
            case objects::UBMatch::State_t::READY:
                timeSinceReady = timeLeft == 0.f ? 0.f
                    : (float)((double)(stop - now) / 1000000.0) - timeLeft;
                timerStyle = -1;
                break;
            case objects::UBMatch::State_t::COMPLETE:
                timerStyle = -1;
                break;
            case objects::UBMatch::State_t::PREROUND:
                timerStyle = 1;
                break;
            default:
                break;
            }

            libcomp::Packet p;
            p.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_UB_PHASE);
            p.WriteU32Little(ubMatch->GetSubType());
            p.WriteS32Little((int32_t)ubMatch->GetState());
            p.WriteS32Little((int32_t)ubMatch->GetPhase());
            p.WriteS32Little(timerStyle);
            p.WriteFloat(timeSinceReady);
            p.WriteFloat(timeLeft);

            ChannelClientConnection::BroadcastPacket(clients, p);
        }
        break;
    default:
        break;
    }
}

void MatchManager::SendUltimateBattleMembers(const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    if(!ubMatch)
    {
        return;
    }

    bool spectating = false;
    if(client)
    {
        auto state = client->GetClientState();
        spectating = !ubMatch->MemberIDsContains(state->GetWorldCID());
    }

    auto playerClients = mServer.lock()->GetManagerConnection()
        ->GetEntityClients(ubMatch->GetMemberIDs(), true);

    playerClients.remove_if([zone](
        const std::shared_ptr<ChannelClientConnection>& c)
        {
            return c->GetClientState()->GetZone() != zone;
        });

    libcomp::Packet notify;
    notify.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_UB_MEMBERS);
    notify.WriteU32Little(ubMatch->GetSubType());
    notify.WriteS32Little((int32_t)playerClients.size());

    for(auto c : playerClients)
    {
        auto cState = c->GetClientState()->GetCharacterState();
        auto character = cState->GetEntity();

        notify.WriteS32Little(cState->GetEntityID());
        notify.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            character ? character->GetName() : "", true);
    }

    if(spectating)
    {
        // Just send to new spectator
        client->QueuePacket(notify);
    }
    else
    {
        // Send to full zone
        mServer.lock()->GetZoneManager()->BroadcastPacket(zone, notify);
    }
}

void MatchManager::SendUltimateBattleMemberState(
    const std::shared_ptr<Zone>& zone,
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
    if(!ubMatch)
    {
        return;
    }

    auto server = mServer.lock();
    auto playerClients = server->GetManagerConnection()->GetEntityClients(
        ubMatch->GetMemberIDs(), true);

    playerClients.remove_if([zone](
        const std::shared_ptr<ChannelClientConnection>& c)
        {
            return c->GetClientState()->GetZone() != zone;
        });

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_UB_MEMBER_STATE);
    p.WriteU32Little(ubMatch->GetSubType());
    p.WriteS32Little((int32_t)playerClients.size());

    for(auto c : playerClients)
    {
        auto s = c->GetClientState();
        auto cState = s->GetCharacterState();
        auto dState = s->GetDemonState();
        for(int32_t entityID : { cState->GetEntityID(), dState->GetEntityID()})
        {
            auto eState = s->GetEntityState(entityID);
            if(eState)
            {
                auto cs = eState->GetCoreStats();

                p.WriteS32Little(entityID);
                p.WriteS8(eState->GetLevel());
                p.WriteS32Little(cs ? cs->GetHP() : 0);
                p.WriteS32Little(eState->GetMaxHP());
                p.WriteS32Little(cs ? cs->GetMP() : 0);
                p.WriteS32Little(eState->GetMaxMP());

                auto statusEffects = eState->GetCurrentStatusEffectStates();

                p.WriteS32Little((int32_t)statusEffects.size());
                for(auto ePair : statusEffects)
                {
                    p.WriteU32Little(ePair.first->GetEffect());
                    p.WriteS32Little((int32_t)ePair.second);
                    p.WriteU8(ePair.first->GetStack());
                }
            }
            else
            {
                p.WriteS32Little(-1);
                p.WriteBlank(21);
            }
        }

        auto character = cState->GetEntity();
        auto demon = dState->GetEntity();
        p.WriteS32Little(character ? character->GetLNC() : 0);
        p.WriteU32Little(demon ? demon->GetType() : 0);
    }

    if(client)
    {
        client->SendPacket(p);
    }
    else
    {
        server->GetZoneManager()->BroadcastPacket(zone, p);
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

bool MatchManager::IsPvPMatchEntry(const std::shared_ptr<
    objects::MatchEntry>& entry)
{
    return entry->GetMatchType() ==
        objects::MatchEntry::MatchType_t::PVP_FATE ||
        entry->GetMatchType() ==
        objects::MatchEntry::MatchType_t::PVP_VALHALLA;
}
