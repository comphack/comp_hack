/**
 * @file server/channel/src/packets/game/BazaarMarketOpen.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to open a market at a bazaar.
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
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <EventInstance.h>
#include <EventState.h>
#include <ServerZone.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "Zone.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::BazaarMarketOpen::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 4) {
    return false;
  }

  int32_t maccaCost = p.ReadS32Little();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto zoneManager = server->GetZoneManager();
  auto worldDB = server->GetWorldDatabase();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto zone = cState->GetZone();

  // Always reload
  auto bazaarData = objects::BazaarData::LoadBazaarDataByAccount(
      worldDB, state->GetAccountUID());

  auto currentEvent = state->GetEventState()->GetCurrent();
  uint32_t marketID = (uint32_t)state->GetCurrentMenuShopID();
  auto bazaar = currentEvent && zone
                    ? zone->GetBazaar(currentEvent->GetSourceEntityID())
                    : nullptr;

  // Check the cost from world setting, otherwise zone definition
  uint32_t actualCost = server->GetWorldSharedConfig()->GetBazaarMarketCost();
  if (!actualCost) {
    actualCost = zone->GetDefinition()->GetBazaarMarketCost();
  }

  bool reserved = bazaar && bazaar->ReserveMarket(marketID, false);
  bool success =
      reserved && marketID != 0 && bazaar && maccaCost == (int32_t)actualCost;
  if (success) {
    if (bazaarData && bazaarData->GetMarketID() == marketID &&
        bazaarData->GetZone() == zone->GetDefinitionID() &&
        bazaarData->GetChannelID() == server->GetChannelID()) {
      LogBazaarError([&]() {
        return libcomp::String(
                   "Player attempted to open the same bazaar market multiple "
                   "times in a row: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      success = false;
    } else if (maccaCost > 0) {
      success =
          server->GetCharacterManager()->PayMacca(client, (uint64_t)maccaCost);
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_OPEN);
  if (success) {
    // Market time uses world setting, otherwise zone definition
    uint32_t timeLeft = server->GetWorldSharedConfig()->GetBazaarMarketTime();
    if (!timeLeft) {
      timeLeft = zone->GetDefinition()->GetBazaarMarketTime();
    }

    // Convert to seconds
    timeLeft = (uint32_t)(timeLeft * 60);

    uint32_t timestamp = (uint32_t)time(0);
    uint32_t expirationTime = (uint32_t)(timestamp + (uint32_t)timeLeft);

    bool isNew = bazaarData == nullptr;
    if (isNew) {
      bazaarData = libcomp::PersistentObject::New<objects::BazaarData>(true);
      bazaarData->SetAccount(state->GetAccountUID());
      bazaarData->SetNPCType(1);
    }

    bazaarData->SetCharacter(cState->GetEntity());
    bazaarData->SetZone(zone->GetDefinition()->GetID());
    bazaarData->SetChannelID(server->GetChannelID());
    bazaarData->SetMarketID(marketID);
    bazaarData->SetState(objects::BazaarData::State_t::BAZAAR_PREPARING);

    bazaarData->SetExpiration(expirationTime);

    auto dbChanges = libcomp::DatabaseChangeSet::Create();
    if (isNew) {
      auto worldData = state->GetAccountWorldData().Get();
      worldData->SetBazaarData(bazaarData);

      dbChanges->Insert(bazaarData);
      dbChanges->Update(worldData);
    } else {
      dbChanges->Update(bazaarData);
    }

    if (!worldDB->ProcessChangeSet(dbChanges)) {
      LogBazaarError([&]() {
        return libcomp::String("BazaarData failed to save: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });

      client->Kill();

      // Roll back the reservation
      bazaar->ReserveMarket(marketID, true);

      return true;
    }

    bazaar->SetCurrentMarket(marketID, bazaarData);

    zoneManager->SendBazaarMarketData(zone, bazaar, marketID);

    // Refresh markets in the same zone
    zoneManager->ExpireRentals(zone);

    reply.WriteS32Little((int32_t)timeLeft);
    reply.WriteS32Little(0);  // Success

    LogBazaarDebug([bazaarData, zone, timeLeft]() {
      return libcomp::String(
                 "Player opened bazaar market %1 in zone %2 for %3 seconds: "
                 "%4\n")
          .Arg(bazaarData->GetMarketID())
          .Arg(zone->GetDefinitionID())
          .Arg(timeLeft)
          .Arg(bazaarData->GetAccount().GetUUID().ToString());
    });
  } else {
    reply.WriteS32Little(-1);
    reply.WriteS32Little(-1);  // Failure
  }

  connection->SendPacket(reply);

  // Lastly clear the reservation
  if (reserved) {
    bazaar->ReserveMarket(marketID, true);
  }

  return true;
}
