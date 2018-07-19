/**
 * @file server/channel/src/packets/game/Barter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to barter for items or other materials.
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
#include <Item.h>
#include <ItemBox.h>
#include <MiItemData.h>
#include <MiNPCBarterData.h>
#include <MiNPCBarterItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"

using namespace channel;

void HandleBarter(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client, uint16_t barterID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto barterData = definitionManager->GetNPCBarterData(barterID);

    int32_t spAdjust = 0;
    int64_t coinAdjust = 0;
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
                spAdjust = (int32_t)(spAdjust - itemData->GetSubtype());
                break;
            case objects::MiNPCBarterItemData::Type_t::BETHEL:
                LOG_ERROR("Bethel bartering not yet supported\n");
                failed = true;
                break;
            case objects::MiNPCBarterItemData::Type_t::COIN:
                {
                    int64_t total = (int64_t)((int64_t)itemData->GetSubtype() *
                        1000000 + (int64_t)itemData->GetAmount());
                    coinAdjust = (int64_t)(coinAdjust - total);
                }
                break;
            case objects::MiNPCBarterItemData::Type_t::NONE:
                break;
            case objects::MiNPCBarterItemData::Type_t::ONE_TIME_VALUABLE:
            case objects::MiNPCBarterItemData::Type_t::EVENT_COUNTER:
            case objects::MiNPCBarterItemData::Type_t::COOLDOWN:
            case objects::MiNPCBarterItemData::Type_t::SKILL_CHARACTER:
            case objects::MiNPCBarterItemData::Type_t::SKILL_DEMON:
            case objects::MiNPCBarterItemData::Type_t::PLUGIN:
            default:
                LOG_ERROR(libcomp::String("Invalid barter trade item"
                    " type encountered: %1\n").Arg((uint8_t)type));
                failed = true;
                break;
            }
        }
    }

    std::list<int32_t> characterSkills;
    std::list<int32_t> demonSkills;
    std::list<int32_t> pluginIDs;
    std::set<uint16_t> oneTimeValuables;
    std::unordered_map<int32_t, uint32_t> cooldowns;
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
            case objects::MiNPCBarterItemData::Type_t::ONE_TIME_VALUABLE:
                {
                    uint16_t valuableID = (uint16_t)itemData->GetSubtype();
                    oneTimeValuables.insert(valuableID);

                    if(CharacterManager::HasValuable(character, valuableID))
                    {
                        LOG_ERROR(libcomp::String("Player attempted to"
                            " perform barter with a one-time valuable they"
                            " already have: %1\n").Arg(valuableID));
                        failed = true;
                    }
                }
                break;
            case objects::MiNPCBarterItemData::Type_t::SOUL_POINT:
                spAdjust = (int32_t)(spAdjust + itemData->GetSubtype());
                break;
            case objects::MiNPCBarterItemData::Type_t::EVENT_COUNTER:
                /// @todo: for now just don't set event counter
                break;
            case objects::MiNPCBarterItemData::Type_t::COOLDOWN:
                // Calculate the cooldown(s) below (negate for system types)
                cooldowns[-itemData->GetSubtype()] = 0;
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
                    LOG_ERROR(libcomp::String("Attempted to add a barter demon"
                        " skill to a player without a demon summoned: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
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
                {
                    int64_t total = (int64_t)((int64_t)itemData->GetSubtype() *
                        1000000 + (int64_t)itemData->GetAmount());
                    coinAdjust = (int64_t)(coinAdjust + total);
                }
                break;
            case objects::MiNPCBarterItemData::Type_t::NONE:
                break;
            default:
                LOG_ERROR(libcomp::String("Invalid barter result item"
                    " type encountered: %1\n").Arg((uint8_t)type));
                failed = true;
                break;
            }
        }
    }

    if(cooldowns.size() > 0 && !failed)
    {
        // Fail if any cooldowns are active, otherwise calculate new times
        uint32_t now = (uint32_t)std::time(0);

        cState->RefreshActionCooldowns(false, now);
        for(auto& pair : cooldowns)
        {
            if(!cState->ActionCooldownActive(pair.first, false))
            {
                auto it = SVR_CONST.BARTER_COOLDOWNS.find(-pair.first);
                if(it != SVR_CONST.BARTER_COOLDOWNS.end())
                {
                    pair.second = (uint32_t)(now + it->second);
                }
            }
            else
            {
                LOG_ERROR(libcomp::String("Attempted to execute barter"
                    " with active action cooldown type %1: %2\n")
                    .Arg(-pair.first).Arg(state->GetAccountUID().ToString()));
                failed = true;
                break;
            }
        }
    }

    if(!failed)
    {
        // If there have not been failures yet, determine item adjustments
        // and apply all changes
        auto inventory = character->GetItemBoxes(0).Get();

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
            if(spAdjust != 0)
            {
                characterManager->UpdateSoulPoints(client, spAdjust, true);
            }

            if(coinAdjust != 0)
            {
                characterManager->UpdateCoinTotal(client, coinAdjust, true);
            }

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
                    failed |= !characterManager->LearnSkill(client,
                        demonEntityID, (uint32_t)skillID);
                }
            }

            for(int32_t pluginID : pluginIDs)
            {
                failed |= !characterManager->AddPlugin(client,
                    (uint16_t)pluginID);
            }

            for(uint16_t valuableID : oneTimeValuables)
            {
                failed |= characterManager->AddPlugin(client, valuableID);
            }

            if(!failed)
            {
                bool updated = false;
                for(auto& pair : cooldowns)
                {
                    if(pair.second)
                    {
                        character->SetActionCooldowns(pair.first, pair.second);
                        updated = true;
                    }
                }

                if(updated)
                {
                    server->GetWorldDatabase()->QueueUpdate(character,
                        state->GetAccountUID());
                }
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BARTER);
    reply.WriteS8(failed ? -1 : 0); // No special error codes for this
    reply.WriteU16Little(barterID);

    client->SendPacket(reply);

    // Certain types need to force the event to end so pre-barter checks can
    // run again
    if(!failed && (oneTimeValuables.size() > 0 || cooldowns.size() > 0))
    {
        server->GetEventManager()->HandleEvent(client, nullptr);
    }
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
