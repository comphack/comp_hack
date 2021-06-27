/**
 * @file server/channel/src/packets/game/EnchantItem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update an item used for enchantment.
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

// object Includes
#include <Item.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ManagerConnection.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::EnchantItem::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 12) {
    return false;
  }

  int64_t itemID = p.ReadS64Little();
  int32_t functionalType = p.ReadS32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto characterManager = server->GetCharacterManager();
  auto cState = state->GetCharacterState();
  auto exchangeSession = state->GetExchangeSession();

  auto otherClient = exchangeSession && exchangeSession->GetSourceEntityID() !=
                                            cState->GetEntityID()
                         ? server->GetManagerConnection()->GetEntityClient(
                               exchangeSession->GetSourceEntityID(), false)
                         : nullptr;

  bool success = false;
  int16_t effectID = 0;
  std::list<int32_t> successRates;
  uint32_t specialEnchantItemType = static_cast<uint32_t>(-1);

  auto item = itemID != -1 ? std::dynamic_pointer_cast<objects::Item>(
                                 libcomp::PersistentObject::GetObjectByUUID(
                                     state->GetObjectUUID(itemID)))
                           : nullptr;

  bool error = false;
  if (exchangeSession && (itemID == -1 || item)) {
    auto previous = exchangeSession->GetItems((size_t)functionalType);
    exchangeSession->SetItems((size_t)functionalType, item);

    auto otherState = otherClient ? otherClient->GetClientState() : state;
    success = characterManager->GetSynthOutcome(otherState, exchangeSession,
                                                specialEnchantItemType,
                                                successRates, &effectID);

    if (!success) {
      // Put the previous item back and recalc old values
      exchangeSession->SetItems((size_t)functionalType, previous);
      itemID = state->GetObjectID(previous.GetUUID());

      if (!characterManager->GetSynthOutcome(otherState, exchangeSession,
                                             specialEnchantItemType,
                                             successRates, &effectID)) {
        error = true;
      }
    }
  } else {
    error = true;
  }

  int32_t normalRate = successRates.size() > 0 ? successRates.front() : 0;
  int32_t specialRate = successRates.size() > 1 ? successRates.back() : 0;

  libcomp::Packet reply;
  reply.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_ENCHANT_ITEM_UPDATE);
  reply.WriteS64Little(itemID);
  reply.WriteS32Little(functionalType);
  reply.WriteS16Little(effectID);
  reply.WriteS32Little(normalRate);
  reply.WriteU32Little(specialEnchantItemType);
  reply.WriteS32Little(specialRate);
  reply.WriteS32Little(error ? -1 : 0);

  client->SendPacket(reply);

  if (success && otherClient) {
    auto otherState = otherClient->GetClientState();
    int64_t otherItemID = item ? otherState->GetObjectID(item->GetUUID()) : -1;

    libcomp::Packet notify;
    notify.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_ENCHANT_ITEM_UPDATED);
    notify.WriteS64Little(otherItemID);

    characterManager->GetItemDetailPacketData(notify, item);

    notify.WriteS32Little(functionalType);
    notify.WriteS16Little(effectID);
    notify.WriteS32Little(normalRate);
    notify.WriteU32Little(specialEnchantItemType);
    notify.WriteS32Little(specialRate);

    otherClient->SendPacket(notify);
  }

  return true;
}
