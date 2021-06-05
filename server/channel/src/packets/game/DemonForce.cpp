/**
 * @file server/channel/src/packets/game/DemonForce.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to consume a demon force item.
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
#include <Constants.h>
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>

// object Includes
#include <Item.h>
#include <MiDevilBoostData.h>
#include <MiDevilBoostItemData.h>
#include <MiDevilBoostRequirementData.h>
#include <MiDevilBoostResultData.h>
#include <MiDevilData.h>
#include <MiGrowthData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::DemonForce::Parse(
    libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection> &connection,
    libcomp::ReadOnlyPacket &p) const {
  if (p.Size() < 21) {
    return false;
  }

  int64_t demonID = p.ReadS64Little();
  int64_t itemID = p.ReadS64Little();
  uint32_t dfType = p.ReadU32Little();

  bool toStack = p.ReadS8() == 1;
  int8_t stackSlot = -1;
  if (toStack) {
    if (p.Left() == 1) {
      stackSlot = p.ReadS8();
    } else {
      return false;
    }
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto dState = state->GetDemonState();
  auto devilData = dState->GetDevilData();
  auto demon = dState->GetEntity();

  auto item = std::dynamic_pointer_cast<objects::Item>(
      libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

  auto dfData = definitionManager->GetDevilBoostData(dfType);
  auto dfItem = item ? definitionManager->GetDevilBoostItemData(item->GetType())
                     : nullptr;

  std::unordered_map<int8_t, int32_t> boosted;

  bool success = dfData && dfItem && demon &&
                 state->GetObjectID(demon->GetUUID()) == demonID &&
                 (!toStack || (stackSlot >= 0 && stackSlot <= 7)) &&
                 demon->GetForceStackPending() == 0;
  if (success) {
    // Verify that the item consumed has the force effect
    bool dfValid = false;
    for (uint32_t boostID : dfItem->GetBoostIDs()) {
      if (boostID == dfType) {
        dfValid = true;
        break;
      }
    }

    if (!dfValid) {
      success = false;
    }
  }

  if (success) {
    // Check boost restrictions
    int8_t lvl = demon->GetCoreStats()->GetLevel();
    if ((dfData->GetMinLevel() > 0 && dfData->GetMinLevel() > lvl) ||
        (dfData->GetMaxLevel() > 0 && dfData->GetMaxLevel() < lvl)) {
      success = false;
    }

    if (dfData->GetGrowthType() > 0) {
      int8_t rank = demon->GetReunion((size_t)(dfData->GetGrowthType() - 1));
      if (rank < dfData->GetGrowthRank()) {
        success = false;
      }
    }

    if (toStack && demon->GetForceStack((size_t)stackSlot) == 0 &&
        devilData->GetGrowth()->GetForceStack() < (stackSlot + 1)) {
      success = false;
    }

    for (auto req : dfData->GetRequirements()) {
      switch (req->GetType()) {
      case objects::MiDevilBoostRequirementData::Type_t::LNC:
        // LAW/NEUTRAL/CHAOS = 0/1/2
        if ((int32_t)(dState->GetLNCType() / 2) != req->GetValue1()) {
          success = false;
        }
        break;
      case objects::MiDevilBoostRequirementData::Type_t::FAMILIARITY: {
        // Familiarity requirements are listed as 1-8 from max
        // to min rank
        int32_t fRank = (int32_t)(characterManager->GetFamiliarityRank(
                                      demon->GetFamiliarity()) +
                                  3);
        if ((req->GetValue1() > 0 && (8 - req->GetValue1()) > fRank) ||
            (req->GetValue2() > 0 && (8 - req->GetValue2()) < fRank)) {
          success = false;
        }
      } break;
      case objects::MiDevilBoostRequirementData::Type_t::NONE:
      default:
        break;
      }
    }
  }

  bool statRaised = false;
  uint16_t pendingEffect = 0;
  if (success) {
    // As long as a force stack effect is set or a value is raised, the
    // force operation has succeeded
    bool resultExists = false;

    // Items typically apply normal caps, cap at 1000 points just in case
    const int32_t rMax = 1000 * DEMON_FORCE_PRECISION;
    for (auto result : dfData->GetResults()) {
      // Make sure its a value effect index
      int8_t rType = result->GetType();
      if (rType >= 0 && rType <= 19) {
        // Check requirements and max values
        int32_t points = demon->GetForceValues((size_t)rType);
        if (points < rMax &&
            (result->GetMinPoints() < 0 || result->GetMinPoints() <= points) &&
            (result->GetMaxPoints() < 0 || result->GetMaxPoints() >= points)) {
          // Add points
          if ((points + result->GetPoints()) > rMax) {
            boosted[rType] = rMax;
          } else {
            boosted[rType] = points + result->GetPoints();
          }

          statRaised |= (boosted[rType] / DEMON_FORCE_PRECISION) !=
                        (points / DEMON_FORCE_PRECISION);
        }

        resultExists = true;
      }
    }

    std::unordered_map<uint32_t, uint32_t> items;
    items[item->GetType()] = 1;

    if ((!resultExists || boosted.size() > 0 || toStack) &&
        characterManager->AddRemoveItems(client, items, false, itemID)) {
      // Perform the update
      auto dbChanges =
          libcomp::DatabaseChangeSet::Create(state->GetAccountUID());

      for (auto &bPair : boosted) {
        demon->SetForceValues((size_t)bPair.first, bPair.second);
      }

      if (toStack) {
        demon->SetForceStack((size_t)stackSlot, dfData->GetExtraID());
      }

      int32_t bGauge = demon->GetBenefitGauge();
      bGauge++;

      demon->SetBenefitGauge(bGauge);

      auto lotIDs = definitionManager->GetDevilBoostLotIDs(bGauge);
      if (lotIDs.size() > 0) {
        // Determine which effect will become pending
        std::set<uint16_t> existingIDs;
        for (uint16_t existing : demon->GetForceStack()) {
          if (existing) {
            existingIDs.insert(existing);
          }
        }

        // Remove dupes
        lotIDs.remove_if([existingIDs](uint16_t val) {
          return existingIDs.find(val) != existingIDs.end();
        });

        pendingEffect = libcomp::Randomizer::GetEntry(lotIDs);
        if (pendingEffect) {
          demon->SetForceStackPending(pendingEffect);
        }
      }

      server->GetWorldDatabase()->QueueUpdate(demon, state->GetAccountUID());
    } else {
      success = false;
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_FORCE);
  reply.WriteS64Little(demonID);
  reply.WriteS8(success ? 0 : -1);
  if (success) {
    reply.WriteS32Little(demon->GetBenefitGauge());

    reply.WriteS8((int8_t)boosted.size());

    for (auto &bPair : boosted) {
      reply.WriteS8(bPair.first);
      reply.WriteS32Little(bPair.second);

      // If base or derived stat is not sent, the value will drop to 0. Oddly
      // enough, this doesn't happen if multiple stats update at once.
      switch ((CorrectTbl)libhack::DEMON_FORCE_CONVERSION[bPair.first]) {
      case CorrectTbl::STR:
        reply.WriteS16(demon->GetCoreStats()->GetSTR());
        break;
      case CorrectTbl::MAGIC:
        reply.WriteS16(demon->GetCoreStats()->GetMAGIC());
        break;
      case CorrectTbl::VIT:
        reply.WriteS16(demon->GetCoreStats()->GetVIT());
        break;
      case CorrectTbl::INT:
        reply.WriteS16(demon->GetCoreStats()->GetINTEL());
        break;
      case CorrectTbl::SPEED:
        reply.WriteS16(demon->GetCoreStats()->GetSPEED());
        break;
      case CorrectTbl::LUCK:
        reply.WriteS16(demon->GetCoreStats()->GetLUCK());
        break;
      case CorrectTbl::CLSR:
        reply.WriteS16(demon->GetCoreStats()->GetCLSR());
        break;
      case CorrectTbl::LNGR:
        reply.WriteS16(demon->GetCoreStats()->GetLNGR());
        break;
      case CorrectTbl::SPELL:
        reply.WriteS16(demon->GetCoreStats()->GetSPELL());
        break;
      case CorrectTbl::SUPPORT:
        reply.WriteS16(demon->GetCoreStats()->GetSUPPORT());
        break;
      case CorrectTbl::PDEF:
        reply.WriteS16(demon->GetCoreStats()->GetPDEF());
        break;
      case CorrectTbl::MDEF:
        reply.WriteS16(demon->GetCoreStats()->GetMDEF());
        break;
      default:
        reply.WriteS16(0); // Not necessary
        break;
      }
    }

    reply.WriteS8(stackSlot);

    if (stackSlot >= 0) {
      reply.WriteU16Little(dfData->GetExtraID());
    }

    reply.WriteU16Little(pendingEffect);

    if (pendingEffect) {
      libcomp::Packet notify;
      notify.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_DEMON_FORCE_GAUGE);
      notify.WriteS32Little(cState->GetEntityID());

      server->GetZoneManager()->BroadcastPacket(client, notify, true);
    }
  }

  client->SendPacket(reply);

  if (success && (statRaised || toStack)) {
    dState->UpdateDemonState(definitionManager);
    server->GetTokuseiManager()->Recalculate(
        state->GetCharacterState(), true,
        std::set<int32_t>{dState->GetEntityID()});
    characterManager->RecalculateStats(dState, client);
  }

  return true;
}
