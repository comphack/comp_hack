/**
 * @file server/channel/src/packets/game/Barter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief
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
#include <Item.h>
#include <ItemBox.h>
#include <MiItemData.h>
#include <MiNPCBarterData.h>
#include <MiNPCBarterItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

void HandleBarter(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client, uint16_t barterID)
{
    auto state = client->GetClientState();
    auto definitionManager = server->GetDefinitionManager();

    auto barterData = definitionManager->GetNPCBarterData(barterID);

    std::unordered_map<uint32_t, int32_t> itemAdjustments;

    bool failed = barterData == nullptr;
    if(!failed)
    {
        for(auto itemData : barterData->GetTradeItems())
        {
            auto type = itemData->GetType();
            switch(type)
            {
            case objects::MiNPCBarterItemData::Type_t::ITEM:
                {
                    uint32_t itemType = (uint32_t)itemData->GetSubtype();
                    if(itemAdjustments.find(itemType) == itemAdjustments.end())
                    {
                        itemAdjustments[itemType] = -itemData->GetAmount();
                    }
                    else
                    {
                        itemAdjustments[itemType] = (int32_t)(
                            itemAdjustments[itemType] - itemData->GetAmount());
                    }
                }
                break;
            case objects::MiNPCBarterItemData::Type_t::SOUL_POINT:
                /// @todo: for now just don't pay soul points
                break;
            case objects::MiNPCBarterItemData::Type_t::BETHEL:
                /// @todo: for now just don't pay bethel
                break;
            case objects::MiNPCBarterItemData::Type_t::COIN:
                /// @todo: for now just don't pay coins
                break;
            case objects::MiNPCBarterItemData::Type_t::EVENT_COUNTER:
            case objects::MiNPCBarterItemData::Type_t::CASINO_RESTRICTION:
            case objects::MiNPCBarterItemData::Type_t::SKILL_CHARACTER:
            case objects::MiNPCBarterItemData::Type_t::SKILL_DEMON:
            case objects::MiNPCBarterItemData::Type_t::PLUGIN:
                LOG_ERROR(libcomp::String("Invalid barter trade item"
                    " type encountered: %1\n").Arg((uint8_t)type));
                failed = true;
                break;
            default:
                break;
            }
        }
    }

    std::list<int32_t> characterSkills;
    std::list<int32_t> demonSkills;
    std::list<int32_t> pluginIDs;
    if(!failed)
    {
        for(auto itemData : barterData->GetResultItems())
        {
            auto type = itemData->GetType();
            switch(type)
            {
            case objects::MiNPCBarterItemData::Type_t::ITEM:
                {
                    uint32_t itemType = (uint32_t)itemData->GetSubtype();
                    if(itemAdjustments.find(itemType) == itemAdjustments.end())
                    {
                        itemAdjustments[itemType] = itemData->GetAmount();
                    }
                    else
                    {
                        itemAdjustments[itemType] = (int32_t)(
                            itemAdjustments[itemType] + itemData->GetAmount());
                    }
                }
                break;
            case objects::MiNPCBarterItemData::Type_t::SOUL_POINT:
                /// @todo: for now just don't receive soul points
                break;
            case objects::MiNPCBarterItemData::Type_t::EVENT_COUNTER:
                /// @todo: for now just don't set event counter
                break;
            case objects::MiNPCBarterItemData::Type_t::CASINO_RESTRICTION:
                /// @todo: for now just don't set casino restriction
                break;
            case objects::MiNPCBarterItemData::Type_t::BETHEL:
                /// @todo: for now just don't receive bethel
                break;
            case objects::MiNPCBarterItemData::Type_t::SKILL_CHARACTER:
                characterSkills.push_back(itemData->GetSubtype());
                break;
            case objects::MiNPCBarterItemData::Type_t::SKILL_DEMON:
                if(!state->GetDemonState()->GetEntity())
                {
                    LOG_ERROR("Attempted to add a barter demon skill"
                        " to a player without a demon summoned\n");
                    failed = true;
                }
                else
                {
                    demonSkills.push_back(itemData->GetSubtype());
                }
                break;
            case objects::MiNPCBarterItemData::Type_t::PLUGIN:
                pluginIDs.push_back(itemData->GetSubtype());
                break;
            case objects::MiNPCBarterItemData::Type_t::COIN:
                /// @todo: for now just don't receive coins
                break;
                break;
            default:
                break;
            }
        }
    }

    if(!failed)
    {
        // If there have not been failures yet, determine item adjustments
        // and apply all changes
        auto character = state->GetCharacterState()->GetEntity();
        auto inventory = character->GetItemBoxes(0).Get();
        auto characterManager = server->GetCharacterManager();

        std::list<std::shared_ptr<objects::Item>> insertItems;
        std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems;
        for(auto itemPair : itemAdjustments)
        {
            auto itemData = definitionManager->GetItemData(itemPair.first);
            if(!itemData)
            {
                LOG_ERROR(libcomp::String("Invalid item type encountered for"
                    " barter request: %1\n").Arg(itemPair.first));
                failed = true;
                break;
            }

            int32_t qtyLeft = itemPair.second;

            auto existing = characterManager->GetExistingItems(character,
                itemPair.first, inventory);
            if(qtyLeft > 0)
            {
                // Update existing stacks first if we aren't adding a full stack
                int32_t maxStack = (int32_t)itemData->GetPossession()->GetStackSize();
                if(qtyLeft < maxStack)
                {
                    for(auto item : existing)
                    {
                        if(qtyLeft == 0) break;

                        uint16_t stackLeft = (uint16_t)(maxStack - item->GetStackSize());
                        if(stackLeft == 0) continue;

                        uint16_t stackAdd = (uint16_t)((qtyLeft <= stackLeft)
                            ? qtyLeft : stackLeft);

                        if(stackAdjustItems.find(item) == stackAdjustItems.end())
                        {
                            stackAdjustItems[item] = item->GetStackSize();
                        }

                        stackAdjustItems[item] = (uint16_t)(
                            stackAdjustItems[item] + stackAdd);
                        qtyLeft = (int32_t)(qtyLeft - stackAdd);
                    }
                }

                // If there are still more to create, add as new items
                while(qtyLeft > 0)
                {
                    uint16_t stack = (uint16_t)((qtyLeft > maxStack)
                        ? maxStack : qtyLeft);
                    insertItems.push_back(characterManager->GenerateItem(
                        itemPair.first, stack));
                    qtyLeft = (int32_t)(qtyLeft - stack);
                }
            }
            else if(qtyLeft < 0)
            {
                // Remove from the last stack first and reverse the quantity
                // for easier comparison
                existing.reverse();
                qtyLeft = qtyLeft * -1;

                for(auto item : existing)
                {
                    if(qtyLeft == 0) break;

                    if(qtyLeft <= (int32_t)item->GetStackSize())
                    {
                        stackAdjustItems[item] = (uint16_t)(
                            item->GetStackSize() - qtyLeft);
                        qtyLeft = 0;
                    }
                    else
                    {
                        stackAdjustItems[item] = 0;
                        qtyLeft = qtyLeft - item->GetStackSize();
                    }
                }

                if(qtyLeft > 0)
                {
                    failed = true;
                    break;
                }
            }
        }

        // Update items first as they're the only thing that can actually
        // fail past this point when everything is working right
        if(!failed && (stackAdjustItems.size() > 0 || insertItems.size() > 0))
        {
            failed = !characterManager->UpdateItems(client, false,
                insertItems, stackAdjustItems);
        }

        // Now apply the rest of the updates
        if(!failed)
        {
            if(characterSkills.size() > 0)
            {
                int32_t characterEntityID = state->GetCharacterState()
                    ->GetEntityID();
                for(int32_t skillID : characterSkills)
                {
                    failed |= !characterManager->LearnSkill(client, characterEntityID,
                        (uint32_t)skillID);
                }
            }

            if(demonSkills.size() > 0)
            {
                int32_t demonEntityID = state->GetDemonState()
                    ->GetEntityID();
                for(int32_t skillID : demonSkills)
                {
                    failed |= !characterManager->LearnSkill(client, demonEntityID,
                        (uint32_t)skillID);
                }
            }

            if(pluginIDs.size() > 0)
            {
                for(int32_t pluginID : pluginIDs)
                {
                    failed |= !characterManager->AddPlugin(client,
                        (uint16_t)pluginID);
                }
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BARTER);
    reply.WriteS8(failed ? -1 : 0); // No special error codes for this
    reply.WriteU16Little(barterID);

    client->SendPacket(reply);
}

bool Parsers::Barter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 2)
    {
        return false;
    }

    uint16_t barterID = p.ReadU16Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    server->QueueWork(HandleBarter, server, client, barterID);

    return true;
}
