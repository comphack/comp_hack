/**
 * @file server/channel/src/packets/game/DigitalizePoints.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the current player's digitalize
 *  point information.
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

// object Includes
#include <CharacterProgress.h>

// channel Includes
#include "ChannelServer.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::DigitalizePoints::Parse(
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
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto definitionManager = server->GetDefinitionManager();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto progress = character ? character->GetProgress().Get() : nullptr;

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DIGITALIZE_POINTS);
  reply.WriteS32Little(0);  // Unknown
  reply.WriteS32Little(progress ? 0 : -1);

  if (progress) {
    for (uint8_t raceID : definitionManager->GetGuardianRaceIDs()) {
      reply.WriteU8(raceID);
      reply.WriteS8(progress->GetDigitalizeLevels(raceID));
      reply.WriteS32Little(progress->GetDigitalizePoints(raceID));
    }
  }

  client->SendPacket(reply);

  return true;
}
