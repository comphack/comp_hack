/**
 * @file server/channel/src/packets/game/TradeAddItem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to add an item to the trade.
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
#include <TradeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "ClientState.h"

using namespace channel;

bool Parsers::TradeAddItem::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 12)
    {
        return false;
    }

    int64_t itemID = p.ReadS64Little();
    int32_t slot = p.ReadS32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto tradeSession = state->GetTradeSession();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    bool cancel = false;
    if(!item || slot >= 30)
    {
        LOG_ERROR("Invalid item trade request.\n");
        cancel = true;
    }

    auto otherCState = std::dynamic_pointer_cast<CharacterState>(
        state->GetTradeSession()->GetOtherCharacterState());
    auto otherChar = otherCState != nullptr ? otherCState->GetEntity() : nullptr;
    auto otherClient = otherChar != nullptr ?
        server->GetManagerConnection()->GetClientConnection(
            otherChar->GetAccount()->GetUsername()) : nullptr;

    if(!otherClient)
    {
        LOG_ERROR("Invalid trade session.\n");
        cancel = true;
    }

    if(cancel)
    {
        characterManager->EndTrade(client);
        if(otherClient)
        {
            characterManager->EndTrade(otherClient);
        }

        return true;
    }

    tradeSession->SetItems((size_t)slot, item);

    auto otherState = otherClient->GetClientState();
    auto otherObjectID = otherState->GetObjectID(item->GetUUID());
    if(otherObjectID == 0)
    {
        otherObjectID = server->GetNextObjectID();
        otherState->SetObjectID(item->GetUUID(), otherObjectID);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_ADD_ITEM);
    reply.WriteS32Little(0);    //Unknown
    reply.WriteS64Little(itemID);
    reply.WriteS32Little(slot);
    reply.WriteS32Little(0);    //Unknown, seems to be the same as other packet

    client->SendPacket(reply);

    reply.Clear();
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_ADDED_ITEM);
    reply.WriteS32Little(slot);
    reply.WriteS64Little(otherObjectID);
    reply.WriteU32Little(item->GetType());
    reply.WriteU16Little(item->GetStackSize());
    reply.WriteU16Little(item->GetDurability());
    reply.WriteS8(item->GetMaxDurability());
    reply.WriteS16Little(item->GetTarot());
    reply.WriteS16Little(item->GetSoul());

    for(auto modSlot : item->GetModSlots())
    {
        reply.WriteU16Little(modSlot);
    }

    reply.WriteS32Little(0);    //Unknown

    auto basicEffect = item->GetBasicEffect();
    reply.WriteU32Little(basicEffect ? basicEffect
        : static_cast<uint32_t>(-1));

    auto specialEffect = item->GetSpecialEffect();
    reply.WriteU32Little(specialEffect ? specialEffect
        : static_cast<uint32_t>(-1));

    for(auto bonus : item->GetFuseBonuses())
    {
        reply.WriteS8(bonus);
    }

    reply.WriteS32Little(0);    //Unknown, seems to be the same as other packet

    otherClient->SendPacket(reply);

    return true;
}
