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
#include <PlayerExchangeSession.h>

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
    auto exchangeSession = state->GetExchangeSession();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    bool cancel = false;
    if(!item || slot >= 30)
    {
        LOG_ERROR("Invalid item trade request.\n");
        cancel = true;
    }

    auto otherCState = exchangeSession ? std::dynamic_pointer_cast<CharacterState>(
        exchangeSession->GetOtherCharacterState()) : nullptr;
    auto otherClient = otherCState ? server->GetManagerConnection()->GetEntityClient(
        otherCState->GetEntityID(), false) : nullptr;

    if(!otherClient)
    {
        LOG_ERROR("Invalid trade session.\n");
        cancel = true;
    }

    if(cancel)
    {
        characterManager->EndExchange(client);
        if(otherClient)
        {
            characterManager->EndExchange(otherClient);
        }

        return true;
    }

    exchangeSession->SetItems((size_t)slot, item);

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

    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_ADDED_ITEM);
    notify.WriteS32Little(slot);
    notify.WriteS64Little(otherObjectID);

    characterManager->GetItemDetailPacketData(notify, item);

    notify.WriteS32Little(0);    //Unknown, seems to be the same as other packet

    otherClient->SendPacket(notify);

    return true;
}
