/**
 * @file server/channel/src/packets/game/CultureItem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to add an item to increase culture machine
 *  success.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <CultureData.h>
#include <EventInstance.h>
#include <EventState.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiCultureItemData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiSkillData.h>
#include <MiSkillSpecialParams.h>
#include <MiUseRestrictionsData.h>
#include <ServerCultureMachineSet.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "CultureMachineState.h"
#include "ZoneManager.h"

using namespace channel;

void HandleCultureItem(const std::shared_ptr<ChannelServer> server,
                       const std::shared_ptr<ChannelClientConnection> client,
                       int64_t itemID, int8_t day) {
  const int8_t PMODE_NORMAL = 0;
  const int8_t PMODE_MAX = 1;
  const int8_t PMODE_MAX_ALL = 2;

  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  auto state = client->GetClientState();
  if (state->GetExchangeSession()) {
    // The client is in some kind of transaction with another. Kill their
    // connection, as this is probably a packet injection attemnpt.
    LogGeneralError([&]() {
      return libcomp::String(
                 "Player attempted to add a cultivation item while in the "
                 "middle of a transaction with another player: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    client->Kill();

    return;
  }

  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto inventory = character->GetItemBoxes(0).Get();
  auto cData = character ? character->GetCultureData().Get() : nullptr;
  auto cItem = cData ? cData->GetItem().Get() : nullptr;
  auto zone = cState->GetZone();

  auto item = std::dynamic_pointer_cast<objects::Item>(
      libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

  auto currentEvent = state->GetEventState()->GetCurrent();
  auto cmState =
      currentEvent && zone
          ? zone->GetCultureMachine(currentEvent->GetSourceEntityID())
          : nullptr;
  auto cmDef = cmState ? cmState->GetEntity() : nullptr;

  bool match = cData && cData == cmState->GetRentalData();
  bool success = cmState && cItem && item &&
                 (item->GetItemBox() == inventory->GetUUID()) && match;

  int8_t pointMode = PMODE_NORMAL;

  int32_t points = 0;
  std::set<uint8_t> daysLeft;
  if (success) {
    // Calculate points
    auto itemData = definitionManager->GetItemData(item->GetType());
    auto cultureItem = definitionManager->GetCultureItemData(item->GetType());
    if (cultureItem) {
      // Special item values
      switch (cultureItem->GetPoints()) {
        case static_cast<uint32_t>(-1):
          pointMode = PMODE_MAX;
          break;
        case static_cast<uint32_t>(-2):
          pointMode = PMODE_MAX_ALL;
          break;
        default:
          points = (int32_t)cultureItem->GetPoints();
          break;
      }
    } else {
      if (CharacterManager::IsCPItem(itemData)) {
        // Base point value is fixed
        points = 200000;
      } else {
        // Base point value determined by sell price
        points = itemData ? itemData->GetBasic()->GetSellPrice() : 0;
      }
    }

    // Determine which days are valid (starting at day 0 for less than
    // 24 hours passed)
    int32_t remaining = (int32_t)ceil(
        (double)ChannelServer::GetExpirationInSeconds(cData->GetExpiration()) /
        (double)(24 * 60 * 60));
    for (uint8_t i = 0; i < cmDef->GetDays(); i++) {
      if ((int32_t)(cmDef->GetDays() - i) <= remaining) {
        daysLeft.insert(i);
      }
    }

    uint16_t dayRate =
        daysLeft.size() > 0 ? cmDef->GetDailyItemRates(*daysLeft.begin()) : 0;
    if (dayRate && pointMode == PMODE_NORMAL) {
      // Calculate multipliers
      /// @todo: calculation is close but not exact
      double passiveBoost = 1.0;
      double demonBoost = 1.0;

      for (uint32_t skillID :
           definitionManager->GetFunctionIDSkills(SVR_CONST.SKILL_CULTURE_UP)) {
        if (cState->CurrentSkillsContains(skillID)) {
          auto skillData = definitionManager->GetSkillData(skillID);
          int32_t boost =
              skillData ? skillData->GetSpecial()->GetSpecialParams(0) : 0;
          passiveBoost = passiveBoost + (double)boost * 0.01;
        }
      }

      auto dState = state->GetDemonState();
      if (dState->Ready()) {
        int16_t intel = dState->GetINTEL();
        int16_t luck = dState->GetLUCK();
        for (auto pair : SVR_CONST.ADJUSTMENT_SKILLS) {
          if (pair.second[0] == 4 && pair.second[1] == 2 &&
              dState->CurrentSkillsContains(pair.first)) {
            demonBoost = demonBoost + (double)(intel * luck) / 100000.0;
          }
        }
      }

      // "Distance" from full moon reduces point boost
      auto worldClock = server->GetWorldClockTime();
      int8_t phaseDelta = (int8_t)abs(8 - worldClock.MoonPhase);

      // Same type and same gender items give a flat boost
      auto cItemData = definitionManager->GetItemData(cItem->GetType());

      bool sameType = itemData->GetBasic()->GetEquipType() ==
                      cItemData->GetBasic()->GetEquipType();
      bool sameGender = itemData->GetRestriction()->GetGender() == GENDER_NA ||
                        cItemData->GetRestriction()->GetGender() == GENDER_NA ||
                        itemData->GetRestriction()->GetGender() ==
                            cItemData->GetRestriction()->GetGender();

      // Calculate final point value
      double calc = (double)points;

      calc = floor(calc * passiveBoost);

      calc = floor(calc * demonBoost);

      if (sameType) {
        calc = calc * 1.25;
      }

      if (sameGender) {
        calc = calc * 1.25;
      }

      points = (int32_t)ceil(calc * (1.15 - (double)phaseDelta * 0.07) *
                             (double)dayRate * 0.01);
    }
  }

  if (success && pointMode != PMODE_MAX_ALL &&
      (day < 0 || daysLeft.find((uint8_t)day) == daysLeft.end())) {
    LogGeneralError([&]() {
      return libcomp::String(
                 "Day '%1' is no longer valid for CultureItem request: %2\n")
          .Arg(day)
          .Arg(state->GetAccountUID().ToString());
    });

    success = false;
  }

  int32_t expertPoints = 0;
  if (success) {
    // Consume item and update points upon success
    std::unordered_map<uint32_t, uint32_t> consumed;
    consumed[item->GetType()] = 1;
    if (characterManager->AddRemoveItems(client, consumed, false, itemID)) {
      uint32_t maxDaily = cmDef->GetMaxDailyPoints();
      switch (pointMode) {
        case PMODE_NORMAL:
          // Increase day points by value, raise expertise
          {
            uint32_t oldPoints = cData->GetPoints((size_t)day);
            uint32_t newPoints = (uint32_t)(oldPoints + (uint32_t)points);

            // Points cannot go down
            if (newPoints >= oldPoints) {
              if (newPoints > cmDef->GetMaxDailyPoints()) {
                newPoints = cmDef->GetMaxDailyPoints();
              }

              cData->SetPoints((size_t)day, newPoints);

              // Calculate expertise point gain
              /// @todo: determine proper calculation
              double demonBoost = 1.0;

              auto dState = state->GetDemonState();
              if (dState->Ready()) {
                int16_t intel = dState->GetINTEL();
                int16_t luck = dState->GetLUCK();
                for (auto pair : SVR_CONST.ADJUSTMENT_SKILLS) {
                  if (pair.second[0] == 4 && pair.second[1] == 1 &&
                      dState->CurrentSkillsContains(pair.first)) {
                    demonBoost = demonBoost + (double)(intel * luck) / 10000.0;
                  }
                }
              }

              expertPoints =
                  (int32_t)floor((double)points * demonBoost / 2000.0 *
                                 (double)cmDef->GetExpertiseRate() * 0.01);
            }
          }
          break;
        case PMODE_MAX:
          // Current day points set to max
          cData->SetPoints((size_t)day, maxDaily);
          break;
        case PMODE_MAX_ALL:
          // Remaining day points set to max
          for (uint8_t d : daysLeft) {
            cData->SetPoints(d, maxDaily);
          }
          break;
        default:
          break;
      }

      // Add the new item to the item history
      for (size_t i = (size_t)(cData->ItemHistoryCount() - 1); i > 0; i--) {
        cData->SetItemHistory(i, cData->GetItemHistory((size_t)(i - 1)));
      }

      cData->SetItemHistory(0, item->GetType());
      cData->SetItemCount((uint32_t)(cData->GetItemCount() + 1));

      server->GetWorldDatabase()->QueueUpdate(cData, state->GetAccountUID());
    } else {
      LogGeneralError([&]() {
        return libcomp::String(
                   "Failed to consume item for CultureItem request: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      success = false;
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CULTURE_ITEM);
  reply.WriteS8(success ? 0 : -1);

  if (match) {
    for (size_t i = 0; i < 5; i++) {
      reply.WriteS32Little((int32_t)cData->GetPoints(i));
    }

    reply.WriteS32Little(
        ChannelServer::GetExpirationInSeconds(cData->GetExpiration()));

    for (size_t i = 0; i < 10; i++) {
      uint32_t itemType = cData->GetItemHistory(i);
      reply.WriteU32Little(itemType ? itemType : 0xFFFFFFFF);
    }

    reply.WriteU32Little(cData->GetItemCount());
  }

  client->QueuePacket(reply);

  if (expertPoints) {
    // Update expertise points
    expertPoints = (int32_t)(
        (double)expertPoints *
        (double)cState->GetCorrectValue(CorrectTbl::RATE_EXPERTISE) * 0.01);

    std::list<std::pair<uint8_t, int32_t>> expPoints;
    expPoints.push_back(
        std::pair<uint8_t, int32_t>(EXPERTISE_CREATION, expertPoints));

    characterManager->UpdateExpertisePoints(client, expPoints);
  }

  client->FlushOutgoing();
}

bool Parsers::CultureItem::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 9) {
    return false;
  }

  int64_t itemID = p.ReadS64Little();
  int8_t day = p.ReadS8();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

  server->QueueWork(HandleCultureItem, server, client, itemID, day);

  return true;
}
