/**
 * @file server/channel/src/packets/game/DiasporaBaseCapture.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to capture a Diaspora instance base.
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
#include <DiasporaBase.h>
#include <MiUraFieldTowerData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "MatchManager.h"

using namespace channel;

bool Parsers::DiasporaBaseCapture::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 4) {
    return false;
  }

  int32_t baseID = p.ReadS32Little();

  const int8_t ERROR_ALREAY_CAPTURED = -1;
  const int8_t ERROR_NO_ITEM = -2;

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto zone = state->GetZone();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();

  auto bState = zone ? zone->GetDiasporaBase(baseID) : nullptr;
  uint32_t itemType =
      bState ? bState->GetEntity()->GetDefinition()->GetCaptureItem() : 0;

  uint32_t itemCount =
      itemType ? characterManager->GetExistingItemCount(character, itemType)
               : 0;

  int8_t errorCode = 0;
  if (itemCount == 0) {
    errorCode = ERROR_NO_ITEM;
  } else if (server->GetMatchManager()->ToggleDiasporaBase(
                 state->GetZone(), baseID, cState->GetEntityID(), true)) {
    std::unordered_map<uint32_t, uint32_t> items;
    items[itemType] = 1;
    characterManager->AddRemoveItems(client, items, false);
  } else {
    errorCode = ERROR_ALREAY_CAPTURED;
  }

  libcomp::Packet reply;
  reply.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_DIASPORA_BASE_CAPTURE);
  reply.WriteS32Little(baseID);
  reply.WriteS32Little(errorCode);

  client->SendPacket(reply);

  return true;
}
