/**
 * @file server/channel/src/packets/game/DemonQuestActive.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for information about the active demon quest.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

 // object Includes
#include <Demon.h>
#include <DemonBox.h>
#include <DemonQuest.h>

 // channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::DemonQuestActive::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto dQuest = character->GetDemonQuest().Get();

    auto demon = dQuest ? std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(dQuest->GetDemon()))
        : nullptr;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_QUEST_ACTIVE);
    reply.WriteS8(0);   // Success

    bool started = demon != nullptr;
    reply.WriteS8(started ? 1 : 0);
    if(started)
    {
        reply.WriteS8((int8_t)dQuest->GetType());

        reply.WriteS8((int8_t)dQuest->TargetsCount());
        for(auto& pair : dQuest->GetTargets())
        {
            reply.WriteU32Little(pair.first);
            reply.WriteS32Little(pair.second);
            reply.WriteS32Little(dQuest->GetTargetCurrentCounts(pair.first));
        }

        reply.WriteS8((int8_t)dQuest->RewardItemsCount());
        for(auto& pair : dQuest->GetRewardItems())
        {
            reply.WriteU32Little(pair.first);
            reply.WriteU16Little(pair.second);
        }

        reply.WriteS32Little(dQuest->GetXPReward());

        int8_t bonusCount = (int8_t)(dQuest->BonusItemsCount() +
            dQuest->BonusTitlesCount() + dQuest->BonusXPCount());
        reply.WriteS8(bonusCount);

        for(auto& pair : dQuest->GetBonusItems())
        {
            reply.WriteS8(0);
            reply.WriteU32Little(pair.first);
            reply.WriteU16Little(pair.second);
        }

        for(uint32_t xp : dQuest->GetBonusXP())
        {
            reply.WriteS8(1);
            reply.WriteU32Little(0);
            reply.WriteU16Little((uint16_t)xp); // Known client display issue
        }

        for(uint16_t title : dQuest->GetBonusTitles())
        {
            reply.WriteS8(4);
            reply.WriteU32Little(title);
            reply.WriteU16Little(1);
        }

        reply.WriteU32Little(0);   // Unknown
        reply.WriteU32Little(0);   // Unknown
    }

    client->SendPacket(reply);

    return true;
}
