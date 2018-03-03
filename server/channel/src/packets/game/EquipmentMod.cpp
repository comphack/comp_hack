/**
 * @file server/channel/src/packets/game/EquipmentMod.cpp
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

// Standard C++11 Includes
#include <math.h>

// objects Includes
#include <Item.h>
#include <ItemBox.h>
#include <MiCategoryData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiModificationData.h>
#include <MiModificationExtEffectData.h>
#include <MiModificationExtRecipeData.h>
#include <MiModificationTriggerData.h>
#include <MiModifiedEffectData.h>
#include <MiSkillItemStatusCommonData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::EquipmentMod::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 32)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto definitionManager = server->GetDefinitionManager();
    auto state = client->GetClientState();

    int64_t modItemID = p.ReadS64Little();
    int64_t equipmentID = p.ReadS64Little();
    int64_t slotItemID = p.ReadS64Little();
    int64_t catalystID = p.ReadS64Little();

    auto modificationItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(modItemID)));
    auto equipmentItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(equipmentID)));
    auto slotItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(slotItemID)));
    auto catalyst = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(catalystID)));

    const int32_t RESULT_CODE_ERROR = -1;
    const int32_t RESULT_CODE_SUCCESS = 0;
    const int32_t RESULT_CODE_FAIL = 1;
    const int32_t RESULT_CODE_GREAT_SUCCESS = 2;
    const int32_t RESULT_CODE_GREAT_FAIL = 3;

    int32_t resultCode = RESULT_CODE_ERROR;

    uint8_t slot = 0;
    uint16_t effectID = 0;
    uint32_t greatFailItemType = 0;
    uint16_t greatFailItemCount = 0;

    // Make sure we have the 3 main items involved
    if(modificationItem && equipmentItem && slotItem)
    {
        resultCode = RESULT_CODE_FAIL;

        auto equipmentData = definitionManager->GetItemData(equipmentItem->GetType());
        bool isWeapon = equipmentData->GetBasic()->GetEquipType() ==
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON;

        int16_t successRate = 0;
        int16_t greatSuccessRate = 0;
        int16_t greatFailRate = 0;
        uint16_t successScaling = 0;

        // Not used by weapons, the groupID is equal to the item's subcategory value
        uint8_t groupID = equipmentData->GetCommon()->GetCategory()->GetSubCategory();
        int16_t effectType = 0;
        int16_t effectSequenceID = 0;

        // Weapons and armor use completely different objects for modification
        if(isWeapon)
        {
            size_t successScaleIdx = 0;
            for(uint32_t itemType : SVR_CONST.SLOT_MOD_ITEMS[0])
            {
                if(itemType == modificationItem->GetType())
                {
                    break;
                }
                successScaleIdx++;
            }

            auto modData = definitionManager->GetModificationDataByItemID(
                slotItem->GetType());
            auto triggerData = modData ? definitionManager->GetModificationTriggerData(
                modData->GetEffectID()) : nullptr;
            auto effectData = modData ? definitionManager->GetModifiedEffectData(
                modData->GetEffectID()) : nullptr;

            if(modData && triggerData && effectData && successScaleIdx < 8)
            {
                slot = modData->GetSlot();

                successRate = modData->GetSuccessRate();
                greatSuccessRate = modData->GetGreatSuccessRate();
                greatFailRate = modData->GetGreatFailRate();
                greatFailItemType = modData->GetGreatFailItemType();
                greatFailItemCount = modData->GetGreatFailItemCount();

                successScaling = triggerData->GetRateScaling(successScaleIdx);

                effectID = effectData->GetID();
                effectType = (int16_t)effectData->GetType();
                effectSequenceID = (int16_t)effectData->GetSequenceID();
            }
            else
            {
                LOG_ERROR(libcomp::String("Invalid data encountered for weapon"
                    " modification for slot item: %1\n").Arg(slotItem->GetType()));
                resultCode = RESULT_CODE_ERROR;
            }
        }
        else
        {
            size_t successScaleIdx = 0;
            for(uint32_t itemType : SVR_CONST.SLOT_MOD_ITEMS[1])
            {
                if(itemType == modificationItem->GetType())
                {
                    break;
                }
                successScaleIdx++;
            }

            auto recipeData = definitionManager->GetModificationExtRecipeDataByItemID(
                slotItem->GetType());
            auto effectData = recipeData ? definitionManager->GetModificationExtEffectData(
                groupID, recipeData->GetSlot(), recipeData->GetEffectSubID()) : nullptr;

            if(recipeData && effectData && successScaleIdx < 8)
            {
                slot = recipeData->GetSlot();

                successRate = recipeData->GetSuccessRate();
                greatSuccessRate = recipeData->GetGreatSuccessRate();
                greatFailRate = recipeData->GetGreatFailRate();
                // Armor great failures do not leave leftover items

                successScaling = effectData->GetRateScaling(successScaleIdx);

                effectID = effectData->GetSubID();
                effectType = effectData->GetType();
                effectSequenceID = effectData->GetSequenceID();
            }
            else
            {
                LOG_ERROR(libcomp::String("Invalid data encountered for equipment"
                    " modification for slot item: %1\n").Arg(slotItem->GetType()));
                resultCode = RESULT_CODE_ERROR;
            }
        }

        if(resultCode != RESULT_CODE_ERROR)
        {
            // Verify if the change is either an increase of one rank
            // or a change to a new or completely different effect
            uint16_t currentEffectID = equipmentItem->GetModSlots(slot);

            int16_t currentEffectType = 0;
            int16_t currentEffectSequenceID = 0;
            if(currentEffectID)
            {
                if(isWeapon)
                {
                    auto currentEffectData = definitionManager->GetModifiedEffectData(
                        currentEffectID);
                    if(currentEffectData)
                    {
                        currentEffectType = currentEffectData->GetType();
                        currentEffectSequenceID = currentEffectData->GetSequenceID();
                    }
                }
                else
                {
                    auto currentEffectData = definitionManager->GetModificationExtEffectData(
                        groupID, slot, currentEffectID);
                    if(currentEffectData)
                    {
                        currentEffectType = currentEffectData->GetType();
                        currentEffectSequenceID = currentEffectData->GetSequenceID();
                    }
                }
            }

            if(currentEffectType && currentEffectType == effectType)
            {
                if(effectSequenceID != currentEffectSequenceID + 1)
                {
                    // Attempting to update to a value other than the next step
                    LOG_ERROR(libcomp::String("Invalid request to update modification effect"
                        " %1 from rank %2 to %3\n").Arg(effectType)
                        .Arg(currentEffectSequenceID).Arg(effectSequenceID));
                    resultCode = RESULT_CODE_ERROR;
                }
            }
            else if(effectSequenceID != 1)
            {
                // Attempting to skip past first level
                LOG_ERROR(libcomp::String("Invalid request to update modification effect"
                    " %1 directly to rank %2\n").Arg(effectType).Arg(effectSequenceID));
                resultCode = RESULT_CODE_ERROR;
            }
        }

        // If there are no errors up to this point, attempt the modification
        if(resultCode != RESULT_CODE_ERROR)
        {
            successRate = (int16_t)floor((float)successRate * (successScaling * 0.01f));
            if(successRate > 0 &&
                (successRate >= 10000 || RNG(int16_t, 1, 10000) <= successRate))
            {
                resultCode = RESULT_CODE_SUCCESS;

                if(greatSuccessRate > 0 &&
                    (greatSuccessRate >= 10000 || RNG(int16_t, 1, 10000) <= greatSuccessRate))
                {
                    resultCode = RESULT_CODE_GREAT_SUCCESS;

                    // Add an additional effect rank if not already max
                    uint16_t nextEffectID = (uint16_t)(effectID + 1);
                    if(isWeapon)
                    {
                        auto nextEffectData = definitionManager->GetModifiedEffectData(
                            nextEffectID);
                        if(nextEffectData && nextEffectData->GetType() == effectType &&
                            nextEffectData->GetSequenceID() == (int8_t)(effectSequenceID + 1))
                        {
                            effectID = nextEffectID;
                        }
                    }
                    else
                    {
                        auto nextEffectData = definitionManager->GetModificationExtEffectData(
                            groupID, slot, nextEffectID);
                        if(nextEffectData && nextEffectData->GetType() == effectType &&
                            nextEffectData->GetSequenceID() == (int16_t)(effectSequenceID + 1))
                        {
                            effectID = nextEffectID;
                        }
                    }
                }
            }
            else
            {
                resultCode = RESULT_CODE_FAIL;

                if(greatFailRate > 0 &&
                    (greatFailRate >= 10000 || RNG(int16_t, 1, 10000) <= greatFailRate))
                {
                    resultCode = RESULT_CODE_GREAT_FAIL;
                }
            }
        }
    }

    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems;
    if(resultCode != RESULT_CODE_ERROR)
    {
        stackAdjustItems[slotItem] = (uint16_t)(slotItem->GetStackSize() - 1);
        stackAdjustItems[modificationItem] = (uint16_t)(
            modificationItem->GetStackSize() - 1);
    }

    auto characterManager = server->GetCharacterManager();
    switch(resultCode)
    {
    case RESULT_CODE_SUCCESS:
    case RESULT_CODE_GREAT_SUCCESS:
        if(effectID != 0)
        {
            equipmentItem->SetModSlots((size_t)slot, effectID);

            std::list<uint16_t> slots = { (uint16_t)equipmentItem->GetBoxSlot() };
            characterManager->SendItemBoxData(client, equipmentItem->GetItemBox().Get(),
                slots);

            server->GetWorldDatabase()->QueueUpdate(equipmentItem,
                state->GetAccountUID());
        }
        break;
    case RESULT_CODE_FAIL:
    case RESULT_CODE_GREAT_FAIL:
        {
            auto destroyItem = catalyst;
            if(resultCode == RESULT_CODE_GREAT_FAIL && !catalyst)
            {
                destroyItem = equipmentItem;
            }
            else
            {
                // Drop durability but don't destroy the item
                characterManager->UpdateDurability(client, equipmentItem,
                    -5000);

                resultCode = RESULT_CODE_FAIL;
            }

            if(destroyItem)
            {
                stackAdjustItems[destroyItem] = (uint16_t)(
                    destroyItem->GetStackSize() - 1);
            }
        }
        break;
    default:
        break;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EQUIPMENT_MODIFY);
    reply.WriteS64Little(modItemID);
    reply.WriteS64Little(equipmentID);
    reply.WriteS64Little(slotItemID);
    reply.WriteS64Little(catalystID);
    reply.WriteS32Little(resultCode);

    client->QueuePacket(reply);

    if(stackAdjustItems.size() > 0)
    {
        std::list<std::shared_ptr<objects::Item>> inserts;

        // Certain great failures result in materials breaking off the original item
        if(resultCode == RESULT_CODE_GREAT_FAIL && greatFailItemType)
        {
            auto pieces = characterManager->GenerateItem(greatFailItemType,
                greatFailItemCount);
            if(pieces)
            {
                inserts.push_back(pieces);
            }
        }

        characterManager->UpdateItems(client, false, inserts, stackAdjustItems);
    }

    client->FlushOutgoing();

    return true;
}
