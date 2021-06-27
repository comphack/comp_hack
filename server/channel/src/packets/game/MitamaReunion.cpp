/**
 * @file server/channel/src/packets/game/MitamaReunion.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to perform a mitama reunion reinforcement.
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
#include <Randomizer.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <DemonBox.h>
#include <MiDevilData.h>
#include <MiDevilLVUpRateData.h>
#include <MiGrowthData.h>
#include <MiMitamaReunionBonusData.h>
#include <MiUnionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "FusionManager.h"
#include "TokuseiManager.h"

using namespace channel;

static void HandleMitamaReunion(
    const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client, int64_t mitamaID,
    int8_t reunionIdx) {
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();
  auto fusionManager = server->GetFusionManager();

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto demon = dState->GetEntity();
  auto demonData = dState->GetDevilData();

  auto mitama = mitamaID ? std::dynamic_pointer_cast<objects::Demon>(
                               libcomp::PersistentObject::GetObjectByUUID(
                                   state->GetObjectUUID(mitamaID)))
                         : nullptr;
  auto mitamaData =
      mitama ? definitionManager->GetDevilData(mitama->GetType()) : nullptr;

  bool isMitama = characterManager->IsMitamaDemon(demonData);

  int8_t bonusID = 0;
  size_t newIdx = 0;

  uint8_t mPoints = (uint8_t)(demon ? (12 + demon->GetMitamaRank()) : 0);

  bool success = demon && mitama && demon != mitama && isMitama && mitamaData &&
                 reunionIdx >= 0 && reunionIdx < 12;
  if (success) {
    auto reunion = demon->GetReunion();
    auto mReunion = demon->GetMitamaReunion();

    // Check default growth type rank 1
    auto defaultGrowthType = definitionManager->GetDevilLVUpRateData(
        demonData->GetGrowth()->GetGrowthType());
    if (defaultGrowthType && defaultGrowthType->GetGroupID() > 0 &&
        reunion[(size_t)(defaultGrowthType->GetGroupID() - 1)] == 0) {
      reunion[(size_t)(defaultGrowthType->GetGroupID() - 1)] = 1;
    }

    int8_t rTotal = 0;
    std::array<uint8_t, 4> mitamaTotals = {{0, 0, 0, 0}};
    for (size_t i = 0; i < 96; i++) {
      uint8_t bonus = mReunion[i];
      if (bonus) {
        uint8_t idx = (uint8_t)(bonus / 32);
        mitamaTotals[idx]++;

        if ((int32_t)(i / 8) == (int32_t)reunionIdx) {
          rTotal++;
        }
      }
    }

    size_t mitamaIdx = fusionManager->GetMitamaIndex(
        mitamaData->GetUnionData()->GetBaseDemonID(), success);

    success &= mitamaTotals[mitamaIdx] < mPoints &&
               rTotal < reunion[(size_t)reunionIdx];
    if (success) {
      // Generate bonus
      uint32_t startIdx = (uint32_t)(mitamaIdx * 32);

      std::list<std::shared_ptr<objects::MiMitamaReunionBonusData>> bonuses;
      for (uint32_t i = startIdx; i < (uint32_t)(startIdx + 32); i++) {
        auto bonus = definitionManager->GetMitamaReunionBonusData(i);
        if (bonus && bonus->GetValue() > 0) {
          bonuses.push_back(bonus);
        }
      }

      auto bonus = libcomp::Randomizer::GetEntry(bonuses);
      if (bonus) {
        bonusID = (int8_t)bonus->GetID();
      } else {
        success = false;
      }
    }

    if (success) {
      // Request valid, pay the cost
      success =
          characterManager->PayMacca(client, (uint64_t)((rTotal + 1) * 50000));
    }

    if (success) {
      // Add the bonus
      newIdx = (size_t)(reunionIdx * 8 + rTotal);
      demon->SetMitamaReunion(newIdx, (uint8_t)bonusID);
      characterManager->CalculateDemonBaseStats(demon);

      auto dbChanges =
          libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
      dbChanges->Update(demon);

      // Delete the mitama
      int8_t slot = mitama->GetBoxSlot();
      auto box = std::dynamic_pointer_cast<objects::DemonBox>(
          libcomp::PersistentObject::GetObjectByUUID(mitama->GetDemonBox()));

      characterManager->DeleteDemon(mitama, dbChanges);
      if (box) {
        characterManager->SendDemonBoxData(client, box->GetBoxID(), {slot});
      }

      server->GetWorldDatabase()->QueueChangeSet(dbChanges);

      dState->UpdateDemonState(definitionManager);
      server->GetTokuseiManager()->Recalculate(
          cState, true, std::set<int32_t>{dState->GetEntityID()});
      characterManager->RecalculateStats(dState, client);
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MITAMA_REUNION);
  reply.WriteS8(success ? 0 : -1);
  reply.WriteS8(reunionIdx);
  reply.WriteS8((int8_t)newIdx);
  reply.WriteU8((uint8_t)bonusID);

  if (success) {
    characterManager->GetEntityStatsPacketData(
        reply, demon->GetCoreStats().Get(), dState, 1);
    reply.WriteS8(0);
  }

  client->SendPacket(reply);
}

bool Parsers::MitamaReunion::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 9) {
    return false;
  }

  int64_t mitamaID = p.ReadS64Little();
  int8_t reunionIdx = p.ReadS8();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

  HandleMitamaReunion(server, client, mitamaID, reunionIdx);

  return true;
}
