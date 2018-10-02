/**
 * @file server/channel/src/packets/game/TeamForm.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to form a new team.
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

#include "Packets.h"

// libcomp Includes
#include <ErrorCodes.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>
#include <ServerZone.h>

// object Includes
#include <PvPData.h>
#include <Team.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "MatchManager.h"

using namespace channel;

bool Parsers::TeamForm::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 6)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();

    int8_t type = p.ReadS8();
    int8_t unk2 = p.ReadS8();
    int8_t unk3 = p.ReadS8();
    int8_t unk4 = p.ReadS8();

    if(p.Left() < (uint32_t)(2 + p.PeekU16Little()))
    {
        return false;
    }

    libcomp::String unk5 = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    if(unk2 || unk3 || unk4 || !unk5.IsEmpty())
    {
        // All other parameters seem to never be specified, debug
        // if they are not
        LOG_DEBUG(libcomp::String("TeamForm 2: %1\n").Arg(unk2));
        LOG_DEBUG(libcomp::String("TeamForm 3: %1\n").Arg(unk3));
        LOG_DEBUG(libcomp::String("TeamForm 4: %1\n").Arg(unk4));
        LOG_DEBUG(libcomp::String("TeamForm 5: %1\n").Arg(unk5));
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto zone = state->GetZone();

    int8_t errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;
    if(type >= (int8_t)objects::Team::Category_t::CATHEDRAL)
    {
        // LNC needs to match
        int8_t lncAdjust = (int8_t)(cState->GetLNCType() / 2);
        if(lncAdjust ==
            (int8_t)(type - (int8_t)objects::Team::Category_t::CATHEDRAL))
        {
            errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
        }
    }
    else if(type >= (int8_t)objects::Team::Category_t::PVP)
    {
        auto pvpData = character->GetPvPData();
        auto matchManager = server->GetMatchManager();
        if(pvpData && pvpData->GetPenaltyCount() >= 3)
        {
            errorCode = (int8_t)TeamErrorCodes_t::PENALTY_ACTIVE;
        }
        else if(matchManager->GetMatchEntry(state->GetWorldCID()))
        {
            errorCode = (int8_t)TeamErrorCodes_t::AWAITING_ENTRY;
        }
        else if(state->GetPendingMatch())
        {
            errorCode = (int8_t)TeamErrorCodes_t::MATCH_ACTIVE;
        }
        else
        {
            errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
        }
    }
    
    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        // Type verification passed, check valuables and other restrictions
        bool hasValuables = true;
        auto it = SVR_CONST.TEAM_VALUABLES.find(type);
        if(it != SVR_CONST.TEAM_VALUABLES.end())
        {
            for(uint16_t valuableID : it->second)
            {
                hasValuables &= characterManager->HasValuable(character,
                    valuableID);
            }
        }

        if(!hasValuables)
        {
            errorCode = (int8_t)TeamErrorCodes_t::VALUABLE_MISSING;
        }
        else if(!zone || !zone->GetDefinition()->ValidTeamTypesContains(type))
        {
            errorCode = (int8_t)TeamErrorCodes_t::ZONE_INVALID;
        }
        else
        {
            auto it2 = SVR_CONST.TEAM_STATUS_COOLDOWN.find(type);
            if(it2 != SVR_CONST.TEAM_STATUS_COOLDOWN.end() &&
                cState->StatusEffectActive(it2->second))
            {
                errorCode = (int8_t)TeamErrorCodes_t::COOLDOWN_20H;
            }
        }
    }

    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_TEAM_UPDATE);
        request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_ADD);
        request.WriteS32Little(0);
        request.WriteS32Little(state->GetWorldCID());
        request.WriteS8(type);

        server->GetManagerConnection()->GetWorldConnection()
            ->SendPacket(request);
    }
    else
    {
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_FORM);
        reply.WriteS32Little(-1);
        reply.WriteS8(errorCode);
        reply.WriteS8(type);

        client->SendPacket(reply);
    }

    return true;
}
