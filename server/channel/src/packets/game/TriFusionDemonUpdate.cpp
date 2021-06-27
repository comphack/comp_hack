/**
 * @file server/channel/src/packets/game/TriFusionDemonUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to set the demons involved in a tri-fusion.
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
#include <ItemBox.h>
#include <TriFusionHostSession.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "FusionManager.h"
#include "ManagerConnection.h"
#include "Prefecture.h"

using namespace channel;

bool Parsers::TriFusionDemonUpdate::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 40) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto tfSession = std::dynamic_pointer_cast<objects::TriFusionHostSession>(
      state->GetExchangeSession());

  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto characterManager = server->GetCharacterManager();
  auto fusionManager = server->GetFusionManager();
  auto managerConnection = server->GetManagerConnection();

  libcomp::Packet reply;
  reply.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_TRIFUSION_DEMON_UPDATE);

  // Double back and write the status later
  reply.WriteS8(0);

  libcomp::Packet notify;
  notify.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_TRIFUSION_DEMON_UPDATED);

  bool failure = tfSession == nullptr;
  if (tfSession) {
    std::set<int32_t> participantIDs;
    std::unordered_map<int32_t, int8_t> inventoryFree;
    participantIDs.insert(tfSession->GetSourceEntityID());
    for (auto pState : tfSession->GetGuests()) {
      participantIDs.insert(pState->GetEntityID());
      inventoryFree[pState->GetEntityID()] = -1;
    }

    std::unordered_map<uint32_t, std::shared_ptr<objects::Demon>> dMap;

    int64_t demonIDs[3];
    for (size_t i = 0; i < 3; i++) {
      int8_t unknown = p.ReadS8();
      int32_t ownerEntityID = p.ReadS32Little();
      int64_t demonID = p.ReadS64Little();

      demonIDs[i] = demonID;

      notify.WriteS8(unknown);
      notify.WriteS32Little(ownerEntityID);

      if (participantIDs.find(ownerEntityID) == participantIDs.end()) {
        LogGeneralErrorMsg(
            "Received TriFusion demon update request for an invalid "
            "demon/character pair\n");

        failure = true;
        break;
      }

      auto demonUID = state->GetObjectUUID(demonID);
      auto demon = std::dynamic_pointer_cast<objects::Demon>(
          libcomp::PersistentObject::GetObjectByUUID(demonUID));
      if (demon) {
        tfSession->SetDemons(i, demon);

        reply.WriteS32Little(ownerEntityID);

        // Gather how many free slots the player's inventory has
        // to ensure they have space for rewards
        int8_t freeSlots = inventoryFree[ownerEntityID];
        if (freeSlots == -1) {
          auto pClient =
              managerConnection->GetEntityClient(ownerEntityID, false);
          freeSlots = (int8_t)characterManager->GetFreeSlots(pClient).size();
        }

        reply.WriteS8(freeSlots);

        dMap[notify.Size()] = demon;
        notify.WriteS64Little(0);
      } else {
        LogGeneralError([&]() {
          return libcomp::String(
                     "Invalid TriFusion demon update UID encountered: %1\n")
              .Arg(demonUID.ToString());
        });

        failure = true;
        break;
      }
    }

    uint32_t resultDemon = fusionManager->GetResultDemon(
        client, demonIDs[0], demonIDs[1], demonIDs[2]);
    notify.WriteU32Little(resultDemon);
    if (resultDemon == 0) {
      failure = true;
      tfSession->SetSelectionID(0);
    } else {
      tfSession->SetSelectionID(resultDemon);
    }

    if (!failure) {
      std::list<std::shared_ptr<ChannelClientConnection>> pClients;
      for (int32_t pID : participantIDs) {
        auto pClient = managerConnection->GetEntityClient(pID, false);
        if (pClient && client != pClient) {
          pClients.push_back(pClient);
        }
      }

      // Create a copy for each participant with local object IDs
      for (auto pClient : pClients) {
        auto pState = pClient->GetClientState();

        // Clear out any items if they exist from a cancelled
        // attempt
        auto pExchange = pState->GetExchangeSession();
        for (size_t i = 0; i < 4; i++) {
          pExchange->SetItems(i, NULLUUID);
        }

        libcomp::Packet nCopy(notify);
        for (auto dPair : dMap) {
          auto d = dPair.second;

          int64_t objID = pState->GetObjectID(d->GetUUID());
          if (objID <= 0) {
            objID = server->GetNextObjectID();
            pState->SetObjectID(d->GetUUID(), objID);
          }

          nCopy.Seek(dPair.first);
          nCopy.WriteS64Little(objID);
        }

        pClient->SendPacket(nCopy);
      }
    }
  }

  int8_t unknownByte = p.ReadS8();  // Always 1?
  (void)unknownByte;

  reply.Seek(2);
  reply.WriteS8(failure ? 1 : 0);

  client->SendPacket(reply);

  return true;
}
