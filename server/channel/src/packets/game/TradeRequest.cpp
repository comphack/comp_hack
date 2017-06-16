/**
 * @file server/channel/src/packets/game/TradeRequest.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to request a trade with another player.
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

bool Parsers::TradeRequest::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    uint32_t targetEntityID = p.ReadU32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_REQUEST);

    std::shared_ptr<CharacterState> otherCState;
    std::shared_ptr<ChannelClientConnection> otherClient;
    auto otherState = ClientState::GetEntityClientState((int32_t)targetEntityID);
    if(otherState != nullptr)
    {
        otherCState = otherState->GetCharacterState();
        auto otherChar = otherCState != nullptr ? otherCState->GetEntity() : nullptr;
        otherClient = otherChar != nullptr ?
            server->GetManagerConnection()->GetClientConnection(
                otherChar->GetAccount()->GetUsername()) : nullptr;
    }

    bool cancel = false;
    if(!otherClient)
    {
        cancel = true;
    }
    else
    {
        auto otherSession = otherClient->GetClientState()->GetTradeSession();
        if(otherSession->GetOtherCharacterState() != nullptr)
        {
            cancel = true;
        }
    }

    if(cancel)
    {
        reply.WriteS32Little(-1);
        client->SendPacket(reply);
        return true;
    }

    // Set the trade session info
    auto tradeSession = state->GetTradeSession();
    tradeSession->SetOtherCharacterState(otherCState);

    auto otherTradeSession = otherState->GetTradeSession();
    otherTradeSession->SetOtherCharacterState(cState);

    reply.WriteS32Little(0);

    libcomp::Packet p2;
    p2.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_REQUESTED);
    p2.WriteS32Little(cState->GetEntityID());

    otherClient->SendPacket(p2);

    client->SendPacket(reply);

    return true;
}
