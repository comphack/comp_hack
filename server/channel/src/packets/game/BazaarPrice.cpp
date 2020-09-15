/**
 * @file server/channel/src/packets/game/BazaarPrice.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to get a suggested sales price for a bazaar item.
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
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// objects Includes
#include <Item.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::BazaarPrice::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 8) {
    return false;
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();

  int64_t itemID = p.ReadS64Little();

  auto item = std::dynamic_pointer_cast<objects::Item>(
      libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_PRICE);
  reply.WriteS64Little(itemID);

  if (item) {
    auto definitionManager = server->GetDefinitionManager();
    auto itemData = definitionManager->GetItemData(item->GetType());

    reply.WriteS32Little(0);  // Success

    int32_t refPrice =
        itemData->GetBasic()->GetBuyPrice() * (int32_t)item->GetStackSize();

    reply.WriteS32Little(refPrice);  // Reference

    // High/low suggestions default to +/-20% the reference price
    reply.WriteS32Little((int32_t)((double)refPrice * 1.2));
    reply.WriteS32Little((int32_t)((double)refPrice * 0.8));
  } else {
    reply.WriteS32Little(-1);  // Failure
  }

  client->SendPacket(reply);

  return true;
}
