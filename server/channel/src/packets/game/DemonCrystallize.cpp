/**
 * @file server/channel/src/packets/game/DemonCrystallize.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to perform a demon crystallization.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>

// object Includes
#include <DemonBox.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiDevilCrystalData.h>
#include <MiDevilData.h>
#include <MiEnchantData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiPossessionData.h>
#include <MiUnionData.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::DemonCrystallize::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto exchangeSession = state->GetExchangeSession();

    EntrustErrorCodes_t responseCode = EntrustErrorCodes_t::SUCCESS;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_CRYSTALLIZE);
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
    auto targetCState = targetState->GetCharacterState();
    auto targetDState = targetState->GetDemonState();
    auto targetDemon = targetDState->GetEntity();
    auto targetInventory = targetCState->GetEntity()->GetItemBoxes(0).Get();

    auto demonData = targetDemon
        ? definitionManager->GetDevilData(targetDemon->GetType()) : nullptr;
    auto enchantData = demonData ? definitionManager->GetEnchantDataByDemonID(
        demonData->GetUnionData()->GetBaseDemonID()) : nullptr;

    // If any of the core parts are missing, stop here
    if(!targetDemon || !enchantData)
    {
        responseCode = EntrustErrorCodes_t::SYSTEM_ERROR;
    }
    else if(!targetDState->IsAlive())
    {
        responseCode = EntrustErrorCodes_t::INVALID_DEMON_STATE;
    }
    else if(characterManager->GetFamiliarityRank(
        targetDemon->GetFamiliarity()) < 3)
    {
        /// @todo: include reunion demons
        responseCode = EntrustErrorCodes_t::INVALID_DEMON_TARGET;
    }

    // Stop if any errors have already occurred
    if(responseCode != EntrustErrorCodes_t::SUCCESS)
    {
        reply.WriteS32Little((int32_t)responseCode);

        client->SendPacket(reply);

        characterManager->EndExchange(client, 0);

        if(otherClient)
        {
            characterManager->EndExchange(otherClient, 0);
        }

        return true;
    }

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

    auto useItem = exchangeSession->GetItems(0).Get();

    // Find the existing item that will have its stack increased
    // or generate a new one. Fail if theres not enough space
    std::shared_ptr<objects::Item> updateItem;
    if(responseCode == EntrustErrorCodes_t::SUCCESS)
    {
        uint32_t itemType = enchantData->GetDevilCrystal()->GetItemID();
        auto itemData = server->GetDefinitionManager()->GetItemData(itemType);
        for(auto existingItem : characterManager->GetExistingItems(
            targetCState->GetEntity(), itemType))
        {
            if(existingItem->GetStackSize() <
                itemData->GetPossession()->GetStackSize())
            {
                updateItem = existingItem;
                break;
            }
        }

        // No existing item found, generate a new one but dont add to
        // the inventory until later
        if(!updateItem)
        {
            updateItem = characterManager->GenerateItem(itemType, 0);

            int8_t crystalTargetSlot = -1;
            for(size_t i = 0; i < 50; i++)
            {
                if(targetInventory->GetItems(i).IsNull())
                {
                    crystalTargetSlot = (int8_t)i;
                    break;
                }
            }

            if(useItem && useItem->GetStackSize() == 1 &&
                (crystalTargetSlot == -1 || useItem->GetBoxSlot() < crystalTargetSlot))
            {
                crystalTargetSlot = useItem->GetBoxSlot();
            }

            if(crystalTargetSlot == -1)
            {
                // If any rewards will be given out on success, ensure that there will
                // be a free slot before continuing
                if(rewards.find(1) != rewards.end() || rewards.find(2) != rewards.end())
                {
                    // Determine slot upon success
                    updateItem->SetBoxSlot(-1);
                }
                else
                {
                    responseCode = EntrustErrorCodes_t::INVENTORY_SPACE_NEEDED;
                }
            }
            else
            {
                updateItem->SetBoxSlot(crystalTargetSlot);
            }
        }
    }

    // If there are any rewards, check how much inventory space is free
    std::list<int8_t> sourceInventoryFree;
    if(responseCode == EntrustErrorCodes_t::SUCCESS && rewards.size() > 0)
    {
        auto inventory = cState->GetEntity()->GetItemBoxes(0).Get();
        for(size_t i = 0; i < 50; i++)
        {
            if(inventory->GetItems(i).IsNull())
            {
                sourceInventoryFree.push_back((int8_t)i);
            }
        }

        size_t successRewardCount = rewards[0].size() + rewards[1].size();
        size_t failRewardCount = rewards[1].size() + rewards[2].size();

        // Stop here if there is not enough space for a success or failure
        if(sourceInventoryFree.size() < successRewardCount ||
            sourceInventoryFree.size() < failRewardCount)
        {
            responseCode = EntrustErrorCodes_t::INVENTORY_SPACE_NEEDED;
        }
    }

    bool success = false;
    std::list<int32_t> successRates;
    uint32_t itemType = 0;

    if(responseCode == EntrustErrorCodes_t::SUCCESS)
    {
        if(characterManager->GetSynthOutcome(state, exchangeSession,
            itemType, successRates))
        {
            success = successRates.front() > 0 &&
                RNG(int32_t, 0, 100) <= successRates.front();
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
        std::list<std::shared_ptr<ChannelClientConnection>> clients;
        clients.push_back(client);

        if(otherClient)
        {
            clients.push_back(otherClient);
        }

        libcomp::Packet notify;
        notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_CRYSTALLIZED);
        notify.WriteS32Little(cState->GetEntityID());
        notify.WriteS32Little(targetCState->GetEntityID());
        notify.WriteS32Little(targetDState->GetEntityID());

        auto dbChanges = libcomp::DatabaseChangeSet::Create();

        if(success)
        {
            int8_t slot = targetDemon->GetBoxSlot();
            auto box = targetDemon->GetDemonBox().Get();

            characterManager->StoreDemon(targetClient, true, 15);
            characterManager->DeleteDemon(targetDemon, dbChanges);
            characterManager->SendDemonBoxData(targetClient, box->GetBoxID(),
                { slot });

            notify.WriteS32Little(0);
        }
        else
        {
            notify.WriteS32Little(-1);
        }

        ChannelClientConnection::BroadcastPacket(clients, notify);

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

                    int8_t openSlot = sourceInventoryFree.front();
                    sourceInventoryFree.erase(sourceInventoryFree.begin());

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

        if(!success)
        {
            // Failure results in lowered familiarity
            int32_t newFam = (int32_t)(targetDemon->GetFamiliarity() -
                floor((float)targetDemon->GetFamiliarity() / 20.f));
            characterManager->UpdateFamiliarity(targetClient, newFam);
        }
        else
        {
            if(updateItem->GetStackSize() == 0)
            {
                // Item is new
                updateItem->SetStackSize(1);

                // Get the earliest slot free now that rewards have
                // been sent
                int8_t slot = updateItem->GetBoxSlot();
                if(rewards.size() > 0)
                {
                    for(size_t i = 0; i < 50; i++)
                    {
                        if(targetInventory->GetItems(i).IsNull() &&
                            (slot == -1 || slot >(int8_t)i))
                        {
                            slot = (int8_t)i;
                        }
                    }

                    updateItem->SetBoxSlot(slot);
                }

                updateItem->SetItemBox(targetInventory);

                targetInventory->SetItems((size_t)slot, updateItem);

                dbChanges->Insert(updateItem);
            }
            else
            {
                // Increase stack
                updateItem->SetStackSize((uint16_t)(
                    updateItem->GetStackSize() + 1));

                dbChanges->Update(updateItem);
            }

            targetSlots.push_back((uint16_t)updateItem->GetBoxSlot());
        }

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

        if(targetSlots.size() > 0)
        {
            dbChanges->Update(targetInventory);

            characterManager->SendItemBoxData(targetClient, targetInventory,
                targetSlots);
        }

        if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
        {
            LOG_ERROR("Crystallize result failed to save, disconnecting"
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
