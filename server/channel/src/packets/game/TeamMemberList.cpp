/**
 * @file server/channel/src/packets/game/TeamMemberList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to list all members in the player's team.
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

// object Includes
#include <Team.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::TeamMemberList::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 4) {
    return false;
  }

  int32_t teamID = p.ReadS32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

  libcomp::Packet request;
  request.WritePacketCode(InternalPacketCode_t::PACKET_TEAM_UPDATE);
  request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LIST);
  request.WriteS32Little(teamID);
  request.WriteS32Little(state->GetWorldCID());

  server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

  return true;
}
