/**
 * @file server/channel/src/packets/game/TradeFinish.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to finish the trade.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

// object Includes
#include <Account.h>
#include <Character.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <TradeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "ClientState.h"

using namespace channel;

bool Parsers::TradeFinish::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }
    
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto tradeSession = state->GetTradeSession();

    auto otherCState = std::dynamic_pointer_cast<CharacterState>(
        state->GetTradeSession()->GetOtherCharacterState());
    auto otherChar = otherCState != nullptr ? otherCState->GetEntity() : nullptr;
    auto otherClient = otherChar != nullptr ?
        server->GetManagerConnection()->GetClientConnection(
            otherChar->GetAccount()->GetUsername()) : nullptr;
    if(!otherClient)
    {
        characterManager->EndTrade(client);
        return true;
    }

    auto otherState = otherClient->GetClientState();
    auto otherTradeSession = otherState->GetTradeSession();

    // Nothing wrong with the trade setup
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_FINISH);
    reply.WriteS32Little(0);
    client->SendPacket(reply);

    reply.Clear();
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_FINISHED);
    otherClient->SendPacket(reply);

    tradeSession->SetFinished(true);

    // Wait on the other player
    if(!otherTradeSession->GetFinished())
    {
        return true;
    }

    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();

    auto otherCharacter = otherCState->GetEntity();
    auto otherInventory = otherCharacter->GetItemBoxes(0).Get();

    std::set<size_t> freeSlots;
    for(size_t i = 0; i < inventory->ItemsCount(); i++)
    {
        if(inventory->GetItems(i).IsNull())
        {
            freeSlots.insert(i);
        }
    }

    std::vector<std::shared_ptr<objects::Item>> tradeItems;
    for(auto tradeItem : tradeSession->GetItems())
    {
        if(!tradeItem.IsNull())
        {
            tradeItems.push_back(tradeItem.Get());
            freeSlots.insert((size_t)tradeItem->GetBoxSlot());
        }
    }

    std::set<size_t> otherFreeSlots;
    for(size_t i = 0; i < otherInventory->ItemsCount(); i++)
    {
        if(otherInventory->GetItems(i).IsNull())
        {
            otherFreeSlots.insert(i);
        }
    }

    std::vector<std::shared_ptr<objects::Item>> otherTradeItems;
    for(auto tradeItem : otherTradeSession->GetItems())
    {
        if(!tradeItem.IsNull())
        {
            otherTradeItems.push_back(tradeItem.Get());
            otherFreeSlots.insert((size_t)tradeItem->GetBoxSlot());
        }
    }

    if(tradeItems.size() > otherFreeSlots.size())
    {
        characterManager->EndTrade(client, 3);
        characterManager->EndTrade(otherClient, 2);
        return true;
    }
    else if(otherTradeItems.size() > freeSlots.size())
    {
        characterManager->EndTrade(client, 2);
        characterManager->EndTrade(otherClient, 3);
        return true;
    }

    // Trade is valid so process it

    // Step 1: Unequip all equipment being traded
    std::list<uint16_t> updatedSlots;
    for(auto item : tradeItems)
    {
        characterManager->UnequipItem(client, item);

        auto box = item->GetItemBox();
        updatedSlots.push_back((uint16_t)item->GetBoxSlot());
        box->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
    }

    std::list<uint16_t> otherUpdatedSlots;
    for(auto item : otherTradeItems)
    {
        characterManager->UnequipItem(otherClient, item);

        auto box = item->GetItemBox();
        otherUpdatedSlots.push_back((uint16_t)item->GetBoxSlot());
        box->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
    }

    // Step 2: Transfer items and prepare changes
    auto changes = libcomp::DatabaseChangeSet::Create();

    changes->Update(inventory);
    for(size_t i = 0; i < inventory->ItemsCount()
        && otherTradeItems.size() > 0; i++)
    {
        if(freeSlots.find(i) != freeSlots.end())
        {
            auto item = otherTradeItems.front();
            otherTradeItems.erase(otherTradeItems.begin());
            inventory->SetItems(i, item);
            item->SetBoxSlot((int8_t)i);
            item->SetItemBox(inventory);
            updatedSlots.push_back((uint16_t)i);
            changes->Update(item);
        }
    }

    changes->Update(otherInventory);
    for(size_t i = 0; i < otherInventory->ItemsCount()
        && tradeItems.size() > 0; i++)
    {
        if(otherFreeSlots.find(i) != otherFreeSlots.end())
        {
            auto item = tradeItems.front();
            tradeItems.erase(tradeItems.begin());
            otherInventory->SetItems(i, item);
            item->SetBoxSlot((int8_t)i);
            item->SetItemBox(otherInventory);
            otherUpdatedSlots.push_back((uint16_t)i);
            changes->Update(item);
        }
    }

    // Step 3: Handle transaction
    auto db = server->GetWorldDatabase();
    if(!db->ProcessChangeSet(changes))
    {
        LOG_ERROR("Trade failed to save.\n");
        state->SetLogoutSave(true);
        otherState->SetLogoutSave(true);
        client->Close();
        otherClient->Close();
    }
    else
    {
        characterManager->SendItemBoxData(client, inventory,
            updatedSlots);
        characterManager->SendItemBoxData(otherClient, otherInventory,
            otherUpdatedSlots);

        characterManager->EndTrade(client, 0);
        characterManager->EndTrade(otherClient, 0);
    }

    return true;
}
