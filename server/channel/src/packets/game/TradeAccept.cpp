/**
 * @file server/channel/src/packets/game/TradeAccept.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to accept a trade request.
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

bool Parsers::TradeAccept::Parse(libcomp::ManagerPacket *pPacketManager,
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

    auto otherCState = std::dynamic_pointer_cast<CharacterState>(
        state->GetTradeSession()->GetOtherCharacterState());
    auto otherChar = otherCState != nullptr ? otherCState->GetEntity() : nullptr;
    auto otherClient = otherChar != nullptr ?
        server->GetManagerConnection()->GetClientConnection(
            otherChar->GetAccount()->GetUsername()) : nullptr;

    bool cancel = false;
    if(!otherClient)
    {
        cancel = true;
    }
    else
    {
        auto otherSession = otherClient->GetClientState()->GetTradeSession();
        if(otherSession->GetOtherCharacterState() != cState)
        {
            cancel = true;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_ACCEPT);

    if(cancel)
    {
        state->SetTradeSession(std::make_shared<objects::TradeSession>());

        // Rejected
        reply.WriteS32Little(-1);
        client->SendPacket(reply);
        return true;
    }

    // Accepted
    reply.WriteS32Little(0);

    characterManager->SetStatusIcon(otherClient, 8);

    libcomp::Packet reply2(reply);
    otherClient->SendPacket(reply);

    client->QueuePacket(reply2);
    characterManager->SetStatusIcon(client, 8);

    return true;
}
