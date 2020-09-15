/**
 * @file server/channel/src/packets/game/CultureStart.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to start using a culture machine.
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
#include <CultureData.h>
#include <EventInstance.h>
#include <EventState.h>
#include <Item.h>
#include <ItemBox.h>
#include <ServerCultureMachineSet.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "CultureMachineState.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::CultureStart::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 8) {
    return false;
  }

  int64_t itemID = p.ReadS64Little();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto cData = character ? character->GetCultureData().Get() : nullptr;
  auto zone = cState->GetZone();

  auto item = std::dynamic_pointer_cast<objects::Item>(
      libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

  auto currentEvent = state->GetEventState()->GetCurrent();
  auto cmState =
      currentEvent && zone
          ? zone->GetCultureMachine(currentEvent->GetSourceEntityID())
          : nullptr;

  // Item must be specified, machine must not be rented already and current
  // character must not have an active rental
  bool success = character && item && cmState &&
                 cmState->GetRentalData() == nullptr &&
                 (cData == nullptr || !cData->GetActive());
  if (success) {
    // Pay the cost if one exists
    uint16_t cost = cmState->GetEntity()->GetCost();
    if (cost) {
      std::unordered_map<uint32_t, uint32_t> payment;
      payment[SVR_CONST.ITEM_KREUZ] = cost;
      success =
          server->GetCharacterManager()->AddRemoveItems(client, payment, false);
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CULTURE_START);
  if (success) {
    auto def = cmState->GetEntity();
    uint32_t timeLeft = (uint32_t)(def->GetDays() * 24 * 60 * 60);
    uint32_t timestamp = (uint32_t)time(0);
    uint32_t expirationTime = (uint32_t)(timestamp + (uint32_t)timeLeft);

    auto worldDB = server->GetWorldDatabase();

    bool isNew = cData == nullptr;
    if (isNew) {
      cData = libcomp::PersistentObject::New<objects::CultureData>(true);
      cData->SetCharacter(character->GetUUID());
    } else {
      // Reset default values
      for (size_t i = 0; i < cData->PointsCount(); i++) {
        cData->SetPoints(i, 0);
      }

      for (size_t i = 0; i < cData->ItemHistoryCount(); i++) {
        cData->SetItemHistory(i, 0);
      }

      cData->SetItemCount(0);
    }

    cData->SetZone(zone->GetDefinitionID());
    cData->SetMachineID(cmState->GetMachineID());
    cData->SetItem(item);

    cData->SetExpiration(expirationTime);
    cData->SetActive(true);

    auto dbChanges = libcomp::DatabaseChangeSet::Create();
    if (isNew) {
      character->SetCultureData(cData);

      dbChanges->Insert(cData);
      dbChanges->Update(character);
    } else {
      dbChanges->Update(cData);
    }

    int8_t oldSlot = item->GetBoxSlot();

    auto box = std::dynamic_pointer_cast<objects::ItemBox>(
        libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
    if (box && box->GetItems((size_t)oldSlot).Get() == item) {
      box->SetItems((size_t)oldSlot, NULLUUID);
      dbChanges->Update(box);
    }

    item->SetBoxSlot(-1);
    item->SetItemBox(NULLUUID);

    dbChanges->Update(item);

    if (!worldDB->ProcessChangeSet(dbChanges)) {
      auto accountUID = state->GetAccountUID();
      LogGeneralError([accountUID]() {
        return libcomp::String("CultureData failed to save: %1\n")
            .Arg(accountUID.ToString());
      });

      client->Kill();
      return true;
    }

    if (box && oldSlot != -1) {
      server->GetCharacterManager()->SendItemBoxData(client, box,
                                                     {(uint16_t)oldSlot});
    }

    cmState->SetRentalData(cData);

    auto zoneManager = server->GetZoneManager();

    // Send new rental information
    zoneManager->SendCultureMachineData(zone, cmState);

    // Expire existing and recalculate
    zoneManager->ExpireRentals(zone);

    reply.WriteS8(0);  // Success
    reply.WriteS32Little((int32_t)timeLeft);

    LogItemDebug([cData, zone, timeLeft]() {
      return libcomp::String(
                 "Character started culture machine %1 in zone %2 for %3 "
                 "seconds: %4\n")
          .Arg(cData->GetMachineID())
          .Arg(zone->GetDefinitionID())
          .Arg(timeLeft)
          .Arg(cData->GetCharacter().ToString());
    });
  } else {
    reply.WriteS8(-1);  // Failure
    reply.WriteS32Little(-1);
  }

  client->SendPacket(reply);

  return true;
}
