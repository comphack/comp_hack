/**
 * @file server/channel/src/packets/game/TitleBuild.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to build a new character title.
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
#include <Character.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::TitleBuild::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 27)
    {
        return false;
    }

    uint8_t index = p.ReadU8();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    bool valid = index < 5;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TITLE_BUILD);
    reply.WriteU8(index);
    reply.WriteS32Little(valid ? 0 : -1);

    if(valid)
    {
        auto titles = character->GetCustomTitles();
        for(size_t i = (size_t)(index * MAX_TITLE_PARTS);
            i < (size_t)((index + 1) * MAX_TITLE_PARTS); i++)
        {
            titles[i] = p.ReadS16Little();
            reply.WriteS16Little(titles[i]);
        }

        character->SetCustomTitles(titles);

        server->GetWorldDatabase()->QueueUpdate(character);
    }
    else
    {
        reply.WriteBlank(26);
    }

    client->SendPacket(reply);

    if(valid && index == character->GetCurrentTitle())
    {
        // Send the new title to everyone
        server->GetCharacterManager()->SendCharacterTitle(client, true);
    }

    return true;
}
