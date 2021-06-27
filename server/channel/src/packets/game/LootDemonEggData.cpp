/**
 * @file server/channel/src/packets/game/LootDemonEggData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for information about the demon in a
 *  demon egg.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Loot.h>
#include <LootBox.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::LootDemonEggData::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 4) {
    return false;
  }

  int32_t lootEntityID = p.ReadS32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();
  auto cState = state->GetCharacterState();
  auto zone = cState->GetZone();

  auto lState = zone ? zone->GetLootBox(lootEntityID) : nullptr;
  auto enemy = lState ? lState->GetEntity()->GetEnemy() : nullptr;
  if (enemy) {
    uint32_t demonType = enemy->GetType();
    auto demonData = definitionManager->GetDevilData(demonType);
    auto tempDemon = characterManager->GenerateDemon(demonData);
    auto cs = tempDemon->GetCoreStats().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_LOOT_DEMON_EGG_DATA);
    reply.WriteU32Little(demonType);
    reply.WriteS16Little((int16_t)cs->GetMaxHP());
    reply.WriteS16Little((int16_t)cs->GetMaxMP());
    reply.WriteS8(cs->GetLevel());
    characterManager->GetEntityStatsPacketData(reply, cs, nullptr, 0);

    reply.WriteS32Little((int32_t)tempDemon->LearnedSkillsCount());
    for (uint32_t skillID : tempDemon->GetLearnedSkills()) {
      reply.WriteU32Little(skillID == 0 ? (uint32_t)-1 : skillID);
    }
    reply.WriteS8(-1);  // Unknown

    connection->SendPacket(reply);
  }

  return true;
}
