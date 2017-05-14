/**
 * @file server/channel/src/packets/game/LearnSkill.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for a character to learn a skill.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

// objects Includes
#include <Character.h>

using namespace channel;

bool Parsers::LearnSkill::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();
    uint32_t skillID = p.ReadU32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto definitionManager = server->GetDefinitionManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    auto def = definitionManager->GetSkillData(skillID);
    if(cState->GetEntityID() != entityID || nullptr == def)
    {
        return false;
    }

    auto character = cState->GetEntity();
    character->AppendLearnedSkills(skillID);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LEARN_SKILL);
    reply.WriteS32Little(entityID);
    reply.WriteU32Little(skillID);

    client->SendPacket(reply);

    server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

    return true;
}
