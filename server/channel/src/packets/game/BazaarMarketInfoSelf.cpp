/**
 * @file server/channel/src/packets/game/BazaarMarketInfoSelf.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request for details about the player's bazaar market.
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

// object Includes
#include <BazaarData.h>
#include <BazaarItem.h>
#include <Item.h>

// channel Includes
#include "BazaarState.h"
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::BazaarMarketInfoSelf::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 0) {
    return false;
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto worldDB = server->GetWorldDatabase();

  // Always reload
  auto bazaarData = objects::BazaarData::LoadBazaarDataByAccount(
      worldDB, state->GetAccountUID());

  auto character = bazaarData ? bazaarData->LoadCharacter(worldDB) : nullptr;

  libcomp::Packet reply;
  reply.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_INFO_SELF);
  reply.WriteS32Little(0);  // Success
  reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                            character ? character->GetName() : "", true);
  reply.WriteS8(bazaarData ? (int8_t)bazaarData->GetChannelID() : -1);
  reply.WriteU32Little(bazaarData ? bazaarData->GetZone() : 0);
  reply.WriteU32Little(bazaarData ? bazaarData->GetMarketID() : 0);
  reply.WriteU32Little(bazaarData ? bazaarData->GetMarketID()
                                  : 0);  // Unique ID?
  reply.WriteS32Little(15);              // Max item slots
  reply.WriteS16Little(bazaarData ? bazaarData->GetNPCType() : 0);
  reply.WriteS32Little(
      bazaarData && bazaarData->GetState() !=
                        objects::BazaarData::State_t::BAZAAR_INACTIVE
          ? ChannelServer::GetExpirationInSeconds(bazaarData->GetExpiration())
          : -1);

  if (bazaarData) {
    reply.WriteString16Little(state->GetClientStringEncoding(),
                              bazaarData->GetComment(), true);

    int32_t itemCount = 0;
    for (auto bItem : bazaarData->GetItems()) {
      if (!bItem.IsNull()) {
        itemCount++;
      }
    }

    bool marketActive =
        bazaarData->GetState() == objects::BazaarData::State_t::BAZAAR_ACTIVE;
    reply.WriteS32Little(itemCount);
    for (size_t i = 0; i < 15; i++) {
      auto bItem = bazaarData->GetItems(i);
      if (!bItem.IsNull()) {
        auto item = bItem->GetItem().Get(worldDB);

        reply.WriteS8((int8_t)i);

        // Item states: Selling/removable/sold
        reply.WriteS8(bItem->GetSold() ? 2 : (marketActive ? 0 : 1));

        reply.WriteFloat(0.f);  // Unknown
        reply.WriteS64Little(item ? state->GetObjectID(item->GetUUID()) : -1);

        reply.WriteS32Little((int32_t)bItem->GetCost());

        reply.WriteU32Little(bItem->GetType());
        reply.WriteU16Little(bItem->GetStackSize());

        characterManager->GetItemDetailPacketData(reply, item, 1);
      }
    }
  } else {
    reply.WriteString16Little(state->GetClientStringEncoding(), "", true);
    reply.WriteS32Little(0);
  }

  reply.WriteS32Little(15);  // Unknown

  client->SendPacket(reply);

  return true;
}
