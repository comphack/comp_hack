/**
 * @file server/channel/src/packets/game/Enchant.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to perform an enchantment.
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
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// object Includes
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::Enchant::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    int8_t choice = p.ReadS8();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto exchangeSession = state->GetExchangeSession();

    EntrustErrorCodes_t responseCode = EntrustErrorCodes_t::SUCCESS;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENCHANT);
    if(!exchangeSession)
    {
        reply.WriteS32Little((int32_t)EntrustErrorCodes_t::INVALID_CHAR_STATE);

        client->SendPacket(reply);

        return true;
    }

    int32_t otherEntityID = exchangeSession->GetOtherCharacterState()->GetEntityID();
    auto otherClient = otherEntityID != cState->GetEntityID()
        ? server->GetManagerConnection()->GetEntityClient(otherEntityID, false) : nullptr;
    auto targetClient = otherClient ? otherClient : client;
    auto targetState = targetClient->GetClientState();

    // Sort rewards by outcome types: success (0), any (1), failure (2)
    std::unordered_map<uint8_t, std::list<std::shared_ptr<objects::Item>>> rewards;
    for(size_t i = 10; i < 22; i++)
    {
        auto reward = exchangeSession->GetItems(i).Get();
        if(reward)
        {
            // If the item cannot be traded, error here
            auto itemData = definitionManager->GetItemData(reward->GetType());
            if((itemData->GetBasic()->GetFlags() & 0x01) == 0)
            {
                responseCode = EntrustErrorCodes_t::NONTRADE_ITEMS;
                break;
            }

            uint8_t outcomeType = 0;
            if(i >= 18)
            {
                outcomeType = 2;
            }
            else if(i >= 14)
            {
                outcomeType = 1;
            }

            rewards[outcomeType].push_back(reward);
        }
    }

    // If there are any rewards, check how much inventory space is free
    std::list<int8_t> inventoryFree;
    if(responseCode == EntrustErrorCodes_t::SUCCESS && rewards.size() > 0)
    {
        auto inventory = cState->GetEntity()->GetItemBoxes(0).Get();
        for(size_t i = 0; i < 50; i++)
        {
            if(inventory->GetItems(i).IsNull())
            {
                inventoryFree.push_back((int8_t)i);
            }
        }

        size_t successRewardCount = rewards[0].size() + rewards[1].size();
        size_t failRewardCount = rewards[1].size() + rewards[2].size();

        // Stop here if there is not enough space for a success or failure
        if(inventoryFree.size() < successRewardCount ||
            inventoryFree.size() < failRewardCount)
        {
            responseCode = EntrustErrorCodes_t::INVENTORY_SPACE_NEEDED;
        }
    }

    // If no error has occurred yet, go forward with the enchantment
    bool success = false;
    int16_t effectID = 0;
    std::list<int32_t> successRates;
    uint32_t specialEnchantItemType = static_cast<uint32_t>(-1);

    auto inputItem = exchangeSession->GetItems(0).Get();

    std::shared_ptr<objects::Item> updateItem;
    if(responseCode == EntrustErrorCodes_t::SUCCESS)
    {
        if(characterManager->GetSynthOutcome(state, exchangeSession,
            specialEnchantItemType, successRates, &effectID))
        {
            int32_t successRate = choice == 0
                ? successRates.front() : successRates.back();

            if(successRate > 0 && RNG(int32_t, 0, 100) <= successRate)
            {
                if(choice == 0)
                {
                    // Enchant existing item
                    updateItem = inputItem;

                    switch(exchangeSession->GetType())
                    {
                    case objects::PlayerExchangeSession::Type_t::ENCHANT_TAROT:
                        updateItem->SetTarot(effectID);
                        success = true;
                        break;
                    case objects::PlayerExchangeSession::Type_t::ENCHANT_SOUL:
                        updateItem->SetSoul(effectID);
                        success = true;
                        break;
                    default:
                        responseCode = EntrustErrorCodes_t::SYSTEM_ERROR;
                        break;
                    }
                }
                else if(specialEnchantItemType)
                {
                    // Item becomes new item
                    updateItem = characterManager->GenerateItem(specialEnchantItemType,
                        1);
                    updateItem->SetBoxSlot(inputItem->GetBoxSlot());
                    success = true;
                }
                else
                {
                    responseCode = EntrustErrorCodes_t::SYSTEM_ERROR;
                }
            }
        }
        else
        {
            responseCode = EntrustErrorCodes_t::SYSTEM_ERROR;
        }
    }

    reply.WriteS32Little((int32_t)responseCode);

    client->SendPacket(reply);

    if(responseCode == EntrustErrorCodes_t::SUCCESS)
    {
        auto targetCState = targetState->GetCharacterState();
        auto targetInventory = targetCState->GetEntity()->GetItemBoxes(0).Get();

        std::list<std::shared_ptr<ChannelClientConnection>> clients;
        clients.push_back(client);

        if(otherClient)
        {
            clients.push_back(otherClient);
        }

        libcomp::Packet notify;
        notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENCHANTED);
        notify.WriteS32Little(cState->GetEntityID());
        notify.WriteS32Little(targetCState->GetEntityID());
        notify.WriteS32Little(success ? 0 : -1);

        ChannelClientConnection::BroadcastPacket(clients, notify);

        auto dbChanges = libcomp::DatabaseChangeSet::Create();

        std::list<uint16_t> targetSlots;
        if(rewards.size() > 0)
        {
            auto sourceInventory = cState->GetEntity()
                ->GetItemBoxes(0).Get();

            std::list<uint16_t> sourceSlots;
            for(uint8_t rewardGroup : success ? std::set<uint8_t>{ 0, 1 } :
                std::set<uint8_t>{ 1, 2 })
            {
                for(auto reward : rewards[rewardGroup])
                {
                    // Make sure its not equipped
                    characterManager->UnequipItem(client, reward);

                    // Remove it from the target
                    targetInventory->SetItems((size_t)reward->GetBoxSlot(),
                        NULLUUID);
                    targetSlots.push_back((uint16_t)reward->GetBoxSlot());

                    int8_t openSlot = inventoryFree.front();
                    inventoryFree.erase(inventoryFree.begin());

                    // Give it to the source
                    sourceInventory->SetItems((size_t)openSlot,
                        reward);
                    reward->SetItemBox(sourceInventory);
                    reward->SetBoxSlot(openSlot);

                    sourceSlots.push_back((uint16_t)openSlot);

                    dbChanges->Update(reward);
                }
            }

            if(sourceSlots.size() > 0)
            {
                dbChanges->Update(sourceInventory);

                characterManager->SendItemBoxData(client, sourceInventory,
                    sourceSlots);
            }
        }

        if(updateItem)
        {
            if(!success)
            {
                /// @todo: drop durability
            }

            targetSlots.push_back((uint16_t)updateItem->GetBoxSlot());

            if(updateItem->GetItemBox().IsNull())
            {
                // Item is new, replace old one
                auto box = inputItem->GetItemBox().Get();
                updateItem->SetItemBox(box);
                box->SetItems((size_t)updateItem->GetBoxSlot(), updateItem);

                dbChanges->Delete(inputItem);
                dbChanges->Insert(updateItem);
                dbChanges->Update(box);
            }
            else
            {
                dbChanges->Update(updateItem);
            }
        }

        // Determine which items were consumed
        auto crystal = exchangeSession->GetItems(1).Get();
        auto mirror = exchangeSession->GetItems(2).Get();

        std::list<std::shared_ptr<objects::Item>> useItems = { mirror };
        if(success)
        {
            useItems.push_back(crystal);
        }
        else if(exchangeSession->GetType() ==
            objects::PlayerExchangeSession::Type_t::ENCHANT_SOUL)
        {
            // Check if a mirror exists that prevents removal of the crystal
            // on failure
            auto it = SVR_CONST.SYNTH_ADJUSTMENTS.find(
                mirror ? mirror->GetType() : 0);
            if(it == SVR_CONST.SYNTH_ADJUSTMENTS.end() ||
                it->second[0] != 0 || it->second[2] != 1)
            {
                useItems.push_back(crystal);
            }
        }

        for(auto useItem : useItems)
        {
            if(useItem)
            {
                if(useItem->GetStackSize() == 1)
                {
                    targetInventory->SetItems(
                        (size_t)useItem->GetBoxSlot(), NULLUUID);

                    dbChanges->Delete(useItem);
                }
                else
                {
                    useItem->SetStackSize((uint16_t)(
                        useItem->GetStackSize() - 1));

                    dbChanges->Update(useItem);
                }

                targetSlots.push_back((uint16_t)useItem->GetBoxSlot());
            }
        }

        if(targetSlots.size() > 0)
        {
            dbChanges->Update(targetInventory);

            characterManager->SendItemBoxData(targetClient, targetInventory,
                targetSlots);
        }

        if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
        {
            LOG_ERROR("Enchant result failed to save, disconnecting"
                " player(s)\n");
            state->SetLogoutSave(false);
            client->Close();

            if(targetClient)
            {
                targetState->SetLogoutSave(false);
                targetClient->Close();
            }

            return true;
        }
    }

    // Lastly end the exchange
    characterManager->EndExchange(client, 0);

    if(otherClient)
    {
        characterManager->EndExchange(otherClient, 0);
    }

    return true;
}
