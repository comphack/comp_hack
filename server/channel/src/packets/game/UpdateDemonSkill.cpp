/**
 * @file server/channel/src/packets/game/DemonSkillUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update the partner demon's
 *  learned skill set.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

const int8_t ACTION_LEARN = 0;
const int8_t ACTION_UNKNOWN = 1;
const int8_t ACTION_MOVE = 2;

using namespace channel;

void DemonSkillUpdate(const std::shared_ptr<ChannelClientConnection> client,
    int32_t entityID, int8_t skillSlot, uint32_t skillID)
{
    auto state = client->GetClientState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(dState->GetEntityID() != entityID || nullptr == demon)
    {
        return;
    }

    int8_t oldSlot = -1;
    for(size_t i = 0; i < 8; i++)
    {
        if(demon->GetLearnedSkills(i) == skillID)
        {
            oldSlot = (int8_t)i;
            break;
        }
    }

    int8_t action = ACTION_LEARN;
    if(oldSlot != -1)
    {
        action = ACTION_MOVE;
    }
    /*else if(false)
    {
        action = ACTION_UNKNOWN;
    }*/

    uint32_t currentSkillID = demon->GetLearnedSkills((size_t)skillSlot);
    demon->SetLearnedSkills((size_t)skillSlot, skillID);
    if(action == ACTION_LEARN)
    {
        // Remove from acquired skills if it exists
        size_t skillCount = demon->AcquiredSkillsCount();
        for(size_t i = skillCount; i > 0; i--)
        {
            size_t idx = (size_t)(i - 1);
            if(demon->GetAcquiredSkills(idx) == skillID)
            {
                demon->RemoveAcquiredSkills(idx);
            }
        }
    }
    else if(action == ACTION_MOVE)
    {
        demon->SetLearnedSkills((size_t)oldSlot, currentSkillID);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_UPDATE_DEMON_SKILL);
    reply.WriteS32Little(entityID);
    reply.WriteS8(action);
    reply.WriteS8(skillSlot);
    reply.WriteU32Little(skillID);

    if(action == ACTION_MOVE)
    {
        reply.WriteS8(oldSlot);
        reply.WriteU32Little(currentSkillID);
    }
    else
    {
        /// @todo: figure out if these are actually the default values
        reply.WriteS8(0);
        reply.WriteU32Little(6);
    }

    client->SendPacket(reply);
}

bool Parsers::UpdateDemonSkill::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 9)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    int32_t entityID = p.ReadS32Little();
    int8_t skillSlot = p.ReadS8();
    uint32_t skillID = p.ReadU32Little();

    if(skillSlot < 0 || skillSlot >= 8 || entityID <= 0)
    {
        return false;
    }

    auto skillDef = server->GetDefinitionManager()->GetSkillData(skillID);
    if(nullptr == skillDef)
    {
        LOG_ERROR(libcomp::String("Invalid skill ID encountered when attempting to"
            " to update a demon's skills: %1\n").Arg(skillID));
        return false;
    }

    server->QueueWork(DemonSkillUpdate, client, entityID, skillSlot, skillID);

    return true;
}
