/**
 * @file server/channel/src/packets/game/MitamaReset.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to reset a mitama reunion growth path.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "TokuseiManager.h"

using namespace channel;

void HandleMitamaReset(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int8_t reunionIdx)
{
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();
    auto demonData = dState->GetDevilData();

    bool isMitama = characterManager->IsMitamaDemon(demonData);

    bool success = demon && isMitama && reunionIdx >= 0 && reunionIdx < 12;
    if(success)
    {
        uint8_t cleared = 0;

        auto mReunion = demon->GetMitamaReunion();
        for(size_t i = (size_t)(8 * reunionIdx);
            i < (size_t)(8 * (reunionIdx + 1)); i++)
        {
            if(mReunion[i])
            {
                mReunion[i] = 0;
                cleared++;
            }
        }

        // Pay the cost
        success = characterManager->PayMacca(client,
            (uint64_t)(cleared * 30000));

        if(success)
        {
            // Clear type and save
            demon->SetMitamaReunion(mReunion);

            auto dbChanges = libcomp::DatabaseChangeSet::Create(state
                ->GetAccountUID());
            dbChanges->Update(demon);

            server->GetWorldDatabase()->QueueChangeSet(dbChanges);

            dState->UpdateDemonState(definitionManager);
            server->GetTokuseiManager()->Recalculate(cState, true,
                std::set<int32_t>{ dState->GetEntityID() });
            characterManager->RecalculateStats(dState, client);

            // If the current event is a menu, handle the "next" event,
            if(state->GetCurrentMenuShopID() != 0)
            {
                server->GetEventManager()->HandleResponse(client, -1);
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_MITAMA_RESET);
    reply.WriteS8(success ? 0 : -1);
    reply.WriteS8(reunionIdx);

    if(success)
    {
        characterManager->GetEntityStatsPacketData(reply,
            demon->GetCoreStats().Get(), dState, 1);
        reply.WriteS8(0);
    }

    client->SendPacket(reply);
}

bool Parsers::MitamaReset::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    int8_t reunionIdx = p.ReadS8();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);

    server->QueueWork(HandleMitamaReset, server, client, reunionIdx);

    return true;
}
