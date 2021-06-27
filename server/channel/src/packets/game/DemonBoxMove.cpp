/**
 * @file server/channel/src/packets/game/DemonBoxMove.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to move a demon in a box.
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

// objects Includes
#include <Character.h>
#include <CharacterProgress.h>
#include <Demon.h>
#include <DemonBox.h>
#include <DemonQuest.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::DemonBoxMove::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 11) {
    return false;
  }

  int8_t srcBoxID = p.ReadS8();
  int64_t demonID = p.ReadS64Little();
  int8_t destBoxID = p.ReadS8();
  int8_t destSlot = p.ReadS8();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto progress = character->GetProgress();
  auto characterManager = server->GetCharacterManager();

  auto destBox = characterManager->GetDemonBox(state, destBoxID);

  auto srcDemon = std::dynamic_pointer_cast<objects::Demon>(
      libcomp::PersistentObject::GetObjectByUUID(
          state->GetObjectUUID(demonID)));
  auto destDemon =
      destBox ? destBox->GetDemons((size_t)destSlot).Get() : nullptr;

  int8_t srcSlot = srcDemon->GetBoxSlot();
  auto srcBox = std::dynamic_pointer_cast<objects::DemonBox>(
      libcomp::PersistentObject::GetObjectByUUID(srcDemon->GetDemonBox()));

  uint8_t maxDestSlots = destBoxID == 0 ? progress->GetMaxCOMPSlots() : 50;

  bool fail = false;
  if (!srcBox || srcBoxID != srcBox->GetBoxID() || !destBox ||
      srcDemon != srcBox->GetDemons((size_t)srcSlot).Get() ||
      destSlot >= maxDestSlots) {
    LogDemonDebug([&]() {
      return libcomp::String(
                 "DemonBoxMove request failed. Notifying requestor: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    fail = true;
  } else if (srcBox != destBox) {
    // Make sure nothing is being moved into an expired box (allow
    // reorganize because why not?)
    uint32_t now = (uint32_t)std::time(0);
    if ((srcDemon && destBox->GetRentalExpiration() &&
         destBox->GetRentalExpiration() < now) ||
        (destDemon && srcBox->GetRentalExpiration() &&
         srcBox->GetRentalExpiration() < now)) {
      fail = true;
    }
  }

  if (fail) {
    // Request client rollback and quit here
    libcomp::Packet err;
    err.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ERROR_COMP);
    err.WriteS32Little(
        (int32_t)ClientToChannelPacketCode_t::PACKET_DEMON_BOX_MOVE);
    err.WriteS32Little(-1);

    client->SendPacket(err);

    return true;
  }

  auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());

  // If the active demon is being moved to a non-COMP box, store it first
  if ((srcDemon && srcDemon == character->GetActiveDemon().Get() &&
       destBoxID != 0) ||
      (destDemon && destDemon == character->GetActiveDemon().Get() &&
       srcBoxID != 0)) {
    characterManager->StoreDemon(client);
    dbChanges->Update(character);
  }

  dbChanges->Update(srcDemon);
  dbChanges->Update(srcBox);
  dbChanges->Update(destBox);

  srcDemon->SetBoxSlot(destSlot);
  srcDemon->SetDemonBox(destBox->GetUUID());
  if (destDemon) {
    destDemon->SetBoxSlot(srcSlot);
    destDemon->SetDemonBox(srcBox->GetUUID());
    dbChanges->Update(destDemon);
  }

  srcBox->SetDemons((size_t)srcSlot, destDemon);
  destBox->SetDemons((size_t)destSlot, srcDemon);

  if (srcBox != destBox) {
    characterManager->SendDemonBoxData(client, destBoxID, {destSlot});
    characterManager->SendDemonBoxData(client, srcBoxID, {srcSlot});

    // Clear all quests
    auto dQuest = character->GetDemonQuest().Get();
    for (auto& d : {srcDemon, destDemon}) {
      if (d && d->GetHasQuest()) {
        if (dQuest && dQuest->GetDemon() == d->GetUUID()) {
          // Fail the quest
          server->GetEventManager()->EndDemonQuest(client);
        }

        d->SetHasQuest(false);
      }
    }
  } else {
    characterManager->SendDemonBoxData(client, srcBoxID, {srcSlot, destSlot});
  }

  server->GetWorldDatabase()->QueueChangeSet(dbChanges);

  return true;
}
