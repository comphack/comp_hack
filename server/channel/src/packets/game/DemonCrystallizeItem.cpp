/**
 * @file server/channel/src/packets/game/DemonCrystallizeItem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update the item used for demon
 *  crystallization.
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

// object Includes
#include <Item.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::DemonCrystallizeItem::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    int64_t itemID = p.ReadS64Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto exchangeSession = state->GetExchangeSession();

    auto otherClient = exchangeSession &&
        exchangeSession->GetSourceEntityID() != cState->GetEntityID()
        ? server->GetManagerConnection()->GetEntityClient(
            exchangeSession->GetSourceEntityID(), false) : nullptr;

    bool success = false;
    std::list<int32_t> successRates;
    uint32_t itemType = 0;

    auto item = itemID != -1 ? std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)))
        : nullptr;

    if(exchangeSession && (itemID == -1 || item))
    {
        exchangeSession->SetItems(0, item);

        success = characterManager->GetSynthOutcome(otherClient ? otherClient->GetClientState()
            : state, exchangeSession, itemType, successRates);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_DEMON_CRYSTALLIZE_ITEM_UPDATE);
    reply.WriteS64Little(itemID);
    reply.WriteS32Little(successRates.front());
    reply.WriteU32Little(itemType);
    reply.WriteS32Little(success ? 0 : -1);

    client->SendPacket(reply);

    if(success && otherClient)
    {
        auto otherState = otherClient->GetClientState();
        int64_t otherItemID = item ? otherState->GetObjectID(item->GetUUID()) : -1;

        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_DEMON_CRYSTALLIZE_ITEM_UPDATED);
        notify.WriteS64Little(otherItemID);

        characterManager->GetItemDetailPacketData(notify, item);

        notify.WriteS32Little(successRates.front());
        notify.WriteU32Little(itemType);

        otherClient->SendPacket(notify);
    }

    return true;
}
