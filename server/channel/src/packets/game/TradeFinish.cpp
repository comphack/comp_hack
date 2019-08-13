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
#include <Account.h>
#include <Character.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ClientState.h"
#include "ManagerConnection.h"

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
    auto exchangeSession = state->GetExchangeSession();

    auto otherCState = exchangeSession ? std::dynamic_pointer_cast<CharacterState>(
        exchangeSession->GetOtherCharacterState()) : nullptr;
    auto otherClient = otherCState ? server->GetManagerConnection()->GetEntityClient(
        otherCState->GetEntityID(), false) : nullptr;
    if(!otherClient)
    {
        characterManager->EndExchange(client);
        return true;
    }

    bool success = false;

    auto otherState = otherClient->GetClientState();
    auto otherSession = otherState->GetExchangeSession();
    if(otherSession)
    {
        success = true;

        exchangeSession->SetFinished(true);

        libcomp::Packet notify;
        notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_FINISHED);

        otherClient->SendPacket(notify);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_FINISH);
    reply.WriteS32Little(success ? 0 : -1);

    client->SendPacket(reply);

    // Wait on the other player
    if(!otherSession->GetFinished())
    {
        return true;
    }

    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();

    auto otherCharacter = otherCState->GetEntity();
    auto otherInventory = otherCharacter->GetItemBoxes(0).Get();

    auto freeSlots = characterManager->GetFreeSlots(client,
        inventory);

    std::vector<std::shared_ptr<objects::Item>> tradeItems;
    for(auto tradeItem : exchangeSession->GetItems())
    {
        if(!tradeItem.IsNull())
        {
            tradeItems.push_back(tradeItem.Get());
            freeSlots.insert((size_t)tradeItem->GetBoxSlot());
        }
    }

    auto otherFreeSlots = characterManager->GetFreeSlots(otherClient,
        otherInventory);

    std::vector<std::shared_ptr<objects::Item>> otherTradeItems;
    for(auto tradeItem : otherSession->GetItems())
    {
        if(!tradeItem.IsNull())
        {
            otherTradeItems.push_back(tradeItem.Get());
            otherFreeSlots.insert((size_t)tradeItem->GetBoxSlot());
        }
    }

    if(tradeItems.size() > otherFreeSlots.size())
    {
        characterManager->EndExchange(client, 3);
        characterManager->EndExchange(otherClient, 2);
        return true;
    }
    else if(otherTradeItems.size() > freeSlots.size())
    {
        characterManager->EndExchange(client, 2);
        characterManager->EndExchange(otherClient, 3);
        return true;
    }

    // Trade is valid so process it

    // Step 1: Unequip all equipment being traded
    std::list<uint16_t> updatedSlots;
    for(auto item : tradeItems)
    {
        characterManager->UnequipItem(client, item);

        updatedSlots.push_back((uint16_t)item->GetBoxSlot());
        inventory->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
    }

    std::list<uint16_t> otherUpdatedSlots;
    for(auto item : otherTradeItems)
    {
        characterManager->UnequipItem(otherClient, item);

        otherUpdatedSlots.push_back((uint16_t)item->GetBoxSlot());
        otherInventory->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
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
            item->SetItemBox(inventory->GetUUID());
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
            item->SetItemBox(otherInventory->GetUUID());
            otherUpdatedSlots.push_back((uint16_t)i);
            changes->Update(item);
        }
    }

    // Step 3: Handle transaction
    auto db = server->GetWorldDatabase();
    if(!db->ProcessChangeSet(changes))
    {
        LogTradeErrorMsg("Trade failed to save.\n");

        client->Close();
        otherClient->Close();
    }
    else
    {
        characterManager->SendItemBoxData(client, inventory,
            updatedSlots);
        characterManager->SendItemBoxData(otherClient, otherInventory,
            otherUpdatedSlots);

        characterManager->EndExchange(client, 0);
        characterManager->EndExchange(otherClient, 0);
    }

    return true;
}
