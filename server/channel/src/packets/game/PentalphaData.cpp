/**
 * @file server/channel/src/packets/game/PentalphaData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the current world and character Pentalpha
 *  data.
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

// object Includes
#include <CharacterProgress.h>
#include <PentalphaEntry.h>
#include <PentalphaMatch.h>

// channel Includes
#include "ChannelServer.h"
#include "MatchManager.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::PentalphaData::Parse(
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
  auto matchManager = server->GetMatchManager();
  auto character = state->GetCharacterState()->GetEntity();
  auto progress = character
                      ? character->GetProgress().Get(server->GetWorldDatabase())
                      : nullptr;

  auto entry = matchManager->LoadPentalphaData(client);
  auto previousEntry = state->GetPentalphaData(1).Get();
  auto match = matchManager->GetPentalphaMatch(false);
  auto previous = matchManager->GetPentalphaMatch(true);

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PENTALPHA_DATA);
  reply.WriteS32Little(0);

  for (size_t i = 0; i < 5; i++) {
    reply.WriteS32Little(progress ? progress->GetBethel(i) : 0);
  }

  reply.WriteS32Little(
      (int32_t)(previousEntry ? previousEntry->GetTeam() : -1));
  reply.WriteS32Little(entry ? entry->GetTeam() : -1);

  reply.WriteS32Little(progress ? progress->GetCowrie() : 0);
  reply.WriteS32Little(previousEntry ? previousEntry->GetCowrie() : 0);

  reply.WriteS32Little(0);  // Unknown/unused

  for (size_t i = 0; i < 5; i++) {
    reply.WriteS32Little((int32_t)(match ? match->GetPoints(i) : 0));
    reply.WriteS32Little((int32_t)(previous ? previous->GetRankings(i) : 0));
  }

  client->SendPacket(reply);

  return true;
}
