/**
 * @file server/channel/src/packets/game/BlacklistUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to add or remove names from the blacklist.
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
#include <AccountWorldData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::BlacklistUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 11)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto worldData = state->GetAccountWorldData().Get();

    int32_t requestID = p.ReadS32Little();  // Increments each time

    bool isDelete = p.ReadU8() == 1;
    int32_t nameCount = p.ReadS32Little();

    std::list<libcomp::String> names;
    for(int32_t i = 0; i < nameCount; i++)
    {
        if(p.Left() < (uint16_t)(2 + p.PeekU16Little()))
        {
            return false;
        }

        libcomp::String name = p.ReadString16Little(
            state->GetClientStringEncoding(), true);
        names.push_back(name);
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());

    bool success = true;
    if(isDelete)
    {
        for(auto& name : names)
        {
            size_t count = worldData->BlacklistCount();
            for(size_t i = 0; i < count; i++)
            {
                if(worldData->GetBlacklist(i) == name)
                {
                    worldData->RemoveBlacklist(i);
                    break;
                }
            }
        }

        server->GetWorldDatabase()->QueueUpdate(worldData,
            state->GetAccountUID());
    }
    else
    {
        std::set<std::string> existing;
        for(auto& entry : worldData->GetBlacklist())
        {
            existing.insert(entry.C());
        }

        for(auto& name : names)
        {
            if(existing.find(name.C()) != existing.end())
            {
                // Already in the list
                success = false;
                break;
            }
        }

        if(success)
        {
            if(existing.size() + names.size() > MAX_BLACKLIST_COUNT)
            {
                success = false;
            }
            else
            {
                for(auto& name : names)
                {
                    worldData->AppendBlacklist(name);
                }

                server->GetWorldDatabase()->QueueUpdate(worldData,
                    state->GetAccountUID());
            }
        }
    }

    libcomp::Packet reply;
    reply.WriteS32Little(requestID);
    reply.WriteS32Little(success ? 0 : 1);

    client->SendPacket(reply);

    return true;
}
