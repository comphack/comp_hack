/**
 * @file server/channel/src/packets/game/CultureMachineAccess.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to access a culture machine.
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
#include <DefinitionManager.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

 // object Includes
#include <CultureData.h>
#include <Item.h>
#include <MiSkillData.h>
#include <MiSkillSpecialParams.h>
#include <ServerCultureMachineSet.h>

 // channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "CultureMachineState.h"
#include "EventManager.h"

using namespace channel;

bool Parsers::CultureMachineAccess::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    const int8_t STATUS_FREE = 0;
    const int8_t STATUS_ACTIVE_EXISTS = 1;
    const int8_t STATUS_ITEM_PENDING = 2;
    const int8_t STATUS_PREVIOUS = 3;
    //const int8_t STATUS_OTHER_ERROR = 4;  // Fail state for other owner
    const int8_t STATUS_SELF = 5;
    const int8_t STATUS_OTHER = 6;
    const int8_t STATUS_EXPERT_LOW = 7;
    const int8_t STATUS_FAIL = -1;

    int32_t machineEntityID = p.ReadS32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto eventManager = server->GetEventManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto zone = cState->GetZone();

    auto cmState = zone ? zone->GetCultureMachine(machineEntityID) : nullptr;
    auto rental = cmState ? cmState->GetRentalData() : nullptr;
    auto def = cmState->GetEntity();

    bool itemSet = rental && !rental->GetItem().IsNull();
    auto item = itemSet ? libcomp::PersistentObject::LoadObjectByUUID<
        objects::Item>(server->GetWorldDatabase(), rental->GetItem().GetUUID())
        : nullptr;

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_CULTURE_MACHINE_ACCESS);
    reply.WriteS32Little(machineEntityID);
    reply.WriteS8(0);   // Success

    if(cmState && (!itemSet || item) && eventManager->RequestMenu(client,
        (int32_t)SVR_CONST.MENU_CULTURE, 0, machineEntityID))
    {
        bool isOwner = rental &&
            rental->GetCharacter() == cState->GetEntityUUID();
        
        // Get the right status
        int8_t status = STATUS_FAIL;
        if(isOwner)
        {
            status = STATUS_SELF;
        }
        else if(rental)
        {
            status = STATUS_OTHER;
        }
        else if(cState->GetExpertiseRank(EXPERTISE_CHAIN_CRAFTMANSHIP,
            definitionManager) < 10)
        {
            // Class 1 required to use
            status = STATUS_EXPERT_LOW;
        }
        else
        {
            // Make sure the player can rent the specific machine
            auto character = cState->GetEntity();
            auto cData = character
                ? character->GetCultureData().Get() : nullptr;
            if(cData && cData->GetMachineID() == cmState->GetMachineID())
            {
                // Cannot rent the same machine twice
                status = STATUS_PREVIOUS;
            }
            else if(cData && cData->GetActive())
            {
                // Cannot rent two machines
                status = STATUS_ACTIVE_EXISTS;
            }
            else if(cData && !cData->GetItem().IsNull())
            {
                // Cannot rent while an item is pending pickup
                status = STATUS_ITEM_PENDING;
            }
            else
            {
                status = STATUS_FREE;
            }
        }

        reply.WriteS8(status);
        if(status == STATUS_SELF || status == STATUS_FREE ||
            status == STATUS_OTHER)
        {
            // Calculate multipliers
            double passiveBoost = 1.0;
            double demonBoost = 1.0;

            for(uint32_t skillID : definitionManager->GetFunctionIDSkills(
                SVR_CONST.SKILL_CULTURE_UP))
            {
                if(cState->CurrentSkillsContains(skillID))
                {
                    auto skillData = definitionManager->GetSkillData(skillID);
                    int32_t boost = skillData ? skillData->GetSpecial()
                        ->GetSpecialParams(0) : 0;
                    passiveBoost = passiveBoost + (double)boost * 0.01;
                }
            }

            auto dState = state->GetDemonState();
            if(dState->Ready())
            {
                int16_t intel = dState->GetINTEL();
                int16_t luck = dState->GetLUCK();
                for(auto pair : SVR_CONST.ADJUSTMENT_SKILLS)
                {
                    if(pair.second[0] == 4 && pair.second[1] == 2 &&
                        dState->CurrentSkillsContains((uint32_t)pair.first))
                    {
                        demonBoost = demonBoost +
                            (double)(intel * luck) / 100000;
                    }
                }
            }

            reply.WriteS8((int8_t)def->GetDays());
            reply.WriteU32Little(def->GetRequiredDailyPoints());
            reply.WriteU32Little(def->GetMaxDailyPoints());
            reply.WriteU32Little(def->GetCost());

            for(size_t i = 0; i < 5; i++)
            {
                reply.WriteU16Little(def->GetDailyItemRates(i));
            }

            reply.WriteDouble(passiveBoost);
            reply.WriteDouble(demonBoost);

            characterManager->GetItemDetailPacketData(reply, item);

            if(rental)
            {
                for(size_t i = 0; i < 5; i++)
                {
                    reply.WriteS32Little((int32_t)rental->GetPoints(i));
                }

                reply.WriteS32Little(ChannelServer::GetExpirationInSeconds(
                    rental->GetExpiration()));
            }
            else
            {
                reply.WriteBlank(24);
            }

            for(size_t i = 0; i < 10; i++)
            {
                uint32_t itemID = rental ? rental->GetItemHistory(i) : 0;
                reply.WriteU32Little(itemID ? itemID : 0xFFFFFFFF);
            }

            reply.WriteU32Little(rental ? rental->GetItemCount() : 0);
        }
    }
    else
    {
        reply.WriteS8(STATUS_FAIL);
    }

    client->SendPacket(reply);

    return true;
}
