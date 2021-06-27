/**
 * @file server/channel/src/packets/game/SyncCharacter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to sync the player character (or partner
 *  demon) data. This happens if the client expected an expiration to take
 *  place that the server never sent.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"
#include "Prefecture.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::SyncCharacter::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 4) {
    return false;
  }

  int32_t entityID = p.ReadS32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto eState = state->GetEntityState(entityID);

  if (eState == nullptr) {
    LogGeneralError([&]() {
      return libcomp::String(
                 "Entity not belonging to the client requested for "
                 "SyncCharacter: %1\n")
          .Arg(entityID);
    });

    return true;
  }

  auto cs = eState->GetCoreStats();
  auto statusEffects = eState->GetCurrentStatusEffectStates();

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SYNC_CHARACTER);
  reply.WriteS32Little(entityID);
  reply.WriteU32Little((uint32_t)cs->GetHP());
  reply.WriteU32Little((uint32_t)cs->GetMP());
  reply.WriteU8((uint8_t)statusEffects.size());
  for (auto ePair : statusEffects) {
    reply.WriteU32Little(ePair.first->GetEffect());
    reply.WriteS32Little((int32_t)ePair.second);
    reply.WriteU8(ePair.first->GetStack());
  }

  // Send back to the whole zone just in case
  server->GetZoneManager()->BroadcastPacket(client, reply);

  return true;
}
