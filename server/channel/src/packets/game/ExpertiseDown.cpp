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
#include <DefinitionManager.h>
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
#include "CharacterManager.h"
#include "SkillManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

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
        LOG_ERROR("Invalid activation ID encountered for ExpertiseDown request\n");
    }
    else
    {
        bool failure = false;

        auto exp = character->GetExpertises((size_t)expertiseID).Get();
        auto skillData = definitionManager->GetSkillData(activatedAbility->GetSkillID());
        if(exp && skillData)
        {
            int32_t points = exp->GetPoints();

            // Always remove the progress to next rank
            int32_t remove = points % 10000;

            uint16_t functionID = skillData->GetDamage()->GetFunctionID();
            if (functionID == SVR_CONST.SKILL_EXPERT_CLASS_DOWN)
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
            else
            {
                // Shouldn't happen
                remove = 0;
            }

            if(remove > 0)
            {
                skillManager->ExecuteSkill(sourceState, activationID,
                    activatedAbility->GetTargetObjectID());

                std::list<std::pair<uint8_t, int32_t>> pointMap;
                pointMap.push_back(std::make_pair((uint8_t)expertiseID, -remove));

                server->GetCharacterManager()->UpdateExpertisePoints(client,
                    pointMap, true);
            }
            else
            {
                failure = true;
            }
        }
        else
        {
            failure = true;
        }

        if(failure)
        {
            skillManager->CancelSkill(sourceState, activationID);
        }
    }

    return true;
}
