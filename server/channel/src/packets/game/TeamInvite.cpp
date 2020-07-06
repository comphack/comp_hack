/**
 * @file server/channel/src/packets/game/TeamInvite.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to invite a player to join the player's team.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "Packets.h"

// libcomp Includes
#include <ErrorCodes.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

 // objects Includes
#include <CharacterProgress.h>
#include <PvPData.h>
#include <Team.h>

 // channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ManagerConnection.h"
#include "MatchManager.h"

using namespace channel;

bool Parsers::TeamInvite::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 6)
    {
        return false;
    }

    int32_t teamID = p.ReadS32Little();

    if(p.Left() != (uint32_t)(2 + p.PeekU16Little()))
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();

    libcomp::String targetName = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto worldDB = server->GetWorldDatabase();

    auto team = state->GetTeam();
    auto zone = state->GetZone();

    std::shared_ptr<ChannelClientConnection> targetClient;
    ClientState* targetState = 0;
    std::shared_ptr<CharacterState> targetCState;
    if(zone)
    {
        for(auto otherClient : zone->GetConnectionList())
        {
            auto otherState = otherClient->GetClientState();
            auto cState = otherState->GetCharacterState();
            if(cState->GetEntity()->GetName().ToLower() == targetName.ToLower())
            {
                targetClient = otherClient;
                targetState = otherState;
                targetCState = cState;
                break;
            }
        }
    }

    int8_t errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;
    int8_t targetError = (int8_t)TeamErrorCodes_t::SUCCESS;
    if(!team || teamID != team->GetID())
    {
        errorCode = (int8_t)TeamErrorCodes_t::INVALID_TEAM;
    }
    else if(!targetCState)
    {
        errorCode = (int8_t)TeamErrorCodes_t::INVALID_TARGET;
    }
    else if(!targetCState->IsAlive())
    {
        targetError = (int8_t)TeamErrorCodes_t::INVALID_TARGET_STATE;
    }
    else
    {
        errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
    }

    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        // Make sure the target has the required valuables too
        bool hasValuables = true;
        auto it = SVR_CONST.TEAM_VALUABLES.find(team->GetType());
        if(it != SVR_CONST.TEAM_VALUABLES.end())
        {
            for(uint16_t valuableID : it->second)
            {
                hasValuables &= characterManager->HasValuable(
                    targetCState->GetEntity(), valuableID);
            }
        }

        if(!hasValuables &&
            team->GetCategory() != objects::Team::Category_t::CATHEDRAL)
        {
            targetError = (int8_t)TeamErrorCodes_t::TARGET_VALUABLE_MISSING;
        }
        else if(targetState->GetParty())
        {
            errorCode = (int8_t)TeamErrorCodes_t::TARGET_IN_PARTY;
        }
        else if(team->GetCategory() == objects::Team::Category_t::PVP)
        {
            auto pvpData = targetCState->GetEntity()->GetPvPData();
            auto matchManager = server->GetMatchManager();
            if(pvpData && pvpData->GetPenaltyCount() >= 3)
            {
                errorCode = (int8_t)TeamErrorCodes_t::PENALTY_ACTIVE_REJECT;
            }
            else if(matchManager->GetMatchEntry(targetState->GetWorldCID()))
            {
                errorCode = (int8_t)TeamErrorCodes_t::AWAITING_ENTRY_REJECT;
            }
            else if(targetState->GetPendingMatch())
            {
                errorCode = (int8_t)TeamErrorCodes_t::MATCH_ACTIVE_REJECT;
            }
        }
        else
        {
            auto it2 = SVR_CONST.TEAM_STATUS_COOLDOWN.find(team->GetType());
            if(it2 != SVR_CONST.TEAM_STATUS_COOLDOWN.end() &&
                targetCState->StatusEffectActive(it2->second))
            {
                targetError = (int8_t)TeamErrorCodes_t::TARGET_COOLDOWN_20H;
            }
        }
    }

    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS &&
        targetError == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_TEAM_UPDATE);
        request.WriteU8((int8_t)
            InternalPacketAction_t::PACKET_ACTION_YN_REQUEST);
        request.WriteS32Little(teamID);
        request.WriteS32Little(state->GetWorldCID());
        request.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, targetName, true);

        server->GetManagerConnection()->GetWorldConnection()
            ->SendPacket(request);
    }
    else
    {
        bool relayError = false;
        if(targetError != (int8_t)TeamErrorCodes_t::GENERIC_ERROR)
        {
            // Simulate response for things we can check here
            errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
            relayError = true;
        }

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_INVITE);
        reply.WriteS32Little(teamID);
        reply.WriteS8(errorCode);

        client->QueuePacket(reply);

        if(relayError)
        {
            reply.Clear();
            reply.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TEAM_ANSWERED);
            reply.WriteS32Little(teamID);
            reply.WriteS8(targetError);
            reply.WriteString16Little(state->GetClientStringEncoding(),
                targetName, true);
            reply.WriteS8(team ? team->GetType() : -1);

            client->QueuePacket(reply);
        }

        client->FlushOutgoing();
    }

    return true;
}
