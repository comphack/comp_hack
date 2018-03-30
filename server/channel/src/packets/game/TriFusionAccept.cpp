/**
 * @file server/channel/src/packets/game/TriFusionAccept.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to accept (or reject) a tri-fusion.
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
#include <Demon.h>
#include <DemonBox.h>
#include <PlayerExchangeSession.h>
#include <TriFusionHostSession.h>

// channel Includes
#include "ChannelServer.h"
#include "FusionManager.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::TriFusionAccept::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 5)
    {
        return false;
    }

    int8_t result = p.ReadS8();
    int32_t unknown = p.ReadS32Little();    // Always 0
    (void)unknown;

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto managerConnection = server->GetManagerConnection();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto exchangeSession = state->GetExchangeSession();
    auto tfSession = std::dynamic_pointer_cast<objects::TriFusionHostSession>(
        exchangeSession);

    bool doFusion = false;
    std::array<int64_t, 3> demonIDs = { { 0, 0, 0 } };

    bool success = false;
    if(exchangeSession)
    {
        switch(exchangeSession->GetType())
        {
        case objects::PlayerExchangeSession::Type_t::TRIFUSION_GUEST:
            {
                auto otherCState = exchangeSession ? std::dynamic_pointer_cast<
                    CharacterState>(exchangeSession->GetOtherCharacterState()) : nullptr;
                auto otherClient = otherCState ? managerConnection->GetEntityClient(
                    otherCState->GetEntityID(), false) : nullptr;
                auto otherState = otherClient ? otherClient->GetClientState() : nullptr;
                tfSession = otherState ? std::dynamic_pointer_cast<
                    objects::TriFusionHostSession>(otherState->GetExchangeSession())
                    : nullptr;

                if(tfSession)
                {
                    exchangeSession->SetFinished(result == 1);
                    success = true;
                }
                else
                {
                    LOG_ERROR(libcomp::String("Player attempted to accept a TriFusion"
                        " but is not participating in one: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                }
            }
            break;
        case objects::PlayerExchangeSession::Type_t::TRIFUSION_HOST:
            if(tfSession)
            {
                // Ensure that everyone has accepted and all 3 demons
                // have been set
                exchangeSession->SetFinished(result == 1);
                success = true;

                if(result == 1)
                {
                    std::set<std::shared_ptr<objects::Character>> pCharacters;
                    for(auto d : tfSession->GetDemons())
                    {
                        auto dBox = d ? d->GetDemonBox().Get() : nullptr;
                        auto pCharacter = dBox
                            ? dBox->GetCharacter().Get() : nullptr;

                        if(pCharacter)
                        {
                            pCharacters.insert(pCharacter);
                        }
                        else
                        {
                            success = false;
                        }
                    }

                    demonIDs[0] = state->GetObjectID(
                        tfSession->GetDemons(0).GetUUID());
                    demonIDs[1] = state->GetObjectID(
                        tfSession->GetDemons(1).GetUUID());
                    demonIDs[2] = state->GetObjectID(
                        tfSession->GetDemons(2).GetUUID());

                    if(success)
                    {
                        for(auto guest : tfSession->GetGuests())
                        {
                            auto pState = ClientState::GetEntityClientState(
                                guest->GetEntityID(), false);
                            auto pCharacter = pState
                                ? pState->GetCharacterState()->GetEntity() : nullptr;
                            if(pCharacter &&
                                pCharacters.find(pCharacter) != pCharacters.end())
                            {
                                auto pExchange = pState->GetExchangeSession();
                                success &= pExchange && pExchange->GetFinished();
                            }
                        }

                        // If we're still good, the fusion is ready to go
                        doFusion = success;
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_TRIFUSION_ACCEPT);
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
                ChannelToClientPacketCode_t::PACKET_TRIFUSION_ACCEPTED);
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

    if(doFusion)
    {
        server->QueueWork([](const std::shared_ptr<ChannelServer> pServer,
            const std::shared_ptr<ChannelClientConnection> pClient,
            int64_t pDemonID1, int64_t pDemonID2, int64_t pDemonID3)
            {
                pServer->GetFusionManager()->HandleTriFusion(pClient, pDemonID1,
                    pDemonID2, pDemonID3, false);
            }, server, client, demonIDs[0], demonIDs[1], demonIDs[2]);
    }

    return true;
}
