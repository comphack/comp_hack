/**
 * @file server/channel/src/packets/game/DemonQuestPending.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to retrieve pending demon quest info.
 *  This is seemingly an old packet as pending quests must always be
 *  accepted or rejected before this request would ever be sent. The
 *  client also does not appear to respond to this request properly as
 *  a request window will open but no data will display within it.
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
#include <CharacterProgress.h>
#include <Demon.h>
#include <DemonBox.h>
#include <DemonQuest.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::DemonQuestPending::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 0) {
    return false;
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto dQuest = character->GetDemonQuest().Get();
  auto progress = character->GetProgress().Get();

  auto demon =
      dQuest
          ? std::dynamic_pointer_cast<objects::Demon>(
                libcomp::PersistentObject::GetObjectByUUID(dQuest->GetDemon()))
          : nullptr;

  libcomp::Packet reply;
  reply.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_DEMON_QUEST_PENDING);

  bool questNotAccepted = false;  // Not possible/supported
  reply.WriteU8(questNotAccepted ? 1 : 0);
  /*if(questNotAccepted)
  {
      reply.WriteS64Little(state->GetObjectID(demon->GetUUID()));

      reply.WriteS8((int8_t)dQuest->GetType());

      bool hasTargetCount = dQuest->GetTargetCount() > 0;
      reply.WriteS8(hasTargetCount ? 1 : 0);
      if(hasTargetCount)
      {
          reply.WriteU32Little(dQuest->GetTargetType());
          reply.WriteS32Little(dQuest->GetTargetCount());
      }

      bool itemReward = dQuest->GetRewardType() > 0;
      reply.WriteS8(itemReward ? 1 : 0);
      if(itemReward)
      {
          reply.WriteU32Little(dQuest->GetRewardType());
          reply.WriteU16Little(dQuest->GetRewardCount());
      }

      reply.WriteS32Little(dQuest->GetXPReward());

      bool hasBonus = dQuest->GetBonusType() !=
          objects::DemonQuest::BonusType_t::NONE;
      reply.WriteS8(hasBonus ? 1 : 0);
      if(hasBonus)
      {
          reply.WriteS8((int8_t)dQuest->GetBonusType());
          reply.WriteU32Little(dQuest->GetBonusSubType());
          reply.WriteU16Little(dQuest->GetBonusCount());
      }

      reply.WriteS16Little((int16_t)progress->GetDemonQuestSequence());

      reply.WriteU32Little(0);   // Unknown
      reply.WriteU32Little(0);   // Unknown
  }*/

  client->SendPacket(reply);

  return true;
}
