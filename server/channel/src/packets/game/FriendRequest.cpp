/**
 * @file server/channel/src/packets/game/FriendRequest.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to add a player as a friend.
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

// objects Includes
#include <AccountLogin.h>
#include <CharacterLogin.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::FriendRequest::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 2 || (p.Size() != (uint32_t)(2 + p.PeekU16Little())))
    {
        return false;
    }

    libcomp::String targetName = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_CP932, true);

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();
    auto worldDB = server->GetWorldDatabase();

    auto target = objects::Character::LoadCharacterByName(worldDB, targetName);
    if(target && target != character)
    {
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
        request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_YN_REQUEST);
        request.WriteS32Little(state->GetAccountLogin()->GetCharacterLogin()
            ->GetWorldCID());
        request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
            character->GetName(), true);
        request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
            targetName, true);

        server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
    }
    else
    {
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FRIEND_REQUEST);
        reply.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
            targetName, true);
        reply.WriteS32Little(-1);
        client->SendPacket(reply);
    }

    return true;
}
