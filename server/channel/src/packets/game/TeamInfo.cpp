/**
 * @file server/channel/src/packets/game/TeamInfo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the current player's team info.
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
#include <ErrorCodes.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Team.h>

// channel Includes
#include "ChannelClientConnection.h"

using namespace channel;

bool Parsers::TeamInfo::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 4) {
    return false;
  }

  int32_t teamID = p.ReadS32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto team = state->GetTeam();

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_INFO);

  if (team && team->GetID() == teamID) {
    reply.WriteS32Little(teamID);
    reply.WriteS8((int8_t)TeamErrorCodes_t::SUCCESS);

    reply.WriteS32Little(team->GetLeaderCID());
    reply.WriteS8(team->GetType());

    // It seems there was more planned for teams at one point but the
    // client does not respond to any of the following fields
    reply.WriteS8(0);
    reply.WriteS8(0);
    reply.WriteS8(0);
    reply.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_DEFAULT,
                              "", true);

    reply.WriteS32Little(team->GetSmallZiotite());
    reply.WriteS8(team->GetLargeZiotite());
    reply.WriteS8((int8_t)team->MemberIDsCount());
  } else {
    reply.WriteS32Little(teamID);
    reply.WriteS8((int8_t)TeamErrorCodes_t::INVALID_TEAM);
  }

  client->SendPacket(reply);

  return true;
}
