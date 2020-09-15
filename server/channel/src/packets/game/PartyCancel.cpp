/**
 * @file server/channel/src/packets/game/PartyCancel.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to cancel joining a party.
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
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::PartyCancel::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() < 2 || (p.Size() != (uint32_t)(6 + p.PeekU16Little()))) {
    return false;
  }

  libcomp::String targetName =
      p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932, true);
  uint32_t partyID = p.ReadU32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto state = client->GetClientState();
  auto character = state->GetCharacterState()->GetEntity();

  libcomp::Packet request;
  request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
  request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_RESPONSE_NO);
  request.WriteS32Little(state->GetWorldCID());
  request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                              character->GetName(), true);
  request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                              targetName, true);
  request.WriteU32Little(partyID);

  server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

  return true;
}
