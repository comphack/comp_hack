/**
 * @file server/channel/src/packets/game/ItemDepoRemote.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to open the remote item depos.
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
#include <ServerConstants.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"
#include "EventManager.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::ItemDepoRemote::Parse(
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

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_DEPO_REMOTE);

  if (server->GetEventManager()->RequestMenu(
          client, (int32_t)SVR_CONST.MENU_ITEM_DEPO)) {
    reply.WriteS32Little(0);  // Success
  } else {
    reply.WriteS32Little(-1);  // Failure
  }

  client->SendPacket(reply);

  return true;
}
