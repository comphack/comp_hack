/**
 * @file server/channel/src/packets/game/UBLeave.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to leave a UB instance prematurely.
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

// objects Includes
#include <UBMatch.h>

// channel Includes
#include "ChannelServer.h"
#include "MatchManager.h"
#include "Prefecture.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::UBLeave::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 4) {
    return false;
  }

  uint32_t matchSubType = p.ReadU32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto zone = state->GetZone();

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_UB_LEAVE);
  reply.WriteU32Little(matchSubType);

  auto ubMatch = zone ? zone->GetUBMatch() : nullptr;
  if (ubMatch && ubMatch->GetSubType() == matchSubType) {
    reply.WriteS32Little(0);  // Success

    client->QueuePacket(reply);

    auto prefecture = state->GetPrefecture();
    auto server = prefecture->GetServer();
    auto zoneManager = server->GetZoneManager();

    zoneManager->MoveToLobby(client);

    client->FlushOutgoing();
  } else {
    reply.WriteS32Little(-1);  // Failure

    client->SendPacket(reply);
  }

  return true;
}
