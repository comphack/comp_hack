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

    int32_t points = character->GetPoints();

    int16_t currentStat = 0;
    switch((CorrectTbl)correctStatOffset)
    {
        case CorrectTbl::STR:
            currentStat = stats->GetSTR();
            break;
        case CorrectTbl::MAGIC:
            currentStat = stats->GetMAGIC();
            break;
        case CorrectTbl::VIT:
            currentStat = stats->GetVIT();
            break;
        case CorrectTbl::INT:
            currentStat = stats->GetINTEL();
            break;
        case CorrectTbl::SPEED:
            currentStat = stats->GetSPEED();
            break;
        case CorrectTbl::LUCK:
            currentStat = stats->GetLUCK();
            break;
        default:
            return;
    }

    int32_t pointCost = (int32_t)floor((currentStat + 1) / 10) + 1;
    if(points < pointCost)
    {
        LOG_ERROR(libcomp::String("AllocateSkillPoint attempted with an"
            " insufficient amount of stat points available: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        client->Close();
        return;
    }
    
    switch((CorrectTbl)correctStatOffset)
    {
        case CorrectTbl::STR:
            stats->SetSTR(static_cast<int16_t>(stats->GetSTR() + 1));
            break;
        case CorrectTbl::MAGIC:
            stats->SetMAGIC(static_cast<int16_t>(stats->GetMAGIC() + 1));
            break;
        case CorrectTbl::VIT:
            stats->SetVIT(static_cast<int16_t>(stats->GetVIT() + 1));
            break;
        case CorrectTbl::INT:
            stats->SetINTEL(static_cast<int16_t>(stats->GetINTEL() + 1));
            break;
        case CorrectTbl::SPEED:
            stats->SetSPEED(static_cast<int16_t>(stats->GetSPEED() + 1));
            break;
        case CorrectTbl::LUCK:
            stats->SetLUCK(static_cast<int16_t>(stats->GetLUCK() + 1));
            break;
        default:
            break;
    }

    character->SetPoints(static_cast<int32_t>(points - pointCost));

    server->GetTokuseiManager()->Recalculate(cState);

    auto characterManager = server->GetCharacterManager();
    characterManager->RecalculateStats(cState, client, false);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ALLOCATE_SKILL_POINT);
    reply.WriteS32Little(cState->GetEntityID());
    characterManager->GetEntityStatsPacketData(reply, stats, cState, 1);
    reply.WriteS32Little(pointCost);

    client->SendPacket(reply);

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
    dbChanges->Update(character);
    dbChanges->Update(stats);
    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
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

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    if(cState->GetEntityID() != entityID)
    {
        return false;
    }

    server->QueueWork(AllocatePoint, server, client, correctStatOffset);

    return true;
}
