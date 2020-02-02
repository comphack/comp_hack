/**
 * @file server/channel/src/packets/game/DemonQuestList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the player's demon quest list.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <CharacterProgress.h>
#include <Demon.h>
#include <DemonBox.h>
#include <DemonQuest.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::DemonQuestList::Parse(libcomp::ManagerPacket *pPacketManager,
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
    auto progress = character->GetProgress().Get();
    auto dQuest = character->GetDemonQuest().Get();

    std::list<std::shared_ptr<objects::Demon>> questDemons;
    for(auto demon : character->GetCOMP()->GetDemons())
    {
        if(!demon.IsNull() && demon->GetHasQuest())
        {
            questDemons.push_back(demon.Get());
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_QUEST_LIST);
    reply.WriteS8(0);   // Success

    reply.WriteS8((int8_t)questDemons.size());
    for(auto demon : questDemons)
    {
        reply.WriteS64Little(state->GetObjectID(demon->GetUUID()));
    }
    reply.WriteS64Little(dQuest ? state->GetObjectID(dQuest->GetDemon()) : -1);

    reply.WriteS16Little((int16_t)progress->GetDemonQuestSequence());
    reply.WriteS32Little(0);   // Last completed? Not actually used
    reply.WriteS8(progress->GetDemonQuestDaily());

    client->SendPacket(reply);

    return true;
}
