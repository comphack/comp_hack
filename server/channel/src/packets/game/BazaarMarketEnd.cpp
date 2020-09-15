/**
 * @file server/channel/src/packets/game/BazaarMarketEnd.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to stop interacting with a bazaar market.
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

// objects Includes
#include <AccountWorldData.h>
#include <BazaarData.h>

// channel Includes
#include "ChannelServer.h"
#include "EventManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::BazaarMarketEnd::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 8) {
    return false;
  }

  int32_t bazaarEntityID = p.ReadS32Little();
  int32_t responseID = p.ReadS32Little();
  (void)responseID;

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();

  // If the player's own market was the market being interacted with and is
  // currently marked as "preparing", make it active now and update the zone
  auto bState = state->GetBazaarState();
  if (bState && bState->GetEntityID() == bazaarEntityID) {
    auto worldData = state->GetAccountWorldData().Get();
    auto bazaarData = worldData->GetBazaarData().Get();

    if (bazaarData->GetState() ==
        objects::BazaarData::State_t::BAZAAR_PREPARING) {
      bazaarData->SetState(objects::BazaarData::State_t::BAZAAR_ACTIVE);
      server->GetZoneManager()->SendBazaarMarketData(
          state->GetCharacterState()->GetZone(), bState,
          bazaarData->GetMarketID());

      server->GetWorldDatabase()->QueueUpdate(bazaarData);
    }
  }

  // End the current event
  server->GetEventManager()->HandleEvent(client, nullptr);

  return true;
}
