/**
 * @file server/channel/src/packets/game/PartyInvite.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to invite someone to your party.
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
#include <ErrorCodes.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::PartyInvite::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() < 2 || (p.Size() != (uint32_t)(2 + p.PeekU16Little()))) {
    return false;
  }

  libcomp::String targetName = p.ReadString16Little(
      libcomp::Convert::Encoding_t::ENCODING_DEFAULT, true);

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto character = state->GetCharacterState()->GetEntity();
  auto worldDB = server->GetWorldDatabase();

  auto target = objects::Character::LoadCharacterByName(worldDB, targetName);
  if (target && target != character) {
    auto member = state->GetPartyCharacter(true);

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_YN_REQUEST);
    request.WriteU8(0);  // Not from recruit
    member->SavePacket(request, false);
    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                                targetName, true);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
  } else {
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_INVITE);
    reply.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_DEFAULT,
                              targetName, true);
    reply.WriteU16Little((uint16_t)PartyErrorCodes_t::INVALID_OR_OFFLINE);
    client->SendPacket(reply);
  }

  return true;
}
