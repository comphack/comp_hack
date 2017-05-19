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
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <math.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

int32_t GetPointCost(int16_t val)
{
    return (int32_t)floor((val + 1) / 10) + 1;
}

void AllocatePoint(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int8_t correctStatOffset)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto stats = character->GetCoreStats().Get();

    int32_t pointCost = 0;
    switch((libcomp::CorrectData)correctStatOffset)
    {
        case libcomp::CorrectData::CORRECT_STR:
            pointCost = GetPointCost(stats->GetSTR());
            stats->SetSTR(static_cast<int16_t>(stats->GetSTR() + 1));
            break;
        case libcomp::CorrectData::CORRECT_MAGIC:
            pointCost = GetPointCost(stats->GetMAGIC());
            stats->SetMAGIC(static_cast<int16_t>(stats->GetMAGIC() + 1));
            break;
        case libcomp::CorrectData::CORRECT_VIT:
            pointCost = GetPointCost(stats->GetVIT());
            stats->SetVIT(static_cast<int16_t>(stats->GetVIT() + 1));
            break;
        case libcomp::CorrectData::CORRECT_INTEL:
            pointCost = GetPointCost(stats->GetINTEL());
            stats->SetINTEL(static_cast<int16_t>(stats->GetINTEL() + 1));
            break;
        case libcomp::CorrectData::CORRECT_SPEED:
            pointCost = GetPointCost(stats->GetSPEED());
            stats->SetSPEED(static_cast<int16_t>(stats->GetSPEED() + 1));
            break;
        case libcomp::CorrectData::CORRECT_LUCK:
            pointCost = GetPointCost(stats->GetLUCK());
            stats->SetLUCK(static_cast<int16_t>(stats->GetLUCK() + 1));
            break;
        default:
            return;
    }

    character->SetPoints(static_cast<int32_t>(character->GetPoints() - pointCost));

    cState->RecalculateStats(server->GetDefinitionManager());

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ALLOCATE_SKILL_POINT);
    reply.WriteS32Little(cState->GetEntityID());
    server->GetCharacterManager()->GetEntityStatsPacketData(reply, stats, cState, true);
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
