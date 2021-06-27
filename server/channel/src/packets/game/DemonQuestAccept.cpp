/**
 * @file server/channel/src/packets/game/DemonQuestAccept.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to accept a pending demon quest.
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

bool Parsers::DemonQuestAccept::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 8) {
    return false;
  }

  int64_t demonID = p.ReadS64Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto progress = character->GetProgress().Get();
  auto dQuest = character->GetDemonQuest().Get();

  bool success = false;

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_QUEST_ACCEPT);

  auto demon = std::dynamic_pointer_cast<objects::Demon>(
      libcomp::PersistentObject::GetObjectByUUID(
          state->GetObjectUUID(demonID)));
  if (dQuest && demon && dQuest->GetDemon() == demon->GetUUID() &&
      dQuest->Register(dQuest)) {
    reply.WriteS8(0);  // Success
    success = true;

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());

    // Make sure the character updates
    character->SetDemonQuest(dQuest);

    dbChanges->Insert(dQuest);
    dbChanges->Update(character);

    int8_t questDaily = progress->GetDemonQuestDaily();
    questDaily = (int8_t)(questDaily + 1);
    if (questDaily >= 3) {
      // Demon quests have a max daily limit of 3, remove all still
      // active quests that re not the one that was just accepted
      for (auto& d : character->GetCOMP()->GetDemons()) {
        if (!d.IsNull() && d.Get() != demon && d->GetHasQuest()) {
          d->SetHasQuest(false);

          dbChanges->Update(d.Get());
        }
      }
    }

    progress->SetDemonQuestDaily(questDaily);

    dbChanges->Update(progress);

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
  } else {
    reply.WriteS8(-1);  // Failed
  }

  reply.WriteS64Little(demonID);
  reply.WriteS8(progress->GetDemonQuestDaily());

  client->QueuePacket(reply);

  // Perform any remaining setup needed on success
  if (success) {
    // Add the timer status effect
    StatusEffectChanges effects;
    effects[SVR_CONST.STATUS_DEMON_QUEST_ACTIVE] =
        StatusEffectChange(SVR_CONST.STATUS_DEMON_QUEST_ACTIVE, 1, true);

    server->GetCharacterManager()->AddStatusEffectImmediate(client, cState,
                                                            effects);

    switch (dQuest->GetType()) {
      case objects::DemonQuest::Type_t::KILL:
        // Register enemies that need to be killed
        server->GetEventManager()->UpdateQuestTargetEnemies(client);
        break;
      case objects::DemonQuest::Type_t::ITEM:
        // If the items are already in the inventory, update the count
        server->GetEventManager()->UpdateDemonQuestCount(
            client, objects::DemonQuest::Type_t::ITEM);
        break;
      default:
        break;
    }
  }

  client->FlushOutgoing();

  return true;
}
