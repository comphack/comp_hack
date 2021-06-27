/**
 * @file server/channel/src/packets/game/ItemDrop.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to throw away an item from an item box.
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
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "DefinitionManager.h"

using namespace channel;

static void DropItem(const std::shared_ptr<ChannelServer> server,
                     const std::shared_ptr<ChannelClientConnection>& client,
                     int64_t itemID) {
  auto state = client->GetClientState();
  auto character = state->GetCharacterState()->GetEntity();

  auto uuid = state->GetObjectUUID(itemID);

  auto item = std::dynamic_pointer_cast<objects::Item>(
      libcomp::PersistentObject::GetObjectByUUID(uuid));
  auto itemDef =
      item ? server->GetDefinitionManager()->GetItemData(item->GetType())
           : nullptr;
  auto itemBox =
      item ? std::dynamic_pointer_cast<objects::ItemBox>(
                 libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()))
           : nullptr;

  if (itemBox && itemDef &&
      (itemDef->GetBasic()->GetFlags() & ITEM_FLAG_DISCARD) != 0) {
    int8_t slot = item->GetBoxSlot();

    server->GetCharacterManager()->UnequipItem(client, item);
    itemBox->SetItems((size_t)slot, NULLUUID);

    server->GetCharacterManager()->SendItemBoxData(client, itemBox,
                                                   {(uint16_t)slot});

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
    dbChanges->Update(itemBox);
    dbChanges->Delete(item);
    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
  } else {
    LogItemDebug([&]() {
      return libcomp::String(
                 "ItemDrop request failed. Notifying requestor: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    libcomp::Packet err;
    err.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ERROR_ITEM);
    err.WriteS32Little((int32_t)ClientToChannelPacketCode_t::PACKET_ITEM_DROP);
    err.WriteS32Little(-1);
    err.WriteS8(0);
    err.WriteS8(0);

    client->SendPacket(err);
  }
}

bool Parsers::ItemDrop::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 8) {
    return false;
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

  int64_t itemID = p.ReadS64Little();

  DropItem(server, client, itemID);

  return true;
}
