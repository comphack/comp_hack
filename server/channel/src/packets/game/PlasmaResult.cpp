/**
 * @file server/channel/src/packets/game/PlasmaResult.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client containing the results of the
 *  plasma picking minigame.
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

// channel Includes
#include "ActionManager.h"
#include "ChannelServer.h"
#include "EventManager.h"
#include "PlasmaState.h"
#include "Prefecture.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::PlasmaResult::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 6) {
    return false;
  }

  int32_t plasmaID = p.ReadS32Little();
  int8_t pointID = p.ReadS8();
  int8_t result = p.ReadS8();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto zone = state->GetZone();

  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto eventManager = server->GetEventManager();
  auto zoneManager = server->GetZoneManager();

  auto pState =
      zone ? std::dynamic_pointer_cast<PlasmaState>(zone->GetEntity(plasmaID))
           : nullptr;

  auto point = pState ? pState->SetPickResult((uint32_t)pointID,
                                              state->GetWorldCID(), result)
                      : nullptr;

  bool failure = result < 0;

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_RESULT);
  reply.WriteS32Little(plasmaID);
  reply.WriteS8(pointID);
  reply.WriteS8(result);
  reply.WriteS32Little(failure ? 1 : (point ? 0 : -1));

  client->QueuePacket(reply);

  if (point) {
    // Send state to source player
    libcomp::Packet notify;
    pState->GetPointStatusData(notify, (uint32_t)pointID, state->GetWorldCID());
    client->QueuePacket(notify);

    // Send state to other players
    notify.Clear();
    pState->GetPointStatusData(notify, (uint32_t)pointID);
    zoneManager->BroadcastPacket(client, notify, false);
  }

  libcomp::Packet notify;
  notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_END);
  notify.WriteS32Little(plasmaID);
  notify.WriteS8(pointID);
  notify.WriteS32Little(failure ? 1 : 0);

  client->QueuePacket(notify);

  // End the system event
  eventManager->HandleEvent(client, nullptr);

  if (pState) {
    std::list<std::shared_ptr<objects::Action>> actions;
    if (!failure) {
      // Update demon quest if active
      eventManager->UpdateDemonQuestCount(
          client, objects::DemonQuest::Type_t::PLASMA,
          (uint32_t)pState->GetEntity()->GetColor(), 1);

      actions = pState->GetEntity()->GetSuccessActions();
    } else {
      actions = pState->GetEntity()->GetFailActions();
    }

    if (actions.size() > 0) {
      server->GetActionManager()->PerformActions(client, actions,
                                                 pState->GetEntityID(), zone);
    }
  }

  client->FlushOutgoing();

  return true;
}
