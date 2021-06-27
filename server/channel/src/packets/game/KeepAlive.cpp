/**
 * @file server/channel/src/packets/game/KeepAlive.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to keep the connection active.
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
#include <DefinitionManager.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::KeepAlive::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 4) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();

  ServerTime now = ChannelServer::GetServerTime();

  // Keep alive requests should occur once every 10 seconds
  // After a missed request, the configurable server timeout countdown
  // will occur. Stop refreshing if the client is already prepared for
  // a disconnect
  if (state->GetLogoutSave()) {
    client->RefreshTimeout(now, 10);
  }

  // Refresh the client entity positions
  auto cState = state->GetCharacterState();
  cState->RefreshCurrentPosition(now);

  auto dState = state->GetDemonState();
  if (dState->Ready()) {
    dState->RefreshCurrentPosition(now);
  }

  // Sync equipment expiration up with this request since frequent calls
  // are required to keep connected
  if (cState->EquipmentExpired((uint32_t)std::time(0))) {
    auto prefecture = state->GetPrefecture();
    auto server = prefecture->GetServer();

    cState->RecalcEquipState(server->GetDefinitionManager());
    server->GetCharacterManager()->RecalculateTokuseiAndStats(cState, client);
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_KEEP_ALIVE);
  reply.WriteU32Little(p.ReadU32Little());

  client->SendPacket(reply);

  return true;
}