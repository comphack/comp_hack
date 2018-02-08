/**
 * @file server/channel/src/packets/game/ItemDisassemble.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to disassemble an item into materials.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// objects Includes
#include <Item.h>
#include <MiDisassemblyData.h>
#include <MiDisassemblyMaterialData.h>
#include <MiDisassemblyTriggerData.h>
#include <MiItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ItemDisassemble::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 16)
    {
        return false;
    }

    int64_t sourceItemID = p.ReadS64Little();
    int64_t targetItemID = p.ReadS64Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto sourceItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(sourceItemID)));
    uint32_t sourceItemType = sourceItem ? sourceItem->GetType() : 0;

    auto targetItem = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(targetItemID)));
    uint32_t targetItemType = targetItem ? targetItem->GetType() : 0;
    auto disDef = definitionManager->GetDisassemblyDataByItemID(targetItemType);

    bool playerHasTank = characterManager->HasValuable(character,
        SVR_CONST.VALUABLE_MATERIAL_TANK);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_DISASSEMBLE);
    reply.WriteS64Little(sourceItemID);
    reply.WriteS64Little(targetItemID);

    std::map<uint32_t, int32_t> resultMaterials;

    uint16_t disCount = 0;

    bool success = false;
    if(playerHasTank && sourceItem && targetItem && disDef)
    {
        int8_t triggerIdx = -1;
        for(size_t i = 0; i < SVR_CONST.DISASSEMBLY_ITEMS.size(); i++)
        {
            if(SVR_CONST.DISASSEMBLY_ITEMS[i] == sourceItemType)
            {
                triggerIdx = (int8_t)i;
                break;
            }
        }

        if(triggerIdx >= 0)
        {
            // Even if all materials fail to disassemble, this is still
            // success at this point
            success = true;

            disCount = targetItem->GetStackSize();
            if(sourceItem->GetStackSize() < disCount)
            {
                disCount = sourceItem->GetStackSize();
            }

            // For each material that can be obtained, check the success
            // rate for the type and the disassembly item scaling
            for(auto outMaterial : disDef->GetMaterials())
            {
                uint32_t outType = outMaterial->GetType();

                if(outType == 0) continue;

                int16_t successRate = outMaterial->GetSuccessRate();

                if(successRate < 10000)
                {
                    // Scale with disassembly item value
                    auto triggerDef = definitionManager
                        ->GetDisassemblyTriggerData(outType);
                    if(triggerDef)
                    {
                        float scaling = (float)triggerDef
                            ->GetRateScaling((size_t)triggerIdx) * 0.01f;
                        successRate = (int16_t)(successRate * scaling);
                    }
                }

                // Check and add to sum for each stack item
                for(uint16_t k = 0; k < disCount; k++)
                {
                    if(successRate >= 10000 ||
                        RNG(int32_t, 1, 10000) <= successRate)
                    {
                        // Create material
                        auto it = resultMaterials.find(outType);
                        if(it != resultMaterials.end())
                        {
                            it->second += outMaterial->GetAmount();
                        }
                        else
                        {
                            resultMaterials[outType] =
                                outMaterial->GetAmount();
                        }
                    }
                }
            }

             std::unordered_map<std::shared_ptr<objects::Item>,
                uint16_t> stackAdjustItems;
             stackAdjustItems[sourceItem] = (uint16_t)(
                 sourceItem->GetStackSize() - disCount);
             stackAdjustItems[targetItem] = (uint16_t)(
                 targetItem->GetStackSize() - disCount);

             std::list<std::shared_ptr<objects::Item>> empty;
            if(!characterManager->UpdateItems(client, false,
                empty, stackAdjustItems))
            {
                success = false;
            }
        }
    }

    reply.WriteS32Little(success ? 0 : -1);

    if(success)
    {
        reply.WriteU16Little(disCount);

        reply.WriteS32Little((int32_t)resultMaterials.size());
        for(auto resultPair : resultMaterials)
        {
            reply.WriteU32Little(resultPair.first);
            reply.WriteS32Little(resultPair.second);
        }
    }

    client->QueuePacket(reply);

    // Now set the new material counts
    if(success)
    {
        std::set<uint32_t> updates;
        for(auto resultPair : resultMaterials)
        {
            uint32_t itemType = resultPair.first;
            auto itemDef = definitionManager->GetItemData(itemType);

            int32_t maxStack = (int32_t)itemDef->GetPossession()->GetStackSize();

            int32_t newStack = character->GetMaterials(itemType) + resultPair.second;

            if(newStack > maxStack)
            {
                newStack = (int32_t)maxStack;
            }

            character->SetMaterials(itemType, (uint16_t)newStack);

            updates.insert(itemType);
        }

        server->GetWorldDatabase()
            ->QueueUpdate(character, state->GetAccountUID());

        characterManager->SendMaterials(client, updates);
    }

    client->FlushOutgoing();

    return true;
}
