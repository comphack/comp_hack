/**
 * @file server/channel/src/packets/game/AllocateSkillPoint.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to allocate a skill point for a character.
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
#include <Constants.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <math.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"

using namespace channel;

void AllocatePoint(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int8_t correctStatOffset)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto stats = character->GetCoreStats().Get();

    // Process these changes in an operational changeset to ensure no points
    // are lost due to a crash etc
    auto opChangeset = std::make_shared<libcomp::DBOperationalChangeSet>();
    auto expl = std::make_shared<libcomp::DBExplicitUpdate>(
        stats);

    int32_t points = character->GetPoints();

    int16_t currentStat = 0;
    switch((CorrectTbl)correctStatOffset)
    {
        case CorrectTbl::STR:
            currentStat = stats->GetSTR();
            expl->AddFrom<int32_t>("STR", 1, currentStat);
            break;
        case CorrectTbl::MAGIC:
            currentStat = stats->GetMAGIC();
            expl->AddFrom<int32_t>("MAGIC", 1, currentStat);
            break;
        case CorrectTbl::VIT:
            currentStat = stats->GetVIT();
            expl->AddFrom<int32_t>("VIT", 1, currentStat);
            break;
        case CorrectTbl::INT:
            currentStat = stats->GetINTEL();
            expl->AddFrom<int32_t>("INTEL", 1, currentStat);
            break;
        case CorrectTbl::SPEED:
            currentStat = stats->GetSPEED();
            expl->AddFrom<int32_t>("SPEED", 1, currentStat);
            break;
        case CorrectTbl::LUCK:
            currentStat = stats->GetLUCK();
            expl->AddFrom<int32_t>("LUCK", 1, currentStat);
            break;
        default:
            return;
    }

    opChangeset->AddOperation(expl);

    int32_t pointCost = (int32_t)floor((currentStat + 1) / 10) + 1;
    if(points < pointCost)
    {
        LOG_ERROR(libcomp::String("AllocateSkillPoint attempted with an"
            " insufficient amount of stat points available: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        client->Kill();
        return;
    }

    expl = std::make_shared<libcomp::DBExplicitUpdate>(character);
    expl->SubtractFrom<int32_t>("Points", pointCost, points);
    opChangeset->AddOperation(expl);

    if(!server->GetWorldDatabase()->ProcessChangeSet(opChangeset))
    {
        LOG_ERROR(libcomp::String("AllocateSkillPoint failed to process"
            " operational changeset when updating stats: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        client->Kill();
        return;
    }

    server->GetTokuseiManager()->Recalculate(cState);

    auto characterManager = server->GetCharacterManager();
    characterManager->RecalculateStats(cState, client, false);

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_ALLOCATE_SKILL_POINT);
    reply.WriteS32Little(cState->GetEntityID());
    characterManager->GetEntityStatsPacketData(reply, stats, cState, 1);
    reply.WriteS32Little(pointCost);

    client->SendPacket(reply);
}

bool Parsers::AllocateSkillPoint::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 5)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();
    int8_t correctStatOffset = p.ReadS8();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    if(cState->GetEntityID() != entityID)
    {
        return false;
    }

    server->QueueWork(AllocatePoint, server, client, correctStatOffset);

    return true;
}
