/**
 * @file server/channel/src/packets/game/TitleList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the list of available titles.
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
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Character.h>
#include <CharacterProgress.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::TitleList::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 0) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto progress = character->GetProgress().Get();

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TITLE_LIST);
  reply.WriteS32Little(0);
  reply.WriteU8(character->GetCurrentTitle());

  auto specialTitles = progress->GetSpecialTitles();

  reply.WriteS16Little((int16_t)specialTitles.size());
  reply.WriteArray(&specialTitles, (uint32_t)specialTitles.size());

  auto titles = progress->GetTitles();

  reply.WriteS32Little((int32_t)titles.size());
  for (int16_t titleID : titles) {
    reply.WriteS16Little(titleID);
  }

  for (size_t i = 0; i < (size_t)(MAX_TITLE_PARTS * 5); i++) {
    if (i % MAX_TITLE_PARTS == 0) {
      // Write count for each chunk of IDs
      reply.WriteS32Little((int32_t)(i / MAX_TITLE_PARTS));
    }

    reply.WriteS16Little(character->GetCustomTitles(i));
  }

  reply.WriteU8(character->GetTitlePrioritized() ? 1 : 0);

  client->SendPacket(reply);

  return true;
}
