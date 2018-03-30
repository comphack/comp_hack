/**
 * @file server/channel/src/packets/game/TriFusionRewardAccept.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to accept tri-fusion success rewards.
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
#include <TriFusionHostSession.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::TriFusionRewardAccept::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    int8_t result = p.ReadS8();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto managerConnection = server->GetManagerConnection();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto tfSession = std::dynamic_pointer_cast<objects::TriFusionHostSession>(
        state->GetExchangeSession());

    bool success = false;
    if(tfSession)
    {
        tfSession->SetLocked(result == 1);
        success = true;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_TRIFUSION_REWARD_ACCEPT);
    reply.WriteS8(success ? 0 : -1);

    client->SendPacket(reply);

    if(success)
    {
        // Notify the rest of the session
        std::set<int32_t> participantIDs;
        participantIDs.insert(tfSession->GetSourceEntityID());
        for(auto pState : tfSession->GetGuests())
        {
            participantIDs.insert(pState->GetEntityID());
        }

        participantIDs.erase(cState->GetEntityID());

        std::list<std::shared_ptr<ChannelClientConnection>> pClients;
        for(int32_t pID : participantIDs)
        {
            auto pClient = managerConnection->GetEntityClient(pID,
                false);
            if(pClient)
            {
                pClients.push_back(pClient);
            }
        }

        if(pClients.size() > 0)
        {
            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TRIFUSION_REWARD_ACCEPTED);
            notify.WriteS32Little(cState->GetEntityID());
            notify.WriteS8(result);

            ChannelClientConnection::BroadcastPacket(pClients, notify);
        }

        if(result != 1)
        {
            // Back out to pre-demon set
            for(auto pClient : pClients)
            {
                auto pState = pClient->GetClientState();
                auto exchange = pState->GetExchangeSession();
                for(size_t i = 0; i < 4; i++)
                {
                    exchange->SetItems(i, NULLUUID);
                }
            }

            tfSession->SetDemons(0, NULLUUID);
            tfSession->SetDemons(1, NULLUUID);
            tfSession->SetDemons(2, NULLUUID);
        }
    }

    return true;
}
