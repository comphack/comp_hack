/**
 * @file server/channel/src/packets/game/TradeLock.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to lock the trade for acceptance.
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
#include <TradeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "ClientState.h"

using namespace channel;

bool Parsers::TradeLock::Parse(libcomp::ManagerPacket *pPacketManager,
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

    auto otherTradeSession = otherClient->GetClientState()->GetTradeSession();
    tradeSession->SetLocked(true);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_LOCK);
    reply.WriteS32Little(0);    // Successfully locked
    client->SendPacket(reply);

    reply.Clear();
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_LOCKED);

    otherClient->SendPacket(reply);

    return true;
}
