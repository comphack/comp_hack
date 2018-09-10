/**
 * @file server/channel/src/packets/internal/TeamUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Parser to handle all team focused actions between the world
 *  and the channel.
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
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <Team.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"
#include "TokuseiManager.h"

using namespace channel;

bool Parsers::TeamUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    if(p.Size() < 3)
    {
        LOG_ERROR("Invalid response received for TeamUpdate.\n");
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());

    bool connectionsFound = false;
    uint8_t mode = p.ReadU8();

    auto clients = server->GetManagerConnection()
        ->GatherWorldTargetClients(p, connectionsFound);
    if(!connectionsFound)
    {
        LOG_ERROR("Connections not found for TeamUpdate.\n");
        return false;
    }

    switch((InternalPacketAction_t)mode)
    {
    case InternalPacketAction_t::PACKET_ACTION_UPDATE:
        {
            // Team updated
            int32_t teamID = p.ReadS32Little();
            uint8_t exists = p.ReadU8() == 1;

            auto team = exists
                ? std::make_shared<objects::Team>() : nullptr;
            if(team && !team->LoadPacket(p))
            {
                return false;
            }

            for(auto client : clients)
            {
                auto state = client->GetClientState();
                if(team && team->MemberIDsContains(state->GetWorldCID()))
                {
                    // Adding/updating
                    state->GetAccountLogin()->GetCharacterLogin()
                        ->SetTeamID(teamID);
                    state->SetTeam(team);
                }
                else if(state->GetTeamID() == teamID)
                {
                    // Removing
                    state->GetAccountLogin()->GetCharacterLogin()
                        ->SetTeamID(0);
                    state->SetTeam(nullptr);
                }
            }
        }
        break;
    default:
        break;
    }

    return true;
}
