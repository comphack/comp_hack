/**
 * @file server/channel/src/packets/game/SkillForget.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to forget a character skill.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <ActivatedAbility.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::SkillForget::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 9)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();
    int8_t activationID = p.ReadS8();
    uint32_t skillID = p.ReadU32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(cState->GetEntityID() != entityID)
    {
        LOG_ERROR("Player attempted to forget a skill for a character that does"
            " not belong to the client\n");
        state->SetLogoutSave(true);
        client->Close();
        return true;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto definitionManager = server->GetDefinitionManager();
    auto skillManager = server->GetSkillManager();

    auto activatedAbility = cState->GetSpecialActivations(activationID);
    if(!activatedAbility)
    {
        LOG_ERROR("Invalid activation ID encountered for SkillForget request\n");
    }
    else
    {
        skillManager->ExecuteSkill(cState, activationID,
            activatedAbility->GetTargetObjectID());

        character->RemoveLearnedSkills(skillID);

        cState->RecalcDisabledSkills(definitionManager);
        server->GetTokuseiManager()->Recalculate(cState, true,
            std::set<int32_t>{ cState->GetEntityID() });
        server->GetCharacterManager()->RecalculateStats(cState, client);

        server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());
    }

    return true;
}
