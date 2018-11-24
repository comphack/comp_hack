/**
 * @file server/channel/src/packets/game/SkillActivate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to activate a character or demon skill.
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
#include <ErrorCodes.h>
#include <Log.h>
#include <ManagerPacket.h>

// object Includes
#include <Demon.h>
#include <Item.h>

// channel Includes
#include "ChannelServer.h"
#include "SkillManager.h"

using namespace channel;

bool Parsers::SkillActivate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 12)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto skillManager = server->GetSkillManager();

    int32_t sourceEntityID = p.ReadS32Little();
    uint32_t skillID = p.ReadU32Little();

    uint32_t targetType = p.ReadU32Little();
    if(targetType != ACTIVATION_NOTARGET && p.Left() == 0)
    {
        LOG_ERROR(libcomp::String("Invalid skill target type sent from"
            " client: %1\n").Arg(state->GetAccountUID().ToString()));
        return false;
    }

    auto source = state->GetEntityState(sourceEntityID);
    if(!source)
    {
        LOG_ERROR(libcomp::String("Invalid skill source sent from client for"
            " skill activation: %1\n").Arg(state->GetAccountUID().ToString()));
        client->Close();
        return true;
    }

    int64_t activationObjectID = -1;
    int64_t targetObjectID = -1;
    switch(targetType)
    {
        case ACTIVATION_NOTARGET:
            //Nothing special to do
            break;
        case ACTIVATION_DEMON:
            activationObjectID = p.ReadS64Little();
            break;
        case ACTIVATION_ITEM:
            {
                activationObjectID = p.ReadS64Little();
                auto item = std::dynamic_pointer_cast<objects::Item>(
                    libcomp::PersistentObject::GetObjectByUUID(
                        state->GetObjectUUID(activationObjectID)));

                // If the item is invalid or it is an expired rental, fail the skill
                if(!item || (item->GetRentalExpiration() > 0 &&
                    item->GetRentalExpiration() < (uint32_t)std::time(0)))
                {
                    skillManager->SendFailure(source, skillID, client,
                        (uint8_t)SkillErrorCodes_t::ITEM_USE);
                    return true;
                }
            }
            break;
        case ACTIVATION_TARGET:
            activationObjectID = targetObjectID = (int64_t)p.ReadS32Little();
            break;
        case ACTIVATION_FUSION:
            if(source != state->GetCharacterState())
            {
                LOG_ERROR(libcomp::String("Fusion skill activation requested from"
                    " non-character source: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                skillManager->SendFailure(source, skillID, client);
            }
            else if(p.Left() < 28)
            {
                return false;
            }
            else
            {
                int32_t targetEntityID = p.ReadS32Little();
                int64_t summonedDemonID = p.ReadS64Little();
                int64_t compDemonID = p.ReadS64Little();

                float xPos = p.ReadFloat();
                float yPos = p.ReadFloat();

                // The supplied x/y positions appear to be nonsense based on
                // certain zones and positions. The proper position will be
                // calculated in the prepare function.
                (void)xPos;
                (void)yPos;

                // Demon fusion skills are always sent from the client as an
                // "execution skill" that the server needs to convert based
                // on the demons involved
                if(!skillManager->PrepareFusionSkill(client, skillID,
                    targetEntityID, summonedDemonID, compDemonID))
                {
                    return true;
                }

                targetObjectID = (int64_t)targetEntityID;
                activationObjectID = compDemonID;
            }
            break;
        default:
            {
                LOG_ERROR(libcomp::String("Unknown skill target type encountered: %1\n")
                    .Arg(targetType));
                skillManager->SendFailure(source, skillID, client);
                return true;
            }
            break;
    }

    server->QueueWork([](SkillManager* pSkillManager, const std::shared_ptr<
        ActiveEntityState> pSource, uint32_t pSkillID, int64_t pActivationObjectID,
        int64_t pTargetObjectID, uint8_t pTargetType)
        {
            pSkillManager->ActivateSkill(pSource, pSkillID, pActivationObjectID,
                pTargetObjectID, pTargetType);
        }, skillManager, source, skillID, activationObjectID, targetObjectID,
        (uint8_t)targetType);

    return true;
}
