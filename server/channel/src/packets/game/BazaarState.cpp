/**
 * @file server/channel/src/packets/game/BazaarState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request for the current zone's bazaar cost and duration.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <ServerZone.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "Prefecture.h"
#include "Zone.h"

using namespace channel;

bool Parsers::BazaarState::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 0) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto sharedConfig = server->GetWorldSharedConfig();
  auto cState = state->GetCharacterState();
  auto zone = cState->GetZone();
  auto zoneDef = zone ? zone->GetDefinition() : nullptr;

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_STATE);

  if (!zoneDef) {
    reply.WriteS32Little(0);
    reply.WriteS32Little(0);
    reply.WriteS32Little(-1);  // Error
  } else {
    // World settings for market time and cost override zone
    if (sharedConfig->GetBazaarMarketTime()) {
      reply.WriteS32Little((int32_t)sharedConfig->GetBazaarMarketTime());
    } else {
      reply.WriteS32Little((int32_t)zoneDef->GetBazaarMarketTime());
    }

    if (sharedConfig->GetBazaarMarketCost()) {
      reply.WriteS32Little((int32_t)sharedConfig->GetBazaarMarketCost());
    } else {
      reply.WriteS32Little((int32_t)zoneDef->GetBazaarMarketCost());
    }

    reply.WriteS32Little(0);  // No error
  }

  connection->SendPacket(reply);

  return true;
}
