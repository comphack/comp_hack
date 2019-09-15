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
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <InheritedSkill.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"

const int8_t ACTION_LEARN_ACQUIRED = 0;
const int8_t ACTION_LEARN_INHERITED = 1;
const int8_t ACTION_MOVE = 2;

using namespace channel;

void UpdateDemonSkill(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
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

    int8_t action = ACTION_LEARN_ACQUIRED;
    if(oldSlot != -1)
    {
        action = ACTION_MOVE;
    }

    auto changes = libcomp::DatabaseChangeSet::Create(
        state->GetAccountUID());
    changes->Update(demon);

    bool recalc = false;

    uint32_t currentSkillID = demon->GetLearnedSkills((size_t)skillSlot);
    if(action == ACTION_LEARN_ACQUIRED)
    {
        bool found = false;

        // Remove from acquired skills if it exists
        size_t skillCount = demon->AcquiredSkillsCount();
        for(size_t i = skillCount; i > 0; i--)
        {
            size_t idx = (size_t)(i - 1);
            if(demon->GetAcquiredSkills(idx) == skillID)
            {
                demon->RemoveAcquiredSkills(idx);
                found = true;
            }
        }

        // Remove from inherited skills if it exists and update
        // the action
        size_t iSkillIdx = 0;
        for(auto iSkill : demon->GetInheritedSkills())
        {
            if(!iSkill.IsNull() && iSkill->GetSkill() == skillID)
            {
                action = ACTION_LEARN_INHERITED;
                changes->Delete(iSkill.Get());
                demon->RemoveInheritedSkills(iSkillIdx);
                found = true;
                break;
            }
            iSkillIdx++;
        }

        if(!found)
        {
            LogDemonError([&]()
            {
                return libcomp::String("DemonSkillUpdate request received"
                    " for skill ID '%1' which is not on the demon: %2\n")
                    .Arg(skillID).Arg(state->GetAccountUID().ToString());
            });

            client->Close();
            return;
        }
        else
        {
            demon->SetLearnedSkills((size_t)skillSlot, skillID);
        }

        recalc = true;
    }
    else if(action == ACTION_MOVE)
    {
        demon->SetLearnedSkills((size_t)skillSlot, skillID);
        demon->SetLearnedSkills((size_t)oldSlot, currentSkillID);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_DEMON_SKILL_UPDATE);
    reply.WriteS32Little(entityID);
    reply.WriteS8(action);
    reply.WriteS8(skillSlot);
    reply.WriteU32Little(skillID);

    if(action == ACTION_MOVE)
    {
        reply.WriteS8(oldSlot);
        reply.WriteU32Little(currentSkillID ? currentSkillID : (uint32_t)-1);
    }
    else
    {
        reply.WriteS8(0);
        reply.WriteU32Little(6);
    }

    client->SendPacket(reply);

    server->GetWorldDatabase()->QueueChangeSet(changes);

    if(recalc)
    {
        server->GetTokuseiManager()->Recalculate(state->GetCharacterState(), true,
            std::set<int32_t>{ dState->GetEntityID() });
        server->GetCharacterManager()->RecalculateStats(dState, client);
    }
}

bool Parsers::DemonSkillUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
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
        LogDemonError([&]()
        {
            return libcomp::String("Invalid skill ID encountered when "
                "attempting to to update a demon's skills: %1\n").Arg(skillID);
        });

        return false;
    }

    server->QueueWork(UpdateDemonSkill, server, client, entityID, skillSlot, skillID);

    return true;
}
