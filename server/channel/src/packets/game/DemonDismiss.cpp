/**
 * @file server/channel/src/packets/game/DemonDismiss.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to dismiss a demon.
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
#include <CharacterProgress.h>
#include <DemonBox.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

static void DismissDemon(const std::shared_ptr<ChannelServer> server,
                         const std::shared_ptr<ChannelClientConnection> client,
                         int64_t demonID) {
  auto state = client->GetClientState();
  auto dState = state->GetDemonState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto progress = character->GetProgress();
  auto demon = std::dynamic_pointer_cast<objects::Demon>(
      libcomp::PersistentObject::GetObjectByUUID(
          state->GetObjectUUID(demonID)));
  auto characterManager = server->GetCharacterManager();

  if (demon) {
    // Store it first if its summoned
    if (dState->GetEntity() == demon) {
      characterManager->StoreDemon(client);
    }

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());

    int8_t slot = demon->GetBoxSlot();
    auto box = std::dynamic_pointer_cast<objects::DemonBox>(
        libcomp::PersistentObject::GetObjectByUUID(demon->GetDemonBox()));

    characterManager->DeleteDemon(demon, dbChanges);
    if (box) {
      characterManager->SendDemonBoxData(client, box->GetBoxID(), {slot});
    }

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
  } else {
    LogDemonDebug([&]() {
      return libcomp::String(
                 "DemonDismiss request failed. Notifying requestor: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    libcomp::Packet err;
    err.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ERROR_COMP);
    err.WriteS32Little(
        (int32_t)ClientToChannelPacketCode_t::PACKET_DEMON_DISMISS);
    err.WriteS32Little(-1);

    client->SendPacket(err);
  }
}

bool Parsers::DemonDismiss::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 8) {
    return false;
  }

  int64_t demonID = p.ReadS64Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

  if (client->GetClientState()->GetObjectUUID(demonID).IsNull()) {
    return false;
  }

  DismissDemon(server, client, demonID);

  return true;
}
