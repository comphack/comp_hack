/**
 * @file server/channel/src/packets/game/SpotTriggered.cpp
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Request from the client to handle a spot that has been triggered.
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
#include <PacketCodes.h>

// object includes
#include <ServerZone.h>
#include <ServerZoneSpot.h>

// Standard C++14 Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <gsl/gsl>

// channel Includes
#include "ActionManager.h"
#include "ChannelServer.h"
#include "ZoneManager.h"

using namespace channel;

class ActionListB {
 public:
  std::list<std::shared_ptr<objects::Action>> actions;
};

bool Parsers::SpotTriggered::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  // Sanity check the packet size.
  if ((5 * sizeof(uint32_t)) != p.Left()) {
    return false;
  }

  // Read the values from the packet.
  uint32_t entityID = p.ReadU32Little();
  uint32_t spotID = p.ReadU32Little();
  float x = p.ReadFloat();
  float y = p.ReadFloat();
  uint32_t zoneID = p.ReadU32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto zoneManager = server->GetZoneManager();
  auto entity = state->GetCharacterState();
  auto zone = zoneManager->GetCurrentZone(client);
  auto zoneDef = zone ? zone->GetDefinition() : nullptr;

  // Ignore spot triggers that are not for the current character and in the
  // correct zone or ones with no dynamic map loaded.
  if (entity->GetEntityID() != static_cast<int32_t>(entityID) || !zoneDef ||
      zoneDef->GetID() != zoneID) {
    return true;
  } else if (state->GetBikeBoosting()) {
    // Bike boosting players should not trigger spots
    return true;
  }

  auto dynamicMap = zone->GetDynamicMap();
  if (!dynamicMap) {
    LogGeneralError([zoneID, zoneDef]() {
      return libcomp::String(
                 "Dynamic map information could not be found for zone %1 with "
                 "dynamic map ID %2.\n")
          .Arg(zoneID)
          .Arg(zoneDef->GetDynamicMapID());
    });

    return true;
  }

  auto spotIter = dynamicMap->Spots.find(spotID);
  if (spotIter == dynamicMap->Spots.end()) {
    LogGeneralError([spotID, zoneID]() {
      return libcomp::String("Invalid spot %1 sent for zone %2.\n")
          .Arg(spotID)
          .Arg(zoneID);
    });

    return true;
  }

  auto accountUID = state->GetAccountUID();
  bool entered =
      zoneManager->PointInPolygon(Point(x, y), spotIter->second->Vertices);

  // Lookup the spot and see if it has actions.
  auto spot = zoneDef->GetSpots(spotID);

  if (spot) {
    // Get the action list.
    auto pActionList = new ActionListB;
    pActionList->actions =
        entered ? spot->GetActions() : spot->GetLeaveActions();

    // There must be at least 1 action or we are wasting our time.
    if (pActionList->actions.empty()) {
      delete pActionList;

      LogGeneralDebug([entered, spotID, x, y, accountUID]() {
        return libcomp::String(
                   "Player %1 spot %2 @ (%3, %4) with no actions: %5\n")
            .Arg(entered ? "entered" : "exited")
            .Arg(spotID)
            .Arg(x)
            .Arg(y)
            .Arg(accountUID.ToString());
      });

      return true;
    }

    auto actionCount = pActionList->actions.size();
    LogGeneralDebug([entered, spotID, x, y, actionCount, accountUID]() {
      return libcomp::String(
                 "Player %1 spot %2 @ (%3, %4) with %5 action(s): %6\n")
          .Arg(entered ? "entered" : "exited")
          .Arg(spotID)
          .Arg(x)
          .Arg(y)
          .Arg(actionCount)
          .Arg(accountUID.ToString());
    });

    // Perform the action(s) in the list.
    server->QueueWork(
        [](const std::shared_ptr<ChannelServer>& serverWork,
           const std::shared_ptr<ChannelClientConnection> clientWork,
           gsl::owner<ActionListB*> pActionListWork) {
          serverWork->GetActionManager()->PerformActions(
              clientWork, pActionListWork->actions, 0);

          delete pActionListWork;
        },
        server, client, pActionList);
  } else {
    LogGeneralDebug([spotID, zoneID, accountUID]() {
      return libcomp::String(
                 "Undefined spot %1 in zone %2 triggered by player: %3\n")
          .Arg(spotID)
          .Arg(zoneID)
          .Arg(accountUID.ToString());
    });
  }

  return true;
}
