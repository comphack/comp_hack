/**
 * @file server/channel/src/packets/game/FriendInfo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the current player's own friend info.
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
#include <AccountLogin.h>
#include <CharacterLogin.h>
#include <FriendSettings.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::FriendInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto state = client->GetClientState();
    auto cLogin = state->GetAccountLogin()->GetCharacterLogin();
    auto character = state->GetCharacterState()->GetEntity();
    auto fSettings = character->LoadFriendSettings(server->GetWorldDatabase());

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FRIEND_INFO_SELF);
    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        character->GetName(), true);
    reply.WriteU32Little((uint32_t)cLogin->GetWorldCID());
    reply.WriteS8(0);
    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        fSettings->GetFriendMessage(), true);

    auto privacySet = fSettings->GetPublicToZone();
    reply.WriteU8(privacySet ? 1 : 0);
    reply.WriteU8(fSettings->GetPublicToZone() ? 1 : 0);

    connection->SendPacket(reply);

    // Request current friend info from the world to send on reply
    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LIST);
    request.WriteS32Little(cLogin->GetWorldCID());

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

    return true;
}
