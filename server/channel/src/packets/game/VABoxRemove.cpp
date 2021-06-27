/**
 * @file server/channel/src/packets/game/VABoxRemove.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the clien to drop a VA item from the closet.
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

// channel Includes
#include "ChannelServer.h"
#include "Prefecture.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::VABoxRemove::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 9) {
    return false;
  }

  int32_t unused = p.ReadS32Little();  // Always 0
  int8_t slot = p.ReadS8();
  uint32_t itemType = p.ReadU32Little();
  (void)unused;

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  bool success = itemType != static_cast<uint32_t>(-1);
  if (success && character->GetVACloset((size_t)slot) == itemType) {
    character->SetVACloset((size_t)slot, 0);

    success = true;
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_VA_BOX_REMOVE);
  reply.WriteS32Little(success ? 0 : -1);
  reply.WriteS32Little(0);  // Unknown
  reply.WriteS8(slot);
  reply.WriteU32Little(itemType);

  client->SendPacket(reply);

  if (success) {
    // If its equipped, remove it
    for (auto vaPair : character->GetEquippedVA()) {
      if (vaPair.second == itemType) {
        character->RemoveEquippedVA((uint8_t)slot);

        libcomp::Packet notify;
        notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_VA_CHANGED);
        notify.WriteS32Little(cState->GetEntityID());
        notify.WriteS32Little(1);  // Count
        notify.WriteS8(slot);
        notify.WriteU32Little(itemType);

        server->GetZoneManager()->BroadcastPacket(client, notify, false);

        break;
      }
    }

    server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());
  }

  return true;
}
