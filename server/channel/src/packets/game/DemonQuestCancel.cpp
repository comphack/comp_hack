/**
 * @file server/channel/src/packets/game/DemonQuestCancel.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to cancel the active demon quest.
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
#include <Demon.h>
#include <DemonQuest.h>

// channel Includes
#include "ChannelServer.h"
#include "EventManager.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::DemonQuestCancel::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 0) {
    return false;
  }

  // Even though only one demon quest can be active at once, its kinda
  // strange that no demon ID is supplied as confirmation.

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto dQuest = character->GetDemonQuest().Get();

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_QUEST_CANCEL);

  auto demon =
      dQuest
          ? std::dynamic_pointer_cast<objects::Demon>(
                libcomp::PersistentObject::GetObjectByUUID(dQuest->GetDemon()))
          : nullptr;
  if (demon && server->GetEventManager()->EndDemonQuest(client, 3) == 0) {
    reply.WriteS8(0);         // Success
    reply.WriteS16Little(0);  // New sequence count?
  } else {
    reply.WriteS8(-1);  // Failed
  }

  client->SendPacket(reply);

  return true;
}
