/**
 * @file server/channel/src/packets/game/Chat.cpp
 * @ingroup channel
 *
 * @author HikaruM
 *
 * @brief Handles GM Commands and Messages
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
#include "ChatManager.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::Chat::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() < 5) {
    return false;
  }

  uint16_t chatchannel = p.ReadU16Little();

  if (p.Left() != (uint16_t)(2 + p.PeekU16Little())) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();

  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto chatManager = server->GetChatManager();

  libcomp::String line =
      p.ReadString16Little(state->GetClientStringEncoding(), true);

  if (!chatManager->HandleGMand(client, line) &&
      !chatManager->SendChatMessage(client, (ChatType_t)chatchannel, line)) {
    LogChatManagerErrorMsg("Chat message could not be sent.\n");
  }

  return true;
}
