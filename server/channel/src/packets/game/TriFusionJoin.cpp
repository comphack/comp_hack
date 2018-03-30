/**
 * @file server/channel/src/packets/game/TriFusionJoin.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to join a tri-fusion session.
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
#include <DemonBox.h>
#include <Expertise.h>
#include <TriFusionHostSession.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::TriFusionJoin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    int8_t unknown = p.ReadS8();
    (void)unknown;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    auto partyClients = server->GetManagerConnection()
        ->GetPartyConnections(client, false, true);

    ClientState* tfSessionOwner;
    std::shared_ptr<objects::TriFusionHostSession> tfSession;
    for(auto pClient : partyClients)
    {
        auto pState = pClient->GetClientState();
        tfSession = std::dynamic_pointer_cast<
            objects::TriFusionHostSession>(pState->GetExchangeSession());
        if(tfSession)
        {
            tfSessionOwner = pState;
            break;
        }
    }

    if(tfSession)
    {
        std::set<int32_t> existingParticipants;
        existingParticipants.insert(tfSessionOwner->GetCharacterState()
            ->GetEntityID());
        for(auto pState : tfSession->GetGuests())
        {
            existingParticipants.insert(pState->GetEntityID());
        }

        auto guestSession = std::make_shared<objects::PlayerExchangeSession>();
        guestSession->SetSourceEntityID(state->GetCharacterState()->GetEntityID());
        guestSession->SetType(
            objects::PlayerExchangeSession::Type_t::TRIFUSION_GUEST);
        guestSession->SetOtherCharacterState(tfSessionOwner->GetCharacterState());

        state->SetExchangeSession(guestSession);

        std::list<std::shared_ptr<objects::Demon>> sourceDemons;
        for(auto d : cState->GetEntity()->GetCOMP()->GetDemons())
        {
            if(!d.IsNull())
            {
                sourceDemons.push_back(d.Get());
            }
        }

        // Send current participants to self
        for(auto pClient : partyClients)
        {
            auto pState = pClient->GetClientState();
            auto pCState = pState->GetCharacterState();

            if(existingParticipants.find(pCState->GetEntityID()) ==
                existingParticipants.end()) continue;

            std::list<std::shared_ptr<objects::Demon>> demons;
            for(auto d : pCState->GetEntity()->GetCOMP()->GetDemons())
            {
                if(!d.IsNull() && !d->GetLocked())
                {
                    demons.push_back(d.Get());
                }
            }

            auto exp = pCState->GetEntity()->GetExpertises(17);

            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TRIFUSION_PARTICIPANT);
            notify.WriteS32Little(pCState->GetEntityID());
            notify.WriteS32Little(exp ? exp->GetPoints() : 0);

            notify.WriteS8((int8_t)demons.size());
            for(auto d : demons)
            {
                auto cs = d->GetCoreStats().Get();

                int64_t objID = state->GetObjectID(d->GetUUID());
                if(objID == 0)
                {
                    objID = server->GetNextObjectID();
                    state->SetObjectID(d->GetUUID(), objID);
                }

                notify.WriteS64Little(objID);
                notify.WriteU32Little(d->GetType());
                notify.WriteS8(cs ? cs->GetLevel() : 0);
                notify.WriteU16Little(d->GetFamiliarity());

                std::list<uint32_t> skillIDs;
                for(uint32_t skillID : d->GetLearnedSkills())
                {
                    if(skillID != 0)
                    {
                        skillIDs.push_back(skillID);
                    }
                }

                notify.WriteS8((int8_t)skillIDs.size());
                for(uint32_t skillID : skillIDs)
                {
                    notify.WriteU32Little(skillID);
                }
            }

            client->QueuePacket(notify);

            exp = state->GetCharacterState()->GetEntity()
                ->GetExpertises(EXPERTISE_FUSION);

            // Notify existing participant of new participant
            notify.Clear();
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TRIFUSION_PARTICIPANT);
            notify.WriteS32Little(state->GetCharacterState()->GetEntityID());
            notify.WriteS32Little(exp ? exp->GetPoints() : 0);

            notify.WriteS8((int8_t)sourceDemons.size());
            for(auto d : sourceDemons)
            {
                auto cs = d->GetCoreStats().Get();

                int64_t objID = pState->GetObjectID(d->GetUUID());
                if(objID == 0)
                {
                    objID = server->GetNextObjectID();
                    pState->SetObjectID(d->GetUUID(), objID);
                }

                notify.WriteS64Little(objID);
                notify.WriteU32Little(d->GetType());
                notify.WriteS8(cs ? cs->GetLevel() : 0);
                notify.WriteU16Little(d->GetFamiliarity());

                std::list<uint32_t> skillIDs;
                for(uint32_t skillID : d->GetLearnedSkills())
                {
                    if(skillID != 0)
                    {
                        skillIDs.push_back(skillID);
                    }
                }

                notify.WriteS8((int8_t)skillIDs.size());
                for(uint32_t skillID : skillIDs)
                {
                    notify.WriteU32Little(skillID);
                }
            }

            pClient->SendPacket(notify);
        }

        tfSession->AppendGuests(state->GetCharacterState());
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRIFUSION_JOIN);
    reply.WriteS8(tfSession ? 0 : -1);

    client->SendPacket(reply);

    return true;
}
