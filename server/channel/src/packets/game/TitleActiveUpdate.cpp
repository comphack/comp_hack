/**
 * @file server/channel/src/packets/game/TitleActiveUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update the character's active title.
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

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "Prefecture.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::TitleActiveUpdate::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 2) {
    return false;
  }

  uint8_t titleIdx = p.ReadU8();
  uint8_t prioritize = p.ReadU8();

  if (titleIdx >= 5) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto characterManager = server->GetCharacterManager();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  character->SetCurrentTitle(titleIdx);
  character->SetTitlePrioritized(prioritize == 1);

  libcomp::Packet reply;
  reply.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_TITLE_ACTIVE_UPDATE);
  reply.WriteU8(titleIdx);
  reply.WriteS32Little(0);  // Success
  reply.WriteU8(prioritize);

  client->SendPacket(reply);

  characterManager->SendCharacterTitle(client, false);

  server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

  return true;
}
