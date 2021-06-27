/**
 * @file server/channel/src/packets/game/BazaarClerkSet.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to set the player's bazaar clerk NPC.
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
#include <AccountWorldData.h>
#include <BazaarData.h>

// channel Includes
#include "ChannelServer.h"
#include "Prefecture.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::BazaarClerkSet::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 2) {
    return false;
  }

  int16_t npcType = p.ReadS16Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto bState = state->GetBazaarState();
  auto cState = state->GetCharacterState();
  auto zone = cState->GetZone();

  auto worldData = state->GetAccountWorldData().Get();
  auto bazaarData = worldData->GetBazaarData().Get();

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_CLERK_SET);

  if (bazaarData && bState) {
    if (bazaarData->GetNPCType() != npcType) {
      bazaarData->SetNPCType(npcType);

      server->GetZoneManager()->SendBazaarMarketData(zone, bState,
                                                     bazaarData->GetMarketID());

      server->GetWorldDatabase()->QueueUpdate(bazaarData);
    }

    reply.WriteS32Little(0);
  } else {
    reply.WriteS32Little(-1);
  }

  client->SendPacket(reply);

  return true;
}
