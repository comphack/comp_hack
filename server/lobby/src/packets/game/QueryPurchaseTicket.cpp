/**
 * @file server/lobby/src/packets/game/QueryPurchaseTicket.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to handle purchasing lobby tickets.
 *
 * This file is part of the Lobby Server (lobby).
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
#include <LobbyClientConnection.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// object Includes
#include <Account.h>
#include <LobbyConfig.h>

// lobby Includes
#include "LobbyServer.h"

using namespace lobby;

bool Parsers::QueryPurchaseTicket::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 1) {
    return false;
  }

  uint8_t code = p.ReadU8();
  if (1 < code) {
    return false;
  }

  if (code == 1) {
    auto server =
        std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto config =
        std::dynamic_pointer_cast<objects::LobbyConfig>(server->GetConfig());
    auto account = state(connection)->GetAccount();

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyToClientPacketCode_t::PACKET_QUERY_PURCHASE_TICKET);
    reply.WriteU32Little(static_cast<uint32_t>(0));
    reply.WriteU8(1);

    reply.WriteU32Little(config->GetCharacterTicketCost());
    reply.WriteU32Little(account->GetCP());

    connection->SendPacket(reply);
  } else {
    // Request was cancelled, nothing to do
  }

  return true;
}
