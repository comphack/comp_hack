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
#include <ErrorCodes.h>
#include <Log.h>
#include <ManagerPacket.h>

// object Includes
#include <Item.h>

// channel Includes
#include "ChannelServer.h"

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
        LOG_ERROR("Invalid skill target type sent from client\n");
        return false;
    }

    auto source = state->GetEntityState(sourceEntityID);
    if(!source)
    {
        LOG_ERROR("Invalid skill source sent from client for skill activation\n");
        return false;
    }

    int64_t targetObjectID = -1;
    switch(targetType)
    {
        case ACTIVATION_NOTARGET:
            //Nothing special to do
            break;
        case ACTIVATION_DEMON:
            targetObjectID = p.ReadS64Little();
            break;
        case ACTIVATION_ITEM:
            {
                targetObjectID = p.ReadS64Little();
                auto item = std::dynamic_pointer_cast<objects::Item>(
                    libcomp::PersistentObject::GetObjectByUUID(
                        state->GetObjectUUID(targetObjectID)));

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
            targetObjectID = (int64_t)p.ReadS32Little();
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
        ActiveEntityState> pSource, uint32_t pSkillID, int64_t pTargetObjectID)
        {
            pSkillManager->ActivateSkill(pSource, pSkillID, pTargetObjectID);
        }, skillManager, source, skillID, targetObjectID);

    return true;
}
