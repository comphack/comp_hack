/**
 * @file server/channel/src/packets/game/ReunionInject.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to inject reunion conversion points into
 *  a demon.
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
#include <AccountWorldData.h>
#include <MiDevilData.h>
#include <MiGrowthData.h>
#include <MiNPCBasicData.h>
#include <MiUnionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "FusionTables.h"

using namespace channel;

void InjectReunionPoints(const std::shared_ptr<ChannelServer> server,
                         const std::shared_ptr<ChannelClientConnection> client,
                         uint8_t growthType, uint8_t mitamaType,
                         std::array<int8_t, 12> rPointSet,
                         std::array<int8_t, 12> mPointSet) {
  auto characterManager = server->GetCharacterManager();

  auto state = client->GetClientState();
  auto awd = state->GetAccountWorldData().Get();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto dState = state->GetDemonState();
  auto demon = dState->GetEntity();
  auto demonData = dState->GetDevilData();

  int32_t rPoints = 0, mPoints = 0;

  bool isMitama = characterManager->IsMitamaDemon(demonData);
  uint32_t newDemonType =
      demonData ? (isMitama ? demonData->GetBasic()->GetID()
                            : demonData->GetUnionData()->GetMitamaFusionID())
                : 0;

  bool success = awd && demon && demonData;
  if (success) {
    rPoints = (int32_t)awd->GetReunionPoints();
    mPoints = (int32_t)awd->GetMitamaReunionPoints();

    for (size_t rIdx = 0; rIdx < 12; rIdx++) {
      if (!isMitama) {
        // Increase pre-mitama costs
        int8_t setVal = rPointSet[rIdx];
        int8_t rank = demon->GetReunion(rIdx);
        if (setVal > 0 && rank < setVal) {
          for (int8_t p = rank + 1; p <= setVal && p < 10; p++) {
            rPoints = (int32_t)(rPoints - REUNION_RANK_POINTS[(size_t)p]);
          }
        }
      }

      if (mitamaType) {
        // Increase post-mitama costs
        int8_t setVal = mPointSet[rIdx];
        int8_t rank = (int8_t)demon->GetMitamaReunion(rIdx);
        if (setVal > 0 && rank < setVal) {
          for (int8_t p = rank + 1; p <= setVal && p < 10; p++) {
            mPoints = (int32_t)(mPoints - REUNION_RANK_POINTS[(size_t)p]);
          }
        }
      }
    }

    success = rPoints >= 0 && mPoints >= 0;
  }

  if (success) {
    // Apply points and convert demon now
    if (!isMitama) {
      // Backup reunion points in case mitama conversion fails
      auto reunion = demon->GetReunion();

      // Apply pre-mitama points and growth type
      for (size_t rIdx = 0; rIdx < 12; rIdx++) {
        int8_t setVal = rPointSet[rIdx];
        int8_t rank = demon->GetReunion(rIdx);
        if (setVal > 0 && rank < setVal) {
          demon->SetReunion(rIdx, setVal);
        }
      }

      if (mitamaType) {
        // Convert to mitama
        if (!characterManager->MitamaDemon(client,
                                           state->GetObjectID(demon->GetUUID()),
                                           growthType, mitamaType)) {
          demon->SetReunion(reunion);
          success = false;
        }
      }
    }

    if (success) {
      // If no error has occured yet, store the demon and apply
      // any post mitama points
      characterManager->StoreDemon(client);

      // Set growth and mitama type in case they changed
      demon->SetGrowthType(growthType);
      demon->SetMitamaType(mitamaType);

      if (mitamaType) {
        // Apply post-mitama points
        for (size_t mIdx = 0; mIdx < 12; mIdx++) {
          // If mitama conversion occurred, reset all reunion
          // points to 0 by default
          if (!isMitama) {
            demon->SetReunion(mIdx, 0);
          }

          int8_t setVal = mPointSet[mIdx];
          int8_t rank = (int8_t)demon->GetMitamaReunion(mIdx);
          if (setVal > 0 && rank < setVal) {
            demon->SetReunion(mIdx, setVal);
          }
        }
      }
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REUNION_INJECT);
  reply.WriteS32Little(0);  // Unknown
  reply.WriteS32Little(success ? 0 : -1);
  reply.WriteS32Little(rPoints);
  reply.WriteS32Little(mPoints);
  reply.WriteU32Little(demonData ? demonData->GetBasic()->GetID() : 0);
  reply.WriteU32Little(newDemonType);

  client->QueuePacket(reply);

  if (success) {
    // Set the points and recalc the demon
    awd->SetReunionPoints((uint32_t)rPoints);
    awd->SetMitamaReunionPoints((uint32_t)mPoints);

    // Recalculate demon stats
    dState->UpdateDemonState(server->GetDefinitionManager());
    characterManager->CalculateDemonBaseStats(demon);
    characterManager->SendDemonData(client, 0, demon->GetBoxSlot(),
                                    state->GetObjectID(demon->GetUUID()));

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
    dbChanges->Update(awd);
    dbChanges->Update(demon);

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
  }

  client->FlushOutgoing();
}

bool Parsers::ReunionInject::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 30) {
    return false;
  }

  int32_t unknown = p.ReadS32Little();
  (void)unknown;  // Always 0

  uint8_t growthType = p.ReadU8();
  uint8_t mitamaType = p.ReadU8();

  std::array<int8_t, 12> rPointSet;
  std::array<int8_t, 12> mPointSet;

  for (size_t i = 0; i < 12; i++) {
    rPointSet[i] = p.ReadS8();
    mPointSet[i] = p.ReadS8();
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

  server->QueueWork(InjectReunionPoints, server, client, growthType, mitamaType,
                    rPointSet, mPointSet);

  return true;
}
