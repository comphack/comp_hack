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
    auto defIter = SVR_CONST.EQUIP_MOD_EDIT_ITEMS.find(item ? item->GetType() : 0);
    if(defIter != SVR_CONST.EQUIP_MOD_EDIT_ITEMS.end())
    {
        valid = true;
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

    if(valid)
    {
        int32_t successRate = defIter->second[2];

        mode = defIter->second[0];
        subMode = (uint32_t)defIter->second[1];

        switch(mode)
        {
        case MODE_ADD_SLOT:
            {
                subMode = 0;
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
                switch(subMode)
                {
                case 1:
                    successRate = (int32_t)(successRate / 3);
                    break;
                case 2:
                    successRate = (int32_t)(successRate / 6);
                    break;
                case 3:
                case 4:
                    successRate = (int32_t)(successRate / 20);
                    break;
                default:
                    break;
                }

                if(RNG(int32_t, 1, 10000) <= successRate)
                {
                    responseCode = RESULT_CODE_SUCCESS;

                    item->SetModSlots((size_t)subMode, MOD_SLOT_NULL_EFFECT);
                }
                else
                {
                    responseCode = RESULT_CODE_FAIL;
                }
            }
            break;
        case MODE_EMPTY_SLOT:
        case MODE_ADD_SOUL_TAROT:
        case MODE_EMPTY_SOUL_TAROT:
            if(RNG(int32_t, 1, 10000) <= successRate)
            {
                responseCode = RESULT_CODE_SUCCESS;

                switch(mode)
                {
                case MODE_EMPTY_SLOT:
                    item->SetModSlots((size_t)subMode, MOD_SLOT_NULL_EFFECT);
                    break;
                case MODE_ADD_SOUL_TAROT:
                    if(subMode == 0)
                    {
                        /// @todo: enable tarot
                    }
                    else
                    {
                        /// @todo: enable soul
                    }
                    break;
                case MODE_EMPTY_SOUL_TAROT:
                    if(subMode == 0)
                    {
                        item->SetTarot(0);
                    }
                    else
                    {
                        item->SetSoul(0);
                    }
                    break;
                default:
                    break;
                }
            }
            else
            {
                responseCode = RESULT_CODE_FAIL;
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
            characterManager->UpdateDurability(client, item, -5000);
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
