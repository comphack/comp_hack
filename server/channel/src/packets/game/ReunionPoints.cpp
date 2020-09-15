/**
 * @file server/channel/src/packets/game/ReunionPoints.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to retrieve the current account world
 *  level reunion and mitama reunion conversion points.
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
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <AccountWorldData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ReunionPoints::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 4) {
    return false;
  }

  int32_t unknown = p.ReadS32Little();
  (void)unknown;  // Always 0

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto awd = state->GetAccountWorldData().Get();

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REUNION_POINTS);
  reply.WriteS32Little(0);  // Unknown
  reply.WriteS32Little(awd ? 0 : -1);

  if (awd) {
    reply.WriteS32Little((int32_t)awd->GetReunionPoints());
    reply.WriteS32Little((int32_t)awd->GetMitamaReunionPoints());
  }

  client->SendPacket(reply);

  return true;
}
