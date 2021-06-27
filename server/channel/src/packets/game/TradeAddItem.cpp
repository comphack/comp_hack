/**
 * @file server/channel/src/packets/game/TradeAddItem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to add an item to the trade.
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
#include <Account.h>
#include <Character.h>
#include <Item.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ClientState.h"
#include "DefinitionManager.h"
#include "ManagerConnection.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::TradeAddItem::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 12) {
    return false;
  }

  int64_t itemID = p.ReadS64Little();
  int32_t slot = p.ReadS32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto characterManager = server->GetCharacterManager();
  auto cState = state->GetCharacterState();
  auto exchangeSession = state->GetExchangeSession();

  auto item = std::dynamic_pointer_cast<objects::Item>(
      libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

  bool cancel = false;
  if (!item || slot >= 30) {
    LogTradeErrorMsg("Invalid item trade request.\n");

    cancel = true;
  }

  auto itemDef =
      item ? server->GetDefinitionManager()->GetItemData(item->GetType())
           : nullptr;
  if (itemDef && (itemDef->GetBasic()->GetFlags() & ITEM_FLAG_TRADE) == 0) {
    LogTradeError([item, state]() {
      return libcomp::String(
                 "Player attempted to add non-trade item type %1 to a trade: "
                 "%2\n")
          .Arg(item->GetType())
          .Arg(state->GetAccountUID().ToString());
    });

    cancel = true;
  }

  auto otherCState = exchangeSession
                         ? std::dynamic_pointer_cast<CharacterState>(
                               exchangeSession->GetOtherCharacterState())
                         : nullptr;
  auto otherClient = otherCState
                         ? server->GetManagerConnection()->GetEntityClient(
                               otherCState->GetEntityID(), false)
                         : nullptr;

  if (!otherClient) {
    LogTradeErrorMsg("Invalid trade session.\n");

    cancel = true;
  }

  if (cancel) {
    characterManager->EndExchange(client);
    if (otherClient) {
      characterManager->EndExchange(otherClient);
    }

    return true;
  }

  bool error = false;
  if (slot > -1) {
    // Adding, make sure the item is not already there
    auto items = exchangeSession->GetItems();
    for (size_t i = 0; i < 30; i++) {
      if (items[i].Get() == item) {
        auto accountUID = state->GetAccountUID();
        LogTradeDebug([accountUID]() {
          return libcomp::String(
                     "Player attempted to add a trade item more than once: "
                     "%1\n")
              .Arg(accountUID.ToString());
        });

        error = true;
        break;
      }
    }

    if (!error) {
      exchangeSession->SetItems((size_t)slot, item);
    }
  } else {
    // Removing, drop all found instances
    auto items = exchangeSession->GetItems();
    for (size_t i = 0; i < 30; i++) {
      if (items[i].Get() == item) {
        items[i].SetUUID(NULLUUID);
      }
    }

    exchangeSession->SetItems(items);
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_ADD_ITEM);
  reply.WriteS32Little(0);  // Unknown
  reply.WriteS64Little(error ? -1 : itemID);
  reply.WriteS32Little(slot);
  reply.WriteS32Little(0);  // Unknown, seems to be the same as other packet

  client->SendPacket(reply);

  if (!error) {
    auto otherState = otherClient->GetClientState();
    auto otherObjectID = otherState->GetObjectID(item->GetUUID());
    if (otherObjectID <= 0) {
      otherObjectID = server->GetNextObjectID();
      otherState->SetObjectID(item->GetUUID(), otherObjectID);
    }

    libcomp::Packet notify;
    notify.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_TRADE_ADDED_ITEM);
    notify.WriteS32Little(slot);
    notify.WriteS64Little(otherObjectID);

    characterManager->GetItemDetailPacketData(notify, item);

    notify.WriteS32Little(0);  // Unknown

    otherClient->SendPacket(notify);
  }

  return true;
}
