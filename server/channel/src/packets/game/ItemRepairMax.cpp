/**
 * @file server/channel/src/packets/game/ItemRepairMax.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to repair the maximum durability of an item.
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
#include <Randomizer.h>
#include <ServerConstants.h>

// object Includes
#include <ActivatedAbility.h>
#include <Item.h>
#include <MiDamageData.h>
#include <MiSkillData.h>
#include <MiSkillSpecialParams.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "SkillManager.h"

using namespace channel;

bool Parsers::ItemRepairMax::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 13)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();
    int8_t activationID = p.ReadS8();
    int64_t itemID = p.ReadS64Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto sourceState = state->GetEntityState(entityID);

    if(sourceState == nullptr)
    {
        LogItemError([&]()
        {
            return libcomp::String("Invalid entity ID received from a"
                " ItemRepairMax request: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        client->Close();
        return true;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));
    int8_t preDurability = item ? item->GetMaxDurability() : 0;

    auto activatedAbility = sourceState->GetSpecialActivations(activationID);

    bool success = false;
    int32_t adjust = 0;
    if(!item)
    {
        LogItemErrorMsg(
            "Invalid item ID encountered for ItemRepairMax request\n");
    }
    else if(!activatedAbility)
    {
        LogItemErrorMsg(
            "Invalid activation ID encountered for ItemRepairMax request\n");
    }
    else
    {
        auto skillData = activatedAbility->GetSkillData();
        if(skillData)
        {
            uint16_t functionID = skillData->GetDamage()->GetFunctionID();
            if(functionID == SVR_CONST.SKILL_MAX_DURABILITY_FIXED)
            {
                adjust = skillData->GetSpecial()->GetSpecialParams(0);

                success = true;
            }
            else if(functionID == SVR_CONST.SKILL_MAX_DURABILITY_RANDOM)
            {
                int32_t min = skillData->GetSpecial()->GetSpecialParams(0);
                int32_t max = skillData->GetSpecial()->GetSpecialParams(1);

                adjust = RNG(int32_t, min, max);

                success = true;
            }
        }
    }

    if(success)
    {
        if(server->GetSkillManager()->ExecuteSkill(sourceState,
            activationID, itemID))
        {
            if(adjust)
            {
                characterManager->UpdateDurability(client, item, adjust, true,
                    true);
            }
        }
        else
        {
            success = false;
        }
    }
    else
    {
        server->GetSkillManager()->CancelSkill(sourceState, activationID);
    }

    if(success)
    {
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_REPAIR_MAX);
        reply.WriteS32Little(entityID);
        reply.WriteS64Little(itemID);
        reply.WriteU32Little(item ? item->GetType() : 0);
        reply.WriteU8((uint8_t)preDurability);
        reply.WriteU8(item ? (uint8_t)item->GetMaxDurability() : 0);

        client->SendPacket(reply);
    }

    return true;
}
