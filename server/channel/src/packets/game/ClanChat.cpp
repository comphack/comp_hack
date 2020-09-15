/**
 * @file server/channel/src/packets/game/ClanChat.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to send a chat message to the their
 *  clan's chat channel.
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

using namespace channel;

bool Parsers::ClanChat::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() < 6) {
    return false;
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto chatManager = server->GetChatManager();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();

  int32_t clanID = p.ReadS32Little();
  libcomp::String message =
      p.ReadString16Little(state->GetClientStringEncoding(), true);
  (void)clanID;

  if (!chatManager->HandleGMand(client, message) &&
      !chatManager->SendChatMessage(client, ChatType_t::CHAT_CLAN, message)) {
    LogChatManagerErrorMsg("Clan chat message could not be sent.\n");
  }

  return true;
}
