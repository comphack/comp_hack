/**
 * @file server/channel/src/packets/game/ExpertiseDown.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to lower a specific expertise rank or
 *  class by 1.
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
#include <ServerConstants.h>

// object Includes
#include <ActivatedAbility.h>
#include <Expertise.h>
#include <MiDamageData.h>
#include <MiSkillData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ExpertiseDown::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 9)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();
    int8_t activationID = p.ReadS8();
    int32_t expertiseID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto sourceState = state->GetEntityState(entityID);
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(sourceState == nullptr)
    {
        LOG_ERROR("Player attempted to lower expertise from an entity that does"
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
        auto exp = character->GetExpertises((size_t)expertiseID).Get();
        auto skillData = definitionManager->GetSkillData(activatedAbility->GetSkillID());
        if(exp && skillData)
        {
            int32_t points = exp->GetPoints();

            // Always remove the progress to next rank
            int32_t remove = points % 10000;

            uint16_t functionID = skillData->GetDamage()->GetFunctionID();
            if(functionID == SVR_CONST.SKILL_EXPERT_CLASS_DOWN)
            {
                // Remove one class
                if(points >= 100000)
                {
                    remove = remove + 100000;
                }
            }
            else if(functionID == SVR_CONST.SKILL_EXPERT_RANK_DOWN)
            {
                // Remove one rank
                if(points >= 10000)
                {
                    remove = remove + 10000;
                }
            }

            bool success = remove > 0;
            if(success)
            {
                points = points - remove;

                skillManager->ExecuteSkill(sourceState, activationID,
                    activatedAbility->GetTargetObjectID());

                exp->SetPoints(points);
            }

            libcomp::Packet notify;
            notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EXPERTISE_DOWN);
            notify.WriteS32Little(cState->GetEntityID());
            notify.WriteS8(success ? 1 : 0);
            notify.WriteS8((int8_t)expertiseID);
            notify.WriteS32Little(points);

            if(success)
            {
                server->GetZoneManager()->BroadcastPacket(client, notify);

                cState->RecalcDisabledSkills(definitionManager);
                server->GetTokuseiManager()->Recalculate(cState, true,
                    std::set<int32_t>{ cState->GetEntityID() });
                server->GetCharacterManager()->RecalculateStats(cState, client);

                server->GetWorldDatabase()->QueueUpdate(exp);
            }
            else
            {
                client->SendPacket(notify);
            }
        }
        else
        {
            skillManager->CancelSkill(sourceState, activationID);
        }
    }

    return true;
}
