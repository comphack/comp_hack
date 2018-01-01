/**
 * @file server/channel/src/packets/game/EquipmentModEdit.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

// objects Includes
#include <ActivatedAbility.h>
#include <Item.h>
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::EquipmentModEdit::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 17)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    int32_t entityID = p.ReadS32Little();
    int8_t skillActivationID = p.ReadS8();
    int64_t itemID = p.ReadS64Little();
    uint32_t modItemType = p.ReadU32Little();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    bool valid = false;
    uint8_t type = 0;
    uint8_t offset = 0;
    for(auto& itemSet : SVR_CONST.EQUIP_MOD_EDIT_ITEMS)
    {
        offset = 0;

        for(uint32_t itemType : itemSet)
        {
            if(itemType == modItemType)
            {
                valid = true;
                break;
            }

            offset++;
        }

        if(valid)
        {
            break;
        }

        type++;
    }

    const int32_t RESULT_CODE_ERROR = -1;
    const int32_t RESULT_CODE_SUCCESS = 0;
    const int32_t RESULT_CODE_FAIL = 1;

    const int32_t MODE_ADD_SLOT = 0;
    const int32_t MODE_EMPTY_SLOT = 1;
    const int32_t MODE_ADD_SOUL_TAROT = 2;
    const int32_t MODE_EMPTY_SOUL_TAROT = 3;

    int32_t responseCode = RESULT_CODE_ERROR;
    int32_t mode = 0;
    uint32_t subMode = 0;

    const uint16_t successRates[3][6] = {
            { 2000, 3333, 5000, 7000, 5000, 10000 }, // Mod slot
            { 400, 800, 1200, 1600, 1200, 10000 },   // Tarot
            { 200, 400, 600, 1200, 600, 10000 }      // Soul
        };

    if(valid)
    {
        switch(type)
        {
        case 0:
            // Mod slot add
            if(offset < 6)
            {
                mode = MODE_ADD_SLOT;

                for(uint32_t modSlot : item->GetModSlots())
                {
                    if(modSlot == 0)
                    {
                        break;
                    }

                    subMode++;
                }

                if(subMode >= 5)
                {
                    // No mod slots available
                    break;
                }

                // Start with the base success rate and lower for
                // the later slots
                uint16_t successRate = successRates[0][(size_t)offset];
                switch(offset)
                {
                case 1:
                    successRate = (uint16_t)(successRate / 3);
                    break;
                case 2:
                    successRate = (uint16_t)(successRate / 6);
                    break;
                case 3:
                case 4:
                    successRate = (uint16_t)(successRate / 20);
                    break;
                default:
                    break;
                }

                if(RNG(uint16_t, 1, 10000) <= successRate)
                {
                    item->SetModSlots((size_t)subMode, MOD_SLOT_NULL_EFFECT);
                    responseCode = RESULT_CODE_SUCCESS;
                }
                else
                {
                    responseCode = RESULT_CODE_FAIL;
                }
            }
            break;
        case 1:
            // Tarot add
            if(offset < 6)
            {
                /// @todo
                mode = MODE_ADD_SOUL_TAROT;
                subMode = 1;
            }
            break;
        case 2:
            // Soul add
            if(offset < 6)
            {
                /// @todo
                mode = MODE_ADD_SOUL_TAROT;
                subMode = 0;
            }
            break;
        case 3:
            // Tarot empty
            if(offset < 3)
            {
                /// @todo
                mode = MODE_EMPTY_SOUL_TAROT;
                subMode = 1;
            }
            break;
        case 4:
            // Soul empty
            if(offset < 3)
            {
                /// @todo
                mode = MODE_EMPTY_SOUL_TAROT;
                subMode = 0;
            }
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            // Mod slot empty
            if(offset < 3)
            {
                mode = MODE_EMPTY_SLOT;
                subMode = (uint32_t)(type - 5);

                // Start with the base success rate, decrease the failure rate
                // by 50%, then lower for the later slots
                uint16_t successRate = successRates[0][(size_t)offset];
                successRate = (uint16_t)(successRate + (10000 - successRate) / 2);
                switch(offset)
                {
                case 1:
                    successRate = (uint16_t)(successRate / 3);
                    break;
                case 2:
                    successRate = (uint16_t)(successRate / 6);
                    break;
                case 3:
                case 4:
                    successRate = (uint16_t)(successRate / 20);
                    break;
                default:
                    break;
                }

                if(RNG(uint16_t, 1, 10000) <= successRate)
                {
                    item->SetModSlots((size_t)subMode, MOD_SLOT_NULL_EFFECT);
                    responseCode = RESULT_CODE_SUCCESS;
                }
                else
                {
                    responseCode = RESULT_CODE_FAIL;
                }
            }
            break;
        default:
            break;
        }
    }

    if(responseCode != RESULT_CODE_ERROR)
    {
        auto characterManager = server->GetCharacterManager();
        std::unordered_map<uint32_t, uint32_t> itemCounts = { { modItemType, 1 } };
        characterManager->AddRemoveItems(client, itemCounts, false);

        if(responseCode == RESULT_CODE_SUCCESS)
        {
            std::list<uint16_t> slots = { (uint16_t)item->GetBoxSlot() };
            characterManager->SendItemBoxData(client, item->GetItemBox().Get(),
                slots);

            server->GetWorldDatabase()->QueueUpdate(item, state->GetAccountUID());
        }
        else
        {
            /// @todo: drop durability
        }
    }

    auto activatedAbility = cState->GetActivatedAbility();
    if(activatedAbility && activatedAbility->GetActivationID() == skillActivationID)
    {
        if(responseCode != RESULT_CODE_ERROR)
        {
            server->GetSkillManager()->ExecuteSkill(cState, (uint8_t)skillActivationID,
                (int64_t)cState->GetEntityID());
        }
        else
        {
            server->GetSkillManager()->SendFailure(cState, activatedAbility->GetSkillID(),
                client);
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EQUIPMENT_MOD_EDIT);
    reply.WriteS32Little(entityID);
    reply.WriteS64Little(itemID);
    reply.WriteU32Little(item->GetType());
    reply.WriteU32Little(modItemType);
    reply.WriteS32Little(mode);
    reply.WriteU32Little(subMode);
    reply.WriteS32Little(responseCode);

    client->SendPacket(reply);

    return true;
}
