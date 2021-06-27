/**
 * @file server/world/src/ManagerConnection.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Manager to handle world connections to lobby and channel servers.
 *
 * This file is part of the World Server (world).
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

#include "ManagerConnection.h"

// libcomp Includes
#include <Log.h>
#include <MessageConnectionClosed.h>
#include <MessageEncrypted.h>
#include <PacketCodes.h>

// world Includes
#include "AccountManager.h"
#include "CharacterManager.h"
#include "WorldServer.h"
#include "WorldSyncManager.h"

using namespace world;

std::list<libcomp::Message::MessageType> ManagerConnection::sSupportedTypes = {
    libcomp::Message::MessageType::MESSAGE_TYPE_CONNECTION};

ManagerConnection::ManagerConnection(std::weak_ptr<libcomp::BaseServer> server)
    : mServer(server) {}

ManagerConnection::~ManagerConnection() {}

std::list<libcomp::Message::MessageType> ManagerConnection::GetSupportedTypes()
    const {
  return sSupportedTypes;
}

bool ManagerConnection::ProcessMessage(
    const libcomp::Message::Message *pMessage) {
  const libcomp::Message::ConnectionMessage *cMessage =
      dynamic_cast<const libcomp::Message::ConnectionMessage *>(pMessage);

  switch (cMessage->GetConnectionMessageType()) {
    case libcomp::Message::ConnectionMessageType::
        CONNECTION_MESSAGE_ENCRYPTED: {
      const libcomp::Message::Encrypted *encrypted =
          dynamic_cast<const libcomp::Message::Encrypted *>(cMessage);

      if (!LobbyConnected()) {
        auto connection = encrypted->GetConnection();

        /// @todo: verify this is in fact the lobby
        /// @todo: if it's not this reacts horribly
        /// @todo: if the lobby pointer is not set before a channel it will
        /// crash the world

        mLobbyConnection =
            std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);
      } else {
        // Nothing to do upon encrypting a channel connection (for now)
      }

      return true;
    } break;
    case libcomp::Message::ConnectionMessageType::
        CONNECTION_MESSAGE_CONNECTION_CLOSED: {
      const libcomp::Message::ConnectionClosed *closed =
          dynamic_cast<const libcomp::Message::ConnectionClosed *>(cMessage);

      auto connection = closed->GetConnection();

      auto server = mServer.lock();
      server->RemoveConnection(connection);

      if (mLobbyConnection == connection) {
        LogConnectionInfoMsg("Lobby connection closed. Shutting down.");

        server->Shutdown();
      } else {
        auto iConnection =
            std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);
        RemoveConnection(iConnection);
      }

      return true;
    } break;
    default:
      break;
  }

  return false;
}

std::shared_ptr<libcomp::InternalConnection>
ManagerConnection::GetLobbyConnection() {
  return mLobbyConnection;
}

bool ManagerConnection::LobbyConnected() { return nullptr != mLobbyConnection; }

void ManagerConnection::RemoveConnection(
    std::shared_ptr<libcomp::InternalConnection> &connection) {
  auto server = std::dynamic_pointer_cast<WorldServer>(mServer.lock());
  auto svr = server->GetChannel(connection);

  if (nullptr != svr) {
    server->RemoveChannel(connection);

    auto db = server->GetWorldDatabase();
    svr->Delete(db);

    auto accountManager = server->GetAccountManager();
    auto characterManager = server->GetCharacterManager();
    auto syncManager = server->GetWorldSyncManager();
    auto loggedOut = accountManager->LogoutUsersOnChannel((int8_t)svr->GetID());

    if (loggedOut.size() > 0) {
      LogConnectionWarning([&]() {
        return libcomp::String(
                   "%1 user(s) forcefully logged out from channel %2.\n")
            .Arg(loggedOut.size())
            .Arg(svr->GetID());
      });

      bool flushSyncData = false;
      std::list<std::shared_ptr<objects::CharacterLogin>> cLogOuts;
      for (auto logOut : loggedOut) {
        auto cLogin = logOut->GetCharacterLogin();

        characterManager->PartyLeave(cLogin, nullptr);
        characterManager->TeamLeave(cLogin);

        flushSyncData |=
            syncManager->CleanUpCharacterLogin(cLogin->GetWorldCID());

        cLogOuts.push_back(cLogin);
      }

      if (flushSyncData) {
        syncManager->SyncOutgoing();
      }

      characterManager->SendStatusToRelatedCharacters(
          cLogOuts, (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_BASIC);
    }

    std::list<std::shared_ptr<libcomp::TcpConnection>> connections;
    connections.push_back(mLobbyConnection);

    for (auto kv : server->GetChannels()) {
      if (kv.first != connection) {
        connections.push_back(kv.first);
      }
    }

    // Channel disconnected
    libcomp::Packet packet;
    packet.WritePacketCode(InternalPacketCode_t::PACKET_SET_CHANNEL_INFO);
    packet.WriteU8(to_underlying(InternalPacketAction_t::PACKET_ACTION_REMOVE));
    packet.WriteU8(svr->GetID());

    libcomp::TcpConnection::BroadcastPacket(connections, packet);
  }
}
