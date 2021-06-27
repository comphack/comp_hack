/**
 * @file server/channel/src/packets/game/FriendAddRemove.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to add or remove a friend.  Since packet
 *  sizes for this differ, this parser handles both functions.
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
#include "ManagerConnection.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::FriendAddRemove::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() < 4) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();

  bool add = p.Size() > 4;
  if (add) {
    if (p.Size() != (uint32_t)(p.PeekU16Little() + 6)) {
      return false;
    }

    libcomp::String targetName = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_DEFAULT, true);

    // 0 = Accepted/add, 1 = rejected
    int32_t mode = p.ReadS32Little();

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
    request.WriteU8(
        mode == 0 ? (int8_t)InternalPacketAction_t::PACKET_ACTION_ADD
                  : (int8_t)InternalPacketAction_t::PACKET_ACTION_RESPONSE_NO);
    request.WriteS32Little(state->GetWorldCID());
    request.WriteString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8,
        state->GetCharacterState()->GetEntity()->GetName(), true);
    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                                targetName, true);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
  } else {
    int32_t worldCID = p.ReadS32Little();

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_REMOVE);
    request.WriteS32Little(state->GetWorldCID());
    request.WriteS32Little(worldCID);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
  }

  return true;
}
