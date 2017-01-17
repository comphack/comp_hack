/**
 * @file server/lobby/src/packets/SetChannelInfo.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Parser to handle detailing a channel for the lobby.
 *
 * This file is part of the Lobby Server (lobby).
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
#include <Decrypt.h>
#include <InternalConnection.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// lobby Includes
#include "LobbyServer.h"

using namespace lobby;

bool Parsers::SetChannelInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 2)
    {
        return false;
    }

    auto action = static_cast<InternalPacketAction_t>(p.ReadU8());
    auto channelID = p.ReadU8();

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto conn = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);
    auto world = server->GetWorldByConnection(conn);

    auto svr = world->GetChannelByID(channelID);
    if(nullptr == svr)
    {
        auto db = world->GetWorldDatabase();

        svr = objects::RegisteredChannel::LoadRegisteredChannelByID(db, channelID);
    }

    if(InternalPacketAction_t::PACKET_ACTION_REMOVE == action)
    {
        world->RemoveChannelByID(svr->GetID());

        server->QueueWork([](std::shared_ptr<LobbyServer> lobbyServer,
            int8_t worldID, int8_t channel)
            {
                auto accountManager = lobbyServer->GetAccountManager();
                auto usernames = accountManager->LogoutUsersInWorld(worldID, channel);

                if(usernames.size() > 0)
                {
                    LOG_WARNING(libcomp::String("%1 user(s) forcefully logged out"
                        " from channel %2 on world %3.\n").Arg(usernames.size())
                        .Arg(channel).Arg(worldID));
                }
            }, server, world->GetRegisteredWorld()->GetID(), channelID);
    }
    else
    {
        LOG_DEBUG(libcomp::String("Updating Channel Server: (%1) %2\n")
            .Arg(svr->GetID()).Arg(svr->GetName()));
        world->RegisterChannel(svr);
    }

    return true;
}
