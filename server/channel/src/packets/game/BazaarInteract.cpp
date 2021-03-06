/**
 * @file server/channel/src/packets/game/BazaarInteract.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to interact with a specific bazaar market.
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
#include <ServerConstants.h>

// object Includes
#include <EventInstance.h>

// channel Includes
#include "ChannelServer.h"
#include "EventManager.h"

using namespace channel;

bool Parsers::BazaarInteract::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 8) {
    return false;
  }

  int32_t bazaarEntityID = p.ReadS32Little();
  int32_t bazaarMarketID = p.ReadS32Little();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto eventManager = server->GetEventManager();

  if (!eventManager->RequestMenu(client, (int32_t)SVR_CONST.MENU_BAZAAR,
                                 bazaarMarketID, bazaarEntityID)) {
    LogBazaarError([&]() {
      return libcomp::String("Failed to open bazaar market: %1\n")
          .Arg(bazaarMarketID);
    });
  }

  return true;
}
